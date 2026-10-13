// test_overeni/b307/demo_zvuk.cpp
// B304/B307: cely zvuk dema (stejny C++ jako v appce) -> WAV + hlasitost po vterinach.
// B307: do stopy se primicha zvuk Reneho hry z klipu (jako v appce v introStart).
// Pouziti: demo_zvuk VYSTUP.wav [KLIP]
#include <chrono>
#include <cstdio>
#include <cmath>
#include <thread>
#include <vector>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#include "../../app/src/main/cpp/vendor/stb/stb_image.h"
#include "../../app/src/main/cpp/intro/nap_demo_zvuk.h"
#include "../../app/src/main/cpp/intro/nap_demo_klip.h"

int main(int argc, char **argv) {
  const char *ven = argc > 1 ? argv[1] : "demo.wav";
  const char *kp = argc > 2 ? argv[2] : "../../app/src/main/assets/intro/pittkitt.klip";
  napdemo::zv::Stopa st;
  {
    std::vector<unsigned char> d;
    FILE *f = std::fopen(kp, "rb");
    if (f) { unsigned char b[65536]; size_t n; while ((n = std::fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); std::fclose(f); }
    napdemo::KlipAtari k;
    std::vector<int16_t> z;
    if (k.nacti(d.data(), d.size()) && k.zvukUseku(2, 6, napdemo::cas::STRIH_16 - napdemo::cas::ATARI_JIZDA, z)) {
      st.nastavZaznam(z.data(), z.size(), napdemo::cas::ATARI_JIZDA, 1.8f);
      std::printf("zvuk hry z klipu: %zu vzorku (%.2f s) od %.2f s\n", z.size(), z.size() / 44100.0, napdemo::cas::ATARI_JIZDA);
    } else {
      std::printf("klip %s se nenacetl - bez zvuku hry\n", kp);
    }
  }
  auto a = std::chrono::steady_clock::now();
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
  for (int pul = 0; pul * 22050 < n; pul++) {
    double sum = 0; int pk = 0;
    for (int i = pul * 22050; i < std::min(n, (pul + 1) * 22050); i++) {
      for (int c = 0; c < 2; c++) { int x = d[i * 2 + c]; sum += (double)x * x; if (std::abs(x) > pk) pk = std::abs(x); if (std::abs(x) >= 31990) orez++; }
    }
    double rms = std::sqrt(sum / (22050.0 * 2));
    if (pul >= 52) std::printf("%5.1f s  RMS %6.1f dBFS  spicka %6.1f dBFS\n", pul * 0.5, 20 * std::log10(rms / 32768 + 1e-9), 20 * std::log10(pk / 32768.0 + 1e-9));
  }
  std::printf("vzorku na hranici (orez): %d\n", orez);
  return 0;
}
