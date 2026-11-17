// test_overeni/b335/scena_dune.h
// B335: "Dune 2000 na PS1 ve 2x" pro test vodorovnych carek na TV (Rene: "v dune vodorovne carky
// zustaly ale mazou se"; fotka B332: slabe zelene carky na pisku po ceste jednotek se zelenym pruhem
// zdravi, na radcich obrazu 16k-1).
//  - obraz 736x480 (hra 368x240, 2x Enhanced Resolution jako v logu: "obraz 368x240 ... 2x ANO"),
//  - pisek jako polygony s texturou ve 2x (gpu_neon): texely 2x2 body, ale posunute o bod proti
//    mrizce 2x2 a s jemnym zrnem - cast proto NENI presne 2x zvetsena (v logu Dune jen 790 z 4982
//    casti v polovicnim rozliseni), jde JPEGem v plnem rozliseni (do kvality 90 barvy 4:2:0),
//  - jednotky 12x12 bodu hry (24x24 ve 2x), nad nimi pruh zdravi 14x2 (28x4) jasne zeleny; jdou
//    sikmo (1 bod hry za 3 snimky) a obcas zmeni smer, panel vpravo stoji, kredity se meni.
// Barvy po 15 bitech jako PS1.
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

struct ScenaDune {
  static constexpr int W = 736, H = 480;      // obraz (2x)
  static constexpr int VW = 592;              // vyhled na mapu (296 bodu hry ve 2x), vpravo panel
  std::vector<uint32_t> pozadi;               // pisek + skaly + panel (bez jednotek)
  struct Jed { int x, y, vx, vy; uint32_t barva; };   // x, y = bod hry (vyhled 296x240)
  std::vector<Jed> jed;
  int tik = 0, kredity = 4370;
  uint32_t rs = 1;
  bool stoji = false;                         // jednotky stoji (cely obraz v klidu)

  uint32_t rnd() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
  static uint32_t c15(int r, int g, int b) {
    r = r < 0 ? 0 : (r > 255 ? 255 : r); g = g < 0 ? 0 : (g > 255 ? 255 : g); b = b < 0 ? 0 : (b > 255 ? 255 : b);
    return ((uint32_t)(r & 0xF8) << 16) | ((uint32_t)(g & 0xF8) << 8) | (uint32_t)(b & 0xF8);
  }
  static constexpr uint32_t ZELENA = (48u << 16) | (240u << 8) | 48u;   // pruh zdravi (c15(48,240,48))

  void vytvor(uint32_t seed) {
    rs = seed ? seed : 1;
    pozadi.assign((size_t)W * H, 0);
    // texely pisku (kazdy 2x2 body), posunute o 1 bod proti mrizce 2x2 (polygon ve 2x)
    const int TW = W / 2 + 2, TH = H / 2 + 2;
    std::vector<int> zrno((size_t)TW * TH);
    for (int ty = 0; ty < TH; ty++)
      for (int tx = 0; tx < TW; tx++) zrno[(size_t)ty * TW + tx] = (int)(rnd() % 13) - 6;
    for (int y = 0; y < H; y++)
      for (int x = 0; x < VW; x++) {
        const int tx = (x + 1) / 2, ty = (y + 1) / 2;
        const int z = zrno[(size_t)ty * TW + tx];
        const int vlna = ((x / 37 + y / 23) & 3) * 3;          // mirne duny
        pozadi[(size_t)y * W + x] = c15(214 + z + vlna, 186 + z + vlna, 120 + z / 2);
      }
    // par tmavsich skal (aby JPEG mel i hrany)
    for (int i = 0; i < 6; i++) {
      const int cx = 40 + (int)(rnd() % (VW - 80)), cy = 40 + (int)(rnd() % (H - 80)), r = 18 + (int)(rnd() % 30);
      for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++) {
          if (x < 0 || y < 0 || x >= VW || y >= H) continue;
          const int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
          if (d > r * r) continue;
          const int z = (int)(rnd() % 30) - 15;
          pozadi[(size_t)y * W + x] = c15(176 + z, 112 + z, 64 + z / 2);
        }
    }
    // panel vpravo (stoji)
    for (int y = 0; y < H; y++)
      for (int x = VW; x < W; x++) {
        const int v = (((x / 2) * 7 + (y / 2) * 3) & 15);
        pozadi[(size_t)y * W + x] = c15(90 + v, 70 + v, 40 + v);
      }
    jed.clear();
    const int smery[6][2] = {{1, 1}, {1, 1}, {-1, 1}, {1, -1}, {1, 1}, {-1, -1}};
    for (int i = 0; i < 6; i++)
      jed.push_back({20 + 40 * i, 20 + 30 * (i % 3), smery[i][0], smery[i][1],
                     c15(70 + 20 * i, 80 + 10 * i, 160 + 12 * i)});
  }

  void krok() {
    tik++;
    if (!stoji && tik % 3 == 0)
      for (Jed &j : jed) {
        j.x += j.vx; j.y += j.vy;
        if (j.x < 2 || j.x > 296 - 16) j.vx = -j.vx;
        if (j.y < 6 || j.y > 240 - 14) j.vy = -j.vy;
        if ((rnd() % 400) == 0) j.vx = -j.vx;
      }
    if (tik % 25 == 0) kredity += 5;
  }

  void kresli(std::vector<uint32_t> &F) {
    F = pozadi;
    auto bod = [&](int hx, int hy, uint32_t c) {      // bod hry -> 2x2 body obrazu
      if (hx < 0 || hy < 0 || hx >= VW / 2 || hy >= H / 2) return;
      uint32_t *r = &F[(size_t)(2 * hy) * W + 2 * hx];
      r[0] = r[1] = r[W] = r[W + 1] = c;
    };
    for (const Jed &j : jed) {
      for (int y = 0; y < 12; y++)
        for (int x = 0; x < 12; x++) {
          if ((x == 0 || x == 11) && (y == 0 || y == 11)) continue;
          const bool okraj = x == 0 || y == 0 || x == 11 || y == 11;
          bod(j.x + x, j.y + y, okraj ? c15(20, 20, 28) : (((x + y) & 3) ? j.barva : c15(230, 230, 240)));
        }
      for (int y = 0; y < 2; y++)
        for (int x = 0; x < 14; x++) bod(j.x - 1 + x, j.y - 4 + y, ZELENA);
    }
    // kredity vlevo nahore (meni se)
    for (int d = 0; d < 4; d++) {
      int c = kredity;
      for (int k = 0; k < 3 - d; k++) c /= 10;
      c %= 10;
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 6; x++) bod(8 + d * 8 + x, 8 + y, (x < 1 + c / 2 && y > 0) ? c15(250, 250, 160) : c15(60, 60, 50));
    }
  }
};
