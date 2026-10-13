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
//    B306: prehrava ji C++ pres OpenSL ES (drive Java AudioTrack; ten zustal
//    jen jako zaloha, kdyby OpenSL na telefonu nesel)
//  - cas obrazu se ridi podle toho, co reproduktor UZ PREHRAL
//    (OpenSL GetPosition, v zaloze AudioTrack.getPlaybackHeadPosition),
//    takze obraz a zvuk sedi
//  - B306: na TV / PC jde demo proudem z C++ (/intro.stream, jako Sega a PS1
//    v B305): obraz 16:9 jako JPEG (nap_tv_proud_jpeg.h) + zvuk, ktery
//    reproduktor prave dohral, v jednom paketu - Java jen predava bajty
//  - B307: zvuk startuje a konci C++ samo (introStart / introStop) - zalozni
//    Java AudioTrack je pryc (kdyz OpenSL nejde, demo bezi podle hodin bez
//    zvuku). Dotyk: Java jen posle souradnice (introDotyk) a C++ rozhodne,
//    jestli to bylo tlacitko SKIP (kreslene v C++) nebo jinam. Zabery
//    z Reneho hry Pitt-Kitt Platinum (asset intro/pittkitt.klip) nacita
//    C++: obraz pro CRT v demu, zvuk hry primichany do stopy.
//    Java v intru zbyla: plocha (SurfaceView), predani dotyku, start / stop
//    a preposilani hotovych bajtu proudu na TV (+ stara cesta introGrabFrame
//    jen pro prohlizece bez ReadableStream - snimek uz otoceny z C++).
// ====================================================================
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>

#include <climits>
#include <cmath>
#include <cstring>

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
#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"

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
std::atomic<long long> g_startNs{0};        // B307: kdy zacal beh (dotyk hned po startu se nepocita)
std::atomic<int> g_plochaW{0}, g_plochaH{0};   // B307: velikost plochy, do ktere C++ kresli (pro dotyk)
std::atomic<bool> g_klipObraz{false};       // B307: zabery z Pitt-Kitt nacteny (obraz)
std::atomic<bool> g_klipZvuk{false};        // B307: zvuk hry primichany do stopy

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

// ---- B306: TV / PC proudem z C++ (jako Sega / PS1 v B305) ----
// Obraz 16:9 (854x480) jako JPEG jen zmenenych casti, zvuk 44,1 kHz ve stejnem
// paketu. Zamerne "new" bez delete - vlakna prohlizecu nesmi pri ukonceni
// procesu narazit na zniceny objekt.
nap::proud::Hub<nap::proud::KodekJpeg> &g_introProud =
    *new nap::proud::Hub<nap::proud::KodekJpeg>("INTRO", napdemo::zv::SR, 16, 9, false);

// ---- B306: zvuk dema prehrava C++ (OpenSL ES) ----
namespace sl {
SLObjectItf engObj = nullptr, mixObj = nullptr, hracObj = nullptr;
SLEngineItf eng = nullptr;
SLPlayItf hrac = nullptr;
SLAndroidSimpleBufferQueueItf fronta = nullptr;
constexpr int BLOK = 1024;           // ramcu na blok (23 ms)
constexpr int BLOKU = 4;             // ve fronte ~93 ms (jako PS1)
int16_t bloky[BLOKU][BLOK * 2];
int dalsi = 0;
std::atomic<bool> bezi{false};
std::atomic<long long> bloku{0};
}  // namespace sl

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

