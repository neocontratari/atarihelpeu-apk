// test_overeni/b354/test_b354_atari.cpp
// B354: DVA HERNI OVLADACE = DVA HRACI NA ATARI 130XE - SKUTECNY nap_atari_native.cpp (pristroj, vlakna, stuby
// z ../b291/stub; infrastruktura okopirovana z ../b320/test_b320_atari_ovladac.cpp). Hrac 2 = druhy ovladac
// (Java NapOvladac.Hraci prideli podle prvniho stisku) -> devOvladacKlavesaHracNative / devOvladacOsyHracNative(1, ...)
// -> joystick 2: PORTA horni 4 bity + TRIG1; OS je ve VBLANK opisuje do STICK1 ($279) a STRIG1 ($285).
//  1) hrac 2: A = FIRE joysticku 2 (TRIG1, STRIG1), krizek = smer joysticku 2 (PORTA horni bity, STICK1);
//     joystick 1 se nehne
//  2) oba hraci naraz: hrac 1 vpravo, hrac 2 vlevo -> STICK0 7, STICK1 B; pusteni jednoho nepusti druheho
//  3) hrac 2: packa = smer, START = konzolove START (5 snimku i kratky stisk); klavesy X / Y jen hrac 1
//  4) ovladac odpojen (devOvladacPustNative) -> oba joysticky pusteny; log B354 JOYSTICK 2
// Preklad (z test_overeni/b354, JAVA_HOME = JDK):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b354_atari test_b354_atari.cpp
//   ./test_b354_atari                                               -> VYSTUP_test_b354_atari.txt
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

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.SetIntArrayRegion = fSetIntArrayRegion; g_tbl.SetShortArrayRegion = fSetShortArrayRegion;
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B354 - dva herni ovladace = dva hraci na Atari 130XE (C++ pristroj z appky, JNI jako Java)\n");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");
  Joy j = joy();
  printf("klid: %s\n", popis(j).c_str());
  over(j.porta == 0xFF && j.t0 == 1 && j.t1 == 1 && j.stick1 == 0xF && j.strig1 == 1, "klid: oba joysticky pusteny (PORTA $FF, TRIG0 / TRIG1 = 1, STICK1 $F)");

  // 1) hrac 2
  {
    ovl2(96, true); spi(200); const Joy f = joy();
    ovl2(96, false); ovl2(19, true); spi(200); const Joy n = joy();
    ovl2(19, false); ovl2(270, true); spi(200); const Joy s = joy();   // DPAD_UP_RIGHT
    ovl2(270, false); spi(200); const Joy k = joy();
    printf("hrac 2 A: %s\nhrac 2 nahoru: %s\nhrac 2 nahoru-vpravo: %s\npusteno: %s\n", popis(f).c_str(), popis(n).c_str(), popis(s).c_str(), popis(k).c_str());
    over(f.t1 == 0 && f.strig1 == 0 && f.t0 == 1 && f.porta == 0xFF, "1a) hrac 2: A = FIRE joysticku 2 (TRIG1 0, OS STRIG1 0), joystick 1 nic");
    over(n.porta == 0xEF && n.stick1 == 0xE && n.stick0 == 0xF && s.porta == 0x6F && s.stick1 == 0x6,
         "1b) hrac 2: krizek nahoru = PORTA $EF (STICK1 $E), uhlopricka nahoru-vpravo $6F (STICK1 $6); STICK0 porad $F");
    over(k.porta == 0xFF && k.t1 == 1 && k.stick1 == 0xF, "1c) hrac 2 pusteno = klid");
  }
  // 2) oba hraci naraz
  {
    ovl(22, true); ovl2(21, true); ovl(96, true); spi(200); const Joy o = joy();
    ovl(22, false); ovl(96, false); spi(200); const Joy d = joy();
    ovl2(21, false); spi(200); const Joy k = joy();
    printf("hrac 1 vpravo + FIRE, hrac 2 vlevo: %s\nhrac 1 pustil: %s\n", popis(o).c_str(), popis(d).c_str());
    over(o.porta == 0xB7 && o.stick0 == 0x7 && o.stick1 == 0xB && o.t0 == 0 && o.t1 == 1,
         "2a) oba naraz: hrac 1 vpravo + FIRE, hrac 2 vlevo -> PORTA $B7 (STICK0 $7, STICK1 $B), TRIG0 0, TRIG1 1");
    over(d.porta == 0xBF && d.t0 == 1 && k.porta == 0xFF, "2b) hrac 1 pustil -> hrac 2 porad vlevo ($BF); pak i hrac 2 pustil");
  }
  // 3) packa, START, klavesy
  {
    osy2(0.9f, 0, 0, 0, 15); spi(200); const Joy p = joy();
    osy2(0.1f, 0, 0, 0, 15); spi(200); const Joy pk = joy();
    ovl2(108, true); spi(30); ovl2(108, false); spi(30); const Joy st = joy();   // kratky START
    int conDrz = -1;
    { std::lock_guard<std::mutex> l(g_mStroj); conDrz = g_consolDrzetDo[0] > g_snimkuCelkem.load() ? 1 : 0; }
    ovl2(108, true); spi(150); const Joy st2 = joy();
    ovl2(108, false); spi(250); const Joy st3 = joy();
    const int kbPred = joy().kb;
    const int x = ovl2(99, true); spi(150); const Joy kx = joy(); ovl2(99, false);
    printf("hrac 2 packa vpravo: %s | START drzeny: consol %d, pusteny %d | X: vraci %d, KBCODE $%02X (pred $%02X)\n",
           popis(p).c_str(), st2.consol, st3.consol, x, kx.kb, kbPred);
    over(p.porta == 0x7F && p.stick1 == 0x7 && pk.porta == 0xFF, "3a) hrac 2: leva packa 0,9 = vpravo (PORTA $7F, STICK1 $7), 0,1 = klid");
    over((st2.consol & 1) == 0 && (st3.consol & 1) == 1 && conDrz == 1, "3b) hrac 2: START = konzolove START (i kratky stisk drzi aspon 5 snimku), pusteny = nic");
    over(kx.kb == kbPred, "3c) hrac 2: klavesy (X = MEZERA, Y = RETURN) jen hrac 1 - klavesnice je jedna");
    (void)st;
  }
  // 4) odpojeni
  {
    ovl2(96, true); ovl2(20, true); ovl(96, true); spi(200); const Joy d = joy();
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacPustNative(&g_env, nullptr);
    spi(200); const Joy k = joy();
    printf("oba drzi: %s\npo devOvladacPust: %s\n", popis(d).c_str(), popis(k).c_str());
    over(d.porta == 0xDF && d.t0 == 0 && d.t1 == 0 && k.porta == 0xFF && k.t0 == 1 && k.t1 == 1,
         "4a) ovladac odpojen uprostred stisku obou hracu (PORTA $DF, oba FIRE) -> devOvladacPust: oba joysticky pusteny");
    spi(200);
    jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
    const std::string log = s ? ((FakeStr *)s)->s : "";
    const size_t p = log.find("B354 JOYSTICK 2");
    if (p != std::string::npos) printf("LOG: %s\n", log.substr(p, log.find('\n', p) - p).c_str());
    over(p != std::string::npos, "4b) log: B354 JOYSTICK 2 z druheho herniho ovladace");
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
