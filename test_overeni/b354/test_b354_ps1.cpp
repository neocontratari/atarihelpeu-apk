// test_overeni/b354/test_b354_ps1.cpp
// B354: DVA HERNI OVLADACE = DVA HRACI NA PS1 - SKUTECNE jadro PCSX-ReARMed z appky (BIOS HLE) + nap_ps1_native.cpp,
// "hra" mys_test.exe (../b318) cte pres BIOS oba porty a v RAM nechava, co prislo. Infrastruktura okopirovana
// z ../b320/integrace_b320_ovladac.cpp. Hrac 2 = druhy ovladac (Java NapOvladac.Hraci) -> ps1OvladacKlavesaHrac /
// ps1OvladacOsyHrac(1, ...) -> port 2 (jen s vypnutou mysi; s mysi port 2 patri mysi nebo ovladaci 1).
//  1) mys vypnuta: v portu 2 standardni ovladac; hrac 2 = tlacitka v portu 2, port 1 nic
//  2) oba hraci naraz (hrac 1 ovladacem i dotykem na strance) - kazdy svuj port
//  3) hrac 2: osy (packa, krizek HAT, spoust L2)
//  4) mys v portu 1 + ovladac v portu 2: port 2 = ovladac hrace 1, hrac 2 nic (log "OVLADAC 2")
//  5) ps1OvladacPust (odpojeni): oba porty nic
// Preklad (PRAC/jadro = ../b318/stavba_jadra.sh, PRAC/mys_test.exe = ../b318/udelej_exe.py, $PS1 viz ../b320/vse.sh):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/test_b354_ps1 test_b354_ps1.cpp PRAC/jadro/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./test_b354_ps1 PRAC PRAC/mys_test.exe        -> VYSTUP_test_b354_ps1.txt
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

