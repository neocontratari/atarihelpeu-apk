// nap_intro_gl.cpp
// ====================================================================
//  B304: UVODNI DEMO N&P - most mezi Androidem a C++ demem.
//
//  - vlastni vlakno s OpenGL ES 2.0 (EGL) kresli demo primo do plochy
//    (SurfaceView NapDemoView) - zadne WebView, zadne video
//  - obraz pro TV / PC se kresli zvlast v 16:9 (854 x 480) kazdy druhy
//    snimek a Java si ho vyzvedne (introGrabFrame) - telefon na vysku,
//    televize na sirku, obe se svou kompozici
//  - zvuk: cela stopa se spocita dopredu ve vlakne (nap_demo_zvuk.h),
//    Java ji prehrava pres AudioTrack a zaroven posila na TV
//  - cas obrazu se ridi podle toho, co reproduktor UZ PREHRAL
//    (AudioTrack.getPlaybackHeadPosition), takze obraz a zvuk sedi
// ====================================================================
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <string>
#include <mutex>
#include <thread>
#include <vector>

#define NAPDEMO_LOG(...) __android_log_print(ANDROID_LOG_INFO, "NAP_DEMO", __VA_ARGS__)

// stb_truetype: implementace jen tady (staticka - knihovna napintro je samostatna)
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "../vendor/stb/stb_image.h"

#include "nap_demo.h"
#include "nap_demo_zvuk.h"

