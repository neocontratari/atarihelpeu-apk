// fuzz_b351.cpp - B351: fuzz PRIMO dekoderu archivu (nap::archiv::Archiv::rozbal na VSECHNY polozky).
// ../b349/fuzz_b349.cpp jde pres nap::AtariArchiv, ktery soubory .txt / .html ... vubec nerozbaluje (nejsou to hry)
// - PPMd v textovych souborech (ppmd_text, ppmd_pevny, compress_best, unicode) tak nezkousel. Tady se rozbali
// kazda polozka, cast pokusu na preskacku (pevny archiv = model / slovnik se stavi znovu od zacatku retezu).
// Cile poskozeni navic k fuzz_b349: prvnich 8 bajtu dat polozky (hlavicka bloku PPMd: priznaky, MaxMB, unikovy
// znak, 4 bajty rozsahoveho dekoderu / u LZ tabulky), nahodne bity v datech polozky, zkraceni uprostred dat.
//   g++ -std=c++17 -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -I../../app/src/main/cpp/atari
//       -o fuzz_b351 fuzz_b351.cpp                                           (vse na jednom radku)
//   SEMINKO=41 ./fuzz_b351 150 /tmp/b351_archivy/*.rar [dalsi archivy ...]
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <utility>
#include <vector>
#include "nap_archiv.h"

static std::vector<uint8_t> nacti(const char *c) {
  std::vector<uint8_t> d; FILE *f = fopen(c, "rb"); if (!f) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); fclose(f); return d;
}
struct Vzor { std::vector<uint8_t> d; std::vector<std::pair<size_t, size_t>> data; };   // zacatek a delka dat polozek

int main(int argc, char **argv) {
  if (argc < 3) { printf("pouziti: SEMINKO=n %s SEKUND ARCHIV...\n", argv[0]); return 2; }
  const int sekund = atoi(argv[1]);
  std::vector<Vzor> vzory;
  for (int i = 2; i < argc; i++) {
    Vzor v; v.d = nacti(argv[i]);
    if (v.d.size() < 16) continue;
    nap::archiv::Archiv a;
    if (a.otevri(v.d.data(), v.d.size()))
      for (const auto &p : a.polozky)
        if (!p.adresar && p.baleno > 0 && p.data < v.d.size()) v.data.push_back({p.data, (size_t)std::min<uint64_t>(p.baleno, v.d.size() - p.data)});
    vzory.push_back(std::move(v));
  }
  if (vzory.empty()) { printf("zadne vzory\n"); return 2; }
  std::mt19937 rng((unsigned)atoi(getenv("SEMINKO") ? getenv("SEMINKO") : "12345"));
  const auto t0 = std::chrono::steady_clock::now();
  long pokusu = 0, otevreno = 0, ok = 0, odmitnuto = 0; double nej = 0;
  while (std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count() < sekund) {
    const Vzor &vz = vzory[rng() % vzory.size()];
    std::vector<uint8_t> d = vz.d;
    int druh = (int)(rng() % 8);
    if (druh >= 5 && vz.data.empty()) druh = 0;
    const int n = 1 + (int)(rng() % 12);
    if (druh == 0) { for (int j = 0; j < n; j++) d[rng() % d.size()] ^= (uint8_t)(1 + rng() % 255); }
    else if (druh == 1) d.resize(rng() % d.size());
    else if (druh == 2) { const size_t a = rng() % d.size(); for (int j = 0; j < n && a + j < d.size(); j++) d[a + j] = (uint8_t)rng(); }
    else if (druh == 3) { const size_t p = 7 + rng() % (d.size() > 400 ? 400 : d.size() - 7); d[p] ^= (uint8_t)(1 << (rng() % 8)); }   // hlavicky
    else if (druh == 4) { const size_t p = rng() % d.size(); d.insert(d.begin() + (long)p, (size_t)(1 + rng() % 32), (uint8_t)rng()); }
    else {
      const auto &z = vz.data[rng() % vz.data.size()];
      if (druh == 5) {                                   // hlavicka bloku (PPMd: priznaky, MaxMB, esc, rozsahovy dekoder)
        const size_t okno = std::min<size_t>(z.second, 8);
        for (int j = 0; j < 1 + (int)(rng() % 2); j++) {
          const size_t p = z.first + rng() % okno;
          if (rng() % 2) d[p] ^= (uint8_t)(1 << (rng() % 8)); else d[p] = (uint8_t)rng();
        }
      } else if (druh == 6) { for (int j = 0; j < n; j++) d[z.first + rng() % z.second] ^= (uint8_t)(1 << (rng() % 8)); }
      else d.resize(z.first + rng() % z.second);         // zkraceni uprostred dat polozky
    }
    const auto t1 = std::chrono::steady_clock::now();
    nap::archiv::Archiv a;
    if (a.otevri(d.data(), d.size())) {
      otevreno++;
      std::vector<size_t> poradi;
      for (size_t i = 0; i < a.polozky.size(); i++) poradi.push_back(i);
      if (rng() % 3 == 0) std::shuffle(poradi.begin(), poradi.end(), rng);
      for (size_t i : poradi) {
        if (a.polozky[i].adresar) continue;
        std::vector<uint8_t> o;
        if (a.rozbal(i, o)) ok++; else odmitnuto++;
      }
    }
    nej = std::max(nej, std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t1).count());
    pokusu++;
  }
  printf("fuzz: %ld pokusu, %ld otevreno, polozek rozbaleno %ld (CRC sedi) / odmitnuto %ld, nejdelsi %.1f ms\n", pokusu, otevreno, ok, odmitnuto, nej);
  return 0;
}
