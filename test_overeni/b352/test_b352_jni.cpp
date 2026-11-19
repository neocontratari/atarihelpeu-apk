// test_b352_jni.cpp - B352: SKUTECNY nap_atari_native.cpp (vlakna jako v telefonu, stuby z ../b291/stub) -
// rozsirena pamet 1088K se zapne sama pro program, ktery hleda vic bank nez 320K:
//  1) samokontrola jadra: cpuHash 4A2C6CBA a memHash B0649DC5 beze zmeny (vychozi 320K)
//  2) XEX s testem pameti jako Tony (vlastni kod z vyrob_b352.py) -> appka ho spusti znovu s 1088K -> 32 bank;
//     log "B352 PAMET: XEX ... hleda vic bank" a "studeny start s 1088K"; LOG/CHYBA (devDiagNative) ukaze 1088K
//  3) stejny XEX znovu (jako z XEX/MOBIL) -> rovnou 1088K, bez dalsiho restartu
//  4) XEX pro 130XE (4 banky) -> zustava 320K
//  5) bootovaci disketa s testem Tony (devLoadAtrNative) -> znovu s 1088K, disketa zustava v D1:
//  6) volitelne skutecna hra (TONY_ZAMEK_VBXE_DEV31_PARTIAL.xex Reneho - v repu neni): nahraje se a bezi ve VBXE
// Preklad (z test_overeni/b352, JAVA_HOME = JDK kvuli jni.h, vse na jednom radku):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b352_jni test_b352_jni.cpp
//   ./test_b352_jni [TONY_ZAMEK_VBXE_DEV31_PARTIAL.xex]                     -> VYSTUP_test_b352_jni.txt
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include "test_b352_program.h"
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
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fGetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, jbyte *buf) { std::memcpy(buf, ((FakeArr *)a)->b.data() + st, n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.resize(n); return (jbyteArray)a; }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, n); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const std::string &co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); fflush(stdout); }
static void spi(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
static std::string obrazovka() {
  std::lock_guard<std::mutex> l(g_mStroj);
  Machine &m = *g_stroj;
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) { std::string line;
    for (int c = 0; c < 40; c++) { int v = m.mem.ram[(sav + r * 40 + c) & 0xFFFF] & 0x7F; int a; if (v < 64) a = v + 32; else if (v < 96) a = v - 64; else a = v; if (a < 32 || a > 126) a = '.'; line.push_back((char)a); }
    while (!line.empty() && line.back() == ' ') line.pop_back();
    out += line + "\n";
  }
  return out;
}
static std::string ctl(const char *c) { FakeStr s{c}; return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&s))->s; }
static int peekS(int a) { std::lock_guard<std::mutex> l(g_mStroj); return (int)g_stroj->mem.ram[a & 0xFFFF]; }
static std::string g_log;
static void sberLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) g_log += ((FakeStr *)s)->s;
}
static std::vector<uint8_t> nacti(const std::string &c) {
  std::vector<uint8_t> d; FILE *f = fopen(c.c_str(), "rb"); if (!f) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); fclose(f); return d;
}


static bool nahrajXex(const uint8_t *d, size_t n, const std::string &jm) {
  FakeArr a; a.b.assign(d, d + n);
  FakeStr s{jm};
  return Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&s) == JNI_TRUE;
}
static std::string nahrajAtr(const std::vector<uint8_t> &d, const std::string &jm) {
  FakeArr a; a.b = d;
  FakeStr n{jm};
  return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n))->s;
}
static bool je1088() { std::lock_guard<std::mutex> l(g_mStroj); return g_stroj && g_stroj->mem.rezim1088; }
static int pocet(const std::string &co) { int n = 0; for (size_t p = g_log.find(co); p != std::string::npos; p = g_log.find(co, p + 1)) n++; return n; }
static void radkyLogu(const char *co) {
  for (size_t p = g_log.find(co); p != std::string::npos; p = g_log.find(co, p + 1)) printf("LOG: %s\n", g_log.substr(p, g_log.find('\n', p) - p).c_str());
}
// ceka na vysledek testu pameti ($0601 = $A5) - snimky bezi v realnem case (50 / s)
static int pockejNaTest(int ms) {
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_stroj) g_stroj->mem.ram[0x601] = 0;
  }
  for (int i = 0; i < ms / 20 && peekS(0x601) != 0xA5; i++) spi(20);
  return peekS(0x601) == 0xA5 ? peekS(0x600) : -1;
}

