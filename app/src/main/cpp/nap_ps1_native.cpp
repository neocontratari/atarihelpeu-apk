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
#include <malloc.h> // BUILD2SK150: mallinfo() - rozpad CPU heap pameti vedle VmRSS
#include <iterator>
#include <EGL/egl.h>   // BUILD2SK98: pro gpu-gles (skutecny GL vykreslovac s texturovym filtrovanim)
#include <GLES/gl.h>   // GLES1 - presne to, co gpu-gles pouziva (fixed-function pipeline)
#include <SLES/OpenSLES.h>          // CESTA A: zvuk bez Javy (od API 9)
#include <SLES/OpenSLES_Android.h>
#include <signal.h>   // A10: zachytavac padu (sigaction)
#include <fcntl.h>    // A10: open() - zapis padu do log souboru (async-signal-safe)
#include <unistd.h>   // A10: write()/close()
#define NAPLOG(...) __android_log_print(ANDROID_LOG_INFO, "NAP_PS1", __VA_ARGS__)

// BUILD2SK115: presunuto sem (drive bylo hluboko v souboru, radek 274+) -
// DRUHY vyskyt stejne tridy chyby (pouziti pred deklaraci - poprve
// g_gles_ready, ted NAPDIAG makro) presvedcil, ze spravna oprava neni
// zalatat jeden konkretni radek, ale presunout CELOU tuhle infrastrukturu
// na zacatek souboru - takze uz ODSUD DAL v celem souboru je NAPDIAG/
// nap_diag_log vzdy k dispozici, bez ohledu na to, kde presne v souboru se
// pouzije.
static std::mutex g_diag_log_mutex;
static std::string g_diag_log_path;

// =====================================================================
//  A10: ZACHYTAVAC PADU -> /8765/log
//  Nativni pad (SIGSEGV/…) se do logu sam nezapise (jde do tombstone/logcatu,
//  kam bez adb nevidime) - proto jsme u FMV padu byli slepi. Tady chytime
//  signal a zapiseme KAM to spadlo (drobecek g_crash_stage: jadro / muj grab
//  / eglrender) do STEJNEHO souboru, ktery Rene vidi na /8765/log.
//  Vse v handleru je async-signal-safe: jen open/write/close + zasobnik,
//  ZADNY malloc/std::string/snprintf (ty by v handleru mohly zatuhnout).
static char g_crash_log_path[512]  = {0};         // signal-safe kopie cesty k hlavnimu logu
static char g_crash_last_path[512] = {0};         // A11: SAMOSTATNY soubor pro posledni pad - PREZIJE pad procesu i smazani hlavniho logu pri restartu
extern "C" { volatile const char* g_crash_stage = "init"; } // kde jsme byli, kdyz to spadlo

static void nap_wr(int fd, const char* s) { size_t n = 0; while (s[n]) n++; while (n) { ssize_t w = write(fd, s, n); if (w <= 0) break; s += w; n -= (size_t)w; } }
static void nap_wr_dec(int fd, long v) {
    char b[24]; int i = sizeof(b); int neg = v < 0; unsigned long u = neg ? (unsigned long)(-(v + 1)) + 1UL : (unsigned long)v;
    if (u == 0) b[--i] = '0';
    while (u) { b[--i] = (char)('0' + (u % 10)); u /= 10; }
    if (neg) b[--i] = '-';
    while (i < (int)sizeof(b)) { char c = b[i++]; ssize_t w = write(fd, &c, 1); if (w <= 0) break; }
}
static void nap_wr_hex(int fd, unsigned long u) {
    static const char* H = "0123456789abcdef"; char b[2 + sizeof(void*) * 2]; int i = sizeof(b);
    if (u == 0) b[--i] = '0';
    while (u) { b[--i] = H[u & 0xf]; u >>= 4; }
    b[--i] = 'x'; b[--i] = '0';
    while (i < (int)sizeof(b)) { char c = b[i++]; ssize_t w = write(fd, &c, 1); if (w <= 0) break; }
}
static void nap_write_crash_to(const char* path, int sig, void* addr) {
    if (!path || !path[0]) return;
    int fd = open(path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (fd < 0) return;
    const char* st = (const char*)g_crash_stage;
    nap_wr(fd, "\nNAP_CRASH sig=");   nap_wr_dec(fd, sig);
    nap_wr(fd, " addr=");              nap_wr_hex(fd, (unsigned long)addr);
    nap_wr(fd, " stage=");             nap_wr(fd, st ? st : "?");
    nap_wr(fd, " -- TADY to spadlo (A11)\n");
    close(fd);
}
static void nap_crash_handler(int sig, siginfo_t* si, void* uc) {
    (void)uc;
    void* addr = si ? si->si_addr : (void*)0;
    // 1) SAMOSTATNY soubor - prezije smazani hlavniho logu pri restartu; odtud
    //    to pri pristim spusteni PS1 vytahneme do /8765/log (viz boot nize).
    nap_write_crash_to(g_crash_last_path, sig, addr);
    // 2) i do hlavniho logu - kdyby appka prece jen prezila a server bezel dal.
    nap_write_crash_to(g_crash_log_path, sig, addr);
    signal(sig, SIG_DFL); // pak necháme normalni pad (tombstone), at se nic nezmizi
    raise(sig);
}
static void nap_install_crash_handler(void) {
    static int done = 0; if (done) return; done = 1;
    struct sigaction sa; memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = nap_crash_handler; sa.sa_flags = SA_SIGINFO; sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, NULL); sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGBUS,  &sa, NULL); sigaction(SIGILL,  &sa, NULL); sigaction(SIGFPE, &sa, NULL);
}

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
  // A10: signal-safe kopie (handler nesmi sahat na std::string)
  { const char* pp = p ? p : ""; size_t i = 0; for (; pp[i] && i < sizeof(g_crash_log_path) - 1; i++) g_crash_log_path[i] = pp[i]; g_crash_log_path[i] = '\0'; }
  // A11: dedikovana cesta pro posledni pad = hlavni log + ".lastcrash". Hlavni
  //      log se pri startu appky maze, tenhle sourozenec NE - proto pad prezije.
  { size_t i = 0; for (; g_crash_log_path[i] && i < sizeof(g_crash_last_path) - 11; i++) g_crash_last_path[i] = g_crash_log_path[i];
    const char* suf = ".lastcrash"; for (size_t j = 0; suf[j] && i < sizeof(g_crash_last_path) - 1; j++, i++) g_crash_last_path[i] = suf[j];
    g_crash_last_path[i] = '\0'; }
  env->ReleaseStringUTFChars(jpath, p);
}
// BUILD2SK99: obojí najednou - logcat (kdyby nekdy byl adb pristup) i durable
// soubor (co Rene skutecne vidi).
#define NAPDIAG(...) do { NAPLOG(__VA_ARGS__); nap_diag_log(__VA_ARGS__); } while(0)

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
// BUILD2SK114: presunuto sem (bylo puvodne u zbytku GLES kodu, radek 269) -
// nap_video() ji potrebuje driv, nez se tam GLES kod vubec deklaruje.
// Zachyceno PRED odeslanim explicitni kontrolou poradi deklaraci - stejna
// trida chyby jako minuly TRUE/FALSE build fail, tentokrat vcas.
static bool g_gles_ready = false;
// CESTA A: surface+kontext gpu-gles (pbuffer) a ulozeny kontext eglrenderu,
// aby tick mohl prepnout na gpu-gles pro retro_run a po grabu zpet na
// eglrender pro kresleni na displej. GLES1 a GLES2 nemohou byt oba current.
static EGLSurface g_gles_surface_A = EGL_NO_SURFACE;
static EGLDisplay g_gles_display_A = EGL_NO_DISPLAY;
static EGLContext g_gles_context_A = EGL_NO_CONTEXT;
static EGLContext g_egl_render_ctx = EGL_NO_CONTEXT;   // kontext eglrenderu
static EGLSurface g_egl_render_surf = EGL_NO_SURFACE;  // okno eglrenderu

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
  // BUILD2SK142: SK141 VRACENO ZPET - enhanced rozliseni je presne DUVOD,
  // proc Rene presel ze softwaroveho (hranateho) vykreslovani na gpu-gles
  // vubec - vypnuti tim padem nebylo prijatelny kompromis k otestovani,
  // bylo to zruseni cele smysluplnosti teto vetve. Zustava ZAPNUTO,
  // nedotknutelne. Zbytek zvukoveho problemu se musi resit jinak - viz
  // nova cista diagnostika v nap_worker (mereni bez zmeny chovani).
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
  // BUILD2SK114: nase gpu-gles cesta VZDY balí data jako RGB565 (viz
  // nap_gles_readback_and_push v gpulib_if.c) - misto spolehani na to, co
  // si jadro samo nastavilo pres SET_PIXEL_FORMAT (mohlo by byt jine,
  // zpusobilo by to zkreslene barvy), radeji to tady natvrdo vynutime na
  // hodnotu, o ktere s jistotou vime, ze odpovida datum, co posilame.
  // BUILD2SK118: gpu-gles readback ted produkuje ARGB8888 primo (uz ne
  // RGB565 - viz gpulib_if.c, zbytecny dvojity prevod odstranen), takze
  // format tady musi odpovidat.
  if (g_gles_ready) { g_pixfmt.store(PIXFMT_XRGB8888); }
  const int fmt = g_pixfmt.load();
  // BUILD2SK114: KRITICKA kontrola - nas gpu-gles readback VZDY balí data
  // jako RGB565 (PIXFMT_RGB565=2), ale tenhle kod si o tom, jak data cist,
  // rozhoduje podle g_pixfmt - promenne, kterou NASTAVUJE JADRO SAMO (ne
  // my), s vychozi hodnotou PIXFMT_0RGB1555=0. Pokud jadro nikdy nezavola
  // SET_PIXEL_FORMAT s RGB565 (nebo to zavola s necim jinym), nase RGB565
  // data by se cetla SPATNYM bitovym rozlozenim - barvy by vysly zkreslene/
  // spatne, presne takovy druh chyby, ktery by vysvetlil "nativne se
  // opravdu neco kresli (ASCII dump ukazal skutecny obsah), ale po
  // prevodu na Java stranu je to spatne". Throttled log, ne kazdy snimek.
  static int nap_pixfmt_log_count = 0;
  if (nap_pixfmt_log_count < 20) {
    nap_pixfmt_log_count++;
    NAPDIAG("BUILD2SK118 PS1_PIXFMT_CHECK fmt=%d (0=0RGB1555 1=XRGB8888_ocekavano 2=RGB565)", fmt);
  }
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
static std::mutex g_amutex;          // uz jen pro stara Java rozhrani
static std::vector<int16_t> g_afifo; // ponechano kvuli ps1PullAudio (Java cesta)

