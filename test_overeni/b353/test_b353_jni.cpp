// test_b353_jni.cpp - B353: SKUTECNY nap_atari_native.cpp (vlakna a tempo 50 snimku/s jako v telefonu, stuby
// z ../b291/stub) - TURBO PRI NAHRAVANI. Rene: "to atr urychli prosim tak jak si psal".
//  1) samokontrola jadra beze zmeny (B353 meni jen tempo emulace v nap_atari_native.cpp)
//  2) disketa s vlastnim zavadecem pres POKEY (program z ../b348 = jako xBIOS v Prince of Persia): 31 sektoru
//     BEZ turba (g_turboPovoleno = false) a S turbem - data stejna a spravna, s turbem rychleji (cas na PC),
//     log "B353 TURBO start: cteni diskety D1:" a "B353 TURBO konec ... (N x rychleji)"
//  3) zvuk pri turbu: do fronty jde v realnem case jen ticho (fronta neroste nad 44100 vzorku / s)
//  4) hrac drzi joystick (herni ovladac) -> zadne turbo, i kdyz se cte disketa
//  5) kdyz se nic nenahrava, bezi emulace dal 50 snimku / s (turbo samo skonci)
//  6) volitelne XEX s dlouhym rozbalovanim (Tony VBXE Reneho, v repu neni): zavadec XEX v turbu
//  7) volitelne Prince of Persia (PoP_ATR_20211206_final.rar Reneho): ze hry v RAR (devArchivNative) az po titulku
//     (~145 sektoru vlastnim zavadecem) - v telefonu B350 to trvalo ~32 s
// Preklad (z test_overeni/b353, JAVA_HOME = JDK kvuli jni.h, vse na jednom radku):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b353_jni test_b353_jni.cpp
//   ./test_b353_jni [TONY_ZAMEK_VBXE_DEV31_PARTIAL.xex [PoP_ATR_20211206_final.rar]]   -> VYSTUP_test_b353_jni.txt
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


// disketa jako v ../b348/test_b348_jni.cpp: bootovaci s programem (vlastni SIO pres POKEY), data = vzorec
static uint8_t obsah(int s, int i) { return (uint8_t)(s * 7 + i * 13 + (s >> 8) * 5 + 1); }
static std::vector<uint8_t> atr(int sektoru) {
  std::vector<uint8_t> a;
  const size_t bajtu = (size_t)sektoru * 128, par = bajtu / 16;
  a.assign(16 + bajtu, 0);
  a[0] = 0x96; a[1] = 0x02; a[2] = par & 0xFF; a[3] = (par >> 8) & 0xFF; a[4] = 128;
  for (int s = 1; s <= sektoru; s++) for (int i = 0; i < 128; i++) a[16 + (size_t)(s - 1) * 128 + i] = obsah(s, i);
  for (size_t i = 0; i < sizeof(B348_KOD); i++) a[16 + i] = B348_KOD[i];
  return a;
}
static std::string nahrajAtr(const std::vector<uint8_t> &d, const std::string &jm) {
  FakeArr a; a.b = d;
  FakeStr n{jm};
  return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n))->s;
}
static long long snimky() { return g_snimkuCelkem.load(); }
static unsigned long long zvukZapsano() { std::lock_guard<std::mutex> z(g_mTvZvuk); return g_tvZvukZapis; }
static double ted() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

