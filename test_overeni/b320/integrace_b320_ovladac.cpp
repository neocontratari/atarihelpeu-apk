// B320: HERNI OVLADAC (Retroid Pocket 6, gamepad) SE SKUTECNYM JADREM PS1.
//
// Uzivatel z fora: "jestli to pujde na Retro Pocket 6 s tim, ze by se na obrazovce po pripojeni
// stahnul nas d-pad a najelo to na origo tlacitka". Tlacitka a packy ovladace jdou z Javy (NapOvladac,
// MainActivity.dispatchKeyEvent / dispatchGenericMotionEvent) pres JNI ps1OvladacKlavesa / ps1OvladacOsy
// do C++ (nap_ovladac.h) - tady presne ta JNI volani se SKUTECNYM jadrem PCSX-ReARMed z appky (BIOS HLE)
// a nap_ps1_native.cpp z appky; "hra" mys_test.exe (../b318/mys_test.S) cte ovladac / mys pres BIOS jako
// hry a v RAM nechava, co prislo (tlacitka portu 1 a 2, posun a tlacitka mysi).
// Infrastruktura (okno Androidu, OpenSL ES, JNIEnv, nahrada cipu GPU) je okopirovana z
// ../b318/integrace_b318.cpp (B318) beze zmeny.
//
// Kontroly:
//  1) mys vypnuta: kazde tlacitko ovladace -> hra vidi v portu 1 SPRAVNE tlacitko PlayStation
//     (A spodni = KRIZEK, B prave = KOLECKO, X leve = CTVEREC, Y horni = TROJUHELNIK, L1 R1 L2 R2,
//     START, SELECT, krizek), pusteni, uhlopricky, ZPET neni tlacitko hry
//  2) osy: leva packa = smer, krizek HAT, analogove spouste L2 / R2; dotyk + ovladac soucasne
//  3) ps1OvladacPust (ovladac odpojen) a konec hry: nic nezustane drzet
//  4) mys PlayStation v portu 1 (Dune 2000): prava packa = kurzor analogove (~6 bodu za snimek),
//     leva packa taky, KRIZEK = leve tlacitko mysi, KOLECKO = prave, krizek = kurzor
//  5) mys v portu 1 + ovladac v portu 2: tlacitka a leva packa = ovladac v portu 2, prava packa = mys
//  6) denik PS1: "OVLADAC: klavesa 96 dolu -> A(spodni) -> PS1 KRIZEK"
//  7) (TSan) udalosti ovladace z jineho vlakna nez vlakno emulace
//
// Preklad (PRAC/jadro = ../b318/stavba_jadra.sh, PRAC/mys_test.exe = ../b318/udelej_exe.py):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/integrace_b320_ovladac integrace_b320_ovladac.cpp PRAC/jadro/*.o \
//       -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b320_ovladac PRAC PRAC/mys_test.exe
#include "../../app/src/main/cpp/nap_ps1_native.cpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

extern "C" {
#include "../../app/src/main/cpp/vendor/pcsx_rearmed/frontend/plugin_lib.h"
unsigned char PAD1_startPoll(void);
unsigned char PAD1_poll(unsigned char, int *);
}

// ---------- vykreslovac GPU (v appce gpu_neon + gpulib, ARM NEON) - grafika se tu nekresli ----------
static std::atomic<int> g_flipu{0};
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
long GPUinit(void) { return 0; }
long GPUshutdown(void) { return 0; }
long GPUopen(unsigned long *, char *, char *) { return 0; }
long GPUclose(void) { return 0; }
uint32_t GPUreadStatus(void) { return 0x14802000u; }
uint32_t GPUreadData(void) { return 0; }
void GPUreadDataMem(uint32_t *p, int n) { if (p && n > 0) std::memset(p, 0, (size_t)n * 4); }
// B318: hra prepla zobrazovanou oblast (GP1(05h)) = novy obraz - frontend libretro to dostane
// jako od gpulib (vout_flip), na konci retro_run pak posle obraz misto "stejny" (NULL)
void GPUwriteStatus(uint32_t d) {
  if ((d >> 24) == 5 && pl_rearmed_cbs.pl_vout_flip) {
    g_flipu.fetch_add(1);
    pl_rearmed_cbs.pl_vout_flip(nullptr, 0, 0, 0, 0, 320, 240, 0);
  }
}
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