// ==================================================================
//  ZVUKOVA FRONTA BEZ ZAMKU (kruhovy buffer)
//  Drive byla zvukova fronta obycejny vektor pod SPOLECNYM zamkem:
//  emulace (vlakno s grafikou) do nej vkladala - a pri zvetseni se cely
//  REALOKOVAL a kopiroval - a zvukovy callback z nej mazal ZEPREDU, coz
//  znamena posunout cely zbytek pameti. Oboji pod jednim zamkem.
//  Kdyz byla grafika zatizena, zvukove vlakno na tom zamku cekalo a zvuk
//  vypadl - i kdyz se snimky stihaly. Presne to Rene popsal: zvuk a
//  grafika protlacene jednim mistem.
//  Ted maji kazdy svuj konec kruhu, hlidany atomickym citacem: zadny
//  zamek, zadne kopirovani, zadna alokace. Zvuk uz na grafiku necekha.
// ==================================================================
#define NAP_ARING_SHORTS (1u << 17)          // kapacita kruhu (strop)
// Cilova naplnenost fronty. Kdyz se nechala rust az ke stropu, latence
// narostla na 1,4 s a pak se jednorazove zahodil velky kus - a to je slyset.
// Drzime ji nizko a pripadny prebytek ubirame po kouskach (~1 ms), coz slysitelne
// neni. 44100 Hz stereo => 88,2 shortu na milisekundu.
#define NAP_ARING_MS(ms)   ((unsigned)((ms) * 882u / 10u))
#define NAP_ARING_TARGET   NAP_ARING_MS(90)    // kolem 90 ms
#define NAP_ARING_HIGH     NAP_ARING_MS(140)   // nad 140 ms zacneme ubirat
#define NAP_ARING_TRIM     NAP_ARING_MS(1)     // ubirame po 1 ms
static int16_t              g_aring[NAP_ARING_SHORTS];
static std::atomic<unsigned> g_aring_w{0};   // pise emulace
static std::atomic<unsigned> g_aring_r{0};   // cte zvukovy callback

static inline unsigned nap_aring_avail(void) {
  return g_aring_w.load(std::memory_order_acquire) - g_aring_r.load(std::memory_order_relaxed);
}
static void nap_aring_write(const int16_t *src, unsigned n) {
  unsigned w = g_aring_w.load(std::memory_order_relaxed);
  unsigned r = g_aring_r.load(std::memory_order_acquire);
  unsigned free_ = NAP_ARING_SHORTS - (w - r);
  if (n > free_) {                 // fronta plna - zahodit nejstarsi
    unsigned drop = n - free_;
    g_aring_r.store(r + drop, std::memory_order_release);
  }
  for (unsigned i = 0; i < n; i++) g_aring[(w + i) & (NAP_ARING_SHORTS - 1)] = src[i];
  w += n;
  g_aring_w.store(w, std::memory_order_release);
}

// ==================================================================
//  ZVUK RIDI TEMPO EMULACE (zpetny tlak)
//  Prevzato z naseho Sega emulatoru, kde je zvuk davno vyreseny stejnym
//  zpusobem ("audio-clock backpressure").
//  Problem byl v tom, ze jadro krokujeme jednou na kazdy snimek displeje
//  (60 Hz), ale hra muze bezet na 50 Hz (PAL) nebo mit v nabidce jine tempo.
//  Pak se vyrobi vic zvuku, nez se stihne odebrat. Drive jsem prebytek
//  ZAHAZOVAL - a prave to znelo jako "zpomali se a pak zrychli", protoze
//  se ze zvuku ubiraly kusy.
//  Ted misto zahazovani KRATCE POCKAME. Emulace se tim srovna na tempo
//  zvuku, zadny vzorek se nezahodi a vyska ani rychlost tonu se nemeni.
//  Kdyz je fronta naopak nizka, nespime vubec, aby zvuk nikdy nevyschl.
// ==================================================================
// ==================================================================
//  EMULACE MA VLASTNI VLAKNO (cesta A)
//  Drive se krok emulace volal z draw_frame, tedy z vlakna, ktere kresli
//  a ceka na vsync. Cokoli, co zdrzelo kresleni, zdrzelo i emulaci - a s ni
//  vyrobu zvuku. Odtud kousani zavisle na zatezi grafiky. Ve stare (funkcni)
//  ceste bezela emulace na vlastnim vlakne - presne na to Rene ukazoval.
//  Ted je to zpatky: emulace bezi sama, kresleni si jen bere posledni hotovy
//  snimek. Zadne z nich uz nebrzdi druhe.
// ==================================================================
static std::thread            g_core_thread;
static std::atomic<bool>      g_core_run{false};
static std::vector<unsigned char> g_frame_buf[2];   // dva snimky - jeden se plni, druhy se kresli
static std::atomic<int>       g_frame_ready{-1};    // index hotoveho snimku
static std::atomic<int>       g_frame_w{0}, g_frame_h{0};
static std::mutex             g_frame_swap;

