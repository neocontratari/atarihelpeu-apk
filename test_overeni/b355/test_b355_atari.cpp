// test_overeni/b355/test_b355_atari.cpp
// B355: TLACITKA OVLADACE V ATARI 130XE - SKUTECNY nap_atari_native.cpp (pristroj, vlakna, stuby z ../b291/stub;
// infrastruktura z ../b354/test_b354_atari.cpp). Java (NapOvladacMapa) posle mapu devOvladacMapaNative a
// katalog si vezme z devOvladacKatalogNative; klavesy ovladace jdou jako drive pres devOvladacKlavesaHracNative.
//  1) vychozi mapa (nic neposlano): Y = RETURN drzena, X = MEZERA, HOME uz neni OPTION (log Reneho: PoP menu)
//  2) vlastni mapa: A = klavesa P, L1 = HELP, X = joystick nahoru, HOME = OPTION, B = BREAK (OS: BRKKEY = 0);
//     drzi-li hrac dve klavesy, plati naposled stisknuta; po pusteni zase ta drzena
//  3) hrac 2 se stejnou mapou: X = joystick 2 nahoru; klavesy (A = P) jen hrac 1
//  4) turbo (B353): "pust vse" bez zmeny (odchod ze hry, fronta pri startu) se do vstupu hrace NEPOCITA
//  5) katalog pro Javu (vsechna jadra) a navrat na vychozi mapu ("")
// Preklad (z test_overeni/b355, JAVA_HOME = JDK):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b355_atari test_b355_atari.cpp
//   ./test_b355_atari                                               -> VYSTUP_test_b355_atari.txt
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include "../b292/syntaz_kazeta.h"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <set>
#include <algorithm>

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
struct FakeArr { std::vector<uint8_t> b; std::vector<float> f; int druh = 0; };   // B320: druh 1 = float[] (osy ovladace)
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { FakeArr *x = (FakeArr *)a; return (jsize)(x->druh == 1 ? x->f.size() : x->b.size()); }
static void JNICALL fGetFloatArrayRegion(JNIEnv *, jfloatArray a, jsize st, jsize n, jfloat *buf) { std::memcpy(buf, ((FakeArr *)a)->f.data() + st, (size_t)n * sizeof(float)); }
static void JNICALL fGetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, jbyte *buf) { std::memcpy(buf, ((FakeArr *)a)->b.data() + st, n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.resize(n); return (jbyteArray)a; }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, n); }
// B301: pole int[] / short[] (delka = b.size(), data ve v) pro obraz a zvuk pro TV
struct FakeIntArr { std::vector<uint8_t> b; std::vector<int32_t> v; };
struct FakeShortArr { std::vector<uint8_t> b; std::vector<int16_t> v; };
static void JNICALL fSetIntArrayRegion(JNIEnv *, jintArray a, jsize st, jsize n, const jint *buf) { std::memcpy(((FakeIntArr *)a)->v.data() + st, buf, n * 4); }
static void JNICALL fSetShortArrayRegion(JNIEnv *, jshortArray a, jsize st, jsize n, const jshort *buf) { std::memcpy(((FakeShortArr *)a)->v.data() + st, buf, n * 2); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const char *co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co); fflush(stdout); }
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
static void vypisLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) { printf("%s", ((FakeStr *)s)->s.c_str()); }
}
static void prst(float ux, float uy, int pid, int akce) {
  float x, y;
  { std::lock_guard<std::mutex> dl(g_mDev); x = devGet().X(ux); y = devGet().Y(uy); }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, akce, x, y);
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
static const nap::dev::KeyDef *klavesaDef(const char *l2) {
  for (int i = 0; i < 57; i++) if (std::string(nap::dev::KEYS[i].l2) == l2) return &nap::dev::KEYS[i];
  printf("klavesa %s nenalezena\n", l2); return nullptr;
}
static void klavesa(const char *l2) { const auto *k = klavesaDef(l2); if (k) klep(k->x + k->w / 2, k->y + k->h / 2); }
static void napis(const char *t) {
  FakeStr s{t};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&s, JNI_FALSE);
}
static int pocet(const std::string &s, char c) { int n = 0; for (char x : s) if (x == c) n++; return n; }
// B303: Ctrl+V / pretazeny TXT z prohlizece -> devVlozNative (jako POST /vloz z Javy)
static std::string vlozPc(const std::string &t) {
  FakeArr a; a.b.assign(t.begin(), t.end());
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devVlozNative(&g_env, nullptr, (jbyteArray)&a);
  const std::string s = ((FakeStr *)r)->s;
  printf("vloz %zu B -> %s\n", t.size(), s.c_str());
  return s;
}


