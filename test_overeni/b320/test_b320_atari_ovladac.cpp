// B320: HERNI OVLADAC NA ATARI 130XE (C++ pristroj z appky, nap_atari_native.cpp) - JNI presne jako Java.
//
// Uzivatel z fora (Retroid Pocket 6): "po pripojeni by se na obrazovce stahnul nas d-pad a najelo to na origo
// tlacitka". Na sirku ma Atari D-pad a tlacitka jako Sega (B297); B320 je schova, kdyz hraje ovladac
// (Java: devCtl "ovladac:1"), dotyk na misto tlacitka je zase ukaze (devTouch vraci bit 0x2000 - Java to
// rekne NapOvladac), "ovladac:2" = schovane vzdy. Tlacitka a osy ovladace jdou pres JNI
// devOvladacKlavesa / devOvladacOsy do C++ (nap_ovladac.h atariBity) - samostatne od klavesnice telefonu
// (devPad): do stroje jde soucet, jedno druhemu nic nepusti.
// Infrastruktura (okno, OpenSL, JNIEnv, pomocne funkce) je okopirovana z ../b292/test_b292_jni_host.cpp.
//
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b320_atari_ovladac test_b320_atari_ovladac.cpp   (jeden radek)
//   ./test_b320_atari_ovladac
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

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.SetIntArrayRegion = fSetIntArrayRegion; g_tbl.SetShortArrayRegion = fSetShortArrayRegion;
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B320 - herni ovladac na Atari 130XE (C++ pristroj z appky, JNI jako Java)\n");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");
  int pa, tr, co, kb, sk;

  // ======== 1) tlacitka ovladace (na vysku i na sirku stejne) ========
  {
    ovl(96, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool fire = tr == 0;
    ovl(96, false); ovl(19, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool nahoru = pa == 0x0E && tr == 1;
    ovl(19, false); ovl(270, true); spi(150); stav(pa, tr, co, kb, sk);     // DPAD_UP_RIGHT
    const bool sikmo = pa == 0x06;
    ovl(270, false); spi(150); stav(pa, tr, co, kb, sk);
    const bool klid = pa == 0x0F && tr == 1;
    over(fire && nahoru && sikmo && klid, "1a) A = FIRE (TRIG0 0), krizek nahoru = joystick NAHORU (PORTA $E), uhlopricka nahoru-vpravo ($6), pusteno = klid");
    ovl(99, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool mezera = kb == 33 && !(sk & 4);
    ovl(99, false); spi(150); stav(pa, tr, co, kb, sk);
    const bool mezeraPust = (sk & 4) != 0;
    ovl(100, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool ret = kb == 12 && !(sk & 4);
    ovl(100, false); spi(250);
    klavesa("Return");
    over(mezera && mezeraPust && ret, "1b) X = klavesa MEZERA drzena a pustena, Y = RETURN (Wolfenstein: dvere / menu)");
    ovl(108, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool start = (co & 1) == 0;
    ovl(108, false); ovl(109, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool select = (co & 2) == 0;
    ovl(109, false); ovl(104, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool option = (co & 4) == 0;
    ovl(104, false); spi(250); stav(pa, tr, co, kb, sk);
    over(start && select && option && (co & 7) == 7, "1c) START / SELECT / L2 = konzolove START / SELECT / OPTION, po pusteni nic");
    const int zpet = ovl(4, true);
    ovl(4, false);
    over(zpet == 0 && ovl(110, true) != 0, "1d) ZPET neni tlacitko ovladace (vraci 0 - Android ji zpracuje jako drive), MODE ano");
    ovl(110, false); spi(250);
  }

  // ======== 2) osy ========
  {
    osy(0.9f, 0, 0, 0, 0, 0, 1 | 4 | 8); spi(150); stav(pa, tr, co, kb, sk);
    const bool vpravo = pa == 0x07;
    osy(0.4f, 0, 0, 0, 0, 0, 1 | 4 | 8); spi(150); stav(pa, tr, co, kb, sk);
    const bool hyst = pa == 0x07;
    osy(0, 0, 0, -1, 0, 0, 1 | 4 | 8); spi(150); stav(pa, tr, co, kb, sk);
    const bool hat = pa == 0x0E;
    osy(0, 0, 0, 0, 0, 0.8f, 1 | 4 | 8); spi(150); stav(pa, tr, co, kb, sk);
    const bool spoust = (co & 4) == 0 && pa == 0x0F;
    osy(0, 0, 0, 0, 0, 0, 1 | 4 | 8); spi(250); stav(pa, tr, co, kb, sk);
    over(vpravo && hyst && hat && spoust && pa == 0x0F && (co & 7) == 7,
         "2) leva packa 0,9 = VPRAVO ($7), 0,4 porad (hystereze), krizek HAT nahoru ($E), spoust R2 = OPTION, pusteno = klid");
  }

  // ======== 3) klavesnice telefonu (devPad z Javy) + ovladac: soucet, jedno nepusti druhe ========
  {
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 16);   // FIRE z klavesnice (stred)
    ovl(21, true);                                                                     // ovladac: vlevo
    spi(150); stav(pa, tr, co, kb, sk);
    const bool oboje = tr == 0 && pa == 0x0B;
    ovl(21, false); spi(150); stav(pa, tr, co, kb, sk);
    const bool jenFire = tr == 0 && pa == 0x0F;
    ovl(96, true); spi(100);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0);    // klavesnice pustila FIRE, ovladac drzi A
    spi(150); stav(pa, tr, co, kb, sk);
    const bool drziOvl = tr == 0;
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devOvladacPustNative(&g_env, nullptr);   // ovladac odpojen
    spi(150); stav(pa, tr, co, kb, sk);
    over(oboje && jenFire && drziOvl && tr == 1 && pa == 0x0F,
         "3) klavesnice telefonu (FIRE) + ovladac (vlevo) soucasne; pusteni ovladace nepusti klavesnici a naopak; ovladac odpojen (devOvladacPust) = klid");
  }

  // ======== 4) na sirku: dotykove ovladani schovat / ukazat ========
  {
    using D = nap::dev::Device;
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 2400, 1080);
    spi(1500);
    bool land = false;
    float dcx = 0, dcy = 0, rd = 0, fx = 0, fy = 0, gx = 0, gy = 0;
    {
      std::lock_guard<std::mutex> dl(g_mDev); D &d = devGet();
      land = d.land; dcx = d.cpx(D::C_DPAD); dcy = d.cpy(D::C_DPAD); rd = d.rDpad; fx = d.cpx(D::C_FIRE); fy = d.cpy(D::C_FIRE);
      gx = d.gearCx; gy = d.gearCy;
    }
    over(land, "4) displej 2400x1080 = na sirku (D-pad a tlacitka jako Sega)");
    const int W = 2400;
    const std::vector<uint32_t> videt = displej();
    const std::string r1 = ctl("ovladac:1");
    const std::vector<uint32_t> schovane = displej();
    const int dx0 = (int)(dcx - rd), dy0 = (int)(dcy - rd), dx1 = (int)(dcx + rd), dy1 = (int)(dcy + rd);
    const double zmenaDpad = rozdilOblasti(videt, schovane, W, dx0, dy0, dx1, dy1);
    const double zmenaFire = rozdilOblasti(videt, schovane, W, (int)fx - 40, (int)fy - 40, (int)fx + 40, (int)fy + 40);
    const double zmenaKolo = rozdilOblasti(videt, schovane, W, (int)gx - 20, (int)gy - 20, (int)gx + 20, (int)gy + 20);
    printf("ovladac:1 -> zmena oblasti D-padu %.0f %%, FIRE %.0f %%, kolecka %.0f %%\n", zmenaDpad, zmenaFire, zmenaKolo);
    over(r1 == "1" && zmenaDpad > 20 && zmenaFire > 20 && zmenaKolo < 1,
         "4a) devCtl \"ovladac:1\" (hraje herni ovladac): D-pad a tlacitka z displeje zmizi, ozubene kolecko zustava");
    // ovladac porad hraje, i kdyz je ovladani schovane
    ovl(96, true); spi(150); stav(pa, tr, co, kb, sk);
    const bool fireOvl = tr == 0;
    ovl(96, false); spi(150);
    // dotyk na schovany D-pad: ovladani se ukaze (bit 0x2000 pro Javu) a D-pad zaroven funguje
    const int r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 5, 0, dcx, dcy);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 5, 3, dcx, dcy - rd * .5f);
    spi(150); stav(pa, tr, co, kb, sk);
    const bool dpadJede = pa == 0x0E;
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 5, 1, dcx, dcy - rd * .5f);
    const std::vector<uint32_t> poDotyku = displej();
    const double zpet = rozdilOblasti(videt, poDotyku, W, (int)fx - 40, (int)fy - 40, (int)fx + 40, (int)fy + 40);
    int rez;
    { std::lock_guard<std::mutex> dl(g_mDev); rez = devGet().ovladacRezim; }
    over(fireOvl && (r & 0x2000) != 0 && dpadJede && rez == 0 && zpet < 5,
         "4b) schovane: ovladac hraje dal (FIRE); dotyk na misto D-padu ho ukaze (devTouch bit 0x2000 -> Java NapOvladac.dotyk), D-pad zaroven jede nahoru, FIRE zase vykresleny");
    // dotyk mimo ovladani (na obraz) ovladani neukaze
    ctl("ovladac:1");
    float ox, oy;
    { std::lock_guard<std::mutex> dl(g_mDev); D &d = devGet(); ox = d.obrX + d.obrW / 2; oy = d.obrY + d.obrH / 2; }
    const int rObraz = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 6, 0, ox, oy);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 6, 1, ox, oy);
    { std::lock_guard<std::mutex> dl(g_mDev); rez = devGet().ovladacRezim; }
    over((rObraz & 0x2000) == 0 && rez == 1, "4c) dotyk do obrazu (mimo ovladani) schovane ovladani neukaze");
    // rezim 2 = schovane vzdy: dotyk na D-pad nic (joystick se nehne), ovladani zustava schovane
    ctl("ovladac:2");
    const int r2 = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 7, 0, dcx, dcy);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 7, 3, dcx, dcy - rd * .5f);
    spi(150); stav(pa, tr, co, kb, sk);
    const bool nic = pa == 0x0F;
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 7, 1, dcx, dcy - rd * .5f);
    { std::lock_guard<std::mutex> dl(g_mDev); rez = devGet().ovladacRezim; }
    // kolecko (nastaveni) funguje vzdy
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 9, 0, gx, gy);
    const int ak = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, 9, 1, gx, gy);
    over((r2 & 0x2000) == 0 && nic && rez == 2 && (ak & 0xFFF) == nap::dev::SVC_CTRL,
         "4d) \"ovladac:2\" (OPTIONS: SKRYT DOTYK): dotyk na misto D-padu nic, ovladani zustava schovane; ozubene kolecko otevre menu jako drive");
    // uprava rozlozeni (edit) ukaze ovladani i pri schovani
    ctl("edit");
    bool skryte;
    { std::lock_guard<std::mutex> dl(g_mDev); skryte = devGet().ctlSkryte(); }
    ctl("done");
    const std::string r0 = ctl("ovladac:0");
    const std::vector<uint32_t> zase = displej();
    over(!skryte && r0 == "0" && rozdilOblasti(videt, zase, W, dx0, dy0, dx1, dy1) < 5,
         "4e) v uprave rozlozeni je ovladani videt i pri schovani; \"ovladac:0\" = zase vse jako drive");
    // na vysku se nic neschovava (pristroj Atari, joystick dotykem)
    ctl("ovladac:1");
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
    spi(1500);
    { std::lock_guard<std::mutex> dl(g_mDev); skryte = devGet().ctlSkryte(); }
    prst(300, 400, 12, 0); spi(60); prst(300, 340, 12, 3); spi(150);
    stav(pa, tr, co, kb, sk);
    prst(300, 340, 12, 1); spi(100);
    over(!skryte && pa == 0x0E, "4f) na vysku (pristroj Atari) se nic neschovava - joystick dotykem jako drive");
  }
  vypisLog();
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
