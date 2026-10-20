// test_overeni/b313/konzole_nahled.cpp
// ====================================================================
//  B313: KONZOLE N&P 16 A N&P 32 V KARTE MENU - obrazky prubehu animace (Mesa, bez telefonu)
//
//  Presne ten C++ z appky (menu/nap_menu_umeni_3d.h -> intro/nap_demo_konzole.h): karta
//  uprostred (fokus 1) od okamziku, kdy tam prijela, a v case po ni; karta mimo stred
//  (fokus 0,2) = konzole "venku" (cartridge / disk nad konzoli).
//    konzole_nahled VYSTUP W H        -> VYSTUP/nap16_*.png, nap32_*.png (+ vypis jasu a casu)
//  Vypisuje i cas kresleni karty (Mesa na CPU - jen pro porovnani mezi verzemi).
//
//   g++ -std=c++17 -O2 -w -I$INC -o konzole_nahled konzole_nahled.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
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
  for (int v = 3; v >= 2; v--) {
    if (getenv("NAPMENU_ES2") && v == 3) continue;
    const EGLint xa[] = {EGL_CONTEXT_CLIENT_VERSION, v, EGL_NONE};
    EGLContext x = eglCreateContext(d, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, xa);
    if (x == EGL_NO_CONTEXT) continue;
    if (!eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, x)) continue;
    std::printf("GL %s | %s (kontext ES %d)\n", glGetString(GL_VERSION), glGetString(GL_RENDERER), v);
    return true;
  }
  return false;
}

int main(int argc, char **argv) {
  if (argc < 4) { std::fprintf(stderr, "pouziti: konzole_nahled VYSTUP W H\n"); return 2; }
  const std::string ven = argv[1];
  const int W = std::atoi(argv[2]), H = std::atoi(argv[3]);
  if (!glKontext()) { std::printf("CHYBA: EGL/GLES kontext nejde\n"); return 1; }
  // spolecne prostredky jako Kresba::init (pisma z appky, sum, logo)
  Pismo bold, semi, med, ital;
  bold.init(NAP_FONT_CHAKRA_BOLD_TTF);
  semi.init(NAP_FONT_CHAKRA_SEMIBOLD_TTF);
  med.init(NAP_FONT_CHAKRA_MEDIUM_TTF);
  ital.init(NAP_FONT_CHAKRA_BOLDITALIC_TTF);
  KontextUmeni k;
  k.bold = &bold; k.semi = &semi; k.med = &med; k.ital = &ital;
  k.nacti = nactiSoubor;
  if (!k.init()) { std::printf("CHYBA: kontext umeni\n"); return 1; }
  {
    std::vector<unsigned char> n(256 * 256 * 4);
    napdemo::Los los(20261120);
    for (int i = 0; i < 256 * 256; i++) { n[i * 4] = (unsigned char)(los.dalsi() >> 24); n[i * 4 + 2] = (unsigned char)(los.dalsi() >> 24); n[i * 4 + 3] = 255; }
    for (int y = 0; y < 256; y++)
      for (int x = 0; x < 256; x++) n[(y * 256 + x) * 4 + 1] = n[(((y + 17) & 255) * 256 + ((x + 37) & 255)) * 4];
    k.texSum = napdemo::novaTextura(256, 256, GL_RGBA, n.data(), true, true);
  }
  {
    std::vector<unsigned char> ls = nactiSoubor("design/nap_logo_original.png");
    int lw = 0, lh = 0, lc = 0;
    unsigned char *logo = ls.empty() ? nullptr : stbi_load_from_memory(ls.data(), (int)ls.size(), &lw, &lh, &lc, 4);
    if (logo) {
      // jako Kresba::nastavLogo: prevzorkovat na 512x512 kvuli mipmapam
      std::vector<unsigned char> pot((size_t)512 * 512 * 4);
      for (int y = 0; y < 512; y++)
        for (int x = 0; x < 512; x++) {
          const int sx = std::min(lw - 1, x * lw / 512), sy = std::min(lh - 1, y * lh / 512);
          std::memcpy(&pot[((size_t)y * 512 + x) * 4], &logo[((size_t)sy * lw + sx) * 4], 4);
        }
      k.texLogo = napdemo::novaTextura(512, 512, GL_RGBA, pot.data(), true);
      glBindTexture(GL_TEXTURE_2D, k.texLogo);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
      stbi_image_free(logo);
    }
  }
  napdemo::Fbo fbo;
  fbo.priprav(W, H, true);
  Karta karta16, karta32;
  karta16.id = "sega"; karta16.barva = 0x2E8BFF;
  karta32.id = "ps1"; karta32.barva = 0xB04CFF;
  int chybGl = 0;
  for (int bitu : {16, 32}) {
    UmeniKonzole u(bitu);
    const auto t0 = std::chrono::steady_clock::now();
    const bool ok = u.init(k);
    std::printf("%s: init %s za %.0f ms\n", u.nazev(), ok ? "OK" : "CHYBA", std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count());
    struct S { const char *jm; double t; float fokus; bool nahled; };
    // karta mimo stred (venku), pak prijede doprostred v case 2,0 a animace bezi
    const S snimky[] = {{"a_venku", 1.0, 0.2f, false}, {"b_prijela", 2.0, 1.f, false}, {"c_0.45", 2.45, 1.f, false}, {"d_0.90", 2.90, 1.f, false},
                        {"e_1.10", 3.10, 1.f, false}, {"f_1.30", 3.30, 1.f, false}, {"g_1.60", 3.60, 1.f, false}, {"h_2.00", 4.00, 1.f, false},
                        {"i_3.50", 5.50, 1.f, false}, {"j_nahled", 1.5, 1.f, true}};
    for (const S &s : snimky) {
      RamecUmeni r;
      r.w = W; r.h = H; r.t = s.t; r.d = 0.f; r.fokus = s.fokus; r.nahled = s.nahled;
      r.barva = barvaV3(bitu == 16 ? karta16.barva : karta32.barva);
      r.karta = bitu == 16 ? &karta16 : &karta32;
      glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
      glViewport(0, 0, W, H);
      glClearColor(0, 0, 0, 1);
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      const auto t1 = std::chrono::steady_clock::now();
      u.kresli(k, r);
      glFinish();
      const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count();
      std::vector<unsigned char> px((size_t)W * H * 4), ot((size_t)W * H * 3);
      glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
      double jas = 0;
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++)
          for (int q = 0; q < 3; q++) {
            const unsigned char c = px[((size_t)(H - 1 - y) * W + x) * 4 + q];
            ot[((size_t)y * W + x) * 3 + q] = c;
            jas += c;
          }
      const std::string jm = ven + "/nap" + std::to_string(bitu) + "_" + s.jm + ".png";
      stbi_write_png(jm.c_str(), W, H, 3, ot.data(), W * 3);
      std::printf("  %-10s t %.2f fokus %.1f: jas %5.1f, kresleni %.0f ms -> %s\n", s.jm, s.t, s.fokus, jas / ((double)W * H * 3), ms, jm.c_str());
    }
    GLenum e;
    while ((e = glGetError()) != GL_NO_ERROR) { chybGl++; std::printf("GL chyba 0x%x\n", e); }
  }
  std::printf("CHYB GL celkem: %d\n", chybGl);
  return chybGl ? 1 : 0;
}
