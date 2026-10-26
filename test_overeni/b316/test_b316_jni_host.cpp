// =====================================================================
//  B316 - CARTRIDGE PRES SKUTECNY nap_atari_native.cpp (jako appka)
//
//  Stejny princip jako test_overeni/b292/test_b292_jni_host.cpp: Android je
//  nahrazen malymi nahradami (../b291/stub), bezi skutecna vlakna emulace a
//  kresleni a test vola JNI funkce presne jako Java:
//   1) start pristroje: READY, v logu radek B316 CARTRIDGE, slot prazdny
//   2) spatne soubory .car (neni .car, 5200, pravy slot, neznamy typ, typ,
//      ktery neumime, neuplny soubor): poctiva hlaska do logu i na pristroj,
//      Atari bezi dal (program v BASICu zustal)
//   3) cartridge (Standard 8 KB sestavena testem): OS ji spusti, devCtl
//      "cart" = jmeno, hlaska na pristroji; POWER vyp/zap: cartridge zustava
//      a spusti se znovu; LOG/CHYBA ji popise
//   4) VYSUNOUT CARTRIDGE (devCtl "cart_eject"): BASIC, READY, TRIG3 = 0
//   5) jiny program cartridge vytahne: XEX, disketa ATR, TXT do BASICu,
//      VYSUNOUT DISKETU, kazeta s hrou (boot); a obracene - cartridge
//      vysune disketu z D1:
//   6) Reneho Wonderboy_VBXE.car (1. argument): hra bezi pres vlakna
//      (prepinani bank, VBXE, obraz pro TV neni cerny), POWER vyp/zap
//      ji spusti znovu
//
//  g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//      -I../../app/src/main/cpp/atari -I../b292 -o test_b316_jni_host test_b316_jni_host.cpp
//  ./test_b316_jni_host [Wonderboy_VBXE.car]
// =====================================================================
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include "syntaz_kazeta.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// ---------- nahrady OpenSL: zvuk "neni k dispozici" ----------
static const SLInterfaceID_ iidE{1}, iidP{2}, iidQ{3};
const SLInterfaceID SL_IID_ENGINE = &iidE;
const SLInterfaceID SL_IID_PLAY = &iidP;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &iidQ;
SLresult slCreateEngine(SLObjectItf *, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) { return 1; }

// ---------- nahrada okna displeje ----------
struct ANativeWindow { int w = 0, h = 0, fmt = 0; std::vector<uint32_t> buf; int refs = 1; int posts = 0; };
static ANativeWindow g_fakeWin;
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *w) { return w->w; }
int32_t ANativeWindow_getHeight(ANativeWindow *w) { return w->h; }
int32_t ANativeWindow_getFormat(ANativeWindow *w) { return w->fmt; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *w, int32_t width, int32_t height, int32_t format) {
  w->w = width; w->h = height; w->fmt = format; w->buf.assign((size_t)width * height, 0); return 0;
}
int32_t ANativeWindow_lock(ANativeWindow *w, ANativeWindow_Buffer *b, ARect *) {
  b->width = w->w; b->height = w->h; b->stride = w->w; b->format = w->fmt;
  w->buf.resize((size_t)b->stride * w->h); b->bits = w->buf.data(); return 0;
}
int32_t ANativeWindow_unlockAndPost(ANativeWindow *w) { w->posts++; return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject s) { if (!s) return nullptr; g_fakeWin.refs++; return &g_fakeWin; }