// Vraci 1, kdyz se ma TENTO krok VYNECHAT.
//
// POZOR NA CHYBU, KTERA TU BYLA PREDTIM: zpetny tlak byl resen uspanim
// (usleep) - ale na vlakne, ktere kresli a ceka na vsync. Uspani zpusobilo
// minuti vykresleni, snimek se protahl z 16 na 33 ms, jadro udelalo jen
// polovinu kroku, zvuk spadl pod cil, spani se vypnulo, zvuk zase narostl -
// a cele se to ROZKMITALO. Presne to znelo jako "zpomali se a pak zrychli".
//
// Spravne resenim je krok emulace VYNECHAT. Tim se vyrobi presne o jeden
// snimek zvuku mine, timing displeje se vubec nedotkneme a hra na 50 Hz
// na 60Hz displeji se srovna sama (vynecha se kazdy sesty krok).
static int nap_audio_skip_step(void) {
  unsigned avail = nap_aring_avail();
  if (avail < NAP_ARING_TARGET) return 0;        // zvuku je malo - krokovat
  return 1;                                      // je ho dost - tento tick vynechat
}
// ==================================================================
//  ODBOCKA ZVUKU PRO TV
//  Zvuk pro TV se driv bral z Javove zvukove cesty (writePs1AudioTrack).
//  Ta uz nebezi - zvuk obsluhuje nativni OpenSL - takze TV nemela odkud brat
//  a byla nema. Tady si z prehravaneho zvuku poridime KOPII, ktera hlavni
//  cestu nijak nezdrzuje: zapis je jen presun bajtu, bez zamku.
//  Kdyz si TV kopii nevyzvedava (nikdo se nedivá), nejstarsi data se prepisuji.
#define NAP_TVRING_SHORTS (1u << 16)          // ~0,7 s stereo
static int16_t               g_tvring[NAP_TVRING_SHORTS];
static std::atomic<unsigned> g_tvring_w{0};
static std::atomic<unsigned> g_tvring_r{0};

static void nap_tvring_write(const int16_t *src, unsigned n) {
  unsigned w = g_tvring_w.load(std::memory_order_relaxed);
  unsigned r = g_tvring_r.load(std::memory_order_acquire);
  unsigned free_ = NAP_TVRING_SHORTS - (w - r);
  if (n > free_) g_tvring_r.store(r + (n - free_), std::memory_order_release);
  for (unsigned i = 0; i < n; i++) g_tvring[(w + i) & (NAP_TVRING_SHORTS - 1)] = src[i];
  g_tvring_w.store(w + n, std::memory_order_release);
}

static unsigned nap_aring_read(int16_t *dst, unsigned n) {
  unsigned r = g_aring_r.load(std::memory_order_relaxed);
  unsigned have = g_aring_w.load(std::memory_order_acquire) - r;
  unsigned take = have < n ? have : n;
  for (unsigned i = 0; i < take; i++) dst[i] = g_aring[(r + i) & (NAP_ARING_SHORTS - 1)];
  g_aring_r.store(r + take, std::memory_order_release);
  return take;
}
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
  { std::lock_guard<std::mutex> lock(g_amutex); g_afifo.clear(); }
  g_aring_r.store(g_aring_w.load(std::memory_order_acquire), std::memory_order_release);
}
static void nap_audio_push(const int16_t *data, size_t frames) {
  if (!data || !frames) return;
  nap_aring_write(data, (unsigned)(frames * 2));   // bez zamku
}

// ==================================================================
//  CESTA A ZVUK — OpenSL ES primo v jadre (bez Javy).
//  V ceste A stary ps1AudioThread (Java) nebezi, takze g_afifo by
//  nikdo nevybiral. Tenhle OpenSL player si bere vzorky z TE SAME
//  fronty g_afifo pres svuj callback - zadna Java, jedno vlakno.
//  OpenSL ES funguje od API 9 (na rozdil od AAudio od API 26), takze
//  jede i na minSdk 24.
// ==================================================================
static SLObjectItf s_sl_engine_obj = nullptr;
static SLEngineItf s_sl_engine = nullptr;
static SLObjectItf s_sl_mix_obj = nullptr;
static SLObjectItf s_sl_player_obj = nullptr;
static SLPlayItf   s_sl_play = nullptr;
static SLAndroidSimpleBufferQueueItf s_sl_queue = nullptr;
static bool s_sl_ready = false;

#define NAP_SL_BLOCK_FRAMES 1024
// OPRAVA ZVUKU: drive existoval JEDINY blok pameti, ktery se do fronty
// zaradil dvakrat - prehravalo se tedy z pameti, kterou callback zaroven
// prepisoval, a rezerva byla prakticky nulova. Jakmile se snimek o chlup
// zdrzel, fronta dobehla a doplnila se TICHEM = kousani.
// Ted je bloku vic a stridaji se, takze je v ceste rezerva ~185 ms.
#define NAP_SL_BLOCKS 4   // 4 x 23 ms = ~93 ms rezervy; drive 8 = 186 ms, tedy zbytecne nafouknute
static int16_t s_sl_blocks[NAP_SL_BLOCKS][NAP_SL_BLOCK_FRAMES * 2];
static int     s_sl_next = 0;

// OpenSL si rekne o dalsi blok - naplnime ho z g_afifo (nebo tichem).
static void nap_sl_callback(SLAndroidSimpleBufferQueueItf bq, void*) {
  size_t need = NAP_SL_BLOCK_FRAMES * 2; // shorts
  int16_t *blk = s_sl_blocks[s_sl_next];
  s_sl_next = (s_sl_next + 1) % NAP_SL_BLOCKS;   // dalsi blok - nikdy neprepisujeme ten, co se prave hraje
  {
    size_t take = nap_aring_read(blk, (unsigned)need);   // bez zamku
    if (take < need) {
      // Misto tvrdeho ticha (lupanec) dozniva posledni vzorek - pri kratkem
      // vypadku je to slyset mnohem min.
      int16_t lastL = take >= 2 ? blk[take - 2] : 0;
      int16_t lastR = take >= 1 ? blk[take - 1] : 0;
      for (size_t i = take; i + 1 < need; i += 2) {
        lastL = (int16_t)(lastL * 7 / 8);
        lastR = (int16_t)(lastR * 7 / 8);
        blk[i] = lastL; blk[i + 1] = lastR;
      }
    }
  }
  nap_tvring_write(blk, (unsigned)need);   // kopie pro TV (hlavni cestu nezdrzuje)
  (*bq)->Enqueue(bq, blk, need * sizeof(int16_t));
}

static void nap_sl_open(void) {
  if (s_sl_ready) return;
  if (slCreateEngine(&s_sl_engine_obj, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_engine_obj)->Realize(s_sl_engine_obj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_engine_obj)->GetInterface(s_sl_engine_obj, SL_IID_ENGINE, &s_sl_engine) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_engine)->CreateOutputMix(s_sl_engine, &s_sl_mix_obj, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_mix_obj)->Realize(s_sl_mix_obj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
  SLDataLocator_AndroidSimpleBufferQueue locBufq = { SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, NAP_SL_BLOCKS };
  SLDataFormat_PCM fmt = { SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
    SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
    SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN };
  SLDataSource src = { &locBufq, &fmt };
  SLDataLocator_OutputMix locMix = { SL_DATALOCATOR_OUTPUTMIX, s_sl_mix_obj };
  SLDataSink sink = { &locMix, nullptr };
  const SLInterfaceID ids[1] = { SL_IID_ANDROIDSIMPLEBUFFERQUEUE };
  const SLboolean req[1] = { SL_BOOLEAN_TRUE };
  if ((*s_sl_engine)->CreateAudioPlayer(s_sl_engine, &s_sl_player_obj, &src, &sink, 1, ids, req) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_player_obj)->Realize(s_sl_player_obj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_player_obj)->GetInterface(s_sl_player_obj, SL_IID_PLAY, &s_sl_play) != SL_RESULT_SUCCESS) return;
  if ((*s_sl_player_obj)->GetInterface(s_sl_player_obj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &s_sl_queue) != SL_RESULT_SUCCESS) return;
  (*s_sl_queue)->RegisterCallback(s_sl_queue, nap_sl_callback, nullptr);
  (*s_sl_play)->SetPlayState(s_sl_play, SL_PLAYSTATE_PLAYING);
  // nakopnout - naplnit CELOU frontu tichem, kazdy blok vlastni pameti
  memset(s_sl_blocks, 0, sizeof(s_sl_blocks));
  for (int i = 0; i < NAP_SL_BLOCKS; i++)
    (*s_sl_queue)->Enqueue(s_sl_queue, s_sl_blocks[i], sizeof(s_sl_blocks[i]));
  s_sl_next = 0;
  s_sl_ready = true;
  NAPDIAG("CESTA_A ZVUK OpenSL ES otevren (44100/2/i16, z g_afifo, bez Javy, funguje od API 9)");
}

