// nap_menu_gl.cpp
// ====================================================================
//  B311: HLAVNI MENU - most mezi Androidem a C++ (knihovna napmenu)
//
//  Rene: "cela ta uvodni stranka musi byt uplne jinak ... plynule
//  horizontalni skrolovani velkych obrazku a karet z prava do leva a zpatky
//  ... udelej na to cistej lehkej C++ kod pro grafiku od androidu 9, at to
//  lita s nulovou odezvou."
//
//  - vlastni vlakno s OpenGL ES (3.0, kdyz telefon umi - jinak 2.0) kresli
//    menu primo do plochy NapMenuView (SurfaceView), synchronne se snimky
//    displeje (eglSwapInterval 1 = 60 / 90 / 120 Hz podle telefonu)
//  - dotyk: Java preda kazdy bod prstu hned (i historicke body z jedne
//    udalosti), C++ ho zpracuje pod zamkem a vrati, co se ma stat (spustit
//    kartu, OPTIONS, nic); kresleni si pri kazdem snimku vezme NEJNOVEJSI
//    polohu prstu - karta jede s prstem 1:1
//  - karty ze souboru assets/menu/karty.txt (nap_menu_karty.h), fyzika
//    a rozlozeni nap_menu_karusel.h, kresleni nap_menu_kresba.h
//  - vykon: vlakno meri, jestli stiha snimky displeje; kdyz ne, prekresluje
//    zive obrazky karet mene casto (karusel sam jede vzdy plynule)
//  - TV / PC: obraz menu 16:9 proudem z C++ (/menu.stream, stejne jako uvodni
//    demo B306) - kresli se jen kdyz se nekdo diva
//
//  B313 (Rene: "grafika hlavniho menu je rozmazana na TV ci PC prenosu"):
//  - obraz pro TV podle kvality v OPTIONS (Java menuNastav): NIZKA 854x480,
//    STREDNI 1024x576, VYSOKA 1280x720 (drive vzdy 854x480 -> na TV 2,25x zvetseno)
//  - na TV v klidu stoji ozdoby (mlhovina, hvezdy, lesky) - konzole na karte se toci
//    dal; stojici casti se v prohlizeci zjemni na kvalitu 94 (kodek: vic obdelniku,
//    zjemneni za pohybu). Drive se v menu porad vsechno hybalo -> "zjemneni 0".
//  - vykon: perioda displeje ze skutecne obnovovaci frekvence (Java), ne odhad -
//    odhad u S8 ujel na 6-8 ms a menu si myslelo, ze nestiha (uroven 0 porad)
//
//  B314 (Rene: "slo by dat vyber jazyka - Cze a Ang?"): Java menuJazyk - karty
//  z anglickych sloupcu karty.txt (pouzijJazyk), napisy menu (LOCKED, napoveda na TV)
//  a obrazky karet s textem (kniha HELP) v anglictine; prepnout jde tam i zpet.
// ====================================================================
#include <jni.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <android/log.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include <climits>
#include <cmath>
#include <cstring>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#define NAPDEMO_LOG(...) __android_log_print(ANDROID_LOG_INFO, "NAP_MENU", __VA_ARGS__)

// stb: implementace jen tady a staticky (knihovna napmenu je samostatna, nic nesdili s napintro)
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "../vendor/stb/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#include "nap_menu_kresba.h"
#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"

#ifndef EGL_OPENGL_ES3_BIT_KHR
#define EGL_OPENGL_ES3_BIT_KHR 0x00000040
#endif

