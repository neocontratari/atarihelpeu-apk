// test_overeni/b304/demo_snimky.cpp
// ====================================================================
//  B304: overeni uvodniho dema BEZ telefonu.
//
//  Presne ten samy C++ kod (app/src/main/cpp/intro/nap_demo*.h), ktery
//  na telefonu kresli na displej, tady kresli pres Mesa (EGL bez okna,
//  OpenGL ES 2.0) do framebufferu a snimky se ulozi jako PNG. Tak se da
//  zkontrolovat kazda scena v case t, na vysku i na sirku (TV), a zmerit,
//  jestli se vsechny shadery prelozi.
//
//  Pouziti:  demo_snimky VYSTUP W H t1 [t2 ...]
//            demo_snimky VYSTUP W H --vse KROK      (cele demo po KROK s)
// ====================================================================
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../../app/src/main/cpp/vendor/stb/stb_image.h"
#include <zlib.h>

#include "../../app/src/main/cpp/intro/nap_demo.h"

// PNG (RGBA 8 bit) pres zlib - at test nepotrebuje nic navic
static void pngZapis(const char *jm, int w, int h, const unsigned char *rgba) {
  std::vector<unsigned char> raw;
  raw.reserve((size_t)(w * 4 + 1) * h);
  for (int y = 0; y < h; y++) { raw.push_back(0); raw.insert(raw.end(), rgba + (size_t)y * w * 4, rgba + (size_t)(y + 1) * w * 4); }
  uLongf zl = compressBound(raw.size());
  std::vector<unsigned char> z(zl);
  compress2(z.data(), &zl, raw.data(), raw.size(), 6);
  FILE *f = std::fopen(jm, "wb");
  if (!f) return;
  auto be32 = [&](uint32_t v) { unsigned char b[4] = {(unsigned char)(v >> 24), (unsigned char)(v >> 16), (unsigned char)(v >> 8), (unsigned char)v}; std::fwrite(b, 1, 4, f); };
  auto blok = [&](const char *typ, const unsigned char *d, size_t n) {
    be32((uint32_t)n);
    uLong c = crc32(0, (const Bytef *)typ, 4);
    if (n) c = crc32(c, d, (uInt)n);
    std::fwrite(typ, 1, 4, f);
    if (n) std::fwrite(d, 1, n, f);
    be32((uint32_t)c);
  };
  static const unsigned char sig[8] = {137, 80, 78, 71, 13, 10, 26, 10};
  std::fwrite(sig, 1, 8, f);
  unsigned char ih[13] = {(unsigned char)(w >> 24), (unsigned char)(w >> 16), (unsigned char)(w >> 8), (unsigned char)w,
                          (unsigned char)(h >> 24), (unsigned char)(h >> 16), (unsigned char)(h >> 8), (unsigned char)h, 8, 6, 0, 0, 0};
  blok("IHDR", ih, 13);
  blok("IDAT", z.data(), zl);
  blok("IEND", nullptr, 0);
  std::fclose(f);
}