static void nap_sl_close(void) {
  if (s_sl_player_obj) { (*s_sl_player_obj)->Destroy(s_sl_player_obj); s_sl_player_obj = nullptr; }
  if (s_sl_mix_obj)    { (*s_sl_mix_obj)->Destroy(s_sl_mix_obj);       s_sl_mix_obj = nullptr; }
  if (s_sl_engine_obj) { (*s_sl_engine_obj)->Destroy(s_sl_engine_obj); s_sl_engine_obj = nullptr; }
  s_sl_engine = nullptr; s_sl_play = nullptr; s_sl_queue = nullptr; s_sl_ready = false;
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
  // NAPLES2: vnitrnosti stareho GLES1 pluginu (iResX/iResY/rRatioRect,
  // GLinitialize, InitializeTextureStore, MakeDisplayLists, updateFrontDisplay,
  // SetOGLDisplaySettings) uz neexistuji - novy GLES2 renderer je nema.
  int n2_init(void);   // vytvori shadery, FBO a texturu VRAM
  void nap_gles_present_frame(void); // BUILD2SK153: JEDINE misto prezentace snimku - presne 1x za tick, po retro_run (= po VBlanku emulovaneho PS1), s cerstvym gpu.screen (respektuje GP1(05h) flip)
  void nap_gles_sync_display_settings(void); // BUILD2SK119: viz gpulib_if.c - bezpecne obnovi iResX/iResY/rRatioRect pred volanim SetOGLDisplaySettings
  // CESTA A: dvirka z gpulib_if.c - kopie canvasu do snapshot textury BEZ
  // ctecky a vraceni jejiho id + vyrezu, pro prime kresleni v eglrender.
  unsigned nap_gles_grab_texture(int* out_x, int* out_y, int* out_w, int* out_h);
  const void* nap_gles_grab_pixels(int* out_w, int* out_h); // BOD 2: pres procesor
  int nap_gles_vram_w(void);
  int nap_gles_vram_h(void);
  // BUILD2SK106: POZOR presne typy - BOOL je v tomhle projektu #define BOOL
  // unsigned short (NE int - to by byla skutecna chyba, cteni/zapis 4 bajtu
  // pres 2-bajtovou promennou). Overeno primo v gpuExternals.h pred pouzitim.
}


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
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT, // NAPLES2: renderer je GLES2
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

  const EGLint contextAttribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE }; // GLES2
  // NAPLES2: renderer je nove cely v GLES2, takze kontext UZ SDILIME s
  // eglrenderem. Driv to neslo: gpu-gles byl GLES1 a Mali sdileni GLES1<->GLES2
  // odmitalo (eglCreateContext selhal, viz log A5) - proto se obraz musel tahat
  // pres procesor pomoci glReadPixels. GLES2<->GLES2 sdileni projde, takze
  // eglrender muze kreslit texturu VRAM primo, bez kopirovani.
  g_egl_render_ctx  = eglGetCurrentContext();
  g_egl_render_surf = eglGetCurrentSurface(EGL_DRAW);
  EGLContext context = eglCreateContext(display, config, g_egl_render_ctx, contextAttribs);
  if (context == EGL_NO_CONTEXT) {
    // Kdyby sdileni presto selhalo, zkusime vlastni kontext bez sdileni -
    // obraz pak pujde zalozni cestou pres pixely (funguje, jen je pomalejsi).
    NAPDIAG("NAPLES2 sdileni kontextu selhalo (err=0x%x), zkousim bez sdileni", eglGetError());
    context = eglCreateContext(display, config, EGL_NO_CONTEXT, contextAttribs);
  }
  if (context == EGL_NO_CONTEXT) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglCreateContext err=0x%x", eglGetError());
    return false;
  }

  // CESTA A: NEDELAT makecurrent tady natvrdo. eglrender a gpu-gles maji
  // kazdy svuj kontext (sdilene textury). Prepinani resi tick: pred
  // retro_run se udela current gpu-gles, po nem zpet eglrender. Ulozime
  // si nas surface+context, at je tick ma cim prepnout.
  g_gles_surface_A = surface;
  g_gles_display_A = display;
  g_gles_context_A = context;
  if (!eglMakeCurrent(display, surface, surface, context)) {
    NAPDIAG("BUILD2SK98 GLES_INIT_FAIL step=eglMakeCurrent err=0x%x", eglGetError());
    return false;
  }

  // NAPLES2: stary gpu-gles potreboval pred GLinitialize nastavit globalni
  // iResX/iResY/rRatioRect (kreslil v souradnicich displeje). Novy GLES2
  // renderer kresli rovnou v souradnicich VRAM, takze nic takoveho nema.

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
  // BUILD2SK107: build FIX - TRUE/FALSE jsou #define v gpu-gles vlastnim
  // gpuExternals.h (C hlavicka), ktera se sem, do C++ souboru, nezahrnuje -
  // GitHub Actions to spravne odchytil jako "undeclared identifier" driv,
  // nez se cokoli dostalo na telefon. 1 je presne to, na co TRUE stejne
  // expanduje (overeno: #define TRUE 1) - zadna nova makra, zadne riziko.
  // NAPLES2: bDisplayNotSet / bSetClip / CSTEXTURE / CSVERTEX / CSCOLOR byly
  // globalni promenne stareho GLES1 pluginu (stav fixed-function pipeline).
  // Novy GLES2 renderer zadny takovy globalni stav nema - vse si drzi sam.

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
  // NAPLES2: novy GLES2 renderer se inicializuje sam v renderer_init()
  // (shadery, FBO, textura VRAM). Zadny InitializeTextureStore /
  // GLinitialize / MakeDisplayLists - to byly vnitrnosti GLES1 pluginu.
  {
    int rc = n2_init();
    if (rc != 0) {
      NAPDIAG("NAPLES2 GLES_INIT_FAIL step=n2_init kod=%d", rc);
      return false;
    }
  }
  // BUILD2SK108: SetOGLDisplaySettings(1) tady v initu byla CHYBA - potvrzeno
  // logem (GLES_SCISSOR_CHECK w=1 h=1). V tomhle okamziku jeste hra vubec
  // neposlala zadny GPU prikaz, takze PSXDisplay.DrawArea.* jsou porad na
  // vychozich/nulovych hodnotach - vypocet scissor z nich dal nesmyslny
  // 1x1 obdelnik. A HORE - tohle volani "spotrebovalo" bSetClip (nastavi
  // se zpatky na FALSE uvnitr funkce), takze se to uz nikdy samo neopravilo.
  // Presunuto do worker smycky (kazdy tick, viz nize) - tam uz hra bude mit
  // realne DrawArea hodnoty nastavene, a funkce sama premapuje scissor na
  // spravnou hodnotu pri kazde zmene (ma vlastni "zmenilo se to?" kontrolu,
  // takze opakovane volani je levne, jakmile se stav ustali).

  // KONTEXT JADRA MUSI ZUSTAT VOLNY.
  // Inicializace si ho vzala, aby mohla vytvorit shadery, FBO a textury
  // (patri tedy kontextu jadra - to je spravne). Ale zustal by "current"
  // na vlakne kresleni, a jeden kontext nemuze byt current na dvou vlaknech.
  // Vlakno emulace by si ho pak neprevzalo a nekreslilo by se nic.
  // Vratime tedy vlaknu kresleni jeho vlastni kontext a jadru ten jeho
  // uvolnime.
  if (g_egl_render_ctx != EGL_NO_CONTEXT)
    eglMakeCurrent(display, g_egl_render_surf, g_egl_render_surf, g_egl_render_ctx);
  else
    eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

  NAPDIAG("BUILD2SK98 GLES_INIT_OK pbuffer=1024x768 initial=320x240");
  return true;
}