// ================== B320 ==================
static std::string ctl(const char *c) { FakeStr s{c}; return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&s))->s; }
static int ovl(int kod, bool dolu) { return Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKlavesaNative(&g_env, nullptr, kod, dolu ? JNI_TRUE : JNI_FALSE); }
static void osy(float lx, float ly, float hx, float hy, float l2, float r2, int maska) {
  FakeArr a; a.druh = 1; a.f = {lx, ly, 0, 0, hx, hy, l2, r2};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacOsyNative(&g_env, nullptr, (jfloatArray)&a, maska);
}
static void stav(int &pa, int &tr, int &co, int &kb, int &sk) { std::lock_guard<std::mutex> l(g_mStroj); pa = g_stroj->porta & 15; tr = g_stroj->trig[0]; co = g_stroj->consol; kb = g_stroj->kbcode; sk = g_stroj->skstat; }
// podil bodu oblasti (x0..x1, y0..y1) displeje, ktere se lisi mezi dvema obrazky displeje
static double rozdilOblasti(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b, int W, int x0, int y0, int x1, int y1) {
  long long n = 0, r = 0;
  for (int y = std::max(0, y0); y < y1; y++)
    for (int x = std::max(0, x0); x < x1 && x < W; x++) { n++; if (a[(size_t)y * W + x] != b[(size_t)y * W + x]) r++; }
  return n ? 100.0 * r / n : 0;
}
static std::vector<uint32_t> displej() { spi(400); return g_fakeWin.buf; }   // okno B292: buffer = displej (kresli se jen zmenene obdelniky)


static int ovl2(int kod, bool dolu) { return Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKlavesaHracNative(&g_env, nullptr, 1, kod, dolu ? JNI_TRUE : JNI_FALSE); }
static void osy2(float lx, float ly, float hx, float hy, int maska) {
  FakeArr a; a.druh = 1; a.f = {lx, ly, 0, 0, hx, hy, 0, 0};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacOsyHracNative(&g_env, nullptr, 1, (jfloatArray)&a, maska);
}
struct Joy { int porta, t0, t1, consol, kb, stick0, stick1, strig0, strig1; };
static Joy joy() {
  std::lock_guard<std::mutex> l(g_mStroj);
  Machine &m = *g_stroj;
  return {m.porta, m.trig[0], m.trig[1], m.consol, m.kbcode, m.mem.ram[0x278], m.mem.ram[0x279], m.mem.ram[0x284], m.mem.ram[0x285]};
}
static std::string popis(const Joy &j) {
  char b[160];
  snprintf(b, sizeof(b), "PORTA $%02X TRIG0 %d TRIG1 %d | OS: STICK0 $%X STICK1 $%X STRIG0 %d STRIG1 %d", j.porta, j.t0, j.t1, j.stick0, j.stick1, j.strig0, j.strig1);
  return b;
}


