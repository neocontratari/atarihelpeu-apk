// test_overeni/b307/test_skip.cpp
// ====================================================================
//  B307: tlacitko SKIP - kresli ho C++ a C++ taky rozhoduje o dotyku
//  (Demo::dotykNaSkip). Tady se overi, ze NAKRESLENE tlacitko lezi presne
//  tam, kde ho hleda dotyk, na vysku, na sirku i na tabletu:
//    - stejny snimek dema se nakresli dvakrat - pro telefon (s tlacitkem)
//      a pro TV (bez tlacitka) - rozdil obou = jen tlacitko,
//    - obdelnik rozdilu musi lezet uvnitr Demo::skipObd a skoro ho vyplnit,
//    - stred nakresleneho tlacitka = dotyk "SKIP", kousek vedle = "jinam",
//    - pred 1,2 s a po 58,6 s (B309) tlacitko neni (demo jeste nezacalo / konci),
//    - na TV / PC se nekresli nikdy.
//   g++ -std=c++17 -O2 -I$INC -o test_skip test_skip.cpp -l:libEGL.so.1 -l:libGLESv2.so.2 -lz
//   EGL_PLATFORM=surfaceless ./test_skip           -> VYSTUP_test_skip.txt
// ====================================================================
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GLES2/gl2.h>
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

#include "../../app/src/main/cpp/intro/nap_demo.h"

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

static std::vector<unsigned char> soubor(const char *jm) {
  std::vector<unsigned char> d;
  FILE *f = std::fopen(jm, "rb");
  if (!f) return d;
  unsigned char b[65536];
  size_t n;
  while ((n = std::fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n);
  std::fclose(f);
  return d;
}

static int g_ok = 0, g_ch = 0;
static void kontrola(bool ok, const std::string &co) {
  std::printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str());
  (ok ? g_ok : g_ch)++;
}

// snimek t do RGBA (radky shora dolu)
static void kresli(napdemo::Demo &demo, napdemo::Cil &c, napdemo::Fbo &f, double t, std::vector<unsigned char> &px) {
  demo.snimek(t, c, f.fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);
  std::vector<unsigned char> g((size_t)c.w * c.h * 4);
  glReadPixels(0, 0, c.w, c.h, GL_RGBA, GL_UNSIGNED_BYTE, g.data());
  px.resize(g.size());
  for (int y = 0; y < c.h; y++) std::memcpy(&px[(size_t)y * c.w * 4], &g[(size_t)(c.h - 1 - y) * c.w * 4], (size_t)c.w * 4);
}

int main() {
  if (!glKontext()) { std::printf("EGL/GLES2 kontext se nepodaril\n"); return 1; }
  int lw = 0, lh = 0, lc = 0;
  unsigned char *logo = stbi_load("../../app/src/main/assets/design/nap_logo_original.png", &lw, &lh, &lc, 4);
  if (!logo) { std::printf("CHYBI LOGO\n"); return 1; }
  napdemo::Demo demo;
  const bool ok = demo.init(logo, lw, lh);
  stbi_image_free(logo);
  const std::vector<unsigned char> klip = soubor("../../app/src/main/assets/intro/pittkitt.klip");
  const bool kOk = !klip.empty() && demo.nastavKlip(klip.data(), klip.size());
  kontrola(ok && kOk, "demo pripraveno, klip Pitt-Kitt nacteny");
  if (!ok) return 1;

  struct Vel { int w, h; const char *jm; };
  const Vel VEL[] = {{540, 1170, "telefon na vysku"}, {1170, 540, "telefon na sirku"}, {800, 1280, "tablet"}};
  const double CASY[] = {7.3, 17.9, 36.8, 48.0};   // B309: demo 59 s
  char b[400];
  for (const Vel &v : VEL) {
    napdemo::Cil tel, tv;
    tel.w = tv.w = v.w; tel.h = tv.h = v.h;
    tel.tv = false; tv.tv = true;
    tel.meritko = tv.meritko = 0.5f;
    napdemo::Fbo f1, f2;
    f1.priprav(v.w, v.h, true);
    f2.priprav(v.w, v.h, true);
    demo.pripravTexty(tel, 100);
    demo.pripravTexty(tv, 100);
    float x0, y0, x1, y1;
    napdemo::Demo::skipObd(v.w, v.h, x0, y0, x1, y1);
    std::vector<unsigned char> a, c;
    for (double t : CASY) {
      kresli(demo, tel, f1, t, a);
      kresli(demo, tv, f2, t, c);
      int bx0 = v.w, by0 = v.h, bx1 = -1, by1 = -1;
      long long ruznych = 0;
      for (int y = 0; y < v.h; y++)
        for (int x = 0; x < v.w; x++) {
          const unsigned char *p = &a[((size_t)y * v.w + x) * 4], *q = &c[((size_t)y * v.w + x) * 4];
          if (std::abs(p[0] - q[0]) > 10 || std::abs(p[1] - q[1]) > 10 || std::abs(p[2] - q[2]) > 10) {
            ruznych++;
            bx0 = std::min(bx0, x); by0 = std::min(by0, y); bx1 = std::max(bx1, x); by1 = std::max(by1, y);
          }
        }
      const bool uvnitr = ruznych > 0 && bx0 >= x0 - 2 && by0 >= y0 - 2 && bx1 <= x1 + 2 && by1 <= y1 + 2;
      const bool vyplni = ruznych > 0 && (bx1 - bx0 + 1) >= 0.9f * (x1 - x0) && (by1 - by0 + 1) >= 0.9f * (y1 - y0);
      const float sx = (bx0 + bx1) * 0.5f, sy = (by0 + by1) * 0.5f;
      const bool dotyk = ruznych > 0 && napdemo::Demo::dotykNaSkip(v.w, v.h, sx, sy) &&
                         !napdemo::Demo::dotykNaSkip(v.w, v.h, bx0 - 0.08f * std::min(v.w, v.h), sy) &&
                         !napdemo::Demo::dotykNaSkip(v.w, v.h, sx, by1 + 0.08f * std::min(v.w, v.h));
      std::snprintf(b, sizeof b, "%s %dx%d t=%.1f: nakresleno [%d,%d]-[%d,%d] (%lld bodu), dotyk hleda [%.0f,%.0f]-[%.0f,%.0f]",
                    v.jm, v.w, v.h, t, bx0, by0, bx1, by1, ruznych, x0, y0, x1, y1);
      kontrola(uvnitr && vyplni && dotyk, std::string(b) + " - stred nakresleneho = SKIP, vedle = jinam");
    }
    // pred 1,2 s a po 58,6 s (B309) tlacitko neni; TV nikdy
    for (double t : {0.6, (double)napdemo::cas::KONEC - 0.2}) {   // B309: konec 59 s
      kresli(demo, tel, f1, t, a);
      kresli(demo, tv, f2, t, c);
      long long ruznych = 0;
      for (size_t i = 0; i < a.size(); i += 4)
        if (std::abs(a[i] - c[i]) > 10 || std::abs(a[i + 1] - c[i + 1]) > 10 || std::abs(a[i + 2] - c[i + 2]) > 10) ruznych++;
      std::snprintf(b, sizeof b, "%s t=%.1f: tlacitko neni (rozdil telefon / TV %lld bodu)", v.jm, t, ruznych);
      kontrola(ruznych == 0, b);
    }
    f1.uvolni();
    f2.uvolni();
  }
  const GLenum e = glGetError();
  kontrola(e == GL_NO_ERROR, "zadna chyba OpenGL");
  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_ch);
  return g_ch ? 1 : 0;
}
