// B308: knihovna pro testy DENIKU PS1 a KLAVESNICE Z TV (test_ps1_denik.py,
// test_b308_klavesnice.py).
//
// SKUTECNY nap_ps1_native.cpp: start pres skutecne JNI ps1BootDoMonitoru, skutecne
// vlakno emulace (nap_core_thread_fn: denik, zdravi, tlacitka, hlidac klavesnice,
// zamek kolem retro_run), skutecny OpenSL callback, skutecne JNI ps1PcKlavesa,
// ps1Denik, ps1SaveState / ps1LoadState, ps1Stop.
// Nahrazene je jen to, co na PC neni:
//  - jadro PCSX (retro_*): "hra" 320x240 na 60 snimku/s, zvuk 735 vzorku na snimek,
//    hlasky jadra pres zpetne volani logu (jako PCSX pri startu disku), cteni
//    tlacitek pres retro_input_state (co jadro skutecne vidi), pametova karta,
//    ulozeni pozice (hlida, jestli nebezi soubezne s retro_run),
//  - popisy z jadra (libretro.c / nap_most.c): stejne funkce, test meni stav obrazu,
//  - OpenSL ES: vlakno jako zvukova karta (blok 1024 vzorku kazdych 23,2 ms),
//  - vykreslovac GPU (n2_*, nap_gles_*), EGL / GLES: skutecne (Mesa, bez okna).
#include "../../app/src/main/cpp/nap_ps1_native.cpp"
#include <cmath>
#include <algorithm>

// ---------- vykreslovac GPU (v appce gpu_neon) ----------
extern "C" {
int n2_init(void) { return 0; }
void n2_finish(void) {}
void nap_gles_present_frame(void) {}
void nap_gles_sync_display_settings(void) {}
unsigned nap_gles_grab_texture(int *, int *, int *, int *) { return 0; }
const void *nap_gles_grab_pixels(int *, int *) { return nullptr; }
int nap_gles_vram_w(void) { return 1024; }
int nap_gles_vram_h(void) { return 512; }
int nap_zjisti_psx_region(char *b, int n) { if (b && n > 0) snprintf(b, (size_t)n, "scph5501.bin"); return 0; }
}

// ---------- popisy z jadra (v appce libretro.c a nap_most.c) ----------
namespace gpustav {
static std::atomic<int> hres{320}, vres{240}, rgb24{0}, pal{0}, enh{1};
static std::atomic<unsigned> tenke{0};
}
extern "C" {
int nap_ps1_jadro_popis(char *buf, int len) {
  return snprintf(buf, (size_t)len, "BIOS scph5501.bin (soubor) | region USA, NTSC 60 Hz | procesor dynarec (rychly) | "
                  "disk ID \"SLUS00077\" nazev \"DOOM\" | XA ANO, CD audio ANO, pomaly start 0");
}
int nap_ps1_nastaveni_popis(char *buf, int len) {
  return snprintf(buf, (size_t)len, "2x (Enhanced Resolution) ZAPNUTO, bez 1x obrazu NE, uprava textur ANO | prokladani AUTO");
}
unsigned int nap_ps1_gpu_otisk(void) {
  using namespace gpustav;
  return (unsigned)hres * 7919u + (unsigned)vres * 31u + (unsigned)rgb24 * 3u + (unsigned)pal * 5u + (unsigned)enh;
}
int nap_ps1_gpu_popis(char *buf, int len) {
  using namespace gpustav;
  return snprintf(buf, (size_t)len, "obraz %dx%d, %s, %s | 2x %s", (int)hres, (int)vres, rgb24 ? "24 bit (video)" : "15 bit",
                  pal ? "PAL" : "NTSC", enh ? "ANO" : "NE");
}
unsigned int nap_ps1_b308_tenkych(void) { return gpustav::tenke.load(); }
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
      t += std::chrono::microseconds(1024 * 1000000LL / 44100);
      std::this_thread::sleep_until(t);
      if (cb && hraje.load()) cb(&qItf, cbCtx);
    }
  });
}
static void destroy(SLObjectItf self) {
  if (self == &playerObj && hraje.exchange(false) && karta.joinable()) karta.join();
}
}  // namespace sl
SLresult slCreateEngine(SLObjectItf *p, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &sl::engineObj; return SL_RESULT_SUCCESS; }

