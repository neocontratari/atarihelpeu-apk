// test_overeni/b311/menu_snimky.cpp
// ====================================================================
//  B311: hlavni menu BEZ telefonu - presne ten samy C++ (app/src/main/cpp/menu)
//  kresli pres Mesa (EGL bez okna) do framebufferu a snimky se ulozi jako PNG.
//
//  menu_snimky VYSTUP W H [--tv] [--zamky] poz:t [poz:t ...]
//      snimky menu (poz = poloha karuselu, t = cas menu v s; t < 0.7 = nastup)
//  menu_snimky VYSTUP W H --karty
//      obrazky karet bez textu (pro HTML zalohu: VYSTUP/karta_<id>.jpg)
//  menu_snimky VYSTUP W H --vykon SEKUND
//      kolik snimku za sekundu (svih tam a zpet, vsechny karty zive)
//  Promenne: NAPMENU_ASSETS (vychozi ../../app/src/main/assets)
// ====================================================================
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "../../app/src/main/cpp/vendor/stb/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "../../app/src/main/cpp/menu/nap_menu_kresba.h"

using namespace napmenu;

static std::string g_assets = "../../app/src/main/assets";
static int g_glVerze = 2;

static std::vector<unsigned char> nactiSoubor(const std::string &cesta) {
  std::vector<unsigned char> v;
  FILE *f = std::fopen((g_assets + "/" + cesta).c_str(), "rb");
  if (!f) return v;
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (n > 0) { v.resize((size_t)n); if (std::fread(v.data(), 1, (size_t)n, f) != (size_t)n) v.clear(); }
  std::fclose(f);
  return v;
}

static bool glKontext() {
  auto getPD = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  EGLDisplay d = getPD ? getPD(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr) : EGL_NO_DISPLAY;
  if (d == EGL_NO_DISPLAY) return false;
  EGLint ma, mi;
  if (!eglInitialize(d, &ma, &mi)) return false;
  eglBindAPI(EGL_OPENGL_ES_API);
  // jako v telefonu: nejdriv OpenGL ES 3 (mipmapy i u obrazku karet), jinak 2
  for (int v = 3; v >= 2; v--) {
    if (getenv("NAPMENU_ES2") && v == 3) continue;
    const EGLint xa[] = {EGL_CONTEXT_CLIENT_VERSION, v, EGL_NONE};
    EGLContext x = eglCreateContext(d, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, xa);
    if (x == EGL_NO_CONTEXT) continue;
    if (!eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, x)) continue;
    g_glVerze = v;
    std::printf("GL %s | %s (kontext ES %d)\n", glGetString(GL_VERSION), glGetString(GL_RENDERER), v);
    return true;
  }
  return false;
}

static void ulozPng(const std::string &jm, int w, int h, GLuint fbo) {
  std::vector<unsigned char> px((size_t)w * h * 4), ot((size_t)w * h * 4);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
  for (int y = 0; y < h; y++) std::memcpy(&ot[(size_t)y * w * 4], &px[(size_t)(h - 1 - y) * w * 4], (size_t)w * 4);
  for (size_t i = 0; i < (size_t)w * h; i++) ot[i * 4 + 3] = 255;
  stbi_write_png(jm.c_str(), w, h, 4, ot.data(), w * 4);
}

static double prumernyJas(int w, int h, GLuint fbo) {
  std::vector<unsigned char> px((size_t)w * h * 4);
  glBindFramebuffer(GL_FRAMEBUFFER, fbo);
  glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
  double s = 0;
  for (size_t i = 0; i < (size_t)w * h; i++) s += (px[i * 4] + px[i * 4 + 1] + px[i * 4 + 2]) / 3.0;
  return s / ((double)w * h);
}

