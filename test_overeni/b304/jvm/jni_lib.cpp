// B304: SKUTECNY nap_intro_gl.cpp jako knihovna pro JVM na PC (test JNI podpisu):
// Java trida NativeIntroBridge z appky ji nacte pres System.loadLibrary("napintro")
// a zavola vsechny sve native metody - kdyby se nejaky nazev nebo typ nelisil,
// spadne to tady (UnsatisfiedLinkError), ne az na telefonu.
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <jni.h>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>
#include "android/native_window.h"
struct ANativeWindow { int w = 320, h = 640; };
void ANativeWindow_acquire(ANativeWindow *) {}
void ANativeWindow_release(ANativeWindow *) {}
int32_t ANativeWindow_getWidth(ANativeWindow *w) { return w->w; }
int32_t ANativeWindow_getHeight(ANativeWindow *w) { return w->h; }
int32_t ANativeWindow_getFormat(ANativeWindow *) { return 1; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t) { return 0; }
int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *) { return -1; }
int32_t ANativeWindow_unlockAndPost(ANativeWindow *) { return 0; }
static ANativeWindow g_okno;
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject s) { return s ? &g_okno : nullptr; }
#undef EGL_WINDOW_BIT
#define EGL_WINDOW_BIT EGL_PBUFFER_BIT
static EGLSurface testPlocha(EGLDisplay d, EGLConfig c, ANativeWindow *w) {
  const EGLint a[] = {EGL_WIDTH, w->w, EGL_HEIGHT, w->h, EGL_NONE};
  return eglCreatePbufferSurface(d, c, a);
}
#define eglCreateWindowSurface(d, c, w, a) testPlocha(d, c, (ANativeWindow *)(w))
#include "android/asset_manager.h"
struct AAssetManager { int x; };
struct AAsset { std::vector<char> d; };
AAssetManager *AAssetManager_fromJava(JNIEnv *, jobject) { static AAssetManager m; return &m; }
AAsset *AAssetManager_open(AAssetManager *, const char *jm, int) {
  std::ifstream f(std::string("../../../app/src/main/assets/") + jm, std::ios::binary);
  if (!f) return nullptr;
  AAsset *a = new AAsset;
  a->d.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  return a;
}
const void *AAsset_getBuffer(AAsset *a) { return a->d.data(); }
off_t AAsset_getLength(AAsset *a) { return (off_t)a->d.size(); }
void AAsset_close(AAsset *a) { delete a; }
#include "../../../app/src/main/cpp/intro/nap_intro_gl.cpp"