namespace {

using hodiny = std::chrono::steady_clock;
inline long long tedNs() { return std::chrono::duration_cast<std::chrono::nanoseconds>(hodiny::now().time_since_epoch()).count(); }

// ---- prostredky ----
jobject g_assetsRef = nullptr;            // Java AssetManager (globalni reference, at zije)
AAssetManager *g_assets = nullptr;

// ---- obraz ----
std::mutex g_oknoZamek;
std::thread g_vlakno;
std::atomic<bool> g_bezi{false};
std::atomic<bool> g_pripraveno{false};    // shadery prelozene, demo muze zacit
std::atomic<bool> g_chyba{false};         // GL se nepodarilo - Java demo hned ukonci
std::atomic<bool> g_hotovo{false};
std::atomic<long long> g_snimku{0};
std::atomic<int> g_casMs{0};

// ---- zvuk ----
std::unique_ptr<napdemo::zv::Stopa> g_stopa;
std::thread g_zvukVlakno;
std::mutex g_zvukZamek;
std::atomic<long long> g_zapsano{0};        // ramcu predanych Jave (vc. ticha na zacatku)
std::atomic<long long> g_zacatekStopy{-1};  // od ktereho zapsaneho ramce hraje stopa
std::atomic<long long> g_hlava{0};          // co reproduktor uz prehral (ramce)
std::atomic<long long> g_hlavaNs{0};
std::atomic<long long> g_pripravenoNs{0};

// ---- informace do logu appky (Java si je vyzvedne) ----
std::mutex g_infoZamek;
std::string g_infoGl, g_infoBeh;

// ---- TV ----
std::mutex g_tvZamek;
std::vector<unsigned char> g_tvBody;
int g_tvW = 0, g_tvH = 0;
bool g_tvNovy = false;
std::atomic<long long> g_tvChceNs{0};
constexpr int TV_W = 854, TV_H = 480;

// Cas dema podle zvuku. Dokud reproduktor nezacal hrat stopu, stoji na 0
// (prvni ctvrt vteriny dema je stejne tma). Kdyz zvuk nejde, bezi podle hodin.
double cilovyCas() {
  const long long ted = tedNs();
  const long long zac = g_zacatekStopy.load();
  const long long hl = g_hlava.load(), hlNs = g_hlavaNs.load();
  const long long prip = g_pripravenoNs.load();
  if (zac >= 0 && hlNs > 0 && hl > zac) {
    double t = (double)(hl - zac) / 44100.0 + std::min(0.25, (double)(ted - hlNs) * 1e-9);
    return t;
  }
  if (prip > 0 && (ted - prip) > 900000000LL && (zac < 0 || hlNs == 0 || ted - hlNs > 600000000LL)) {
    // zvuk nehraje (nebo vypadl) - hodiny od chvile, kdy bylo demo pripravene
    return (double)(ted - prip - 900000000LL) * 1e-9;
  }
  return 0.0;
}

std::vector<unsigned char> nactiLogo(int &w, int &h) {
  std::vector<unsigned char> ven;
  w = h = 0;
  if (!g_assets) { NAPDEMO_LOG("DEMO: chybi AssetManager - logo nebude"); return ven; }
  AAsset *a = AAssetManager_open(g_assets, "design/nap_logo_original.png", AASSET_MODE_BUFFER);
  if (!a) { NAPDEMO_LOG("DEMO: logo design/nap_logo_original.png nenalezeno"); return ven; }
  const void *d = AAsset_getBuffer(a);
  const off_t n = AAsset_getLength(a);
  int c = 0;
  unsigned char *px = d ? stbi_load_from_memory((const stbi_uc *)d, (int)n, &w, &h, &c, 4) : nullptr;
  AAsset_close(a);
  if (!px) { NAPDEMO_LOG("DEMO: logo se nepodarilo dekodovat"); w = h = 0; return ven; }
  ven.assign(px, px + (size_t)w * h * 4);
  stbi_image_free(px);
  return ven;
}

EGLConfig vyberKonfiguraci(EGLDisplay dpy) {
  // nepruhledna plocha (bez alfy): demo lezi nad strankou a nic nesmi prosvitat
  const EGLint a[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
                      EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
  EGLint n = 0;
  eglChooseConfig(dpy, a, nullptr, 0, &n);
  if (n <= 0) return nullptr;
  std::vector<EGLConfig> v(n);
  eglChooseConfig(dpy, a, v.data(), n, &n);
  EGLConfig nej = v[0];
  int nejBody = -1;
  for (int i = 0; i < n; i++) {
    EGLint r = 0, g = 0, b = 0, al = 0, d = 0, s = 0;
    eglGetConfigAttrib(dpy, v[i], EGL_RED_SIZE, &r);
    eglGetConfigAttrib(dpy, v[i], EGL_GREEN_SIZE, &g);
    eglGetConfigAttrib(dpy, v[i], EGL_BLUE_SIZE, &b);
    eglGetConfigAttrib(dpy, v[i], EGL_ALPHA_SIZE, &al);
    eglGetConfigAttrib(dpy, v[i], EGL_DEPTH_SIZE, &d);
    eglGetConfigAttrib(dpy, v[i], EGL_STENCIL_SIZE, &s);
    int body = 0;
    if (r == 8 && g == 8 && b == 8) body += 100;
    if (al == 0) body += 20;
    if (d == 0) body += 5;
    if (s == 0) body += 2;
    if (body > nejBody) { nejBody = body; nej = v[i]; }
  }
  return nej;
}

void vlaknoDema(ANativeWindow *okno) {
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLint ma = 0, mi = 0;
  if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, &ma, &mi)) {
    NAPDEMO_LOG("DEMO: EGL se nepodarilo otevrit");
    g_chyba.store(true);
    ANativeWindow_release(okno);
    return;
  }
  EGLConfig cfg = vyberKonfiguraci(dpy);
  if (!cfg) {
    NAPDEMO_LOG("DEMO: zadna konfigurace EGL");
    g_chyba.store(true);
    ANativeWindow_release(okno);
    return;
  }
  EGLint fmt = 0;
  eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_ID, &fmt);
  ANativeWindow_setBuffersGeometry(okno, 0, 0, fmt);
  EGLSurface surf = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)okno, nullptr);
  const EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLContext ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa);
  if (surf == EGL_NO_SURFACE || ctx == EGL_NO_CONTEXT || !eglMakeCurrent(dpy, surf, surf, ctx)) {
    NAPDEMO_LOG("DEMO: kontext se nepodarilo pripravit (0x%x)", eglGetError());
    g_chyba.store(true);
    if (ctx != EGL_NO_CONTEXT) eglDestroyContext(dpy, ctx);
    if (surf != EGL_NO_SURFACE) eglDestroySurface(dpy, surf);
    ANativeWindow_release(okno);
    return;
  }
  eglSwapInterval(dpy, 1);
  NAPDEMO_LOG("DEMO: GL %s | %s", (const char *)glGetString(GL_VERSION), (const char *)glGetString(GL_RENDERER));
  {
    const char *ver = (const char *)glGetString(GL_VERSION), *ren = (const char *)glGetString(GL_RENDERER);
    std::lock_guard<std::mutex> z(g_infoZamek);
    g_infoGl = std::string("GPU ") + (ren ? ren : "?") + " | " + (ver ? ver : "?");
    g_infoBeh.clear();
  }

  {
    std::unique_ptr<napdemo::Demo> demo(new napdemo::Demo());
    int lw = 0, lh = 0;
    std::vector<unsigned char> logo = nactiLogo(lw, lh);
    const long long t0 = tedNs();
    bool ok = demo->init(logo.empty() ? nullptr : logo.data(), lw, lh);
    logo.clear(); logo.shrink_to_fit();
    napdemo::Cil cil, tv;
    tv.w = TV_W; tv.h = TV_H; tv.tv = true;
    napdemo::Fbo tvFbo;
    std::vector<unsigned char> tvBuf;
    if (ok) {
      // texty pro telefon vysadit hned (pak uz se jen kresli)
      EGLint w = 0, h = 0;
      eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
      eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
      cil.w = w; cil.h = h;
      if (w > 0 && h > 0) demo->pripravTexty(cil, 100);
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, w, h);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      eglSwapBuffers(dpy, surf);
      NAPDEMO_LOG("DEMO: pripraveno za %lld ms (%dx%d)", (tedNs() - t0) / 1000000LL, w, h);
      {
        char b[160];
        std::snprintf(b, sizeof b, " | priprava %lld ms, plocha %dx%d", (tedNs() - t0) / 1000000LL, (int)w, (int)h);
        std::lock_guard<std::mutex> z(g_infoZamek);
        g_infoGl += b;
      }
      if (g_pripravenoNs.load() == 0) g_pripravenoNs.store(tedNs());
      g_pripraveno.store(true);
    } else {
      NAPDEMO_LOG("DEMO: priprava selhala - demo se preskoci");
      g_chyba.store(true);
    }

    double tVid = 0.0;
    long long predNs = tedNs();
    float prumer = 16.7f;
    int snimekCislo = 0, hlaseni = 0;
    while (ok && g_bezi.load()) {
      EGLint w = 0, h = 0;
      eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
      eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
      if (w <= 0 || h <= 0) { std::this_thread::sleep_for(std::chrono::milliseconds(16)); continue; }
      cil.w = w; cil.h = h; cil.tv = false;

      // cas: plynule za zvukem (mala odchylka se dorovna, velka skoci)
      const long long ted = tedNs();
      const double dt = std::min(0.25, (double)(ted - predNs) * 1e-9);
      predNs = ted;
      const double cilT = cilovyCas();
      tVid += dt;
      const double chyba = cilT - tVid;
      // dorovnani s casovou konstantou ~0,2 s (stejne rychle pri 60 i pri 10 snimcich/s)
      if (std::fabs(chyba) > 0.3 || cilT <= 0.0) tVid = cilT;
      else tVid += chyba * std::min(1.0, dt * 5.0);
      if (tVid < 0.0) tVid = 0.0;
      const double t = std::min(tVid, (double)napdemo::DELKA);
      g_casMs.store((int)(t * 1000.0));

      demo->snimek(t, cil, 0);
      // alfa plochy vzdy 1 (kdyby EGL dalo plochu s alfou, menu pod demem nesmi prosvitat)
      glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
      eglSwapBuffers(dpy, surf);
      g_snimku.fetch_add(1);
      snimekCislo++;

      // TV / PC: vlastni obraz 16:9, kazdy druhy snimek, jen kdyz si ho nekdo bere
      if ((snimekCislo & 1) == 0 && ted - g_tvChceNs.load() < 2000000000LL) {
        tvFbo.priprav(TV_W, TV_H, false);
        demo->snimek(t, tv, tvFbo.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tvFbo.fbo);
        tvBuf.resize((size_t)TV_W * TV_H * 4);
        glReadPixels(0, 0, TV_W, TV_H, GL_RGBA, GL_UNSIGNED_BYTE, tvBuf.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::lock_guard<std::mutex> z(g_tvZamek);
        g_tvBody.swap(tvBuf);
        g_tvW = TV_W; g_tvH = TV_H; g_tvNovy = true;
      }

      // rychlost GPU: tezke vrstvy (mraky, garaz) v mensim rozliseni, kdyz to nestiha
      const float ms = (float)((tedNs() - ted) / 1000000LL);
      prumer = prumer * 0.95f + ms * 0.05f;
      if (snimekCislo % 30 == 0) {
        if (prumer > 24.f && cil.meritko > 0.34f) cil.meritko -= 0.05f;
        else if (prumer < 13.f && cil.meritko < 0.62f) cil.meritko += 0.04f;
        tv.meritko = std::max(0.5f, cil.meritko);
        if (++hlaseni % 10 == 1)
          NAPDEMO_LOG("DEMO t=%.2f s snimek %.1f ms meritko %.2f (%dx%d)", t, prumer, cil.meritko, w, h);
        char b[220];
        std::snprintf(b, sizeof b, "t=%.1f s, snimek %.1f ms, mraky/garaz %.0f %%, plocha %dx%d, zvuk %s, odchylka obrazu %.0f ms",
                      t, prumer, cil.meritko * 100.f, (int)w, (int)h,
                      (g_zacatekStopy.load() >= 0 && g_hlavaNs.load() > 0) ? "ridi cas" : "nehraje (hodiny)", chyba * 1000.0);
        std::lock_guard<std::mutex> z(g_infoZamek);
        g_infoBeh = b;
      }
      if (t >= napdemo::DELKA && !g_hotovo.load()) {
        g_hotovo.store(true);
        NAPDEMO_LOG("DEMO: dobehlo (%lld snimku)", (long long)g_snimku.load());
      }
    }
    tvFbo.uvolni();
    // demo (GL objekty) se uvolni s kontextem
  }
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(dpy, surf);
  eglDestroyContext(dpy, ctx);
  ANativeWindow_release(okno);
  NAPDEMO_LOG("DEMO: plocha uvolnena");
}

