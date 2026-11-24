// test_b361_jni.cpp - B361: POKEY STEREO ve SKUTECNEM nap_atari_native.cpp (vlakno emulace jako v telefonu, stuby
// z ../b291/stub). Rene 23. 11. 2026: "udelej prosim jeste POKEY Dual Stereo".
//  1) samokontrola jadra: cpuHash 4A2C6CBA a memHash B0649DC5 beze zmeny
//  2) Atari nastartuje, POKEY STEREO je zapnute (devCtlNative "pokey_stereo" -> "1")
//  3) BASIC program (napsany klavesnici jako TXT z appky) pozna STEREO a hraje ton na kazdem POKEYi: zvuk, ktery jde
//     do reproduktoru i na TV (devPullTvAudioNative, stereo int16): levy kanal ~488 Hz (POKEY 1), pravy ~327 Hz
//     (POKEY 2); v logu "B361 POKEY 2" a "B361 POKEY STEREO"
//  4) vypnuti z menu (pokey_stereo:0): hned mono - levy = pravy, druhy POKEY ztichl
//  5) Atari bez upgradu: RESET + RUN -> program napise MONO, $D210 = zrcadlo prvniho POKEYe; zapnuti zpet: "1"
// Preklad (z test_overeni/b361, JAVA_HOME = JDK kvuli jni.h, vse na jednom radku):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b361_jni test_b361_jni.cpp
//   ./test_b361_jni                                                -> VYSTUP_test_b361_jni.txt
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

