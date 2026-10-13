// B305 test: synteticka hra jako Sonic pro test prenosu Segy (dve vrstvy
// dlazdic s paralaxou, vlnici se voda, postavicky, skore, kolobeh barev,
// stmivani; levy sloupec vypnuty jako v nap_real_scanline_rendered).
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

// =====================================================================
//  SEGA - synteticka hra (dlazdice, paralaxa, postavicky)
// =====================================================================
struct Rng { uint32_t s; explicit Rng(uint32_t x) : s(x) {} uint32_t operator()() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; } };

static uint32_t mdBarva(unsigned c9) {   // stejne jako nap_md_colour_to_argb (3 bity na slozku)
  const int r3 = c9 & 7, g3 = (c9 >> 3) & 7, b3 = (c9 >> 6) & 7;
  const int r = (r3 << 5) | (r3 << 2) | (r3 >> 1), g = (g3 << 5) | (g3 << 2) | (g3 >> 1), b = (b3 << 5) | (b3 << 2) | (b3 >> 1);
  return 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

struct SegaHra {
  int W = 320, H = 224;
  unsigned cram[64];
  uint8_t dlazdice[400][64];                 // 4bit indexy
  struct Pole { uint16_t t; uint8_t pal; bool hf, vf; };
  Pole mapaA[32][64], mapaB[32][64];
  struct Sprite { double x, y, vx, vy; int t0, pal; bool svet; };
  std::vector<Sprite> sprity;
  int snimek = 0;
  double kamX = 0, kamY = 0, rychlost = 0;
  int ztmaveni = 0;                          // 0..7 (stmivani: kolik dilku ubrat)
  Rng rng{12345};

  SegaHra() {
    for (int i = 0; i < 64; i++) cram[i] = rng() & 0x1FF;
    cram[0] = 0x1C0 | 0x30;                  // modra obloha
    // dlazdice: prechody (nebe), cihly, sachovnice (hlina), trava, voda, nahodne
    for (int t = 0; t < 400; t++)
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++) {
          uint8_t v;
          if (t < 16) v = (uint8_t)(1 + ((y + t) / 3) % 4);
          else if (t < 48) v = (uint8_t)(((x / 4 + y / 2 + t) & 1) ? 5 + t % 3 : 8);
          else if (t < 80) v = (uint8_t)(((x / 4) ^ (y / 4)) & 1 ? 9 : 10);
          else if (t < 112) v = (uint8_t)(y < 3 ? 11 + (x + t) % 2 : 12 + ((x * 3 + y + t) % 3));
          else if (t < 140) v = (uint8_t)(13 + ((x + y * 2 + t) % 3));
          else v = (uint8_t)(rng() % 16);
          dlazdice[t][y * 8 + x] = v;
        }
    for (int t = 300; t < 400; t++) for (int i = 0; i < 64; i++) dlazdice[t][i] = (uint8_t)((rng() % 3) ? rng() % 16 : 0);   // sprity s pruhlednymi body
    for (int my = 0; my < 32; my++)
      for (int mx = 0; mx < 64; mx++) {
        Pole &b = mapaB[my][mx];
        if (my < 8) { b.t = (uint16_t)(my * 2 % 16); b.pal = 0; }
        else if (my < 14) { b.t = (uint16_t)(16 + ((mx * 7 + my * 3) % 32)); b.pal = 1; }
        else { b.t = (uint16_t)(112 + ((mx + my) % 28)); b.pal = 1; }
        b.hf = (mx * 5 + my) % 7 == 0; b.vf = false;
        Pole &a = mapaA[my][mx];
        const int zem = 22 + (int)(3 * std::sin(mx * 0.2));
        if (my >= zem) { a.t = (uint16_t)(my == zem ? 80 + mx % 32 : 48 + (mx + my) % 32); a.pal = 2; }
        else if ((mx % 16 >= 4 && mx % 16 < 9 && my == 14) || (mx % 23 == 3 && my > 10 && my < zem)) { a.t = (uint16_t)(140 + (mx * 13 + my * 7) % 160); a.pal = 3; }
        else { a.t = 0xFFFF; a.pal = 0; }
        a.hf = a.vf = false;
      }
    for (int i = 0; i < 10; i++)
      sprity.push_back({(double)(rng() % 600), (double)(40 + rng() % 140), (double)((int)(rng() % 5) - 2) * 0.7, (double)((int)(rng() % 3) - 1) * 0.5, 300 + (int)(rng() % 80), 3, i != 0});
    sprity[0].x = 140; sprity[0].y = 130; sprity[0].svet = false;    // hrac uprostred obrazovky
  }
  uint32_t barva(int pal, int idx) const {
    unsigned c = cram[pal * 16 + idx];
    int r = c & 7, g = (c >> 3) & 7, b = (c >> 6) & 7;
    r = std::max(0, r - ztmaveni); g = std::max(0, g - ztmaveni); b = std::max(0, b - ztmaveni);
    return mdBarva((unsigned)(r | (g << 3) | (b << 6)));
  }
  int bodVrstvy(const Pole (*mapa)[64], int px, int py, int &pal) const {
    px &= 511; py &= 255;
    const Pole &p = mapa[py >> 3][px >> 3];
    if (p.t == 0xFFFF) return 0;
    int x = px & 7, y = py & 7;
    if (p.hf) x = 7 - x;
    if (p.vf) y = 7 - y;
    pal = p.pal;
    return dlazdice[p.t % 400][y * 8 + x];
  }
  void krok() {
    snimek++;
    rychlost = std::min(6.0, rychlost + 0.08);
    kamX += rychlost;
    kamY = 8 * std::sin(snimek * 0.03);
    for (auto &s : sprity) { s.x += s.vx; s.y += s.vy; if (s.y < 20 || s.y > 180) s.vy = -s.vy; }
    if (snimek % 8 == 0) { const unsigned t = cram[2 * 16 + 10]; for (int i = 10; i < 13; i++) cram[2 * 16 + i] = cram[2 * 16 + i + 1]; cram[2 * 16 + 13] = t; }
  }
  // barva palety ve formatu CRAM Mega Drive (0x0BGR, 3 bity) - pro nap_real_colour_updated
  unsigned cramSlovo(int i) const {
    const unsigned c = cram[i];
    int r = c & 7, g = (c >> 3) & 7, b = (c >> 6) & 7;
    r = std::max(0, r - ztmaveni); g = std::max(0, g - ztmaveni); b = std::max(0, b - ztmaveni);
    return (unsigned)((r << 1) | (g << 5) | (b << 9));
  }
  // snimek jako indexy barev (pal*16 + barva) - jak je VDP posila do scanline_rendered
  void kresliIndexy(std::vector<uint8_t> &I) {
    I.assign((size_t)W * H, 0);
    for (int y = 0; y < H; y++) {
      int bx;
      if (y < 32) bx = (int)(kamX / 8);
      else if (y < 80) bx = (int)(kamX / 4);
      else if (y < 112) bx = (int)(kamX / 2);
      else if (y < 150) bx = (int)(kamX * 3 / 4) + (int)(2 * std::sin(y * 0.4 + snimek * 0.2));
      else bx = (int)(kamX * 3 / 4);
      const int ax = (int)kamX, ay = (int)(kamY + 16);
      for (int x = 0; x < W; x++) {
        int pal = 0, i = bodVrstvy(mapaA, x + ax, y + ay, pal);
        if (!i) { pal = 0; i = bodVrstvy(mapaB, x + bx, y, pal); }
        I[(size_t)y * W + x] = (uint8_t)(i ? pal * 16 + i : 0);
      }
    }
    for (const auto &s : sprity) {
      const int sx0 = (int)(s.svet ? s.x - kamX * 0.1 : s.x), sy0 = (int)s.y;
      for (int ty = 0; ty < 4; ty++)
        for (int tx = 0; tx < 4; tx++) {
          const uint8_t *d = dlazdice[s.t0 + ty * 4 + tx < 400 ? s.t0 + ty * 4 + tx : 399];
          for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
              const int X = sx0 + tx * 8 + x, Y = sy0 + ty * 8 + y;
              if (X < 0 || Y < 0 || X >= W || Y >= H || !d[y * 8 + x]) continue;
              I[(size_t)Y * W + X] = (uint8_t)(s.pal * 16 + d[y * 8 + x]);
            }
        }
    }
  }
  void kresli(std::vector<uint32_t> &F) {
    F.assign((size_t)W * H, 0);
    for (int y = 0; y < H; y++) {
      int bx;
      if (y < 32) bx = (int)(kamX / 8);
      else if (y < 80) bx = (int)(kamX / 4);
      else if (y < 112) bx = (int)(kamX / 2);
      else if (y < 150) bx = (int)(kamX * 3 / 4) + (int)(2 * std::sin(y * 0.4 + snimek * 0.2));   // voda se vlni
      else bx = (int)(kamX * 3 / 4);
      const int ax = (int)kamX, ay = (int)(kamY + 16);
      for (int x = 0; x < W; x++) {
        int pal = 0, i = bodVrstvy(mapaA, x + ax, y + ay, pal);
        if (!i) { pal = 0; i = bodVrstvy(mapaB, x + bx, y, pal); }
        F[(size_t)y * W + x] = i ? barva(pal, i) : barva(0, 0);
      }
    }
    for (const auto &s : sprity) {
      const int sx0 = (int)(s.svet ? s.x - kamX * 0.1 : s.x), sy0 = (int)s.y;
      for (int ty = 0; ty < 4; ty++)
        for (int tx = 0; tx < 4; tx++) {
          const uint8_t *d = dlazdice[s.t0 + ty * 4 + tx < 400 ? s.t0 + ty * 4 + tx : 399];
          for (int y = 0; y < 8; y++)
            for (int x = 0; x < 8; x++) {
              const int X = sx0 + tx * 8 + x, Y = sy0 + ty * 8 + y;
              if (X < 0 || Y < 0 || X >= W || Y >= H || !d[y * 8 + x]) continue;
              F[(size_t)Y * W + X] = barva(s.pal, d[y * 8 + x]);
            }
        }
    }
    // skore nahore (cisla se meni)
    char t[32]; snprintf(t, sizeof(t), "SCORE %06d", snimek / 10 * 10);
    for (int c = 0; t[c]; c++)
      for (int y = 0; y < 8; y++)
        for (int x = 0; x < 8; x++)
          if (((t[c] * 7 + x * 3 + y * 5) % 4) == 0) F[(size_t)(8 + y) * W + 16 + c * 8 + x] = barva(3, 15);
    if (W < 320) return;
    // levy sloupec vypnuty jako v nap_real_scanline_rendered: obraz posunuty, vpravo 8 bodu cerne
    for (int y = 0; y < H; y++) {
      uint32_t *r = &F[(size_t)y * W];
      std::memmove(r, r + 8, (size_t)(W - 8) * 4);
      for (int x = W - 8; x < W; x++) r[x] = 0xFF000000u;
    }
  }
};