// ---------- rucne sestaveny JNIEnv (jako B308 / B317) ----------
struct FakeStr { std::string s; };
struct FakeArr { int druh = 0; std::vector<uint8_t> b; std::vector<float> f; };   // B320: druh 1 = float[] (osy ovladace)
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { FakeArr *x = (FakeArr *)a; return (jsize)(x->druh == 1 ? x->f.size() : x->b.size()); }
static void JNICALL fGetFloatArrayRegion(JNIEnv *, jfloatArray a, jsize st, jsize n, jfloat *buf) { std::memcpy(buf, ((FakeArr *)a)->f.data() + st, (size_t)n * sizeof(float)); }
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
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_env.functions = &g_tbl;
}

// ---------- JNI presne jako Java (NativePs1CoreBridge) ----------
static const float W = 732.f, H = 412.f;          // obraz hry na strance (na sirku)
static const double LANO = 0.015 * 732.0;          // B318: prvnich 1,5 % sirky kurzor stoji
static std::string j_mysNastav(int rezim, int citlivost) {
  return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysNastav(&g_env, nullptr, rezim, citlivost));
}
static std::string j_mysZrychleni(int zap) { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysZrychleni(&g_env, nullptr, zap)); }
static std::string j_mysStav() { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysStav(&g_env, nullptr)); }
static int j_dotyk(int akce, int id, float x, float y) {
  return Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysDotyk(&g_env, nullptr, akce, id, x, y, W, H);
}
static void j_tlacitko(int id, bool dolu) { Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetInput(&g_env, nullptr, id, dolu ? JNI_TRUE : JNI_FALSE); }
static std::string j_denik() {
  FakeArr *a = (FakeArr *)Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Denik(&g_env, nullptr);
  std::string r(a->b.begin(), a->b.end());
  delete a;
  return r;
}
static std::string j_pcMys(int dx, int dy, int tl) { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PcMys(&g_env, nullptr, dx, dy, tl)); }

// ---------- vysledky "hry" v pameti PS1 (mys_test.S) ----------
struct Port {
  uint32_t cteni, posl, sx, sy, stiskL, stiskP, snL, snP, tlacitka, predL, predP, mys, ovl, nic, maxdx;
  int32_t X() const { return (int32_t)sx; }
  int32_t Y() const { return (int32_t)sy; }
  int stav() const { return (int)(posl >> 8); }
  int id() const { return (int)(posl & 0xFF); }
};
static uint8_t *ram() { return (uint8_t *)retro_get_memory_data(2 /* RETRO_MEMORY_SYSTEM_RAM */); }
static Port port(int i) {
  Port p;
  std::memcpy(&p, ram() + 0x100100 + i * 0x40, sizeof(p));
  return p;
}
static Port rozdil(const Port &b, const Port &a) {   // co se zmenilo mezi a a b
  Port d = b;
  d.cteni -= a.cteni; d.sx -= a.sx; d.sy -= a.sy; d.stiskL -= a.stiskL; d.stiskP -= a.stiskP; d.snL -= a.snL; d.snP -= a.snP;
  d.mys -= a.mys; d.ovl -= a.ovl; d.nic -= a.nic;
  return d;
}
// B318: "hra" cte mys a kresli jen kazdy P. snimek
static void perioda(uint32_t p) { std::memcpy(ram() + 0x1000F0, &p, 4); }

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string t(long v) { return std::to_string(v); }
static std::string tf(double v, int d = 1) { char b[32]; snprintf(b, sizeof(b), "%.*f", d, v); return b; }
static std::string hex(unsigned v) { char b[16]; snprintf(b, sizeof(b), "%04X", v); return b; }

