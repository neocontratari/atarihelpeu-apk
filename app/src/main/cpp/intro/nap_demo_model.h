// nap_demo_model.h
// ====================================================================
//  B304: 3D MODEL ATARI 130XE (a mobil) pro uvodni demo.
//
//  Skutecne 3D - skrin s profilem jako 130XE (spredu nizka, vzadu vyssi
//  s sikmymi zebry), 62 klaves s popisky, funkcni klavesy HELP / START /
//  SELECT / OPTION / RESET a napis ATARI 130XE. Jednotky = centimetry,
//  X doprava, Y nahoru, Z dozadu (kamera je vepredu, na zaporne Z).
//
//  Material (aM.x):  0 skrin  1 deska klavesnice  2 bok klavesy
//                    3 vrsek klavesy (popisek z atlasu)  4 zebra vzadu
//                    5 napis ATARI 130XE (atlas)  6 funkcni klavesa (atlas)
//                    7 kontrolka POWER  8 stul  9 telo mobilu  10 displej
//                    11 stena s oknem
// ====================================================================
#pragma once
#include "nap_demo_gl.h"
#include "nap_demo_text.h"

namespace napdemo {

struct Stavitel {
  std::vector<Vrchol> v;
  void tri(const V3 &a, const V3 &b, const V3 &c, const V3 &n, const V2 &ua, const V2 &ub, const V2 &uc, float m0,
           float m1 = 0, float m2 = 0, float m3 = 0) {
    const V3 *p[3] = {&a, &b, &c};
    const V2 *u[3] = {&ua, &ub, &uc};
    for (int i = 0; i < 3; i++) {
      Vrchol x;
      x.p[0] = p[i]->x; x.p[1] = p[i]->y; x.p[2] = p[i]->z;
      x.n[0] = n.x; x.n[1] = n.y; x.n[2] = n.z;
      x.uv[0] = u[i]->x; x.uv[1] = u[i]->y;
      x.m[0] = m0; x.m[1] = m1; x.m[2] = m2; x.m[3] = m3;
      v.push_back(x);
    }
  }
  // ctyruhelnik a-b-c-d; normala = (d-a) x (b-a) - pri stavbe zepredu
  // (a vlevo dole, b vpravo dole, d vlevo nahore) miri ven
  void quad(const V3 &a, const V3 &b, const V3 &c, const V3 &d, float m0, V2 ua = {0, 0}, V2 ub = {1, 0}, V2 uc = {1, 1},
            V2 ud = {0, 1}, float m1 = 0, float m2 = 0, float m3 = 0) {
    V3 n = norm(cross(d - a, b - a));
    tri(a, b, c, n, ua, ub, uc, m0, m1, m2, m3);
    tri(a, c, d, n, ua, uc, ud, m0, m1, m2, m3);
  }
  // kvadr s osou Y nahoru: dole obdelnik (x0..x1, z0..z1) ve vysce y0,
  // nahore zuzeny o "zuz" a ve vysce y1 (klavesa); horni plocha ma material mTop
  void klavesa(float x0, float x1, float z0, float z1, float y0, float y1, float zuz, float naklon, float mBok, float mTop,
               V2 uv0, V2 uv1, float m1 = 0) {
    V3 b0(x0, y0, z0), b1(x1, y0, z0), b2(x1, y0 + naklon * (z1 - z0), z1), b3(x0, y0 + naklon * (z1 - z0), z1);
    V3 t0(x0 + zuz, y1, z0 + zuz), t1(x1 - zuz, y1, z0 + zuz), t2(x1 - zuz, y1 + naklon * (z1 - z0 - 2 * zuz), z1 - zuz * 0.6f),
        t3(x0 + zuz, y1 + naklon * (z1 - z0 - 2 * zuz), z1 - zuz * 0.6f);
    // vrsek (popisek: u doleva->doprava, v zepredu->dozadu)
    quad(t0, t1, t2, t3, mTop, {uv0.x, uv1.y}, {uv1.x, uv1.y}, {uv1.x, uv0.y}, {uv0.x, uv0.y}, m1);
    quad(b0, b1, t1, t0, mBok, {0, 0}, {1, 0}, {1, 1}, {0, 1}, m1);    // predni bok
    quad(b1, b2, t2, t1, mBok, {0, 0}, {1, 0}, {1, 1}, {0, 1}, m1);    // pravy
    quad(b2, b3, t3, t2, mBok, {0, 0}, {1, 0}, {1, 1}, {0, 1}, m1);    // zadni
    quad(b3, b0, t0, t3, mBok, {0, 0}, {1, 0}, {1, 1}, {0, 1}, m1);    // levy
  }
  // Model se stavi "zepredu" s prednim okrajem na -Z. Kamera ale stoji na +Z
  // a diva se k -Z (jinak by byla klavesnice zrcadlove). Otocit Z.
  void otocZ() {
    for (auto &x : v) { x.p[2] = -x.p[2]; x.n[2] = -x.n[2]; }
  }
};

// Primo do atlasu (bez GL): vysazeni jedne nebo dvou radek textu do bunky
inline void atlasText(std::vector<unsigned char> &rgba, int AW, int AH, int ox, int oy, int bw, int bh,
                      const Pismo &p, const char *radek1, const char *radek2, float px1, float px2,
                      unsigned char R, unsigned char G, unsigned char B, int zarovnani /*0 stred 1 vlevo-nahore*/) {
  (void)AH;
  const char *radky[2] = {radek1, radek2};
  const float pxs[2] = {px1, px2};
  const int n = (radek2 && *radek2) ? 2 : 1;
  float vys[2], sir[2];
  for (int i = 0; i < n; i++) {
    float sc = p.meritko(pxs[i]); float w = 0;
    for (const char *q = radky[i]; *q;) {
      int cp = utf8Dalsi(q); int adv, lsb; stbtt_GetCodepointHMetrics(&p.info, cp, &adv, &lsb); w += adv * sc;
    }
    sir[i] = w; vys[i] = (p.asc + p.desc) * pxs[i];
  }
  float celkem = 0; for (int i = 0; i < n; i++) celkem += vys[i] * (i ? 1.05f : 1.f);
  float yy = oy + (zarovnani == 0 ? (bh - celkem) * 0.5f : bh * 0.12f);
  std::vector<unsigned char> g;
  for (int i = 0; i < n; i++) {
    const float sc = p.meritko(pxs[i]);
    float xx = ox + (zarovnani == 0 ? (bw - sir[i]) * 0.5f : bw * 0.12f);
    const float base = yy + p.asc * pxs[i];
    for (const char *q = radky[i]; *q;) {
      int cp = utf8Dalsi(q);
      int x0, y0, x1, y1;
      float fx = xx - std::floor(xx);
      stbtt_GetCodepointBitmapBoxSubpixel(&p.info, cp, sc, sc, fx, 0, &x0, &y0, &x1, &y1);
      int gw = x1 - x0, gh = y1 - y0;
      if (gw > 0 && gh > 0) {
        g.assign((size_t)gw * gh, 0);
        stbtt_MakeCodepointBitmapSubpixel(&p.info, g.data(), gw, gh, gw, sc, sc, fx, 0, cp);
        for (int a = 0; a < gh; a++)
          for (int b = 0; b < gw; b++) {
            int px = (int)std::floor(xx) + x0 + b, py = (int)std::floor(base) + y0 + a;
            if (px < ox || py < oy || px >= ox + bw || py >= oy + bh) continue;
            size_t k = ((size_t)py * AW + px) * 4;
            unsigned char c = g[(size_t)a * gw + b];
            if (c > rgba[k + 3]) { rgba[k] = R; rgba[k + 1] = G; rgba[k + 2] = B; rgba[k + 3] = c; }
          }
      }
      int adv, lsb; stbtt_GetCodepointHMetrics(&p.info, cp, &adv, &lsb); xx += adv * sc;
    }
    yy += vys[i] * 1.05f;
  }
}

// trojuhelnik "Fuji" jako v nakresu pristroje (svetly, s prechodem)
inline void atlasFuji(std::vector<unsigned char> &rgba, int AW, int ox, int oy, int sz) {
  for (int yy = 0; yy < sz; yy++)
    for (int xx = 0; xx < sz; xx++) {
      // vrcholy (50% 2%) (96% 96%) (4% 96%) - pokryti s vyhlazenim (4x4 vzorky)
      int hit = 0;
      for (int sy = 0; sy < 4; sy++)
        for (int sx = 0; sx < 4; sx++) {
          float px = (xx + (sx + 0.5f) / 4) / sz, py = (yy + (sy + 0.5f) / 4) / sz;
          if (py < 0.02f || py > 0.96f) continue;
          float t = (py - 0.02f) / 0.94f;
          float l = 0.5f - t * 0.46f, r = 0.5f + t * 0.46f;
          if (px >= l && px <= r) hit++;
        }
      if (!hit) continue;
      size_t k = ((size_t)(oy + yy) * AW + ox + xx) * 4;
      float tt = (float)yy / sz;
      rgba[k] = (unsigned char)(240 - 33 * tt); rgba[k + 1] = (unsigned char)(236 - 36 * tt);
      rgba[k + 2] = (unsigned char)(224 - 44 * tt); rgba[k + 3] = (unsigned char)(hit * 255 / 16);
    }
}

struct ModelAtari {
  Sit sit;
  GLuint atlas = 0;
  V3 led{0, 0, 0};           // kde je kontrolka POWER (pro zare)
  V3 vyduch{0, 0, 0};        // stred zeber vzadu (odtud vyleti energie)
  float sirka = 37.5f, hloubka = 23.f;