struct Vysledek { double s = -1; long long snimku = 0; bool data = false; int ok = 0; unsigned long long zvuk = 0; };
// nabootuje disketu, ceka na program (READY), zapise tabulku 31 cteni (sektory 10-40 do stranky $30) a GO
// (tabulka programu B348 ma index 8 bitu po 8 B - nejvys 31 prikazu + koncova nula)
static Vysledek cteni(bool turbo, bool drzJoystick) {
  Vysledek v;
  { std::lock_guard<std::mutex> l(g_mStroj); g_turboPovoleno = turbo; }
  const std::string r = nahrajAtr(atr(720), "zavadec_pokey.atr");
  if (r.rfind("OK ", 0) != 0) { printf("devLoadAtr: %s\n", r.c_str()); return v; }
  for (int i = 0; i < 1000 && peekS(0x6FE) != 0xA5; i++) spi(10);
  if (peekS(0x6FE) != 0xA5) { printf("program nenabootoval\n"); return v; }
  if (drzJoystick) { Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(nullptr, nullptr, 8); spi(100); }   // vpravo
  const int N = 31;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    for (int k = 0; k < N; k++) {
      const uint8_t e[8] = {0x31, 'R', (uint8_t)(10 + k), 0, 128, 2, 0x30, 0};
      for (int i = 0; i < 8; i++) g_stroj->mem.ram[B348_TAB + k * 8 + i] = e[i];
    }
    for (int i = 0; i < 8; i++) g_stroj->mem.ram[B348_TAB + N * 8 + i] = 0;
    g_stroj->mem.ram[0x6FF] = 1;                                     // GO
  }
  const double t0 = ted(); const long long f0 = snimky(); const unsigned long long z0 = zvukZapsano();
  while (peekS(0x6F0) != 0x5A && ted() - t0 < 30) spi(2);
  v.s = ted() - t0; v.snimku = snimky() - f0; v.zvuk = zvukZapsano() - z0;
  if (drzJoystick) Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(nullptr, nullptr, 0);
  std::lock_guard<std::mutex> l(g_mStroj);
  if (g_stroj->mem.ram[0x6F0] != 0x5A) return v;
  for (int k = 0; k < N; k++) {
    const uint8_t *res = &g_stroj->mem.ram[0x600 + k * 4];
    if (res[0] == 'A' && res[2] == 'C' && res[3] == 1) v.ok++;
  }
  v.data = true;
  for (int i = 0; i < 128; i++) if (g_stroj->mem.ram[0x3000 + i] != obsah(10 + N - 1, i)) v.data = false;   // posledni sektor
  return v;
}

