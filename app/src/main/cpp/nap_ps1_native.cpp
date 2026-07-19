// BUILD2SA2: PS1 libretro host - REALNY boot BIOS+hra, worker s retro_run,
// zachytavani snimku do bufferu, status s pocitadlem snimku. Obraz na
// TextureView je SA2b; zvuk SA3. Zadny fake - kdyz boot selze, status to prizna.
#include <jni.h>
#include <android/log.h>
#include <atomic>
#include <mutex>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <cstdio>
#include <cstdarg>
#include <dirent.h>
#include <cctype> // BUILD2SA6: BIOS audit
#include <cstdint>
#include <fstream>
#include <iterator>
#include <EGL/egl.h>   // BUILD2SK98: pro gpu-gles (skutecny GL vykreslovac s texturovym filtrovanim)
#include <GLES/gl.h>   // GLES1 - presne to, co gpu-gles pouziva (fixed-function pipeline)
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, "NAP_PS1", __VA_ARGS__)

extern "C" {
  // minimalni libretro API (shodne s libretro.h ABI)
  struct retro_system_info { const char *library_name, *library_version, *valid_extensions; unsigned char need_fullpath, block_extract; };
  struct retro_game_geometry { unsigned base_width, base_height, max_width, max_height; float aspect_ratio; };
  struct retro_system_timing { double fps, sample_rate; };
  struct retro_system_av_info { struct retro_game_geometry geometry; struct retro_system_timing timing; };
  struct retro_game_info { const char *path; const void *data; size_t size; const char *meta; };
  struct retro_variable { const char *key; const char *value; };
  struct retro_log_callback { void (*log)(int level, const char *fmt, ...); };
  typedef bool (*retro_environment_t)(unsigned cmd, void *data);
  typedef void (*retro_video_refresh_t)(const void *data, unsigned width, unsigned height, size_t pitch);
  typedef void (*retro_audio_sample_t)(int16_t left, int16_t right);
  typedef size_t (*retro_audio_sample_batch_t)(const int16_t *data, size_t frames);
  typedef void (*retro_input_poll_t)(void);
  typedef int16_t (*retro_input_state_t)(unsigned port, unsigned device, unsigned index, unsigned id);
  void retro_set_environment(retro_environment_t);
  void retro_set_video_refresh(retro_video_refresh_t);
  void retro_set_audio_sample(retro_audio_sample_t);
  void retro_set_audio_sample_batch(retro_audio_sample_batch_t);
  void retro_set_input_poll(retro_input_poll_t);
  void retro_set_input_state(retro_input_state_t);
  void retro_init(void);
  void retro_deinit(void);
  unsigned retro_api_version(void);
  void retro_get_system_info(struct retro_system_info*);
  void retro_get_system_av_info(struct retro_system_av_info*);
  bool retro_load_game(const struct retro_game_info*);
  void retro_unload_game(void);
  void retro_run(void);
  void *retro_get_memory_data(unsigned id);
  size_t retro_get_memory_size(unsigned id);
  size_t retro_serialize_size(void);
  bool retro_serialize(void *data, size_t size);
  bool retro_unserialize(const void *data, size_t size);
}
#define ENV_SET_PIXEL_FORMAT 10
#define ENV_GET_SYSTEM_DIRECTORY 9
#define ENV_GET_SAVE_DIRECTORY 31
#define ENV_GET_CAN_DUPE 3
#define ENV_GET_LOG_INTERFACE 27
#define ENV_GET_VARIABLE 15
#define ENV_GET_VARIABLE_UPDATE 17
#define PIXFMT_0RGB1555 0
#define PIXFMT_XRGB8888 1
#define PIXFMT_RGB565 2

