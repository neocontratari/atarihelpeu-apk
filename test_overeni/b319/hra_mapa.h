// test_overeni/b319/hra_mapa.h
// B319: syntetická "2D strategie jako Dune 2000 na PS1" pro testy proudu na TV / PC.
// Obraz 368x240 (jako Dune 2000 v logu Reneho: "obraz 368x240"), vlevo vyhled na mapu 296x240,
// vpravo panel 72 bodu (ikony, kredity, mala mapa s ramečkem kamery). Mapa 1280x1024 bodu:
// pisek se zrnem, skaly, koreni (oranzove tecky), budovy; jednotky jezdi; kurzor.
// Barvy po 15 bitech jako PS1. kresli(..., dvakrat = true) = 2x Enhanced Resolution u 2D hry:
// kazdy bod 2x2 (736x480) - jako jadro PCSX u spritu.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

struct HraMapa {
  static constexpr int MW = 1280, MH = 1024;          // svet (body hry)
  static constexpr int OW = 368, OH = 240;            // obraz hry
  static constexpr int VW = 296, VH = 240;            // vyhled na mapu
  std::vector<uint32_t> svet, obraz;
  int camX = 400, camY = 300;
  int kurX = 150, kurY = 120;
  int kredity = 1500, tik = 0;
  struct Jed { int x, y, vx, vy; uint32_t b1, b2; };
  std::vector<Jed> jed;
  uint32_t rs = 1;

  uint32_t rnd() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
  static uint32_t c15(int r, int g, int b) {
    r = r < 0 ? 0 : (r > 255 ? 255 : r); g = g < 0 ? 0 : (g > 255 ? 255 : g); b = b < 0 ? 0 : (b > 255 ? 255 : b);
    return ((uint32_t)(r & 0xF8) << 16) | ((uint32_t)(g & 0xF8) << 8) | (uint32_t)(b & 0xF8);
  }

