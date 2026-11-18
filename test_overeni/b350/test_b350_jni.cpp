// test_b350_jni.cpp - B350: SKUTECNY nap_atari_native.cpp (vlakna jako v telefonu, stuby z ../b291/stub) -
// hra v RAR 5 (novy WinRAR) presne tak, jak ji dostane appka:
//  1) samokontrola jadra (cpuHash beze zmeny - B350 jadro Atari nemeni)
//  2) RAR5 z vyrob_rar5.py: strana B (v archivu prvni), navod, strana A (program z ../b348) ->
//     devArchivNative vrati stranu A -> devLoadAtrNative = boot; "archiv_seznam" pro ATR / DISK
//  3) VYMENIT DISKETU BEZ RESTARTU z RAR5: devArchivSouborNative(1) + "atr_vymena" + devLoadAtrNative ->
//     program bezi dal a vlastnim SIO pres POKEY cte sektor 4 strany B
//  4) Prince of Persia v RAR5 (DD / SDA / SDB z vyrob_rar5.py) -> spusti DD; radky B349 ARCHIV ... (RAR5 ...) v logu
// Preklad (z test_overeni/b350, JAVA_HOME = JDK kvuli jni.h, vse na jednom radku):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b350_jni test_b350_jni.cpp
//   ./test_b350_jni /tmp/b350_archivy                               -> VYSTUP_test_b350_jni.txt
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include "../b348/test_b348_program.h"
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

// diskety jako v ../b348/test_b348_jni.cpp: strana A = bootovaci s programem, strana B = data
static uint8_t obsah(int s, int i) { return (uint8_t)(s * 7 + i * 13 + (s >> 8) * 5 + 1); }
static std::vector<uint8_t> atr(int sektoru, bool sProgramem) {
  std::vector<uint8_t> a;
  const size_t bajtu = (size_t)sektoru * 128, par = bajtu / 16;
  a.assign(16 + bajtu, 0);
  a[0] = 0x96; a[1] = 0x02; a[2] = par & 0xFF; a[3] = (par >> 8) & 0xFF; a[4] = 128;
  for (int s = 1; s <= sektoru; s++) for (int i = 0; i < 128; i++) a[16 + (size_t)(s - 1) * 128 + i] = obsah(s, i);
  if (sProgramem) for (size_t i = 0; i < sizeof(B348_KOD); i++) a[16 + i] = B348_KOD[i];
  return a;
}
// devArchivNative jako z Javy: nullptr = neni archiv, jinak obsah souboru (prazdny = nic ke spusteni)
static FakeArr *otevriArchiv(const std::vector<uint8_t> &d, const char *jm, bool vymena) {
  FakeArr a; a.b = d;
  FakeStr n{jm};
  return (FakeArr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devArchivNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n, vymena ? JNI_TRUE : JNI_FALSE);
}
static FakeArr *archivSoubor(int i) { return (FakeArr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devArchivSouborNative(&g_env, nullptr, i); }
static std::string nahraj(const std::vector<uint8_t> &d, const std::string &jm) {
  FakeArr a; a.b = d;
  FakeStr n{jm};
  return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n))->s;
}