static std::string g_sysdir, g_savedir, g_boot_error;
static std::atomic<int> g_pixfmt{PIXFMT_0RGB1555};
static std::atomic<bool> g_running{false};
static std::atomic<int> g_generation{0};
static std::atomic<uint64_t> g_frames{0}, g_dupe_frames{0}, g_audio_samples_dropped{0}, g_audio_resyncs{0};
static std::atomic<int> g_fw{0}, g_fh{0};
static std::atomic<uint32_t> g_input_bits{0}; // BUILD2SA4: libretro joypad bits
static std::mutex g_frame_mutex;
static std::vector<uint32_t> g_frame_argb; // SA2b si tenhle buffer vyzvedne pro TextureView
static std::thread g_worker;
static std::mutex g_life_mutex;
static std::mutex g_core_mutex; // BUILD2SA5: save/load state nesmi bezet soucasne s retro_run
static double g_fps = 60.0;
static std::atomic<bool> g_loaded{false};
// BUILD2SA11: MEMORY CARD per hra
static std::string g_srm_path;
static uint32_t g_srm_last_fnv = 0;
static uint32_t nap_fnv32(const uint8_t *d, size_t n) {
  uint32_t h = 2166136261u;
  for (size_t i = 0; i < n; ++i) { h ^= d[i]; h *= 16777619u; }
  return h;
}
static void nap_srm_set_path(const std::string &gamePath) {
  std::string leaf = gamePath;
  size_t sl = leaf.find_last_of('/');
  if (sl != std::string::npos) leaf = leaf.substr(sl + 1);
  if (leaf.empty() || gamePath.rfind("/proc/self/fd/", 0) == 0) leaf = "rucni_vyber";
  size_t dot = leaf.find_last_of('.');
  if (dot != std::string::npos && dot > 0) leaf = leaf.substr(0, dot);
  for (size_t i = 0; i < leaf.size(); ++i) { char c = leaf[i]; if (!isalnum((unsigned char)c) && c != '-' && c != '_') leaf[i] = '_'; }
  g_srm_path = g_savedir + "/" + leaf + ".srm";
}
static void nap_srm_load() {
  void *mem = retro_get_memory_data(0);
  size_t sz = retro_get_memory_size(0);
  if (!mem || !sz) { NAPLOG("BUILD2SA11 MEMCARD_NONE core nedava SAVE_RAM"); return; }
  FILE *f = fopen(g_srm_path.c_str(), "rb");
  if (f) { size_t rd = fread(mem, 1, sz, f); fclose(f);
    NAPLOG("BUILD2SA11 MEMCARD_LOADED %s bytes=%zu/%zu", g_srm_path.c_str(), rd, sz);
  } else NAPLOG("BUILD2SA11 MEMCARD_NEW %s size=%zu (prvni hrani teto hry)", g_srm_path.c_str(), sz);
  g_srm_last_fnv = nap_fnv32((const uint8_t*)mem, sz);
}
static void nap_srm_save_if_dirty(const char *why) {
  void *mem = retro_get_memory_data(0);
  size_t sz = retro_get_memory_size(0);
  if (!mem || !sz || g_srm_path.empty()) return;
  uint32_t h = nap_fnv32((const uint8_t*)mem, sz);
  if (h == g_srm_last_fnv) return;
  FILE *f = fopen(g_srm_path.c_str(), "wb");
  if (!f) { NAPLOG("BUILD2SA11 MEMCARD_SAVE_FAIL %s", g_srm_path.c_str()); return; }
  fwrite(mem, 1, sz, f); fclose(f);
  g_srm_last_fnv = h;
  NAPLOG("BUILD2SA11 MEMCARD_SAVED %s bytes=%zu why=%s", g_srm_path.c_str(), sz, why);
}

