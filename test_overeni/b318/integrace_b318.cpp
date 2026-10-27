// B318: MYS PLAYSTATION SE SKUTECNYM JADREM PS1 - jemnejsi touchpad, hra kreslici 20 obrazu / s.
//
// Rene (B317): "mys sice funguje ale je dost neomalena i pri nastaveni citlivosti na 300 je to
// divny". SKUTECNE je tu vsechno, co mys dela v telefonu:
//  - jadro PCSX-ReARMed z appky (vendor/pcsx_rearmed: libretro.c, pad.c, plugin.c, sio.c,
//    psxbios.c ... - stavba_jadra.sh), BIOS HLE z jadra,
//  - nap_ps1_native.cpp z appky (JNI ps1EglBoot / ps1EglTick / ps1BootDoMonitoru,
//    ps1MysNastav / ps1MysZrychleni / ps1MysDotyk / ps1MysStav / ps1PcMys, ps1SetInput, denik,
//    vlakno emulace, B318: po retro_run "byl to novy obraz?" -> mys),
//  - nap_ps1_mys.h (logika mysi).
// "Hra" je mys_test.exe (mys_test.S): cte ovladace pres BIOS (InitPAD / StartPAD) jako hry a scita
// posun mysi do kurzoru, pocita kliky. B318: umi byt "hra, ktera kresli jen kazdy P. snimek" (jako
// Dune 2000 - 20 obrazu za sekundu): mys si precte jen v kazdem P. snimku a v nem prepne obraz
// GP1(05h). Test P nastavuje v pameti PS1 za behu a cte vysledky hry z pameti.
// Nahrazene je jen to, co na PC neni: vykreslovac gpu_neon (ARM NEON) - prepnuti obrazu GP1(05h)
// tu nahrada cipu GPU preda frontendu libretro (vout_flip), jako by to udelal gpulib -, OpenSL ES
// (zvuk), okno Androidu.
//
// Preklad (PRAC/jadro = stavba_jadra.sh):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/integrace_b318 integrace_b318.cpp PRAC/jadro/*.o \
//       -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b318 PRAC PRAC/mys_test.exe
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

