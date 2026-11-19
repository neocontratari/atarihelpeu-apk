// test_overeni/b355/test_b355_ps1.cpp
// B355: TLACITKA OVLADACE NA PS1 - SKUTECNE jadro PCSX-ReARMed z appky (BIOS HLE) + nap_ps1_native.cpp, "hra"
// mys_test.exe (../b318) cte pres BIOS oba porty a v RAM nechava, co prislo (infrastruktura z ../b354/test_b354_ps1.cpp).
// Java (NapOvladacMapa) posle mapu ps1OvladacMapa; vlakno emulace ji pouzije pri kazdem snimku.
//  1) vychozi mapa = do B354 (stejne misto jako PlayStation), jen HOME uz neni SELECT (MENU - dialog v appce)
//  2) vlastni mapa: prohozeny krizek a kolecko, Y = L1, L1 = trojuhelnik, SELECT = START, HOME = SELECT, R1 = nahoru,
//     analogova spoust L2 = krizek; jadro mapu vrati a zapise do deniku
//  3) hrac 2 (port 2) se stejnou mapou; mapa "" = zase vychozi
// Preklad jako ../b354/test_b354_ps1.cpp:
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/test_b355_ps1 test_b355_ps1.cpp PRAC/jadro/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./test_b355_ps1 PRAC PRAC/mys_test.exe        -> VYSTUP_test_b355_ps1.txt
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

