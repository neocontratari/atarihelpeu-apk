// vybal.cpp ARCHIV [CILOVA_SLOZKA] - seznam a rozbaleni ZIP / RAR4 / RAR5 pres nap_archiv.h (nastroj k testum B349-B351)
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o vybal vybal.cpp
//   velke soubory (napr. libarchive ppmd_lzss_conversion 241 MB):  ... -DNAP_ARCHIV_LIMIT='(512u << 20)' ...
#include <cstdio>
#include <string>
#include <vector>
#include "nap_archiv.h"
int main(int argc, char **argv) {
  if (argc < 2) { printf("pouziti: %s ARCHIV [CILOVA_SLOZKA]\n", argv[0]); return 2; }
  FILE *f = fopen(argv[1], "rb");
  if (!f) { printf("CHYBA: nejde otevrit %s\n", argv[1]); return 2; }
  std::vector<uint8_t> d; int c; while ((c = fgetc(f)) != EOF) d.push_back((uint8_t)c); fclose(f);
  nap::archiv::Archiv a;
  if (!a.otevri(d.data(), d.size())) { printf("CHYBA otevreni: %s\n", a.chyba.c_str()); return 1; }
  printf("%s: %zu polozek\n", nap::archiv::Archiv::nazevTypu(a.typ), a.polozky.size());
  int chyb = 0;
  for (size_t i = 0; i < a.polozky.size(); i++) {
    const auto &p = a.polozky[i];
    std::vector<uint8_t> o;
    const bool ok = a.rozbal(i, o);
    printf("  %-32s %10llu B baleno %9llu metoda %02X verze %d -> %s %s\n", p.jmeno.c_str(), (unsigned long long)p.velikost, (unsigned long long)p.baleno,
           p.metoda, p.verze, ok ? "OK" : "CHYBA", ok ? "" : a.chyba.c_str());
    if (!ok && !p.adresar) chyb++;
    if (ok && argc > 2) { const std::string fn = std::string(argv[2]) + "/" + p.jmeno; FILE *o2 = fopen(fn.c_str(), "wb"); if (o2) { fwrite(o.data(), 1, o.size(), o2); fclose(o2); } }
  }
  return chyb ? 1 : 0;
}
