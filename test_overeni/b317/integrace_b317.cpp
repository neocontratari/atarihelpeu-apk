// B317: MYS PLAYSTATION SE SKUTECNYM JADREM PS1.
//
// SKUTECNE je tu vsechno, co mys dela v telefonu:
//  - jadro PCSX-ReARMed z appky (vendor/pcsx_rearmed: libretro.c, pad.c, plugin.c, sio.c,
//    psxbios.c ... - stavba_jadra.sh), BIOS HLE z jadra,
//  - nap_ps1_native.cpp z appky (JNI ps1EglBoot / ps1EglTick / ps1BootDoMonitoru,
//    ps1MysNastav / ps1MysDotyk / ps1MysStav, ps1SetInput, denik, vlakno emulace),
//  - nap_ps1_mys.h (logika mysi).
// "Hra" je mys_test.exe (mys_test.S): cte ovladace pres BIOS (InitPAD / StartPAD) jako hry
// a scita posun mysi do kurzoru, pocita kliky - test cte jeji vysledky z pameti PS1.
// Nahrazene je jen to, co na PC neni: vykreslovac gpu_neon (ARM NEON) a OpenSL ES (zvuk),
// okno Androidu.
//
// Preklad (PRAC/jadro = stavba_jadra.sh):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/integrace_b317 integrace_b317.cpp PRAC/jadro/*.o \
//       -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b317 PRAC PRAC/mys_test.exe
// Srovnani s B316 (frontend/plugin.c bez "posun se precte jednou"): stejne, ale misto
// PRAC/jadro/frontend_plugin.c.o objekt z PRAC/jadro_b316 a parametr --b316 (jen kontrola
// cteni 2x v jednom snimku - ukaze, ze bez opravy se posun precte dvakrat).
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

// ---------- vysledky "hry" v pameti PS1 (mys_test.S) ----------
struct Port {
  uint32_t cteni, posl, sx, sy, stiskL, stiskP, snL, snP, tlacitka, predL, predP, mys, ovl, nic, maxdx;
  int32_t X() const { return (int32_t)sx; }
  int32_t Y() const { return (int32_t)sy; }
  int stav() const { return (int)(posl >> 8); }
  int id() const { return (int)(posl & 0xFF); }
};
static Port port(int i) {
  const uint8_t *ram = (const uint8_t *)retro_get_memory_data(2 /* RETRO_MEMORY_SYSTEM_RAM */);
  Port p;
  std::memcpy(&p, ram + 0x100100 + i * 0x40, sizeof(p));
  return p;
}
static Port rozdil(const Port &b, const Port &a) {   // co se zmenilo mezi a a b
  Port d = b;
  d.cteni -= a.cteni; d.sx -= a.sx; d.sy -= a.sy; d.stiskL -= a.stiskL; d.stiskP -= a.stiskP; d.snL -= a.snL; d.snP -= a.snP;
  d.mys -= a.mys; d.ovl -= a.ovl; d.nic -= a.nic;
  return d;
}

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string t(long v) { return std::to_string(v); }
static std::string hex(unsigned v) { char b[16]; snprintf(b, sizeof(b), "%04X", v); return b; }

// snimky jadra v realnem case (60 Hz NTSC - PS-EXE): presne cesta Javy ps1EglTick
static std::chrono::steady_clock::time_point g_takt;
static void snimky(int n) {
  for (int i = 0; i < n; i++) {
    g_takt += std::chrono::microseconds(16683);
    std::this_thread::sleep_until(g_takt);
    Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglTick(&g_env, nullptr);
  }
}
// dotyk s casem: prsty se pohnou mezi snimky
static void tah(int id, float x0, float y0, float x1, float y1, int kroku) {
  j_dotyk(0, id, x0, y0);
  snimky(1);
  for (int k = 1; k <= kroku; k++) { j_dotyk(2, id, x0 + (x1 - x0) * k / kroku, y0 + (y1 - y0) * k / kroku); snimky(1); }
  j_dotyk(1, id, x1, y1);
}

// PS1 tlacitka ovladace v odpovedi (0 = stisknuto): bajt 3 bit 6 = KRIZEK, bit 5 = KOLECKO
static const unsigned PS_KRIZEK = 1u << 14, PS_KOLECKO = 1u << 13, PS_VPRAVO = 1u << 5;

