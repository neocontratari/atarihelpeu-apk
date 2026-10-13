// B305: knihovna pro KONEC-KONCU test prohlizece (test_b305_prohlizec.py) - PS1.
//
// SKUTECNY nap_ps1_native.cpp: start pres skutecne JNI ps1BootDoMonitoru,
// skutecne vlakno emulace (nap_core_thread_fn), skutecny nap_video (prevod
// snimku), skutecny OpenSL callback (zvuk -> kopie pro TV -> B305 prenos).
// Nahrazene je jen to, co na PC neni:
//  - jadro PCSX (retro_*): misto hry snimky 3D sceny (PNG z ../b304/demo_snimky)
//    jako "hra na 30 snimku/s" - jadro posila 60 snimku/s, kazdy druhy je
//    duplikat (NULL jako skutecny PCSX s GET_CAN_DUPE), zvuk 735 vzorku na snimek,
//  - OpenSL ES: vlakno, ktere si jako zvukova karta bere blok 1024 vzorku kazdych
//    23,2 ms a vola zpetne volani appky (presne jako Android),
//  - vykreslovac GPU (n2_*, nap_gles_*): nic nekresli, snimek jde z nap_video,
//  - EGL / GLES: skutecne (Mesa, bez okna).
//
//   viz JAK_SPUSTIT.txt
#include "../../app/src/main/cpp/nap_ps1_native.cpp"
#include <dirent.h>
#include <algorithm>
#include <map>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "stb_image.h"

// ---------- vykreslovac GPU (v appce gpu_neon / NAPLES2) ----------
extern "C" {
int n2_init(void) { return 0; }
void n2_finish(void) {}
void nap_gles_present_frame(void) {}
void nap_gles_sync_display_settings(void) {}
unsigned nap_gles_grab_texture(int *, int *, int *, int *) { return 0; }
const void *nap_gles_grab_pixels(int *, int *) { return nullptr; }   // snimek jde z nap_video (jako s gpu_neon)
int nap_gles_vram_w(void) { return 1024; }
int nap_gles_vram_h(void) { return 512; }
int nap_zjisti_psx_region(char *b, int n) { if (b && n > 0) snprintf(b, (size_t)n, "test"); return 0; }
}

// ---------- okno (nepouziva se) ----------
struct ANativeWindow { int refs = 1; };
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *) { return 0; }
int32_t ANativeWindow_getHeight(ANativeWindow *) { return 0; }
int32_t ANativeWindow_getFormat(ANativeWindow *) { return 0; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t) { return 0; }
int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *) { return -1; }
int32_t ANativeWindow_unlockAndPost(ANativeWindow *) { return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject) { return nullptr; }

// ---------- OpenSL ES: zvukova karta, ktera si bere bloky v realnem case ----------
static const SLInterfaceID_ iidE{1}, iidP{2}, iidQ{3};
const SLInterfaceID SL_IID_ENGINE = &iidE;
const SLInterfaceID SL_IID_PLAY = &iidP;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &iidQ;
namespace sl {
static slAndroidSimpleBufferQueueCallback cb = nullptr;
static void *cbCtx = nullptr;
static std::thread karta;
static std::atomic<bool> hraje{false};
static std::atomic<long long> bloku{0};
static SLresult realize(SLObjectItf, SLboolean) { return SL_RESULT_SUCCESS; }
static void destroy(SLObjectItf);
static SLresult objGetInterface(SLObjectItf self, const SLInterfaceID iid, void *p);
static const SLObjectItf_ objTbl = {realize, objGetInterface, destroy};
static const SLObjectItf_ *engineObj = &objTbl, *mixObj = &objTbl, *playerObj = &objTbl;
static SLresult createPlayer(SLEngineItf, SLObjectItf *p, SLDataSource *, SLDataSink *, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &playerObj; return SL_RESULT_SUCCESS; }
static SLresult createMix(SLEngineItf, SLObjectItf *p, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &mixObj; return SL_RESULT_SUCCESS; }
static const SLEngineItf_ engTbl = {createPlayer, createMix};
static const SLEngineItf_ *engItf = &engTbl;
static void spustKartu();
static SLresult setPlayState(SLPlayItf, SLuint32 st) { if (st == SL_PLAYSTATE_PLAYING) spustKartu(); return SL_RESULT_SUCCESS; }
static const SLPlayItf_ playTbl = {setPlayState};
static const SLPlayItf_ *playItf = &playTbl;
static SLresult enqueue(SLAndroidSimpleBufferQueueItf, const void *, SLuint32) { return SL_RESULT_SUCCESS; }
static SLresult registerCb(SLAndroidSimpleBufferQueueItf, slAndroidSimpleBufferQueueCallback c, void *ctx) { cb = c; cbCtx = ctx; return SL_RESULT_SUCCESS; }
static const SLAndroidSimpleBufferQueueItf_ qTbl = {enqueue, registerCb};
static const SLAndroidSimpleBufferQueueItf_ *qItf = &qTbl;
static SLresult objGetInterface(SLObjectItf, const SLInterfaceID iid, void *p) {
  if (iid == SL_IID_ENGINE) *(SLEngineItf *)p = &engItf;
  else if (iid == SL_IID_PLAY) *(SLPlayItf *)p = &playItf;
  else if (iid == SL_IID_ANDROIDSIMPLEBUFFERQUEUE) *(SLAndroidSimpleBufferQueueItf *)p = &qItf;
  else return 1;
  return SL_RESULT_SUCCESS;
}
static void spustKartu() {
  if (hraje.exchange(true)) return;
  karta = std::thread([] {
    auto t = std::chrono::steady_clock::now();
    while (hraje.load()) {
      t += std::chrono::microseconds(1024 * 1000000LL / 44100);    // blok 1024 vzorku dohral
      std::this_thread::sleep_until(t);
      if (cb && hraje.load()) { cb(&qItf, cbCtx); bloku++; }
    }
  });
}
static void destroy(SLObjectItf self) {
  if (self == &playerObj && hraje.exchange(false) && karta.joinable()) karta.join();
}
}  // namespace sl
SLresult slCreateEngine(SLObjectItf *p, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &sl::engineObj; return SL_RESULT_SUCCESS; }

