// test_overeni/b304/demo_zvuk.cpp
// B304: cely zvuk dema (stejny C++ jako v appce) -> WAV + hlasitost po vterinach.
// Pouziti: demo_zvuk VYSTUP.wav
#include <chrono>
#include <cstdio>
#include <cmath>
#include <thread>
#include "../../app/src/main/cpp/intro/nap_demo_zvuk.h"

int main(int argc, char **argv) {
  const char *ven = argc > 1 ? argv[1] : "demo.wav";
  napdemo::zv::Stopa st;
  auto a = std::chrono::steady_clock::now();
  // stejne jako v appce: pocita se ve vlakne a hotovo() roste prubezne
  std::thread v([&] { st.vyrob(); });
  int pred = 0; double prvniVterina = -1;
  while (st.hotovo() < st.celkem()) {
    int h = st.hotovo();
    if (prvniVterina < 0 && h >= 44100) prvniVterina = std::chrono::duration<double>(std::chrono::steady_clock::now() - a).count();
    if (h == pred) std::this_thread::sleep_for(std::chrono::milliseconds(2));
    pred = h;
  }
  v.join();
  double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - a).count();
  std::printf("vyrobeno %d ramcu (%.1f s zvuku) za %.2f s = %.0fx rychleji nez prehravani; prvni vterina za %.3f s\n",
              st.celkem(), st.celkem() / 44100.0, s, st.celkem() / 44100.0 / s, prvniVterina);
  const int16_t *d = st.data();
  const int n = st.celkem();
  FILE *f = std::fopen(ven, "wb");
  auto u32 = [&](uint32_t x) { std::fwrite(&x, 4, 1, f); };
  auto u16 = [&](uint16_t x) { std::fwrite(&x, 2, 1, f); };
  std::fwrite("RIFF", 1, 4, f); u32(36 + n * 4); std::fwrite("WAVEfmt ", 1, 8, f);
  u32(16); u16(1); u16(2); u32(44100); u32(44100 * 4); u16(4); u16(16);
  std::fwrite("data", 1, 4, f); u32(n * 4); std::fwrite(d, 2, (size_t)n * 2, f); std::fclose(f);
  int orez = 0;
  for (int sec = 0; sec * 44100 < n; sec++) {
    double sum = 0; int pk = 0;
    for (int i = sec * 44100; i < std::min(n, (sec + 1) * 44100); i++) {
      for (int c = 0; c < 2; c++) { int x = d[i * 2 + c]; sum += (double)x * x; if (std::abs(x) > pk) pk = std::abs(x); if (std::abs(x) >= 31990) orez++; }
    }
    double rms = std::sqrt(sum / (44100.0 * 2));
    std::printf("%2d s  RMS %6.1f dBFS  spicka %6.1f dBFS\n", sec, 20 * std::log10(rms / 32768 + 1e-9), 20 * std::log10(pk / 32768.0 + 1e-9));
  }
  std::printf("vzorku na hranici (orez): %d\n", orez);
  return 0;
}