int main(int argc, char **argv) {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B353 - skutecny nap_atari_native.cpp: turbo pri nahravani (disketa s vlastnim zavadecem pres POKEY, XEX)\n\n");

  // 1) samokontrola
  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    const std::string js = ((FakeStr *)r)->s;
    const size_t p = js.find("\"cpuHash\":\""), q = js.find("\"memHash\":\"");
    const std::string h = p == std::string::npos ? "?" : js.substr(p + 11, 8), mh = q == std::string::npos ? "?" : js.substr(q + 11, 8);
    printf("SELFTEST cpuHash %s memHash %s\n", h.c_str(), mh.c_str());
    over(h == "4A2C6CBA" && mh == "B0649DC5", "samokontrola: cpuHash 4A2C6CBA a memHash B0649DC5 beze zmeny");
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");

  // 2) bez turba a s turbem
  const Vysledek bez = cteni(false, false);
  printf("BEZ turba: 31 sektoru za %.2f s, %lld snimku Atari (%.1f snimku / s), OK %d, data %s\n", bez.s, bez.snimku, bez.snimku / bez.s, bez.ok,
         bez.data ? "sedi" : "NESEDI");
  over(bez.data && bez.ok == 31 && bez.snimku / bez.s < 56, "bez turba: 31 sektoru vlastnim SIO pres POKEY, data sedi, tempo realne (50 snimku / s)");
  sberLog();
  const int turboRadkuPred = (int)std::count(g_log.begin(), g_log.end(), '\n');
  (void)turboRadkuPred;
  const Vysledek s = cteni(true, false);
  sberLog();
  printf("S turbem:  31 sektoru za %.2f s, %lld snimku Atari (%.1f snimku / s), OK %d, data %s\n", s.s, s.snimku, s.snimku / s.s, s.ok,
         s.data ? "sedi" : "NESEDI");
  over(s.data && s.ok == 31, "s turbem: stejnych 31 sektoru, data sedi (jadro bezi stejne, jen rychleji)");
  printf("zrychleni: %.1fx (snimku Atari na sekundu %.1f proti %.1f)\n", bez.s / s.s, s.snimku / s.s, bez.snimku / bez.s);
  over(s.s < bez.s * 0.6 && s.snimku / s.s > 75, "turbo opravdu zrychli (na PC aspon 1,5x proti realnemu tempu)");
  {
    const size_t p = g_log.find("B353 TURBO start: cteni diskety D1: zavadec_pokey.atr");
    const size_t q = g_log.find("B353 TURBO konec:", p == std::string::npos ? 0 : p);
    if (p != std::string::npos) printf("LOG: %s\n", g_log.substr(p, g_log.find('\n', p) - p).c_str());
    if (q != std::string::npos) printf("LOG: %s\n", g_log.substr(q, g_log.find('\n', q) - q).c_str());
    over(p != std::string::npos && q != std::string::npos, "log: \"B353 TURBO start: cteni diskety D1: ...\" a \"B353 TURBO konec: ... rychleji\"");
  }
  // 3) zvuk: v realnem case (2 kanaly x 44100 / s), ne za kazdy emulovany snimek
  {
    const double zvukS = s.zvuk / 2.0 / 44100.0, atariS = s.snimku / 50.0;
    printf("zvuk pri turbu: %.2f s zvuku za %.2f s skutecneho casu (emulovano %.2f s Atari)\n", zvukS, s.s, atariS);
    over(zvukS < s.s * 1.3 + 0.1 && atariS > zvukS * 1.4, "zvuk pri turbu: fronta dostava jen realny cas (ticho), ne vsechny emulovane snimky");
  }
  // 4) hrac drzi joystick -> bez turba
  {
    const Vysledek j = cteni(true, true);
    printf("S turbem, ale joystick drzeny: 31 sektoru za %.2f s, %.1f snimku / s, OK %d\n", j.s, j.snimku / j.s, j.ok);
    over(j.data && j.ok == 31 && j.snimku / j.s < 56, "hrac drzi joystick (herni ovladac): zadne turbo, i kdyz se cte disketa");
  }
  // 5) nic se nenahrava -> 50 snimku / s
  {
    spi(1500);
    const long long f0 = snimky(); const double t0 = ted();
    spi(2000);
    const double fps = (snimky() - f0) / (ted() - t0);
    printf("program skoncil, nic se necte: %.1f snimku / s, turbo bezi %d\n", fps, (int)g_turboBezi);
    over(fps > 44 && fps < 56 && !g_turboBezi, "po nahrani turbo samo skonci - emulace zase 50 snimku / s");
  }
  // 6) XEX
  if (argc > 1) {
    std::vector<uint8_t> d = nacti(argv[1]);
    std::string jm = argv[1];
    const size_t lom = jm.find_last_of('/');
    if (lom != std::string::npos) jm = jm.substr(lom + 1);
    FakeArr a; a.b = d; FakeStr n{jm};
    const double t0 = ted(); const long long f0 = snimky();
    const bool ok = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n) == JNI_TRUE;
    bool hotovo = false;
    for (int i = 0; i < 1200 && !hotovo; i++) { spi(50); sberLog(); hotovo = g_log.find("nahrano: 444 segmentu") != std::string::npos; }
    const double s6 = ted() - t0; const long long f6 = snimky() - f0;
    spi(500); sberLog();
    printf("XEX %s: nahrano za %.1f s skutecneho casu, %lld snimku Atari (%.1f s Atari, %.1fx)\n", jm.c_str(), s6, f6, f6 / 50.0, f6 / 50.0 / s6);
    for (size_t p = g_log.find("B353 TURBO"); p != std::string::npos; p = g_log.find("B353 TURBO", p + 1))
      if (g_log.compare(p, 30, "B353 TURBO start: zavadec XEX ") == 0 || g_log.compare(p, 17, "B353 TURBO konec:") == 0)
        printf("LOG: %s\n", g_log.substr(p, g_log.find('\n', p) - p).c_str());
    over(ok && hotovo && f6 / 50.0 > s6 * 1.5 && g_log.find("B353 TURBO start: zavadec XEX " + jm) != std::string::npos,
         "XEX " + jm + ": zavadec segmentu a rozbalovani (INIT) v turbu - nahrano rychleji nez v realnem case");
  }

  // 7) Prince of Persia z RAR az po titulku
  if (argc > 2) {
    std::vector<uint8_t> d = nacti(argv[2]);
    std::string jm = argv[2];
    const size_t lom = jm.find_last_of('/');
    if (lom != std::string::npos) jm = jm.substr(lom + 1);
    FakeArr a; a.b = d; FakeStr n{jm};
    FakeArr *r = (FakeArr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devArchivNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n, JNI_FALSE);
    const std::string vybrany = ctl("archiv_vybrany");
    const double t0 = ted(); const long long f0 = snimky();
    const std::string rr = r ? nahrajAtr(r->b, vybrany) : std::string("CHYBA");
    long long cteno = 0;
    while (ted() - t0 < 60) {
      spi(100);
      std::lock_guard<std::mutex> l(g_mStroj);
      cteno = g_stroj->disk.reads;
      if (cteno >= 145) break;
    }
    const double s7 = ted() - t0; const long long f7 = snimky() - f0;
    printf("PoP %s (%s): %lld sektoru do titulky za %.1f s skutecneho casu, %.1f s Atari (%.1fx); telefon B350 bez turba ~32 s\n",
           vybrany.c_str(), rr.substr(0, 2).c_str(), cteno, s7, f7 / 50.0, f7 / 50.0 / s7);
    over(rr.rfind("OK ", 0) == 0 && cteno >= 145 && f7 / 50.0 > s7 * 1.5, "Prince of Persia z RAR: nahravani do titulky v turbu (data ctena vlastnim zavadecem pres POKEY)");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