static std::string logy;                 // vse z devPollLogNative (B355 radky hledame v nem)
static void sbirejLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) logy += ((FakeStr *)s)->s;
}
static std::string mapa(const char *m) {
  FakeStr s{m};
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacMapaNative(&g_env, nullptr, (jstring)&s);
  return r ? ((FakeStr *)r)->s : std::string("(null)");
}
static std::string katalogJ(int j, bool en) {
  jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacKatalogNative(&g_env, nullptr, j, en ? JNI_TRUE : JNI_FALSE);
  return r ? ((FakeStr *)r)->s : std::string();
}
static int radkuSPrefixem(const std::string &s, const char *p) {
  int n = 0; size_t i = 0;
  while (i < s.size()) { const size_t e = s.find('\n', i); const std::string r = s.substr(i, e == std::string::npos ? std::string::npos : e - i);
    if (r.rfind(p, 0) == 0) n++; if (e == std::string::npos) break; i = e + 1; }
  return n;
}
static uint8_t ram(int a) { std::lock_guard<std::mutex> l(g_mStroj); return g_stroj->mem.ram[a & 0xFFFF]; }
static void pokeRam(int a, uint8_t v) { std::lock_guard<std::mutex> l(g_mStroj); g_stroj->mem.ram[a & 0xFFFF] = v; }

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.SetIntArrayRegion = fSetIntArrayRegion; g_tbl.SetShortArrayRegion = fSetShortArrayRegion;
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B355 - tlacitka herniho ovladace v Atari 130XE (C++ pristroj z appky, JNI jako Java)\n");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");
  sbirejLog();

  // 1) vychozi mapa
  {
    ovl(100, true); spi(200); const Joy y = joy(); int pa, tr, co, kb, sk; stav(pa, tr, co, kb, sk);
    ovl(100, false); spi(200); int pa2, tr2, co2, kb2, sk2; stav(pa2, tr2, co2, kb2, sk2);
    ovl(110, true); spi(200); const Joy h = joy();
    ovl(110, false); spi(200);
    ovl(99, true); spi(200); int pa3, tr3, co3, kb3, sk3; stav(pa3, tr3, co3, kb3, sk3);
    ovl(99, false); spi(200);
    printf("vychozi: Y -> KBCODE %d SKSTAT bit2 %d, pusteno bit2 %d | HOME -> CONSOL %d | X -> KBCODE %d\n", kb, (sk >> 2) & 1, (sk2 >> 2) & 1, h.consol, kb3);
    over(kb == 12 && !(sk & 4) && (sk2 & 4) && y.porta == 0xFF && kb3 == 33 && !(sk3 & 4),
         "1a) vychozi mapa: Y = klavesa RETURN drzena (KBCODE 12), pustena; X = MEZERA (KBCODE 33) - jako do B354");
    over(h.consol == 7, "1b) vychozi mapa: HOME (MODE) uz neni OPTION - CONSOL 7 (do B354 OPTION: v Prince of Persia otevrel menu hry)");
  }
  // 2) vlastni mapa
  {
    const std::string n = mapa("A=K10;B=BREAK;X=NAHORU;MODE=OPTION;L1=K17;Q=FIRE");
    spi(100); sbirejLog();
    printf("mapa -> jadro: %s\n", n.c_str());
    over(n == "A=K10;B=BREAK;X=NAHORU;Y=K12;L1=K17;R1=FIRE;L2=OPTION;R2=OPTION;L3=0;R3=0;START=START;SELECT=SELECT;MODE=OPTION" &&
             logy.find("B355 OVLADAC TLACITKA Atari: A(spodni)=klavesa P, B(prave)=BREAK, X(leve)=joystick nahoru") != std::string::npos &&
             logy.find("HOME=OPTION | neznamych dvojic 1") != std::string::npos,
         "2a) devOvladacMapaNative: jadro vrati mapu (neznamy radek Q zustal vychozi) a zapise ji do logu citelne");
    ovl(96, true); spi(200); int pa, tr, co, kb, sk; stav(pa, tr, co, kb, sk);
    ovl(102, true); spi(200); int pa2, tr2, co2, kb2, sk2; stav(pa2, tr2, co2, kb2, sk2);
    ovl(102, false); spi(200); int pa3, tr3, co3, kb3, sk3; stav(pa3, tr3, co3, kb3, sk3);
    ovl(96, false); spi(200); int pa4, tr4, co4, kb4, sk4; stav(pa4, tr4, co4, kb4, sk4);
    printf("A -> KBCODE %d | A+L1 -> %d | L1 pusteno -> %d (drzena %d) | vse pusteno: SKSTAT bit2 %d\n", kb, kb2, kb3, !(sk3 & 4), (sk4 >> 2) & 1);
    over(kb == 10 && !(sk & 4) && kb2 == 17 && !(sk2 & 4) && kb3 == 10 && !(sk3 & 4) && (sk4 & 4) && tr == 1,
         "2b) A = klavesa P (KBCODE 10), k tomu L1 = HELP (17 - plati posledni), L1 pusteno -> zase P, vse pusteno; FIRE nic");
    ovl(99, true); spi(200); const Joy x = joy();
    ovl(99, false); ovl(110, true); spi(200); const Joy o = joy();
    ovl(110, false); spi(300); const Joy k = joy();
    printf("X -> %s | HOME -> CONSOL %d | pusteno CONSOL %d\n", popis(x).c_str(), o.consol, k.consol);
    over(x.porta == 0xFE && x.stick0 == 0xE && o.consol == 3 && k.consol == 7 && k.porta == 0xFF,
         "2c) X = joystick 1 nahoru (PORTA $FE, STICK0 $E), HOME = OPTION (CONSOL 3), pusteno klid");
    // BREAK: preruseni POKEY -> obsluha OS (vektor BRKKY) nuluje BRKKEY, SSFLAG ($2FF), CRSINH ($2F0) a ATRACT.
    // BRKKEY vrati editor (E:) hned zpet na $80 - proto SSFLAG / CRSINH, ty nikdo jiny nemeni
    const int irqen = ram(0x10);                              // POKMSK - OS ma BREAK povoleny
    pokeRam(0x2FF, 0xFF); pokeRam(0x2F0, 0x01);
    spi(100); const int ss0 = ram(0x2FF), cr0 = ram(0x2F0);
    ovl(97, true); spi(200); const int ss = ram(0x2FF), cr = ram(0x2F0);
    ovl(97, false); spi(200);
    pokeRam(0x2F0, 0x00);
    printf("B (BREAK): POKMSK $%02X | pred: SSFLAG $%02X CRSINH $%02X | po: SSFLAG $%02X CRSINH $%02X\n", irqen, ss0, cr0, ss, cr);
    over((irqen & 0x80) && ss0 == 0xFF && cr0 == 1 && ss == 0 && cr == 0,
         "2d) B = BREAK: preruseni POKEY -> obsluha BREAK v OS (vynulovala SSFLAG a CRSINH), jako klavesa BREAK");
  }
  // 3) hrac 2 se stejnou mapou
  {
    int pa0, tr0, co0, kb0, sk0; stav(pa0, tr0, co0, kb0, sk0);
    ovl2(99, true); spi(200); const Joy x = joy();
    ovl2(99, false); ovl2(96, true); spi(200); int pa, tr, co, kb, sk; stav(pa, tr, co, kb, sk);
    ovl2(96, false); spi(200); const Joy k = joy();
    printf("hrac 2 X -> %s | hrac 2 A -> KBCODE %d SKSTAT bit2 %d (pred %d)\n", popis(x).c_str(), kb, (sk >> 2) & 1, kb0);
    over(x.porta == 0xEF && x.stick1 == 0xE && x.stick0 == 0xF && (sk & 4) && kb == kb0 && k.porta == 0xFF,
         "3) hrac 2: X = joystick 2 nahoru (PORTA $EF, STICK1 $E); A = klavesa P jen hrac 1 (klavesnice je jedna)");
  }
  // 4) turbo: pusteni bez zmeny se do vstupu hrace nepocita
  {
    spi(300);
    g_posledniVstupMs.store(0);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacPustNative(&g_env, nullptr);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0);   // jako Java pri vypnuti pristroje
    { std::lock_guard<std::mutex> l(g_mEv); nap::dev::Ev e; e.t = nap::dev::Ev::PAD; e.v = 0; g_evQ.push_back(e); e.v = PAD_HRAC2; g_evQ.push_back(e); }
    spi(300);
    const long long po = g_posledniVstupMs.load();
    ovl(96, true); spi(200); const long long stisk = g_posledniVstupMs.load();
    g_posledniVstupMs.store(0);
    ovl(96, false); spi(200); const long long pust = g_posledniVstupMs.load();
    printf("posledni vstup: po 'pust vse' %lld, po stisku %s, po pusteni %s\n", po, stisk > 0 ? "ted" : "0", pust > 0 ? "ted" : "0");
    over(po == 0 && stisk > 0 && pust > 0,
         "4) turbo B353: \"pust vse\" bez zmeny (odchod ze hry, fronta na zacatku dalsi hry) neni vstup hrace; skutecny stisk i pusteni ano "
         "(log Reneho: turbo u Tonyho a PoP az po ~3 s)");
  }
  // 5) katalog a navrat na vychozi
  {
    bool vse = true;
    std::string pop;
    for (int j = 0; j < 3; j++) for (int en = 0; en < 2; en++) {
      const std::string k = katalogJ(j, en != 0);
      const int r = radkuSPrefixem(k, "R\t"), c = radkuSPrefixem(k, "C\t"), d = radkuSPrefixem(k, "D\t");
      vse = vse && r == 13 && c >= 14 && d == 1 && k == nap::ovladac::katalog(j, en != 0);
      pop += " " + std::to_string(j) + (en ? "en" : "cs") + "=" + std::to_string(r) + "/" + std::to_string(c);
    }
    printf("katalog radku/cilu:%s\n", pop.c_str());
    over(vse, "5a) devOvladacKatalogNative: Atari / Sega / PS1, CZ / EN - 13 radku, cile a vychozi mapa (Java nic neopisuje)");
    logy.clear();
    const std::string v = mapa("");
    spi(100); sbirejLog();
    ovl(110, true); spi(200); const Joy h = joy(); ovl(110, false); spi(200);
    over(v == nap::ovladac::mapaNaRetezec(nap::ovladac::vychoziMapa(nap::ovladac::J_ATARI)) && v.find("MODE=MENU") != std::string::npos &&
             logy.find("HOME=MENU (tlacitka ovladace)") != std::string::npos && h.consol == 7,
         "5b) mapa \"\" = zase vychozi (HOME = MENU - dialog otevre Java, jadro nic), v logu");
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
