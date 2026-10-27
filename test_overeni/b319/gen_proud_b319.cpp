// test_overeni/b319/gen_proud_b319.cpp
// B319: proud PS1 pro test stranky WEB TV v Chromiu (test_b319_prohlizec.py) - SKUTECNY Hub +
// KodekJpeg z appky, synteticka 2D strategie jako Dune 2000 (hra_mapa.h) v 2x Enhanced Resolution
// (736x480), posun mapy ruznymi smery, prohlizec obcas vynecha snimek.
//   gen_proud_b319 <adresar> <volby>     volby 3 = stranka B319 (kopie + polovicni), 0 = jako drive
// Zapise: proud.bin (4 B delka + paket, za sebou, presne jak je posila Java), snimky.txt (pro kazdy
// snimek: konec v proud.bin, pocet hlavnich paketu, kopii, polovicnich casti), snimek_<i>.raw
// (ocekavany obraz 0x00RRGGBB little endian) pro kontrolni body.
#include <cstdio>
#include <string>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"
#include "hra_mapa.h"

using namespace nap::proud;

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: gen_proud_b319 <adresar> <volby>\n"); return 2; }
  const std::string adr = argv[1];
  const int volby = std::atoi(argv[2]);
  Hub<KodekJpeg> hub("PS1", 44100);
  hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); }, "test");
  const int id = hub.otevri(volby);
  HraMapa hra;
  hra.vytvor(7);
  FILE *fp = std::fopen((adr + "/proud.bin").c_str(), "wb");
  FILE *fs = std::fopen((adr + "/snimky.txt").c_str(), "w");
  if (!fp || !fs) return 3;
  struct Faze { int snimku, dx, dy; bool vynechat; };
  const Faze faze[] = {{8, 0, 0, false}, {30, 8, 0, false}, {20, 0, 8, false}, {20, -8, -8, false},
                       {16, -16, 0, true}, {16, 0, -16, true}, {20, 0, 0, false}, {20, 4, 2, false}, {10, 0, 0, false}};
  std::vector<uint32_t> F;
  std::vector<uint8_t> pk;
  int W = 0, H = 0, i = 0;
  long long konec = 0;
  for (const Faze &f : faze) {
    for (int k = 0; k < f.snimku; k++) {
      hra.kurX = 150 + (int)(60 * std::sin(i * 0.11));
      hra.kurY = 120 + (int)(40 * std::cos(i * 0.07));
      hra.krok(f.dx, f.dy);
      if (f.vynechat && (k % 4) == 2) continue;
      hra.kresli(F, W, H, true);
      hub.snimek(F.data(), W, H);
      if (hub.dalsi(id, 200, 0, pk) != 1) return 4;
      std::fwrite(pk.data(), 1, pk.size(), fp);
      konec += (long long)pk.size();
      // kolik hlavnich paketu (bez priznaku 8), kopii a polovicnich casti
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
      FILE *fr = std::fopen((adr + "/snimek_" + std::to_string(i) + ".raw").c_str(), "wb");
      std::fwrite(F.data(), 4, F.size(), fr);
      std::fclose(fr);
      i++;
    }
  }
  std::fclose(fp);
  std::fclose(fs);
  hub.zavri(id);
  std::printf("%d snimku %dx%d, %lld bajtu\n%s", i, W, H, konec, hub.vezmiLog().c_str());
  return 0;
}