// ---------- jadro PS1 (libretro) ----------
namespace hra {
static retro_environment_t env = nullptr;
static retro_video_refresh_t video = nullptr;
static retro_audio_sample_batch_t zvuk = nullptr;
static retro_input_state_t vstup = nullptr;
static retro_log_callback logcb{nullptr};
static std::atomic<unsigned> bityJadra{0};         // co jadro naposledy precetlo z ovladace
static std::atomic<long long> behu{0};
static std::atomic<int> uvnitrRun{0}, soubeznych{0}, ulozeni{0};
static std::atomic<int> zpomal{0};                  // ms navic v kazdem retro_run (pomaly telefon)
static std::atomic<int> hlasekNavic{0};             // kolik hlasek jadra poslat v pristim retro_run
static std::vector<uint32_t> snimek(320 * 240);
static uint8_t karta[128 * 1024];
static unsigned long long zvukN = 0;
}  // namespace hra
extern "C" {
void retro_set_environment(retro_environment_t cb) { hra::env = cb; }
void retro_set_video_refresh(retro_video_refresh_t cb) { hra::video = cb; }
void retro_set_audio_sample(retro_audio_sample_t) {}
void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) { hra::zvuk = cb; }
void retro_set_input_poll(retro_input_poll_t) {}
void retro_set_input_state(retro_input_state_t cb) { hra::vstup = cb; }
void retro_init(void) {}
void retro_deinit(void) {}
unsigned retro_api_version(void) { return 1; }
void retro_get_system_info(struct retro_system_info *i) { std::memset(i, 0, sizeof(*i)); i->library_name = "test"; }
void retro_get_system_av_info(struct retro_system_av_info *av) {
  std::memset(av, 0, sizeof(*av));
  av->timing.fps = 59.94; av->timing.sample_rate = 44100;
  av->geometry.base_width = 320; av->geometry.base_height = 240; av->geometry.max_width = 1024; av->geometry.max_height = 512;
  av->geometry.aspect_ratio = 4.f / 3.f;
}
bool retro_load_game(const struct retro_game_info *gi) {
  int f = PIXFMT_XRGB8888;
  if (hra::env) { hra::env(ENV_SET_PIXEL_FORMAT, &f); hra::env(ENV_GET_LOG_INTERFACE, &hra::logcb); }
  // jako PCSX pri startu disku (libretro.c SysPrintf -> log_cb INFO)
  if (hra::logcb.log) {
    hra::logcb.log(1, "CD-ROM Label: DOOM                            \n");
    hra::logcb.log(1, "CD-ROM ID: SLUS00077\n");
    hra::logcb.log(0, "ladici hlaska - do deniku nema\n");
    hra::logcb.log(2, "warning: test varovani jadra\n");
  }
  if (gi && gi->path && std::strstr(gi->path, "ODMITNOUT")) return false;
  return true;
}
void retro_unload_game(void) {}
void retro_run(void) {
  using namespace hra;
  if (uvnitrRun.exchange(1) != 0) soubeznych++;
  behu++;
  unsigned b = 0;
  if (vstup) for (unsigned i = 0; i < 16; i++) if (vstup(0, 1, 0, i)) b |= 1u << i;
  bityJadra = b;
  int16_t s[735 * 2];
  for (int i = 0; i < 735; i++, zvukN++) { s[2 * i] = s[2 * i + 1] = (int16_t)(3000 * std::sin(6.2831853 * 440.0 * (double)zvukN / 44100.0)); }
  if (zvuk) zvuk(s, 735);
  for (size_t i = 0; i < snimek.size(); i++) snimek[i] = (uint32_t)(i * 2654435761u + (unsigned)behu);
  if (video) video(snimek.data(), 320, 240, 320 * 4);
  for (int n = hlasekNavic.exchange(0); n > 0; n--) if (logcb.log) logcb.log(1, "hlaska jadra behem hry %d\n", n);
  if (const int z = zpomal.load()) std::this_thread::sleep_for(std::chrono::milliseconds(z));
  uvnitrRun = 0;
}
void *retro_get_memory_data(unsigned id) { return id == 0 ? hra::karta : nullptr; }
size_t retro_get_memory_size(unsigned id) { return id == 0 ? sizeof(hra::karta) : 0; }
size_t retro_serialize_size(void) { return 4096; }
bool retro_serialize(void *d, size_t n) {
  if (hra::uvnitrRun.load()) hra::soubeznych++;    // ulozeni UPROSTRED kroku emulace = chyba
  std::this_thread::sleep_for(std::chrono::milliseconds(2));
  if (hra::uvnitrRun.load()) hra::soubeznych++;
  std::memset(d, 0x5A, n);
  hra::ulozeni++;
  return true;
}
bool retro_unserialize(const void *, size_t) {
  if (hra::uvnitrRun.load()) hra::soubeznych++;
  return true;
}
}

