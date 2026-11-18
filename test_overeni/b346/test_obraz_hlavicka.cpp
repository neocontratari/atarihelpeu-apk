// B346: nap_ps1_obraz.h sama o sobe (bez nap_ps1_native.cpp) - preklad g++ i clang++ a par vypoctu
// g++ -std=c++17 -Wall -Wextra -I../../app/src/main/cpp -o PRAC/hlavicka test_obraz_hlavicka.cpp && PRAC/hlavicka
#include "nap_ps1_obraz.h"
#include <cassert>
int main() {
  assert(nap_obraz::sektor_stopy("  TRACK 01 MODE2/2352") == 2352);
  assert(nap_obraz::sektor_stopy("  TRACK 02 AUDIO") == 2352);
  assert(nap_obraz::sektor_stopy("  track 01 mode1/2048") == 2048);
  assert(nap_obraz::sektor_stopy("  TRACK 01 MODE2/2336") == 2336);
  assert(nap_obraz::sektor_stopy("  TRACK 01 CDG") == 2448);
  assert(nap_obraz::sektor_stopy("  TRACK 01 NECO") == 0 && nap_obraz::sektor_stopy(nullptr) == 0);
  assert(nap_obraz::vada("a.bin", 2352LL * 5, 2352).empty());
  assert(nap_obraz::vada("a.bin", 0, 2352).empty() && nap_obraz::vada("a.bin", -1, 2352).empty() && nap_obraz::vada("a.bin", 7, 0).empty());
  assert(nap_obraz::vada("cac.bin", 557574385LL, 2352) ==
         "cac.bin ma 557574385 B - neni cely pocet sektoru 2352 B (237063 celych + 2209 B, do celeho chybi 143 B)");
  assert(nap_obraz::vada_bez_cue("x.iso", 2048LL * 3).empty() && nap_obraz::vada_bez_cue("x.bin", 2352LL * 3).empty());
  assert(!nap_obraz::vada_bez_cue("x.bin", 2352LL * 3 + 1).empty());
  puts("OK nap_ps1_obraz.h sama: sektory stop (MODE2/2352, AUDIO, MODE1/2048 malymi, MODE2/2336, CDG, nezname), vada C&C presne, .iso/.bin bez .cue");
  return 0;
}
