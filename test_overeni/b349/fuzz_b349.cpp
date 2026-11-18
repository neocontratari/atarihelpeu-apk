// fuzz_b349.cpp - B349: dlouhy fuzz rozbalovani ZIP / RAR pod ASan / UBSan. Bere vzorove archivy (z
// vyrob_archivy.py, PoP RAR Reneho, libarchive test_read_format_rar_filter.rar ...), N sekund je nahodne
// poskozuje (prehozene bajty / bity v hlavickach i datech, zkraceni, vlozene bajty) a otevira pres
// nap::AtariArchiv (= rozbali vse, co by appka rozbalila). Chyba pameti = ASan zastavi s vypisem.
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer
//       -I../../app/src/main/cpp/atari -o fuzz_b349 fuzz_b349.cpp            (vse na jednom radku)
//   SEMINKO=1 ./fuzz_b349 240 /tmp/b349_archivy/*.rar /tmp/b349_archivy/*.zip [dalsi archivy ...]
#include <chrono>
#include <cstdlib>
#include <cstdio>
#include <random>
#include <string>
#include <vector>
#include "nap_atari_archiv.h"
static std::vector<uint8_t> nacti(const char *c) { std::vector<uint8_t> d; FILE *f = fopen(c, "rb"); if (!f) return d; int ch; while ((ch = fgetc(f)) != EOF) d.push_back((uint8_t)ch); fclose(f); return d; }
int main(int argc, char **argv) {
  const int sekund = atoi(argv[1]);
  std::vector<std::vector<uint8_t>> vzory;
  for (int i = 2; i < argc; i++) { auto d = nacti(argv[i]); if (!d.empty()) vzory.push_back(d); }
  std::mt19937 rng((unsigned)atoi(getenv("SEMINKO") ? getenv("SEMINKO") : "12345"));
  auto t0 = std::chrono::steady_clock::now();
  long pokusu = 0, ok = 0; double nej = 0;
  while (std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() < sekund) {
    std::vector<uint8_t> d = vzory[rng() % vzory.size()];
    const int druh = rng() % 6;
    const int n = 1 + rng() % 12;
    if (druh == 0) for (int j = 0; j < n; j++) d[rng() % d.size()] ^= (uint8_t)(1 + rng() % 255);
    else if (druh == 1) d.resize(rng() % d.size());
    else if (druh == 2) { size_t a = rng() % d.size(); for (int j = 0; j < n && a + j < d.size(); j++) d[a + j] = (uint8_t)rng(); }
    else if (druh == 3) { size_t p = 7 + rng() % (d.size() > 400 ? 400 : d.size() - 7); d[p] ^= (uint8_t)(1 << (rng() % 8)); }   // hlavicky
    else if (druh == 4) { size_t p = rng() % d.size(); d.insert(d.begin() + p, (size_t)(1 + rng() % 32), (uint8_t)rng()); }
    else { for (int j = 0; j < n; j++) d[d.size() / 3 + rng() % (d.size() / 3 + 1)] ^= (uint8_t)(1 << (rng() % 8)); }   // jen bit v datech
    auto t1 = std::chrono::steady_clock::now();
    nap::AtariArchiv a;
    if (a.otevri(d.data(), d.size(), "f")) ok++;
    nej = std::max(nej, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count());
    pokusu++;
  }
  printf("fuzz: %ld pokusu, %ld otevreno, nejdelsi %.1f ms\n", pokusu, ok, nej);
}