// ---------- rucne sestaveny JNIEnv ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; };
struct FakeIntArr { std::vector<uint8_t> b; std::vector<int32_t> v; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fGetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, jbyte *buf) { std::memcpy(buf, ((FakeArr *)a)->b.data() + st, n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.resize(n); return (jbyteArray)a; }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, n); }
static void JNICALL fSetIntArrayRegion(JNIEnv *, jintArray a, jsize st, jsize n, const jint *buf) { std::memcpy(((FakeIntArr *)a)->v.data() + st, buf, n * 4); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const std::string &co) { kontrol++; if (!ok) chyb++; std::printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); std::fflush(stdout); }
static void spi(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
static std::string g_log;                 // vse, co C++ poslalo do logu (jako appendNativeLog v Jave)
static void sberLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) g_log += ((FakeStr *)s)->s;
}
static bool vLogu(const std::string &t) { sberLog(); return g_log.find(t) != std::string::npos; }
static void vypisLogB316() {
  sberLog();
  size_t p = 0;
  while (p < g_log.size()) {
    size_t e = g_log.find('\n', p); if (e == std::string::npos) e = g_log.size();
    const std::string r = g_log.substr(p, e - p);
    if (r.find("B316") != std::string::npos) std::printf("   LOG| %s\n", r.c_str());
    p = e + 1;
  }
}
static std::string obrazovka() {
  std::lock_guard<std::mutex> l(g_mStroj);
  Machine &m = *g_stroj;
  const int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    std::string line;
    for (int c = 0; c < 40; c++) { int v = m.mem.ram[(sav + r * 40 + c) & 0xFFFF] & 0x7F; int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v); if (a < 32 || a > 126) a = '.'; line.push_back((char)a); }
    while (!line.empty() && line.back() == ' ') line.pop_back();
    out += line + "\n";
  }
  return out;
}
static bool cekejText(const char *t, int ms) { for (int i = 0; i < ms / 100; i++) { if (obrazovka().find(t) != std::string::npos) return true; spi(100); } return obrazovka().find(t) != std::string::npos; }
static bool cekejRam(uint16_t a, uint8_t v, int ms) {
  for (int i = 0; i < ms / 50; i++) { { std::lock_guard<std::mutex> l(g_mStroj); if (g_stroj->mem.ram[a] == v) return true; } spi(50); }
  return false;
}
static int klep(float ux, float uy, int pid = 0, int drzetMs = 70) {
  float x, y;
  { std::lock_guard<std::mutex> dl(g_mDev); x = devGet().X(ux); y = devGet().Y(uy); }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 0, x, y);
  spi(drzetMs);
  int r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 1, x, y);
  spi(90);
  return r;
}
static void napis(const char *t) {
  FakeStr s{t};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&s, JNI_FALSE);
}
static std::string ctl(const char *c) {
  FakeStr s{c};
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&s);
  return r ? ((FakeStr *)r)->s : std::string("(null)");
}
static std::string nactiCar(const std::vector<uint8_t> &d, const char *jm) {
  FakeArr a; a.b = d;
  FakeStr n{jm};
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadCarNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n);
  const std::string s = r ? ((FakeStr *)r)->s : std::string("(null)");
  std::printf("   devLoadCar(%s, %zu B) -> %s\n", jm, d.size(), s.c_str());
  return s;
}
static std::string hlaska() { std::lock_guard<std::mutex> dl(g_mDev); return devGet().extraMsg; }
static std::vector<uint8_t> carSoubor(uint32_t typ, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> f(16 + data.size());
  f[0] = 'C'; f[1] = 'A'; f[2] = 'R'; f[3] = 'T';
  f[4] = (uint8_t)(typ >> 24); f[5] = (uint8_t)(typ >> 16); f[6] = (uint8_t)(typ >> 8); f[7] = (uint8_t)typ;
  uint32_t s = 0; for (uint8_t b : data) s += b;
  f[8] = (uint8_t)(s >> 24); f[9] = (uint8_t)(s >> 16); f[10] = (uint8_t)(s >> 8); f[11] = (uint8_t)s;
  if (!data.empty()) std::memcpy(&f[16], data.data(), data.size());
  return f;
}
// Standard 8 KB: zapise $42 do $0600, TRIG3 do $0601 a ceka
static std::vector<uint8_t> std8() {
  std::vector<uint8_t> d(0x2000, 0xEA);
  const uint8_t kod[] = {0xA9, 0x42, 0x8D, 0x00, 0x06, 0xAD, 0x13, 0xD0, 0x8D, 0x01, 0x06, 0x4C, 0x0B, 0xA0};
  std::memcpy(&d[0], kod, sizeof kod);
  d[0x30] = 0x60;
  const uint8_t vek[] = {0x00, 0xA0, 0x00, 0x04, 0x30, 0xA0};
  std::memcpy(&d[0x1FFA], vek, sizeof vek);
  return carSoubor(1, d);
}
// disketa ATR (720 x 128 B): boot sektor zapise $99 do $0600
static std::vector<uint8_t> atrDisketa() {
  std::vector<uint8_t> a(16 + 720 * 128, 0);
  a[0] = 0x96; a[1] = 0x02; a[2] = (uint8_t)((720 * 128 / 16) & 0xFF); a[3] = (uint8_t)((720 * 128 / 16) >> 8); a[4] = 0x80;
  const uint8_t boot[] = {0x00, 0x01, 0x00, 0x07, 0x0E, 0x07, 0xA9, 0x99, 0x8D, 0x00, 0x06, 0x4C, 0x0B, 0x07, 0x60};
  std::memcpy(&a[16], boot, sizeof boot);
  return a;
}
static std::vector<uint8_t> nactiSoubor(const char *p) {
  std::vector<uint8_t> d; FILE *f = std::fopen(p, "rb"); if (!f) return d;
  uint8_t b[65536]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); std::fclose(f); return d;
}
static void power() { klep(8 + 17, 393 + 25); spi(400); klep(8 + 17, 393 + 25); }
static bool cartVeSlotu(const char *jm) { std::lock_guard<std::mutex> l(g_mStroj); return g_stroj->cart.vlozena() && g_stroj->cart.jmeno == jm; }
static bool slotPrazdny() { std::lock_guard<std::mutex> l(g_mStroj); return !g_stroj->cart.vlozena(); }