// snimky jadra v realnem case (60 Hz NTSC - PS-EXE): presne cesta Javy ps1EglTick
static std::chrono::steady_clock::time_point g_takt;
static void snimek1() {
  g_takt += std::chrono::microseconds(16683);
  std::this_thread::sleep_until(g_takt);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglTick(&g_env, nullptr);
}
static void snimky(int n) { for (int i = 0; i < n; i++) snimek1(); }
// tah prstem: prst se pohne mezi snimky
static void tah(int id, float x0, float y0, float x1, float y1, int kroku) {
  j_dotyk(0, id, x0, y0);
  snimky(1);
  for (int k = 1; k <= kroku; k++) { j_dotyk(2, id, x0 + (x1 - x0) * k / kroku, y0 + (y1 - y0) * k / kroku); snimky(1); }
  j_dotyk(1, id, x1, y1);
}
static double body(double px, int citlivost = 100) { return px * 320.0 / W * citlivost / 100.0; }

// PS1 tlacitka ovladace v odpovedi (0 = stisknuto): bajt 3 bit 6 = KRIZEK, bit 5 = KOLECKO
static const unsigned PS_KRIZEK = 1u << 14, PS_KOLECKO = 1u << 13, PS_VPRAVO = 1u << 5;


// ================== B320: herni ovladac ==================
// PS1 tlacitka v odpovedi ovladace (bajt 2 | bajt 3 << 8, 0 = stisknuto)
static const unsigned PS_SELECT = 1u << 0, PS_START = 1u << 3, PS_NAHORU = 1u << 4, PS_DOLU = 1u << 6, PS_VLEVO = 1u << 7,
                      PS_L2 = 1u << 8, PS_R2 = 1u << 9, PS_L1 = 1u << 10, PS_R1 = 1u << 11, PS_TROJ = 1u << 12, PS_CTVEREC = 1u << 15;