// B318: hra cte jen kazdy 3. snimek. Prst jede rovnomerne, dotyky chodi ~60x za sekundu, ale
// nerovnomerne (0, 1 nebo 2 mezi dvema snimky - jako most WebView -> Java -> C++ v telefonu).
// Vraci nejvetsi odchylku kurzoru ve hre od tretiny posunu, ktery mys poslala (hra 2 snimky ze 3
// nevidi - i na skutecnem PlayStationu), a o kolik se kurzor posunul.
struct Ctvrt { double odchylka; long hra, poslano; int cteni; };
static Ctvrt tahDune(int id, int vynutit) {
  g_mys.vynutVyhlazeni(vynutit);
  static const int vzor[] = {1, 1, 2, 0, 1, 1, 1, 2, 0, 1, 0, 2, 1, 1, 1, 2, 0, 1, 2, 0, 1, 1};
  const int N = (int)(sizeof(vzor) / sizeof(vzor[0]));
  const Port p0 = port(0);
  float x = 40;
  j_dotyk(0, id, x, 200);
  snimky(1);
  long poslano = 0;
  double maxOdch = 0;
  uint32_t ctPred = port(0).cteni;
  int cteni = 0;
  for (int k = 0; k < 120; k++) {
    for (int i = 0; i < vzor[k % N]; i++) { x += 2.6f; j_dotyk(2, id, x, 200); }
    snimek1();
    poslano += g_mys.mysDotaz(0, nap::Ps1Mys::ID_X);   // co mys v tomhle snimku poslala jadru
    const Port p = port(0);
    if (p.cteni != ctPred) {   // hra si v tomhle snimku mys precetla
      ctPred = p.cteni;
      cteni++;
      if (k > 10) maxOdch = std::max(maxOdch, std::fabs((double)rozdil(p, p0).X() - poslano / 3.0));
    }
  }
  j_dotyk(1, id, x, 200);
  snimky(8);
  const Port p = port(0);
  g_mys.vynutVyhlazeni(0);
  return Ctvrt{maxOdch, rozdil(p, p0).X(), poslano, cteni};
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b318 PRAC mys_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B318 - mys PlayStation se SKUTECNYM jadrem PS1 (PCSX-ReARMed z appky, BIOS HLE, hra mys_test.exe)\n");
  const std::string sys = prac + "/system", karta = prac + "/karta";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);

  // ======== A) start hry jako Java ps1EglBoot, snimky ps1EglTick (presne po snimcich) ========
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mys_test.exe (ps1EglBoot = " + t(boot) + ")");
  g_takt = std::chrono::steady_clock::now();
  snimky(90);
  Port p1 = port(0), p2 = port(1);
  over(p1.cteni > 60 && p1.id() == 0x41 && p1.stav() == 0 && p1.ovl == p1.cteni,
       "mys vypnuta (hra bez zaznamu): BIOS cte port 1 kazdy snimek - standardni ovladac (ID 41), " + t(p1.cteni) + " cteni (jako B316 / B317)");
  std::string st = j_mysStav();
  over(st == "0|100|32|mys_test.exe", "ps1MysStav: \"" + st + "\" (bit5 = 32: zrychleni zapnuto - vychozi)");
  j_tlacitko(0, true);
  snimky(3);
  p1 = port(0); p2 = port(1);
  over(!(p1.tlacitka & PS_KRIZEK) && (p2.tlacitka & PS_KRIZEK), "mys vypnuta: KRIZEK = tlacitko ovladace v portu 1 (" + hex(p1.tlacitka) + "), port 2 nic");
  j_tlacitko(0, false);
  snimky(3);
  const int flipu0 = g_flipu.load();
  snimky(60);
  over(g_flipu.load() - flipu0 >= 55, "hra kresli kazdy snimek (P = 1): " + t(g_flipu.load() - flipu0) + " prepnuti obrazu za 60 snimku");

  // ======== zapnuti mysi v portu 1, zrychleni vypnout (presne pocty bodu) ========
  st = j_mysNastav(1, 100);
  over(st == "1|100|33|mys_test.exe", "ps1MysNastav(1, 100) -> \"" + st + "\"");
  st = j_mysZrychleni(0);
  over(st == "1|100|1|mys_test.exe", "ps1MysZrychleni(0) -> \"" + st + "\" (bit5 dole)");
  Port pred = port(0);
  snimky(45);
  p1 = port(0); p2 = port(1);
  Port d1 = rozdil(p1, pred);
  over(p1.id() == 0x12 && p1.stav() == 0 && d1.mys > 5 && p2.stav() == 0xFF, "hra vidi v portu 1 MYS PlayStation (ID 12), port 2 prazdny (FF)");
  std::string dn = j_denik();
  over(dn.find("MYS: zrychleni VYPNUTO (kurzor jede presne jako prst)") != std::string::npos,
       "denik PS1: \"MYS: zrychleni VYPNUTO (kurzor jede presne jako prst)\"");
  {
    std::ifstream fz(karta + "/PS1_mys_pro_hry.txt");
    std::stringstream sz;
    sz << fz.rdbuf();
    over(sz.str().find("mys_test.exe\t1\t100\t0\n") != std::string::npos, "zapamatovano pro hru vcetne zrychleni: \"mys_test.exe 1 100 0\"");
  }

  // ======== tah prstem: zadny skok, presne za lanem ========
  pred = port(0);
  const float dx = 73.2f + (float)LANO * 0.894427f, dy = 36.6f + (float)LANO * 0.447214f;
  tah(1, 300, 200, 300 + dx, 200 + dy, 4);
  snimky(6);
  d1 = rozdil(port(0), pred);
  over(d1.X() == 32 && d1.Y() == 16 && d1.stiskL == 0 && d1.stiskP == 0,
       "tah 10 % x 5 % sirky obrazu za lanem 1,5 %: kurzor ve hre +32 / +16 (je " + t(d1.X()) + " / " + t(d1.Y()) + "), zadny klik");
  pred = port(0);
  {   // pomaly tah po 1 bodu stranky za snimek pres hranici lana: kurzor nejvys 1 bod za snimek
    j_dotyk(0, 2, 300, 200);
    snimky(1);
    int maxKrok = 0;
    for (int k = 1; k <= 40; k++) {
      j_dotyk(2, 2, 300.f + k, 200);
      snimek1();
      maxKrok = std::max(maxKrok, std::abs((int)g_mys.mysDotaz(0, nap::Ps1Mys::ID_X)));
    }
    j_dotyk(1, 2, 340, 200);
    snimky(6);
    d1 = rozdil(port(0), pred);
    over(d1.X() == std::lround(body(40 - LANO)) && maxKrok <= 1,
         "prst po 1 bodu za snimek pres hranici lana: kurzor plynule +" + t(d1.X()) + " (vse za lanem), nejvys " + t(maxKrok) +
             " bod za snimek - zadny skok (B317: skok o 10 bodu)");
  }
  pred = port(0);
  tah(3, 700, 300, 700 - 2 * W, 300, 2);       // 2x cela sirka doleva rychle
  snimky(10);
  Port p = port(0);
  d1 = rozdil(p, pred);
  const long ocekR = -std::lround(body(2 * W - LANO));
  over(std::labs(d1.X() - ocekR) <= 1 && p.maxdx == 127,
       "rychly tah 2x pres celou sirku: " + t(d1.X()) + " bodu (vypocet " + t(ocekR) + "), za snimek nejvys 127 (" + t(p.maxdx) + ")");

  // ======== tuknuti = levy klik, dva prsty = pravy ========
  pred = port(0);
  j_dotyk(0, 4, 200, 200);
  snimky(4);
  const int u = j_dotyk(1, 4, 201, 201);
  snimky(12);
  d1 = rozdil(port(0), pred);
  over((u & 8) && d1.stiskL == 1 && d1.snL == 5 && d1.X() == 0 && d1.stiskP == 0,
       "tuknuti (67 ms): hra vidi 1 klik levym, tlacitko 5 snimku (" + t(d1.snL) + "), kurzor stoji; dotyk vratil bit klik L");
  pred = port(0);
  j_dotyk(0, 5, 200, 200);
  j_dotyk(0, 6, 330, 220);
  snimky(5);
  const int u2 = j_dotyk(1, 6, 330, 220);
  j_dotyk(1, 5, 200, 200);
  snimky(12);
  d1 = rozdil(port(0), pred);
  over((u2 & 16) && d1.stiskP == 1 && d1.snP == 5 && d1.stiskL == 0, "tuknuti dvema prsty: hra vidi 1 klik pravym (5 snimku), levym ne");

  // ======== podrzeni 0,65 s = leve drzi + tah (vyber ramem) ========
  pred = port(0);
  j_dotyk(0, 7, 400, 200);
  snimky(30);                                       // 0,5 s - B317 uz drzelo
  const std::string stav05 = j_mysStav();
  snimky(12);                                       // 0,7 s
  const std::string stavDrzi = j_mysStav();
  snimky(6);
  j_dotyk(2, 7, 400 + 73.2f + (float)LANO, 200);
  snimky(4);
  j_dotyk(1, 7, 400 + 73.2f + (float)LANO, 200);
  snimky(6);
  d1 = rozdil(port(0), pred);
  over(stav05.find("1|100|5|") == 0 && stavDrzi.find("1|100|7|") == 0,
       "prst drzeny 0,5 s: jeste nic (" + stav05.substr(0, 8) + "), 0,7 s: leve drzi (" + stavDrzi.substr(0, 8) + " - bit1 -> DRZIM LEVE TLACITKO)");
  over(d1.stiskL == 1 && d1.snL >= 9 && d1.X() == 32, "hra: leve tlacitko 1x stisknuto a drzeno " + t(d1.snL) + " snimku, behem toho posun +" + t(d1.X()) + " (vyber ramem)");

  // ======== KRIZEK / KOLECKO / D-pad ridi mys (ovladac neni zapojeny) ========
  pred = port(0);
  j_tlacitko(0, true);
  snimky(10);
  j_tlacitko(0, false);
  j_tlacitko(8, true);
  snimky(6);
  j_tlacitko(8, false);
  snimky(3);
  d1 = rozdil(port(0), pred);
  over(d1.stiskL == 1 && d1.snL == 10 && d1.stiskP == 1 && d1.snP == 6, "KRIZEK = leve (10 snimku), KOLECKO = prave tlacitko mysi (6 snimku)");
  pred = port(0);
  j_tlacitko(7, true);
  snimky(60);
  j_tlacitko(7, false);
  snimky(4);
  d1 = rozdil(port(0), pred);
  over(d1.X() > 150 && d1.X() < 330 && d1.Y() == 0, "D-pad vpravo 1 s: kurzor +" + t(d1.X()) + " (zrychluje z 1 na 6 bodu za snimek)");

  // ======== mys z PC (prohlizec TV): 1/100000 sirky obrazu ========
  pred = port(0);
  const std::string odp = j_pcMys(-15625, 7813, 1);   // -5 % / +2,5 % sirky obrazu v prohlizeci
  snimky(5);
  j_pcMys(0, 0, 0);
  snimky(4);
  d1 = rozdil(port(0), pred);
  over(odp == "PS1:MYS" && d1.X() == -50 && d1.Y() == 25 && d1.stiskL == 1,
       "mys z PC: -15625 / +7813 (1/100000 sirky obrazu v prohlizeci) = -50 / +25 bodu a leve tlacitko ve hre");

  // ======== dva cteni v jednom snimku: posun jen jednou (oprava jadra z B317 plati) ========
  in_mouse[0][0] = 10; in_mouse[0][1] = -7;
  auto cti = [](int &x, int &y, int &id) {
    int more = 0;
    PAD1_startPoll();
    id = PAD1_poll(0x42, &more);
    PAD1_poll(0, &more);
    PAD1_poll(0, &more); PAD1_poll(0, &more);
    x = (int8_t)PAD1_poll(0, &more); y = (int8_t)PAD1_poll(0, &more);
  };
  int dx1, dy1, id1, dx2, dy2, id2;
  cti(dx1, dy1, id1);
  cti(dx2, dy2, id2);
  over(id1 == 0x12 && dx1 == 10 && dy1 == -7 && dx2 == 0 && dy2 == 0,
       "hra cte mys 2x v jednom snimku: poprve +10 / -7, podruhe 0 / 0 (posun se precte jednou - oprava B317)");
  snimky(3);

  // ======== B318: hra kresli jen kazdy 3. snimek (Dune 2000: 20 obrazu za sekundu) ========
  perioda(3);
  const int f0 = g_flipu.load();
  snimky(120);
  dn = j_denik();
  over(g_flipu.load() - f0 >= 38 && g_flipu.load() - f0 <= 42 && g_mys.vyhlazeni() == 3,
       "hra kresli kazdy 3. snimek (" + t(g_flipu.load() - f0) + " obrazu za 120 snimku): mys rozklada posun do " + t(g_mys.vyhlazeni()) + " snimku");
  const size_t zh = dn.find("MYS: hra kresli kazdy ");
  over(zh != std::string::npos && dn.find("se rozklada do 3 snimku", zh) != std::string::npos,
       "denik: \"" + (zh == std::string::npos ? std::string("-") : dn.substr(zh, dn.find('\n', zh) - zh)) + "\"");
  const Ctvrt b318 = tahDune(10, 0), b317 = tahDune(11, 1);
  over(b318.odchylka <= 2.5 && b318.cteni >= 38,
       "tah prstem, dotyky nerovnomerne, hra cte jen kazdy 3. snimek (" + t(b318.cteni) + "x): kurzor ve hre presne na tretine posunu mysi - odchylka nejvys " +
           tf(b318.odchylka) + " bodu (hra +" + t(b318.hra) + " z " + t(b318.poslano) + ")");
  over(b317.odchylka >= 2.0 * b318.odchylka && b317.odchylka >= 4.0,
       "stejne jako B317 (posun bez rozlozeni): kurzor ujizdel od tretiny az o " + tf(b317.odchylka) + " bodu (hra +" + t(b317.hra) + " z " +
           t(b317.poslano) + ") - cukal se podle toho, do kterych snimku dotyky padly");
  over(std::labs(b318.poslano - b317.poslano) <= 2,
       "mys poslala v obou pripadech stejne (" + t(b318.poslano) + " / " + t(b317.poslano) + " bodu) - soucet presny, zmena je jen v rozlozeni");
  // klik po tuknuti hra uvidi i pri cteni jen kazdy 3. snimek
  pred = port(0);
  j_dotyk(0, 12, 200, 200);
  snimky(3);
  j_dotyk(1, 12, 200, 200);
  snimky(15);
  d1 = rozdil(port(0), pred);
  over(d1.stiskL == 1 && d1.snL >= 1, "tuknuti: hra, ktera cte jen kazdy 3. snimek, klik uvidi (1 stisk, " + t(d1.snL) + " cteni s tlacitkem)");
  perioda(1);
  snimky(120);
  over(g_mys.vyhlazeni() == 2, "hra zase kresli kazdy snimek: zpet rozlozeni do 2 snimku");

  // ======== ostatni porty ========
  st = j_mysNastav(2, 100);
  snimky(45);
  j_tlacitko(0, true);
  j_tlacitko(7, true);
  snimky(4);
  p1 = port(0); p2 = port(1);
  pred = p1;
  tah(13, 100, 100, 100 + 73.2f + (float)LANO, 100, 2);
  snimky(4);
  d1 = rozdil(port(0), pred);
  over(st.rfind("2|100|", 0) == 0 && p1.id() == 0x12 && p2.id() == 0x41 && !(p2.tlacitka & PS_KRIZEK) && !(p2.tlacitka & PS_VPRAVO) &&
           p1.snL == pred.snL && d1.X() == 32 && d1.snL == 0,
       "mys port 1 + ovladac port 2: KRIZEK a smer jdou ovladaci v portu 2 (" + hex(p2.tlacitka) + "), mys jen z dotyku (+" + t(d1.X()) + ")");
  j_tlacitko(0, false);
  j_tlacitko(7, false);
  st = j_mysNastav(3, 100);
  snimky(45);
  j_tlacitko(8, true);
  snimky(3);
  p1 = port(0);
  Port p2pred = port(1);
  j_tlacitko(8, false);
  tah(14, 100, 100, 100 + 36.6f + (float)LANO * 0.447214f, 100 + 73.2f + (float)LANO * 0.894427f, 2);
  j_dotyk(0, 15, 50, 50);
  snimky(3);
  j_dotyk(1, 15, 50, 50);
  snimky(12);
  Port d2 = rozdil(port(1), p2pred);
  over(st.rfind("3|100|", 0) == 0 && p1.id() == 0x41 && !(p1.tlacitka & PS_KOLECKO) && port(1).id() == 0x12 && d2.X() == 16 && d2.Y() == 32 &&
           d2.stiskL == 1,
       "ovladac port 1 + mys port 2: KOLECKO na ovladaci v portu 1, mys v portu 2 (posun +" + t(d2.X()) + " / +" + t(d2.Y()) + ", klik)");

  // ======== vypnuti: zpet presne jako B316 ========
  st = j_mysNastav(0, 100);
  snimky(45);
  j_tlacitko(0, true);
  snimky(3);
  p1 = port(0); p2 = port(1);
  j_tlacitko(0, false);
  over(st.rfind("0|100|", 0) == 0 && p1.id() == 0x41 && !(p1.tlacitka & PS_KRIZEK) && p2.id() == 0x41 && p2.tlacitka == 0xFFFF,
       "mys vypnuta: zpet standardni ovladac v portu 1 (KRIZEK ve hre), port 2 bez stisku");
  pred = port(0);
  j_dotyk(0, 16, 100, 100);
  j_dotyk(2, 16, 300, 100);
  j_dotyk(1, 16, 300, 100);
  j_pcMys(50000, 0, 1);
  snimky(5);
  over(rozdil(port(0), pred).mys == 0 && j_pcMys(0, 0, 0) == "PS1:MYS_VYP", "vypnuta: dotyk ani mys z PC hru nijak neovlivni (PC dostane PS1:MYS_VYP)");

  // ======== nova hra: zapamatovana mys (i zrychleni) se zapoji sama hned od startu ========
  j_mysNastav(1, 150);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  const int boot2 = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  g_takt = std::chrono::steady_clock::now();
  snimky(60);
  st = j_mysStav();
  p1 = port(0);
  dn = j_denik();
  over(boot2 >= 0 && st.rfind("1|150|1|", 0) == 0 && p1.id() == 0x12 && p1.ovl == 0,
       "znovu spustena hra: mys v portu 1, 150 %, zrychleni vypnuto (zapamatovano) od prvniho snimku (" + st + ")");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  // ======== B) cesta appky: ps1BootDoMonitoru + vlakno emulace (jako telefon) ========
  FakeStr jkarta{karta};
  const std::string r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&jsys, (jstring)&jkarta, (jstring)&jhra));
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  Port v0 = port(0);
  for (int k = 0; k < 3; k++) {   // dotyk z jineho vlakna (most WebView) za chodu emulace
    j_dotyk(0, 20 + k, 100, 100);
    for (int s = 1; s <= 15; s++) { j_dotyk(2, 20 + k, 100 + (float)LANO * s, 100); std::this_thread::sleep_for(std::chrono::milliseconds(8)); }
    j_dotyk(1, 20 + k, 100 + (float)LANO * 15, 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  j_dotyk(0, 30, 100, 100);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  j_dotyk(1, 30, 100, 100);
  std::this_thread::sleep_for(std::chrono::milliseconds(6000));
  Port v1 = port(0);
  Port dv = rozdil(v1, v0);
  dn = j_denik();
  const double ocek = 3 * body(14 * LANO, 150);
  over(r == "PS1_HRA_OK" && v1.id() == 0x12 && std::fabs(dv.X() - ocek) <= 1.0 && dv.stiskL == 1,
       "telefonni cesta (ps1BootDoMonitoru, vlakno emulace, dotyk z jineho vlakna): mys od startu, 3 tahy = +" + t(dv.X()) +
           " (vypocet " + t(std::lround(ocek)) + "), 1 klik");
  const size_t z = dn.find("MYS ZDRAVI:");
  over(z != std::string::npos && dn.find("zrychleni VYP, vyhlazeni 2 snimky (hra kresli kazdy 1.0. snimek)", z) != std::string::npos &&
           dn.find("hra cte port 1 ", z) != std::string::npos,
       "denik: MYS ZDRAVI: " + (z == std::string::npos ? std::string("-") : dn.substr(z + 12, dn.find('\n', z) - z - 12)));
  const size_t zt = dn.find("MYS TAH 1: ");
  over(zt != std::string::npos, "denik: kazdy tah - " + (zt == std::string::npos ? std::string("-") : dn.substr(zt, dn.find('\n', zt) - zt)));
  jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(&g_env, nullptr));
  nap_sl_close();
  {
    std::ofstream o(prac + "/UKAZKA_denik_mys.txt");
    o << dn;
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}