  void vytvor(uint32_t seed) {
    rs = seed ? seed : 1;
    svet.assign((size_t)MW * MH, 0);
    // pisek se zrnem
    for (int y = 0; y < MH; y++)
      for (int x = 0; x < MW; x++) {
        const int z = (int)(rnd() % 40) - 20, z2 = (int)(rnd() % 12) - 6;
        svet[(size_t)y * MW + x] = c15(200 + z, 160 + z + z2, 100 + z / 2);
      }
    // skaly (kruhove ostrovy s tmavsim zrnem a okrajem)
    for (int i = 0; i < 70; i++) {
      const int cx = (int)(rnd() % MW), cy = (int)(rnd() % MH), r = 20 + (int)(rnd() % 60);
      for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++) {
          if (x < 0 || y < 0 || x >= MW || y >= MH) continue;
          const int d = (x - cx) * (x - cx) + (y - cy) * (y - cy);
          if (d > r * r) continue;
          const int z = (int)(rnd() % 50) - 25;
          svet[(size_t)y * MW + x] = d > (r - 3) * (r - 3) ? c15(70, 55, 40) : c15(120 + z, 100 + z, 80 + z);
        }
    }
    // koreni
    for (int i = 0; i < 9000; i++) {
      const int x = (int)(rnd() % (MW - 2)), y = (int)(rnd() % (MH - 2));
      const uint32_t c = c15(230, 110 + (int)(rnd() % 40), 30);
      svet[(size_t)y * MW + x] = c; svet[(size_t)y * MW + x + 1] = c; svet[(size_t)(y + 1) * MW + x] = c;
    }
    // budovy 32x32 se vzorem
    for (int i = 0; i < 40; i++) {
      const int bx = (int)(rnd() % (MW - 40)), by = (int)(rnd() % (MH - 40));
      for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
          const bool okraj = x == 0 || y == 0 || x == 31 || y == 31;
          const int v = ((x / 4 + y / 4) & 1) ? 40 : 0;
          svet[(size_t)(by + y) * MW + bx + x] = okraj ? c15(30, 30, 30) : c15(90 + v, 90 + v, 110 + v);
        }
    }
    jed.clear();
    for (int i = 0; i < 40; i++)
      jed.push_back({(int)(rnd() % MW), (int)(rnd() % MH), (int)(rnd() % 3) - 1, (int)(rnd() % 3) - 1,
                     c15(40 + (int)(rnd() % 200), 40 + (int)(rnd() % 200), 40 + (int)(rnd() % 200)), c15(240, 240, 240)});
  }

  // jeden snimek hry (hra kresli 20x / s): posun kamery, jednotky, animace
  void krok(int dx, int dy) {
    camX = std::max(0, std::min(MW - VW, camX + dx));
    camY = std::max(0, std::min(MH - VH, camY + dy));
    for (Jed &j : jed) {
      j.x = (j.x + j.vx + MW) % MW; j.y = (j.y + j.vy + MH) % MH;
      if ((rnd() & 63) == 0) { j.vx = (int)(rnd() % 3) - 1; j.vy = (int)(rnd() % 3) - 1; }
    }
    if ((tik % 25) == 0) kredity += 25;
    tik++;
  }

  void kresli(std::vector<uint32_t> &F, int &W, int &H, bool dvakrat) {
    obraz.assign((size_t)OW * OH, 0);
    // vyhled na mapu
    for (int y = 0; y < VH; y++) std::memcpy(&obraz[(size_t)y * OW], &svet[(size_t)(camY + y) * MW + camX], (size_t)VW * 4);
    // jednotky (12x12 s okrajem)
    for (const Jed &j : jed) {
      const int sx = j.x - camX, sy = j.y - camY;
      if (sx < -12 || sy < -12 || sx >= VW || sy >= VH) continue;
      for (int y = 0; y < 12; y++)
        for (int x = 0; x < 12; x++) {
          const int px = sx + x, py = sy + y;
          if (px < 0 || py < 0 || px >= VW || py >= VH) continue;
          const bool okraj = x == 0 || y == 0 || x == 11 || y == 11;
          if ((x == 0 || x == 11) && (y == 0 || y == 11)) continue;   // kulate rohy
          obraz[(size_t)py * OW + px] = okraj ? c15(10, 10, 10) : (((x + y) & 3) ? j.b1 : j.b2);
        }
    }
    // kurzor (sipka 10x10)
    for (int y = 0; y < 10; y++)
      for (int x = 0; x <= y && x < 10; x++) {
        const int px = kurX + x, py = kurY + y;
        if (px >= 0 && py >= 0 && px < VW && py < VH) obraz[(size_t)py * OW + px] = (x == 0 || x == y || y == 9) ? c15(0, 0, 0) : c15(255, 255, 255);
      }
    // panel vpravo
    for (int y = 0; y < OH; y++)
      for (int x = VW; x < OW; x++) {
        const int v = ((x * 7 + y * 3) & 15);
        obraz[(size_t)y * OW + x] = c15(50 + v, 50 + v, 64 + v);
      }
    // mala mapa 64x51 nahore v panelu + ramecek kamery
    for (int y = 0; y < 51; y++)
      for (int x = 0; x < 64; x++) obraz[(size_t)(6 + y) * OW + VW + 4 + x] = svet[(size_t)(y * 20) * MW + x * 20];
    {
      const int rx = VW + 4 + camX / 20, ry = 6 + camY / 20, rw = VW / 20, rh = VH / 20;
      for (int x = 0; x <= rw; x++) { obraz[(size_t)ry * OW + rx + x] = c15(255, 255, 255); obraz[(size_t)(ry + rh) * OW + rx + x] = c15(255, 255, 255); }
      for (int y = 0; y <= rh; y++) { obraz[(size_t)(ry + y) * OW + rx] = c15(255, 255, 255); obraz[(size_t)(ry + y) * OW + rx + rw] = c15(255, 255, 255); }
    }
    // ikony staveb (staticke)
    for (int i = 0; i < 6; i++)
      for (int y = 0; y < 24; y++)
        for (int x = 0; x < 28; x++)
          obraz[(size_t)(70 + i * 27 + y) * OW + VW + 6 + (i & 1) * 32 + x] = c15(60 + i * 30 + x * 3, 120 + y * 4, 200 - i * 20);
    // kredity (cislice jako svisle carky)
    for (int d = 0; d < 5; d++) {
      const int c = (kredity / (int)std::pow(10, 4 - d)) % 10;
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 6; x++) obraz[(size_t)(228 + y) * OW + VW + 8 + d * 8 + x] = (x < 1 + c / 2 && y > 0) ? c15(250, 250, 120) : c15(20, 20, 20);
    }
    if (!dvakrat) { W = OW; H = OH; F = obraz; return; }
    W = OW * 2; H = OH * 2;
    F.resize((size_t)W * H);
    for (int y = 0; y < OH; y++) {
      uint32_t *r = &F[(size_t)(2 * y) * W];
      for (int x = 0; x < OW; x++) r[2 * x] = r[2 * x + 1] = obraz[(size_t)y * OW + x];
      std::memcpy(r + W, r, (size_t)W * 4);
    }
  }
};