void zastavKresleni() {
  g_bezi.store(false);
  if (g_vlakno.joinable()) g_vlakno.join();
}

void zastavZvuk() {
  std::lock_guard<std::mutex> z(g_zvukZamek);
  if (g_stopa) g_stopa->zastav();
  if (g_zvukVlakno.joinable()) g_zvukVlakno.join();
  g_stopa.reset();
}

}  // namespace

// ====================================================================
//  JNI
// ====================================================================
extern "C" {

JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introNastavAssety(JNIEnv *env, jclass, jobject am) {
  if (!am) return;
  if (g_assetsRef) env->DeleteGlobalRef(g_assetsRef);
  g_assetsRef = env->NewGlobalRef(am);
  g_assets = AAssetManager_fromJava(env, g_assetsRef);
}

/** Novy beh dema: vynulovat stav a zacit pocitat zvuk. */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(JNIEnv *, jclass) {
  zastavKresleni();
  zastavZvuk();
  g_pripraveno.store(false);
  g_chyba.store(false);
  g_hotovo.store(false);
  g_snimku.store(0);
  g_casMs.store(0);
  g_zapsano.store(0);
  g_zacatekStopy.store(-1);
  g_hlava.store(0);
  g_hlavaNs.store(0);
  g_pripravenoNs.store(0);
  {
    std::lock_guard<std::mutex> z(g_tvZamek);
    g_tvNovy = false;
  }
  {
    std::lock_guard<std::mutex> z(g_zvukZamek);
    g_stopa.reset(new napdemo::zv::Stopa());
    napdemo::zv::Stopa *s = g_stopa.get();
    g_zvukVlakno = std::thread([s] {
      const long long t0 = tedNs();
      s->vyrob();
      NAPDEMO_LOG("DEMO: zvuk spocitan za %lld ms", (tedNs() - t0) / 1000000LL);
    });
  }
  NAPDEMO_LOG("DEMO START");
}

/** Konec (dobehlo, preskoceno, pauza): zastavit kresleni i zvuk. */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introStop(JNIEnv *, jclass) {
  zastavKresleni();
  zastavZvuk();
  g_pripraveno.store(false);
  {
    std::lock_guard<std::mutex> z(g_tvZamek);
    g_tvNovy = false;
    std::vector<unsigned char>().swap(g_tvBody);
  }
  NAPDEMO_LOG("DEMO STOP");
}

JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(JNIEnv *env, jclass, jobject jsurf) {
  std::lock_guard<std::mutex> z(g_oknoZamek);
  zastavKresleni();
  if (!jsurf) { NAPDEMO_LOG("DEMO: plocha odpojena"); return; }
  ANativeWindow *okno = ANativeWindow_fromSurface(env, jsurf);
  if (!okno) { NAPDEMO_LOG("DEMO: okno se nepodarilo ziskat"); g_chyba.store(true); return; }
  g_bezi.store(true);
  g_vlakno = std::thread(vlaknoDema, okno);
}

JNIEXPORT jboolean JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introHotovo(JNIEnv *, jclass) {
  return (g_hotovo.load() || g_chyba.load()) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introPripraveno(JNIEnv *, jclass) {
  return g_pripraveno.load() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introSnimku(JNIEnv *, jclass) {
  return (jlong)g_snimku.load();
}

JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introCasMs(JNIEnv *, jclass) {
  return (jint)g_casMs.load();
}

/** Kratka zprava do logu appky: GPU, priprava, rychlost kresleni, zvuk. */
JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introInfo(JNIEnv *env, jclass) {
  std::string s;
  {
    std::lock_guard<std::mutex> z(g_infoZamek);
    s = g_infoGl + " || " + g_infoBeh;
  }
  return env->NewStringUTF(s.c_str());
}

/**
 * Java si sem chodi pro vzorky (stereo, 44,1 kHz). Dokud neni obraz
 * pripraveny, dostava ticho; pak stopu od zacatku. Vraci pocet ramcu,
 * -1 = konec (demo zastavene nebo cela stopa prehrana).
 */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introNaplnZvuk(JNIEnv *env, jclass, jshortArray jbuf, jint ramcu) {
  if (!jbuf || ramcu <= 0) return 0;
  if (env->GetArrayLength(jbuf) < ramcu * 2) return 0;
  jshort *p = env->GetShortArrayElements(jbuf, nullptr);
  if (!p) return 0;
  std::memset(p, 0, sizeof(jshort) * (size_t)ramcu * 2);
  const long long zapsano = g_zapsano.load();
  bool konec = false;
  {
    std::lock_guard<std::mutex> z(g_zvukZamek);
    if (!g_stopa) konec = true;                       // demo uz skoncilo
    else if (g_pripraveno.load() || g_zacatekStopy.load() >= 0) {
      if (g_zacatekStopy.load() < 0) g_zacatekStopy.store(zapsano);
      const long long pos = zapsano - g_zacatekStopy.load();
      const long long hotovo = g_stopa->hotovo();
      if (pos >= g_stopa->celkem()) konec = true;     // cela stopa uz je venku
      const long long k = std::max(0LL, std::min((long long)ramcu, hotovo - pos));
      if (k > 0) std::memcpy(p, g_stopa->data() + pos * 2, sizeof(jshort) * (size_t)k * 2);
    }
  }
  env->ReleaseShortArrayElements(jbuf, p, 0);
  if (konec) return -1;
  g_zapsano.fetch_add(ramcu);
  return ramcu;
}

/** Kolik ramcu uz reproduktor opravdu prehral (AudioTrack.getPlaybackHeadPosition). */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introCasZvuku(JNIEnv *, jclass, jlong hlava) {
  if (hlava != g_hlava.load()) {
    g_hlava.store((long long)hlava);
    g_hlavaNs.store(tedNs());
  } else if (g_hlavaNs.load() == 0) {
    g_hlavaNs.store(tedNs());
  }
}

/**
 * Posledni snimek pro TV (16:9) - stejny tvar jako Ps1GlTextureView.borrowFrame:
 *   (sirka<<16)|vyska kdyz predal, zaporne kdyz je pole male, 0 kdyz nic noveho.
 */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introGrabFrame(JNIEnv *env, jclass, jintArray jven) {
  g_tvChceNs.store(tedNs());
  std::lock_guard<std::mutex> z(g_tvZamek);
  if (!g_tvNovy || g_tvW <= 0 || g_tvH <= 0) return 0;
  const int potreba = g_tvW * g_tvH;
  if (!jven || env->GetArrayLength(jven) < potreba) return -((g_tvW << 16) | g_tvH);
  jint *ven = env->GetIntArrayElements(jven, nullptr);
  if (!ven) return 0;
  for (int y = 0; y < g_tvH; y++) {               // OpenGL cte zdola nahoru; RGBA -> ARGB
    const unsigned char *r = &g_tvBody[(size_t)(g_tvH - 1 - y) * g_tvW * 4];
    jint *w = ven + (size_t)y * g_tvW;
    for (int x = 0; x < g_tvW; x++)
      w[x] = (jint)(0xFF000000u | ((unsigned)r[x * 4] << 16) | ((unsigned)r[x * 4 + 1] << 8) | (unsigned)r[x * 4 + 2]);
  }
  env->ReleaseIntArrayElements(jven, ven, 0);
  g_tvNovy = false;
  return (jint)((g_tvW << 16) | g_tvH);
}

}  // extern "C"