// ---------- nahrady OpenSL: zvuk "neni k dispozici" (zvuk pro TV jde dal) ----------
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
struct FakeShortArr { std::vector<uint8_t> b; std::vector<int16_t> v; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fGetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, jbyte *buf) { std::memcpy(buf, ((FakeArr *)a)->b.data() + st, n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.resize(n); return (jbyteArray)a; }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, n); }
static void JNICALL fSetShortArrayRegion(JNIEnv *, jshortArray a, jsize st, jsize n, const jshort *buf) { std::memcpy(((FakeShortArr *)a)->v.data() + st, buf, n * 2); }
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
// cely radek obrazovky (vypis programu obsahuje PRINT "STEREO" - ten se nepocita, jen to, co program napsal)
// (Atari ma levy okraj 2 znaky - radek obrazovky je "  STEREO")
static bool radek(const char *co) { return ("\n" + obrazovka()).find("\n  " + std::string(co) + "\n") != std::string::npos; }
static std::string ctl(const char *c) { FakeStr s{c}; return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&s))->s; }
static std::string g_log;
static void sberLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) g_log += ((FakeStr *)s)->s;
}
static void radkyLogu(const char *co) {
  for (size_t p = g_log.find(co); p != std::string::npos; p = g_log.find(co, p + 1)) printf("LOG: %s\n", g_log.substr(p, g_log.find('\n', p) - p).c_str());
}
// zvuk, ktery appka posila do reproduktoru a na TV: L a R zvlast (vyzvednout vse, co se od minula prehralo)
static void zvuk(std::vector<float> &l, std::vector<float> &r, int ms) {
  FakeShortArr z; z.b.resize(1 << 18); z.v.assign(1 << 18, 0);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPullTvAudioNative(&g_env, nullptr, (jshortArray)&z);   // stare pryc
  l.clear(); r.clear();
  for (int t = 0; t < ms; t += 100) {
    spi(100);
    const jint n = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPullTvAudioNative(&g_env, nullptr, (jshortArray)&z);
    for (jint i = 0; i + 1 < n; i += 2) { l.push_back(z.v[i] / 32768.0f); r.push_back(z.v[i + 1] / 32768.0f); }
  }
}
static double ton(const std::vector<float> &x, double f) {
  const double w = 2.0 * 3.14159265358979323846 * f / 44100.0, k = 2.0 * std::cos(w);
  double s1 = 0, s2 = 0;
  for (float v : x) { const double s = v + k * s1 - s2; s2 = s1; s1 = s; }
  return x.empty() ? 0 : std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - k * s1 * s2)) / (double)x.size();
}
static std::string c4(double v) { char b[32]; snprintf(b, sizeof b, "%.4f", v); return b; }

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.SetShortArrayRegion = fSetShortArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B361 - POKEY stereo ve skutecnem nap_atari_native.cpp (zvuk do reproduktoru a na TV)\n\n");

  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    const std::string js = ((FakeStr *)r)->s;
    const size_t p = js.find("\"cpuHash\":\""), q = js.find("\"memHash\":\"");
    const std::string h = p == std::string::npos ? "?" : js.substr(p + 11, 8), mh = q == std::string::npos ? "?" : js.substr(q + 11, 8);
    over(h == "4A2C6CBA" && mh == "B0649DC5", "1) samokontrola jadra: cpuHash " + h + " memHash " + mh + " (beze zmeny)");
  }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  const std::string zap = ctl("pokey_stereo");
  over(obrazovka().find("READY") != std::string::npos && zap == "1", "2) Atari nastartovalo do READY, POKEY STEREO zapnute (\"" + zap + "\")");

  {
    FakeStr prog{"10 POKE 53791,0:A=PEEK(53770):B=PEEK(53770):POKE 53791,3:POKE 53775,3\n"
                 "20 IF A=B THEN PRINT \"MONO\":GOTO 40\n"
                 "30 PRINT \"STEREO\"\n"
                 "40 SOUND 0,64,10,8\n"
                 "50 POKE 53784,0:POKE 53776,96:POKE 53777,166\n"
                 "60 GOTO 60"};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&prog, JNI_TRUE);
    for (int i = 0; i < 300 && !radek("STEREO") && !radek("MONO"); i++) spi(100);
    spi(800);
    std::vector<float> l, r;
    zvuk(l, r, 1500);
    sberLog();
    const double f1 = 1773447.0 / 28.0 / 130.0 * 44100.0 / (882.0 * 1773447.0 / 35568.0), f2 = 1773447.0 / 28.0 / 194.0 * 44100.0 / (882.0 * 1773447.0 / 35568.0);
    const double l1 = ton(l, f1), l2 = ton(l, f2), r1 = ton(r, f1), r2 = ton(r, f2);
    const bool stereo = radek("STEREO");
    if (!stereo) printf("%s", obrazovka().c_str());
    over(stereo, "3) program na obrazovce: STEREO (druhy POKEY nalezen - samostatny radek, ne vypis programu)");
    over(l.size() > 22050 && l1 > 20 * l2 && r2 > 20 * r1 && l1 > 0.05 && r2 > 0.05,
         "   zvuk pro reproduktor / TV (" + std::to_string(l.size()) + " vzorku na kanal): levy ton " + c4(f1).substr(0, 5) + " Hz sila " + c4(l1) +
             " (druhy " + c4(l2) + "), pravy ton " + c4(f2).substr(0, 5) + " Hz sila " + c4(r2) + " (prvni " + c4(r1) + ")");
    over(g_log.find("B361 POKEY 2: program pouziva druhy POKEY") != std::string::npos && g_log.find("B361 POKEY STEREO: program hraje") != std::string::npos,
         "   log: 'B361 POKEY 2 ...' a 'B361 POKEY STEREO: program hraje na druhy POKEY ...'");

    const std::string vyp = ctl("pokey_stereo:0");
    spi(400);
    zvuk(l, r, 1000);
    sberLog();
    bool stejne = !l.empty();
    for (size_t i = 0; i < l.size() && stejne; i++) if (l[i] != r[i]) stejne = false;
    over(vyp == "0" && stejne && ton(r, f2) < 0.002 && ton(l, f1) > 0.05 && g_log.find("B361 POKEY STEREO vypnuto") != std::string::npos,
         "4) vypnuti z menu (pokey_stereo:0 -> \"" + vyp + "\"): hned mono - levy = pravy (" + std::to_string(l.size()) + " vzorku), druhy POKEY ztichl (" +
             c4(ton(r, f2)) + "), prvni hraje dal (" + c4(ton(l, f1)) + ")");

    // 5) Atari BEZ upgradu (stereo vypnute): RESET (program v pameti zustane) a RUN -> program najde jen jeden POKEY,
    //    zapisy na $D210/$D211 jdou do prvniho POKEYe (zrcadlo) - kanal 1 prehraje 327 Hz misto 488 Hz, oba kanaly stejne
    ctl("atari_reset");   // RESET jde frontou do vlakna emulace: pockat, az OS smaze obrazovku (STEREO zmizi) a napise READY
    for (int i = 0; i < 100 && (radek("STEREO") || !radek("READY")); i++) spi(100);
    FakeStr nic{""};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&nic, JNI_TRUE);
    for (int i = 0; i < 200 && !radek("MONO") && !radek("STEREO"); i++) spi(100);
    spi(600);
    zvuk(l, r, 1000);
    sberLog();
    stejne = !l.empty();
    for (size_t i = 0; i < l.size() && stejne; i++) if (l[i] != r[i]) stejne = false;
    if (!radek("MONO")) printf("%s", obrazovka().c_str());
    over(radek("MONO") && stejne && ton(l, f2) > 0.05 && ton(l, f1) < 0.002 && g_log.find("B361 POKEY STEREO konec (vypnuto v menu)") != std::string::npos,
         "5) Atari bez upgradu: RESET + RUN -> na obrazovce MONO, levy = pravy, ton " + c4(f2).substr(0, 5) + " Hz z prvniho POKEYe (zrcadlo $D210) sila " +
             c4(ton(l, f2)) + ", " + c4(f1).substr(0, 5) + " Hz " + c4(ton(l, f1)) + "; log 'konec (vypnuto v menu)'");
    const std::string zpet = ctl("pokey_stereo:1");
    sberLog();
    over(zpet == "1" && g_log.find("B361 POKEY STEREO zapnuto") != std::string::npos, "   zapnuti zpet (pokey_stereo:1 -> \"" + zpet + "\"), v logu 'zapnuto'");
  }
  radkyLogu("B361");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d CHYBA\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