int main(int argc, char **argv) {
  if (argc < 5) { std::fprintf(stderr, "pouziti: menu_snimky VYSTUP W H [--tv] [--zamky] poz:t ... | --karty | --vykon S\n"); return 2; }
  if (getenv("NAPMENU_ASSETS")) g_assets = getenv("NAPMENU_ASSETS");
  const std::string ven = argv[1];
  const int W = std::atoi(argv[2]), H = std::atoi(argv[3]);
  if (!glKontext()) { std::printf("CHYBA: EGL/GLES kontext nejde\n"); return 1; }
  const char *ext = (const char *)glGetString(GL_EXTENSIONS);
  const bool npot = g_glVerze >= 3 || (ext && std::strstr(ext, "GL_OES_texture_npot"));

  // logo
  std::vector<unsigned char> logoSoubor = nactiSoubor("design/nap_logo_original.png");
  int lw = 0, lh = 0, lc = 0;
  unsigned char *logo = logoSoubor.empty() ? nullptr : stbi_load_from_memory(logoSoubor.data(), (int)logoSoubor.size(), &lw, &lh, &lc, 4);
  // karty
  std::vector<unsigned char> kt = nactiSoubor("menu/karty.txt");
  std::string chyby;
  std::vector<Karta> karty = nactiKarty((const char *)kt.data(), kt.size(), &chyby);
  if (karty.empty()) karty = nactiKarty(KARTY_ZALOHA, std::strlen(KARTY_ZALOHA));
  if (!chyby.empty()) std::printf("karty.txt varovani:\n%s", chyby.c_str());

  Kresba k;
  const auto t0 = std::chrono::steady_clock::now();
  if (!k.init(logo, lw, lh, nactiSoubor, npot)) { std::printf("CHYBA: kresba se nepripravila\n"); return 1; }
  if (logo) stbi_image_free(logo);
  k.nastavKarty(karty);
  std::printf("MENU pripraveno za %.0f ms, karet %d, mipmapy %s\n",
              std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(), (int)karty.size(), npot ? "ano" : "ne");

  bool tv = false, zamky = false, karticky = false;
  double vykon = 0;
  std::vector<std::pair<float, double>> snimky;
  for (int i = 4; i < argc; i++) {
    if (!std::strcmp(argv[i], "--tv")) tv = true;
    else if (!std::strcmp(argv[i], "--zamky")) zamky = true;
    else if (!std::strcmp(argv[i], "--karty")) karticky = true;
    else if (!std::strcmp(argv[i], "--vykon") && i + 1 < argc) vykon = std::atof(argv[++i]);
    else {
      float p = 0; double t = 0;
      if (std::sscanf(argv[i], "%f:%lf", &p, &t) == 2) snimky.push_back({p, t});
    }
  }
  if (zamky) {
    for (int i = 0; i < (int)karty.size(); i++) {
      if (karty[i].zamekKlic == "sega") k.nastavZamek(i, true, u8"Napiš v Atari ještě 45 řádků programu (počítá se při RUN nebo LIST).");
      if (karty[i].zamekKlic == "ps1") k.nastavZamek(i, true, u8"Nejdřív odemkni Segu, pak 30 minut hraní na Seze.");
    }
  }

  CilMenu c;
  c.w = W; c.h = H; c.tv = tv; c.hlavni = true;
  Fbo cil;
  cil.priprav(W, H, false);
  c.fbo = cil.fbo;

  if (karticky) {
    // obrazky karet BEZ nazvu (nazev a ramecek dopise HTML): kazda karta uprostred,
    // cas 1,5 s -> VYSTUP/karta_<id>.jpg (pro HTML zalohu v index.html) + _s_nazvem.png (kontrola)
    for (int i = 0; i < (int)karty.size(); i++) {
      for (int sNazvem = 0; sNazvem < 2; sNazvem++) {
        const GLuint f = k.exportKarty(i, 1.5, W, H, sNazvem != 0);
        if (!f) { std::printf("CHYBA: karta %d se nevykreslila\n", i); continue; }
        const int w = k.umeniW(), h = k.umeniH();
        std::vector<unsigned char> px((size_t)w * h * 4), ot((size_t)w * h * 3);
        glBindFramebuffer(GL_FRAMEBUFFER, f);
        glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
        for (int y = 0; y < h; y++)
          for (int x = 0; x < w; x++)
            for (int q = 0; q < 3; q++) ot[((size_t)y * w + x) * 3 + q] = px[((size_t)(h - 1 - y) * w + x) * 4 + q];
        const std::string jm = ven + "/karta_" + karty[(size_t)i].id + (sNazvem ? "_s_nazvem.png" : ".jpg");
        if (sNazvem) stbi_write_png(jm.c_str(), w, h, 3, ot.data(), w * 3);
        else stbi_write_jpg(jm.c_str(), w, h, 3, ot.data(), 88);
        if (!sNazvem) std::printf("karta %-7s %dx%d -> %s\n", karty[(size_t)i].id.c_str(), w, h, jm.c_str());
      }
    }
    std::printf("umeni: %s\n%s", k.umeniNazvy().c_str(), k.log().c_str());
    GLenum e; int ch = 0;
    while ((e = glGetError()) != GL_NO_ERROR) { ch++; std::printf("GL chyba 0x%x\n", e); }
    std::printf("CHYB GL celkem: %d\n", ch);
    return 0;
  }

  if (vykon > 0) {
    // svih tam a zpet pres vsechny karty, vse zive, uroven 3
    k.nastavUroven(3);
    const int n = (int)karty.size();
    int snimku = 0;
    double soucet = 0, nejhorsi = 0;
    const auto z = std::chrono::steady_clock::now();
    while (std::chrono::duration<double>(std::chrono::steady_clock::now() - z).count() < vykon) {
      const double t = snimku / 60.0;
      StavMenu s;
      s.t = t; s.tPrichod = -10;
      s.poz = (float)((n - 1) * 0.5 * (1.0 - std::cos(t * 0.8)));
      s.pohyb = true;
      const auto a = std::chrono::steady_clock::now();
      k.snimek(s, c);
      glFinish();
      const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count();
      soucet += ms; nejhorsi = std::max(nejhorsi, ms); snimku++;
    }
    std::printf("VYKON %dx%d%s: %d snimku, prumer %.2f ms, nejhorsi %.2f ms (Mesa llvmpipe na CPU - telefon ma GPU)\n", W, H, tv ? " TV" : "", snimku, soucet / snimku, nejhorsi);
    return 0;
  }

  int chybGl = 0;
  for (auto &sn : snimky) {
    StavMenu s;
    s.poz = sn.first; s.t = sn.second; s.tPrichod = 0.0;
    s.pohyb = std::fabs(sn.first - std::round(sn.first)) > 0.01f;
    const auto a = std::chrono::steady_clock::now();
    k.snimek(s, c);
    glFinish();
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count();
    GLenum e;
    int ch = 0;
    while ((e = glGetError()) != GL_NO_ERROR) { ch++; std::printf("GL chyba 0x%x\n", e); }
    chybGl += ch;
    char jm[512];
    std::snprintf(jm, sizeof jm, "%s/menu_%dx%d%s_p%.2f_t%.2f.png", ven.c_str(), W, H, tv ? "_tv" : "", sn.first, sn.second);
    ulozPng(jm, W, H, c.fbo);
    std::printf("snimek poz=%.2f t=%.2f: %.1f ms, jas %.1f, prekresleno karet %d, chyb GL %d -> %s\n", sn.first, sn.second, ms, prumernyJas(W, H, c.fbo),
                k.prekreslenoMinule(), ch, jm);
  }
  std::printf("umeni: %s\n", k.umeniNazvy().c_str());
  std::printf("%s", k.log().c_str());
  std::printf("CHYB GL celkem: %d\n", chybGl);
  return 0;
}