int main(int argc, char **argv) {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B352 - skutecny nap_atari_native.cpp: rozsirena pamet 1088K sama pro program, ktery hleda vic bank nez 320K\n\n");

  // 1) samokontrola
  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    const std::string js = ((FakeStr *)r)->s;
    const size_t p = js.find("\"cpuHash\":\""), q = js.find("\"memHash\":\"");
    const std::string h = p == std::string::npos ? "?" : js.substr(p + 11, 8), mh = q == std::string::npos ? "?" : js.substr(q + 11, 8);
    printf("SELFTEST cpuHash %s memHash %s\n", h.c_str(), mh.c_str());
    over(h == "4A2C6CBA" && mh == "B0649DC5", "samokontrola: cpuHash 4A2C6CBA a memHash B0649DC5 (OCEKAVANY_* v NativeAtariCoreBridge beze zmeny)");
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos && !je1088(), "Atari nastartovalo do READY (320K)");

  // 2) XEX s testem pameti jako Tony
  {
    over(nahrajXex(B352_XEX_TONY, sizeof B352_XEX_TONY, "pamet_tony.xex"), "devLoadXexNative(pamet_tony.xex) prijat");
    // prvni beh skonci restartem (vysledek prvniho behu se nezapise - restart prijde driv); cekat na beh s 1088K
    int n = -1;
    for (int k = 0; k < 3 && !(n == 32 && je1088()); k++) n = pockejNaTest(4000);
    sberLog();
    radkyLogu("B352 PAMET");
    printf("test pameti: %d bank, 1088K %d\n", n, (int)je1088());
    over(n == 32 && je1088() && pocet("B352 PAMET: XEX pamet_tony.xex hleda vic bank nez 320K (zapsal do okna $4000-$7FFF pod 32 ruznymi PORTB") == 1 &&
             pocet("B352 PAMET: studeny start s 1088K") == 1,
         "XEX hleda vic bank -> appka ho sama spusti znovu s 1088K (log B352 PAMET), test napocita 32 bank");
    jstring dg = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devDiagNative(&g_env, nullptr);
    const std::string diag = ((FakeStr *)dg)->s;
    over(diag.find(" z 64 (1088K)") != std::string::npos && diag.find("rozsirena 1088K (64 bank, B352)") != std::string::npos,
         "LOG/CHYBA (devDiagNative): banka \"z 64 (1088K)\", CRC \"rozsirena 1088K (64 bank, B352)\"");
  }
  // 3) stejny XEX znovu -> rovnou 1088K
  {
    over(nahrajXex(B352_XEX_TONY, sizeof B352_XEX_TONY, "pamet_tony.xex"), "stejny XEX znovu (XEX/MOBIL)");
    const int n = pockejNaTest(4000);
    sberLog();
    printf("znovu: %d bank, 1088K %d, radku 'hleda vic bank' %d, 'studeny start s 1088K' %d\n", n, (int)je1088(),
           pocet("hleda vic bank nez 320K"), pocet("B352 PAMET: studeny start s 1088K"));
    over(n == 32 && je1088() && pocet("hleda vic bank nez 320K") == 1 && pocet("B352 PAMET: studeny start s 1088K") == 2,
         "podruhe rovnou s 1088K (appka si program pamatuje), zadny dalsi restart");
  }
  // 4) XEX pro 130XE
  {
    over(nahrajXex(B352_XEX_130XE, sizeof B352_XEX_130XE, "pamet_130xe.xex"), "devLoadXexNative(pamet_130xe.xex)");
    const int n = pockejNaTest(4000);
    sberLog();
    printf("130XE: %d bank, 1088K %d\n", n, (int)je1088());
    over(n == 4 && !je1088() && pocet("hleda vic bank nez 320K") == 1, "program pro 130XE (4 banky): zustava 320K, zadny restart");
  }
  // 5) bootovaci disketa s testem Tony
  {
    std::vector<uint8_t> atr(B352_ATR_TONY_ZACATEK, B352_ATR_TONY_ZACATEK + sizeof B352_ATR_TONY_ZACATEK);
    atr.resize(16 + (size_t)B352_ATR_SEKTORU * 128, 0);
    const std::string r = nahrajAtr(atr, "pamet_tony.atr");
    printf("devLoadAtr: %s\n", r.c_str());
    int n = -1;
    for (int k = 0; k < 3 && !(n == 32 && je1088()); k++) n = pockejNaTest(6000);
    sberLog();
    radkyLogu("B352 PAMET: disketa");
    printf("disketa: %d bank, 1088K %d, D1: %s\n", n, (int)je1088(), ctl("d1").c_str());
    over(r.rfind("OK ", 0) == 0 && n == 32 && je1088() && pocet("B352 PAMET: disketa pamet_tony.atr hleda vic bank nez 320K") == 1 && ctl("d1") == "pamet_tony.atr",
         "disketa hleda vic bank -> studeny start s 1088K, disketa zustava v D1:, test napocita 32 bank");
  }
  // 6) skutecna hra
  if (argc > 1) {
    std::vector<uint8_t> d = nacti(argv[1]);
    std::string jm = argv[1];
    const size_t lom = jm.find_last_of('/');
    if (lom != std::string::npos) jm = jm.substr(lom + 1);
    over(nahrajXex(d.data(), d.size(), jm), "skutecna hra " + jm + " (" + std::to_string(d.size()) + " B) prijata");
    bool hotovo = false;
    for (int i = 0; i < 600 && !hotovo; i++) {
      spi(100); sberLog();
      hotovo = g_log.find("nahrano: 444 segmentu") != std::string::npos;
    }
    spi(3000); sberLog();
    bool xdl = false;
    { std::lock_guard<std::mutex> l(g_mStroj); xdl = g_stroj->vbx.xdlEnabled; }
    radkyLogu(("B352 PAMET: XEX " + jm).c_str());
    radkyLogu("nahrano: 444 segmentu");
    over(hotovo && je1088() && xdl && pocet("B352 PAMET: XEX " + jm + " hleda vic bank nez 320K") == 1,
         "hra: spustena znovu s 1088K, nahrano vsech 444 segmentu, bezi ve VBXE (XDL zapnuto)");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
