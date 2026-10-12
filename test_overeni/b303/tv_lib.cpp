// B303: knihovna pro KONEC-KONCU test prohlizece (test_b303_prohlizec.py).
//
// SKUTECNY nap_atari_native.cpp (Android nahrazen stuby z ../b291/stub, stejne
// jako test_b292_jni_host.cpp): vlakno emulace a kresleni bezi jako v appce.
// Python server dela presne to, co v appce Java (MainActivity: /status,
// /klavesa(y), /atari.stream, /vloz) a vola TYTEZ JNI funkce - devPcKlavesa,
// devTvProudOtevri / Dalsi / Zavri, devVloz, devGrabFrame.
//
//   g++ -std=c++17 -O2 -shared -fPIC -pthread -I../b291/stub -I$JAVA_HOME/include \
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/atari -o libtvproud.so tv_lib.cpp
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include <string>
#include <vector>

// ---------- nahrady OpenSL: zvuk "neni k dispozici" (zvuk pro TV jde dal do proudu) ----------
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

// ---------- rucne sestaveny JNIEnv (jen to, co JNI funkce pouzivaji) ----------
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

static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }

static std::string obrazovkaText() {
  std::lock_guard<std::mutex> l(g_mStroj);
  if (!g_stroj) return std::string();
  Machine &m = *g_stroj;
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    std::string line;
    for (int c = 0; c < 40; c++) {
      int v = m.mem.ram[(sav + r * 40 + c) & 0xFFFF] & 0x7F;
      int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v);
      if (a < 32 || a > 126) a = ' ';
      line.push_back((char)a);
    }
    while (!line.empty() && line.back() == ' ') line.pop_back();
    out += line + "\n";
  }
  return out;
}

extern "C" {
// appka: Atari 130XE (HELP) na obrazovce, studeny start s BASICem az do READY
int tv_start() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.SetIntArrayRegion = fSetIntArrayRegion;
  g_env.functions = &g_tbl;
  static FakeStr surface{"plocha"};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160; i++) {
    if (obrazovkaText().find("READY") != std::string::npos) return 1;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }
  return 0;
}
void tv_stop() { Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr); }

// /klavesa(y) -> devPcKlavesaNative (Java napTvWebJednaKlavesa)
const char *tv_klavesa(const char *code, const char *znak, int mod, int dolu) {
  thread_local std::string r;
  FakeStr c{code ? code : ""}, z{znak ? znak : ""};
  r = jstr(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPcKlavesaNative(&g_env, nullptr, (jstring)&c, (jstring)&z, mod, dolu ? JNI_TRUE : JNI_FALSE));
  return r.c_str();
}

// /atari.stream -> devTvProud* (Java napTvWebAtariProud)
int tv_otevri() { return Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTvProudOtevriNative(&g_env, nullptr); }
int tv_dalsi(int id, uint8_t *out, int kap, int cekejMs) {
  thread_local FakeArr a;
  if ((int)a.b.size() != kap) a.b.assign((size_t)kap, 0);
  const jint n = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTvProudDalsiNative(&g_env, nullptr, id, (jbyteArray)&a, cekejMs);
  if (n > 0) std::memcpy(out, a.b.data(), (size_t)n);
  return n;
}
void tv_zavri(int id) { Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTvProudZavriNative(&g_env, nullptr, id); }

// /vloz -> devVlozNative (Java napTvWebVloz)
const char *tv_vloz(const uint8_t *d, int n) {
  thread_local std::string r;
  FakeArr a; a.b.assign(d, d + (n > 0 ? n : 0));
  r = jstr(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devVlozNative(&g_env, nullptr, (jbyteArray)&a));
  return r.c_str();
}

// obraz z jadra (devGrabFrameNative: 672x240 ARGB) - srovnani s canvasem v prohlizeci
int tv_obraz(int32_t *out) {
  FakeIntArr a; a.b.resize(672 * 240); a.v.assign(672 * 240, 0);
  const jint wh = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devGrabFrameNative(&g_env, nullptr, (jintArray)&a);
  if (wh > 0) std::memcpy(out, a.v.data(), a.v.size() * 4);
  return wh;
}
long long tv_seq() { return (long long)g_snimekSeq.load(); }
int tv_psani() { std::lock_guard<std::mutex> l(g_mStroj); return (int)g_psani.stav; }
int tv_fronta() {
  std::lock_guard<std::mutex> l(g_mStroj);
  std::lock_guard<std::mutex> p(g_mPc);
  return (int)(g_pcHrac.q.size() + g_pcText.size() + g_pcVstup.size());
}
const char *tv_obrazovka() { thread_local std::string r; r = obrazovkaText(); return r.c_str(); }
const char *tv_log() { thread_local std::string r; r = jstr(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr)); return r.c_str(); }
}
