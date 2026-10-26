// test_overeni/b314/intro_jazyk.cpp
// ====================================================================
//  B314: UVODNI DEMO CESKY / ANGLICKY (Mesa, presne C++ appky - nap_demo.h)
//  Demo cesky (nastavJazyk(false) - titulky cesky + anglicky jako dosud) a anglicky
//  (nastavJazyk(true) - jen anglicke titulky a anglicke zaverecne titulky, jako
//  po volbe ENGLISH v OPTIONS - Java introJazyk). Stejne casy, porovnani snimku:
//   - kde neni zadny text, jsou snimky stejne (jazyk nic jineho nemeni),
//   - kde je titulek nebo zaverecne titulky, se lisi,
//   - bez chyb GL; PNG do VYSTUP (jen pro kontrolu ocima).
//  intro_jazyk VYSTUP W H
//  g++ -std=c++17 -O2 -I$INC -o intro_jazyk intro_jazyk.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz
// ====================================================================
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
#include <cmath>
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
#include "../../app/src/main/cpp/vendor/stb/stb_image.h"
#include <zlib.h>

#include "../../app/src/main/cpp/intro/nap_demo.h"

static void pngZapis(const std::string &jm, int w, int h, const unsigned char *rgba) {
  std::vector<unsigned char> raw;
  raw.reserve((size_t)(w * 4 + 1) * h);
  for (int y = 0; y < h; y++) { raw.push_back(0); raw.insert(raw.end(), rgba + (size_t)y * w * 4, rgba + (size_t)(y + 1) * w * 4); }
  uLongf zl = compressBound(raw.size());
  std::vector<unsigned char> z(zl);
  compress2(z.data(), &zl, raw.data(), raw.size(), 6);
  FILE *f = std::fopen(jm.c_str(), "wb");
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
  return eglMakeCurrent(d, EGL_NO_SURFACE, EGL_NO_SURFACE, x);
}

struct Beh {
  napdemo::Demo demo;
  napdemo::Cil cil;
  napdemo::Fbo fbo;
};

static int kontrol = 0, chyb = 0;
static void over(bool ok, const std::string &co) {
  kontrol++;
  if (!ok) chyb++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

int main(int argc, char **argv) {
  if (argc < 4) { std::fprintf(stderr, "pouziti: %s VYSTUP W H\n", argv[0]); return 2; }
  const std::string ven = argv[1];
  const int W = std::atoi(argv[2]), H = std::atoi(argv[3]);
  if (!glKontext()) { std::printf("CHYBA: EGL/GLES2 kontext\n"); return 1; }
  int lw = 0, lh = 0, lc = 0;
  unsigned char *logo = stbi_load("../../app/src/main/assets/design/nap_logo_original.png", &lw, &lh, &lc, 4);
  if (!logo) { std::printf("CHYBA: logo\n"); return 1; }
  std::vector<unsigned char> klip;
  if (FILE *f = std::fopen("../../app/src/main/assets/intro/pittkitt.klip", "rb")) {
    unsigned char b[65536]; size_t n;
    while ((n = std::fread(b, 1, sizeof b, f)) > 0) klip.insert(klip.end(), b, b + n);
    std::fclose(f);
  }
  Beh *beh[2];
  for (int j = 0; j < 2; j++) {
    beh[j] = new Beh();
    beh[j]->demo.nastavJazyk(j == 1);                 // jako Java introJazyk pred startem dema
    if (!beh[j]->demo.init(logo, lw, lh)) { std::printf("CHYBA: init\n"); return 1; }
    if (!klip.empty()) beh[j]->demo.nastavKlip(klip.data(), klip.size());
    beh[j]->cil.w = W; beh[j]->cil.h = H; beh[j]->cil.tv = W > H; beh[j]->cil.meritko = 0.5f;
    beh[j]->fbo.priprav(W, H, true);
    beh[j]->demo.pripravTexty(beh[j]->cil, 100);
  }
  stbi_image_free(logo);
  over(!beh[0]->demo.jazykEn() && beh[1]->demo.jazykEn(), "demo cesky / anglicky (nastavJazyk jako Java introJazyk)");
  struct Cas { double t; bool text; const char *co; };
  const Cas casy[] = {
      {0.5, false, "pred prvnim titulkem"},
      {2.2, true, "titulek 1 (Welcome to the past...)"},
      {10.0, true, "titulek 3"},
      {17.6, false, "mezi titulky 4 a 5"},
      {20.5, true, "titulek 5 (I invented nothing...)"},
      {34.0, true, "titulek 8 (One code...)"},
      {37.6, false, "po titulcich (defile na CRT)"},
      {43.8, false, "CRT 16-BIT (napis 16-BIT stejny)"},
      {57.0, true, "zaverecne titulky (scena 5: datum, (C))"},
  };
  std::vector<unsigned char> a((size_t)W * H * 4), b((size_t)W * H * 4), rad((size_t)W * 4);
  int glChyb = 0;
  for (const Cas &c : casy) {
    std::vector<unsigned char> *px[2] = {&a, &b};
    for (int j = 0; j < 2; j++) {
      beh[j]->demo.snimek(c.t, beh[j]->cil, beh[j]->fbo.fbo);
      glBindFramebuffer(GL_FRAMEBUFFER, beh[j]->fbo.fbo);
      glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px[j]->data());
      if (glGetError() != GL_NO_ERROR) glChyb++;
      for (int y = 0; y < H / 2; y++) {
        std::memcpy(rad.data(), &(*px[j])[(size_t)y * W * 4], W * 4);
        std::memcpy(&(*px[j])[(size_t)y * W * 4], &(*px[j])[(size_t)(H - 1 - y) * W * 4], W * 4);
        std::memcpy(&(*px[j])[(size_t)(H - 1 - y) * W * 4], rad.data(), W * 4);
      }
    }
    long long s = 0, roz = 0;
    for (size_t i = 0; i < a.size(); i += 4) {
      const int d = std::abs(a[i] - b[i]) + std::abs(a[i + 1] - b[i + 1]) + std::abs(a[i + 2] - b[i + 2]);
      s += d;
      if (d > 24) roz++;
    }
    const double prum = (double)s / ((double)W * H), podil = 100.0 * roz / ((double)W * H);
    char jm[64];
    std::snprintf(jm, sizeof jm, "%05.2f", c.t);
    pngZapis(ven + "/intro_cz_" + jm + ".png", W, H, a.data());
    pngZapis(ven + "/intro_en_" + jm + ".png", W, H, b.data());
    char t[300];
    std::snprintf(t, sizeof t, "t=%5.2f %-36s rozdil cesky / anglicky: prumer %.3f, jinych bodu %.2f %%", c.t, c.co, prum, podil);
    over(c.text ? podil > 0.2 : podil < 0.01, std::string(t) + (c.text ? " (ma se lisit)" : " (ma byt stejne)"));
  }
  over(glChyb == 0, "zadna chyba GL (" + std::to_string(glChyb) + ")");
  std::printf("VYSLEDEK: %d kontrol, %d CHYB\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