static int j_ovl2(int kod, bool dolu) {
  return Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1OvladacKlavesaHrac(&g_env, nullptr, 1, kod, dolu ? JNI_TRUE : JNI_FALSE);
}
static void j_osy2(float lx, float ly, float hx, float hy, float l2, int maska) {
  FakeArr a;
  a.druh = 1;
  a.f = {lx, ly, 0, 0, hx, hy, l2, 0};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1OvladacOsyHrac(&g_env, nullptr, 1, (jfloatArray)&a, maska);
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: test_b354_ps1 PRAC mys_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B354 - dva herni ovladace = dva hraci na PS1 (SKUTECNE jadro PCSX-ReARMed z appky, BIOS HLE, hra mys_test.exe)\n");
  const std::string sys = prac + "/system_b354", karta = prac + "/karta_b354";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log_b354.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mys_test.exe (ps1EglBoot = " + t(boot) + ")");
  g_takt = std::chrono::steady_clock::now();
  snimky(90);
  {
    const Port p1 = port(0), p2 = port(1);
    over(p1.id() == 0x41 && p2.id() == 0x41 && p2.cteni > 60 && stisknuto(p1) == 0 && stisknuto(p2) == 0,
         "mys vypnuta: v portu 1 i 2 standardni ovladac (ID 41), hra cte oba porty, nic stisknuto");
  }
  // 1) hrac 2 = port 2
  {
    struct T { int kod; unsigned ps; const char *jm; };
    const T TL[] = {{96, PS_KRIZEK, "A (spodni)"}, {97, PS_KOLECKO, "B (prave)"}, {99, PS_CTVEREC, "X (leve)"}, {100, PS_TROJ, "Y (horni)"},
                    {102, PS_L1, "L1"}, {103, PS_R1, "R1"}, {108, PS_START, "START"}, {109, PS_SELECT, "SELECT"},
                    {19, PS_NAHORU, "nahoru"}, {20, PS_DOLU, "dolu"}, {21, PS_VLEVO, "vlevo"}, {22, PS_VPRAVO, "vpravo"}};
    int spravne = 0;
    std::string chyby;
    for (const T &x : TL) {
      const int r = j_ovl2(x.kod, true);
      snimky(3);
      const unsigned p2 = stisknuto(port(1)), p1 = stisknuto(port(0));
      j_ovl2(x.kod, false);
      snimky(3);
      const unsigned po = stisknuto(port(1));
      if (r != 0 && p2 == x.ps && p1 == 0 && po == 0) spravne++;
      else chyby += std::string(" | ") + x.jm + ": port 2 " + bity(p2) + ", port 1 " + bity(p1) + ", po " + bity(po);
    }
    over(spravne == (int)(sizeof(TL) / sizeof(TL[0])), "1) hrac 2: " + t(spravne) + " z " + t((long)(sizeof(TL) / sizeof(TL[0]))) +
                                                          " tlacitek = spravne tlacitko PlayStation v PORTU 2, port 1 nic" + chyby);
  }
  // 2) oba naraz
  {
    j_ovlKlavesa(108, true);       // hrac 1 START (ovladac)
    j_tlacitko(0, true);           // hrac 1 KRIZEK dotykem na strance PS1 (RetroPad B = id 0 = KRIZEK)
    j_ovl2(97, true);              // hrac 2 KOLECKO
    j_ovl2(21, true);              // hrac 2 vlevo
    snimky(3);
    const unsigned a1 = stisknuto(port(0)), a2 = stisknuto(port(1));
    j_ovlKlavesa(108, false);
    j_tlacitko(0, false);
    snimky(3);
    const unsigned b1 = stisknuto(port(0)), b2 = stisknuto(port(1));
    j_ovl2(97, false); j_ovl2(21, false);
    snimky(3);
    std::printf("oba: port 1 %s, port 2 %s | hrac 1 pustil: port 1 %s, port 2 %s\n", bity(a1).c_str(), bity(a2).c_str(), bity(b1).c_str(), bity(b2).c_str());
    over(a1 == (PS_START | PS_KRIZEK) && a2 == (PS_KOLECKO | PS_VLEVO) && b1 == 0 && b2 == (PS_KOLECKO | PS_VLEVO) &&
             stisknuto(port(1)) == 0,
         "2) oba naraz: hrac 1 (ovladac + dotyk) v portu 1, hrac 2 v portu 2; pusteni hrace 1 nepusti hrace 2");
  }
  // 3) osy hrace 2
  {
    j_osy2(0.9f, 0, 0, 0, 0, 15);
    snimky(3);
    const unsigned p = stisknuto(port(1)), p1 = stisknuto(port(0));
    j_osy2(0, 0, -1, 1, 0, 15);
    snimky(3);
    const unsigned h = stisknuto(port(1));
    j_osy2(0, 0, 0, 0, 0.8f, 15);
    snimky(3);
    const unsigned s = stisknuto(port(1));
    j_osy2(0, 0, 0, 0, 0, 15);
    snimky(3);
    over(p == PS_VPRAVO && p1 == 0 && h == (PS_VLEVO | PS_DOLU) && s == PS_L2 && stisknuto(port(1)) == 0,
         "3) hrac 2: leva packa = VPRAVO (port 1 nic), krizek HAT = " + bity(h) + ", analogova spoust = " + bity(s) + ", pusteno nic");
  }
  // 4) mys v portu 1, ovladac v portu 2
  {
    const std::string r = j_mysNastav(2, 100);           // PORT1_OVL2
    const int ceka = cekejNaZarizeni(0x12, 0x41);
    j_ovl2(96, true);                                    // hrac 2 nic
    j_ovlKlavesa(100, true);                             // hrac 1 TROJUHELNIK = ovladac v portu 2
    snimky(3);
    const unsigned p2 = stisknuto(port(1));
    j_ovl2(96, false); j_ovlKlavesa(100, false);
    snimky(3);
    std::printf("mys v portu 1 (%s, cekani %d snimku): port 2 = %s\n", r.c_str(), ceka, bity(p2).c_str());
    over(ceka > 0 && p2 == PS_TROJ, "4) mys v portu 1 + ovladac v portu 2: port 2 = ovladac hrace 1 (TROJUHELNIK), hrac 2 nic (port 2 neni volny)");
    j_mysNastav(0, 100);
    cekejNaZarizeni(0x41, 0x41);
  }
  // 5) odpojeni
  {
    j_ovl2(99, true); j_ovl2(20, true); j_ovlKlavesa(96, true);
    snimky(3);
    const unsigned d1 = stisknuto(port(0)), d2 = stisknuto(port(1));
    j_ovlPust();
    snimky(3);
    over(d1 == PS_KRIZEK && d2 == (PS_CTVEREC | PS_DOLU) && stisknuto(port(0)) == 0 && stisknuto(port(1)) == 0,
         "5) ovladac odpojen uprostred stisku obou hracu (port 1 " + bity(d1) + ", port 2 " + bity(d2) + ") -> ps1OvladacPust: nic nedrzi");
  }
  {
    std::ifstream f(prac + "/nap_tv_session_log_b354.txt");
    std::stringstream ss; ss << f.rdbuf();
    const std::string d = ss.str();
    const size_t p = d.find("OVLADAC 2 (hrac 2)");
    if (p != std::string::npos) std::printf("DENIK: %s\n", d.substr(p, d.find('\n', p) - p).c_str());
    over(p != std::string::npos, "denik PS1: \"OVLADAC 2 (hrac 2): prvni stisk -> PS1 port 2\"");
  }
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  std::fflush(stdout);
  std::_Exit(g_chyb ? 1 : 0);
}
