// test_overeni/b335/gen_proud_b335.cpp
// B335: proud PS1 pro test stranky WEB TV v Chromiu (test_b335_prohlizec.py) - SKUTECNY Hub + KodekJpeg
// z appky, scena "Dune 2000 ve 2x" (scena_dune.h: pisek, jednotky se zelenym pruhem zdravi jdou sikmo),
// slaba Wi-Fi (kvalita 55 jako v logu Reneho), 600 snimku pohyb, pak 120 snimku stoji.
//   gen_proud_b335 <adresar> <rezim>     rezim 0 = jako B334 (barvy 4:2:0), 1 = B335
// Zapise: proud.bin (4 B delka + paket, za sebou, jak je posila Java), snimky.txt (pro kazdy snimek:
// cislo, konec v proud.bin, pocet hlavnich paketu, kopii, polovicnich casti), snimek_<i>.raw (ocekavany
// obraz 0x00RRGGBB) jen pro kontrolni body BODY.
#include <cstdio>
#include <string>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"
#include "scena_dune.h"

using namespace nap::proud;

static const int BODY[] = {150, 300, 450, 599, 719};

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: gen_proud_b335 <adresar> <rezim 0|1>\n"); return 2; }
  const std::string adr = argv[1];
  const bool b335 = std::atoi(argv[2]) != 0;
  Hub<KodekJpeg> hub("PS1", 44100);
  hub.nastavKodek([](KodekJpeg &k) { k.qStrop = 55; });
  hub.nastavVolby([b335](KodekJpeg &k, int v) { nastavKodekPs1(k, v); if (!b335) k.plneBarvyDo = 0; }, "test");
  const int id = hub.otevri(3);          // stranka WEB TV si rika o volby (v=319 a vys)
  ScenaDune s;
  s.vytvor(5);
  FILE *fp = std::fopen((adr + "/proud.bin").c_str(), "wb");
  FILE *fs = std::fopen((adr + "/snimky.txt").c_str(), "w");
  if (!fp || !fs) return 3;
  std::vector<uint32_t> F;
  std::vector<uint8_t> pk;
  long long konec = 0;
  for (int i = 0; i < 720; i++) {
    s.stoji = i >= 600;
    s.krok();
    s.kresli(F);
    hub.snimek(F.data(), ScenaDune::W, ScenaDune::H);
    if (hub.dalsi(id, 200, 0, pk) != 1) return 4;
    std::fwrite(pk.data(), 1, pk.size(), fp);
    konec += (long long)pk.size();
    int hlavnich = 0, kopii = 0, pol = 0;
    for (size_t o = 0; o + 4 <= pk.size();) {
      const uint32_t len = pk[o] | (pk[o + 1] << 8) | (pk[o + 2] << 16) | ((uint32_t)pk[o + 3] << 24);
      const uint8_t *p = &pk[o + 4];
      if (!(p[3] & 8)) hlavnich++;
      const uint8_t kv = p[28 + 8];
      if (kv == 0xFF) kopii++;
      else if (kv & 0x80) pol++;
      o += 4 + len;
    }
    std::fprintf(fs, "%d %lld %d %d %d\n", i, konec, hlavnich, kopii, pol);
    for (int b : BODY)
      if (b == i) {
        FILE *fr = std::fopen((adr + "/snimek_" + std::to_string(i) + ".raw").c_str(), "wb");
        std::fwrite(F.data(), 4, F.size(), fr);
        std::fclose(fr);
      }
  }
  std::fclose(fp);
  std::fclose(fs);
  hub.zavri(id);
  std::printf("%s: 720 snimku %dx%d, %lld bajtu\n%s", b335 ? "B335" : "B334", ScenaDune::W, ScenaDune::H, konec, hub.vezmiLog().c_str());
  return 0;
}
