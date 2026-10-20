// B311: zaklad pro test HLAVNIHO MENU (skutecny nap_menu_gl.cpp) na PC.
//
// Stejny princip jako ../b307/tv_intro.cpp: plocha telefonu = pbuffer pres Mesa,
// assety ze souboru (app/src/main/assets), JNIEnv sestaveny rucne. Funkce
// menu_* nize volaji presne ty JNI funkce, ktere vola Java (NativeMenuBridge).
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <jni.h>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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

#include "../../app/src/main/cpp/menu/nap_menu_gl.cpp"

// ---------- rucne sestaveny JNIEnv ----------
struct FakeArr { std::vector<uint8_t> b; };
struct FakeIntArr { std::vector<jint> b; };
struct FakeStr { std::string s; };
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, (size_t)n); }
static jobject JNICALL fNewGlobalRef(JNIEnv *, jobject o) { return o; }
static void JNICALL fDeleteGlobalRef(JNIEnv *, jobject) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
// int[]: menuGrabFrame - pole "FakeIntArr" (delka v GetArrayLength se pozna podle znacky)
static jint *JNICALL fGetIntArrayElements(JNIEnv *, jintArray a, jboolean *) { return ((FakeIntArr *)a)->b.data(); }
static void JNICALL fReleaseIntArrayElements(JNIEnv *, jintArray, jint *, jint) {}
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static bool g_intPole = false;    // GetArrayLength: int[] (menuGrabFrame) nebo byte[] (proud)
static jsize JNICALL fGetArrayLength2(JNIEnv *e, jarray a) { return g_intPole ? (jsize)((FakeIntArr *)a)->b.size() : fGetArrayLength(e, a); }
static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }
static FakeStr *jnew(const std::string &s) { return new FakeStr{s}; }
static int g_am = 1, g_plocha = 2;

static void pripravEnv() {
  setenv("EGL_PLATFORM", "surfaceless", 1);
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetArrayLength = fGetArrayLength2; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.NewGlobalRef = fNewGlobalRef; g_tbl.DeleteGlobalRef = fDeleteGlobalRef; g_tbl.NewStringUTF = fNewStringUTF;
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.GetIntArrayElements = fGetIntArrayElements; g_tbl.ReleaseIntArrayElements = fReleaseIntArrayElements;
  g_env.functions = &g_tbl;
}

// ---- volani jako z Javy (NativeMenuBridge) ----
static int menu_assety(const char *assety) {
  if (assety && *assety) g_assety = assety;
  return Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastavAssety(&g_env, nullptr, (jobject)&g_am);
}
static std::string menu_karta(int i) { return jstr(Java_eu_atarihelp_emu10_NativeMenuBridge_menuKarta(&g_env, nullptr, i)); }
static void menu_zamek(const std::string &klic, bool z, const std::string &text) {
  FakeStr *a = jnew(klic), *b = jnew(text);
  Java_eu_atarihelp_emu10_NativeMenuBridge_menuZamek(&g_env, nullptr, (jstring)a, z ? JNI_TRUE : JNI_FALSE, (jstring)b);
  delete a; delete b;
}
static void menu_plocha(bool zapnout) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuPlocha(&g_env, nullptr, zapnout ? (jobject)&g_plocha : nullptr); }
static void menu_ukaz(int jak) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuUkaz(&g_env, nullptr, jak); }
static void menu_prah(float px) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastavPrah(&g_env, nullptr, px); }
static int menu_dotyk(int akce, float x, float y, long long casMs, int w, int h) {
  return Java_eu_atarihelp_emu10_NativeMenuBridge_menuDotyk(&g_env, nullptr, akce, x, y, (jlong)casMs, w, h);
}
static int menu_klavesa(int k) { return Java_eu_atarihelp_emu10_NativeMenuBridge_menuKlavesa(&g_env, nullptr, k); }
static int menu_vybrana() { return Java_eu_atarihelp_emu10_NativeMenuBridge_menuVybrana(&g_env, nullptr); }
static void menu_skoc(int i) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuSkoc(&g_env, nullptr, i); }
static bool menu_pripraveno() { return Java_eu_atarihelp_emu10_NativeMenuBridge_menuPripraveno(&g_env, nullptr) == JNI_TRUE; }
static bool menu_chyba() { return Java_eu_atarihelp_emu10_NativeMenuBridge_menuChyba(&g_env, nullptr) == JNI_TRUE; }
static long long menu_snimku() { return Java_eu_atarihelp_emu10_NativeMenuBridge_menuSnimku(&g_env, nullptr); }
static std::string menu_info() { return jstr(Java_eu_atarihelp_emu10_NativeMenuBridge_menuInfo(&g_env, nullptr)); }
static std::string menu_log() { return jstr(Java_eu_atarihelp_emu10_NativeMenuBridge_menuLog(&g_env, nullptr)); }
static int proud_otevri() { return Java_eu_atarihelp_emu10_NativeMenuBridge_proudOtevriNative(&g_env, nullptr); }
static int proud_dalsi(int id, std::vector<uint8_t> &b, int cekej) {
  FakeArr a; a.b.assign(b.size(), 0);
  g_intPole = false;
  const int n = Java_eu_atarihelp_emu10_NativeMenuBridge_proudDalsiNative(&g_env, nullptr, id, (jbyteArray)&a, cekej, 0);
  if (n > 0) b.assign(a.b.begin(), a.b.begin() + n);
  return n;
}
static void proud_zavri(int id) { Java_eu_atarihelp_emu10_NativeMenuBridge_proudZavriNative(&g_env, nullptr, id); }
static int proud_stav() { return Java_eu_atarihelp_emu10_NativeMenuBridge_proudStavNative(&g_env, nullptr); }
static std::string proud_log() { return jstr(Java_eu_atarihelp_emu10_NativeMenuBridge_proudLogNative(&g_env, nullptr)); }
static int menu_grab(std::vector<jint> &px) {
  FakeIntArr a; a.b.assign(px.size(), 0);
  g_intPole = true;
  const int r = Java_eu_atarihelp_emu10_NativeMenuBridge_menuGrabFrame(&g_env, nullptr, (jintArray)&a);
  g_intPole = false;
  if (r > 0) px = a.b;
  return r;
}