static void nap_retro_log(int level, const char *fmt, ...) {
  char buf[512]; va_list ap; va_start(ap, fmt); vsnprintf(buf, sizeof(buf), fmt, ap); va_end(ap);
  __android_log_print(ANDROID_LOG_INFO, "NAP_PS1_CORE", "[%d] %s", level, buf);
}
static const char *nap_core_option_value(const char *key) {
  if (!key) return nullptr;
  if (strcmp(key, "pcsx_rearmed_bios") == 0) return "auto";
  if (strcmp(key, "pcsx_rearmed_show_bios_bootlogo") == 0) return "enabled";
  if (strcmp(key, "pcsx_rearmed_region") == 0) return "auto";
  if (strcmp(key, "pcsx_rearmed_drc") == 0) return "enabled";
  if (strcmp(key, "pcsx_rearmed_async_cd") == 0) return "async";
  // BUILD2SK94: REVERT (puvodni pokus) - podezreni na souvislost s rostoucimi
  // audio underruny. Nikdy to nebylo jiste - jen casova shoda.
  // BUILD2SK96: ZNOVU ZAPNUTO, izolovane. SK95 nasel a opravil SKUTECNOU
  // pricinu audio problemu (unik pameti v NativePs1InPlaceView - chybejici
  // bitmap.recycle() pri zmene PS1 rozliseni) - Rene potvrdil zvuk uz v
  // poradku, na telefonu i v prenosu. SK94uv podezreni na Enhanced
  // Resolution tedy nejspis bylo vedle - jen nahodna casova shoda se
  // skutecnym unikem pameti, ktery bezel soubezne. Rene poslal screenshoty
  // potvrzujici src=320x240 (nativni rozliseni teto konkretni hry/obsahu) -
  // presne scenar, pro ktery je Enhanced Resolution urcene (zdvojnasobi 3D
  // geometrii, ne 2D/FMV - cutsceny se timhle NEZLEPSI, to je jiny, trvaly
  // strop, viz PS1 rozliseni diskuze).
  if (strcmp(key, "pcsx_rearmed_neon_enhancement_enable") == 0) return "enabled";
  return nullptr;
}
static bool nap_env(unsigned cmd, void *data) {
  switch (cmd) {
    case ENV_SET_PIXEL_FORMAT: { g_pixfmt.store(*(const int*)data); return true; }
    case ENV_GET_SYSTEM_DIRECTORY: { *(const char**)data = g_sysdir.c_str(); return true; }
    case ENV_GET_SAVE_DIRECTORY: { *(const char**)data = g_savedir.c_str(); return true; }
    case ENV_GET_CAN_DUPE: { *(bool*)data = true; return true; }
    case ENV_GET_LOG_INTERFACE: { ((retro_log_callback*)data)->log = nap_retro_log; return true; }
    case ENV_GET_VARIABLE: {
      retro_variable *var = (retro_variable*)data;
      const char *value = var ? nap_core_option_value(var->key) : nullptr;
      if (!value) { if (var) var->value = nullptr; return false; }
      var->value = value;
      return true;
    }
    case ENV_GET_VARIABLE_UPDATE: { *(bool*)data = false; return true; }
    default: return false;
  }
}
static void nap_video(const void *data, unsigned w, unsigned h, size_t pitch) {
  if (!data) { g_dupe_frames.fetch_add(1); g_frames.fetch_add(1); return; }
  std::lock_guard<std::mutex> lock(g_frame_mutex);
  g_frame_argb.resize((size_t)w * h);
  const int fmt = g_pixfmt.load();
  for (unsigned y = 0; y < h; ++y) {
    const uint8_t *row = (const uint8_t*)data + y * pitch;
    uint32_t *dst = g_frame_argb.data() + (size_t)y * w;
    if (fmt == PIXFMT_RGB565) {
      const uint16_t *s = (const uint16_t*)row;
      for (unsigned x = 0; x < w; ++x) { uint16_t c = s[x];
        uint32_t r=(c>>11)&31,g=(c>>5)&63,b=c&31;
        dst[x]=0xFF000000u|((r<<3|r>>2)<<16)|((g<<2|g>>4)<<8)|(b<<3|b>>2); }
    } else if (fmt == PIXFMT_XRGB8888) {
      const uint32_t *s = (const uint32_t*)row;
      for (unsigned x = 0; x < w; ++x) dst[x] = 0xFF000000u | s[x];
    } else { // 0RGB1555
      const uint16_t *s = (const uint16_t*)row;
      for (unsigned x = 0; x < w; ++x) { uint16_t c = s[x];
        uint32_t r=(c>>10)&31,g=(c>>5)&31,b=c&31;
        dst[x]=0xFF000000u|((r<<3|r>>2)<<16)|((g<<3|g>>2)<<8)|(b<<3|b>>2); }
    }
  }
  g_fw.store((int)w); g_fh.store((int)h);
  g_frames.fetch_add(1);
}
// BUILD2SA3/SA5P: audio FIFO (44100 Hz stereo z jadra -> Java AudioTrack)
static std::mutex g_amutex;
static std::vector<int16_t> g_afifo; // interleaved L,R
static const size_t NAP_PS1_AFIFO_TARGET_FRAMES = 735 * 8;  // ~133 ms at 44.1 kHz
static const size_t NAP_PS1_AFIFO_MAX_FRAMES = 735 * 24;    // ~400 ms, jen runaway resync
static void nap_audio_trim_locked(size_t targetFrames) {
  const size_t targetShorts = targetFrames * 2;
  if (g_afifo.size() <= targetShorts) return;
  const size_t dropShorts = g_afifo.size() - targetShorts;
  g_audio_samples_dropped.fetch_add(dropShorts / 2);
  g_audio_resyncs.fetch_add(1);
  g_afifo.erase(g_afifo.begin(), g_afifo.begin() + dropShorts);
}
static void nap_audio_clear(void) {
  std::lock_guard<std::mutex> lock(g_amutex);
  g_afifo.clear();
}
static void nap_audio_push(const int16_t *data, size_t frames) {
  if (!data || !frames) return;
  std::lock_guard<std::mutex> lock(g_amutex);
  g_afifo.insert(g_afifo.end(), data, data + frames * 2);
  if (g_afifo.size() / 2 > NAP_PS1_AFIFO_MAX_FRAMES) nap_audio_trim_locked(NAP_PS1_AFIFO_TARGET_FRAMES);
}
static void nap_audio_sample(int16_t l, int16_t r) { int16_t s[2] = { l, r }; nap_audio_push(s, 1); }
static size_t nap_audio_batch(const int16_t *data, size_t frames) { nap_audio_push(data, frames); return frames; }
static void nap_input_poll(void) {}
static int16_t nap_input_state(unsigned port, unsigned device, unsigned, unsigned id) {
  // BUILD2SA4: RetroPad -> PS1 core. device 1 = RETRO_DEVICE_JOYPAD.
  if (port != 0 || device != 1 || id >= 16) return 0;
  return (g_input_bits.load(std::memory_order_relaxed) & (1u << id)) ? 1 : 0;
}