// ---------- rucne sestaveny JNIEnv ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, (size_t)n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.assign((size_t)n, 0); return (jbyteArray)a; }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }
static void pripravEnv() {
  static bool hotovo = false;
  if (hotovo) return;
  hotovo = true;
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.NewByteArray = fNewByteArray;
  g_env.functions = &g_tbl;
}

extern "C" {
// prac = slozka appky (BIOS, karta, log), hra = cesta ke hre ("" = BIOS)
const char *h_start(const char *prac, const char *hraCesta) {
  thread_local std::string r;
  pripravEnv();
  FakeStr logCesta{std::string(prac) + "/nap_tv_session_log.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&logCesta);
  FakeStr sys{prac}, sav{prac}, h{hraCesta ? hraCesta : ""};
  r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&sys, (jstring)&sav, (jstring)&h));
  return r.c_str();
}
const char *h_stop() {
  thread_local std::string r;
  pripravEnv();
  r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(&g_env, nullptr));
  nap_sl_close();
  return r.c_str();
}
// klavesa z prohlizece -> odpoved C++ (to, co jde prohlizeci)
const char *h_klavesa(const char *code, int dolu) {
  thread_local std::string r;
  pripravEnv();
  FakeStr c{code ? code : ""};
  r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PcKlavesa(&g_env, nullptr, (jstring)&c, dolu ? JNI_TRUE : JNI_FALSE));
  return r.c_str();
}
void h_dotyk(int id, int dolu) { pripravEnv(); Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetInput(&g_env, nullptr, id, dolu ? JNI_TRUE : JNI_FALSE); }
unsigned h_bity_jadra() { return hra::bityJadra.load(); }
long long h_behu() { return hra::behu.load(); }
int h_soubeznych() { return hra::soubeznych.load(); }
int h_ulozeni() { return hra::ulozeni.load(); }
void h_zpomal(int ms) { hra::zpomal = ms; }
void h_hlasky(int n) { hra::hlasekNavic = n; }
void h_obraz(int hres, int vres, int rgb24, int pal, int enh) {
  gpustav::hres = hres; gpustav::vres = vres; gpustav::rgb24 = rgb24; gpustav::pal = pal; gpustav::enh = enh;
}
void h_tenke(unsigned pridat) { gpustav::tenke += pridat; }
const char *h_pozice(const char *cesta, int uloz) {
  thread_local std::string r;
  pripravEnv();
  FakeStr c{cesta ? cesta : ""};
  r = jstr(uloz ? Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SaveState(&g_env, nullptr, (jstring)&c)
                : Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1LoadState(&g_env, nullptr, (jstring)&c));
  return r.c_str();
}
// denik (JNI ps1Denik - presne to, co dostane Java)
const char *h_denik() {
  thread_local std::string r;
  pripravEnv();
  FakeArr *a = (FakeArr *)Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Denik(&g_env, nullptr);
  r.assign(a->b.begin(), a->b.end());
  delete a;
  return r.c_str();
}
}
