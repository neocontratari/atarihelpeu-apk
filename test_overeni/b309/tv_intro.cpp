// B307: knihovna / zaklad pro test UVODNIHO DEMA (skutecny nap_intro_gl.cpp) na PC.
//
// Stejny princip jako ../b306/tv_intro.cpp (plocha telefonu = pbuffer pres Mesa,
// assety ze souboru, JNIEnv rucne, OpenSL ES = "zvukova karta" ve vlakne, ktere si
// kazdych 23,2 ms vezme blok 1024 ramcu a GetPosition vraci, kolik uz prehrala).
// B307 navic:
//  - introStart sam pusti zvuk (zadne introZvukStartNative / Java AudioTrack),
//  - introDotyk (tlacitko SKIP kresli C++, o dotyku rozhoduje C++),
//  - zabery a zvuk z Reneho hry (assets/intro/pittkitt.klip),
//  - karta umi bezet rychleji (cela stopa za par vterin) a umi "nejit" (OpenSL selze).
//   viz JAK_SPUSTIT.txt
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <jni.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <thread>
#include <vector>

// ---------- okno telefonu = pbuffer ----------
#include "android/native_window.h"
struct ANativeWindow { int w = 360, h = 780; int refs = 1; };
static ANativeWindow g_okno;
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *w) { return w->w; }
int32_t ANativeWindow_getHeight(ANativeWindow *w) { return w->h; }
int32_t ANativeWindow_getFormat(ANativeWindow *) { return 1; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t) { return 0; }
int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *) { return -1; }
int32_t ANativeWindow_unlockAndPost(ANativeWindow *) { return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject s) { if (!s) return nullptr; g_okno.refs++; return &g_okno; }

#undef EGL_WINDOW_BIT
#define EGL_WINDOW_BIT EGL_PBUFFER_BIT
static EGLSurface testPlocha(EGLDisplay d, EGLConfig c, ANativeWindow *w) {
  const EGLint a[] = {EGL_WIDTH, w->w, EGL_HEIGHT, w->h, EGL_NONE};
  return eglCreatePbufferSurface(d, c, a);
}
#define eglCreateWindowSurface(d, c, w, a) testPlocha(d, c, (ANativeWindow *)(w))

// ---------- AssetManager: soubory z app/src/main/assets ----------
#include "android/asset_manager.h"
struct AAssetManager { int x; };
struct AAsset { std::vector<char> d; };
static std::string g_assety = "../../app/src/main/assets/";
static std::atomic<int> g_assetuOtevreno{0}, g_assetuZavreno{0};
AAssetManager *AAssetManager_fromJava(JNIEnv *, jobject) { static AAssetManager m; return &m; }
AAsset *AAssetManager_open(AAssetManager *, const char *jm, int) {
  std::ifstream f(g_assety + jm, std::ios::binary);
  if (!f) return nullptr;
  AAsset *a = new AAsset;
  a->d.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  g_assetuOtevreno++;
  return a;
}
const void *AAsset_getBuffer(AAsset *a) { return a->d.data(); }
off_t AAsset_getLength(AAsset *a) { return (off_t)a->d.size(); }
void AAsset_close(AAsset *a) { g_assetuZavreno++; delete a; }

#include "../../app/src/main/cpp/intro/nap_intro_gl.cpp"

