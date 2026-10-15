// B309 (z B307): 16bitova ukazka na PC - snimky do PPM (prohlizeni, rychlost); B309: 6 s = snimky 36..396
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include "../../app/src/main/cpp/atari/nap_atari_roms.h"
#include "../../app/src/main/cpp/intro/nap_demo_16bit.h"
int main(int argc, char **argv) {
  const std::string adr = argc > 1 ? argv[1] : ".";
  napdemo::Hra16 h;
  auto t0 = std::chrono::steady_clock::now();
  h.init();
  auto t1 = std::chrono::steady_clock::now();
  std::printf("init %.1f ms\n", std::chrono::duration<double, std::milli>(t1 - t0).count());
  double max = 0, sum = 0; int pocet = 0;
  for (int n = 0; n <= 420; n++) {
    auto a = std::chrono::steady_clock::now();
    h.snimek(n);
    auto b = std::chrono::steady_clock::now();
    double ms = std::chrono::duration<double, std::milli>(b - a).count();
    max = std::max(max, ms); sum += ms; pocet++;
    bool uloz = false;
    for (int i = 2; i < argc; i++) if (std::atoi(argv[i]) == n) uloz = true;
    if (!uloz) continue;
    char jm[512]; std::snprintf(jm, sizeof jm, "%s/u16_%03d.ppm", adr.c_str(), n);
    FILE *o = std::fopen(jm, "wb");
    std::fprintf(o, "P6\n%d %d\n255\n", h.W, h.H);
    for (uint32_t c : h.obr) { unsigned char p[3] = {(unsigned char)c, (unsigned char)(c >> 8), (unsigned char)(c >> 16)}; std::fwrite(p, 1, 3, o); }
    std::fclose(o);
  }
  std::printf("snimek prumer %.2f ms, max %.2f ms\n", sum / pocet, max);
}
