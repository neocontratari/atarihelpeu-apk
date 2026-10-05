// B291: vykresli zarizeni (nap_atari_device.h) do PPM pro vizualni kontrolu
// proti Chromium renderu schvaleneho navrhu. Volitelne: stav (stisky).
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o render_b291 render_b291.cpp
//   ./render_b291 vystup.ppm sirka [scenar]
#define STB_TRUETYPE_IMPLEMENTATION
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_device.h"
using namespace nap;

static double ms(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b) {
  return std::chrono::duration<double, std::milli>(b - a).count();
}
int main(int argc, char **argv) {
  const char *out = argc > 1 ? argv[1] : "dev.ppm";
  int devW = argc > 2 ? atoi(argv[2]) : 1000;
  int scen = argc > 3 ? atoi(argv[3]) : 0;
  int bootFrames = argc > 4 ? atoi(argv[4]) : 600;
  // skutecny obraz Atari (READY) z jadra
  Machine *m = new Machine(); AnticView *v = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM; m->view = v;
  m->reset(); m->consol = 7;
  for (int f = 0; f < bootFrames; f++) m->runFrame();

  dev::Device *d = new dev::Device();
  float s = devW / 941.f;
  int H = (int)std::ceil(1672 * s) + 1;
  auto t0 = std::chrono::steady_clock::now();
  if (scen >= 100) d->layout(devW, (int)(devW * 2.0f));     // skutecne rozlozeni jako na telefonu
  else d->layoutFixed(devW, H, s, 0, 0);
  auto t1 = std::chrono::steady_clock::now();
  d->setAtariFrame(v->fb);
  long long now = 100000;
  dev::Ev ev[8]; int sa = 0;
  auto tapAt = [&](float ux, float uy, int pid) { return d->pointerDown(pid, d->X(ux), d->Y(uy), now, ev, 8); };
  int sc = scen % 100;
  if (sc == 1) {               // stisky: klavesa A, SHIFT zamceny, START drzeny, PLAY, REC, svc XEX
    tapAt(141 + 20, 1034 + 30, 0);                // A drzeno
    tapAt(41 + 30, 1101 + 30, 1); d->pointerUp(1, 0, 0, now, ev, 8, &sa);   // SHIFT zamek
    tapAt(436.4f + 120, 740 + 30, 2);             // START drzeno
    tapAt(383 + 40, 1440 + 30, 3); d->pointerUp(3, 0, 0, now, ev, 8, &sa);  // PLAY
    tapAt(223 + 40, 1440 + 30, 4); d->pointerUp(4, 0, 0, now, ev, 8, &sa);  // REC
    tapAt(42 + 111 + 50, 1537 + 40, 5);           // XEX drzeno
    tapAt(855 + 18, 1328 + 20, 6);                // nulovani drzeno
    d->counter = 123;
  } else if (sc == 2) {        // vypnuto
    tapAt(8 + 17, 393 + 25, 0); d->pointerUp(0, 0, 0, now, ev, 8, &sa);
    now += 1000;
  } else if (sc == 3) {        // dvirka otevrena
    tapAt(627 + 40, 1440 + 30, 0); d->pointerUp(0, 0, 0, now, ev, 8, &sa);
    now += 1000;
  }
  d->update(now);
  auto t2 = std::chrono::steady_clock::now();
  d->render(now);
  auto t3 = std::chrono::steady_clock::now();
  std::vector<uint32_t> outb((size_t)d->W * d->H);
  d->present(outb.data(), d->W, now);
  auto t4 = std::chrono::steady_clock::now();
  // ustaleny beh: 100 snimku, kazdy s JINYM obrazem Atari (nejhorsi pripad -
  // obrazovka se prekresluje cela kazdy snimek) + kopie zmenene oblasti
  std::vector<uint32_t> win((size_t)d->W * d->H);
  double sum = 0, worst = 0; long long area = 0;
  std::vector<uint32_t> fbx(v->fb, v->fb + AnticView::W * AnticView::H);
  for (int i = 0; i < 100; i++) {
    fbx[(i * 7919) % fbx.size()] ^= 0x00FFFFFFu;
    auto a0 = std::chrono::steady_clock::now();
    d->setAtariFrame(fbx.data());
    d->update(now + 20 * (i + 1));
    d->render(now + 20 * (i + 1));
    dev::IRect pr = d->takePresentRect();
    d->copyOut(win.data(), d->W, pr);
    auto a1 = std::chrono::steady_clock::now();
    double t = ms(a0, a1); sum += t; if (t > worst) worst = t;
    area += (long long)(pr.x1 - pr.x0) * (pr.y1 - pr.y0);
  }
  fprintf(stderr, "layout+pozadi %.1f ms, prvni render %.1f ms, present %.1f ms | ustaleny snimek prumer %.2f ms, nejhorsi %.2f ms, kopie %.0f%% plochy  W=%d H=%d\n",
          ms(t0, t1), ms(t2, t3), ms(t3, t4), sum / 100, worst, 100.0 * area / 100 / ((double)d->W * d->H), d->W, d->H);
  FILE *f = fopen(out, "wb");
  fprintf(f, "P6 %d %d 255\n", d->W, d->H);
  for (size_t i = 0; i < outb.size(); i++) { uint32_t c = outb[i]; fputc(c & 255, f); fputc((c >> 8) & 255, f); fputc((c >> 16) & 255, f); }
  fclose(f);
  return 0;
}