// BUILD2SK98: gpu-gles potrebuje EXTERNE dodany EGL display/surface (jinak by
// zkousel svou vlastni X11-specifickou cestu, ktera na Androidu nikdy nemuze
// fungovat) - GLinitialize(display, surface) tohle prijima primo. Viz
// vendor/pcsx_rearmed/plugins/gpu-gles/gpuDraw.c GLinitialize().
extern "C" {
  typedef struct { int left, top, right, bottom; } NapGlesRectShape; // ABI-kompatibilni s vendor RECT
  extern int iResX;
  extern int iResY;
  extern NapGlesRectShape rRatioRect;
  int GLinitialize(void *ext_gles_display, void *ext_gles_surface);
  void InitializeTextureStore(); // BUILD2SK102: viz gpuTexture.c - alokuje texture-cache buffery
  void MakeDisplayLists(); // BUILD2SK102: viz hud.c - font/HUD display listy
  void updateFrontDisplay(void); // BUILD2SK103: viz gpulib_if.c - skutecne swapne+odesle snimek
  void SetOGLDisplaySettings(int DisplaySet); // BUILD2SK106: nastavi GL scissor/clip - viz gpuDraw.c
  // BUILD2SK106: POZOR presne typy - BOOL je v tomhle projektu #define BOOL
  // unsigned short (NE int - to by byla skutecna chyba, cteni/zapis 4 bajtu
  // pres 2-bajtovou promennou). Overeno primo v gpuExternals.h pred pouzitim.
  extern unsigned short bDisplayNotSet; // BUILD2SK106
  extern unsigned short bSetClip; // BUILD2SK106
  extern unsigned int CSTEXTURE, CSVERTEX, CSCOLOR; // BUILD2SK106
}

static bool g_gles_ready = false;
static std::mutex g_diag_log_mutex;
static std::string g_diag_log_path;