  void postav(const Pismo &bold, const Pismo &boldItal, const Pismo &medium) {
    Stavitel s;
    std::vector<unsigned char> atl;
    const int AW = 2048, AH = 1024;
    atl.assign((size_t)AW * AH * 4, 0);
    int ax = 4, ay = 4, radekH = 0;
    auto misto = [&](int w, int h, int &ox, int &oy) {
      if (ax + w + 4 > AW) { ax = 4; ay += radekH + 6; radekH = 0; }
      ox = ax; oy = ay; ax += w + 6; if (h > radekH) radekH = h;
    };
    auto uvBunky = [&](int ox, int oy, int w, int h, V2 &u0, V2 &u1) {
      u0 = {(ox + 0.5f) / AW, (oy + 0.5f) / AH}; u1 = {(ox + w - 0.5f) / AW, (oy + h - 0.5f) / AH};
    };

    const float W2 = sirka * 0.5f;
    const float b = 0.35f;                         // zkoseni hran
    // ---- profil vrsku zepredu dozadu (z, y) ----
    struct P { float z, y; int mat; };
    const P prof[] = {
        {-11.5f, 2.2f, 0}, {-10.7f, 2.55f, 0},   // predni hrana
        {-10.7f, 2.55f, 1}, {2.6f, 3.75f, 1},    // deska klavesnice (sikma)
        {2.6f, 3.75f, 0}, {2.6f, 4.65f, 0},      // schod nahoru
        {2.6f, 4.65f, 0}, {6.1f, 4.95f, 0},      // pruh s napisem a funkcnimi klavesami
        {6.1f, 4.95f, 0}, {6.1f, 5.85f, 0},      // schod
        {6.1f, 5.85f, 4}, {10.8f, 6.25f, 4},     // zebra vzadu
        {10.8f, 6.25f, 0}, {11.5f, 5.7f, 0},     // zadni hrana
    };
    const int NP = sizeof(prof) / sizeof(prof[0]);
    // vrsky a zkosene hrany
    for (int i = 0; i + 1 < NP; i += 2) {
      const P &a = prof[i], &c = prof[i + 1];
      const float m = (float)a.mat;
      const bool svisle = std::fabs(c.z - a.z) < 1e-3f;
      if (svisle) {
        // schod - svisla stena pres celou sirku
        s.quad(V3(-W2, a.y - b, a.z), V3(W2, a.y - b, a.z), V3(W2, c.y, c.z), V3(-W2, c.y, c.z), 0);
        continue;
      }
      // u = x (-1..1), v = z - pro zebra a napis
      s.quad(V3(-W2 + b, a.y, a.z), V3(W2 - b, a.y, a.z), V3(W2 - b, c.y, c.z), V3(-W2 + b, c.y, c.z), m,
             {-W2 + b, a.z}, {W2 - b, a.z}, {W2 - b, c.z}, {-W2 + b, c.z});
      // zkosene hrany vlevo a vpravo
      s.quad(V3(-W2, a.y - b, a.z), V3(-W2 + b, a.y, a.z), V3(-W2 + b, c.y, c.z), V3(-W2, c.y - b, c.z), 0);
      s.quad(V3(W2 - b, a.y, a.z), V3(W2, a.y - b, a.z), V3(W2, c.y - b, c.z), V3(W2 - b, c.y, c.z), 0);
    }
    // predni stena (zkosena hrana nahore uz je v profilu), zadni stena, dno
    s.quad(V3(-W2, 0.2f, -11.5f), V3(W2, 0.2f, -11.5f), V3(W2, 2.2f - b, -11.5f), V3(-W2, 2.2f - b, -11.5f), 0);
    s.quad(V3(W2, 0.2f, 11.5f), V3(-W2, 0.2f, 11.5f), V3(-W2, 5.7f - b, 11.5f), V3(W2, 5.7f - b, 11.5f), 0);
    // boky: pas od dna po vrsek (profil o "b" nize)
    for (int i = 0; i + 1 < NP; i += 2) {
      const P &a = prof[i], &c = prof[i + 1];
      if (std::fabs(c.z - a.z) < 1e-3f) continue;
      s.quad(V3(-W2, 0.2f, c.z), V3(-W2, 0.2f, a.z), V3(-W2, a.y - b, a.z), V3(-W2, c.y - b, c.z), 0);
      s.quad(V3(W2, 0.2f, a.z), V3(W2, 0.2f, c.z), V3(W2, c.y - b, c.z), V3(W2, a.y - b, a.z), 0);
    }
    // schody na bocich (svisle useky profilu)
    for (int i = 0; i + 1 < NP; i += 2) {
      const P &a = prof[i], &c = prof[i + 1];
      if (std::fabs(c.z - a.z) > 1e-3f) continue;
      s.quad(V3(-W2, 0.2f, a.z), V3(-W2, 0.2f, a.z + 0.001f), V3(-W2, c.y - b, a.z + 0.001f), V3(-W2, a.y - b, a.z), 0);
    }
    // ---- klavesnice ----
    const float u = 1.9f;              // rozteč klaves
    auto deskaY = [](float z) { return 2.55f + (z + 10.7f) / 13.3f * 1.2f; };
    const float naklon = 1.2f / 13.3f;
    struct K { const char *a, *b; float w; };
    const K r1[] = {{"Esc", "", 1}, {"!", "1", 1}, {"\"", "2", 1}, {"#", "3", 1}, {"$", "4", 1}, {"%", "5", 1},
                    {"&", "6", 1},  {"'", "7", 1}, {"@", "8", 1},  {"(", "9", 1}, {")", "0", 1}, {"Clear", "<", 1},
                    {"Insert", ">", 1}, {"Delete", "Bk Sp", 1}, {"Break", "", 1}};
    const K r2[] = {{"Tab", "", 1.5f}, {"Q", "", 1}, {"W", "", 1}, {"E", "", 1}, {"R", "", 1}, {"T", "", 1}, {"Y", "", 1},
                    {"U", "", 1},      {"I", "", 1}, {"O", "", 1}, {"P", "", 1}, {"_", "-", 1}, {"|", "=", 1}, {"Return", "", 1.5f}};
    const K r3[] = {{"Control", "", 1.75f}, {"A", "", 1}, {"S", "", 1}, {"D", "", 1}, {"F", "", 1}, {"G", "", 1},
                    {"H", "", 1},           {"J", "", 1}, {"K", "", 1}, {"L", "", 1}, {":", ";", 1}, {"\\", "+", 1},
                    {"^", "*", 1},          {"Caps", "", 1.25f}};
    const K r4[] = {{"Shift", "", 2.25f}, {"Z", "", 1}, {"X", "", 1}, {"C", "", 1}, {"V", "", 1}, {"B", "", 1},
                    {"N", "", 1},         {"M", "", 1}, {"[", ",", 1}, {"]", ".", 1}, {"?", "/", 1}, {"Shift", "", 1.75f},
                    {"", "", 1}};
    struct Rada { const K *k; int n; float z; };
    const Rada rady[] = {{r1, 15, 1.05f}, {r2, 14, 1.05f - u}, {r3, 14, 1.05f - 2 * u}, {r4, 13, 1.05f - 3 * u}};
    const float x0 = -7.5f * u;
    const int CELL = 112;
    for (const Rada &ra : rady) {
      float x = x0;
      for (int i = 0; i < ra.n; i++) {
        const K &k = ra.k[i];
        const float w = k.w * u;
        const float kz0 = ra.z - u * 0.5f + 0.09f, kz1 = ra.z + u * 0.5f - 0.09f;
        const int cw = (int)(CELL * k.w), ch = CELL;
        int ox, oy; misto(cw, ch, ox, oy);
        const bool pismeno = k.a[0] && !k.a[1] && k.b[0] == 0 && k.a[0] >= 'A' && k.a[0] <= 'Z';
        if (k.a[0] && k.b[0])           // dva radky: horni maly, dolni vetsi
          atlasText(atl, AW, AH, ox, oy, cw, ch, medium, k.a, k.b, 26, std::strlen(k.b) > 2 ? 24 : 34, 58, 58, 56, 0);
        else if (pismeno)
          atlasText(atl, AW, AH, ox, oy, cw, ch, medium, k.a, "", 50, 0, 50, 50, 48, 1);
        else if (k.a[0])
          atlasText(atl, AW, AH, ox, oy, cw, ch, medium, k.a, "", std::strlen(k.a) > 4 ? 24 : 30, 0, 58, 58, 56, 0);
        else {                          // klavesa s logem (inverze): pulka ctverce
          for (int yy = 0; yy < 36; yy++)
            for (int xx = 0; xx < 36; xx++) {
              size_t q = ((size_t)(oy + 38 + yy) * AW + ox + (cw - 36) / 2 + xx) * 4;
              bool plne = xx + yy < 36 || xx < 3 || yy < 3 || xx > 32 || yy > 32;
              if (plne) { atl[q] = 60; atl[q + 1] = 60; atl[q + 2] = 58; atl[q + 3] = 230; }
            }
        }
        V2 u0, u1; uvBunky(ox, oy, cw, ch, u0, u1);
        s.klavesa(x + 0.09f, x + w - 0.09f, kz0, kz1, deskaY(kz0) - 0.3f, deskaY(kz0) + 0.95f, 0.2f, naklon, 2, 3, u0, u1);
        x += w;
      }
    }
    {   // mezernik
      const float z = 1.05f - 4 * u;
      const float kz0 = z - u * 0.5f + 0.09f, kz1 = z + u * 0.5f - 0.09f;
      int ox, oy; misto(CELL, CELL, ox, oy);
      V2 u0, u1; uvBunky(ox, oy, CELL, CELL, u0, u1);
      s.klavesa(-4.5f * u, 4.0f * u, kz0, kz1, deskaY(kz0) - 0.3f, deskaY(kz0) + 0.95f, 0.2f, naklon, 2, 3, u0, u1);
    }
    // ---- napis ATARI 130XE vlevo na pruhu ----
    {
      const int cw = 1100, ch = 128;
      int ox, oy; misto(cw, ch, ox, oy);
      atlasFuji(atl, AW, ox + 8, oy + 18, 92);
      atlasText(atl, AW, AH, ox + 120, oy + 4, 440, 120, boldItal, "ATARI", "", 92, 0, 216, 81, 31, 1);
      atlasText(atl, AW, AH, ox + 470, oy + 4, 520, 120, bold, "130XE", "", 92, 0, 43, 42, 40, 1);
      V2 u0, u1; uvBunky(ox, oy, cw, ch, u0, u1);
      // tenka deska tesne nad pruhem (y pruhu 4.65 -> 4.95 mezi z 2.6 a 6.1)
      auto pruhY = [](float z) { return 4.65f + (z - 2.6f) / 3.5f * 0.3f + 0.02f; };
      const float zx0 = 3.0f, zx1 = 5.6f, xL = -16.6f;
      const float delka = (zx1 - zx0) * (float)cw / ch;   // zachovat pomer stran textu
      s.quad(V3(xL, pruhY(zx0), zx0), V3(xL + delka, pruhY(zx0), zx0), V3(xL + delka, pruhY(zx1), zx1), V3(xL, pruhY(zx1), zx1), 5,
             {u0.x, u1.y}, {u1.x, u1.y}, {u1.x, u0.y}, {u0.x, u0.y});
    }
    // ---- funkcni klavesy (sikme) ----
    {
      const char *jm[5] = {"Help", "Start", "Select", "Option", "Reset"};
      const float zA = 3.1f, zB = 5.5f, sik = 1.1f;                 // sikmina vlevo
      auto pruhY = [](float z) { return 4.65f + (z - 2.6f) / 3.5f * 0.3f; };
      for (int i = 0; i < 5; i++) {
        const float xa = 0.6f + i * 3.45f, xb = xa + 3.15f;
        int ox, oy; misto(176, 72, ox, oy);
        atlasText(atl, AW, AH, ox, oy, 176, 72, medium, jm[i], "", 30, 0, 70, 70, 68, 0);
        V2 u0, u1; uvBunky(ox, oy, 176, 72, u0, u1);
        const float h0 = pruhY(zA) - 0.05f, h1 = pruhY(zB) - 0.05f, vy = 0.42f;
        V3 b0(xa, h0, zA), b1(xb, h0, zA), b2(xb + sik, h1, zB), b3(xa + sik, h1, zB);
        V3 t0(xa + 0.12f, h0 + vy, zA + 0.12f), t1(xb - 0.12f, h0 + vy, zA + 0.12f), t2(xb + sik - 0.12f, h1 + vy, zB - 0.12f),
            t3(xa + sik + 0.12f, h1 + vy, zB - 0.12f);
        s.quad(t0, t1, t2, t3, 6, {u0.x, u1.y}, {u1.x, u1.y}, {u1.x, u0.y}, {u0.x, u0.y});
        s.quad(b0, b1, t1, t0, 2); s.quad(b1, b2, t2, t1, 2); s.quad(b3, b0, t0, t3, 2); s.quad(b2, b3, t3, t2, 2);
      }
      // kontrolka POWER mezi napisem a klavesami
      const float lx = -1.6f, lz = 4.35f, ly = pruhY(lz) + 0.04f;
      s.quad(V3(lx - 0.45f, ly, lz - 0.2f), V3(lx + 0.45f, ly, lz - 0.2f), V3(lx + 0.45f, ly, lz + 0.2f), V3(lx - 0.45f, ly, lz + 0.2f), 7);
      led = V3(lx, ly + 0.1f, -lz);
    }
    vyduch = V3(2.0f, 6.1f, -8.4f);
    s.otocZ();
    sit.nahraj(s.v);
    if (atlas) glDeleteTextures(1, &atlas);
    atlas = novaTextura(AW, AH, GL_RGBA, atl.data(), true);
    glBindTexture(GL_TEXTURE_2D, atlas);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    NAPDEMO_LOG("DEMO model Atari 130XE: %d trojuhelniku", (int)(s.v.size() / 3));
  }
};

// Mobil: tenka deska se zkosenymi hranami, displej vpredu (material 10)
struct ModelMobil {
  Sit sit;
  void postav() {
    Stavitel s;
    const float w = 7.4f, h = 15.6f, d = 0.8f, c = 0.55f;     // c = zkoseni rohu
    const float x0 = -w / 2, x1 = w / 2, y0 = -h / 2, y1 = h / 2, z0 = -d / 2, z1 = d / 2;
    // osmiuhelnik obrysu (zkosene rohy)
    const V2 o[8] = {{x0 + c, y0}, {x1 - c, y0}, {x1, y0 + c}, {x1, y1 - c}, {x1 - c, y1}, {x0 + c, y1}, {x0, y1 - c}, {x0, y0 + c}};
    // predni strana (cerne sklo) jako vejir, displej zvlast
    V3 st(0, 0, z0);
    for (int i = 0; i < 8; i++) {
      const V2 &a = o[i], &b = o[(i + 1) % 8];
      s.tri(st, V3(b.x, b.y, z0), V3(a.x, a.y, z0), V3(0, 0, -1), {0.5f, 0.5f}, {0, 0}, {0, 0}, 9, 1);
    }
    const float m = 0.32f;   // ramecek displeje
    s.quad(V3(x0 + m, y0 + 1.1f, z0 - 0.01f), V3(x1 - m, y0 + 1.1f, z0 - 0.01f), V3(x1 - m, y1 - 1.1f, z0 - 0.01f),
           V3(x0 + m, y1 - 1.1f, z0 - 0.01f), 10, {0, 0}, {1, 0}, {1, 1}, {0, 1});
    // zadni strana a boky
    V3 sz(0, 0, z1);
    for (int i = 0; i < 8; i++) {
      const V2 &a = o[i], &b = o[(i + 1) % 8];
      s.tri(sz, V3(a.x, a.y, z1), V3(b.x, b.y, z1), V3(0, 0, 1), {0.5f, 0.5f}, {0, 0}, {0, 0}, 9, 0);
      s.quad(V3(a.x, a.y, z0), V3(b.x, b.y, z0), V3(b.x, b.y, z1), V3(a.x, a.y, z1), 9, {0, 0}, {1, 0}, {1, 1}, {0, 1}, 2);
    }
    s.otocZ();               // displej k +Z (ke kamere)
    sit.nahraj(s.v);
  }
};

}  // namespace napdemo
