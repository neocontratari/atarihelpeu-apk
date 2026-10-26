// test_overeni/b314/atari_jazyk.cpp
// ====================================================================
//  B314: PRISTROJ ATARI 130XE CESKY / ANGLICKY (C++ nap_atari_device.h, bez Androidu)
//  Jako render_b291 (skutecny obraz READY z jadra), na vysku i na sirku:
//   A) cesky,
//   B) prepnuto za chodu (devCtl "jazyk:en": atariJazykEn + Device::jazykZmenen),
//   C) anglicky od zapnuti (Java posle jazyk pred devStart),
//   D) zpet cesky za chodu.
//  Kontroly: A != B (prelozene stitky NET / GAMES, XEX / PHONE, LOG / ERROR, lista pod kazetou
//  OFF / STOPPED ...), B == C (prepnuti za chodu = zapnuti v anglictine), D == A (zpet presne
//  cesky), obraz Atari (obrazovka) se jazykem nemeni. PPM do VYSTUP (jen pro kontrolu ocima).
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o atari_jazyk atari_jazyk.cpp && ./atari_jazyk VYSTUP
// ====================================================================
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_device.h"
using namespace nap;

static int kontrol = 0, chyb = 0;
static void over(bool ok, const std::string &co) {
  kontrol++;
  if (!ok) chyb++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
}

static void ulozPpm(const std::string &jm, const std::vector<uint32_t> &px, int w, int h) {
  FILE *f = std::fopen(jm.c_str(), "wb");
  if (!f) return;
  std::fprintf(f, "P6 %d %d 255\n", w, h);
  for (uint32_t c : px) { std::fputc(c & 255, f); std::fputc((c >> 8) & 255, f); std::fputc((c >> 16) & 255, f); }
  std::fclose(f);
}

// podil ruznych bodu v obdelniku (procenta)
static double rozdil(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b, int w, int x0, int y0, int x1, int y1) {
  long long n = 0, r = 0;
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
      n++;
      if ((a[(size_t)y * w + x] & 0xFFFFFF) != (b[(size_t)y * w + x] & 0xFFFFFF)) r++;
    }
  return n ? 100.0 * r / n : 0.0;
}

int main(int argc, char **argv) {
  const std::string ven = argc > 1 ? argv[1] : ".";
  Machine *m = new Machine();
  AnticView *v = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM; m->view = v;
  m->reset(); m->consol = 7;
  for (int f = 0; f < 600; f++) m->runFrame();
  const long long now = 100000;
  struct Orient { int w, h; const char *jm; };
  const Orient o[2] = {{1080, 2160, "na vysku"}, {2160, 1080, "na sirku"}};
  for (const Orient &or_ : o) {
    const int W = or_.w, H = or_.h;
    std::vector<uint32_t> A((size_t)W * H), B((size_t)W * H), C((size_t)W * H), D((size_t)W * H);
    atariJazykEn().store(false);
    dev::Device *d = new dev::Device();
    d->layout(W, H);
    d->setAtariFrame(v->fb);
    d->update(now);
    d->present(A.data(), W, now);
    atariJazykEn().store(true);                 // devCtl "jazyk:en" za chodu (nap_atari_native.cpp)
    d->jazykZmenen();
    d->update(now);
    d->present(B.data(), W, now);
    atariJazykEn().store(true);                 // zapnuti uz v anglictine
    dev::Device *d2 = new dev::Device();
    d2->layout(W, H);
    d2->setAtariFrame(v->fb);
    d2->update(now);
    d2->present(C.data(), W, now);
    atariJazykEn().store(false);                // zpet cesky za chodu
    d->jazykZmenen();
    d->update(now);
    d->present(D.data(), W, now);
    const double ab = rozdil(A, B, W, 0, 0, W, H), bc = rozdil(B, C, W, 0, 0, W, H), ad = rozdil(A, D, W, 0, 0, W, H);
    char t[300];
    std::snprintf(t, sizeof t, "%s %dx%d: cesky / anglicky jinych bodu %.3f %% (stitky a hlasky)", or_.jm, W, H, ab);
    over(ab > 0.05 && ab < 8.0, t);
    std::snprintf(t, sizeof t, "%s: prepnuti za chodu = zapnuti v anglictine (jinych bodu %.4f %%)", or_.jm, bc);
    over(bc == 0.0, t);
    std::snprintf(t, sizeof t, "%s: zpet cesky za chodu = puvodni cesky obraz (jinych bodu %.4f %%)", or_.jm, ad);
    over(ad == 0.0, t);
    if (W < H) {
      // obrazovka Atari (du 100,95 - 831,720) se jazykem nemeni, servisni lista dole ano
      const double scr = rozdil(A, B, W, (int)d->X(100), (int)d->Y(95), (int)d->X(831), (int)d->Y(720));
      const double svc = rozdil(A, B, W, (int)d->X(25), (int)d->Y(1523), (int)d->X(915), (int)d->Y(1629));
      std::snprintf(t, sizeof t, "na vysku: obrazovka Atari stejna (%.4f %%), servisni lista prelozena (%.2f %% jinych bodu)", scr, svc);
      over(scr == 0.0 && svc > 0.5, t);
    }
    ulozPpm(ven + "/atari_" + std::to_string(W) + "x" + std::to_string(H) + "_cs.ppm", A, W, H);
    ulozPpm(ven + "/atari_" + std::to_string(W) + "x" + std::to_string(H) + "_en.ppm", B, W, H);
    delete d;
    delete d2;
  }
  std::printf("VYSLEDEK: %d kontrol, %d CHYB\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