static int b316(const std::string &prac, const std::string &exe);

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b317 PRAC mys_test.exe [--b316]\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  if (argc > 3 && std::string(argv[3]) == "--b316") return b316(prac, exe);
  std::printf("B317 - mys PlayStation se SKUTECNYM jadrem PS1 (PCSX-ReARMed z appky, BIOS HLE, hra mys_test.exe)\n");
  const std::string sys = prac + "/system", karta = prac + "/karta";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);

  // ======== A) start hry jako Java ps1EglBoot, snimky ps1EglTick (presne po snimcich) ========
  g_savedir = karta;   // (ps1EglBoot slozku karty nebere - nastavi ji ps1BootDoMonitoru, viz B)
  FakeStr jsys{sys}, jhra{exe};
  const int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mys_test.exe (ps1EglBoot = " + t(boot) + ")");
  g_takt = std::chrono::steady_clock::now();
  snimky(90);
  Port p1 = port(0), p2 = port(1);
  over(p1.cteni > 60 && p1.id() == 0x41 && p1.stav() == 0 && p1.ovl == p1.cteni,
       "mys vypnuta (hra bez zaznamu): BIOS cte port 1 kazdy snimek - standardni ovladac (ID 41), " + t(p1.cteni) + " cteni");
  over(p2.id() == 0x41 && p2.tlacitka == 0xFFFF, "port 2: standardni ovladac bez stisku (jako B316)");
  std::string st = j_mysStav();
  over(st == "0|100|0|mys_test.exe", "ps1MysStav: \"" + st + "\" (klic hry = jmeno souboru, protoze PS-EXE ma zastupne ID SLUS99999)");

  // ovladac (mys vypnuta): KRIZEK jde do portu 1 jako vzdy
  j_tlacitko(0, true);
  snimky(3);
  p1 = port(0); p2 = port(1);
  over(!(p1.tlacitka & PS_KRIZEK) && (p2.tlacitka & PS_KRIZEK), "mys vypnuta: KRIZEK = tlacitko ovladace v portu 1 (" + hex(p1.tlacitka) + "), port 2 nic");
  j_tlacitko(0, false);
  snimky(3);
  over((port(0).tlacitka & PS_KRIZEK) != 0, "KRIZEK pusten");

  // ======== zapnuti mysi v portu 1 (tlacitko MYS na strance) ========
  st = j_mysNastav(1, 100);
  over(st == "1|100|1|mys_test.exe", "ps1MysNastav(1, 100) -> \"" + st + "\"");
  Port pred = port(0);
  snimky(45);   // jadro po zmene portu ~32 snimku hlasi "nic zapojeno" (jako vytazeni a zasunuti)
  p1 = port(0); p2 = port(1);
  Port d1 = rozdil(p1, pred);
  over(p1.id() == 0x12 && p1.stav() == 0 && d1.mys > 5, "hra vidi v portu 1 MYS PlayStation (ID 12), " + t(d1.mys) + " cteni s mysi");
  over(p2.stav() == 0xFF, "port 2 je prazdny (BIOS: FF = nic zapojeno)");
  over(d1.nic > 25 && d1.nic < 40, "po zmene portu jadro " + t(d1.nic) + " snimku hlasi \"nic zapojeno\" (vytazeni a zasunuti, ~32)");
  std::string dn = j_denik();
  over(dn.find("MYS: mys v portu 1, port 2 prazdny (citlivost 100 %)") != std::string::npos &&
           dn.find("MYS: jadro - port 1 = mys PlayStation, port 2 = nic") != std::string::npos,
       "denik PS1: \"MYS: mys v portu 1, port 2 prazdny\" a \"MYS: jadro - port 1 = mys PlayStation, port 2 = nic\"");
  std::ifstream fz(karta + "/PS1_mys_pro_hry.txt");
  std::stringstream sz;
  sz << fz.rdbuf();
  over(sz.str().find("mys_test.exe\t1\t100\n") != std::string::npos, "zapamatovano pro hru: PS1_mys_pro_hry.txt ve slozce karty (\"mys_test.exe 1 100\")");

  // ======== tah prstem: kurzor ve hre ========
  pred = port(0);
  tah(1, 300, 200, 300 + 73.2f, 200 + 36.6f, 4);   // 10 % x 5 % sirky obrazu = 32 / 16 bodu
  snimky(4);
  d1 = rozdil(port(0), pred);
  over(d1.X() == 32 && d1.Y() == 16 && d1.stiskL == 0 && d1.stiskP == 0,
       "tah prstem 10 % x 5 % sirky obrazu: kurzor ve hre +32 / +16 (je " + t(d1.X()) + " / " + t(d1.Y()) + "), zadny klik");
  pred = port(0);
  tah(2, 700, 300, 700 - 2 * 732.f, 300, 2);       // 2x cela sirka doleva rychle = -640
  snimky(8);
  Port p = port(0);
  d1 = rozdil(p, pred);
  over(d1.X() == -640 && p.maxdx == 127, "rychly tah 2x pres celou sirku: -640 bodu, za snimek nejvys 127 (je " + t(d1.X()) + ", max " + t(p.maxdx) + ")");

  // ======== tuknuti = levy klik, dva prsty = pravy ========
  pred = port(0);
  j_dotyk(0, 3, 200, 200);
  snimky(4);
  const int u = j_dotyk(1, 3, 201, 201);
  snimky(12);
  d1 = rozdil(port(0), pred);
  over((u & 8) && d1.stiskL == 1 && d1.snL == 5 && d1.X() == 0 && d1.stiskP == 0,
       "tuknuti (67 ms): hra vidi 1 klik levym, tlacitko 5 snimku (" + t(d1.snL) + "), kurzor stoji; dotyk vratil bit klik L");
  pred = port(0);
  j_dotyk(0, 4, 200, 200);
  j_dotyk(0, 5, 330, 220);
  snimky(5);
  const int u2 = j_dotyk(1, 5, 330, 220);
  j_dotyk(1, 4, 200, 200);
  snimky(12);
  d1 = rozdil(port(0), pred);
  over((u2 & 16) && d1.stiskP == 1 && d1.snP == 5 && d1.stiskL == 0, "tuknuti dvema prsty: hra vidi 1 klik pravym (5 snimku), levym ne");

  // ======== podrzeni = leve drzi + tah (vyber ramem) ========
  pred = port(0);
  j_dotyk(0, 6, 400, 200);
  snimky(36);                                       // 0,6 s
  const std::string stavDrzi = j_mysStav();
  j_dotyk(2, 6, 400 + 73.2f, 200);
  snimky(3);
  j_dotyk(1, 6, 400 + 73.2f, 200);
  snimky(6);
  d1 = rozdil(port(0), pred);
  over(stavDrzi.find("1|100|7|") == 0, "prst drzeny 0,6 s: stav pro stranku " + stavDrzi.substr(0, 8) + " (bit1 = leve drzi -> DRZIM LEVE TLACITKO)");
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
  j_tlacitko(7, true);   // vpravo
  snimky(60);            // 1 s: zrychli z 1 na 6 bodu za snimek
  j_tlacitko(7, false);
  snimky(3);
  d1 = rozdil(port(0), pred);
  over(d1.X() > 150 && d1.X() < 330 && d1.Y() == 0, "D-pad vpravo 1 s: kurzor +" + t(d1.X()) + " (zrychluje z 1 na 6 bodu za snimek)");

  // ======== mys z PC (prohlizec TV) ========
  pred = port(0);
  const std::string odp = j_pcMys(-50, 25, 1);
  snimky(5);
  j_pcMys(0, 0, 0);
  snimky(3);
  d1 = rozdil(port(0), pred);
  over(odp == "PS1:MYS" && d1.X() == -50 && d1.Y() == 25 && d1.stiskL == 1, "mys z PC: posun -50 / +25 a leve tlacitko prisly do hry");

  // ======== dva cteni v jednom snimku: posun jen jednou (jako skutecna mys) ========
  in_mouse[0][0] = 10; in_mouse[0][1] = -7;
  auto cti = [](int &dx, int &dy, int &id) {
    int more = 0;
    PAD1_startPoll();
    id = PAD1_poll(0x42, &more);
    PAD1_poll(0, &more);
    PAD1_poll(0, &more); PAD1_poll(0, &more);
    dx = (int8_t)PAD1_poll(0, &more); dy = (int8_t)PAD1_poll(0, &more);
  };
  int dx1, dy1, id1, dx2, dy2, id2;
  cti(dx1, dy1, id1);
  cti(dx2, dy2, id2);
  over(id1 == 0x12 && dx1 == 10 && dy1 == -7 && dx2 == 0 && dy2 == 0,
       "hra cte mys 2x v jednom snimku: poprve +10 / -7, podruhe 0 / 0 (posun se precte jednou - jinak by kurzor jel 2x rychleji)");

  // ======== ostatni porty ========
  st = j_mysNastav(2, 100);
  snimky(45);
  j_tlacitko(0, true);
  j_tlacitko(7, true);
  snimky(4);
  p1 = port(0); p2 = port(1);
  pred = p1;
  tah(7, 100, 100, 100 + 73.2f, 100, 2);
  snimky(3);
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
  tah(8, 100, 100, 100 + 36.6f, 100 + 73.2f, 2);
  j_dotyk(0, 9, 50, 50);
  snimky(3);
  j_dotyk(1, 9, 50, 50);
  snimky(10);
  Port d2 = rozdil(port(1), p2pred);
  over(st.rfind("3|100|", 0) == 0 && p1.id() == 0x41 && !(p1.tlacitka & PS_KOLECKO) && port(1).id() == 0x12 && d2.X() == 16 && d2.Y() == 32 &&
           d2.stiskL == 1,
       "ovladac port 1 + mys port 2: KOLECKO na ovladaci v portu 1, mys v portu 2 (posun +16 / +32, klik)");

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
  j_dotyk(0, 10, 100, 100);
  j_dotyk(2, 10, 300, 100);
  j_dotyk(1, 10, 300, 100);
  snimky(5);
  over(rozdil(port(0), pred).mys == 0, "vypnuta: dotyk na obraze hru nijak neovlivni");

  // ======== nova hra: zapamatovana mys se zapoji sama hned od startu ========
  j_mysNastav(1, 150);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  const int boot2 = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  g_takt = std::chrono::steady_clock::now();
  snimky(60);
  st = j_mysStav();
  p1 = port(0);
  dn = j_denik();
  over(boot2 >= 0 && st.rfind("1|150|", 0) == 0 && p1.id() == 0x12 && p1.ovl == 0,
       "znovu spustena hra: mys v portu 1 (citlivost 150 %) od prvniho snimku - hra ovladac vubec nevidela (" + st + ")");
  over(dn.find("MYS: hra mys_test.exe - mys v portu 1, port 2 prazdny, citlivost 150 % (zapamatovano pro tuto hru)") != std::string::npos,
       "denik: \"MYS: hra mys_test.exe - mys v portu 1, port 2 prazdny, citlivost 150 % (zapamatovano pro tuto hru)\"");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  // ======== B) cesta appky: ps1BootDoMonitoru + vlakno emulace (jako telefon) ========
  FakeStr jkarta{karta};
  const std::string r = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&jsys, (jstring)&jkarta, (jstring)&jhra));
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  Port v0 = port(0);
  // dotyk z jineho vlakna (most WebView) za chodu emulace
  for (int k = 0; k < 3; k++) {
    j_dotyk(0, 20 + k, 100, 100);
    for (int s = 1; s <= 10; s++) { j_dotyk(2, 20 + k, 100 + 7.32f * 1.5f * s, 100); std::this_thread::sleep_for(std::chrono::milliseconds(8)); }
    j_dotyk(1, 20 + k, 100 + 7.32f * 15, 100);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  j_dotyk(0, 30, 100, 100);
  std::this_thread::sleep_for(std::chrono::milliseconds(60));
  j_dotyk(1, 30, 100, 100);
  std::this_thread::sleep_for(std::chrono::milliseconds(6000));
  Port v1 = port(0);
  Port dv = rozdil(v1, v0);
  dn = j_denik();
  const double ocek = 3 * 15 * 7.32 * 320.0 / 732.0 * 1.5;
  over(r == "PS1_HRA_OK" && v1.id() == 0x12 && std::fabs(dv.X() - ocek) <= 1.0 && dv.stiskL == 1,
       "telefonni cesta (ps1BootDoMonitoru, vlakno emulace, dotyk z jineho vlakna): mys od startu, 3 tahy = +" + t(dv.X()) +
           " (vypocet " + t((long)std::lround(ocek)) + "), 1 klik");
  const size_t z = dn.find("MYS ZDRAVI:");
  over(z != std::string::npos && dn.find("hra cte port 1 ", z) != std::string::npos,
       "denik: radek MYS ZDRAVI s tim, jak casto hra cte port 1 / 2: " + (z == std::string::npos ? std::string("-") : dn.substr(z, dn.find('\n', z) - z)));
  jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(&g_env, nullptr));
  nap_sl_close();
  {
    std::ofstream o(prac + "/UKAZKA_denik_mys.txt");
    o << dn;
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}