// B307: cely asset do pameti (zabery z Reneho hry, ~120 kB)
std::vector<unsigned char> nactiAsset(const char *cesta) {
  std::vector<unsigned char> ven;
  if (!g_assets) return ven;
  AAsset *a = AAssetManager_open(g_assets, cesta, AASSET_MODE_BUFFER);
  if (!a) return ven;
  const void *d = AAsset_getBuffer(a);
  const off_t n = AAsset_getLength(a);
  if (d && n > 0) ven.assign((const unsigned char *)d, (const unsigned char *)d + n);
  AAsset_close(a);
  return ven;
}
constexpr const char *KLIP_ASSET = "intro/pittkitt.klip";

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
    if (ok) {
      // B307: zabery z Reneho hry pro CRT (bez nich se na CRT ukaze jen titulek hry)
      std::vector<unsigned char> klip = nactiAsset(KLIP_ASSET);
      const bool kOk = !klip.empty() && demo->nastavKlip(klip.data(), klip.size());
      g_klipObraz.store(kOk);
      NAPDEMO_LOG("DEMO: zabery Pitt-Kitt %s (%zu B)", kOk ? "nacteny" : "NENACTENY", klip.size());
    }
    napdemo::Cil cil, tv;
    tv.w = TV_W; tv.h = TV_H; tv.tv = true;
    napdemo::Fbo tvFbo;
    std::vector<unsigned char> tvBuf;
    std::vector<uint32_t> tvArgb;      // B306: snimek pro proud na TV / PC
    if (ok) {
      // texty pro telefon vysadit hned (pak uz se jen kresli)
      EGLint w = 0, h = 0;
      eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
      eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
      cil.w = w; cil.h = h;
      if (w > 0 && h > 0) demo->pripravTexty(cil, 100);
      // B307: "nanecisto" - par snimku sceny 4 do plochy BEZ zobrazeni (zadny
      // eglSwapBuffers, hned se prekryje cernou). Ovladac GPU si tak pripravi vsechny
      // shadery, textury a framebuffery uz ted, kdy je obrazovka cerna - jinak by
      // to mohl delat az pri prvnim pouziti (strih na 32 bit, konzole, CRT) a obraz
      // by se tam na chvilku zadrhl.
      const long long tn = tedNs();
      int nanecisto = 0;
      if (w > 0 && h > 0) {
        static const float CASY[] = {28.4f, 31.0f, 33.6f, 34.8f, 36.0f};
        for (float tt : CASY) {
          if (!g_bezi.load()) break;
          demo->snimek(tt, cil, 0);
          nanecisto++;
        }
        glFinish();
      }
      const long long nanecistoMs = (tedNs() - tn) / 1000000LL;
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      glViewport(0, 0, w, h);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      eglSwapBuffers(dpy, surf);
      NAPDEMO_LOG("DEMO: pripraveno za %lld ms (%dx%d), z toho nanecisto %d snimku %lld ms", (tedNs() - t0) / 1000000LL, w, h,
                  nanecisto, nanecistoMs);
      {
        char b[240];
        std::snprintf(b, sizeof b, " | priprava %lld ms (nanecisto %d snimku %lld ms), plocha %dx%d, zabery Pitt-Kitt %s, zvuk hry %s",
                      (tedNs() - t0) / 1000000LL, nanecisto, nanecistoMs, (int)w, (int)h,
                      g_klipObraz.load() ? "ano" : "NE", g_klipZvuk.load() ? "ano" : "NE");
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
      g_plochaW.store(w); g_plochaH.store(h);

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
      // (B306: proud z C++ - g_introProud; postaru Java - introGrabFrame)
      const bool tvProud = g_introProud.klientu() > 0;
      const bool tvJava = ted - g_tvChceNs.load() < 2000000000LL;
      if (!tvProud) g_introProud.snimek(nullptr, 0, 0);   // jen "demo bezi" (pro /status proud=intro)
      if ((snimekCislo & 1) == 0 && (tvProud || tvJava)) {
        tvFbo.priprav(TV_W, TV_H, false);
        demo->snimek(t, tv, tvFbo.fbo);
        glBindFramebuffer(GL_FRAMEBUFFER, tvFbo.fbo);
        tvBuf.resize((size_t)TV_W * TV_H * 4);
        glReadPixels(0, 0, TV_W, TV_H, GL_RGBA, GL_UNSIGNED_BYTE, tvBuf.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (tvProud) {
          // OpenGL cte zdola nahoru; RGBA -> 0x00RRGGBB shora dolu, a hned do prenosu
          tvArgb.resize((size_t)TV_W * TV_H);
          for (int y = 0; y < TV_H; y++) {
            const unsigned char *r = &tvBuf[(size_t)(TV_H - 1 - y) * TV_W * 4];
            uint32_t *w = &tvArgb[(size_t)y * TV_W];
            for (int x = 0; x < TV_W; x++, r += 4) w[x] = ((uint32_t)r[0] << 16) | ((uint32_t)r[1] << 8) | (uint32_t)r[2];
          }
          g_introProud.snimek(tvArgb.data(), TV_W, TV_H);
        }
        if (tvJava) {
          std::lock_guard<std::mutex> z(g_tvZamek);
          g_tvBody.swap(tvBuf);
          g_tvW = TV_W; g_tvH = TV_H; g_tvNovy = true;
        }
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

// Davka zvuku pro reproduktor (stereo, 44,1 kHz). Dokud neni obraz pripraveny,
// ticho; pak stopa od zacatku. Vraci pocet ramcu, -1 = konec (demo zastavene
// nebo cela stopa venku) - p je pak ticho.
int naplnZvuk(int16_t *p, int ramcu) {
  std::memset(p, 0, sizeof(int16_t) * (size_t)ramcu * 2);
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
      if (k > 0) std::memcpy(p, g_stopa->data() + pos * 2, sizeof(int16_t) * (size_t)k * 2);
    }
  }
  if (konec) return -1;
  g_zapsano.fetch_add(ramcu);
  return ramcu;
}

// Kolik ramcu uz reproduktor opravdu prehral - podle toho bezi obraz.
void casZvuku(long long hlava) {
  if (hlava != g_hlava.load()) {
    g_hlava.store(hlava);
    g_hlavaNs.store(tedNs());
  } else if (g_hlavaNs.load() == 0) {
    g_hlavaNs.store(tedNs());
  }
}

// ====================================================================
//  B306: ZVUK DEMA PREHRAVA C++ (OpenSL ES) - jako PS1 a Atari
//  Drive Java vlakno s AudioTrack: si chodilo do C++ pro davky a zpatky
//  hlasilo, kolik reproduktor prehral. Ted si OpenSL rekne o dalsi blok
//  primo tady (zpetne volani, ~23 ms), cas prehraneho zvuku da GetPosition
//  a blok, ktery se prave DOHRAL, jde na TV / PC - presne to, co je slyset
//  z telefonu, a obraz dema se ridi stejnym casem, takze na TV sedi spolu.
// ====================================================================
void slZpetneVolani(SLAndroidSimpleBufferQueueItf bq, void *) {
  int16_t *blk = sl::bloky[sl::dalsi];
  sl::dalsi = (sl::dalsi + 1) % sl::BLOKU;
  // 1) tenhle blok se prave dohral -> na TV / PC (kdyz se nikdo nediva, nic se nedeje)
  g_introProud.zvuk(blk, (size_t)sl::BLOK * 2);
  // 2) kolik reproduktor prehral (ms -> ramce)
  SLmillisecond ms = 0;
  if (sl::hrac && (*sl::hrac)->GetPosition(sl::hrac, &ms) == SL_RESULT_SUCCESS)
    casZvuku((long long)ms * napdemo::zv::SR / 1000LL);
  // 3) dalsi blok ze stopy (po konci ticho - cas dema bezi dal, Java demo ukonci)
  naplnZvuk(blk, sl::BLOK);
  sl::bloku.fetch_add(1);
  (*bq)->Enqueue(bq, blk, (SLuint32)(sizeof(int16_t) * sl::BLOK * 2));
}

void slZavri() {
  sl::bezi.store(false);
  if (sl::hracObj) { (*sl::hracObj)->Destroy(sl::hracObj); sl::hracObj = nullptr; }   // zastavi i zpetna volani
  if (sl::mixObj) { (*sl::mixObj)->Destroy(sl::mixObj); sl::mixObj = nullptr; }
  if (sl::engObj) { (*sl::engObj)->Destroy(sl::engObj); sl::engObj = nullptr; }
  sl::eng = nullptr; sl::hrac = nullptr; sl::fronta = nullptr;
}

bool slOtevri() {
  if (sl::bezi.load()) return true;
  auto chyba = [](const char *co) { NAPDEMO_LOG("DEMO: OpenSL %s nejde - demo pobezi bez zvuku (podle hodin)", co); slZavri(); return false; };
  if (slCreateEngine(&sl::engObj, 0, nullptr, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return chyba("engine");
  if ((*sl::engObj)->Realize(sl::engObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return chyba("Realize");
  if ((*sl::engObj)->GetInterface(sl::engObj, SL_IID_ENGINE, &sl::eng) != SL_RESULT_SUCCESS) return chyba("ENGINE");
  if ((*sl::eng)->CreateOutputMix(sl::eng, &sl::mixObj, 0, nullptr, nullptr) != SL_RESULT_SUCCESS) return chyba("OutputMix");
  if ((*sl::mixObj)->Realize(sl::mixObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return chyba("OutputMix Realize");
  SLDataLocator_AndroidSimpleBufferQueue loc = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, (SLuint32)sl::BLOKU};
  SLDataFormat_PCM fmt = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1, SL_PCMSAMPLEFORMAT_FIXED_16, SL_PCMSAMPLEFORMAT_FIXED_16,
                          SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT, SL_BYTEORDER_LITTLEENDIAN};
  SLDataSource src = {&loc, &fmt};
  SLDataLocator_OutputMix locMix = {SL_DATALOCATOR_OUTPUTMIX, sl::mixObj};
  SLDataSink snk = {&locMix, nullptr};
  const SLInterfaceID ids[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
  const SLboolean req[1] = {SL_BOOLEAN_TRUE};
  if ((*sl::eng)->CreateAudioPlayer(sl::eng, &sl::hracObj, &src, &snk, 1, ids, req) != SL_RESULT_SUCCESS) return chyba("AudioPlayer");
  if ((*sl::hracObj)->Realize(sl::hracObj, SL_BOOLEAN_FALSE) != SL_RESULT_SUCCESS) return chyba("AudioPlayer Realize");
  if ((*sl::hracObj)->GetInterface(sl::hracObj, SL_IID_PLAY, &sl::hrac) != SL_RESULT_SUCCESS) return chyba("PLAY");
  if ((*sl::hracObj)->GetInterface(sl::hracObj, SL_IID_ANDROIDSIMPLEBUFFERQUEUE, &sl::fronta) != SL_RESULT_SUCCESS) return chyba("BUFFERQUEUE");
  if ((*sl::fronta)->RegisterCallback(sl::fronta, slZpetneVolani, nullptr) != SL_RESULT_SUCCESS) return chyba("RegisterCallback");
  // nakopnout: celou frontu naplnit (zacatek je ticho, dokud obraz neni pripraveny)
  sl::dalsi = 0;
  sl::bloku.store(0);
  for (int i = 0; i < sl::BLOKU; i++) {
    naplnZvuk(sl::bloky[i], sl::BLOK);
    (*sl::fronta)->Enqueue(sl::fronta, sl::bloky[i], (SLuint32)sizeof(sl::bloky[i]));
  }
  sl::bezi.store(true);
  if ((*sl::hrac)->SetPlayState(sl::hrac, SL_PLAYSTATE_PLAYING) != SL_RESULT_SUCCESS) return chyba("PLAYING");
  NAPDEMO_LOG("DEMO: zvuk prehrava C++ (OpenSL ES 44100/2/i16, %d x %d ramcu)", sl::BLOKU, sl::BLOK);
  return true;
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

/**
 * Novy beh dema: vynulovat stav, zacit pocitat zvuk a (B307) rovnou ho pustit
 * pres OpenSL ES - do pripravy obrazu hraje ticho. Vraci bity:
 *   bit0 = zvuk hraje C++ (OpenSL ES), bit1 = zvuk Reneho hry je ve stope.
 */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(JNIEnv *, jclass) {
  zastavKresleni();
  slZavri();          // B306: prehravac z minuleho behu (jeho cas by nesedel s novym)
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
  g_startNs.store(tedNs());
  g_plochaW.store(0);
  g_plochaH.store(0);
  g_klipObraz.store(false);
  g_klipZvuk.store(false);
  {
    std::lock_guard<std::mutex> z(g_tvZamek);
    g_tvNovy = false;
  }
  {
    std::lock_guard<std::mutex> z(g_zvukZamek);
    g_stopa.reset(new napdemo::zv::Stopa());
    napdemo::zv::Stopa *s = g_stopa.get();
    // B307: zvuk z Reneho hry (jizda Las Vegas) - do stopy pred vypoctem
    {
      std::vector<unsigned char> d = nactiAsset(KLIP_ASSET);
      napdemo::KlipAtari k;
      std::vector<int16_t> z;
      if (!d.empty() && k.nacti(d.data(), d.size()) && k.vzorkovani == napdemo::zv::SR &&
          k.zvukUseku(2, 6, napdemo::cas::STRIH_16 - napdemo::cas::ATARI_JIZDA, z)) {
        s->nastavZaznam(z.data(), z.size(), napdemo::cas::ATARI_JIZDA, 1.8f);
        g_klipZvuk.store(true);
      } else {
        NAPDEMO_LOG("DEMO: zvuk Pitt-Kitt nenacten (asset %s, %zu B)", KLIP_ASSET, d.size());
      }
    }
    g_zvukVlakno = std::thread([s] {
      const long long t0 = tedNs();
      s->vyrob();
      NAPDEMO_LOG("DEMO: zvuk spocitan za %lld ms", (tedNs() - t0) / 1000000LL);
    });
  }
  const bool zvuk = slOtevri();
  NAPDEMO_LOG("DEMO START (zvuk %s, zvuk hry %s)", zvuk ? "C++ OpenSL ES" : "NEJDE - jen obraz", g_klipZvuk.load() ? "ano" : "ne");
  return (zvuk ? 1 : 0) | (g_klipZvuk.load() ? 2 : 0);
}

/** Konec (dobehlo, preskoceno, pauza): zastavit kresleni i zvuk. */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introStop(JNIEnv *, jclass) {
  zastavKresleni();
  slZavri();          // B307: zvuk zastavuje C++ samo (zadna Java)
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
 * B307: dotyk prstem (zvednuti) v bodech plochy NapDemoView. C++ rozhodne:
 *   0 = nic (hned po startu - prst, ktery appku spustil), 1 = tlacitko SKIP,
 *   2 = jinam (taky preskocit, jako drive).
 * Souradnice se prepoctou na velikost plochy, do ktere C++ opravdu kresli.
 */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_introDotyk(JNIEnv *, jclass, jfloat x, jfloat y, jint w, jint h) {
  const long long st = g_startNs.load();
  if (st > 0 && tedNs() - st < 700000000LL) return 0;
  int pw = g_plochaW.load(), ph = g_plochaH.load();
  float fx = x, fy = y;
  if (pw > 0 && ph > 0 && w > 0 && h > 0) { fx = x * pw / (float)w; fy = y * ph / (float)h; }
  else { pw = w; ph = h; }
  if (pw <= 0 || ph <= 0) return 2;
  const bool skip = napdemo::Demo::dotykNaSkip(pw, ph, fx, fy);
  NAPDEMO_LOG("DEMO: dotyk %.0f,%.0f (plocha %dx%d) -> %s", fx, fy, pw, ph, skip ? "SKIP" : "jinam");
  return skip ? 1 : 2;
}

// ===================================================================
//  B306: DEMO NA TV / PC PROUDEM Z C++ (Java /intro.stream jen posila bajty)
//  stejne rozhrani jako NativeSegaCoreBridge / NativePs1CoreBridge (B305)
// ===================================================================
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_proudOtevriNative(JNIEnv *, jclass) {
  return (jint)g_introProud.otevri();
}

JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_proudDalsiNative(JNIEnv *env, jclass, jint id, jbyteArray out, jint cekejMs, jint zapisMs) {
  if (!out) return INT_MIN;
  thread_local std::vector<uint8_t> pk;
  const int r = g_introProud.dalsi((int)id, (int)cekejMs, (int)zapisMs, pk);
  if (r < 0) return INT_MIN;
  if (r == 0) return 0;
  if ((jsize)pk.size() > env->GetArrayLength(out)) {
    const jint potreba = (jint)pk.size();
    g_introProud.odloz((int)id, pk);
    return -potreba;
  }
  env->SetByteArrayRegion(out, 0, (jsize)pk.size(), (const jbyte *)pk.data());
  return (jint)pk.size();
}

JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_proudZavriNative(JNIEnv *, jclass, jint id) {
  g_introProud.zavri((int)id);
}

/** bit0 = demo kresli (posledni snimek pred < 2 s), bit1 = nekdo se diva */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_proudStavNative(JNIEnv *, jclass) {
  return (g_introProud.bezi(2000) ? 1 : 0) | (g_introProud.klientu() > 0 ? 2 : 0);
}

JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeIntroBridge_proudLogNative(JNIEnv *env, jclass) {
  const std::string s = g_introProud.vezmiLog();
  if (s.empty()) return nullptr;
  return env->NewStringUTF(s.c_str());
}

/**
 * Posledni snimek pro TV (16:9) - stejny tvar jako Ps1GlTextureView.borrowFrame:
 *   (sirka<<16)|vyska kdyz predal, zaporne kdyz je pole male, 0 kdyz nic noveho.
 * B307: jen pro stare prohlizece bez ReadableStream (ty /intro.stream neumi);
 * vsechny ostatni berou demo proudem z C++.
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