int main(int argc, char **argv) {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.SetIntArrayRegion = fSetIntArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};

  std::printf("B316 - cartridge pres skutecny nap_atari_native.cpp (vlakna emulace a kresleni jako v appce)\n\n1) START\n");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  over(cekejText("READY", 16000), "Atari nastartovalo do READY (bez cartridge jako dosud)");
  over(vLogu("B316 CARTRIDGE: soubory .car"), "v logu je radek B316 CARTRIDGE (co jadro umi)");
  over(ctl("cart").empty(), "devCtl \"cart\" = \"\" (slot prazdny)");
  { std::lock_guard<std::mutex> l(g_mStroj); over(g_stroj->ioRead(0xD013) == 0, "TRIG3 = 0"); }

  std::printf("\n2) SPATNE SOUBORY - poctiva hlaska, Atari bezi dal\n");
  napis("10 REM PROGRAM B316\n");
  spi(1500);
  {
    const std::vector<uint8_t> xex = {0xFF, 0xFF, 0x00, 0x20, 0x01, 0x20, 0x60, 0x00, 0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20, 0x00, 0x00, 0x00, 0x00};
    std::string r = nactiCar(xex, "hra.car");
    over(r.find("CHYBA hra.car neni .car") == 0 && hlaska() == u8"NENÍ .CAR: hra.car", "XEX prejmenovany na .car: \"neni .car\" + na pristroji NENÍ .CAR");
    std::vector<uint8_t> d(32 * 1024, 0);
    r = nactiCar(carSoubor(4, d), "kosmo.car");
    over(r.find("Atari 5200") != std::string::npos && hlaska() == "CARTRIDGE PRO ATARI 5200", "typ 4: cartridge pro Atari 5200");
    d.assign(8 * 1024, 0);
    r = nactiCar(carSoubor(21, d), "pravy.car");
    over(r.find("praveho slotu") != std::string::npos && hlaska() == u8"JEN PRO PRAVÝ SLOT ATARI 800", "typ 21: pravy slot (130XE ho nema)");
    r = nactiCar(carSoubor(200, d), "divny.car");
    over(r.find("neznamy typ cartridge 200") != std::string::npos && hlaska() == u8"NEZNÁMÝ TYP CARTRIDGE 200", "typ 200: neznamy typ");
    d.assign(32 * 1024, 0);
    r = nactiCar(carSoubor(47, d), "ast.car");
    over(r.find("zatim neumim") != std::string::npos && hlaska() == u8"CARTRIDGE TYP 47 ZATÍM NEUMÍM", "typ 47 (AST 32 KB): zatim neumim");
    d.assign(4 * 1024, 0);
    std::vector<uint8_t> f = carSoubor(1, d);
    r = nactiCar(f, "pulka.car");
    over(r.find("kratsi") != std::string::npos && hlaska() == u8"SOUBOR .CAR JE NEÚPLNÝ", "typ 1 jen se 4 kB dat: soubor je neuplny");
    over(ctl("cart").empty() && slotPrazdny(), "po spatnych souborech je slot porad prazdny");
    napis("LIST\n");
    over(cekejText("10 REM PROGRAM B316", 6000), "Atari se nerestartovalo - program v BASICu zustal (LIST)");
  }

  std::printf("\n3) CARTRIDGE STANDARD 8 KB\n");
  {
    const std::string r = nactiCar(std8(), "std8.car");
    over(r.find("OK std8.car: typ 1 (Standard 8 KB), 8 kB, kontrolni soucet OK") == 0, "devLoadCar: OK, typ, velikost, kontrolni soucet");
    over(hlaska() == "CARTRIDGE: std8.car", "na pristroji CARTRIDGE: std8.car");
    over(cekejRam(0x600, 0x42, 8000), "OS cartridge spustil (program cartridge zapsal $42 do $0600)");
    { std::lock_guard<std::mutex> l(g_mStroj); over(g_stroj->mem.ram[0x601] == 1, "program cartridge cte TRIG3 = 1"); }
    over(ctl("cart") == "std8.car", "devCtl \"cart\" = std8.car (XEX/MOBIL nabidne VYSUNOUT)");
    over(vLogu("B316 CARTRIDGE VLOZENA OK std8.car"), "v logu B316 CARTRIDGE VLOZENA");
    { std::lock_guard<std::mutex> l(g_mStroj); g_stroj->mem.ram[0x600] = 0; }
    power();
    over(cekejRam(0x600, 0x42, 8000), "POWER vyp/zap: cartridge zustala ve slotu a spustila se znovu");
    over(cartVeSlotu("std8.car"), "po POWER je std8.car porad ve slotu");
    FakeStr dummy{""};
    jstring dg = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devDiagNative(&g_env, nullptr);
    const std::string diag = dg ? ((FakeStr *)dg)->s : "";
    over(diag.find("cartridge std8.car typ 1 (Standard 8 KB)") != std::string::npos, "LOG/CHYBA popise cartridge (jmeno, typ, okna, TRIG3)");
  }

  std::printf("\n4) VYSUNOUT CARTRIDGE\n");
  {
    const std::string r = ctl("cart_eject");
    const std::string h = hlaska();                     // hlaska sviti 4 s - kontrola hned
    over(r.empty(), "devCtl \"cart_eject\" probehl");
    over(cekejText("READY", 12000), "Atari se zapnulo znovu s BASICem (READY)");
    over(slotPrazdny() && ctl("cart").empty(), "slot prazdny");
    { std::lock_guard<std::mutex> l(g_mStroj); over(g_stroj->ioRead(0xD013) == 0, "TRIG3 = 0"); }
    over(vLogu("B316 CARTRIDGE std8.car vysunuta - studeny start (BASIC)") && h == "CARTRIDGE VYSUNUTA", "log + na pristroji CARTRIDGE VYSUNUTA");
  }

  std::printf("\n5) JINY PROGRAM CARTRIDGE VYTAHNE (a cartridge vysune disketu)\n");
  {
    // XEX (Turbo-BASIC XL z appky)
    nactiCar(std8(), "std8.car");
    cekejRam(0x600, 0x42, 8000);
    FakeArr tb; tb.b = nactiSoubor("../../app/src/main/assets/emu_atari_cpp/turbo_basic_xl.xex");
    FakeStr tn{"turbo_basic_xl.xex"};
    const jboolean ok = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(&g_env, nullptr, (jbyteArray)&tb, (jstring)&tn);
    over(ok == JNI_TRUE && slotPrazdny(), "XEX: cartridge ze slotu ven, XEX se zavadi");
    over(vLogu("B316 CARTRIDGE std8.car vyjmuta ze slotu (spousti se XEX)"), "log: vyjmuta ze slotu (spousti se XEX)");
    over(vLogu("turbo_basic_xl.xex: INIT c.1 prevzal rizeni") || cekejText("READY", 15000), "Turbo-BASIC XL se nahral a prevzal rizeni");
    bool basicVyp = false;
    if (cekejText("READY", 15000)) { std::lock_guard<std::mutex> l(g_mStroj); basicVyp = (g_stroj->mem.portB() & 2) != 0; }
    over(basicVyp, "READY od Turbo-BASICu (ATARI BASIC vypnuty) - cartridge mu neprekazi");
    // disketa ATR
    nactiCar(std8(), "std8.car");
    cekejRam(0x600, 0x42, 8000);
    FakeArr atr; atr.b = atrDisketa();
    FakeStr an{"boot.atr"};
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&atr, (jstring)&an);
    std::printf("   devLoadAtr -> %s\n", ((FakeStr *)r)->s.c_str());
    over(slotPrazdny() && vLogu("B316 CARTRIDGE std8.car vyjmuta ze slotu (bootuje disketa ATR)"), "ATR: cartridge ze slotu ven (log)");
    over(cekejRam(0x600, 0x99, 10000), "disketa nabootovala (boot sektor zapsal $99)");
    // cartridge pri diskete v D1: -> disketa ven
    nactiCar(std8(), "std8.car");
    over(ctl("d1").empty() && vLogu("B316 CARTRIDGE: disketa vyjmuta z D1: (bootuje cartridge)"), "cartridge vysunula disketu z D1: (log)");
    over(cekejRam(0x600, 0x42, 8000), "a spustila se cartridge, ne disketa");
    // VYSUNOUT DISKETU (= zapnout s BASICem)
    ctl("disk_eject");
    over(slotPrazdny() && vLogu("vyjmuta ze slotu (vysunout disketu = zapnout s BASICem)"), "VYSUNOUT DISKETU: cartridge taky ven (BASIC)");
    over(cekejText("READY", 12000), "READY");
    // TXT program do ATARI BASICu
    nactiCar(std8(), "std8.car");
    cekejRam(0x600, 0x42, 8000);
    FakeArr txt; const std::string prog = "10 PRINT \"B316\"\n20 GOTO 10\n"; txt.b.assign(prog.begin(), prog.end());
    FakeStr txn{"program.txt"};
    jstring pr = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devProgramNative(&g_env, nullptr, (jbyteArray)&txt, (jstring)&txn, 0);
    std::printf("   devProgram -> %s\n", ((FakeStr *)pr)->s.c_str());
    over(slotPrazdny() && vLogu("vyjmuta ze slotu (TXT program do ATARI BASICu)"), "TXT do ATARI BASICu: cartridge ven");
    over(cekejText("20 GOTO 10", 20000), "program z TXT se napsal do BASICu");
    // kazeta s hrou (boot)
    nactiCar(std8(), "std8.car");
    cekejRam(0x600, 0x42, 8000);
    FakeArr wav; wav.b = syntazBootKazetaWav(false);
    FakeStr wn{"hra_boot.wav"};
    jstring kr = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadTapeNative(&g_env, nullptr, (jbyteArray)&wav, (jstring)&wn);
    std::printf("   devLoadTape -> %.120s...\n", ((FakeStr *)kr)->s.c_str());
    over(slotPrazdny() && vLogu("vyjmuta ze slotu (bootuje kazeta)"), "kazeta s hrou (boot): cartridge ven");
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devEjectTapeNative(&g_env, nullptr);
  }

  if (argc > 1) {
    std::printf("\n6) SKUTECNA HRA %s\n", argv[1]);
    const std::vector<uint8_t> f = nactiSoubor(argv[1]);
    const std::string r = nactiCar(f, "Wonderboy_VBXE.car");
    over(r.find("OK Wonderboy_VBXE.car: typ 42 (Atarimax 1 MB Flash (old)), 1024 kB, kontrolni soucet OK") == 0, "Wonderboy: typ 42, 1 MB, kontrolni soucet OK");
    spi(9000);
    long long zmen, vbxe, blitu; bool xdl;
    auto stav = [&]() { std::lock_guard<std::mutex> l(g_mStroj); zmen = g_stroj->cart.zmen; vbxe = g_stroj->vbx.nWrites; blitu = g_stroj->vbx.nBlits; xdl = g_stroj->vbx.xdlEnabled; };
    stav();
    std::printf("   po 9 s: prepnuti bank %lld, zapisu do VBXE %lld, blitu %lld, XDL %d\n", zmen, vbxe, blitu, xdl);
    over(zmen > 50 && vbxe > 100 && xdl, "hra bezi: prepina banky a kresli pres VBXE");
    // obraz, ktery jde na TV / PC (devGrabFrameNative 672x240)
    FakeIntArr img; img.b.resize(672 * 240); img.v.resize(672 * 240);
    const jint wh = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devGrabFrameNative(&g_env, nullptr, (jintArray)&img);
    long svetlych = 0; for (int32_t p : img.v) { const int rr = (p >> 16) & 0xFF, gg = (p >> 8) & 0xFF, bb = p & 0xFF; if (rr + gg + bb > 60) svetlych++; }
    std::printf("   obraz pro TV: %dx%d, svetlych bodu %ld\n", wh >> 16, wh & 0xFFFF, svetlych);
    over(svetlych > 10000, "obraz (i pro TV / PC) neni cerny - titulka Wonder Boy");
    // START a FIRE jako hrac (herni ovladac) -> hra
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 32); spi(300);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0); spi(4000);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 16); spi(300);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0);
    const long long blituPred = blitu;
    for (int i = 0; i < 300; i++) { spi(100); stav(); if (blitu > blituPred + 500) break; }
    std::printf("   po START / FIRE: prepnuti bank %lld, zapisu do VBXE %lld, blitu %lld\n", zmen, vbxe, blitu);
    over(blitu > blituPred + 500, "hra se rozbehla (blitter VBXE kresli postavy - stovky blitu)");
    power();
    { std::lock_guard<std::mutex> l(g_mStroj); over(g_stroj->cart.vlozena() && g_stroj->cart.stav == 0x7f, "POWER: cartridge zustala, banka 127 jako po zapnuti"); }
    spi(6000);
    stav();
    over(zmen > 50 && vbxe > 100, "po POWER se hra spustila znovu");
    vypisLogB316();
  } else {
    std::printf("\n6) skutecna hra: preskoceno (cesta k .car jako 1. argument)\n");
    vypisLogB316();
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  std::printf("\nVYSLEDEK: %d kontrol, %d CHYBA\n", kontrol, chyb);
  std::fflush(stdout);
  std::_Exit(chyb ? 1 : 0);
}