// ---------- OpenSL ES: zvukova karta, ktera si bere bloky v realnem case ----------
static const SLInterfaceID_ iidE{1}, iidP{2}, iidQ{3};
const SLInterfaceID SL_IID_ENGINE = &iidE;
const SLInterfaceID SL_IID_PLAY = &iidP;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &iidQ;
namespace karta {
static slAndroidSimpleBufferQueueCallback cb = nullptr;
static void *cbCtx = nullptr;
static std::thread vl;
static std::atomic<bool> hraje{false};
static std::atomic<long long> prehrano{0};      // ramcu "prehranych" (GetPosition)
static std::atomic<int> zarazeno{0};            // bloku ve fronte
static std::atomic<int> vytvoreno{0}, zniceno{0};
static std::atomic<int> rychlost{1};            // B307: 1 = realny cas, 8 = cela stopa za ~6 s
static std::atomic<bool> selhat{false};         // B307: OpenSL "nejde" (slCreateEngine vrati chybu)
static SLresult realize(SLObjectItf, SLboolean) { return SL_RESULT_SUCCESS; }
static void destroy(SLObjectItf);
static SLresult objGetInterface(SLObjectItf self, const SLInterfaceID iid, void *p);
static const SLObjectItf_ objTbl = {realize, objGetInterface, destroy};
static const SLObjectItf_ *engineObj = &objTbl, *mixObj = &objTbl, *playerObj = &objTbl;
static SLresult createPlayer(SLEngineItf, SLObjectItf *p, SLDataSource *, SLDataSink *, SLuint32, const SLInterfaceID *, const SLboolean *) {
  *p = &playerObj; vytvoreno++; zarazeno = 0; prehrano = 0; return SL_RESULT_SUCCESS;
}
static SLresult createMix(SLEngineItf, SLObjectItf *p, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &mixObj; return SL_RESULT_SUCCESS; }
static const SLEngineItf_ engTbl = {createPlayer, createMix};
static const SLEngineItf_ *engItf = &engTbl;
static void spust();
static SLresult setPlayState(SLPlayItf, SLuint32 st) { if (st == SL_PLAYSTATE_PLAYING) spust(); return SL_RESULT_SUCCESS; }
static SLresult getPosition(SLPlayItf, SLmillisecond *ms) { *ms = (SLmillisecond)(prehrano.load() * 1000 / 44100); return SL_RESULT_SUCCESS; }
static const SLPlayItf_ playTbl = {setPlayState, getPosition};
static const SLPlayItf_ *playItf = &playTbl;
static std::atomic<bool> nahravat{false};        // test: vse, co appka zaradi do fronty (= co uslysi reproduktor)
static std::vector<int16_t> nahrano;
static SLresult enqueue(SLAndroidSimpleBufferQueueItf, const void *b, SLuint32 n) {
  if (nahravat.load()) { const int16_t *s = (const int16_t *)b; nahrano.insert(nahrano.end(), s, s + n / 2); }
  zarazeno++;
  return SL_RESULT_SUCCESS;
}
static SLresult registerCb(SLAndroidSimpleBufferQueueItf, slAndroidSimpleBufferQueueCallback c, void *ctx) { cb = c; cbCtx = ctx; return SL_RESULT_SUCCESS; }
static const SLAndroidSimpleBufferQueueItf_ qTbl = {enqueue, registerCb};
static const SLAndroidSimpleBufferQueueItf_ *qItf = &qTbl;
static SLresult objGetInterface(SLObjectItf, const SLInterfaceID iid, void *p) {
  if (iid == SL_IID_ENGINE) *(SLEngineItf *)p = &engItf;
  else if (iid == SL_IID_PLAY) *(SLPlayItf *)p = &playItf;
  else if (iid == SL_IID_ANDROIDSIMPLEBUFFERQUEUE) *(SLAndroidSimpleBufferQueueItf *)p = &qItf;
  else return 1;
  return SL_RESULT_SUCCESS;
}
static void spust() {
  if (hraje.exchange(true)) return;
  vl = std::thread([] {
    auto t = std::chrono::steady_clock::now();
    while (hraje.load()) {
      t += std::chrono::microseconds(1024 * 1000000LL / 44100 / std::max(1, rychlost.load()));   // blok 1024 ramcu dohral
      std::this_thread::sleep_until(t);
      if (!hraje.load()) break;
      if (zarazeno.load() <= 0) continue;                           // fronta prazdna = ticho
      zarazeno--;
      prehrano += 1024;
      if (cb) cb(&qItf, cbCtx);
    }
  });
}
static void destroy(SLObjectItf self) {
  if (self == &playerObj) {
    zniceno++;
    if (hraje.exchange(false) && vl.joinable()) vl.join();
  }
}
}  // namespace karta
SLresult slCreateEngine(SLObjectItf *p, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) {
  if (karta::selhat.load()) return 9;           // SL_RESULT_... (chyba)
  *p = &karta::engineObj;
  return SL_RESULT_SUCCESS;
}

// ---------- rucne sestaveny JNIEnv ----------
struct FakeArr { std::vector<uint8_t> b; };
struct FakeStr { std::string s; };
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, (size_t)n); }
static jobject JNICALL fNewGlobalRef(JNIEnv *, jobject o) { return o; }
static void JNICALL fDeleteGlobalRef(JNIEnv *, jobject) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }
static int g_am = 1, g_plocha = 2;