int main(int argc, char **argv) {
  if (argc < 2) { printf("pouziti: %s SLOZKA_Z_vyrob_rar5.py\n", argv[0]); return 2; }
  const std::string dir = argv[1];
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B350 - skutecny nap_atari_native.cpp: hra v RAR 5 (devArchivNative, nabidka disket, vymena bez restartu)\n\n");

  // 1) samokontrola
  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    const std::string js = ((FakeStr *)r)->s;
    const size_t p = js.find("\"cpuHash\":\"");
    const std::string h = p == std::string::npos ? "?" : js.substr(p + 11, 8);
    printf("SELFTEST cpuHash %s\n", h.c_str());
    over(h == "4A2C6CBA", "samokontrola: cpuHash 4A2C6CBA = OCEKAVANY_CPU_HASH (B350 jadro Atari nemeni)");
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");

  // 2) RAR5 se stranami A / B (strana B v archivu prvni)
  const std::vector<uint8_t> stranaA = atr(720, true), stranaB = atr(720, false);
  const std::vector<uint8_t> rar = nacti(dir + "/vyber_strany5.rar");
  over(rar.size() > 8 && std::memcmp(rar.data(), "Rar!\x1A\x07\x01\x00", 8) == 0, "vyber_strany5.rar je RAR 5 (signatura Rar! 1A 07 01 00)");
  {
    FakeArr *r = otevriArchiv(rar, "hra5.rar", false);
    over(r && r->b == stranaA, "devArchivNative(hra5.rar): vrati stranu A (rozbalenou z RAR5, B je v archivu prvni)");
    over(ctl("archiv") == "hra5.rar" && ctl("archiv_vybrany") == "Hra (Side A).atr", "devCtlNative \"archiv\" = hra5.rar, \"archiv_vybrany\" = Hra (Side A).atr");
    const std::string s = ctl("archiv_seznam");
    printf("archiv_seznam:\n%s", s.c_str());
    over(s.find("0\tA\t") == 0 && s.find("\t1\tHra (Side A).atr\n") != std::string::npos && s.find("\t0\tHra (Side B).atr\n") != std::string::npos && s.find("navod") == std::string::npos,
         "archiv_seznam pro ATR / DISK: 0 = strana A (aktualni), 1 = strana B, navod.txt neni");
    const std::string rr = r ? nahraj(r->b, ctl("archiv_vybrany")) : std::string("CHYBA");
    printf("devLoadAtr: %s\n", rr.c_str());
    over(rr.rfind("OK ", 0) == 0, "strana A z RAR5 prijata do D1: (studeny start)");
    for (int i = 0; i < 600 && peekS(0x6FE) != 0xA5; i++) spi(10);
    over(peekS(0x6FE) == 0xA5, "program ze strany A nabootoval a ceka na druhou stranu");
  }

  // 3) ATR / DISK -> VYMENIT DISKETU BEZ RESTARTU -> strana B z RAR5
  {
    FakeArr *b = archivSoubor(1);
    over(b && b->b == stranaB && ctl("archiv_vybrany") == "Hra (Side B).atr", "devArchivSouborNative(1): strana B z RAR5");
    ctl("atr_vymena");
    const std::string r = b ? nahraj(b->b, "Hra (Side B).atr") : std::string("CHYBA");
    printf("devLoadAtr (vymena): %s\n", r.c_str());
    over(r.find("VYMENENA bez restartu") != std::string::npos && peekS(0x6FE) == 0xA5 && ctl("d1") == "Hra (Side B).atr",
         "VYMENIT DISKETU BEZ RESTARTU: program bezi dal, v D1: je strana B z RAR5");
    const uint8_t tab[][8] = {{0x31, 'R', 4, 0, 128, 2, 0x30, 0}, {0, 0, 0, 0, 0, 0, 0, 0}};
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      for (int k = 0; k < 2; k++) for (int i = 0; i < 8; i++) g_stroj->mem.ram[B348_TAB + k * 8 + i] = tab[k][i];
      g_stroj->mem.ram[0x6FF] = 1;
    }
    for (int i = 0; i < 1000 && peekS(0x6F0) != 0x5A; i++) spi(10);
    std::lock_guard<std::mutex> l(g_mStroj);
    bool data = true;
    for (int i = 0; i < 128; i++) if (g_stroj->mem.ram[0x3000 + i] != obsah(4, i)) data = false;
    const uint8_t *res = &g_stroj->mem.ram[0x600];
    printf("SIO pres POKEY: %c %c soucet %d\n", res[0], res[2], res[3]);
    over(g_stroj->mem.ram[0x6F0] == 0x5A && res[0] == 'A' && res[2] == 'C' && res[3] == 1 && data, "program precetl sektor 4 strany B vlastnim SIO (data z RAR5 sedi)");
  }

  // 4) Prince of Persia v RAR5 + log
  {
    const std::vector<uint8_t> pop = nacti(dir + "/vyber_pop5.rar");
    FakeArr *r = pop.empty() ? nullptr : otevriArchiv(pop, "vyber_pop5.rar", false);
    const std::string s = ctl("archiv_seznam");
    printf("vyber_pop5.rar -> %s (%zu B)\narchiv_seznam:\n%s", ctl("archiv_vybrany").c_str(), r ? r->b.size() : 0, s.c_str());
    over(r && r->b.size() == 133136 && ctl("archiv_vybrany") == "PoP_DD_final.atr" && s.find("1\tA\t") != std::string::npos && s.find("2\tA\t") != std::string::npos,
         "Prince of Persia v RAR5: spusti PoP_DD_final.atr, v nabidce i SDA a SDB");
    spi(100);
    sberLog();
    for (const char *radek : {"B349 ARCHIV hra5.rar (", "B349 ARCHIV vyber_pop5.rar (", "B350 RAR5"}) {
      const size_t p = g_log.find(radek);
      if (p != std::string::npos) printf("LOG: %s\n", g_log.substr(p, g_log.find('\n', p) - p).c_str());
    }
    over(g_log.find("B349 ARCHIV hra5.rar (RAR5, rozbaleno za ") != std::string::npos && g_log.find("-> SPOUSTIM Hra (Side A).atr") != std::string::npos &&
             g_log.find("B349 ARCHIV vyber_pop5.rar (RAR5, rozbaleno za ") != std::string::npos,
         "log: B349 ARCHIV ... (RAR5, rozbaleno za N ms) ... -> SPOUSTIM ...");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
