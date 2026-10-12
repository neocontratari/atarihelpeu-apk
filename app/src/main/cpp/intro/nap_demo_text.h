// nap_demo_text.h
// ====================================================================
//  B304: TITULKY A NAPISY DEMA - pismo z navrhu N&P (Chakra Petch, Space
//  Mono, ROM pismo Atari) vysazene v C++ pres stb_truetype do textury.
//  Cestina vcetne hacku a carek, „uvozovky“, pomlcka, trojtecka, (C).
//  Kazdy blok textu se vysadi jednou (pro danou velikost obrazovky) i se
//  stinem - pak je to pri kresleni jen jeden obdelnik s texturou.
// ====================================================================
#pragma once
#include "nap_demo_gl.h"
#include "../vendor/stb/stb_truetype.h"

namespace napdemo {

struct Pismo {
  stbtt_fontinfo info{};
  bool ok = false;
  float upm = 1000.f, asc = 0.9f, desc = 0.25f, mezera = 0.f;   // v em
  void init(const unsigned char *data) {
    ok = stbtt_InitFont(&info, data, stbtt_GetFontOffsetForIndex(data, 0)) != 0;
    if (!ok) return;
    int a, d, g; stbtt_GetFontVMetrics(&info, &a, &d, &g);
    const unsigned char *hp = info.data + info.head + 18;
    int u = (hp[0] << 8) | hp[1];
    upm = u > 0 ? (float)u : 1000.f;
    asc = a / upm; desc = -d / upm; mezera = g / upm;
  }
  float meritko(float px) const { return stbtt_ScaleForMappingEmToPixels(&info, px); }
};

inline int utf8Dalsi(const char *&p) {
  unsigned char c = (unsigned char)*p;
  if (c < 0x80) { p++; return c; }
  if ((c & 0xE0) == 0xC0 && p[1]) { int cp = ((c & 0x1F) << 6) | ((unsigned char)p[1] & 0x3F); p += 2; return cp; }
  if ((c & 0xF0) == 0xE0 && p[1] && p[2]) {
    int cp = ((c & 0x0F) << 12) | (((unsigned char)p[1] & 0x3F) << 6) | ((unsigned char)p[2] & 0x3F); p += 3; return cp;
  }
  if ((c & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
    int cp = ((c & 0x07) << 18) | (((unsigned char)p[1] & 0x3F) << 12) | (((unsigned char)p[2] & 0x3F) << 6) | ((unsigned char)p[3] & 0x3F);
    p += 4; return cp;
  }
  p++; return c;
}

// Jeden odstavec bloku (napr. cesky radek a pod nim anglicky)
struct Odstavec {
  std::string text;
  const Pismo *pismo = nullptr;
  float px = 32.f;               // velikost em v bodech
  float proklad = 0.f;           // mezera mezi pismeny v em (filmove titulky)
  float radek = 1.25f;           // vyska radku v em
  float r = 1, g = 1, b = 1, a = 1;
  float mezeraPred = 0.f;        // svisla mezera pred odstavcem v bodech
};

struct TextTex {
  GLuint tex = 0;
  int w = 0, h = 0;              // rozmer textury v bodech (vc. okraje na stin)
  int okraj = 0;
  float obsahW = 0, obsahH = 0;  // rozmer samotneho textu
  void uvolni() { if (tex) glDeleteTextures(1, &tex); tex = 0; w = h = 0; }
};

struct Sazec {
  struct Radek { int od, po; float sirka; };
  struct Slovo { int od, po; float sirka; };

  static float sirkaZnaku(const Odstavec &o, int cp, int dalsi, float sc) {
    int adv, lsb; stbtt_GetCodepointHMetrics(&o.pismo->info, cp, &adv, &lsb);
    float w = adv * sc + o.proklad * o.px;
    if (dalsi > 0) w += stbtt_GetCodepointKernAdvance(&o.pismo->info, cp, dalsi) * sc;
    return w;
  }

  // zalomi odstavec do radku nejvys maxW bodu (podle slov)
  static std::vector<std::vector<int>> zalom(const Odstavec &o, float maxW, std::vector<float> &sirky) {
    std::vector<int> cps;
    for (const char *p = o.text.c_str(); *p;) cps.push_back(utf8Dalsi(p));
    std::vector<std::vector<int>> radky;
    const float sc = o.pismo->meritko(o.px);
    std::vector<int> radek; float sirka = 0;
    size_t i = 0;
    while (i < cps.size()) {
      if (cps[i] == '\n') { radky.push_back(radek); sirky.push_back(sirka); radek.clear(); sirka = 0; i++; continue; }
      // slovo vcetne mezery za nim
      size_t j = i;
      while (j < cps.size() && cps[j] != ' ' && cps[j] != '\n') j++;
      float ws = 0;
      for (size_t k = i; k < j; k++) ws += sirkaZnaku(o, cps[k], k + 1 < j ? cps[k + 1] : 0, sc);
      float sp = (j < cps.size() && cps[j] == ' ') ? sirkaZnaku(o, ' ', 0, sc) : 0.f;
      if (!radek.empty() && sirka + ws > maxW) {
        // mezeru na konci radku nepocitat
        while (!radek.empty() && radek.back() == ' ') { sirka -= sirkaZnaku(o, ' ', 0, sc); radek.pop_back(); }
        radky.push_back(radek); sirky.push_back(sirka); radek.clear(); sirka = 0;
      }
      for (size_t k = i; k < j; k++) radek.push_back(cps[k]);
      sirka += ws;
      if (sp > 0) { radek.push_back(' '); sirka += sp; j++; }
      i = j;
    }
    while (!radek.empty() && radek.back() == ' ') { sirka -= sirkaZnaku(o, ' ', 0, sc); radek.pop_back(); }
    if (!radek.empty()) { radky.push_back(radek); sirky.push_back(sirka); }
    return radky;
  }

  // Vysadi blok do textury. zarovnani: 0 = na stred, 1 = vlevo.
  // stin = polomer rozmazani stinu v bodech (0 = bez stinu)
  static TextTex vysazej(const std::vector<Odstavec> &ods, float maxW, int zarovnani, float stin, float stinSila = 0.75f) {
    TextTex t;
    struct RadekVys { std::vector<int> cps; float sirka; const Odstavec *o; float y; };
    std::vector<RadekVys> vse;
    float y = 0, maxSirka = 0;
    for (const auto &o : ods) {
      if (!o.pismo || !o.pismo->ok) continue;
      y += o.mezeraPred;
      std::vector<float> sirky;
      auto r = zalom(o, maxW, sirky);
      for (size_t i = 0; i < r.size(); i++) {
        vse.push_back({r[i], sirky[i], &o, y});
        if (sirky[i] > maxSirka) maxSirka = sirky[i];
        y += o.radek * o.px;
      }
    }
    if (vse.empty()) return t;
    const int okraj = (int)std::ceil(stin * 2.2f) + 2;
    t.okraj = okraj;
    t.obsahW = maxSirka; t.obsahH = y;
    t.w = (int)std::ceil(maxSirka) + 2 * okraj;
    t.h = (int)std::ceil(y) + 2 * okraj;
    if (t.w > 4096) t.w = 4096;
    if (t.h > 4096) t.h = 4096;
    std::vector<unsigned char> pokryti((size_t)t.w * t.h, 0);
    std::vector<unsigned char> barva((size_t)t.w * t.h * 3, 0);
    std::vector<unsigned char> glyf;
    for (const auto &rv : vse) {
      const Odstavec &o = *rv.o;
      const float sc = o.pismo->meritko(o.px);
      float x = okraj + (zarovnani == 0 ? (maxSirka - rv.sirka) * 0.5f : 0.f);
      // uctari: radek o vysce radek*px, ucari pod horni hranou podle ascent
      const float vyska = o.radek * o.px;
      const float baseline = okraj + rv.y + (vyska - (o.pismo->asc + o.pismo->desc) * o.px) * 0.5f + o.pismo->asc * o.px;
      const unsigned char R = (unsigned char)(clampf(o.r, 0, 1) * 255), G = (unsigned char)(clampf(o.g, 0, 1) * 255),
                          B = (unsigned char)(clampf(o.b, 0, 1) * 255);
      const float A = clampf(o.a, 0, 1);
      for (size_t k = 0; k < rv.cps.size(); k++) {
        const int cp = rv.cps[k];
        const int nx = k + 1 < rv.cps.size() ? rv.cps[k + 1] : 0;
        const float fx = x - std::floor(x);
        int x0, y0, x1, y1;
        stbtt_GetCodepointBitmapBoxSubpixel(&o.pismo->info, cp, sc, sc, fx, 0.f, &x0, &y0, &x1, &y1);
        const int gw = x1 - x0, gh = y1 - y0;
        if (gw > 0 && gh > 0 && cp != ' ') {
          glyf.assign((size_t)gw * gh, 0);
          stbtt_MakeCodepointBitmapSubpixel(&o.pismo->info, glyf.data(), gw, gh, gw, sc, sc, fx, 0.f, cp);
          const int ox = (int)std::floor(x) + x0, oy = (int)std::floor(baseline) + y0;
          for (int yy = 0; yy < gh; yy++) {
            const int py = oy + yy;
            if (py < 0 || py >= t.h) continue;
            for (int xx = 0; xx < gw; xx++) {
              const int px = ox + xx;
              if (px < 0 || px >= t.w) continue;
              int c = (int)(glyf[(size_t)yy * gw + xx] * A);
              size_t i = (size_t)py * t.w + px;
              if (c > pokryti[i]) {
                pokryti[i] = (unsigned char)c;
                barva[i * 3] = R; barva[i * 3 + 1] = G; barva[i * 3 + 2] = B;
              }
            }
          }
        }
        x += sirkaZnaku(o, cp, nx, sc);
      }
    }
    // stin: rozmazane pokryti (3x prumer v okne ~ Gauss), mirne posunute dolu
    std::vector<float> st;
    if (stin > 0.5f) {
      st.assign((size_t)t.w * t.h, 0.f);
      for (size_t i = 0; i < st.size(); i++) st[i] = pokryti[i] / 255.f;
      std::vector<float> tmp(st.size());
      const int r = (int)std::max(1.f, stin * 0.58f);
      for (int pruchod = 0; pruchod < 3; pruchod++) {
        for (int yy = 0; yy < t.h; yy++) {          // vodorovne
          float s = 0; const float *row = &st[(size_t)yy * t.w]; float *o = &tmp[(size_t)yy * t.w];
          for (int xx = -r; xx < t.w + r; xx++) {
            if (xx + r < t.w) s += row[xx + r];
            if (xx - r - 1 >= 0) s -= row[xx - r - 1];
            if (xx >= 0 && xx < t.w) o[xx] = s / (2 * r + 1);
          }
        }
        for (int xx = 0; xx < t.w; xx++) {          // svisle
          float s = 0;
          for (int yy = -r; yy < t.h + r; yy++) {
            if (yy + r < t.h) s += tmp[(size_t)(yy + r) * t.w + xx];
            if (yy - r - 1 >= 0) s -= tmp[(size_t)(yy - r - 1) * t.w + xx];
            if (yy >= 0 && yy < t.h) st[(size_t)yy * t.w + xx] = s / (2 * r + 1);
          }
        }
      }
    }
    // slozit: stin (cerny) pod textem, premultiplied alfa
    std::vector<unsigned char> rgba((size_t)t.w * t.h * 4, 0);
    const int posun = stin > 0.5f ? (int)std::max(1.f, stin * 0.25f) : 0;
    for (int yy = 0; yy < t.h; yy++)
      for (int xx = 0; xx < t.w; xx++) {
        size_t i = (size_t)yy * t.w + xx;
        float ta = pokryti[i] / 255.f;
        float sa = 0;
        if (!st.empty()) { int sy = yy - posun; if (sy >= 0) sa = clampf(st[(size_t)sy * t.w + xx] * 1.6f, 0, 1) * stinSila; }
        float a = ta + sa * (1 - ta);
        rgba[i * 4 + 0] = (unsigned char)(barva[i * 3 + 0] * ta);
        rgba[i * 4 + 1] = (unsigned char)(barva[i * 3 + 1] * ta);
        rgba[i * 4 + 2] = (unsigned char)(barva[i * 3 + 2] * ta);
        rgba[i * 4 + 3] = (unsigned char)(clampf(a, 0, 1) * 255);
      }
    t.tex = novaTextura(t.w, t.h, GL_RGBA, rgba.data(), true);
    return t;
  }
};

}  // namespace napdemo
