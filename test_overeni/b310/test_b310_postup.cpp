// B310: POSTUP HRACE v C++ - SKUTECNY nap_atari_native.cpp (pristroj Atari 130XE,
// vlakna emulace a kresleni) na PC, stejne jako ../b292/test_b292_jni_host.cpp.
//
// Sega se odemyka po 50 napsanych radcich programu v Atari. Pocitalo to jen stare
// Atari v JavaScriptu (emu_vbxe, smazane v B310); ted to pocita C++:
//   - radek zacinajici cislem (radek programu, ne prikaz) = +1 do davky,
//   - RUN / LIST (i zkratka L.) = davka hotova -> devCtl("postup") ji vyda Jave,
//   - Java (NapPostup) uzna davku od 5 radku - to overuje test_b310_napostup (JVM).
// Test pise prstem na klavesnici pristroje (jako hrac v telefonu), z klavesnice
// pocitace (TV/PC), vklada text z PC (Ctrl+V) a zkousi BREAK, BACK S, novy vstup.
//
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b310_postup test_b310_postup.cpp
//   ./test_b310_postup                       -> VYSTUP_test_b310_postup.txt
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
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
static std::string g_celyLog;
static void sberLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) g_celyLog += ((FakeStr *)s)->s;
}
static bool cekejNa(const char *text, int ms) {
  for (int i = 0; i < ms / 100; i++) { if (obrazovka().find(text) != std::string::npos) return true; spi(100); }
  return obrazovka().find(text) != std::string::npos;
}
// prstem na klavesnici pristroje (jako hrac v telefonu)
static int klep(float ux, float uy, int pid = 0, int drzetMs = 70) {
  float x, y;
  { std::lock_guard<std::mutex> dl(g_mDev); x = devGet().X(ux); y = devGet().Y(uy); }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 0, x, y);
  spi(drzetMs);
  int r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 1, x, y);
  spi(90);
  return r;
}
static void klavesa(const std::string &l2) {
  for (int i = 0; i < 57; i++) if (l2 == nap::dev::KEYS[i].l2) { const auto &k = nap::dev::KEYS[i]; klep(k.x + k.w / 2, k.y + k.h / 2); return; }
  printf("klavesa '%s' na pristroji neni\n", l2.c_str());
}
// text prstem: znak po znaku, '\n' = RETURN, '\b' = BACK S, '#' = BREAK
static void prstem(const std::string &t) {
  for (char c : t) {
    if (c == '\n') klavesa("Return");
    else if (c == '\b') klavesa("BkSp");
    else if (c == '#') klavesa("Break");
    else if (c == ' ') klavesa("");
    else klavesa(std::string(1, c));
  }
  spi(250);
}
// klavesnice pocitace (prohlizec na TV/PC -> Java napTvWebKlavesa -> devPcKlavesaNative)
static void pcKlavesa(const std::string &code, const std::string &znak, bool dolu) {
  FakeStr c{code}, z{znak};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPcKlavesaNative(&g_env, nullptr, (jstring)&c, (jstring)&z, 0, dolu ? JNI_TRUE : JNI_FALSE);
}
static void zPc(const std::string &t) {
  for (char ch : t) {
    std::string code, znak(1, ch);
    if (ch == '\n') { code = "Enter"; znak = "Enter"; }
    else if (ch == ' ') code = "Space";
    else if (ch == '=') code = "Equal";
    else if (ch == '.') code = "Period";
    else if (ch >= '0' && ch <= '9') code = std::string("Digit") + ch;
    else code = std::string("Key") + (char)std::toupper((unsigned char)ch);
    pcKlavesa(code, znak, true);
    spi(15);
    pcKlavesa(code, znak, false);
    spi(15);
  }
  // pockat, az C++ vsechno napise v tempu pro OS
  for (int i = 0; i < 200; i++) {
    bool prazdne;
    { std::lock_guard<std::mutex> l(g_mStroj); std::lock_guard<std::mutex> p(g_mPc); prazdne = g_pcHrac.prazdny() && g_pcVstup.empty() && g_pcText.empty(); }
    if (prazdne) break;
    spi(50);
  }
  spi(400);
}
static std::string vlozPc(const std::string &t) {
  FakeArr a; a.b.assign(t.begin(), t.end());
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devVlozNative(&g_env, nullptr, (jbyteArray)&a);
  return ((FakeStr *)r)->s;
}
// to, co si kazdych 400 ms vyzvedne Java (atariCppPostupHrace -> NapPostup)
static std::string postup() {
  FakeStr c{"postup"};
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&c);
  return ((FakeStr *)r)->s;
}

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};

  printf("B310 - postup hrace (odemceni Segy) v C++: skutecny nap_atari_native.cpp, pristroj Atari 130XE\n\n");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  over(cekejNa("READY", 16000), "Atari nastartovalo do READY");
  over(postup().empty(), "na zacatku zadna davka");

  // 1) prstem 5 radku programu + RUN
  printf("\n1) prstem: 10 A=1 .. 50 A=5, RUN\n");
  prstem("10 A=1\n20 A=2\n30 A=3\n40 A=4\n50 A=5\n");
  over(postup().empty(), "radky programu bez RUN / LIST: davka jeste neni hotova");
  prstem("RUN\n");
  std::string p = postup();
  printf("   postup = \"%s\"\n", p.c_str());
  over(p == "5", "RUN -> davka 5 radku pro Javu");
  over(postup().empty(), "Java davku vyzvedla - podruhe uz nic");
  over(cekejNa("50 A=5", 2000) && obrazovka().find("ERROR") == std::string::npos, "BASIC radky opravdu prijal (bez ERROR)");

  // 2) cislo bez prikazu (smazani radku) a prikaz bez cisla se nepocitaji
  printf("\n2) prstem: 60 (jen cislo), A=7 (prikaz), RUN\n");
  prstem("60\nA=7\nRUN\n");
  p = postup();
  printf("   postup = \"%s\"\n", p.c_str());
  over(p.empty(), "jen cislo ani prikaz bez cisla nejsou radky programu - zadna davka");

  // 3) preklep opraveny BACK S, konec davky zkratkou L.
  printf("\n3) prstem: 70 B=3 <BACK S> 2, 71..74, L.\n");
  prstem("70 B=3\b2\n71 B=1\n72 B=4\n73 B=5\n74 B=6\nL.\n");
  p = postup();
  printf("   postup = \"%s\"\n", p.c_str());
  over(p == "5", "L. (zkratka LIST) -> davka 5 radku");
  spi(1500);
  over(obrazovka().find("70 B=2") != std::string::npos, "LIST ukazuje opraveny radek 70 B=2 (BACK S doslo do BASICu)");

  // 4) BREAK zrusi rozepsany radek
  printf("\n4) prstem: 80 C=1 <BREAK>, RUN\n");
  prstem("80 C=1#");
  prstem("RUN\n");
  p = postup();
  printf("   postup = \"%s\"\n", p.c_str());
  over(p.empty(), "radek zruseny BREAK se nepocita");

  // 5) klavesnice pocitace (TV/PC)
  printf("\n5) z klavesnice pocitace: 90 D=1 .. 94 D=5, RUN\n");
  zPc("90 D=1\n91 D=2\n92 D=3\n93 D=4\n94 D=5\nRUN\n");
  p = postup();
  printf("   postup = \"%s\"\n", p.c_str());
  over(p == "5", "klavesnice pocitace (TV/PC) se pocita stejne -> davka 5");

  // 6) vlozeny text z PC (Ctrl+V)
  printf("\n6) prstem 100..104, Ctrl+V \"RUN\" z PC, prstem LIST\n");
  prstem("100 E=1\n101 E=2\n102 E=3\n103 E=4\n104 E=5\n");
  std::string v = vlozPc("RUN\n");
  printf("   vloz -> %s\n", v.c_str());
  zPc("");                                              // pockat, az se vlozene napise
  p = postup();
  printf("   postup po vlozenem RUN = \"%s\"\n", p.c_str());
  over(p.empty(), "vlozeny RUN (Ctrl+V) davku neodesle");
  prstem("LIST\n");
  p = postup();
  printf("   postup po LIST prstem = \"%s\"\n", p.c_str());
  over(p == "5", "napsane radky zustaly - LIST prstem -> davka 5");
  v = vlozPc("200 F=1\n201 F=2\n202 F=3\n203 F=4\n204 F=5\n");
  printf("   vloz program -> %s\n", v.c_str());
  for (int i = 0; i < 100; i++) { bool a; { std::lock_guard<std::mutex> l(g_mStroj); a = g_psani.aktivni(); } if (!a) break; spi(100); }
  spi(500);
  prstem("RUN\n");
  p = postup();
  printf("   postup po vlozenem programu a RUN = \"%s\"\n", p.c_str());
  over(p.empty(), "program vlozeny z PC (pise ho appka sama) se nepocita");

  // 7) hotova davka prezije odchod z Atari, rozepsana ne
  printf("\n7) davka hotova pred odchodem z Atari, rozepsane radky pri odchodu\n");
  prstem("300 G=1\n301 G=2\n302 G=3\n303 G=4\n304 G=5\nRUN\n");
  prstem("400 H=1\n401 H=2\n");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  p = postup();
  printf("   postup po odchodu = \"%s\"\n", p.c_str());
  over(p == "5", "hotova davka zustala pro Javu i po odchodu z Atari (Java ji vyzvedne pri vypnuti pristroje)");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  over(cekejNa("READY", 16000), "novy vstup do Atari - READY");
  prstem("RUN\n");
  p = postup();
  printf("   postup po novem vstupu a RUN = \"%s\"\n", p.c_str());
  over(p.empty(), "rozepsane radky z minula se po novem vstupu nepocitaji (jako nova stranka stareho Atari)");

  // 8) log
  sberLog();
  over(g_celyLog.find("B310 POSTUP HRACE: davka 5 radku programu (RUN)") != std::string::npos, "log: B310 POSTUP HRACE ... (RUN)");
  over(g_celyLog.find("B310 POSTUP HRACE: davka 5 radku programu (L.)") != std::string::npos, "log: B310 POSTUP HRACE ... (L.)");
  over(g_celyLog.find("B299 KLAVESY: 10 A=1 <RETURN>") != std::string::npos, "log B299 KLAVESY beze zmeny (co hrac napsal)");
  over(g_celyLog.find("B297 OVLADANI postup") == std::string::npos, "devCtl \"postup\" nezahlcuje log");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);

  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