static int j_ovlKlavesa(int kod, bool dolu) {
  return Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1OvladacKlavesa(&g_env, nullptr, kod, dolu ? JNI_TRUE : JNI_FALSE);
}
static void j_ovlOsy(float lx, float ly, float rx, float ry, float hx, float hy, float l2, float r2, int maska) {
  FakeArr a;
  a.druh = 1;
  a.f = {lx, ly, rx, ry, hx, hy, l2, r2};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1OvladacOsy(&g_env, nullptr, (jfloatArray)&a, maska);
}
static void j_ovlPust() { Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1OvladacPust(&g_env, nullptr); }
static unsigned stisknuto(const Port &p) { return (~p.tlacitka) & 0xFFFFu; }   // 1 = stisknuto
// po zmene portu (mys / ovladac) chvili trva, nez hra vidi nove zarizeni (BIOS: port chvili "nic" - FF)
static int cekejNaZarizeni(int port0id, int port1id) {
  for (int i = 0; i < 180; i++) {
    snimek1();
    if (port(0).id() == port0id && port(0).stav() == 0 && (port1id < 0 || (port(1).id() == port1id && port(1).stav() == 0))) {
      snimky(5);
      return i + 1;
    }
  }
  return -1;
}
static std::string bity(unsigned b) {
  static const char *J[16] = {"SELECT", "L3", "R3", "START", "NAHORU", "VPRAVO", "DOLU", "VLEVO", "L2", "R2", "L1", "R1", "TROJUHELNIK", "KOLECKO", "KRIZEK", "CTVEREC"};
  std::string s;
  for (int i = 0; i < 16; i++) if (b & (1u << i)) { if (!s.empty()) s += '+'; s += J[i]; }
  return s.empty() ? "nic" : s;
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b320_ovladac PRAC mys_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  const bool tsan = getenv("NAP_TSAN") != nullptr;
  pripravEnv();
  std::printf("B320 - herni ovladac (Retroid Pocket 6, gamepad) se SKUTECNYM jadrem PS1 (PCSX-ReARMed z appky, BIOS HLE, hra mys_test.exe)%s\n",
              tsan ? " - ThreadSanitizer" : "");
  const std::string sys = prac + "/system_ovl", karta = prac + "/karta_ovl";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log_ovl.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mys_test.exe (ps1EglBoot = " + t(boot) + ")");
  g_takt = std::chrono::steady_clock::now();
  snimky(90);
  Port p1 = port(0);
  over(p1.id() == 0x41 && p1.cteni > 60 && stisknuto(p1) == 0, "mys vypnuta: v portu 1 standardni ovladac (ID 41), nic stisknuto");

  if (tsan) {
    // ======== 7) ThreadSanitizer: udalosti ovladace z jineho vlakna (jako vlakno UI Javy) ========
    std::atomic<bool> bezi{true};
    std::thread ui([&] {
      const int K[] = {96, 97, 99, 100, 102, 103, 104, 105, 108, 109, 19, 20, 21, 22};
      int i = 0;
      while (bezi.load()) {
        const int k = K[i % 14];
        j_ovlKlavesa(k, true);
        j_ovlOsy((i % 7) / 7.f, -(i % 5) / 5.f, (i % 3) / 3.f, 0.5f, (float)(i % 3) - 1, 0, (i % 4) / 4.f, 0.9f, 15);
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        j_ovlKlavesa(k, false);
        if ((i % 50) == 49) j_ovlPust();
        if ((i % 97) == 96) j_mysNastav(i % 3, 100);
        i++;
      }
    });
    snimky(240);
    bezi.store(false);
    ui.join();
    j_ovlPust();
    snimky(4);
    over(stisknuto(port(0)) == 0 || port(0).id() != 0x41, "TSan: 4 s udalosti ovladace z vlakna UI soubezne s emulaci, po pusteni nic nedrzi");
    Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
    std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
    std::fflush(stdout);
    std::_Exit(g_chyb ? 1 : 0);
  }

  // ======== 1) tlacitka ovladace -> tlacitka PlayStation v portu 1 ========
  struct T { int kod; unsigned ps; const char *jm; };
  const T TL[] = {{96, PS_KRIZEK, "A (spodni)"}, {97, PS_KOLECKO, "B (prave)"}, {99, PS_CTVEREC, "X (leve)"}, {100, PS_TROJ, "Y (horni)"},
                  {102, PS_L1, "L1"}, {103, PS_R1, "R1"}, {104, PS_L2, "L2"}, {105, PS_R2, "R2"}, {108, PS_START, "START"},
                  {109, PS_SELECT, "SELECT"}, {19, PS_NAHORU, "krizek nahoru"}, {20, PS_DOLU, "krizek dolu"}, {21, PS_VLEVO, "krizek vlevo"},
                  {22, PS_VPRAVO, "krizek vpravo"}, {23, PS_KRIZEK, "DPAD_CENTER"}, {110, PS_SELECT, "MODE"}};
  int spravne = 0;
  std::string chyby;
  for (const T &x : TL) {
    const int r = j_ovlKlavesa(x.kod, true);
    snimky(3);
    const unsigned dole = stisknuto(port(0));
    j_ovlKlavesa(x.kod, false);
    snimky(3);
    const unsigned po = stisknuto(port(0));
    if (r != 0 && dole == x.ps && po == 0) spravne++;
    else chyby += std::string(" | ") + x.jm + ": hra " + bity(dole) + " (ma " + bity(x.ps) + "), po pusteni " + bity(po);
  }
  over(spravne == (int)(sizeof(TL) / sizeof(TL[0])),
       "1a) mys vypnuta: " + t(spravne) + " z " + t((long)(sizeof(TL) / sizeof(TL[0]))) +
           " tlacitek ovladace = spravne tlacitko PlayStation v portu 1 (A KRIZEK, B KOLECKO, X CTVEREC, Y TROJUHELNIK, L1 R1 L2 R2, START, SELECT, krizek), po pusteni nic" + chyby);
  {
    j_ovlKlavesa(270, true);        // DPAD_UP_RIGHT
    j_ovlKlavesa(96, true);
    j_ovlKlavesa(103, true);
    snimky(3);
    const unsigned tri = stisknuto(port(0));
    j_ovlKlavesa(96, false);
    snimky(3);
    const unsigned dva = stisknuto(port(0));
    j_ovlKlavesa(270, false);
    j_ovlKlavesa(103, false);
    const int zpet = j_ovlKlavesa(4, true);   // KEYCODE_BACK
    j_ovlKlavesa(4, false);
    snimky(3);
    over(tri == (PS_NAHORU | PS_VPRAVO | PS_KRIZEK | PS_R1) && dva == (PS_NAHORU | PS_VPRAVO | PS_R1) && zpet == 0 && stisknuto(port(0)) == 0,
         "1b) vic naraz: uhlopricka + A + R1 = " + bity(tri) + ", A pusteno -> " + bity(dva) + "; ZPET (4) neni tlacitko hry (vraci 0 - Android ji zpracuje jako drive)");
  }

  // ======== 2) osy ========
  {
    const int M = 15;   // HAT | prava | spouste | leva
    j_ovlOsy(0.9f, 0, 0, 0, 0, 0, 0, 0, M);
    snimky(3);
    const unsigned leva = stisknuto(port(0));
    j_ovlOsy(0.4f, 0, 0, 0, 0, 0, 0, 0, M);    // hystereze: porad vpravo
    snimky(3);
    const unsigned hyst = stisknuto(port(0));
    j_ovlOsy(0.1f, 0, 0, 0, 0, 0, 0, 0, M);
    snimky(3);
    const unsigned pusteno = stisknuto(port(0));
    j_ovlOsy(0, 0, 0, 0, -1, 1, 0, 0, M);      // HAT vlevo dolu
    snimky(3);
    const unsigned hat = stisknuto(port(0));
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0.8f, 0.2f, M);  // spoust L2 analogove
    snimky(3);
    const unsigned sp = stisknuto(port(0));
    j_ovlOsy(0, 0, 0.9f, 0.9f, 0, 0, 0, 0, M);  // prava packa: s vypnutou mysi nic (PS1 digitalni ovladac)
    snimky(3);
    const unsigned prava = stisknuto(port(0));
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0, 0, M);
    snimky(3);
    over(leva == PS_VPRAVO && hyst == PS_VPRAVO && pusteno == 0 && hat == (PS_VLEVO | PS_DOLU) && sp == PS_L2 && prava == 0 && stisknuto(port(0)) == 0,
         "2a) leva packa 0,9 = vpravo, zpet na 0,4 porad vpravo (hystereze), 0,1 nic; krizek HAT = " + bity(hat) + "; analogova spoust 0,8 = " + bity(sp) +
             "; prava packa bez mysi nic");
    // dotyk (stranka PS1) a ovladac soucasne: hra vidi oboje
    j_tlacitko(3, true);            // START z dotyku
    j_ovlKlavesa(96, true);         // A z ovladace
    snimky(3);
    const unsigned oboje = stisknuto(port(0));
    j_ovlKlavesa(96, false);
    snimky(3);
    const unsigned jenDotyk = stisknuto(port(0));
    j_tlacitko(3, false);
    snimky(3);
    over(oboje == (PS_START | PS_KRIZEK) && jenDotyk == PS_START && stisknuto(port(0)) == 0,
         "2b) dotyk na strance (START) + ovladac (A) soucasne: hra vidi " + bity(oboje) + "; pusteni ovladace dotyk nepusti");
  }

  // ======== 3) odpojeni a konec hry ========
  {
    j_ovlKlavesa(96, true);
    j_ovlKlavesa(105, true);
    j_ovlOsy(-0.9f, 0, 0, 0, 0, 0, 0, 0, 15);
    snimky(3);
    const unsigned drzi = stisknuto(port(0));
    j_ovlPust();                    // Java: ovladac odpojen (ovladacPustVse)
    snimky(3);
    over(drzi == (PS_KRIZEK | PS_R2 | PS_VLEVO) && stisknuto(port(0)) == 0,
         "3) ovladac odpojen uprostred stisku (" + bity(drzi) + "): ps1OvladacPust - hra uz nic drzeneho nevidi");
  }

  // ======== 4) mys PlayStation v portu 1 (Dune 2000) ========
  {
    j_mysNastav(1, 100);
    j_mysZrychleni(0);
    const int za = cekejNaZarizeni(0x12, -1);
    over(za > 0, "4) mys PlayStation zapnuta v portu 1: hra ji vidi (ID 12) po " + t(za) + " snimcich");
    Port pred = port(0);
    j_ovlOsy(0, 0, 1.0f, 0, 0, 0, 0, 0, 15);       // prava packa naplno vpravo, 60 snimku
    snimky(60);
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0, 0, 15);
    snimky(10);
    Port d = rozdil(port(0), pred);
    over(port(0).id() == 0x12 && d.X() >= 330 && d.X() <= 380 && std::abs(d.Y()) <= 2,
         "4a) mys v portu 1: prava packa naplno 1 s -> kurzor ve hre +" + t(d.X()) + " bodu (6 za snimek, ~1 sirka obrazu), Y " + t(d.Y()));
    pred = port(0);
    j_ovlOsy(0, -0.6f, 0, 0, 0, 0, 0, 0, 15);      // leva packa nahoru (mys samotna - i leva ridi kurzor)
    snimky(30);
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0, 0, 15);
    snimky(10);
    d = rozdil(port(0), pred);
    const long ocekY = -std::lround(30 * nap::ovladac::packaRychlost(0.5f, 100));
    over(std::labs(d.Y() - ocekY) <= 3 && d.X() == 0 && stisknuto(port(0)) == 0,
         "4b) leva packa 0,6 nahoru (po mrtve zone 0,5): kurzor " + t(d.Y()) + " (vypocet " + t(ocekY) + "), zadne tlacitko - smer leve packy ridi kurzor, ne krizek");
    pred = port(0);
    j_ovlKlavesa(96, true);
    snimky(8);
    j_ovlKlavesa(96, false);
    snimky(4);
    j_ovlKlavesa(97, true);
    snimky(5);
    j_ovlKlavesa(97, false);
    snimky(4);
    j_ovlKlavesa(22, true);
    snimky(30);
    j_ovlKlavesa(22, false);
    snimky(6);
    d = rozdil(port(0), pred);
    over(d.stiskL == 1 && d.snL == 8 && d.stiskP == 1 && d.snP == 5 && d.X() > 30,
         "4c) A = leve tlacitko mysi (8 snimku), B = prave (5 snimku), krizek vpravo 0,5 s = kurzor +" + t(d.X()));
  }

  // ======== 5) mys v portu 1 + ovladac v portu 2 ========
  {
    j_mysNastav(2, 100);
    const int za = cekejNaZarizeni(0x12, 0x41);
    Port pred = port(0);
    j_ovlKlavesa(96, true);
    j_ovlOsy(0.9f, 0, 0.7f, 0, 0, 0, 0, 0, 15);
    snimky(30);
    Port p2 = port(1);
    Port d = rozdil(port(0), pred);
    j_ovlKlavesa(96, false);
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0, 0, 15);
    snimky(10);
    const long ocek5 = std::lround(30 * nap::ovladac::packaRychlost(0.625f, 100));
    over(za > 0 && port(0).id() == 0x12 && p2.id() == 0x41 && stisknuto(p2) == (PS_KRIZEK | PS_VPRAVO) && d.stiskL == 0 &&
             std::labs(d.X() - ocek5) <= 3 && stisknuto(port(1)) == 0,
         "5) mys v portu 1 + ovladac v portu 2: A a leva packa = ovladac v portu 2 (" + bity(stisknuto(p2)) + "), mys neklika, prava packa 0,7 = kurzor +" +
             t(d.X()) + " (vypocet " + t(ocek5) + ")");
    j_mysNastav(0, 100);
    snimky(10);
  }

  // ======== 6) denik ========
  {
    const std::string dn = j_denik();
    over(dn.find("OVLADAC: klavesa 96 dolu -> A(spodni) -> PS1 KRIZEK") != std::string::npos &&
             dn.find("OVLADAC: klavesa 97 dolu -> B(prave) -> PS1 KOLECKO") != std::string::npos,
         "6) denik PS1: \"OVLADAC: klavesa 96 dolu -> A(spodni) -> PS1 KRIZEK\", \"... 97 dolu -> B(prave) -> PS1 KOLECKO\"");
  }

  // konec hry: ovladac se pusti (ps1EglStop / dalsi start)
  j_ovlKlavesa(96, true);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  const unsigned poKonci = g_ovl_snimek.load() | g_ovl_logicke.load();
  {
    FakeStr j2{sys}, h2{exe};
    Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&j2, (jstring)&h2);
  }
  g_takt = std::chrono::steady_clock::now();
  snimky(60);
  over(stisknuto(port(0)) == 0 && (g_ovl_logicke.load() == 0),
       "3b) A drzene pri konci hry: po dalsim startu hry nic nedrzi (" + bity(stisknuto(port(0))) + ", logicka " + std::to_string(poKonci) + " -> 0)");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  std::fflush(stdout);
  std::_Exit(g_chyb ? 1 : 0);
}