// ---- srovnani s B316: frontend/plugin.c bez "posun se precte jednou" ----
static int b316(const std::string &prac, const std::string &exe) {
  std::printf("B316 (frontend/plugin.c z commitu B316) - mys cteni 2x v jednom snimku\n");
  const std::string sys = prac + "/system";
  g_savedir = prac + "/karta";
  FakeStr jsys{sys}, jhra{exe};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  g_takt = std::chrono::steady_clock::now();
  j_mysNastav(1, 100);
  snimky(45);
  in_mouse[0][0] = 10; in_mouse[0][1] = -7;
  int dx[2], dy[2], id[2];
  for (int i = 0; i < 2; i++) {
    int more = 0;
    PAD1_startPoll();
    id[i] = PAD1_poll(0x42, &more);
    PAD1_poll(0, &more); PAD1_poll(0, &more); PAD1_poll(0, &more);
    dx[i] = (int8_t)PAD1_poll(0, &more); dy[i] = (int8_t)PAD1_poll(0, &more);
  }
  std::printf("B316: 1. cteni %d / %d, 2. cteni %d / %d (ID %02X)\n", dx[0], dy[0], dx[1], dy[1], id[0]);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  // B316 = posun se precte 2x stejny (chyba), vystup pro srovnani
  return (dx[1] == 10 && dy[1] == -7) ? 0 : 1;
}
