// B292: KONEC-KONCU test SKUTECNEHO nap_atari_native.cpp s NOVYM jadrem na PC
// (stejny princip jako test_overeni/b291/test_b291_jni_host.cpp: Android je
// nahrazen malymi nahradami v ../b291/stub, bezi skutecna vlakna emulace a
// kresleni a test "saha prstem" na pristroj jako Java).
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/atari
//       -o test_b292_jni_host test_b292_jni_host.cpp        (vse na jednom radku)
//   ./test_b292_jni_host
// Overuje:
//  1) samokontrolu jadra (runSelfTest) - cisla pro NativeAtariCoreBridge.java
//  2) drzeni klavesy prstem: OS ji po ~1 s zacne opakovat, po pusteni prestane
//  3) CSAVE -> WAV (linka SIO DATA OUT) pres vlakna -> devTakeWav
//  4) EJECT vraci Jave akci "vyber kazetu"
//  5) ten WAV vlozeny jako kazeta (devLoadTape) -> CLOAD, PLAY, RETURN -> LIST
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

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};

  // 1) samokontrola
  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    std::string js = ((FakeStr *)r)->s;
    printf("SELFTEST %s\n", js.c_str());
    over(js.find("\"cpuInstr\":51200") != std::string::npos && js.find("\"memReads\":16252928") != std::string::npos,
         "samokontrola probehla (51200 instrukci, 16252928 cteni)");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");

  // 2) drzeni klavesy: "A" drzene 2,5 s -> OS opakuje (KRPDEL ~1 s, pak KEYREP)
  {
    const auto *k = klavesaDef("A");
    prst(k->x + k->w / 2, k->y + k->h / 2, 1, 0);
    spi(2500);
    prst(k->x + k->w / 2, k->y + k->h / 2, 1, 1);
    spi(400);
    std::string s1 = obrazovka();
    spi(800);
    std::string s2 = obrazovka();
    int a = pocet(s1, 'A') - 1;                        // "READY" ma jedno A
    printf("drzeni A 2,5 s -> %d znaku A\n", a);
    over(a >= 5, "drzena klavesa se opakuje jako na skutecnem Atari");
    over(pocet(s2, 'A') == pocet(s1, 'A'), "po pusteni klavesy opakovani prestalo");
    klavesa("Return");
  }

  // 3) CSAVE pres vlakna
  napis("NEW\n10 PRINT \"KAZETA B292\"\n20 GOTO 10\nCSAVE\n");
  spi(6000);                                           // napsani + dve pipnuti
  klavesa("Return");                                   // "stisk klavesy" -> motor + data
  jbyteArray wav = nullptr;
  for (int i = 0; i < 450 && !wav; i++) { spi(100); wav = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTakeWavNative(&g_env, nullptr); }
  vypisLog();
  over(wav != nullptr, "CSAVE: hotovy WAV prisel do Javy (devTakeWav)");
  if (!wav) { printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb + 1); return 1; }
  // WAV soubor z PCM (presne jako Java ulozitAtariPcmWav)
  FakeArr soubor;
  {
    const std::vector<uint8_t> &pcm = ((FakeArr *)wav)->b;
    const uint32_t nb = (uint32_t)pcm.size();
    uint8_t h[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
    uint32_t r = 36 + nb; std::memcpy(h + 4, &r, 4); std::memcpy(h + 40, &nb, 4);
    soubor.b.assign(h, h + 44); soubor.b.insert(soubor.b.end(), pcm.begin(), pcm.end());
    printf("WAV %.1f s\n", pcm.size() / 2 / 44100.0);
  }

  // 4) EJECT -> akce pro Javu
  int akce = klep(627 + 41, 1440 + 35);
  over(akce == nap::dev::SVC_EJECT, "EJECT vraci Jave akci VYBER KAZETY");
  { std::lock_guard<std::mutex> dl(g_mDev); over(devGet().doorOpen, "EJECT otevrel dvirka"); }

  // 5) kazeta -> CLOAD (po studenem startu: POWER vyp/zap - kazeta zustava v magnetofonu)
  FakeStr nm{"kazeta_b292.wav"};
  jstring vr = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadTapeNative(&g_env, nullptr, (jbyteArray)&soubor, (jstring)&nm);
  printf("devLoadTape: %s\n", ((FakeStr *)vr)->s.c_str());
  over(((FakeStr *)vr)->s.rfind("OK ", 0) == 0, "kazeta vlozena (WAV demodulovan)");
  { std::lock_guard<std::mutex> dl(g_mDev); over(!devGet().doorOpen, "po vlozeni kazety se dvirka zavrela"); }
  klep(8 + 17, 393 + 25); spi(400); klep(8 + 17, 393 + 25);   // POWER vyp / zap
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  napis("CLOAD\n");
  spi(3000);                                           // pipnuti, OS ceka na klavesu
  klep(383 + 40, 1440 + 35);                           // PLAY
  klavesa("Return");
  bool motor = false;
  for (int i = 0; i < 400; i++) {
    spi(100);
    if (g_motorOn.load()) motor = true;
    if (motor && !g_motorOn.load()) break;
  }
  spi(500);
  napis("LIST\n");
  spi(2500);
  vypisLog();
  std::string scr = obrazovka();
  printf("--- obrazovka po CLOAD + LIST ---\n%s---\n", scr.c_str());
  over(motor, "CLOAD zapnul motor (pasek bezel)");
  over(scr.find("10 PRINT \"KAZETA B292\"") != std::string::npos && scr.find("20 GOTO 10") != std::string::npos,
       "CLOAD nahral program z kazety (LIST ho ukazuje)");

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  vypisLog();
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