// BUILD2SK99: primy, OKAMZITY zapis na disk - zadna fronta, zadne bufferovani.
// Kazde volani otevre soubor, zapise radek, hned zavre (fclose implicitne
// flushuje) - i kdyby appka spadla o zlomek vteriny pozdeji, tenhle radek uz
// je na disku. Pise do STEJNEHO souboru jako Java appendNativeLog (cesta
// dorazi pres ps1SetDiagLogPath, volano z onCreate) - Rene ho tak uvidi v
// tom samem /log, na ktery je zvykly, zadny novy soubor/mechanismus pro nej.
extern "C" void nap_diag_log(const char *fmt, ...) {
  std::string path;
  { std::lock_guard<std::mutex> lock(g_diag_log_mutex); path = g_diag_log_path; }
  if (path.empty()) { return; } // cesta jeste nedorazila z Javy - nic bezpecneho k zapisu
  char msg[512];
  va_list ap; va_start(ap, fmt); vsnprintf(msg, sizeof(msg), fmt, ap); va_end(ap);
  FILE *f = fopen(path.c_str(), "a");
  if (!f) return;
  fprintf(f, "%s\n", msg);
  fclose(f); // BUILD2SK99: fclose flushuje - zadny extra fflush/fsync potreba
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(JNIEnv *env, jclass, jstring jpath) {
  const char *p = env->GetStringUTFChars(jpath, nullptr);
  {
    std::lock_guard<std::mutex> lock(g_diag_log_mutex);
    g_diag_log_path = p ? p : "";
  }
  env->ReleaseStringUTFChars(jpath, p);
}
// BUILD2SK99: obojí najednou - logcat (kdyby nekdy byl adb pristup) i durable
// soubor (co Rene skutecne vidi). Jen pro GLES diagnostiku, proto lokalni
// makro tady a ne nahore v souboru (tam by nap_diag_log jeste nebyla znama).
#define NAPDIAG(...) do { NAPLOG(__VA_ARGS__); nap_diag_log(__VA_ARGS__); } while(0)


// BUILD2SK98: cistě offscreen (pbuffer) EGL kontext - ZADNY Android Surface/
// Window potreba vubec, takze zadny konflikt s existujici NativePs1InPlaceView
// TextureView (ktera pořad kresli pres Canvas jako driv - nedotcena). gpu-gles
// renderuje SEM, my si to pak precteme zpet (viz gpulib_if.c
// nap_gles_readback_and_push) a posleme presne tou samou cestou, jakou uz
// pouzival gpu_neon. Kazdy krok kontroluje navratovou hodnotu - pri selhani
// se cistě vrati false (zadny dalsi GL kod se nespusti), PS1 by zustala
// bez obrazu (cerna/zadna), ale appka by NEMELA spadnout.
static bool nap_gles_egl_init() {
  NAPDIAG("BUILD2SK99 GLES_INIT_ENTER"); // BUILD2SK99: kanarek - pokud tohle v logu je, ale nic dal, vime, ze crash je HNED za timhle radkem
  EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  if (display == EGL_NO_DISPLAY) { NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglGetDisplay"); return false; }

  EGLint majorV = 0, minorV = 0;
  if (!eglInitialize(display, &majorV, &minorV)) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglInitialize err=0x%x", eglGetError());
    return false;
  }
  NAPDIAG("BUILD2SK98 GLES_INIT egl version=%d.%d", (int)majorV, (int)minorV);

  const EGLint configAttribs[] = {
    EGL_SURFACE_TYPE, EGL_PBUFFER_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, // GLES1
    EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_NONE
  };
  EGLConfig config;
  EGLint numConfigs = 0;
  if (!eglChooseConfig(display, configAttribs, &config, 1, &numConfigs) || numConfigs < 1) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglChooseConfig err=0x%x", eglGetError());
    return false;
  }

  // BUILD2SK98: velkorysa velikost, at se vejde jakekoli PS1 rozliseni videne
  // v logu (256x240 az 640x480). ZNAMY LIMIT prvniho pokusu: promitaci
  // matice (glOrtho v GLinitialize) se nastavi JEDNOU pri startu podle
  // tehdejsiho PSXDisplay.DisplayMode - pokud hra pozdeji za behu prepne na
  // jine rozliseni, projekce se sama znovu nenastavi (dalsi krok pro
  // budoucnost, ne tenhle prvni pokus).
  const EGLint pbufferAttribs[] = { EGL_WIDTH, 1024, EGL_HEIGHT, 768, EGL_NONE };
  EGLSurface surface = eglCreatePbufferSurface(display, config, pbufferAttribs);
  if (surface == EGL_NO_SURFACE) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglCreatePbufferSurface err=0x%x", eglGetError());
    return false;
  }

  const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 1, EGL_NONE }; // GLES1
  EGLContext context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
  if (context == EGL_NO_CONTEXT) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglCreateContext err=0x%x", eglGetError());
    return false;
  }

  if (!eglMakeCurrent(display, surface, surface, context)) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglMakeCurrent err=0x%x", eglGetError());
    return false;
  }

  // BUILD2SK98: gpu-gles cte tyhle tri promenne PRED tim, nez se GLinitialize
  // vubec pusti do glViewport/glOrtho - musi byt nastavene driv. 320x240
  // odpovida vychozimu PSXDisplay.DisplayMode nastavenemu v renderer_init().
  iResX = 320;
  iResY = 240;
  rRatioRect.left = 0; rRatioRect.top = 0; rRatioRect.right = 320; rRatioRect.bottom = 240;

  // BUILD2SK106: DALSI CHYBEJICI KUS z GPUopen() - vsimnul jsem si az pri
  // znovu-kontrole na Reneho vyslovnou zadost. GPUopen() krome
  // InitializeTextureStore/GLinitialize/MakeDisplayLists TAKE nastavuje
  // tyhle tri stavove promenne. bSetClip=TRUE je obzvlast dulezite - je to
  // spoustec pro SetOGLDisplaySettings(), ktera nastavuje OpenGL scissor
  // (orezavaci) obdelnik pres glScissor(). Bez tohohle zustava scissor na
  // cemkoli, na cem GL kontext zrovna byl - coz muze v praxi znamenat, ze
  // se VYCISTENI pozadi jeste nejak projevi, ale SAMOTNA GEOMETRIE hry se
  // ořeže pryc (proto jsme videli jednolitou barvu, zadnou skutecnou
  // grafiku - presne Reneho postreh). GPUopen() spoleha na to, ze
  // bDisplayNotSet pak zachyti updateDisplay() (nas "interlaced" vetev,
  // kterou nepouzivame - viz SK103) - takze navic explicitne volame
  // SetOGLDisplaySettings(1) sami, rovnou tady, aby se scissor/viewport
  // spravne nastavily uz od prvniho snimku, bez ohledu na to, kterou
  // update-funkci pak volame kazdy tick.
  bDisplayNotSet = TRUE;
  bSetClip = TRUE;
  CSTEXTURE = CSVERTEX = CSCOLOR = 0;

  // BUILD2SK102: SKUTECNA PRICINA PADU (potvrzeno realnym adb crash logem -
  // Rene ho ziskal, dik!). Fatal SIGSEGV v CheckTextureInSubSCache (volano
  // pres SelectSubTextureS<-SetRenderMode<-primPolyFT4 - prvni TEXTUROVANY
  // polygon, co se hra pokusi nakreslit). Pricina: gpu-gles ma VLASTNI,
  // kompletni inicializacni vstupni bod (GPUopen v gpulib_if.c), ktery VZDY
  // vola InitializeTextureStore() PRED GLinitialize() a MakeDisplayLists()
  // PO nem - alokuje pscSubtexStore a dalsi texture-cache buffery, ktere
  // CheckTextureInSubSCache bez kontroly rovnou cte. My ale GPUopen() nikdy
  // nevolame (misto toho volame GLinitialize primo sami) - a jediny init
  // hook, ktery nas libretro tok SKUTECNE vola (renderer_init() v tomhle
  // souboru) NIC z tohohle nedela, jen nastavi par PSXDisplay poli. Presne
  // proto pscSubtexStore zustal na NULL (vychozi C inicializace globalniho
  // pole) az do prvniho pokusu o kresleni. Oprava: zavolat oboji rucne, ve
  // STEJNEM poradi, jake uz proverene pouziva GPUopen().
  InitializeTextureStore();

  if (GLinitialize((void *)display, (void *)surface) != 0) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=GLinitialize");
    return false;
  }
  MakeDisplayLists(); // BUILD2SK102: stejne poradi jako GPUopen() - font/HUD display listy
  SetOGLDisplaySettings(1); // BUILD2SK106: viz vysvetleni vyse - nastavi scissor/viewport hned
  {
    GLint scissorBox[4] = {0,0,0,0};
    glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
    NAPDIAG("BUILD2SK106 GLES_SCISSOR_CHECK x=%d y=%d w=%d h=%d", scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
  }

  NAPDIAG("BUILD2SK98 GLES_INIT_OK pbuffer=1024x768 initial=320x240");
  return true;
}