// referencni stopa (spocitana samostatne, podle zadani B307): stopa dema + zvuk Reneho hry
// (klip: usek 2 = jizda, od 6. snimku, delka STRIH_16 - ATARI_JIZDA, od casu ATARI_JIZDA, hlasitost 1,8)
static bool referencniStopa(napdemo::zv::Stopa &st, const std::string &assety) {
  bool hra = false;
  std::ifstream f(assety + "intro/pittkitt.klip", std::ios::binary);
  if (f) {
    std::vector<unsigned char> d((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    napdemo::KlipAtari k;
    if (k.nacti(d.data(), d.size()) && k.useky.size() >= 3 && k.vzorkovani == 44100) {
      const auto &u = k.useky[2];
      const long long a = (long long)((u.prvni + 6) * (double)k.vzorkovani / k.fps);
      const long long n = (long long)((napdemo::cas::STRIH_16 - napdemo::cas::ATARI_JIZDA) * k.vzorkovani);
      std::vector<int16_t> z;
      for (long long i = 0; i < n && a + i < (long long)k.zvuk.size(); i++) z.push_back(k.zvuk[(size_t)(a + i)]);
      if (!z.empty()) { st.nastavZaznam(z.data(), z.size(), napdemo::cas::ATARI_JIZDA, napdemo::cas::ZVUK_HRY); hra = true; }
    }
  }
  st.vyrob();
  return hra;
}

extern "C" {
// demo jako v appce (MainActivity.demoSpust B307): assety, start (C++ pusti i zvuk), plocha.
// Vraci bity introStart: 1 = zvuk prehrava C++ (OpenSL ES), 2 = zvuk hry je ve stope.
int intro_start(const char *assety) {
  setenv("EGL_PLATFORM", "surfaceless", 1);
  if (assety && *assety) g_assety = assety;
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetArrayLength = fGetArrayLength; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.NewGlobalRef = fNewGlobalRef; g_tbl.DeleteGlobalRef = fDeleteGlobalRef; g_tbl.NewStringUTF = fNewStringUTF;
  g_env.functions = &g_tbl;
  Java_eu_atarihelp_emu10_NativeIntroBridge_introNastavAssety(&g_env, nullptr, (jobject)&g_am);
  const int st = Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(&g_env, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(&g_env, nullptr, (jobject)&g_plocha);
  return st;
}
// MainActivity.demoKonec (B307): introStop (C++ zastavi kresleni i zvuk), pak plocha pryc
void intro_stop() {
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStop(&g_env, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(&g_env, nullptr, nullptr);
}
int intro_pripraveno() { return Java_eu_atarihelp_emu10_NativeIntroBridge_introPripraveno(&g_env, nullptr) == JNI_TRUE; }
int intro_hotovo() { return Java_eu_atarihelp_emu10_NativeIntroBridge_introHotovo(&g_env, nullptr) == JNI_TRUE; }
int intro_cas_ms() { return Java_eu_atarihelp_emu10_NativeIntroBridge_introCasMs(&g_env, nullptr); }
long long intro_snimku() { return Java_eu_atarihelp_emu10_NativeIntroBridge_introSnimku(&g_env, nullptr); }
long long intro_prehrano() { return karta::prehrano.load(); }            // ramcu, co "reproduktor" prehral
long long intro_zacatek() { return g_zacatekStopy.load(); }             // od ktereho ramce hraje stopa
int intro_hracu() { return karta::vytvoreno.load() * 100 + karta::zniceno.load(); }
int intro_okno_refs() { return g_okno.refs; }
// B307: dotyk (NapDemoView.onTouchEvent -> introDotyk): 0 nic, 1 SKIP, 2 jinam
int intro_dotyk(float x, float y, int w, int h) { return Java_eu_atarihelp_emu10_NativeIntroBridge_introDotyk(&g_env, nullptr, x, y, w, h); }
int intro_klip_obraz() { return g_klipObraz.load() ? 1 : 0; }
const char *intro_info() { thread_local std::string r; r = jstr(Java_eu_atarihelp_emu10_NativeIntroBridge_introInfo(&g_env, nullptr)); return r.c_str(); }
// referencni stopa (samostatne spocitana) - pro porovnani bit po bitu
int intro_reference(int16_t *out, int maxRamcu) {
  napdemo::zv::Stopa st;
  referencniStopa(st, g_assety);
  const int n = std::min(maxRamcu, st.celkem());
  std::memcpy(out, st.data(), (size_t)n * 4);
  return n;
}
// /intro.stream -> proudOtevri / Dalsi / Zavri (Java napTvWebJadroProud)
int intro_otevri() { return Java_eu_atarihelp_emu10_NativeIntroBridge_proudOtevriNative(&g_env, nullptr); }
int intro_dalsi(int id, uint8_t *out, int kap, int cekejMs, int zapisMs) {
  thread_local FakeArr a;
  if ((int)a.b.size() != kap) a.b.assign((size_t)kap, 0);
  const jint n = Java_eu_atarihelp_emu10_NativeIntroBridge_proudDalsiNative(&g_env, nullptr, id, (jbyteArray)&a, cekejMs, zapisMs);
  if (n > 0) std::memcpy(out, a.b.data(), (size_t)n);
  return n;
}
void intro_zavri(int id) { Java_eu_atarihelp_emu10_NativeIntroBridge_proudZavriNative(&g_env, nullptr, id); }
int intro_stav() { return Java_eu_atarihelp_emu10_NativeIntroBridge_proudStavNative(&g_env, nullptr); }
const char *intro_log() { thread_local std::string r; r = jstr(Java_eu_atarihelp_emu10_NativeIntroBridge_proudLogNative(&g_env, nullptr)); return r.c_str(); }
}

// JVM test (jvm/TestPodpisu.java): knihovna nactena skutecnou tridou NativeIntroBridge
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
  if (getenv("NAP_JVM_INTRO")) setenv("EGL_PLATFORM", "surfaceless", 1);
  return JNI_VERSION_1_6;
}