static std::string j_mapa(const char *m) {
  FakeStr s{m};
  return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1OvladacMapa(&g_env, nullptr, (jstring)&s));
}
// jedno tlacitko hrace h (0 = port 1, 1 = port 2): co hra vidi v jeho portu a ve druhem; po pusteni nic
static unsigned zmackni(int h, int kod, unsigned *druhy = nullptr, bool *pusteno = nullptr) {
  if (h) j_ovl2(kod, true); else j_ovlKlavesa(kod, true);
  snimky(3);
  const unsigned r = stisknuto(port(h)), d = stisknuto(port(1 - h));
  if (h) j_ovl2(kod, false); else j_ovlKlavesa(kod, false);
  snimky(3);
  if (druhy) *druhy = d;
  if (pusteno) *pusteno = stisknuto(port(h)) == 0;
  return r;
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: test_b355_ps1 PRAC mys_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B355 - tlacitka herniho ovladace na PS1 (SKUTECNE jadro PCSX-ReARMed z appky, BIOS HLE, hra mys_test.exe)\n");
  const std::string sys = prac + "/system_b355", karta = prac + "/karta_b355";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log_b355.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mys_test.exe (ps1EglBoot = " + t(boot) + ")");
  g_takt = std::chrono::steady_clock::now();
  snimky(90);
  over(port(0).id() == 0x41 && port(1).id() == 0x41 && stisknuto(port(0)) == 0, "mys vypnuta: v portu 1 i 2 standardni ovladac (ID 41), nic stisknuto");
  // 1) vychozi mapa
  {
    struct T { int kod; unsigned ps; const char *jm; };
    const T TL[] = {{96, PS_KRIZEK, "A = krizek"}, {97, PS_KOLECKO, "B = kolecko"}, {99, PS_CTVEREC, "X = ctverec"}, {100, PS_TROJ, "Y = trojuhelnik"},
                    {102, PS_L1, "L1"}, {103, PS_R1, "R1"}, {104, PS_L2, "L2"}, {105, PS_R2, "R2"}, {108, PS_START, "START"},
                    {109, PS_SELECT, "SELECT"}, {110, 0, "HOME = MENU (jadro nic; do B354 SELECT)"}, {98, PS_KRIZEK, "C = krizek"}};
    int spravne = 0;
    std::string chyby;
    for (const T &x : TL) {
      unsigned d = 0; bool p = false;
      const unsigned r = zmackni(0, x.kod, &d, &p);
      if (r == x.ps && d == 0 && p) spravne++;
      else chyby += std::string(" | ") + x.jm + ": " + bity(r);
    }
    over(spravne == (int)(sizeof(TL) / sizeof(TL[0])), "1) vychozi mapa: " + t(spravne) + " z " + t((long)(sizeof(TL) / sizeof(TL[0]))) +
                                                          " tlacitek jako do B354 (misto jako PlayStation); HOME uz neni SELECT" + chyby);
  }
  // 2) vlastni mapa
  {
    const std::string n = j_mapa("A=KOLECKO;B=KRIZEK;Y=L1;L1=TROJUHELNIK;SELECT=START;MODE=SELECT;R1=NAHORU;L2=KRIZEK;X=FIRE");
    std::printf("mapa -> jadro: %s\n", n.c_str());
    unsigned d = 0; bool p = true, q = false;
    const unsigned a = zmackni(0, 96, &d, &q); p = p && q;
    const unsigned b = zmackni(0, 97, &d, &q); p = p && q;
    const unsigned y = zmackni(0, 100, &d, &q); p = p && q;
    const unsigned l1 = zmackni(0, 102, &d, &q); p = p && q;
    const unsigned se = zmackni(0, 109, &d, &q); p = p && q;
    const unsigned h = zmackni(0, 110, &d, &q); p = p && q;
    const unsigned r1 = zmackni(0, 103, &d, &q); p = p && q;
    const unsigned x = zmackni(0, 99, &d, &q); p = p && q;
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0.9f, 0, 15); snimky(3); const unsigned l2 = stisknuto(port(0));
    j_ovlOsy(0, 0, 0, 0, 0, 0, 0, 0, 15); snimky(3); const unsigned l2p = stisknuto(port(0));
    std::printf("A %s | B %s | Y %s | L1 %s | SELECT %s | HOME %s | R1 %s | X %s | spoust L2 %s, pustena %s\n", bity(a).c_str(), bity(b).c_str(),
                bity(y).c_str(), bity(l1).c_str(), bity(se).c_str(), bity(h).c_str(), bity(r1).c_str(), bity(x).c_str(), bity(l2).c_str(), bity(l2p).c_str());
    over(n == "A=KOLECKO;B=KRIZEK;X=CTVEREC;Y=L1;L1=TROJUHELNIK;R1=NAHORU;L2=KRIZEK;R2=R2;L3=L3;R3=R3;START=START;SELECT=START;MODE=SELECT" &&
             a == PS_KOLECKO && b == PS_KRIZEK && y == PS_L1 && l1 == PS_TROJ && se == PS_START && h == PS_SELECT && r1 == PS_NAHORU &&
             x == PS_CTVEREC && l2 == PS_KRIZEK && l2p == 0 && p,
         "2a) vlastni mapa: krizek a kolecko prohozene, Y = L1, L1 = trojuhelnik, SELECT = START, HOME = SELECT, R1 = nahoru, "
         "analogova spoust L2 = krizek; FIRE neni cil PS1 (X zustal ctverec); po pusteni nic");
    const std::string dn = j_denik();
    over(dn.find("OVLADAC TLACITKA (B355): A(spodni)=KOLECKO, B(prave)=KRIZEK") != std::string::npos,
         "2b) denik PS1: \"OVLADAC TLACITKA (B355): A(spodni)=KOLECKO, B(prave)=KRIZEK ...\"");
  }
  // 3) hrac 2, navrat
  {
    unsigned d = 0;
    const unsigned a2 = zmackni(1, 96, &d);
    const unsigned d1 = d;
    const unsigned h2 = zmackni(1, 110, &d);
    std::printf("hrac 2: A %s (port 1 %s), HOME %s\n", bity(a2).c_str(), bity(d1).c_str(), bity(h2).c_str());
    over(a2 == PS_KOLECKO && d1 == 0 && h2 == PS_SELECT, "3a) hrac 2 (port 2) se stejnou mapou: A = kolecko, HOME = SELECT; port 1 nic");
    const std::string v = j_mapa("");
    const unsigned a = zmackni(0, 96), h = zmackni(0, 110);
    std::printf("mapa \"\" -> %s | A %s, HOME %s\n", v.c_str(), bity(a).c_str(), bity(h).c_str());
    over(v == "A=KRIZEK;B=KOLECKO;X=CTVEREC;Y=TROJUHELNIK;L1=L1;R1=R1;L2=L2;R2=R2;L3=L3;R3=R3;START=START;SELECT=SELECT;MODE=MENU" &&
             a == PS_KRIZEK && h == 0,
         "3b) mapa \"\" = zase vychozi (A = krizek, HOME = MENU)");
  }
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
