// B328: MERENI VYKONU PS1 SE SKUTECNYM JADREM - JEN MERI, HRU NEMENI.
//
// Rene: "pridej to mereni ... nerad bych prisel o funkcni PS1 kvuli spongebob". Tenhle test dokazuje:
//  A) stejna hra, stejny pocet snimku: pamet PS1 (2 MB) je S merenim i BEZ nej bajt po bajtu stejna,
//     obaly se po dalsim startu neobali samy sebou (stejny pocet volani GPU), bez mereni se nic nepocita;
//  B) cesta telefonu (ps1BootDoMonitoru + vlakno emulace): v deniku je radek VYKON a jeho casti
//     (hra + kresleni + obraz + zvuk) daji dohromady cely snimek; radek JADRO pise XA a CD audio spravne.
// SKUTECNE: jadro PCSX-ReARMed z appky (frontend/libretro.c s obaly B328, dfsound, BIOS HLE),
// nap_ps1_native.cpp z appky. Nahrazene (jako B317): vykreslovac gpu_neon, OpenSL ES, okno Androidu.
// Hra = mys_test.exe z B317 (cte ovladace pres BIOS kazdy snimek).
//
// Preklad (PRAC/jadro = ../b317/stavba_jadra.sh PRAC):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/integrace_b328 integrace_b328_vykon.cpp PRAC/jadro/*.o \
//       -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b328 PRAC PRAC/mys_test.exe
#include "../../app/src/main/cpp/nap_ps1_native.cpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

// z jadra (frontend/libretro.c, libpcsxcore/pad.c): posun mysi za snimek a cteni portu 1 -
// test jimi zkousi, co hra dostane, kdyz port cte 2x v jednom snimku
extern "C" {
extern int in_mouse[8][2];
unsigned char PAD1_startPoll(void);
unsigned char PAD1_poll(unsigned char, int *);
}

// ---------- vykreslovac GPU (v appce gpu_neon + gpulib, ARM NEON) - grafika se tu nekresli ----------
extern "C" {
int n2_init(void) { return 0; }
void n2_finish(void) {}
void nap_gles_present_frame(void) {}
void nap_gles_sync_display_settings(void) {}
void nap_gles_readback_and_push(void) {}
unsigned nap_gles_grab_texture(int *, int *, int *, int *) { return 0; }
const void *nap_gles_grab_pixels(int *, int *) { return nullptr; }
int nap_gles_vram_w(void) { return 1024; }
int nap_gles_vram_h(void) { return 512; }
unsigned int nap_ps1_b308_tenkych(void) { return 0; }
unsigned int nap_ps1_gpu_otisk(void) { return 1; }
int nap_ps1_gpu_popis(char *buf, int len) { return snprintf(buf, (size_t)len, "obraz (test bez vykreslovace)"); }
// cip GPU: prikazy se zahodi, stav "pripraven" (hra / BIOS nikdy necekaji)
struct rearmed_cbs;
long GPUinit(void) { return 0; }
long GPUshutdown(void) { return 0; }
long GPUopen(unsigned long *, char *, char *) { return 0; }
long GPUclose(void) { return 0; }
uint32_t GPUreadStatus(void) { return 0x14802000u; }
uint32_t GPUreadData(void) { return 0; }
void GPUreadDataMem(uint32_t *p, int n) { if (p && n > 0) std::memset(p, 0, (size_t)n * 4); }
void GPUwriteStatus(uint32_t) {}
void GPUwriteData(uint32_t) {}
void GPUwriteDataMem(uint32_t *, int) {}
long GPUdmaChain(uint32_t *, uint32_t, uint32_t *, int32_t *) { return 0; }
long GPUfreeze(uint32_t, void *, uint16_t **) { return 1; }
void GPUvBlank(int, int) {}
void GPUgetScreenInfo(int *y, int *base_hres) { if (y) *y = 0; if (base_hres) *base_hres = 320; }
void GPUrearmedCallbacks(const struct rearmed_cbs *) {}
}

// ---------- okno Androidu (nepouziva se) ----------
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

// ---------- OpenSL ES: zvukova karta jako v testu B308 ----------
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

// ---------- rucne sestaveny JNIEnv (jako B308) ----------
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
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.NewByteArray = fNewByteArray;
  g_env.functions = &g_tbl;
}

// ---------- JNI presne jako Java (NativePs1CoreBridge) ----------
static std::string j_mysNastav(int rezim, int citlivost) {
  return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysNastav(&g_env, nullptr, rezim, citlivost));
}
static std::string j_mysStav() { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysStav(&g_env, nullptr)); }
static int j_dotyk(int akce, int id, float x, float y) {
  return Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysDotyk(&g_env, nullptr, akce, id, x, y, 732.f, 412.f);
}
static void j_tlacitko(int id, bool dolu) { Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetInput(&g_env, nullptr, id, dolu ? JNI_TRUE : JNI_FALSE); }
static std::string j_denik() {
  FakeArr *a = (FakeArr *)Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Denik(&g_env, nullptr);
  std::string r(a->b.begin(), a->b.end());
  delete a;
  return r;
}
static std::string j_pcMys(int dx, int dy, int tl) { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PcMys(&g_env, nullptr, dx, dy, tl)); }

// ======================================================= B328 =======================================================
static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string t(unsigned long long v) { return std::to_string(v); }
static std::string ms(unsigned long long ns) { char b[32]; snprintf(b, sizeof(b), "%.1f ms", ns / 1e6); return b; }