namespace {

using hodiny = std::chrono::steady_clock;
inline long long tedNs() { return std::chrono::duration_cast<std::chrono::nanoseconds>(hodiny::now().time_since_epoch()).count(); }
const long long g_nula = tedNs();
inline double tedS() { return (double)(tedNs() - g_nula) * 1e-9; }

// ---- prostredky ----
jobject g_assetsRef = nullptr;
AAssetManager *g_assets = nullptr;

std::vector<unsigned char> nactiAsset(const std::string &cesta) {
  std::vector<unsigned char> ven;
  if (!g_assets) return ven;
  AAsset *a = AAssetManager_open(g_assets, cesta.c_str(), AASSET_MODE_BUFFER);
  if (!a) return ven;
  const void *d = AAsset_getBuffer(a);
  const off_t n = AAsset_getLength(a);
  if (d && n > 0) ven.assign((const unsigned char *)d, (const unsigned char *)d + n);
  AAsset_close(a);
  return ven;
}

// ---- stav menu (pod zamkem g_m: dotyk z Javy, kresleni z vlakna) ----
std::mutex g_m;
napmenu::Karusel g_kar;
std::vector<napmenu::Karta> g_karty;
uint32_t g_verzeKaret = 1;            // zmena karet nebo zamku -> vlakno prevezme
bool g_maRozlozeni = false;           // vlakno uz spocitalo rozlozeni (dotyk ma kam trefit)
napmenu::StavMenu g_efekty;           // spusteni, zamek, kolecko, nastup
int g_prichodTyp = -1;                // -1 zadny, 1 nastup (karty prileti), 0 rychle prolnuti
int g_plochaW = 0, g_plochaH = 0;     // plocha, do ktere C++ kresli
std::string g_kartyLog;               // varovani z karty.txt (Java si vyzvedne)
std::atomic<bool> g_jazykEn{false};   // B314: menu anglicky (Java menuJazyk)

// ---- vlakno kresleni ----
std::mutex g_oknoZamek;
std::thread g_vlakno;
std::atomic<bool> g_bezi{false};
std::atomic<bool> g_pripraveno{false};
std::atomic<bool> g_chyba{false};
std::atomic<long long> g_snimku{0};
std::atomic<int> g_uroven{2};
std::mutex g_infoZamek;
std::string g_infoGl, g_infoBeh, g_infoLog;

// ---- TV / PC: obraz 16:9 proudem z C++ (jako uvodni demo) ----
// B313: velikost podle kvality TV z OPTIONS (Java: napTvWebQualityTier 0 / 1 / 2)
std::atomic<int> g_tvKvalita{2};
std::atomic<float> g_obnovaHz{0.f};   // B313: obnovovaci frekvence displeje (Display.getRefreshRate)
inline void tvRozmer(int kvalita, int &w, int &h) {
  if (kvalita <= 0) { w = 854; h = 480; }
  else if (kvalita == 1) { w = 1024; h = 576; }
  else { w = 1280; h = 720; }
}
constexpr double TV_OZDOBY_DOBEH_S = 0.8;   // B313: ozdoby na TV dobehnou po poslednim pohybu
nap::proud::Hub<nap::proud::KodekJpeg> &vytvorProud() {
  auto *h = new nap::proud::Hub<nap::proud::KodekJpeg>("MENU", 44100, 16, 9, false);
  h->nastavKodek(nap::proud::nastavKodekMenu);   // B313: vic obdelniku, zjemneni za pohybu, 4 pasy
  return *h;
}
nap::proud::Hub<nap::proud::KodekJpeg> &g_proud = vytvorProud();
std::mutex g_tvZamek;                 // stare prohlizece (Java si snimek vyzvedne)
std::vector<unsigned char> g_tvBody;
int g_tvBodyW = 0, g_tvBodyH = 0;     // B313: rozmer g_tvBody (pod g_tvZamek)
bool g_tvNovy = false;
std::atomic<long long> g_tvChceNs{0};

void logInfo(const std::string &s) {
  if (s.empty()) return;
  std::lock_guard<std::mutex> z(g_infoZamek);
  if (g_infoLog.size() < 16384) g_infoLog += s;
}

EGLConfig vyberKonfiguraci(EGLDisplay dpy, bool es3) {
  // nepruhledna plocha (menu lezi nad strankou), bez hloubky (karty maji vlastni)
  const EGLint a[] = {EGL_SURFACE_TYPE, EGL_WINDOW_BIT, EGL_RENDERABLE_TYPE, es3 ? EGL_OPENGL_ES3_BIT_KHR : EGL_OPENGL_ES2_BIT,
                      EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_NONE};
  EGLint n = 0;
  if (!eglChooseConfig(dpy, a, nullptr, 0, &n) || n <= 0) return nullptr;
  std::vector<EGLConfig> v((size_t)n);
  eglChooseConfig(dpy, a, v.data(), n, &n);
  EGLConfig nej = v[0];
  int nejBody = -1;
  for (int i = 0; i < n; i++) {
    EGLint r = 0, g = 0, b = 0, al = 0, d = 0, s = 0;
    eglGetConfigAttrib(dpy, v[(size_t)i], EGL_RED_SIZE, &r);
    eglGetConfigAttrib(dpy, v[(size_t)i], EGL_GREEN_SIZE, &g);
    eglGetConfigAttrib(dpy, v[(size_t)i], EGL_BLUE_SIZE, &b);
    eglGetConfigAttrib(dpy, v[(size_t)i], EGL_ALPHA_SIZE, &al);
    eglGetConfigAttrib(dpy, v[(size_t)i], EGL_DEPTH_SIZE, &d);
    eglGetConfigAttrib(dpy, v[(size_t)i], EGL_STENCIL_SIZE, &s);
    int body = 0;
    if (r == 8 && g == 8 && b == 8) body += 100;
    if (al == 0) body += 20;
    if (d == 0) body += 5;
    if (s == 0) body += 2;
    if (body > nejBody) { nejBody = body; nej = v[(size_t)i]; }
  }
  return nej;
}

// Kontext OpenGL ES: nejdriv 3.0 (mipmapy obrazku karet, ktere nejsou 2^n), jinak 2.0
bool otevriKontext(EGLDisplay dpy, EGLConfig &cfg, EGLContext &ctx, int &verze) {
  for (int v = 3; v >= 2; v--) {
    cfg = vyberKonfiguraci(dpy, v == 3);
    if (!cfg) continue;
    const EGLint ctxa[] = {EGL_CONTEXT_CLIENT_VERSION, v, EGL_NONE};
    ctx = eglCreateContext(dpy, cfg, EGL_NO_CONTEXT, ctxa);
    if (ctx != EGL_NO_CONTEXT) { verze = v; return true; }
  }
  return false;
}

void vlaknoMenu(ANativeWindow *okno) {
  EGLDisplay dpy = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  EGLint ma = 0, mi = 0;
  if (dpy == EGL_NO_DISPLAY || !eglInitialize(dpy, &ma, &mi)) {
    NAPDEMO_LOG("MENU: EGL se nepodarilo otevrit");
    g_chyba.store(true);
    ANativeWindow_release(okno);
    return;
  }
  EGLConfig cfg = nullptr;
  EGLContext ctx = EGL_NO_CONTEXT;
  int verze = 0;
  if (!otevriKontext(dpy, cfg, ctx, verze)) {
    NAPDEMO_LOG("MENU: zadny kontext OpenGL ES");
    g_chyba.store(true);
    ANativeWindow_release(okno);
    return;
  }
  EGLint fmt = 0;
  eglGetConfigAttrib(dpy, cfg, EGL_NATIVE_VISUAL_ID, &fmt);
  ANativeWindow_setBuffersGeometry(okno, 0, 0, fmt);
  EGLSurface surf = eglCreateWindowSurface(dpy, cfg, (EGLNativeWindowType)okno, nullptr);
  if (surf == EGL_NO_SURFACE || !eglMakeCurrent(dpy, surf, surf, ctx)) {
    NAPDEMO_LOG("MENU: plochu se nepodarilo pripravit (0x%x)", eglGetError());
    g_chyba.store(true);
    eglDestroyContext(dpy, ctx);
    if (surf != EGL_NO_SURFACE) eglDestroySurface(dpy, surf);
    ANativeWindow_release(okno);
    return;
  }
  eglSwapInterval(dpy, 1);
  const char *ver = (const char *)glGetString(GL_VERSION), *ren = (const char *)glGetString(GL_RENDERER);
  const char *ext = (const char *)glGetString(GL_EXTENSIONS);
  const bool npot = verze >= 3 || (ext && std::strstr(ext, "GL_OES_texture_npot"));
  NAPDEMO_LOG("MENU: GL %s | %s (ES %d)", ver ? ver : "?", ren ? ren : "?", verze);
  // hned neco na displeji (tmave pozadi menu), nez se pripravi karty
  {
    EGLint w = 0, h = 0;
    eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
    eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
    glViewport(0, 0, w, h);
    glClearColor(0.012f, 0.008f, 0.005f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);
    eglSwapBuffers(dpy, surf);
  }
  {
    const long long t0 = tedNs();
    std::unique_ptr<napmenu::Kresba> kr(new napmenu::Kresba());
    // logo N&P (minca)
    int lw = 0, lh = 0, lc = 0;
    std::vector<unsigned char> ls = nactiAsset("design/nap_logo_original.png");
    unsigned char *logo = ls.empty() ? nullptr : stbi_load_from_memory(ls.data(), (int)ls.size(), &lw, &lh, &lc, 4);
    bool ok = kr->init(logo, lw, lh, [](const std::string &c) { return nactiAsset(c); }, npot);
    if (logo) stbi_image_free(logo);
    uint32_t verzeKaret = 0;
    std::vector<napmenu::Karta> karty;
    {
      std::lock_guard<std::mutex> z(g_m);
      karty = g_karty;
      verzeKaret = g_verzeKaret;
    }
    if (karty.empty()) {
      karty = napmenu::nactiKarty(napmenu::KARTY_ZALOHA, std::strlen(napmenu::KARTY_ZALOHA));
      napmenu::pouzijJazyk(karty, g_jazykEn.load());
    }
    kr->nastavJazyk(g_jazykEn.load());
    kr->nastavKarty(karty);
    for (size_t i = 0; i < karty.size(); i++) kr->nastavZamek((int)i, karty[i].zamceno, karty[i].zamekText);
    kr->nastavUroven(g_uroven.load());
    napmenu::CilMenu cil, tv;
    tvRozmer(g_tvKvalita.load(), tv.w, tv.h);
    tv.tv = true; tv.hlavni = false;
    napdemo::Fbo tvFbo;
    std::vector<unsigned char> tvBuf;
    if (ok) {
      // "nanecisto": prvni snimek (pripravi obrazky viditelnych karet, texty, shadery
      // v ovladaci GPU) bez zobrazeni - pak uz se jen kresli
      EGLint w = 0, h = 0;
      eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
      eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
      if (w > 0 && h > 0) {
        cil.w = w; cil.h = h;
        napmenu::StavMenu s;
        {
          std::lock_guard<std::mutex> z(g_m);
          s = g_efekty;
          s.poz = g_kar.poz();
        }
        s.t = tedS();
        s.tPrichod = -10.0;
        kr->snimek(s, cil);
        glFinish();
        {
          std::lock_guard<std::mutex> z(g_m);
          g_kar.nastavRozlozeni(cil.roz);
          g_maRozlozeni = true;
          g_plochaW = w; g_plochaH = h;
        }
      }
      char b[400];
      std::snprintf(b, sizeof b, "GPU %s | %s | ES %d, mipmapy karet %s | priprava %lld ms, plocha %dx%d, karet %d, obrazky karet %dx%d",
                    ren ? ren : "?", ver ? ver : "?", verze, npot ? "ano" : "ne", (tedNs() - t0) / 1000000LL, (int)w, (int)h, (int)karty.size(),
                    kr->umeniW(), kr->umeniH());
      {
        std::lock_guard<std::mutex> z(g_infoZamek);
        g_infoGl = b;
        g_infoBeh.clear();
      }
      logInfo(kr->log());
      kr->vymazLog();
      // nastup zacina az ted (priprava mohla trvat)
      {
        std::lock_guard<std::mutex> z(g_m);
        if (g_prichodTyp >= 0) g_efekty.tPrichod = tedS() - (g_prichodTyp == 1 ? 0.0 : 0.45);
      }
      g_pripraveno.store(true);
    } else {
      NAPDEMO_LOG("MENU: priprava selhala");
      g_chyba.store(true);
    }

    // ---- hlavni smycka ----
    long long predNs = tedNs();
    // B313: perioda displeje ze skutecne obnovovaci frekvence (Java menuNastav). Drive
    // odhad "nejkratsi interval" - ten u S8 ujel na 6-8 ms (snimky, ktere eglSwapBuffers
    // pusti hned, protoze je volny buffer), a kazdy normalni snimek 16,7 ms pak vypadal
    // jako zmeskany -> uroven 0 porad, i kdyz telefon stihal.
    int okno60 = 0, zmeskano = 0, bezChyby = 0;
    double soucetMs = 0;
    int hlaseni = 0;
    // B313: TV - ozdoby v klidu stoji, obraz pro TV ~30 za sekundu (podle displeje)
    double tvOzdoby = 0.0, tvPredT = -1.0, tvPohybT = -100.0;
    int tvOdminule = 1000;
    while (ok && g_bezi.load()) {
      EGLint w = 0, h = 0;
      eglQuerySurface(dpy, surf, EGL_WIDTH, &w);
      eglQuerySurface(dpy, surf, EGL_HEIGHT, &h);
      if (w <= 0 || h <= 0) { std::this_thread::sleep_for(std::chrono::milliseconds(16)); continue; }
      cil.w = w; cil.h = h; cil.tv = false; cil.hlavni = true; cil.fbo = 0;
      const long long ted = tedNs();
      const double t = tedS();
      // stav pod zamkem: fyzika do "ted", nejnovejsi poloha prstu
      napmenu::StavMenu s;
      bool noveKarty = false;
      std::vector<napmenu::Karta> kopie;
      {
        std::lock_guard<std::mutex> z(g_m);
        g_kar.krok(t);
        s = g_efekty;
        s.t = t;
        s.poz = g_kar.poz();
        s.pohyb = g_kar.stav() != napmenu::Karusel::KLID || g_kar.prstDole();
        if (g_verzeKaret != verzeKaret) { kopie = g_karty; verzeKaret = g_verzeKaret; noveKarty = true; }
      }
      kr->nastavJazyk(g_jazykEn.load());   // B314: zmena jazyka -> napisy a obrazky karet s textem znovu
      if (noveKarty) {
        if ((int)kopie.size() != kr->pocetKaret()) kr->nastavKarty(kopie);
        else kr->nastavTextyKaret(kopie);  // B314: nazvy / popisy v jinem jazyce (obrazky zustanou)
        for (size_t i = 0; i < kopie.size(); i++) kr->nastavZamek((int)i, kopie[i].zamceno, kopie[i].zamekText);
      }
      kr->nastavUroven(g_uroven.load());
      kr->snimek(s, cil);
      {
        std::lock_guard<std::mutex> z(g_m);
        g_kar.nastavRozlozeni(cil.roz);
        g_maRozlozeni = true;
        g_plochaW = w; g_plochaH = h;
      }
      eglSwapBuffers(dpy, surf);
      g_snimku.fetch_add(1);
      const std::string kl = kr->log();
      if (!kl.empty()) { logInfo(kl); kr->vymazLog(); }

      // TV / PC: vlastni obraz 16:9 ~30x za sekundu, jen kdyz se nekdo diva
      const bool tvProud = g_proud.klientu() > 0;
      const bool tvJava = ted - g_tvChceNs.load() < 2000000000LL;
      if (!tvProud) g_proud.snimek(nullptr, 0, 0);            // jen "menu bezi" (pro /status proud=menu)
      // B313: ozdoby na TV bezi jen pri pohybu (a chvili po nem), jinak stoji
      {
        const bool deje = s.pohyb || s.spustena >= 0 || t - s.tPrichod < 1.0 || (s.tZamek >= 0 && t - s.tZamek < 0.6) ||
                          (s.tOzub >= 0 && t - s.tOzub < 0.6);
        if (deje) tvPohybT = t;
        if (tvPredT >= 0 && t - tvPohybT < TV_OZDOBY_DOBEH_S) tvOzdoby += t - tvPredT;
        tvPredT = t;
      }
      const float hz = g_obnovaHz.load();
      const int tvKrok = (hz >= 20.f && hz <= 250.f) ? std::max(1, (int)std::lround(hz / 30.0)) : 2;
      if (++tvOdminule >= tvKrok && (tvProud || tvJava)) {
        tvOdminule = 0;
        int tvW = 0, tvH = 0;
        tvRozmer(g_tvKvalita.load(), tvW, tvH);
        tv.w = tvW; tv.h = tvH;
        tvFbo.priprav(tvW, tvH, false);
        tv.fbo = tvFbo.fbo;
        napmenu::StavMenu sTv = s;
        sTv.tOzdoby = tvOzdoby;
        kr->snimek(sTv, tv);
        glBindFramebuffer(GL_FRAMEBUFFER, tvFbo.fbo);
        tvBuf.resize((size_t)tvW * tvH * 4);
        glReadPixels(0, 0, tvW, tvH, GL_RGBA, GL_UNSIGNED_BYTE, tvBuf.data());
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (tvJava) {
          std::lock_guard<std::mutex> z(g_tvZamek);
          if (tvProud) g_tvBody = tvBuf; else g_tvBody.swap(tvBuf);
          g_tvBodyW = tvW; g_tvBodyH = tvH;
          g_tvNovy = true;
        }
        // B313: proud dostane buffer z glReadPixels jak je (vymena, bez kopie) - otoceni a
        // prevod barev udela vlakno prohlizece, ne tohle (kresleni menu na telefonu)
        if (tvProud) g_proud.snimekRgbaZdola(tvBuf, tvW, tvH);
      }

      // ---- vykon: stiha displej? (interval mezi snimky vs. perioda displeje) ----
      const long long po = tedNs();
      const double dt = (double)(po - predNs) * 1e-9;
      predNs = po;
      const double perioda = (hz >= 20.f && hz <= 250.f) ? 1.0 / hz : 1.0 / 60.0;
      if (dt > perioda * 1.5) zmeskano++;
      soucetMs += (double)(po - ted) / 1e6;
      if (++okno60 >= 60) {
        int u = g_uroven.load();
        if (zmeskano >= 4 && u > 0) { u--; bezChyby = 0; }
        else if (zmeskano == 0 && ++bezChyby >= 3 && u < 3) { u++; bezChyby = 0; }
        else if (zmeskano > 0) bezChyby = 0;
        g_uroven.store(u);
        // B318: TV srozumitelne (Rene: "zase vidim JAVA"): proud z C++ = prohlizec bere menu primo
        // z C++; stara cesta = Java si snimek bere pro prohlizec bez proudu (od B318 jen kdyz se
        // takovy prohlizec opravdu diva); jinak TV menu nekresli vubec
        char tvTxt[96];
        if (tvProud) std::snprintf(tvTxt, sizeof tvTxt, "proud z C++ %dx%d", tv.w, tv.h);
        else if (tvJava) std::snprintf(tvTxt, sizeof tvTxt, "stara cesta pres Javu (prohlizec bez proudu) %dx%d", tv.w, tv.h);
        else std::snprintf(tvTxt, sizeof tvTxt, "nikdo se nediva - TV obraz se nekresli");
        char b[360];
        std::snprintf(b, sizeof b, "snimek %.1f ms (perioda displeje %.1f ms = %.0f Hz %s), zmeskano %d z 60, uroven %d, plocha %dx%d, prekresleno karet %d, TV: %s",
                      soucetMs / okno60, perioda * 1000.0, 1.0 / perioda, (hz >= 20.f && hz <= 250.f) ? "z displeje" : "vychozi",
                      zmeskano, u, (int)w, (int)h, kr->prekreslenoMinule(), tvTxt);
        {
          std::lock_guard<std::mutex> z(g_infoZamek);
          g_infoBeh = b;
        }
        if (++hlaseni % 10 == 1) NAPDEMO_LOG("MENU %s", b);
        okno60 = 0; zmeskano = 0; soucetMs = 0;
      }
    }
    tvFbo.uvolni();
    // kresba (GL objekty) se uvolni s kontextem
  }
  eglMakeCurrent(dpy, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  eglDestroySurface(dpy, surf);
  eglDestroyContext(dpy, ctx);
  ANativeWindow_release(okno);
  g_pripraveno.store(false);
  {
    std::lock_guard<std::mutex> z(g_m);
    g_maRozlozeni = false;
  }
  NAPDEMO_LOG("MENU: plocha uvolnena");
}

void zastavKresleni() {
  g_bezi.store(false);
  if (g_vlakno.joinable()) g_vlakno.join();
}

// akce karuselu -> efekt (pod zamkem g_m) a navratova hodnota pro Javu
int zpracujAkci(int a, double t) {
  if (a >= napmenu::AKCE_KARTA && a < napmenu::AKCE_NASTAVENI) {
    const int i = a - napmenu::AKCE_KARTA;
    if (i < 0 || i >= (int)g_karty.size()) return napmenu::AKCE_NIC;
    if (g_efekty.spustena >= 0) return napmenu::AKCE_NIC;            // uz se spousti
    if (g_karty[(size_t)i].zamceno) {
      g_efekty.zamekKlep = i;
      g_efekty.tZamek = t;
      return napmenu::AKCE_ZAMCENO + i;
    }
    g_efekty.spustena = i;
    g_efekty.tSpusteni = t;
    return a;
  }
  if (a == napmenu::AKCE_NASTAVENI) {
    if (g_efekty.spustena >= 0) return napmenu::AKCE_NIC;
    g_efekty.tOzub = t;
    return a;
  }
  return a;
}

std::string jstr(JNIEnv *env, jstring s) {
  if (!s) return std::string();
  const char *c = env->GetStringUTFChars(s, nullptr);
  std::string r = c ? c : "";
  if (c) env->ReleaseStringUTFChars(s, c);
  return r;
}

}  // namespace

// ====================================================================
//  JNI (eu.atarihelp.emu10.NativeMenuBridge)
// ====================================================================
extern "C" {

/** Assety + nacteni karet z menu/karty.txt (kdyz chybi: karty zabudovane v C++). Vraci pocet karet. */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastavAssety(JNIEnv *env, jclass, jobject am) {
  if (am) {
    if (g_assetsRef) env->DeleteGlobalRef(g_assetsRef);
    g_assetsRef = env->NewGlobalRef(am);
    g_assets = AAssetManager_fromJava(env, g_assetsRef);
  }
  std::vector<unsigned char> d = nactiAsset("menu/karty.txt");
  std::string chyby;
  std::vector<napmenu::Karta> k = napmenu::nactiKarty((const char *)d.data(), d.size(), &chyby);
  if (k.empty()) {
    chyby += d.empty() ? "menu/karty.txt v appce neni - beru karty zabudovane v C++\n" : "menu/karty.txt nema zadnou kartu - beru karty zabudovane v C++\n";
    k = napmenu::nactiKarty(napmenu::KARTY_ZALOHA, std::strlen(napmenu::KARTY_ZALOHA));
  }
  napmenu::pouzijJazyk(k, g_jazykEn.load());   // B314
  std::lock_guard<std::mutex> z(g_m);
  // zamky, ktere uz Java poslala, zustanou (stejne id)
  for (napmenu::Karta &n : k)
    for (const napmenu::Karta &s : g_karty)
      if (s.id == n.id) { n.zamceno = s.zamceno; n.zamekText = s.zamekText; }
  g_karty = k;
  g_kar.nastavPocet((int)k.size());
  g_verzeKaret++;
  g_kartyLog = chyby;
  return (jint)k.size();
}

JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuKaret(JNIEnv *, jclass) {
  std::lock_guard<std::mutex> z(g_m);
  return (jint)g_karty.size();
}

/** Karta i pro Javu: "id|NAZEV|cil|zamek" (Java podle toho prepne stranku a hlida zamky). */
JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuKarta(JNIEnv *env, jclass, jint i) {
  std::string s;
  {
    std::lock_guard<std::mutex> z(g_m);
    if (i < 0 || i >= (jint)g_karty.size()) return nullptr;
    const napmenu::Karta &k = g_karty[(size_t)i];
    s = k.id + "|" + k.nazev + "|" + k.cil + "|" + k.zamekKlic;
  }
  return env->NewStringUTF(s.c_str());
}

/** Zamek (sega / ps1 ...): vsem kartam s timto klicem. text = proc (ukaze se pod kartami). */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuZamek(JNIEnv *env, jclass, jstring jklic, jboolean zamceno, jstring jtext) {
  const std::string klic = jstr(env, jklic), text = jstr(env, jtext);
  std::lock_guard<std::mutex> z(g_m);
  bool zmena = false;
  for (napmenu::Karta &k : g_karty) {
    if (k.zamekKlic.empty() || k.zamekKlic != klic) continue;
    const bool zam = zamceno == JNI_TRUE;
    if (k.zamceno != zam || k.zamekText != text) { k.zamceno = zam; k.zamekText = text; zmena = true; }
  }
  if (zmena) g_verzeKaret++;
}

JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuPlocha(JNIEnv *env, jclass, jobject jsurf) {
  std::lock_guard<std::mutex> z(g_oknoZamek);
  zastavKresleni();
  if (!jsurf) { NAPDEMO_LOG("MENU: plocha odpojena"); return; }
  ANativeWindow *okno = ANativeWindow_fromSurface(env, jsurf);
  if (!okno) { NAPDEMO_LOG("MENU: okno se nepodarilo ziskat"); g_chyba.store(true); return; }
  g_chyba.store(false);
  g_bezi.store(true);
  g_vlakno = std::thread(vlaknoMenu, okno);
}

/**
 * Menu se ukazuje: jak = 1 nastup (karty prileti zprava - poprve po intru),
 * 0 = jen rychle prolnuti (navrat z emulatoru). Zrusi rozbehnute spusteni.
 */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuUkaz(JNIEnv *, jclass, jint jak) {
  std::lock_guard<std::mutex> z(g_m);
  const double t = tedS();
  g_efekty.spustena = -1;
  g_efekty.tSpusteni = -1;
  g_efekty.zamekKlep = -1;
  g_efekty.tZamek = -1;
  g_efekty.tOzub = -1;
  // nastup 0,65 s; rychle prolnuti = nastup, ktery uz je skoro hotovy
  g_prichodTyp = jak == 1 ? 1 : 0;
  g_efekty.tPrichod = jak == 1 ? t : t - 0.45;
  g_kar.zrus();
  g_kar.skoc(g_kar.vybrana());
}

/** Prah dotyku (ViewConfiguration.getScaledTouchSlop) v bodech view. */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastavPrah(JNIEnv *, jclass, jfloat px) {
  std::lock_guard<std::mutex> z(g_m);
  g_kar.nastavPrah(px);
}

/**
 * Dotyk: akce 0 = dolu, 1 = tah, 2 = nahoru, 3 = zruseno. x, y v bodech view w x h
 * (prepocte se na plochu, do ktere C++ kresli). casMs = cas udalosti (uptimeMillis).
 * Vraci: 0 nic, 1000 + i spustit kartu i (po animaci ~0,3 s), 2000 OPTIONS,
 *        3000 + i karta i je zamcena (C++ ukaze proc).
 */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuDotyk(JNIEnv *, jclass, jint akce, jfloat x, jfloat y, jlong casMs, jint w, jint h) {
  std::lock_guard<std::mutex> z(g_m);
  if (!g_maRozlozeni || g_efekty.spustena >= 0) return 0;
  float fx = x, fy = y;
  if (g_plochaW > 0 && g_plochaH > 0 && w > 0 && h > 0) { fx = x * g_plochaW / (float)w; fy = y * g_plochaH / (float)h; }
  // cas udalosti (uptimeMillis = CLOCK_MONOTONIC) jen pro rychlost prstu - jinak hodiny C++
  const double tUd = (double)casMs * 1e-3;
  const double t = tedS();
  int a = napmenu::AKCE_NIC;
  switch (akce) {
    case 0: g_kar.krok(t); g_kar.dolu(fx, fy, tUd); break;
    case 1: g_kar.tah(fx, fy, tUd); break;
    case 2: g_kar.krok(t); a = g_kar.nahoru(fx, fy, tUd); break;
    default: g_kar.zrus(); break;
  }
  return zpracujAkci(a, t);
}

/** Klavesa (sipky, Enter, Home, End - Java prevede z KeyEvent nebo z prohlizece na TV). */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuKlavesa(JNIEnv *, jclass, jint kl) {
  std::lock_guard<std::mutex> z(g_m);
  if (!g_maRozlozeni || g_efekty.spustena >= 0) return 0;
  const double t = tedS();
  g_kar.krok(t);
  return zpracujAkci(g_kar.klavesa(kl), t);
}

JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuVybrana(JNIEnv *, jclass) {
  std::lock_guard<std::mutex> z(g_m);
  return (jint)g_kar.vybrana();
}

JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuSkoc(JNIEnv *, jclass, jint i) {
  std::lock_guard<std::mutex> z(g_m);
  g_kar.skoc(i);
}

JNIEXPORT jboolean JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuPripraveno(JNIEnv *, jclass) {
  return g_pripraveno.load() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jboolean JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuChyba(JNIEnv *, jclass) {
  return g_chyba.load() ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT jlong JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuSnimku(JNIEnv *, jclass) {
  return (jlong)g_snimku.load();
}

/** Kratka zprava do logu appky: GPU, priprava, rychlost, varovani karet (vyzvedne a smaze). */
JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuInfo(JNIEnv *env, jclass) {
  std::string s;
  {
    std::lock_guard<std::mutex> z(g_infoZamek);
    s = g_infoGl + " || " + g_infoBeh;
  }
  return env->NewStringUTF(s.c_str());
}

JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuLog(JNIEnv *env, jclass) {
  std::string s;
  {
    std::lock_guard<std::mutex> z(g_m);
    s.swap(g_kartyLog);
  }
  {
    std::lock_guard<std::mutex> z(g_infoZamek);
    s += g_infoLog;
    g_infoLog.clear();
  }
  if (s.empty()) return nullptr;
  return env->NewStringUTF(s.c_str());
}

// ===================================================================
//  TV / PC: MENU PROUDEM Z C++ (/menu.stream) - stejne rozhrani jako uvodni demo
// ===================================================================
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_proudOtevriNative(JNIEnv *, jclass) {
  return (jint)g_proud.otevri();
}

JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_proudDalsiNative(JNIEnv *env, jclass, jint id, jbyteArray out, jint cekejMs, jint zapisMs) {
  if (!out) return INT_MIN;
  thread_local std::vector<uint8_t> pk;
  const int r = g_proud.dalsi((int)id, (int)cekejMs, (int)zapisMs, pk);
  if (r < 0) return INT_MIN;
  if (r == 0) return 0;
  if ((jsize)pk.size() > env->GetArrayLength(out)) {
    const jint potreba = (jint)pk.size();
    g_proud.odloz((int)id, pk);
    return -potreba;
  }
  env->SetByteArrayRegion(out, 0, (jsize)pk.size(), (const jbyte *)pk.data());
  return (jint)pk.size();
}

JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_proudZavriNative(JNIEnv *, jclass, jint id) {
  g_proud.zavri((int)id);
}

/** bit0 = menu kresli (posledni snimek pred < 2 s), bit1 = nekdo se diva */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_proudStavNative(JNIEnv *, jclass) {
  return (g_proud.bezi(2000) ? 1 : 0) | (g_proud.klientu() > 0 ? 2 : 0);
}

JNIEXPORT jstring JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_proudLogNative(JNIEnv *env, jclass) {
  const std::string s = g_proud.vezmiLog();
  if (s.empty()) return nullptr;
  return env->NewStringUTF(s.c_str());
}

/**
 * Posledni snimek pro TV (16:9) pro stare prohlizece bez ReadableStream (jako
 * introGrabFrame): (sirka<<16)|vyska, zaporne kdyz je pole male, 0 kdyz nic noveho.
 */
JNIEXPORT jint JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuGrabFrame(JNIEnv *env, jclass, jintArray jven) {
  g_tvChceNs.store(tedNs());
  std::lock_guard<std::mutex> z(g_tvZamek);
  const int W = g_tvBodyW, H = g_tvBodyH;     // B313: velikost podle kvality TV
  if (!g_tvNovy || W <= 0 || H <= 0 || g_tvBody.size() < (size_t)W * H * 4) return 0;
  const int potreba = W * H;
  if (!jven || env->GetArrayLength(jven) < potreba) return -((W << 16) | H);
  jint *ven = env->GetIntArrayElements(jven, nullptr);
  if (!ven) return 0;
  for (int y = 0; y < H; y++) {
    const unsigned char *r = &g_tvBody[(size_t)(H - 1 - y) * W * 4];
    jint *o = ven + (size_t)y * W;
    for (int x = 0; x < W; x++)
      o[x] = (jint)(0xFF000000u | ((unsigned)r[x * 4] << 16) | ((unsigned)r[x * 4 + 1] << 8) | (unsigned)r[x * 4 + 2]);
  }
  env->ReleaseIntArrayElements(jven, ven, 0);
  g_tvNovy = false;
  return (jint)((W << 16) | H);
}

/**
 * B313: obnovovaci frekvence displeje (Display.getRefreshRate - pro mereni, jestli
 * menu stiha) a kvalita TV z OPTIONS (0 NIZKA 854x480, 1 STREDNI 1024x576,
 * 2 VYSOKA 1280x720). Java vola pri ukazani menu a pri zmene kvality.
 */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastav(JNIEnv *, jclass, jfloat hz, jint tvKvalita) {
  if (hz >= 20.f && hz <= 250.f) g_obnovaHz.store(hz);
  g_tvKvalita.store(std::max(0, std::min(2, (int)tvKvalita)));
}

/** B314: jazyk menu (1 = anglicky). Karty z anglickych sloupcu karty.txt, napisy, kniha HELP. */
JNIEXPORT void JNICALL Java_eu_atarihelp_emu10_NativeMenuBridge_menuJazyk(JNIEnv *, jclass, jint en) {
  const bool e = en != 0;
  std::lock_guard<std::mutex> z(g_m);
  g_jazykEn.store(e);
  napmenu::pouzijJazyk(g_karty, e);
  g_verzeKaret++;
}

}  // extern "C"