static void nap_worker(int gen) {
  NAPLOG("BUILD2SA2 PS1 worker start gen=%d fps=%.2f", gen, g_fps);
  NAPDIAG("BUILD2SK99 PS1_WORKER_THREAD_ALIVE gen=%d", gen); // BUILD2SK99: durable kanarek - vlakno samo bezi
  NAPDIAG("BUILD2SK154 NATIVE_VERSION_CONFIRM file=nap_ps1_native.cpp"); // BUILD2SK154: pri kazdem startu jednoznacne potvrdi, KTERA verze tohohle souboru skutecne bezi
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
    // BUILD2SK142: RENE ODMITL SK141 (zdvojnasobene 3D rozliseni je presny
    // duvod, proc presel na gpu-gles - nedotknutelne). Misto dalsiho
    // hadani, co jineho na g_worker (vlakne se zvukem) stoji cas, presne
    // ZMERIT vsechny tri hlavni kroky KAZDEHO ticku - retro_run() (cela
    // PS1 emulace VCETNE zpracovani vsech GP0/GP1 prikazu skrz gpuPrim.c/
    // gpuDraw.c), sync_display_settings (viewport/scissor) a
    // updateFrontDisplay (predani snimku - po SK140 uz jen rychle
    // predani ctecimu vlaknu, nemelo by trvat dlouho). ZADNA zmena
    // chovani/kvality - jen cislo navic v logu.
    double nap_t_run0 = 0, nap_t_run1 = 0, nap_t_sync1 = 0, nap_t_disp1 = 0;
    {
      std::lock_guard<std::mutex> core(g_core_mutex);
      nap_t_run0 = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
      retro_run();
      nap_t_run1 = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
      // BUILD2SK108: presunuto sem z jednorazove inicializace (viz
      // nap_gles_egl_init) - volane driv, PRED tim, nez hra vubec poslala
      // prvni GPU prikaz, to spocitalo nesmyslny 1x1 scissor obdelnik
      // (potvrzeno GLES_SCISSOR_CHECK v logu) a "spotrebovalo" bSetClip,
      // takze se to uz nikdy samo neopravilo. Tady, KAZDY tick, uz retro_run()
      // vyse zpracovala realne GPU prikazy hry - PSXDisplay.DrawArea ma
      // skutecne hodnoty. Funkce ma vlastni "zmenilo se neco?" kontrolu
      // (EqualRect porovnani), takze opakovane volani kazdy tick je levne,
      // jakmile se stav ustali - prepocita jen kdyz je to skutecne potreba.
      if (g_gles_ready) { nap_gles_sync_display_settings(); } // BUILD2SK119: viz gpulib_if.c - drive primo SetOGLDisplaySettings(1), ted pres bezpecny wrapper co nejdriv obnovi iResX/iResY/rRatioRect
      nap_t_sync1 = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
      // BUILD2SK108: throttled kontrola vysledku (kazdych ~90 tick = ~1.5s
      // pri 60fps) - overime v logu, jestli se scissor po prvnich par
      // snimcich (kdy uz hra poslala realne DrawArea prikazy) opravil na
      // neco rozumneho, misto puvodniho degenerovaneho 1x1.
      static int nap_scissor_check_tick = 0;
      if (g_gles_ready && (++nap_scissor_check_tick % 90 == 1)) {
        GLint scissorBox[4] = {0,0,0,0};
        glGetIntegerv(GL_SCISSOR_BOX, scissorBox);
        NAPDIAG("BUILD2SK108 GLES_SCISSOR_CHECK tick=%d x=%d y=%d w=%d h=%d",
          nap_scissor_check_tick, scissorBox[0], scissorBox[1], scissorBox[2], scissorBox[3]);
      }
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
      // BUILD2SK153: updateFrontDisplay() -> nap_gles_present_frame().
      // Prezentace je ted VYHRADNE tady: presne jednou za tick, az kdyz je
      // cely snimek doemulovany (VBlank). Pokud hra behem snimku flipla
      // zobrazovaci buffer (GP1(05h)), gpu.screen uz nese nove src_x/src_y
      // a cte se prave dokoncena polovina VRAM - zadne cteni rozkresleneho
      // back-bufferu (to byla, spolu s 2-snimky-starymi metadaty v
      // gpulib_if.c, pricina stroboskopu u 30fps her).
      if (g_gles_ready) { nap_gles_present_frame(); }
      nap_t_disp1 = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();
      if (++srmTick % 300 == 0) nap_srm_save_if_dirty("periodic");
    }
    {
      static double nap_sum_run = 0, nap_sum_sync = 0, nap_sum_disp = 0;
      static int nap_tick_n = 0;
      nap_sum_run += (nap_t_run1 - nap_t_run0);
      nap_sum_sync += (nap_t_sync1 - nap_t_run1);
      nap_sum_disp += (nap_t_disp1 - nap_t_sync1);
      nap_tick_n++;
      if (nap_tick_n >= 60) {
        // BUILD2SK145: Rene poslal log, kde avgRetroRunMs ROSTE od ~3ms na
        // ~47ms behem jedne session (ne od zacatku vysoke - postupne).
        // Dve hlavni podezreni: (a) tepelne omezovani telefonu (CPU/GPU se
        // pod delsi zatezi sam zpomali - hardwarovy jev, ne chyba v kodu),
        // (b) neco na strane emulatoru roste (napr. texture cache -
        // BUILD2SA13 pad z 19.7. byl prave v CheckTextureInSubSCache) a
        // hledani v ni casem zpomaluje kazdy dalsi snimek. Precteni
        // aktualni CPU frekvence (bezne cist bez specialnich prav) tohle
        // rozliší: kdyby FREKVENCE KLESALA soubezne s tim, jak
        // avgRetroRunMs roste, je to tepelne omezovani (hardware, ne muj
        // kod). Kdyby frekvence zustavala STEJNA a presto se zpomaluje, je
        // to neco v samotnem kodu.
        // BUILD2SK148: RENE MEL PRAVDU SI NA TO DUPNOUT - cpuFreqKhz cetlo
        // JEN cpu0. Telefon ma vic jader s NEZAVISLYMI frekvencemi (big.LITTLE
        // architektura) a sam operacni system frekvenci beznemenne meni podle
        // AKTUALNI ZATEZE (ne jen kvuli teplote) - takze "frekvence klesla"
        // vubec nemusi znamenat "prehrivani", a navic jsem mozna cetl uplne
        // JINE jadro, nez na kterem zrovna bezela emulace. Misto DOHADU z
        // frekvence: cist PRIMO teplotu (kazda Android appka to muze, zadna
        // zvlastni prava) ze VSECH dostupnych tepelnych senzoru a vzit
        // nejvyssi namerenou hodnotu - to je nezpochybnitelne, primo to,
        // "jestli telefon o nejake teplote vi", misto neprimeho odhadu.
        // Navic max frekvence pres VSECHNA jadra (ne jen cpu0).
        long maxTempMilliC = -1;
        for (int tz = 0; tz < 20; tz++) {
          char path[96];
          snprintf(path, sizeof(path), "/sys/class/thermal/thermal_zone%d/temp", tz);
          FILE *tf = fopen(path, "r");
          if (!tf) continue;
          long t = -1;
          if (fscanf(tf, "%ld", &t) == 1 && t > maxTempMilliC) maxTempMilliC = t;
          fclose(tf);
        }
        long maxCpuFreqKhz = -1;
        for (int cpu = 0; cpu < 8; cpu++) {
          char path[96];
          snprintf(path, sizeof(path), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", cpu);
          FILE *cf = fopen(path, "r");
          if (!cf) continue;
          long fr = -1;
          if (fscanf(cf, "%ld", &fr) == 1 && fr > maxCpuFreqKhz) maxCpuFreqKhz = fr;
          fclose(cf);
        }
        // BUILD2SK147: Rene trva na tom, ze je to pamet ("plni se pamet
        // presne jak u Segy") - tohle mereni (VmRSS) zustava, je to primy,
        // spravny zpusob, jak to overit, a beru to jako hlavni podezreni,
        // dokud cisla neukazou jinak.
        long vmRssKb = -1;
        FILE *sf = fopen("/proc/self/status", "r");
        if (sf) {
          char line[256];
          while (fgets(line, sizeof(line), sf)) {
            if (strncmp(line, "VmRSS:", 6) == 0) { sscanf(line + 6, "%ld", &vmRssKb); break; }
          }
          fclose(sf);
        }
        // BUILD2SK150: RENE MEL PRAVDU - vmRssKb roste ROVNOMERNE a
        // NEPRETRZITE (potvrzeno v poslednim logu, 351MB -> 607MB behem
        // jedne session). Prosel jsem gpuTexture.c (kde byl i puvodni pad
        // z 19.7.) - hlavni GL textura-atlas objekty (gTexName a spol.)
        // se vytvareji JEN JEDNOU (hlidano "if(!gTexName)") a znovupouzivaji
        // pres glTexSubImage2D, ne pres opakovane glGenTextures - takze
        // primy GL-objekt-na-objekt unik tam na prvni pohled neni. Aby bylo
        // jasne, jestli je unik v BEZNE (malloc) CPU pameti (a tedy
        // hledatelny), nebo nekde jinde (GPU/ovladac strana, mimo dosah
        // mallinfo) - pridavame mallinfo() rozpad hned vedle VmRSS.
        struct mallinfo mi = mallinfo();
        long heapActiveKb = (long)(mi.uordblks / 1024);
        NAPDIAG("BUILD2SK150 PS1_TICK_TIMING avgRetroRunMs=%.2f avgSyncMs=%.2f avgDispMs=%.2f avgTotalMs=%.2f n=%d maxCpuFreqKhz=%ld maxTempMilliC=%ld vmRssKb=%ld heapActiveKb=%ld",
          nap_sum_run / nap_tick_n, nap_sum_sync / nap_tick_n, nap_sum_disp / nap_tick_n,
          (nap_sum_run + nap_sum_sync + nap_sum_disp) / nap_tick_n, nap_tick_n, maxCpuFreqKhz, maxTempMilliC, vmRssKb, heapActiveKb);
        nap_sum_run = 0; nap_sum_sync = 0; nap_sum_disp = 0; nap_tick_n = 0;
      }
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
  // ukoncit vlakno emulace (cesta A) driv, nez uvolnime jadro
  if (g_core_run.exchange(false) && g_core_thread.joinable()) g_core_thread.join();
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
    g_fw.load(),g_fh.load(),g_pixfmt.load(),(size_t)(nap_aring_avail()/2),(unsigned long long)g_audio_samples_dropped.load(),(unsigned long long)g_audio_resyncs.load(),g_fps,
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
// Zvuk pro TV: Java si vyzvedne, co se mezitim prehralo. Vraci pocet shortu.
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PullTvAudio(JNIEnv *env, jclass, jshortArray out) {
  if (!out) return 0;
  jsize cap = env->GetArrayLength(out);
  if (cap <= 0) return 0;
  unsigned r = g_tvring_r.load(std::memory_order_relaxed);
  unsigned have = g_tvring_w.load(std::memory_order_acquire) - r;
  unsigned take = have < (unsigned)cap ? have : (unsigned)cap;
  if (take == 0) return 0;
  take &= ~1u;                       // zachovat parovani L,R
  if (take == 0) return 0;
  static std::vector<int16_t> tmp;
  if (tmp.size() < take) tmp.resize(take);
  for (unsigned i = 0; i < take; i++) tmp[i] = g_tvring[(r + i) & (NAP_TVRING_SHORTS - 1)];
  g_tvring_r.store(r + take, std::memory_order_release);
  env->SetShortArrayRegion(out, 0, (jsize)take, tmp.data());
  return (jint)take;
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
// ==================================================================
//  CESTA A — eglrender rizeni gpu-gles PRIMO (bez Javy, bez workeru).
//  Varianta 1: eglrender prebira cely boot a vola tyto funkce ZE SVEHO
//  vlakna (kde ma okno a GL kontext sdileny s gpu-gles). Stary worker
//  se pro tuhle cestu nespousti - jedno vlakno vola retro_run, zadna
//  kolize globalniho stavu gpu-gles.
// ==================================================================
extern "C" JNIEXPORT jint JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(JNIEnv* env, jclass,
        jstring jsys, jstring jgame) {
    const char* csys  = env->GetStringUTFChars(jsys, nullptr);
    const char* cgame = env->GetStringUTFChars(jgame, nullptr);
    std::string sys = csys ? csys : "";
    std::string game = cgame ? cgame : "";
    if (csys)  env->ReleaseStringUTFChars(jsys, csys);
    if (cgame) env->ReleaseStringUTFChars(jgame, cgame);

    if (g_loaded.exchange(false)) { retro_unload_game(); retro_deinit(); }
    g_sysdir = sys;
    g_input_bits.store(0);
    g_boot_error.clear();

    // gpu-gles EGL kontext MUSI vzniknout na TOMHLE (eglrender) vlakne
    g_gles_ready = nap_gles_egl_init();
    if (!g_gles_ready) NAPDIAG("CESTA_A GLES_INIT_FAIL v eglrender vlakne");

    retro_set_environment(nap_env);
    retro_set_video_refresh(nap_video); // jadro chce callback; obraz z nej zahodime
    retro_set_audio_sample(nap_audio_sample);
    retro_set_audio_sample_batch(nap_audio_batch);
    retro_set_input_poll(nap_input_poll);
    retro_set_input_state(nap_input_state);
    retro_init();

    retro_game_info gi; memset(&gi, 0, sizeof(gi));
    gi.path = game.c_str();
    if (!retro_load_game(&gi)) {
        NAPDIAG("CESTA_A retro_load_game FAILED path=%s", game.c_str());
        retro_deinit();
        return -1;
    }
    g_loaded.store(true);
    nap_srm_set_path(game);
    nap_srm_load();

    retro_system_av_info av; memset(&av, 0, sizeof(av));
    retro_get_system_av_info(&av);
    g_fps = av.timing.fps > 1 ? av.timing.fps : 60.0;

    NAPDIAG("CESTA_A BOOT_OK gpuGles=%s fps=%.2f zvukJadra=%.0f Hz vystup=44100 Hz",
            g_gles_ready ? "ANO" : "NE", g_fps, av.timing.sample_rate);
    return g_gles_ready ? 1 : 0;
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglTick(JNIEnv*, jclass) {
    if (!g_loaded.load()) return;
    retro_run();
    if (g_gles_ready) nap_gles_sync_display_settings();
}

extern "C" JNIEXPORT void JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(JNIEnv*, jclass) {
    if (g_loaded.exchange(false)) { nap_srm_save_if_dirty("egl_stop"); retro_unload_game(); retro_deinit(); }
    nap_audio_clear();
    g_input_bits.store(0);
    NAPDIAG("CESTA_A EGL_STOP");
}

// eglrender si timhle vezme id hotove gpu-gles textury + vyrez (pres dvirka).
extern "C" unsigned nap_ps1_egl_grab(int* x, int* y, int* w, int* h) {
    if (!g_gles_ready) return 0;
    // Rámec (FBO) se mezi kontexty NESDILI, jen textury. Dokresleni davky
    // proto musi probehnout v kontextu jadra; teprve pak se vratime na
    // eglrender, ktery si sdilenou texturu nakresli sam - bez kopirovani.
    if (g_gles_display_A != EGL_NO_DISPLAY)
        eglMakeCurrent(g_gles_display_A, g_gles_surface_A, g_gles_surface_A, g_gles_context_A);
    unsigned tex = nap_gles_grab_texture(x, y, w, h);
    if (g_egl_render_ctx != EGL_NO_CONTEXT && g_gles_display_A != EGL_NO_DISPLAY)
        eglMakeCurrent(g_gles_display_A, g_egl_render_surf, g_egl_render_surf, g_egl_render_ctx);
    return tex;
}

// BOD 2: obraz pres pixely (funguje napric kontexty, sdileni netreba).
// Prepne na gpu-gles kontext, precte pixely z canvasu, vrati je a
// prepne zpet na eglrender - vse v render vlakne (zvuk se nezdrzuje).
// =====================================================================
//  A14: NAPOJENI EMULATORU NA APPKU (jeden zdroj obrazu)
//  Appka si bere obraz pres ps1GrabFrame, ktery cte g_frame_argb/g_fw/g_fh.
//  Ty ale plni JEN nap_video - softwarovy libretro callback, ktery v ceste A
//  (obraz jde pres GPU) zadna data nedostane. Proto appce chodilo gfw=0 gfh=0,
//  tedy NIC - a TV wiever pak neme co ukazat. Tady dame appce PRESNE TENTYZ
//  snimek, ktery jde na displej. Jeden zdroj, zadna druha cesta.
//  Prevod: pixely z cesty A jsou RGBA a zdola nahoru (GL), appka chce ARGB
//  shora dolu - proto otoceni radku a prohozeni R/B.
static void nap_publish_frame_for_app(const uint8_t* rgba, int w, int h) {
    if (!rgba || w <= 0 || h <= 0) return;
    std::lock_guard<std::mutex> lock(g_frame_mutex);
    g_frame_argb.resize((size_t)w * (size_t)h);
    for (int y = 0; y < h; y++) {
        const uint8_t* src = rgba + (size_t)(h - 1 - y) * (size_t)w * 4;  // otoceni radku
        uint32_t* dst = g_frame_argb.data() + (size_t)y * (size_t)w;
        for (int x = 0; x < w; x++) {
            dst[x] = 0xFF000000u | ((uint32_t)src[0] << 16)   // R
                                 | ((uint32_t)src[1] << 8)    // G
                                 |  (uint32_t)src[2];         // B
            src += 4;
        }
    }
    g_fw.store(w); g_fh.store(h);
    g_frames.fetch_add(1);
}

extern "C" const void* nap_ps1_egl_grab_pixels(int* w, int* h) {
    // Zadne GL, zadne prepinani kontextu, zadne cekani na GPU. Jen si vezmeme
    // posledni snimek, ktery vlakno emulace uz dokoncilo.
    int idx = g_frame_ready.load(std::memory_order_acquire);
    if (idx < 0) return NULL;
    std::lock_guard<std::mutex> lk(g_frame_swap);
    if (w) *w = g_frame_w.load();
    if (h) *h = g_frame_h.load();
    if (g_frame_buf[idx].empty()) return NULL;
    return g_frame_buf[idx].data();

}

// CESTA A: prepnuti kontextu, ktere vola RENDER vlakno (egl_main) - NE tick.
// Diky tomu se retro_run (co plni zvukovou frontu) nezdrzuje a zvuk neskrece.
// bind_core: pred retro_run+kreslenim gpu-gles do canvasu.
// bind_render: pred kreslenim vysledne textury na okno eglrenderu.
extern "C" void nap_ps1_egl_bind_core(void) {
    if (g_gles_ready && g_gles_display_A != EGL_NO_DISPLAY)
        eglMakeCurrent(g_gles_display_A, g_gles_surface_A, g_gles_surface_A, g_gles_context_A);
}
extern "C" void nap_ps1_egl_bind_render(void) {
    if (g_egl_render_ctx != EGL_NO_CONTEXT && g_gles_display_A != EGL_NO_DISPLAY)
        eglMakeCurrent(g_gles_display_A, g_egl_render_surf, g_egl_render_surf, g_egl_render_ctx);
}
extern "C" int nap_ps1_egl_vram_w(void) { return nap_gles_vram_w(); }
extern "C" int nap_ps1_egl_vram_h(void) { return nap_gles_vram_h(); }

// Ne-JNI wrappery, aby je eglrender (C) mohl volat pres dlsym primo,
// bez JNIEnv. Delaji totez co JNI verze vyse.
extern "C" int nap_ps1_egl_boot_c(const char* sys, const char* game) {
    // Kdyz uz bezi BIOS (start bez disku), musime ho korektne zastavit DRIV,
    // nez zacneme nacitat hru - jinak by vlakno emulace tocilo nad uvolnenym
    // jadrem a spadlo by to.
    if (g_core_run.exchange(false) && g_core_thread.joinable()) {
        nap_diag_log("PS1: zastavuji bezici BIOS, nacitam hru");
        g_core_thread.join();
        nap_sl_close();
        nap_audio_clear();
        if (g_loaded.exchange(false)) { retro_unload_game(); retro_deinit(); }
    }
    // ===== UKLIDIT GRAFIKU PO BIOSU =====
    // BIOS bezel s VLASTNIM (headless) grafickym kontextem. Kdyz se ted
    // rozjizdi hra, vytvori se kontext novy - ale renderer si porad mysli,
    // ze je hotovy, protoze jeho ramec a textury patri tomu STAREMU kontextu.
    // Kreslilo se pak do neceho, co v novem kontextu neexistuje => cerna
    // obrazovka. Proto stary kontext i renderer poradne zrusime a hra si
    // grafiku postavi od zacatku - presne jako kdyz zadny BIOS nebezel.
    if (g_gles_ready) {
        void n2_finish(void);
        if (g_gles_display_A != EGL_NO_DISPLAY) {
            eglMakeCurrent(g_gles_display_A, g_gles_surface_A, g_gles_surface_A, g_gles_context_A);
            n2_finish();
            eglMakeCurrent(g_gles_display_A, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
            if (g_gles_surface_A != EGL_NO_SURFACE) eglDestroySurface(g_gles_display_A, g_gles_surface_A);
            if (g_gles_context_A != EGL_NO_CONTEXT) eglDestroyContext(g_gles_display_A, g_gles_context_A);
        }
        g_gles_surface_A = EGL_NO_SURFACE;
        g_gles_context_A = EGL_NO_CONTEXT;
        g_gles_ready = false;
        nap_diag_log("PS1: grafika po BIOSu uklizena, hra si ji postavi znovu");
    }
    nap_diag_log("=== NEOCONTR B37 31-07-2026 (verzi hleda v radku VERZE APKY) ===");
    nap_install_crash_handler(); // od tohohle bodu zachytime pripadny pad
    // A11: minuly pad server nestihl ukazat (umrel s procesem) a hlavni log se
    // pri restartu smazal - ale ulozili jsme ho do samostatneho souboru. Tady ho
    // VYTAHNEME do cerstveho /8765/log, at ho Rene po restartu + spusteni PS1 uvidi.
    if (g_crash_last_path[0]) {
        int cfd = open(g_crash_last_path, O_RDONLY);
        if (cfd >= 0) {
            char cb[512]; ssize_t rd = read(cfd, cb, sizeof(cb) - 1); close(cfd);
            if (rd > 0) { cb[(size_t)rd] = '\0'; nap_diag_log("=== PREDCHOZI PAD (z minuleho behu; server ho zive nestihl ukazat):%s===", cb); }
            unlink(g_crash_last_path);
        }
    }
    if (g_loaded.exchange(false)) { retro_unload_game(); retro_deinit(); }
    g_sysdir = sys ? sys : "";
    g_input_bits.store(0);
    g_boot_error.clear();
    g_gles_ready = nap_gles_egl_init();
    if (!g_gles_ready) NAPDIAG("CESTA_A GLES_INIT_FAIL v eglrender vlakne");
    retro_set_environment(nap_env);
    retro_set_video_refresh(nap_video);
    retro_set_audio_sample(nap_audio_sample);
    retro_set_audio_sample_batch(nap_audio_batch);
    retro_set_input_poll(nap_input_poll);
    retro_set_input_state(nap_input_state);
    retro_init();
    retro_game_info gi; memset(&gi, 0, sizeof(gi));
    gi.path = game ? game : "";
    if (!retro_load_game(&gi)) {
        NAPDIAG("CESTA_A retro_load_game FAILED path=%s", game ? game : "?");
        retro_deinit();
        return -1;
    }
    g_loaded.store(true);
    nap_srm_set_path(game ? game : "");
    nap_srm_load();
    retro_system_av_info av; memset(&av, 0, sizeof(av));
    retro_get_system_av_info(&av);
    g_fps = av.timing.fps > 1 ? av.timing.fps : 60.0;
    nap_sl_open(); // CESTA A: spustit OpenSL zvuk (bere z g_afifo, bez Javy)
    NAPDIAG("CESTA_A BOOT_OK gpuGles=%s fps=%.2f zvukJadra=%.0f Hz vystup=44100 Hz",
            g_gles_ready ? "ANO" : "NE", g_fps, av.timing.sample_rate);
    return g_gles_ready ? 1 : 0;
}
// Kolik zvuku ceka ve fronte (v milisekundach). Pouziva to renderer:
// kdyz zvuk dochazi, preskoci na jeden snimek drahe cteni obrazu z GPU
// a necha jadro bezet naplno - lepsi je zadrhnout obraz nez zvuk.
extern "C" int nap_audio_level_ms(void) {
  unsigned n = nap_aring_avail();
  return (int)((n / 2) * 1000 / 44100);
}

// Vlakno emulace: drzi si kontext jadra, krokuje, kresli a precte snimek.
static void nap_core_thread_fn(void) {
    if (g_gles_display_A != EGL_NO_DISPLAY)
        eglMakeCurrent(g_gles_display_A, g_gles_surface_A, g_gles_surface_A, g_gles_context_A);
    // Renderer musi byt pripraveny. Kdyz se jadro mezitim odinicializovalo
    // (napr. neuspesny start), zrusil se s nim i renderer - a bez tehle
    // pojistky by se uz nikdy nevytvoril a kreslilo by se do niceho
    // (na obrazovce z toho byly svisle zelene cary).
    // n2_init() si sam hlida, jestli uz hotovo je, takze je to levne.
    { int rc = n2_init(); if (rc != 0) nap_diag_log("NAPLES2: renderer se nepodarilo pripravit (kod %d)", rc); }
    nap_diag_log("CESTA_A EMULACE MA VLASTNI VLAKNO (kresleni ji uz nebrzdi)");
    int slot = 0;
    while (g_core_run.load(std::memory_order_relaxed)) {
        if (!g_loaded.load()) { usleep(2000); continue; }

        if (nap_audio_skip_step()) {      // zvuku dost - chvilku pockat
            usleep(1500);                 // tady uspani NEVADI, nejsme na vlakne kresleni
            continue;
        }
        g_crash_stage = "retro_run(vlakno emulace)";
        retro_run();
        g_crash_stage = "present(vlakno emulace)";
        if (g_gles_ready) { nap_gles_sync_display_settings(); nap_gles_present_frame(); }

        // precist hotovy snimek do volneho slotu
        g_crash_stage = "cteni snimku(vlakno emulace)";
        {
            int w = 0, h = 0;
            const void *px = g_gles_ready ? nap_gles_grab_pixels(&w, &h) : NULL;
            if (px && w > 0 && h > 0) {
                size_t need = (size_t)w * (size_t)h * 4;
                if (g_frame_buf[slot].size() < need) g_frame_buf[slot].resize(need);
                memcpy(g_frame_buf[slot].data(), px, need);
                { std::lock_guard<std::mutex> lk(g_frame_swap);
                  g_frame_w.store(w); g_frame_h.store(h);
                  g_frame_ready.store(slot, std::memory_order_release); }
                slot ^= 1;
                nap_publish_frame_for_app((const uint8_t*)g_frame_buf[slot ^ 1].data(), w, h);
            }
        }
    }
    if (g_gles_display_A != EGL_NO_DISPLAY)
        eglMakeCurrent(g_gles_display_A, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
}

// ==================================================================
//  VYPNUTI JADRA PRI ZAVRENI OKNA HRY
//  Chybelo uplne. Kdyz se zavrelo nativni okno, eglrender uklidil jen sebe -
//  ale jadro PS1 bezelo dal: vlakno emulace poracovalo, OpenSL prehravac
//  hral, hra zustala nactena. Odtud "po vyskoceni z emulatoru hraje zvuk dal
//  a PS1 se nevypne" a pri dalsim spusteni pribyval dalsi bezici zvuk.
// ==================================================================
// ==================================================================
//  START PS1 BEZ DISKU, BEZ CELOOBRAZOVKOVEHO OKNA
//  Chova se jako skutecna PlayStation po zapnuti bez CD: nabehne BIOS
//  a jeho menu (MEMORY CARD / CD PLAYER). Obraz jde do monitoru v appce.
//  Grafika si vytvari VLASTNI displej i kreslici plochu (pbuffer), takze
//  zadne okno hry k tomu neni potreba.
// ==================================================================
extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootBios(JNIEnv *env, jclass,
                                                        jstring jSys, jstring jSave) {
  const char *sys  = jSys  ? env->GetStringUTFChars(jSys,  nullptr) : nullptr;
  const char *save = jSave ? env->GetStringUTFChars(jSave, nullptr) : nullptr;
  std::string sysDir  = sys  ? sys  : "";
  std::string saveDir = save ? save : "";
  if (sys)  env->ReleaseStringUTFChars(jSys,  sys);
  if (save) env->ReleaseStringUTFChars(jSave, save);

  nap_diag_log("PS1 START BEZ DISKU: pripravuji BIOS (obraz pujde do monitoru)");

  // uklidit pripadny predchozi beh
  if (g_core_run.exchange(false) && g_core_thread.joinable()) g_core_thread.join();
  if (g_loaded.exchange(false)) { retro_unload_game(); retro_deinit(); }

  g_sysdir  = sysDir;
  g_savedir = saveDir;   /* BIOS pracuje s memory kartami */

  if (!g_gles_ready) g_gles_ready = nap_gles_egl_init();   // vlastni kontext, bez okna
  if (!g_gles_ready) {
    nap_diag_log("PS1 START BEZ DISKU: grafiku se nepodarilo pripravit");
    return env->NewStringUTF("PS1_BIOS_FAIL grafika");
  }

  retro_set_environment(nap_env);
  retro_set_video_refresh(nap_video);
  retro_set_audio_sample(nap_audio_sample);
  retro_set_audio_sample_batch(nap_audio_batch);
  retro_set_input_poll(nap_input_poll);
  retro_set_input_state(nap_input_state);
  retro_init();

  if (!retro_load_game(nullptr)) {          // bez disku -> BIOS menu
    retro_deinit();
    nap_diag_log("PS1 START BEZ DISKU: jadro odmitlo start bez disku");
    return env->NewStringUTF("PS1_BIOS_FAIL jadro");
  }
  g_loaded.store(true);

  retro_system_av_info av; memset(&av, 0, sizeof(av));
  retro_get_system_av_info(&av);
  g_fps = av.timing.fps > 1 ? av.timing.fps : 60.0;

  nap_sl_open();                            // zvuk BIOSu
  g_core_run.store(true);
  g_core_thread = std::thread(nap_core_thread_fn);

  nap_diag_log("PS1 START BEZ DISKU OK: bezi BIOS, fps=%.2f", g_fps);
  return env->NewStringUTF("PS1_BIOS_OK");
}

extern "C" void nap_ps1_egl_shutdown_c(void) {
    nap_diag_log("CESTA_A VYPINAM JADRO (zavreno okno hry)");
    // 1) zastavit vlakno emulace a pockat, az dobehne
    if (g_core_run.exchange(false) && g_core_thread.joinable()) g_core_thread.join();
    // 2) zastavit zvuk a vyprazdnit frontu
    nap_sl_close();
    nap_audio_clear();
    // 3) uvolnit hru a jadro
    if (g_loaded.exchange(false)) {
        nap_srm_save_if_dirty("vypnuti");
        retro_unload_game();
        retro_deinit();
    }
    g_gles_ready = false;
    g_frame_ready.store(-1);
    nap_diag_log("CESTA_A JADRO VYPNUTO (zvuk i emulace zastaveny)");
}

extern "C" void nap_ps1_egl_tick_c(void) {
    // Emulace bezi na vlastnim vlakne (nap_core_thread_fn), takze tady uz
    // nic nekrokujeme. Drive se tu volal retro_run() a tim byla emulace
    // svazana s vsyncem kresleni.
    if (!g_loaded.load()) return;
    if (!g_core_run.load(std::memory_order_relaxed)) {
        g_core_run.store(true);
        g_core_thread = std::thread(nap_core_thread_fn);
    }

}

extern "C" JNIEXPORT jstring JNICALL
Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(JNIEnv *env, jclass) {
  std::lock_guard<std::mutex> life(g_life_mutex);
  g_generation.fetch_add(1); g_running.store(false);
  // ukoncit vlakno emulace (cesta A) driv, nez uvolnime jadro
  if (g_core_run.exchange(false) && g_core_thread.joinable()) g_core_thread.join();
  if (g_worker.joinable()) g_worker.join();
  if (g_loaded.exchange(false)) { nap_srm_save_if_dirty("stop"); retro_unload_game(); retro_deinit(); }
  nap_audio_clear();
  g_input_bits.store(0);
  NAPLOG("BUILD2SA2 PS1 stop frames=%llu", (unsigned long long)g_frames.load());
  return env->NewStringUTF("PS1_STOPPED");
}