// otisk cele pameti PS1 (2 MB RAM) - FNV-1a 64
static uint64_t otiskRam() {
  const uint8_t *ram = (const uint8_t *)retro_get_memory_data(2 /* RETRO_MEMORY_SYSTEM_RAM */);
  const size_t n = retro_get_memory_size(2);
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; ram && i < n; i++) { h ^= ram[i]; h *= 1099511628211ull; }
  return h;
}
struct Mereni { unsigned long long gpu = 0, vout = 0, spu = 0, vol = 0; };
static Mereni mereni() { Mereni m; nap_mereni_stav(&m.gpu, &m.vout, &m.spu, &m.vol); return m; }
// presne N kroku jadra (bez casovani zvuku - stejny pocet snimku pokazde)
static void kroky(int n) { for (int i = 0; i < n; i++) { std::lock_guard<std::mutex> l(g_core_mutex); retro_run(); } }

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b328 PRAC mys_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B328 - mereni vykonu PS1 se SKUTECNYM jadrem (PCSX-ReARMed z appky, BIOS HLE, hra mys_test.exe)\n");
  const std::string sys = prac + "/system", karta = prac + "/karta_b328";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log_b328.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int N = 900;   // 15 s hry

  // ---- A1: start s merenim (jako v telefonu) ----
  int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mys_test.exe (ps1EglBoot = " + std::to_string(boot) + ")");
  std::string dn = j_denik();
  over(dn.find("MERENI VYKONU: zapnuto (13 / 13 funkci GPU a zvuku) - jen meri, hra se nemeni") != std::string::npos,
       "denik: \"MERENI VYKONU: zapnuto (13 / 13 funkci GPU a zvuku) - jen meri, hra se nemeni\"");
  Mereni a0 = mereni();
  kroky(N);
  Mereni a1 = mereni();
  const uint64_t s1 = otiskRam();
  over(a1.vol - a0.vol >= (unsigned long long)N && a1.gpu > a0.gpu && a1.spu > a0.spu,
       "s merenim: " + std::to_string(N) + " snimku = " + t(a1.vol - a0.vol) + " volani GPU (" + ms(a1.gpu - a0.gpu) + "), zvuk " + ms(a1.spu - a0.spu));
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  // ---- A2: znovu start, zase s merenim: obaly se nesmi obalit samy sebou ----
  boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  Mereni b0 = mereni();
  kroky(N);
  Mereni b1 = mereni();
  const uint64_t s2 = otiskRam();
  over(boot >= 0 && s2 == s1, "druhy start, stejny pocet snimku: pamet PS1 stejna (hra je deterministicka - test ma smysl)");
  over(b1.vol - b0.vol == a1.vol - a0.vol, "druhy start: stejny pocet volani GPU (" + t(b1.vol - b0.vol) + ") - obal neobalil sam sebe");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  // ---- A3: start, mereni VYPNUTE (puvodni funkce) - pamet musi byt stejna jako s merenim ----
  boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  nap_mereni_vypni();
  Mereni c0 = mereni();
  kroky(N);
  Mereni c1 = mereni();
  const uint64_t s3 = otiskRam();
  over(boot >= 0 && c1.vol == c0.vol && c1.gpu == c0.gpu && c1.spu == c0.spu, "bez mereni: nic se nepocita (puvodni funkce GPU a zvuku zpet)");
  over(s3 == s1, "BEZ mereni presne stejna pamet PS1 (2 MB) jako S merenim po " + std::to_string(N) + " snimcich - mereni hru nemeni");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  // ---- B: cesta telefonu - ps1BootDoMonitoru + vlakno emulace, denik ----
  FakeStr jkarta{karta};
  const std::string r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&jsys, (jstring)&jkarta, (jstring)&jhra));
  std::this_thread::sleep_for(std::chrono::milliseconds(11500));   // 2 okna ZDRAVI (po 5 s)
  dn = j_denik();
  over(r == "PS1_HRA_OK" && dn.find("MERENI VYKONU: zapnuto (13 / 13") != std::string::npos, "telefonni cesta: hra bezi, mereni zapnuto (" + r + ")");
  size_t v = dn.find("VYKON ");
  std::string radek = v == std::string::npos ? std::string() : dn.substr(v, dn.find('\n', v) - v);
  double celk = -1, hra = -1, kr = -1, ob = -1, zv = -1;
  if (!radek.empty())
    std::sscanf(radek.c_str() + radek.find("snimek prumer"), "snimek prumer %lf ms = hra (procesor PS1) %lf + kresleni %lf + predani obrazu %lf + zvuk %lf",
                &celk, &hra, &kr, &ob, &zv);
  over(celk > 0 && hra >= 0 && kr >= 0 && ob >= 0 && zv >= 0 && std::fabs(hra + kr + ob + zv - celk) <= 0.25,
       "radek VYKON: casti daji dohromady cely snimek (" + radek.substr(0, radek.find(" | jadro")) + ")");
  over(radek.find("| jadro procesoru: cpu") != std::string::npos && radek.find(" %") != std::string::npos,
       "radek VYKON: na kterych jadrech procesoru emulace bezela (" + (radek.find("| jadro") == std::string::npos ? std::string("-") : radek.substr(radek.find("| jadro"))) + ")");
  over(dn.find("| zvuk XA ZAPNUTY, CD audio ZAPNUTY, pomaly start") != std::string::npos,
       "radek JADRO: \"zvuk XA ZAPNUTY, CD audio ZAPNUTY\" (B308 psalo obracene \"XA NE, CD audio NE\")");
  jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(&g_env, nullptr));
  nap_sl_close();
  {
    std::ofstream o(prac + "/UKAZKA_denik_vykon_b328.txt");
    o << dn;
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