// ---------- jadro PS1 (libretro): snimky 3D sceny jako hra na 30 snimku/s ----------
namespace hra {
static retro_environment_t env = nullptr;
static retro_video_refresh_t video = nullptr;
static retro_audio_sample_batch_t zvuk = nullptr;
static std::vector<std::vector<uint32_t>> snimky;   // XRGB8888
static int W = 0, H = 0;
static unsigned long long beh = 0, zvukN = 0;
static std::atomic<bool> ctverec{false}, stuj{false};
static std::mutex mOcek;
static std::map<unsigned, std::vector<uint32_t>> ocek;   // cislo -> snimek (jak ho jadro poslalo)
static unsigned cislo = 0;
static std::vector<uint32_t> akt;
static void vlozCislo(std::vector<uint32_t> &F, unsigned n) {   // 16 bitu jako ctverecky 4x4 (preziji JPEG)
  for (int b = 0; b < 16; b++)
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++) F[(size_t)((b / 4) * 4 + y) * W + (b % 4) * 4 + x] = ((n >> b) & 1) ? 0xFFFFFFu : 0x000000u;
}
}  // namespace hra
extern "C" {
void retro_set_environment(retro_environment_t cb) { hra::env = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { hra::video = cb; }
void retro_set_audio_sample(retro_audio_sample_t) {}
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { hra::zvuk = cb; }
void retro_set_input_poll(retro_input_poll_t) {}
void retro_set_input_state(retro_input_state_t) {}
void retro_init(void) {}
void retro_deinit(void) {}
unsigned retro_api_version(void) { return 1; }
void retro_get_system_info(struct retro_system_info *i) { std::memset(i, 0, sizeof(*i)); i->library_name = "test"; }
void retro_get_system_av_info(struct retro_system_av_info *av) {
  std::memset(av, 0, sizeof(*av));
  av->timing.fps = 59.94; av->timing.sample_rate = 44100;
  av->geometry.base_width = (unsigned)hra::W; av->geometry.base_height = (unsigned)hra::H; av->geometry.aspect_ratio = 4.f / 3.f;
}
bool retro_load_game(const struct retro_game_info *) {
  int f = PIXFMT_XRGB8888;
  if (hra::env) hra::env(ENV_SET_PIXEL_FORMAT, &f);
  return true;
}
void retro_unload_game(void) {}
void retro_run(void) {
  using namespace hra;
  beh++;
  // zvuk jednoho snimku (44100 / 60): pila L / R
  int16_t b[735 * 2];
  for (int i = 0; i < 735; i++, zvukN++) { b[2 * i] = (int16_t)(((zvukN * 41) % 14000) - 7000); b[2 * i + 1] = (int16_t)(((zvukN * 29) % 10000) - 5000); }
  if (zvuk) zvuk(b, 735);
  if (!video || snimky.empty()) return;
  if ((beh & 1) == 0 && !akt.empty()) { video(nullptr, (unsigned)W, (unsigned)H, 0); return; }   // duplikat (hra na 30/s)
  if (!stuj.load() || akt.empty()) {
    cislo++;
    akt = snimky[(cislo / 1) % snimky.size()];
    vlozCislo(akt, cislo);
  }
  std::vector<uint32_t> F = akt;
  if (ctverec.load()) for (int y = 64; y < 112; y++) for (int x = 96; x < 176; x++) F[(size_t)y * W + x] = 0xFFFFFFu;
  {
    std::lock_guard<std::mutex> l(mOcek);
    ocek[cislo & 0xFFFF] = F;
    if (ocek.size() > 300) ocek.erase(ocek.begin());
  }
  video(F.data(), (unsigned)W, (unsigned)H, (size_t)W * 4);
}
void *retro_get_memory_data(unsigned) { return nullptr; }
size_t retro_get_memory_size(unsigned) { return 0; }
size_t retro_serialize_size(void) { return 0; }
bool retro_serialize(void *, size_t) { return false; }
bool retro_unserialize(const void *, size_t) { return false; }
}

// ---------- rucne sestaveny JNIEnv ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, (size_t)n); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }

extern "C" {
// adresar s PNG snimky 3D sceny; vraci odpoved ps1BootDoMonitoru ("PS1_BIOS_OK")
const char *ps1_start(const char *adr, const char *pracovni) {
  thread_local std::string r;
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  std::vector<std::string> v;
  if (DIR *d = opendir(adr)) { while (dirent *e = readdir(d)) { std::string s = e->d_name; if (s.size() > 4 && s.substr(s.size() - 4) == ".png") v.push_back(std::string(adr) + "/" + s); } closedir(d); }
  std::sort(v.begin(), v.end());
  for (auto &f : v) {
    int w, h, n; unsigned char *p = stbi_load(f.c_str(), &w, &h, &n, 3);
    if (!p) continue;
    std::vector<uint32_t> F((size_t)w * h);
    for (size_t i = 0; i < F.size(); i++) F[i] = ((uint32_t)p[i * 3] << 16) | ((uint32_t)p[i * 3 + 1] << 8) | p[i * 3 + 2];
    stbi_image_free(p);
    hra::W = w; hra::H = h;
    hra::snimky.push_back(F);
  }
  if (hra::snimky.empty()) { r = "ZADNE SNIMKY"; return r.c_str(); }
  FakeStr logCesta{std::string(pracovni) + "/ps1_diag.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&logCesta);
  FakeStr sys{pracovni}, sav{pracovni}, hraCesta{""};
  r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&sys, (jstring)&sav, (jstring)&hraCesta));
  return r.c_str();
}
void ps1_stop() { nap_ps1_egl_shutdown_c(); }
void ps1_ctverec(int z) { hra::ctverec = z != 0; }
void ps1_stuj(int z) { hra::stuj = z != 0; }
long long ps1_bloku_zvuku() { return sl::bloku.load(); }
int ps1_rozmer() { return (hra::W << 16) | hra::H; }
// ocekavany snimek podle cisla vyctene z obrazu v prohlizeci
int ps1_ocekavany(unsigned cislo, uint32_t *out) {
  std::lock_guard<std::mutex> l(hra::mOcek);
  auto it = hra::ocek.find(cislo & 0xFFFF);
  if (it == hra::ocek.end()) return 0;
  std::memcpy(out, it->second.data(), it->second.size() * 4);
  return (hra::W << 16) | hra::H;
}
// /ps1.stream -> proudOtevri / Dalsi / Zavri (Java napTvWebJadroProud)
int ps1_otevri() { return Java_eu_atarihelp_emu10_NativePs1CoreBridge_proudOtevriNative(&g_env, nullptr); }
int ps1_dalsi(int id, uint8_t *out, int kap, int cekejMs, int zapisMs) {
  thread_local FakeArr a;
  if ((int)a.b.size() != kap) a.b.assign((size_t)kap, 0);
  const jint n = Java_eu_atarihelp_emu10_NativePs1CoreBridge_proudDalsiNative(&g_env, nullptr, id, (jbyteArray)&a, cekejMs, zapisMs);
  if (n > 0) std::memcpy(out, a.b.data(), (size_t)n);
  return n;
}
void ps1_zavri(int id) { Java_eu_atarihelp_emu10_NativePs1CoreBridge_proudZavriNative(&g_env, nullptr, id); }
int ps1_stav() { return Java_eu_atarihelp_emu10_NativePs1CoreBridge_proudStavNative(&g_env, nullptr); }
const char *ps1_log() { thread_local std::string r; r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_proudLogNative(&g_env, nullptr)); return r.c_str(); }
}

// JVM test (jvm/TestPodpisu.java): knihovna nactena skutecnou tridou NativePs1CoreBridge
// - "hra" se rozbehne sama (adresar se snimky v NAP_JVM_PNG)
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
  if (const char *d = getenv("NAP_JVM_PNG")) { ps1_start(d, getenv("NAP_JVM_PRAC") ? getenv("NAP_JVM_PRAC") : "."); atexit(ps1_stop); }
  return JNI_VERSION_1_6;
}