static void nap_worker(int gen) {
  NAPLOG("BUILD2SA2 PS1 worker start gen=%d fps=%.2f", gen, g_fps);
  NAPDIAG("BUILD2SK99 PS1_WORKER_THREAD_ALIVE gen=%d", gen); // BUILD2SK99: durable kanarek - vlakno samo bezi
  // BUILD2SK98: EGL kontext MUSI se nastavit na TOMHLE vlakne (jedine vlakno,
  // ktere kdy vola retro_run(), tedy jedine vlakno, na kterem gpu-gles vubec
  // kresli) - GL kontexty jsou vazane na vlakno, ktere je aktivovalo.
  g_gles_ready = nap_gles_egl_init();
  if (!g_gles_ready) {
    NAPDIAG("BUILD2SK98 PS1 worker pokracuje BEZ funkcniho GL kontextu - video pravdepodobne cerne, zvuk/vstup nedotcene");
  }
  const auto period = std::chrono::nanoseconds((long long)(1e9 / (g_fps > 1 ? g_fps : 60.0)));
  auto next = std::chrono::steady_clock::now();
  uint64_t srmTick = 0;
  while (g_running.load() && gen == g_generation.load()) {
    {
      std::lock_guard<std::mutex> core(g_core_mutex);
      retro_run();
      // BUILD2SK103: gpu-gles normalne ceka na vout_update() (z gpu.c), ktera
      // ale sama vola updateDisplay/updateFrontDisplay JEN kdyz je bud
      // PSXDisplay.Interlaced=true (spravne neni - normalni progresivni PS1
      // stav), NEBO bRenderFrontBuffer=true (strukturalne blokovano -
      // podminene iOffscreenDrawing==4, a to je v GPUopen() natvrdo 0,
      // konfiguracni volba z puvodniho ne-libretro frontendu, kterou jsme
      // nikdy nezapojili). Bez tohohle appka jela bez padu, ale NIKDY
      // nedoslo ke skutecnemu swap+odeslani snimku - proto jen zvuk, zadny
      // obraz. Reseni: zavolat primo, obchazi vout_update() uplne. Vlastni
      // iDrawnSomething kontrola uvnitr updateFrontDisplay() porad spravne
      // presskoci swap, kdyz se nic noveho nenakreslilo - zadna zbytecna
      // prace navic.
      if (g_gles_ready) { updateFrontDisplay(); }
      if (++srmTick % 300 == 0) nap_srm_save_if_dirty("periodic");
    }
    next += period;
    auto now = std::chrono::steady_clock::now();
    if (next > now) std::this_thread::sleep_for(next - now); else next = now;
  }
  NAPLOG("BUILD2SA2 PS1 worker stop gen=%d frames=%llu", gen, (unsigned long long)g_frames.load());
}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1CoreInfo(JNIEnv *env, jclass) {
  char buf[512]; retro_system_info si; memset(&si,0,sizeof(si)); retro_get_system_info(&si);
  snprintf(buf,sizeof(buf),"PS1_CORE_COMPILED name=%s version=%s api=%u ext=%s dynarec=INTERPRETER boot=SA2_READY",
    si.library_name?si.library_name:"?", si.library_version?si.library_version:"?", retro_api_version(), si.valid_extensions?si.valid_extensions:"?");
  return env->NewStringUTF(buf);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Boot(JNIEnv *env, jclass, jstring jsys, jstring jsave, jstring jgame) {
  std::lock_guard<std::mutex> life(g_life_mutex);
  char out[768];
  const char *sys = env->GetStringUTFChars(jsys, nullptr);
  const char *sav = env->GetStringUTFChars(jsave, nullptr);
  const char *game = env->GetStringUTFChars(jgame, nullptr);
  std::string gamePath = game ? game : "";
  g_sysdir = sys ? sys : ""; g_savedir = sav ? sav : "";
  env->ReleaseStringUTFChars(jsys, sys); env->ReleaseStringUTFChars(jsave, sav); env->ReleaseStringUTFChars(jgame, game);
  // stop pripadneho predchoziho behu
  g_generation.fetch_add(1); g_running.store(false);
  if (g_worker.joinable()) g_worker.join();
  if (g_loaded.exchange(false)) { nap_srm_save_if_dirty("stop"); retro_unload_game(); retro_deinit(); }
  nap_audio_clear();
  g_input_bits.store(0);
  g_frames.store(0); g_dupe_frames.store(0); g_audio_samples_dropped.store(0); g_audio_resyncs.store(0); g_fw.store(0); g_fh.store(0);
  g_boot_error.clear();
  retro_set_environment(nap_env);
  retro_set_video_refresh(nap_video);
  retro_set_audio_sample(nap_audio_sample);
  retro_set_audio_sample_batch(nap_audio_batch);
  retro_set_input_poll(nap_input_poll);
  retro_set_input_state(nap_input_state);
  retro_init();
  retro_game_info gi; memset(&gi,0,sizeof(gi));
  gi.path = gamePath.c_str();
  bool ok = retro_load_game(&gi);
  if (!ok) {
    g_boot_error = "retro_load_game FAILED path=" + gamePath;
    retro_deinit();
    snprintf(out,sizeof(out),"PS1_BOOT_FAIL %s (zkontroluj BIOS v systemove slozce a cestu ke hre)", g_boot_error.c_str());
    NAPLOG("%s", out);
    return env->NewStringUTF(out);
  }
  g_loaded.store(true);
  nap_srm_set_path(gamePath); // BUILD2SA11
  nap_srm_load();
  retro_system_av_info av; memset(&av,0,sizeof(av));
  retro_get_system_av_info(&av);
  g_fps = av.timing.fps > 1 ? av.timing.fps : 60.0;
  const int gen = g_generation.load();
  g_running.store(true);
  g_worker = std::thread(nap_worker, gen);
  char biosList[256] = "prazdna";
  { // BUILD2SA6: BIOS AUDIT - vypis, co jadro REALNE vidi v system slozce.
    // Kdyz tu nebude scph1001.bin, mame pricinu chybejici SONY znelky (HLE BIOS).
    DIR *d = opendir(g_sysdir.c_str());
    if (d) { size_t off = 0; biosList[0] = 0; struct dirent *e;
      while ((e = readdir(d)) && off < sizeof(biosList) - 40) {
        if (e->d_name[0] == '.') continue;
        off += snprintf(biosList + off, sizeof(biosList) - off, "%s%s", off ? "," : "", e->d_name);
      }
      closedir(d); if (!biosList[0]) snprintf(biosList, sizeof(biosList), "prazdna");
    }
  }
  snprintf(out,sizeof(out),"PS1_BOOT_OK sysdirFiles=[%s] path=%s fps=%.2f baseRes=%ux%u sampleRate=%.0f sysdir=%s",
    biosList, gamePath.c_str(), av.timing.fps, av.geometry.base_width, av.geometry.base_height, av.timing.sample_rate, g_sysdir.c_str());
  NAPLOG("%s", out);
  return env->NewStringUTF(out);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Status(JNIEnv *env, jclass) {
  char out[768];
  snprintf(out,sizeof(out),"PS1_RUN running=%s frames=%llu dupes=%llu res=%dx%d pixfmt=%d audioFifoFrames=%zu audioDropped=%llu audioResyncs=%llu fps=%.2f err=%s",
    g_running.load()?"YES":"NO",(unsigned long long)g_frames.load(),(unsigned long long)g_dupe_frames.load(),
    g_fw.load(),g_fh.load(),g_pixfmt.load(),({std::lock_guard<std::mutex> al(g_amutex); g_afifo.size()/2;}),(unsigned long long)g_audio_samples_dropped.load(),(unsigned long long)g_audio_resyncs.load(),g_fps,
    g_boot_error.empty()?"none":g_boot_error.c_str());
  return env->NewStringUTF(out);
}
// BUILD2SA2B: vyzvednuti snimku pro nahled (a pozdeji TextureView). Vraci (w<<16)|h, 0 = nic.
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1GrabFrame(JNIEnv *env, jclass, jintArray out) {
  static int nap_grabframe_calls = 0; // BUILD2SK101: durable tep - viz komentar vyse
  nap_grabframe_calls++;
  if (nap_grabframe_calls % 30 == 1) {
    nap_diag_log("BUILD2SK101 PS1_GRABFRAME_HEARTBEAT n=%d gfw=%d gfh=%d", nap_grabframe_calls, g_fw.load(), g_fh.load());
  }
  std::lock_guard<std::mutex> lock(g_frame_mutex);
  const int w = g_fw.load(), h = g_fh.load();
  if (w <= 0 || h <= 0 || g_frame_argb.size() < (size_t)w * h || !out) return 0;
  jsize cap = env->GetArrayLength(out);
  if (cap < w * h) return -(w << 16 | h); // buffer maly - Java si ho zvetsi
  env->SetIntArrayRegion(out, 0, w * h, (const jint*)g_frame_argb.data());
  return (w << 16) | h;
}
// BUILD2SA3: Java audio vlakno si tahne stereo framy; vraci pocet framu.
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PullAudio(JNIEnv *env, jclass, jshortArray out, jint frames) {
  if (!out || frames <= 0) return 0;
  std::lock_guard<std::mutex> lock(g_amutex);
  size_t have = g_afifo.size() / 2;
  if (have > NAP_PS1_AFIFO_MAX_FRAMES) {
    nap_audio_trim_locked(NAP_PS1_AFIFO_TARGET_FRAMES);
    have = g_afifo.size() / 2;
  }
  const size_t minPull = 128; // BUILD2SA5P: avoid tiny crumbs, but keep natural small PS1 audio batches flowing.
  if (have < minPull) return 0;
  size_t n = have < (size_t)frames ? have : (size_t)frames;
  if (!n) return 0;
  env->SetShortArrayRegion(out, 0, (jsize)(n * 2), (const jshort*)g_afifo.data());
  g_afifo.erase(g_afifo.begin(), g_afifo.begin() + n * 2);
  return (jint)n;
}
extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetInput(JNIEnv *, jclass, jint id, jboolean down) {
  if (id < 0 || id >= 16) return;
  const uint32_t bit = 1u << (uint32_t)id;
  if (down) g_input_bits.fetch_or(bit, std::memory_order_relaxed);
  else g_input_bits.fetch_and(~bit, std::memory_order_relaxed);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SaveState(JNIEnv *env, jclass, jstring jpath) {
  if (!jpath) return env->NewStringUTF("PS1_STATE_SAVE_FAIL path=null");
  if (!g_loaded.load()) return env->NewStringUTF("PS1_STATE_SAVE_FAIL core_not_loaded");
  const char *pathChars = env->GetStringUTFChars(jpath, nullptr);
  std::string path = pathChars ? pathChars : "";
  env->ReleaseStringUTFChars(jpath, pathChars);
  if (path.empty()) return env->NewStringUTF("PS1_STATE_SAVE_FAIL path_empty");
  std::vector<uint8_t> data;
  {
    std::lock_guard<std::mutex> core(g_core_mutex);
    size_t size = retro_serialize_size();
    if (size == 0) return env->NewStringUTF("PS1_STATE_SAVE_FAIL serialize_size_0");
    data.resize(size);
    if (!retro_serialize(data.data(), data.size())) return env->NewStringUTF("PS1_STATE_SAVE_FAIL retro_serialize_false");
  }
  std::ofstream out(path.c_str(), std::ios::binary | std::ios::trunc);
  if (!out.good()) return env->NewStringUTF("PS1_STATE_SAVE_FAIL open_failed");
  out.write((const char*)data.data(), (std::streamsize)data.size());
  out.close();
  char msg[512];
  snprintf(msg, sizeof(msg), "PS1_STATE_SAVE_OK bytes=%zu path=%s", data.size(), path.c_str());
  return env->NewStringUTF(msg);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1LoadState(JNIEnv *env, jclass, jstring jpath) {
  if (!jpath) return env->NewStringUTF("PS1_STATE_LOAD_FAIL path=null");
  if (!g_loaded.load()) return env->NewStringUTF("PS1_STATE_LOAD_FAIL core_not_loaded");
  const char *pathChars = env->GetStringUTFChars(jpath, nullptr);
  std::string path = pathChars ? pathChars : "";
  env->ReleaseStringUTFChars(jpath, pathChars);
  if (path.empty()) return env->NewStringUTF("PS1_STATE_LOAD_FAIL path_empty");
  std::ifstream in(path.c_str(), std::ios::binary);
  if (!in.good()) return env->NewStringUTF("PS1_STATE_LOAD_FAIL missing_file");
  std::vector<uint8_t> data((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  in.close();
  if (data.empty()) return env->NewStringUTF("PS1_STATE_LOAD_FAIL empty_file");
  bool ok = false;
  {
    std::lock_guard<std::mutex> core(g_core_mutex);
    ok = retro_unserialize(data.data(), data.size());
  }
  if (!ok) return env->NewStringUTF("PS1_STATE_LOAD_FAIL retro_unserialize_false");
  nap_audio_clear();
  char msg[512];
  snprintf(msg, sizeof(msg), "PS1_STATE_LOAD_OK bytes=%zu path=%s", data.size(), path.c_str());
  return env->NewStringUTF(msg);
}
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(JNIEnv *env, jclass) {
  std::lock_guard<std::mutex> life(g_life_mutex);
  g_generation.fetch_add(1); g_running.store(false);
  if (g_worker.joinable()) g_worker.join();
  if (g_loaded.exchange(false)) { nap_srm_save_if_dirty("stop"); retro_unload_game(); retro_deinit(); }
  nap_audio_clear();
  g_input_bits.store(0);
  NAPLOG("BUILD2SA2 PS1 stop frames=%llu", (unsigned long long)g_frames.load());
  return env->NewStringUTF("PS1_STOPPED");
}