static bool glKontext() {
  auto getPD = (PFNEGLGETPLATFORMDISPLAYEXTPROC)eglGetProcAddress("eglGetPlatformDisplayEXT");
  EGLDisplay d = getPD ? getPD(EGL_PLATFORM_SURFACELESS_MESA, EGL_DEFAULT_DISPLAY, nullptr) : EGL_NO_DISPLAY;
  if (d == EGL_NO_DISPLAY) return false;
  EGLint ma, mi;
  if (!eglInitialize(d, &ma, &mi)) return false;
  eglBindAPI(EGL_OPENGL_ES_API);
  const EGLint xa[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
  EGLContext x = eglCreateContext(d, EGL_NO_CONFIG_KHR, EGL_NO_CONTEXT, xa);
  if (x == EGL_NO_CONTEXT) return false;
  if (!eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, x)) return false;
  std::printf("GL %s | %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));
  return true;
}

int main(int argc, char **argv) {
  if (argc < 5) { std::fprintf(stderr, "pouziti: %s VYSTUP W H t1 [t2..] | --vse KROK\n", argv[0]); return 2; }
  const std::string ven = argv[1];
  const int W = std::atoi(argv[2]), H = std::atoi(argv[3]);
  std::vector<double> casy;
  if (std::string(argv[4]) == "--vse") {
    double krok = argc > 5 ? std::atof(argv[5]) : 1.0;
    for (double t = 0; t <= napdemo::DELKA + 1e-6; t += krok) casy.push_back(t);
  } else {
    for (int i = 4; i < argc; i++) casy.push_back(std::atof(argv[i]));
  }
  if (!glKontext()) { std::fprintf(stderr, "EGL/GLES2 kontext se nepodaril\n"); return 1; }

  int lw = 0, lh = 0, lc = 0;
  // logo: z promenne NAPDEMO_LOGO, jinak relativne k repozitari
  const char *kandidati[] = {std::getenv("NAPDEMO_LOGO"), "../../app/src/main/assets/design/nap_logo_original.png",
                             "app/src/main/assets/design/nap_logo_original.png"};
  unsigned char *logo = nullptr;
  for (const char *k : kandidati) {
    if (!k) continue;
    logo = stbi_load(k, &lw, &lh, &lc, 4);
    if (logo) break;
  }
  if (!logo) { std::fprintf(stderr, "CHYBI LOGO (nastav NAPDEMO_LOGO)\n"); return 1; }
  std::printf("logo %dx%d\n", lw, lh);

  auto t0 = std::chrono::steady_clock::now();
  napdemo::Demo demo;
  bool ok = demo.init(logo, lw, lh);
  stbi_image_free(logo);
  auto t1 = std::chrono::steady_clock::now();
  std::printf("init %s za %.0f ms\n", ok ? "OK" : "SELHAL", std::chrono::duration<double, std::milli>(t1 - t0).count());
  if (!ok) return 1;

  napdemo::Cil cil;
  cil.w = W; cil.h = H; cil.tv = W > H;
  cil.meritko = 0.5f;
  napdemo::Fbo cilovy;
  cilovy.priprav(W, H, true);
  demo.pripravTexty(cil, 100);

  std::vector<unsigned char> px((size_t)W * H * 4), rad((size_t)W * 4);
  int chyb = 0;
  for (double t : casy) {
    auto a = std::chrono::steady_clock::now();
    demo.snimek(t, cil, cilovy.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, cilovy.fbo);
    glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    auto b = std::chrono::steady_clock::now();
    GLenum e = glGetError();
    if (e != GL_NO_ERROR) { std::printf("  GL chyba 0x%x v t=%.2f\n", e, t); chyb++; }
    for (int y = 0; y < H / 2; y++) {          // OpenGL ma pocatek dole
      std::memcpy(rad.data(), &px[(size_t)y * W * 4], W * 4);
      std::memcpy(&px[(size_t)y * W * 4], &px[(size_t)(H - 1 - y) * W * 4], W * 4);
      std::memcpy(&px[(size_t)(H - 1 - y) * W * 4], rad.data(), W * 4);
    }
    // prumerny jas (kontrola, ze neni cerno tam, kde nema byt)
    double s = 0;
    for (size_t i = 0; i < px.size(); i += 4) s += 0.2126 * px[i] + 0.7152 * px[i + 1] + 0.0722 * px[i + 2];
    s /= (double)W * H;
    char jm[512];
    std::snprintf(jm, sizeof jm, "%s/f_%05.2f.png", ven.c_str(), t);
    pngZapis(jm, W, H, px.data());
    std::printf("t=%6.2f  jas %6.1f  %7.0f ms  %s\n", t, s, std::chrono::duration<double, std::milli>(b - a).count(), jm);
  }
  return chyb ? 1 : 0;
}
