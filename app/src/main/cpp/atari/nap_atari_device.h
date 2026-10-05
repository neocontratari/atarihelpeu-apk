// nap_atari_device.h
// B291: CELE zarizeni Atari 130XE ze schvaleneho navrhu (claude.ai Design,
// project/Main.dc.html) - CISTE V C++.
//
// Rene (po B290): "tlacitka se nezamackavaji nemas kazetak - nemas nic !!!
// v emu atri - to preved v helpu presne tak vcetne kazetaku a obrazovky -
// podle toho navrhu na kterem jsme se dohodli". B289/B290 mely jen
// klavesnici a konzolovou listu a kazdy dotek posilal ~3,6 MB obrazku pres
// JavaScript - stisk se proto na telefonu vubec nestihl ukazat.
//
// Tenhle soubor kresli VSECHNO ze schvaleneho navrhu, pixel po pixelu, do
// RGBA bufferu libovolne velikosti (podle displeje telefonu):
//   N&P logo, ramecek obrazovky + ZIVY obraz Atari (s CRT linkami),
//   vypinac POWER, 3D napis ATARI 130XE, barevne pruhy, konzolova lista
//   HELP/START/SELECT/OPTION/RESET (vcetne prekryti, ktere se Renemu libi),
//   deska klavesnice + 57 klaves, kazetak (telo, okenko s civkami, paskou,
//   kladkou a napisem AUTOMATIC STOP, dvirka ktera se pri EJECT otevrou a
//   prstem zavrou, pocitadlo 000.0 + nulovaci spinac, mrizky reproduktoru,
//   6 tlacitek REC/REW/PLAY/FWD/STOP/EJECT), servisni panel s 8 tlacitky a
//   LED, duhovy pruh dole a stavovy radek pod pristrojem.
//
// Zadny PNG obrazek na pozadi, zadne HTML tlacitko, zadny Java/JS kreslic -
// jen tenhle C++ kod. Telefon (nap_atari_native.cpp) uz jen posle hotove
// pixely primo na displej (ANativeWindow) a vrati sem souradnice prstu.
//
// PUVOD CISEL: kazda pozice/velikost/barva/stin je 1:1 z CSS schvaleneho
// navrhu (citovano u kazdeho prvku). Prostor navrhu je 941x1672 "du".
// Hodnoty v CSS "px" (stiny, posuny pri stisku) se prepocitavaji tak, jak
// je Rene videl na telefonu: pristroj ~376 CSS px siroky -> 1 CSS px =
// 941/376 du (konstanta CSSPX_DU). Vysledek byl kontrolovan proti obrazku,
// ktery z PRESNE stejneho CSS vykreslil Chromium (test_overeni/b291).
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include "../vendor/stb/stb_truetype.h"
#include "nap_atari_fonts.h"
#include "nap_atari_fonts2.h"
#include "nap_atari_logo.h"

namespace nap {
namespace dev {

// =====================================================================
//  ZAKLAD: barvy, platno, tvary
// =====================================================================
struct RGBA { float r, g, b, a; };                 // r,g,b 0..255, a 0..1
static inline RGBA hexc(uint32_t h, float a = 1.f) {
  return RGBA{ (float)((h >> 16) & 255), (float)((h >> 8) & 255), (float)(h & 255), a };
}
static inline RGBA rgbac(int r, int g, int b, float a) { return RGBA{ (float)r, (float)g, (float)b, a }; }

static inline uint32_t packRGB(float r, float g, float b) {
  int ir = (int)(r + 0.5f), ig = (int)(g + 0.5f), ib = (int)(b + 0.5f);
  if (ir < 0) ir = 0; else if (ir > 255) ir = 255;
  if (ig < 0) ig = 0; else if (ig > 255) ig = 255;
  if (ib < 0) ib = 0; else if (ib > 255) ib = 255;
  // RGBA_8888 v pameti R,G,B,A (presne format ANativeWindow WINDOW_FORMAT_RGBA_8888)
  return 0xFF000000u | ((uint32_t)ib << 16) | ((uint32_t)ig << 8) | (uint32_t)ir;
}
static inline void blendPx(uint32_t &d, float r, float g, float b, float a) {
  if (a <= 0.0005f) return;
  if (a >= 0.9995f) { d = packRGB(r, g, b); return; }
  float dr = (float)(d & 255), dg = (float)((d >> 8) & 255), db = (float)((d >> 16) & 255);
  d = packRGB(dr + (r - dr) * a, dg + (g - dg) * a, db + (b - db) * a);
}

struct IRect { int x0, y0, x1, y1; };              // [x0,x1) x [y0,y1)
static inline bool irEmpty(const IRect &r) { return r.x1 <= r.x0 || r.y1 <= r.y0; }
static inline IRect irIsect(const IRect &a, const IRect &b) {
  return IRect{ std::max(a.x0, b.x0), std::max(a.y0, b.y0), std::min(a.x1, b.x1), std::min(a.y1, b.y1) };
}
static inline IRect irUnion(const IRect &a, const IRect &b) {
  if (irEmpty(a)) return b;
  if (irEmpty(b)) return a;
  return IRect{ std::min(a.x0, b.x0), std::min(a.y0, b.y0), std::max(a.x1, b.x1), std::max(a.y1, b.y1) };
}

struct Canvas {
  uint32_t *px = nullptr;
  int w = 0, h = 0, stride = 0;                   // stride v pixelech
  IRect clip{0, 0, 0, 0};
  inline uint32_t &at(int x, int y) { return px[(size_t)y * stride + x]; }
  // prunik obdelniku (float, px) s orezem -> celociselny rozsah
  inline bool span(float fx0, float fy0, float fx1, float fy1, int &x0, int &y0, int &x1, int &y1) const {
    x0 = std::max(clip.x0, (int)std::floor(fx0)); y0 = std::max(clip.y0, (int)std::floor(fy0));
    x1 = std::min(clip.x1, (int)std::ceil(fx1));  y1 = std::min(clip.y1, (int)std::ceil(fy1));
    return x1 > x0 && y1 > y0;
  }
};

// Zaobleny obdelnik s elipsovymi rohy (CSS border-radius rx/ry), v px.
struct RRect { float x0, y0, x1, y1, rx, ry; };
// CSS: kdyz soucet polomeru presahne rozmer, VSECHNY polomery se zmensi
// stejnym pomerem (presne to dela prohlizec napr. u .svcpanel 10%/60%).
static inline RRect mkRR(float x, float y, float w, float h, float rx, float ry) {
  float f = 1.f;
  if (rx * 2.f > w && rx > 0) f = std::min(f, w / (rx * 2.f));
  if (ry * 2.f > h && ry > 0) f = std::min(f, h / (ry * 2.f));
  return RRect{ x, y, x + w, y + h, rx * f, ry * f };
}
static inline RRect rrMove(const RRect &r, float dx, float dy) {
  return RRect{ r.x0 + dx, r.y0 + dy, r.x1 + dx, r.y1 + dy, r.rx, r.ry };
}
static inline RRect rrGrow(const RRect &r, float sp) {     // CSS spread (+ ven, - dovnitr)
  float rx = r.rx > 0 ? std::max(0.f, r.rx + sp) : 0.f, ry = r.ry > 0 ? std::max(0.f, r.ry + sp) : 0.f;
  return RRect{ r.x0 - sp, r.y0 - sp, r.x1 + sp, r.y1 + sp, rx, ry };
}
// Znamenkova vzdalenost (px) od okraje: <0 uvnitr, >0 venku.
static inline float sdRR(float px, float py, const RRect &r) {
  float cx = (r.x0 + r.x1) * 0.5f, cy = (r.y0 + r.y1) * 0.5f;
  float hw = (r.x1 - r.x0) * 0.5f, hh = (r.y1 - r.y0) * 0.5f;
  float rx = r.rx, ry = r.ry;
  float qx = std::fabs(px - cx) - (hw - rx), qy = std::fabs(py - cy) - (hh - ry);
  if (qx <= 0.f && qy <= 0.f) return std::max(qx - rx, qy - ry);
  if (qy <= 0.f) return qx - rx;
  if (qx <= 0.f) return qy - ry;
  if (rx < 0.05f || ry < 0.05f) return std::sqrt(qx * qx + qy * qy);
  float ax = qx / rx, ay = qy / ry;
  float k0 = std::sqrt(ax * ax + ay * ay);
  float bx = qx / (rx * rx), by = qy / (ry * ry);
  float k1 = std::sqrt(bx * bx + by * by);
  return k0 * (k0 - 1.f) / k1;
}
static inline float covFromSd(float d) { float c = 0.5f - d; return c < 0.f ? 0.f : (c > 1.f ? 1.f : c); }

// 0.5*erfc(x/sqrt2) = podil gaussovsky rozmazaneho tvaru (CSS blur, sigma=blur/2)
static inline float gaussEdge(float d, float sigma) {
  if (sigma < 0.3f) return covFromSd(d);
  float x = d / (sigma * 1.41421356f);
  float ax = std::fabs(x);
  float t = 1.f / (1.f + 0.3275911f * ax);
  float y = t * (0.254829592f + t * (-0.284496736f + t * (1.421413741f + t * (-1.453152027f + t * 1.061405429f))));
  float e = y * std::exp(-ax * ax);                 // erfc(|x|)
  float erfcx = x >= 0 ? e : 2.f - e;
  return 0.5f * erfcx;
}

// Gradient (CSS linear-gradient), az 4 zastavky 0..1
struct Grad {
  int n = 1; float pos[4] = {0, 1, 1, 1}; RGBA col[4];
  static Grad solid(RGBA c) { Grad g; g.n = 1; g.col[0] = c; return g; }
  static Grad two(RGBA a, RGBA b) { Grad g; g.n = 2; g.pos[0] = 0; g.pos[1] = 1; g.col[0] = a; g.col[1] = b; return g; }
  static Grad three(RGBA a, RGBA b, float pb, RGBA c) {
    Grad g; g.n = 3; g.pos[0] = 0; g.pos[1] = pb; g.pos[2] = 1; g.col[0] = a; g.col[1] = b; g.col[2] = c; return g;
  }
  RGBA at(float t) const {
    if (n == 1 || t <= pos[0]) return col[0];
    for (int i = 1; i < n; i++) {
      if (t <= pos[i]) {
        float span = pos[i] - pos[i - 1];
        float f = span > 1e-6f ? (t - pos[i - 1]) / span : 1.f;
        const RGBA &a = col[i - 1], &b = col[i];
        return RGBA{ a.r + (b.r - a.r) * f, a.g + (b.g - a.g) * f, a.b + (b.b - a.b) * f, a.a + (b.a - a.a) * f };
      }
    }
    return col[n - 1];
  }
};

// Vyplni tvar svislym gradientem (180deg) - gradient bezi pres gy0..gy1.
static void fillRR(Canvas &c, const RRect &r, const Grad &g, float gy0, float gy1, float opacity = 1.f) {
  int x0, y0, x1, y1;
  if (!c.span(r.x0, r.y0, r.x1, r.y1, x0, y0, x1, y1)) return;
  float gh = gy1 - gy0; if (gh < 1e-3f) gh = 1e-3f;
  for (int y = y0; y < y1; y++) {
    float fy = y + 0.5f;
    RGBA col = g.at((fy - gy0) / gh);
    float a0 = col.a * opacity;
    uint32_t *row = &c.at(0, y);
    for (int x = x0; x < x1; x++) {
      float cov = covFromSd(sdRR(x + 0.5f, fy, r));
      if (cov > 0.f) blendPx(row[x], col.r, col.g, col.b, a0 * cov);
    }
  }
}
// CSS outer box-shadow (ox,oy,blur,spread,barva)
static void shadowOut(Canvas &c, const RRect &r, float ox, float oy, float blur, float spread, RGBA col) {
  RRect s = rrMove(rrGrow(r, spread), ox, oy);
  float sigma = blur * 0.5f, m = sigma * 3.f + 1.f;
  int x0, y0, x1, y1;
  if (!c.span(s.x0 - m, s.y0 - m, s.x1 + m, s.y1 + m, x0, y0, x1, y1)) return;
  for (int y = y0; y < y1; y++) {
    uint32_t *row = &c.at(0, y);
    float fy = y + 0.5f;
    for (int x = x0; x < x1; x++) {
      float a = col.a * gaussEdge(sdRR(x + 0.5f, fy, s), sigma);
      if (a > 0.002f) blendPx(row[x], col.r, col.g, col.b, a);
    }
  }
}
// CSS inset box-shadow
static void shadowIn(Canvas &c, const RRect &r, float ox, float oy, float blur, float spread, RGBA col) {
  RRect hole = rrMove(rrGrow(r, -spread), ox, oy);
  float sigma = blur * 0.5f;
  int x0, y0, x1, y1;
  if (!c.span(r.x0, r.y0, r.x1, r.y1, x0, y0, x1, y1)) return;
  for (int y = y0; y < y1; y++) {
    uint32_t *row = &c.at(0, y);
    float fy = y + 0.5f;
    for (int x = x0; x < x1; x++) {
      float cov = covFromSd(sdRR(x + 0.5f, fy, r));
      if (cov <= 0.f) continue;
      float inHole = gaussEdge(sdRR(x + 0.5f, fy, hole), sigma);
      float a = col.a * cov * (1.f - inHole);
      if (a > 0.002f) blendPx(row[x], col.r, col.g, col.b, a);
    }
  }
}

// =====================================================================
//  PISMO (stb_truetype) - CSS font-size = velikost "em" (ScaleForMapping
//  EmToPixels). POZOR: B290 omylem pouzival ScaleForPixelHeight (= vyska
//  ascent-descent), cimz bylo vsechno pismo o 23 % mensi nez v navrhu.
// =====================================================================
struct Font {
  stbtt_fontinfo info; bool ok = false; float asc = 0.992f, desc = 0.308f;   // v em
};
static inline Font &fontInit(Font &f, bool &init, const unsigned char *data) {
  if (!init) {
    f.ok = stbtt_InitFont(&f.info, data, stbtt_GetFontOffsetForIndex(data, 0)) != 0;
    if (f.ok) {
      int a, d, g; stbtt_GetFontVMetrics(&f.info, &a, &d, &g);
      // unitsPerEm z tabulky 'head' (offset 18, big-endian) - Chakra Petch i Space Mono maji 1000
      const unsigned char *hp = f.info.data + f.info.head + 18;
      int upm = (hp[0] << 8) | hp[1];
      if (upm <= 0) upm = 1000;
      f.asc = (float)a / upm; f.desc = (float)-d / upm;
    }
    init = true;
  }
  return f;
}
static Font &fMedium()     { static Font f; static bool i = false; return fontInit(f, i, NAP_FONT_CHAKRA_MEDIUM_TTF); }
static Font &fSemi()       { static Font f; static bool i = false; return fontInit(f, i, NAP_FONT_CHAKRA_SEMIBOLD_TTF); }
static Font &fBold()       { static Font f; static bool i = false; return fontInit(f, i, NAP_FONT_CHAKRA_BOLD_TTF); }
static Font &fBoldItalic() { static Font f; static bool i = false; return fontInit(f, i, NAP_FONT_CHAKRA_BOLDITALIC_TTF); }
static Font &fMono()       { static Font f; static bool i = false; return fontInit(f, i, NAP_FONT_SPACEMONO_REGULAR_TTF); }

static inline int utf8Next(const char *&p) {
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
// Sirka textu presne jako v prohlizeci: pricitam letter-spacing i ZA
// poslednim znakem (Chromium to tak dela - proto je centrovany text s
// letter-spacing posunuty o pul mezery doleva).
static float textW(Font &f, const char *s, float fs, float ls) {
  if (!f.ok || !s) return 0.f;
  float sc = stbtt_ScaleForMappingEmToPixels(&f.info, fs);
  float w = 0; int prev = 0; const char *p = s;
  while (*p) {
    int cp = utf8Next(p);
    int adv, lsb; stbtt_GetCodepointHMetrics(&f.info, cp, &adv, &lsb);
    if (prev) w += stbtt_GetCodepointKernAdvance(&f.info, prev, cp) * sc;
    w += adv * sc + ls; prev = cp;
  }
  return w;
}
// Vykresli glyfy do "masky" (alfa 0..1, px) nebo primo barvou.
struct Mask {
  int x0 = 0, y0 = 0, w = 0, h = 0; std::vector<float> a;
  void init(int X0, int Y0, int W, int H) { x0 = X0; y0 = Y0; w = std::max(0, W); h = std::max(0, H); a.assign((size_t)w * h, 0.f); }
};
static void textToMask(Mask &m, Font &f, float x, float baseline, const char *s, float fs, float ls) {
  if (!f.ok || !s) return;
  float sc = stbtt_ScaleForMappingEmToPixels(&f.info, fs);
  float xp = x; int prev = 0; const char *p = s;
  std::vector<unsigned char> bmp;
  while (*p) {
    int cp = utf8Next(p);
    int adv, lsb; stbtt_GetCodepointHMetrics(&f.info, cp, &adv, &lsb);
    if (prev) xp += stbtt_GetCodepointKernAdvance(&f.info, prev, cp) * sc;
    float ix = std::floor(xp), fx = xp - ix;
    float iy = std::floor(baseline), fyy = baseline - iy;
    int gx0, gy0, gx1, gy1;
    stbtt_GetCodepointBitmapBoxSubpixel(&f.info, cp, sc, sc, fx, fyy, &gx0, &gy0, &gx1, &gy1);
    int gw = gx1 - gx0, gh = gy1 - gy0;
    if (gw > 0 && gh > 0 && gw < 1024 && gh < 1024) {
      bmp.assign((size_t)gw * gh, 0);
      stbtt_MakeCodepointBitmapSubpixel(&f.info, bmp.data(), gw, gh, gw, sc, sc, fx, fyy, cp);
      int ox = (int)ix + gx0 - m.x0, oy = (int)iy + gy0 - m.y0;
      for (int yy = 0; yy < gh; yy++) {
        int my = oy + yy; if (my < 0 || my >= m.h) continue;
        for (int xx = 0; xx < gw; xx++) {
          int mx = ox + xx; if (mx < 0 || mx >= m.w) continue;
          unsigned char v = bmp[(size_t)yy * gw + xx];
          if (v) { float &d = m.a[(size_t)my * m.w + mx]; d = std::min(1.f, d + v / 255.f); }
        }
      }
    }
    xp += adv * sc + ls; prev = cp;
  }
}
static void maskBlit(Canvas &c, const Mask &m, float dx, float dy, RGBA col) {
  // posun o necele px: bilinearne (stiny textu maji posuny ~1 CSS px)
  int ix = (int)std::floor(dx), iy = (int)std::floor(dy);
  float fx = dx - ix, fy = dy - iy;
  int X0 = m.x0 + ix, Y0 = m.y0 + iy;
  int x0 = std::max(c.clip.x0, X0), y0 = std::max(c.clip.y0, Y0);
  int x1 = std::min(c.clip.x1, X0 + m.w + 1), y1 = std::min(c.clip.y1, Y0 + m.h + 1);
  auto A = [&](int mx, int my) -> float { return (mx < 0 || my < 0 || mx >= m.w || my >= m.h) ? 0.f : m.a[(size_t)my * m.w + mx]; };
  for (int y = y0; y < y1; y++) {
    uint32_t *row = &c.at(0, y);
    int my = y - Y0;
    for (int x = x0; x < x1; x++) {
      int mx = x - X0;
      float a = A(mx, my) * (1 - fx) * (1 - fy) + A(mx - 1, my) * fx * (1 - fy)
              + A(mx, my - 1) * (1 - fx) * fy + A(mx - 1, my - 1) * fx * fy;
      a *= col.a;
      if (a > 0.002f) blendPx(row[x], col.r, col.g, col.b, a);
    }
  }
}
static void maskBlur(Mask &m, float sigma) {           // 3x krabicove rozmazani ~ gauss
  if (sigma < 0.4f || m.w == 0 || m.h == 0) return;
  int r = std::max(1, (int)std::lround(sigma * 0.95f));
  std::vector<float> t(m.a.size());
  for (int pass = 0; pass < 3; pass++) {
    for (int y = 0; y < m.h; y++) {
      float acc = 0; int n = 2 * r + 1;
      for (int x = -r; x <= r; x++) acc += (x >= 0 && x < m.w) ? m.a[(size_t)y * m.w + x] : 0.f;
      for (int x = 0; x < m.w; x++) {
        t[(size_t)y * m.w + x] = acc / n;
        int xo = x - r, xi = x + r + 1;
        acc -= (xo >= 0 && xo < m.w) ? m.a[(size_t)y * m.w + xo] : 0.f;
        acc += (xi >= 0 && xi < m.w) ? m.a[(size_t)y * m.w + xi] : 0.f;
      }
    }
    for (int x = 0; x < m.w; x++) {
      float acc = 0; int n = 2 * r + 1;
      for (int y = -r; y <= r; y++) acc += (y >= 0 && y < m.h) ? t[(size_t)y * m.w + x] : 0.f;
      for (int y = 0; y < m.h; y++) {
        m.a[(size_t)y * m.w + x] = acc / n;
        int yo = y - r, yi = y + r + 1;
        acc -= (yo >= 0 && yo < m.h) ? t[(size_t)yo * m.w + x] : 0.f;
        acc += (yi >= 0 && yi < m.h) ? t[(size_t)yi * m.w + x] : 0.f;
      }
    }
  }
}
// Jednoduchy text (bez stinu) primo do platna
static void drawText(Canvas &c, Font &f, float x, float baseline, const char *s, float fs, float ls, RGBA col) {
  if (!s || !s[0]) return;
  float w = textW(f, s, fs, ls);
  Mask m; int mx0 = (int)std::floor(x) - 2, my0 = (int)std::floor(baseline - fs * 1.3f) - 2;
  m.init(mx0, my0, (int)std::ceil(w + fs * 0.4f) + 6, (int)std::ceil(fs * 1.9f) + 6);
  IRect mr{ m.x0, m.y0, m.x0 + m.w, m.y0 + m.h };
  if (irEmpty(irIsect(mr, c.clip))) return;
  textToMask(m, f, x, baseline, s, fs, ls);
  maskBlit(c, m, 0, 0, col);
}
// CSS radek textu: box (top, vyska radku lh) -> uaktualni baseline podle
// pravidla "half-leading" (presne model prohlizece).
static inline float baselineIn(Font &f, float top, float lineH, float fs) {
  float content = (f.asc + f.desc) * fs;
  return top + (lineH - content) * 0.5f + f.asc * fs;
}

// =====================================================================
//  GEOMETRIE SCHVALENEHO NAVRHU (du, 941x1672)
// =====================================================================
static const float DW = 941.f, DH = 1672.f;
static const float CSSPX_DU = 941.f / 376.f;   // 1 CSS px na telefonu (pristroj ~376 CSS px)
static const float CQW = 9.41f;                // 1cqw = 1 % sirky pristroje
// Jemne svisle doladeni textu (v CSS px), zmerene proti Chromium renderu
// navrhu: prohlizec zaokrouhluje metriky pisma na cele pixely, tim posouva
// popisky klaves o ~2 px vys a napis ATARI 130XE o ~2 px niz nez presny
// vypocet. Tady stejne, at to sedi 1:1 s tim, co Rene videl.
static const float TEXT_DY_KEY = -0.8f, TEXT_DY_SVC = -0.4f, TEXT_DY_WM = 0.8f;

// 57 klaves - pozice/popisky 1:1 z rowX/rowY/rowW/rowH/rowLabels navrhu,
// scankody ze stareho overeneho JS jadra (emu_vbxe ROWS) - stejne jako B290.
// scan: -1=CTRL (zamek), -2=SHIFT (zamek), -3=BREAK
struct KeyDef { float x, y, w, h; int16_t scan; const char *l1; const char *l2; };
static const KeyDef KEYS[57] = {
  {41,901,58,64,28,"","Esc"},{108,901,51,64,31,"!","1"},{168,901,50,64,30,"\"","2"},{227,901,48,64,26,"#","3"},
  {284,901,48,64,24,"$","4"},{340,901,47,64,29,"%","5"},{395,901,48,64,27,"&","6"},{452,901,45,64,51,"'","7"},
  {505,901,46,64,53,"@","8"},{560,901,45,64,48,"(","9"},{614,901,46,64,50,")","0"},{667,901,48,64,54,"Clear","<"},
  {723,901,48,64,55,"Insert",">"},{779,901,50,64,52,"Delete","BkSp"},{837,901,58,64,-3,"","Break"},
  {41,966,81,67,44,"Clr Set","Tab"},{130,966,54,67,47,"","Q"},{190,966,52,67,46,"","W"},{249,966,53,67,42,"","E"},
  {308,966,52,67,40,"","R"},{367,966,50,67,45,"","T"},{423,966,49,67,43,"","Y"},{478,966,50,67,11,"","U"},
  {533,966,47,67,13,"","I"},{587,966,49,67,8,"","O"},{641,966,49,67,10,"","P"},{696,966,49,67,14,"\xE2\x86\x91","-"},
  {751,966,51,67,15,"\xE2\x86\x93","="},{808,966,87,67,12,"","Return"},
  {41,1034,90,65,-1,"","Control"},{141,1034,52,65,63,"","A"},{202,1034,50,65,62,"","S"},{261,1034,50,65,58,"","D"},
  {320,1034,50,65,56,"","F"},{378,1034,49,65,61,"","G"},{437,1034,46,65,57,"","H"},{492,1034,47,65,1,"","J"},
  {548,1034,45,65,5,"","K"},{601,1034,47,65,0,"","L"},{656,1034,45,65,2,":",";"},{710,1034,49,65,6,"\xE2\x86\x90","+"},
  {766,1034,52,65,7,"\xE2\x86\x92","*"},{825,1034,70,65,60,"","Caps"},
  {41,1101,121,68,-2,"","Shift"},{171,1101,51,68,23,"","Z"},{231,1101,48,68,22,"","X"},{289,1101,49,68,18,"","C"},
  {345,1101,48,68,16,"","V"},{403,1101,46,68,21,"","B"},{459,1101,46,68,35,"","N"},{514,1101,47,68,37,"","M"},
  {570,1101,47,68,32,"[","/"},{626,1101,46,68,34,"]","."},{680,1101,45,68,38,"?","/"},{734,1101,91,68,-2,"","Shift"},
  {833,1101,62,68,39,"Fuji","Inverse"},
  {216,1175,484,52,33,"",""},
};
// Konzolova lista (consoleDefs) - zamerne se prekryva, kazde dalsi navrchu.
struct ConDef { float x, w; const char *lab; };
static const ConDef CONSOLE[5] = {
  {316.9f,179.2f,"HELP"},{436.4f,163.2f,"START"},{539.9f,165.2f,"SELECT"},{645.4f,165.7f,"OPTION"},{751.4f,163.2f,"RESET"},
};
static const float CON_Y = 740.f, CON_H = 73.f;
// Tlacitka kazety (tapeBounds) - poradi REC/REW/PLAY/FWD/STOP/EJECT
static const float TAPE_B[7] = {223, 302, 383, 465, 547, 627, 710};
static const float TAPE_Y = 1440.f, TAPE_H = 70.f;
// Servisni tlacitka (serviceDefs)
struct SvcDef { const char *l1; const char *l2; uint32_t led; bool small; };
static const SvcDef SVC[8] = {
  {"NET","HRY",0xff8a00,false},{"XEX","MOBIL",0x00e2da,false},{"ATR","DISK",0x1ab4ff,false},
  {"TURBO","BASIC",0x00e2da,false},{"BASIC/TBXL","TXT",0x1a8cff,true},{"LOG","CHYBA",0xff8a00,false},
  {"","HELP",0xff8a00,false},{"","MENU",0xff8a00,false},
};
static const float SVC_Y = 1537.f, SVC_H = 78.f, SVC_X0 = 42.f, SVC_W = 100.f, SVC_GAP = 11.f;
// Okenko kazety (twin) - presne procenta z navrhu
static const float TB_X = 218.f, TB_Y = 1260.f, TB_W = 497.f, TB_H = 177.f;
static const float TW_X = TB_X + TB_W * 0.0604f, TW_Y = TB_Y + TB_H * 0.0565f;
static const float TW_W = TB_W * 0.8793f, TW_H = TB_H * 0.8362f;

// Identifikatory dotykovych prvku
enum : int {
  ID_NONE = -1, ID_KEY0 = 0, ID_CON0 = 100, ID_TAPE0 = 200, ID_DOOR = 210, ID_CNTRST = 211,
  ID_SVC0 = 300, ID_POWER = 400,
};
// Servisni akce (vraci se Jave - otevreni vyberu souboru, dialogu, menu...)
enum : int {
  SVC_NET = 1, SVC_XEX = 2, SVC_ATR = 3, SVC_TBXL = 4, SVC_TXT = 5, SVC_LOG = 6, SVC_HELP = 7, SVC_MENU = 8,
  SVC_EJECT = 9,   // B292: EJECT na kazetaku -> Java nabidne ulozene WAV (kazety)
};

// Udalost pro skutecny stroj (zpracuje ji emulacni vlakno)
// B292: KEYUP (pusteni klavesy - OS pak klavesu opakuje jako skutecna
// klavesnice, dokud je drzena), SHIFT (zamek SHIFT = SKSTAT bit 3),
// TAPE (v: bit0 PLAY, bit1 REC), REWIND/FFWD (pretoceni pasky).
struct Ev {
  enum T { NONE, KEY, CONSOL, RESET, BREAK, POWER_ON, POWER_OFF, KEYUP, SHIFT, TAPE, REWIND, FFWD } t = NONE;
  int v = 0;
};

// cubic-bezier(x1,y1,x2,y2) jako v CSS transition
static inline float cubicBezier(float x1, float y1, float x2, float y2, float x) {
  if (x <= 0) return 0;
  if (x >= 1) return 1;
  float t = x;
  for (int i = 0; i < 8; i++) {
    float u = 1 - t;
    float bx = 3 * u * u * t * x1 + 3 * u * t * t * x2 + t * t * t;
    float dx = 3 * u * u * x1 + 6 * u * t * (x2 - x1) + 3 * t * t * (1 - x2);
    if (std::fabs(dx) < 1e-5f) break;
    t -= (bx - x) / dx;
    if (t < 0) t = 0;
    if (t > 1) t = 1;
  }
  float u = 1 - t;
  return 3 * u * u * t * y1 + 3 * u * t * t * y2 + t * t * t;
}

// =====================================================================
//  ZARIZENI
// =====================================================================
class Device {
public:
  static const int MIN_PRESS_MS = 110;   // kratke tuknuti je videt aspon takhle dlouho

  // --- stav, ktery vidi uzivatel ---
  bool keyHeld[57] = {}; long long keyAt[57] = {};
  bool conHeld[5] = {};  long long conAt[5] = {};
  bool shiftLatched = false, ctrlLatched = false;
  bool power = true;     long long powerAt = -100000;
  bool rec = false, play = false;
  bool tapeHeld[6] = {}; long long tapeAt[6] = {};
  bool doorOpen = false; long long doorAt = -100000;
  int counter = 0;       long long counterAcc = 0; long long fastAcc = 0;
  bool swHeld = false;   long long swAt = -100000;
  bool svcHeld[8] = {};  long long svcAt[8] = {};
  float reelAngle = 0.f;
  bool motorOn = false;                  // kazetovy motor Atari (PACTL) - zvenku
  char extraMsg[96] = {0}; long long extraUntil = 0;

  // --- vystup ---
  int W = 0, H = 0;                      // velikost bufferu (px)
  float s = 1.f, devX = 0, devY = 0;     // mapovani du -> px
  std::vector<uint32_t> bg, frame, out;  // pozadi (staticke prvky), snimek, snimek + POWER navrchu
  std::vector<IRect> dirty;
  bool needPresent = true;

  // --- obraz Atari (384x240, RGBA jako AnticView::fb) ---
  static const int AW = 384, AH = 240;
  std::vector<uint32_t> atari;  bool atariValid = false;

  Device() { atari.assign((size_t)AW * AH, 0xFF000000u); }

  inline float X(float du) const { return devX + du * s; }
  inline float Y(float du) const { return devY + du * s; }
  inline float L(float du) const { return du * s; }
  inline float PX(float cssPx) const { return cssPx * CSSPX_DU * s; }   // CSS px -> px

  // ---------------- rozlozeni ----------------
  void layout(int w, int h) {
    W = std::max(1, w); H = std::max(1, h);
    // jako stranka navrhu: okraj 14 CSS px, nahore 18 CSS px, pod
    // pristrojem stavovy radek (legend) 14+~16 CSS px.
    float margin = W * (14.f / 404.f);
    float devW = W - 2 * margin;
    float sTry = devW / DW;
    float top = 18.f * CSSPX_DU * sTry;
    float legendH = (14.f + 18.f) * CSSPX_DU * sTry;
    if (top + DH * sTry + legendH > H) {             // na sirku / nizky displej
      float avail = H - 2 * margin - legendH;
      sTry = std::max(0.05f, avail / DH);
      top = margin;
    }
    s = sTry;
    devX = std::floor((W - DW * s) * 0.5f);
    // zbyva-li misto, pristroj + stavovy radek svisle na stred displeje
    float blok = DH * s + (14.f + 18.f) * CSSPX_DU * s;
    float volno = H - blok;
    devY = std::floor(volno > 2 * top ? volno * 0.5f : top);
    bg.assign((size_t)W * H, 0xFF0D0B0Bu);
    frame.assign((size_t)W * H, 0xFF0D0B0Bu);
    out.assign((size_t)W * H, 0xFF0D0B0Bu);
    outDirty.clear(); presentRect = IRect{0, 0, 0, 0}; lastOv = -1.f; lastNub = -1.f;
    buildScreenTables();
    renderBackground();
    dirty.clear();
    dirty.push_back(IRect{0, 0, W, H});
    needPresent = true;
  }

  // Pro testy: presne zadane meritko a poloha pristroje.
  void layoutFixed(int w, int h, float scale, float dx, float dy) {
    W = std::max(1, w); H = std::max(1, h); s = scale; devX = dx; devY = dy;
    bg.assign((size_t)W * H, 0xFF0D0B0Bu);
    frame.assign((size_t)W * H, 0xFF0D0B0Bu);
    out.assign((size_t)W * H, 0xFF0D0B0Bu);
    outDirty.clear(); presentRect = IRect{0, 0, 0, 0}; lastOv = -1.f; lastNub = -1.f;
    buildScreenTables();
    renderBackground();
    dirty.clear();
    dirty.push_back(IRect{0, 0, W, H});
    needPresent = true;
  }

  // ---------------- doteky ----------------
  int pidElem[16] = {ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE,
                     ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE, ID_NONE};

  // souradnice displeje (px) -> du
  inline float toDuX(float x) const { return (x - devX) / s; }
  inline float toDuY(float y) const { return (y - devY) / s; }

  int hitTest(float x, float y) const {
    float u = toDuX(x), v = toDuY(y);
    auto in = [&](float x0, float y0, float w, float h, float pad = 0.f) {
      return u >= x0 - pad && u < x0 + w + pad && v >= y0 - pad && v < y0 + h + pad;
    };
    if (in(8, 393, 34, 50, 6)) return ID_POWER;
    if (!power) return ID_NONE;                      // vypnuto: prekryv blokuje vse krome POWER
    for (int i = 0; i < 8; i++) if (in(SVC_X0 + i * (SVC_W + SVC_GAP), SVC_Y, SVC_W, SVC_H)) return ID_SVC0 + i;
    for (int i = 0; i < 6; i++) if (in(TAPE_B[i], TAPE_Y, TAPE_B[i + 1] - TAPE_B[i], TAPE_H)) return ID_TAPE0 + i;
    if (in(855, 1328, 37, 48, 4)) return ID_CNTRST;
    if (doorOpen && in(TW_X, TW_Y, TW_W, TW_H + 14)) return ID_DOOR;
    for (int i = 0; i < 57; i++) if (in(KEYS[i].x, KEYS[i].y, KEYS[i].w, KEYS[i].h)) return ID_KEY0 + i;
    // konzole: prekryv - navrchu je to POZDEJSI, proto odzadu
    for (int i = 4; i >= 0; i--) if (in(CONSOLE[i].x, CON_Y, CONSOLE[i].w, CON_H)) return ID_CON0 + i;
    // prst trefil mezeru mezi klavesami (par du): vezmi nejblizsi klavesu
    int best = -1; float bestD = 9.f;
    for (int i = 0; i < 57; i++) {
      const KeyDef &k = KEYS[i];
      float dx = std::max(0.f, std::max(k.x - u, u - (k.x + k.w)));
      float dy = std::max(0.f, std::max(k.y - v, v - (k.y + k.h)));
      float dd = std::sqrt(dx * dx + dy * dy);
      if (dd < bestD) { bestD = dd; best = i; }
    }
    if (best >= 0) return ID_KEY0 + best;
    return ID_NONE;
  }

  // Polozeni prstu. Vraci pocet udalosti pro stroj (do out).
  int pointerDown(int pid, float x, float y, long long now, Ev *out, int maxOut) {
    int n = 0;
    int id = hitTest(x, y);
    if (pid >= 0 && pid < 16) pidElem[pid] = id;
    if (id == ID_NONE) return 0;
    auto push = [&](Ev::T t, int v) { if (n < maxOut) { out[n].t = t; out[n].v = v; n++; } };
    if (id == ID_POWER) {
      power = !power; powerAt = now;
      push(power ? Ev::POWER_ON : Ev::POWER_OFF, 0);
      if (power) { atariValid = false; markScreen(); }          // novy start: obraz od cerne
      if (!power) {                                   // vypnuti: vse pustit
        for (int i = 0; i < 57; i++) keyHeld[i] = false;
        for (int i = 0; i < 5; i++) conHeld[i] = false;
        rec = play = false;
        for (int i = 0; i < 6; i++) tapeHeld[i] = false;
        for (int i = 0; i < 8; i++) svcHeld[i] = false;
        markAll();
      }
      needPresent = true; markLegend();
      return n;
    }
    if (id >= ID_KEY0 && id < ID_KEY0 + 57) {
      int k = id - ID_KEY0; const KeyDef &d = KEYS[k];
      keyAt[k] = now;
      if (d.scan == -2) {                            // SHIFT: tuknuti = zamek / odemknuti
        shiftLatched = !shiftLatched;
        for (int q = 0; q < 57; q++) if (KEYS[q].scan == -2) markKey(q);   // obe klavesy SHIFT
        push(Ev::SHIFT, shiftLatched ? 1 : 0);
        return n;
      }
      if (d.scan == -1) { ctrlLatched = !ctrlLatched; markKey(k); return n; }
      keyHeld[k] = true; markKey(k);
      if (d.scan == -3) { push(Ev::BREAK, 0); return n; }
      int sc = d.scan;
      if (shiftLatched) sc |= 0x40;
      if (ctrlLatched) sc |= 0x80;
      push(Ev::KEY, sc & 0xFF);
      return n;
    }
    if (id >= ID_CON0 && id < ID_CON0 + 5) {
      int ci = id - ID_CON0;
      conHeld[ci] = true; conAt[ci] = now; markCon(ci);
      if (ci == 0) push(Ev::KEY, 17);                // HELP = klavesa matice (scan 17)
      else if (ci == 4) push(Ev::RESET, 0);
      else push(Ev::CONSOL, consolMask());
      return n;
    }
    if (id >= ID_TAPE0 && id < ID_TAPE0 + 6) {
      int t = id - ID_TAPE0;
      tapeAt[t] = now;
      const bool pr = play, rr = rec;
      if (t == 0) { rec = !rec; if (rec) counterAcc = 0; }
      else if (t == 2) { play = !play; if (play) counterAcc = 0; }
      else {
        tapeHeld[t] = true;
        if (t == 4) { rec = false; play = false; }                       // STOP
        if (t == 5) { rec = false; play = false; setDoor(true, now); }  // EJECT
        if (t == 1 || t == 3) fastAcc = 0;
        if (t == 1) push(Ev::REWIND, 0);
        if (t == 3) push(Ev::FFWD, 0);
      }
      if (pr != play || rr != rec) push(Ev::TAPE, (play ? 1 : 0) | (rec ? 2 : 0));
      markTape(); markWindow(); markCounter(); markLegend();
      return n;
    }
    if (id == ID_CNTRST) { swHeld = true; swAt = now; counter = 0; counterAcc = 0; markCounter(); return n; }
    if (id == ID_DOOR) { setDoor(false, now); return n; }
    if (id >= ID_SVC0 && id < ID_SVC0 + 8) { int i = id - ID_SVC0; svcHeld[i] = true; svcAt[i] = now; markSvc(i); return n; }
    return n;
  }

  // Zvednuti prstu. svcAction (out) = servisni akce pro Javu (0 = zadna).
  int pointerUp(int pid, float x, float y, long long now, Ev *out, int maxOut, int *svcAction) {
    int n = 0;
    if (svcAction) *svcAction = 0;
    int id = (pid >= 0 && pid < 16) ? pidElem[pid] : ID_NONE;
    if (pid >= 0 && pid < 16) pidElem[pid] = ID_NONE;
    if (id == ID_NONE) return 0;
    auto push = [&](Ev::T t, int v) { if (n < maxOut) { out[n].t = t; out[n].v = v; n++; } };
    if (id >= ID_KEY0 && id < ID_KEY0 + 57) {
      int k = id - ID_KEY0; keyHeld[k] = false; markKey(k);
      // pusteni klavesy - jen kdyz uz zadna jina klavesa (ani HELP) neni drzena
      if (KEYS[k].scan >= 0 && !anyKeyHeld()) push(Ev::KEYUP, 0);
      return n;
    }
    if (id >= ID_CON0 && id < ID_CON0 + 5) {
      int ci = id - ID_CON0; conHeld[ci] = false; markCon(ci);
      if (ci >= 1 && ci <= 3) push(Ev::CONSOL, consolMask());
      if (ci == 0 && !anyKeyHeld()) push(Ev::KEYUP, 0);     // HELP = klavesa matice
      return n;
    }
    if (id >= ID_TAPE0 && id < ID_TAPE0 + 6) {
      tapeHeld[id - ID_TAPE0] = false; markTape(); markWindow();
      // EJECT pusten nad tlacitkem -> Java nabidne kazety (ulozene WAV)
      if (id == ID_TAPE0 + 5 && hitTest(x, y) == id && svcAction) *svcAction = SVC_EJECT;
      return n;
    }
    if (id == ID_CNTRST) { swHeld = false; markCounter(); return n; }
    if (id >= ID_SVC0 && id < ID_SVC0 + 8) {
      int i = id - ID_SVC0; svcHeld[i] = false; markSvc(i);
      // akce az pri pusteni NAD stejnym tlacitkem (jako bezne kliknuti)
      if (hitTest(x, y) == id && svcAction) *svcAction = i + 1;
      return n;
    }
    (void)x; (void)y; (void)now;
    return n;
  }
  int pointerCancelAll(long long now, Ev *out, int maxOut) {
    int n = 0;
    for (int p = 0; p < 16; p++) if (pidElem[p] != ID_NONE) {
      int dummy = 0; n += pointerUp(p, -1e6f, -1e6f, now, out + n, maxOut - n, &dummy);
    }
    return n;
  }
  // drzi prst jeste nejakou klavesu matice (vcetne HELP)?
  bool anyKeyHeld() const {
    for (int i = 0; i < 57; i++) if (keyHeld[i] && KEYS[i].scan >= 0) return true;
    return conHeld[0];
  }
  int consolMask() const {
    int m = 0;
    if (conHeld[1]) m |= 1;
    if (conHeld[2]) m |= 2;
    if (conHeld[3]) m |= 4;
    return 7 & ~m;
  }

  void setDoor(bool open, long long now) {
    if (doorOpen == open) return;
    // pri prerusene animaci navazat (CSS transition se otaci z aktualniho stavu)
    float p = doorProgressRaw(now);
    doorOpen = open;
    long long dur = 320;
    float remain = open ? (1.f - p) : p;     // kolik zbyva do noveho cile (linearne)
    doorAt = now - (long long)((1.f - remain) * dur);
    markWindow();
  }
  // 0 = zavreno, 1 = otevreno (linearni cas, pred easingem)
  float doorProgressRaw(long long now) const {
    float t = (float)(now - doorAt) / 320.f; if (t < 0) t = 0; if (t > 1) t = 1;
    return doorOpen ? t : 1.f - t;
  }
  float doorProgressEased(long long now) const {
    float t = (float)(now - doorAt) / 320.f; if (t < 0) t = 0; if (t > 1) t = 1;
    float e = cubicBezier(0.34f, 1.45f, 0.64f, 1.f, t);
    return doorOpen ? e : 1.f - e;
  }

  // Vychozi stav pri vstupu do HELP (jako zapnuti pristroje)
  void resetUi() {
    for (int i = 0; i < 57; i++) { keyHeld[i] = false; keyAt[i] = -100000; }
    for (int i = 0; i < 5; i++) { conHeld[i] = false; conAt[i] = -100000; }
    for (int i = 0; i < 6; i++) { tapeHeld[i] = false; tapeAt[i] = -100000; }
    for (int i = 0; i < 8; i++) { svcHeld[i] = false; svcAt[i] = -100000; }
    for (int i = 0; i < 16; i++) pidElem[i] = ID_NONE;
    shiftLatched = ctrlLatched = false;
    power = true; powerAt = -100000;
    rec = play = false; doorOpen = false; doorAt = -100000;
    counter = 0; counterAcc = 0; fastAcc = 0; swHeld = false; swAt = -100000;
    reelAngle = 0; motorOn = false; extraMsg[0] = 0; extraUntil = 0;
    atariValid = false;
    if (W > 1) markAll();
  }

  void setStatusMessage(const char *msg, long long now, int ms) {
    std::snprintf(extraMsg, sizeof(extraMsg), "%s", msg ? msg : "");
    extraUntil = now + ms; markLegend();
  }

  bool reelsSpinning() const { return power && (play || motorOn || tapeHeld[1] || tapeHeld[3]); }

  // ---------------- casovy krok (animace) ----------------
  long long lastNow = -1;
  void update(long long now) {
    if (lastNow < 0) lastNow = now;
    long long dt = now - lastNow; if (dt < 0) dt = 0; if (dt > 250) dt = 250;
    lastNow = now;
    // konec minimalni doby "zmacknuto" po kratkem tuknuti
    for (int i = 0; i < 57; i++) if (!keyHeld[i] && within(now - dt, keyAt[i]) != within(now, keyAt[i])) markKey(i);
    for (int i = 0; i < 5; i++) if (!conHeld[i] && within(now - dt, conAt[i]) != within(now, conAt[i])) markCon(i);
    for (int i = 0; i < 6; i++) if (!tapeHeld[i] && within(now - dt, tapeAt[i]) != within(now, tapeAt[i])) { markTape(); }
    for (int i = 0; i < 8; i++) if (!svcHeld[i] && within(now - dt, svcAt[i]) != within(now, svcAt[i])) markSvc(i);
    if (!swHeld && within(now - dt, swAt) != within(now, swAt)) markCounter();
    // pocitadlo: +1 kazdou vterinu pri PLAY/REC (navrh: setInterval 1000ms)
    if (power && (play || rec || motorOn)) {
      counterAcc += dt;
      while (counterAcc >= 1000) { counterAcc -= 1000; counter = (counter + 1) % 10000; markCounter(); }
    }
    // REW/FWD: rychle pretaceni pocitadla
    if (power && (tapeHeld[1] || tapeHeld[3])) {
      fastAcc += dt;
      while (fastAcc >= 60) {
        fastAcc -= 60;
        if (tapeHeld[3]) counter = (counter + 1) % 10000;
        else if (counter > 0) counter--;
        markCounter();
      }
    }
    // civky: 1 otacka za 0,6 s (navrh: animation spin .6s linear)
    if (reelsSpinning()) {
      float speed = 360.f / 600.f;
      if (tapeHeld[1]) speed = -speed * 4.f; else if (tapeHeld[3]) speed *= 4.f;
      reelAngle = std::fmod(reelAngle + speed * dt + 3600.f, 360.f);
      markReels();
    }
    // dvirka - animace 320 ms
    if (now - doorAt <= 340) markWindow();
    // POWER - prechod prekryvu a jezdce 150 ms
    if (now - powerAt <= 170) needPresent = true;
    if (extraUntil && now > extraUntil) { extraUntil = 0; extraMsg[0] = 0; markLegend(); }
  }
  static inline bool within(long long now, long long at) { return now - at < MIN_PRESS_MS && now >= at; }
  bool keyVisual(int i, long long now) const { return keyHeld[i] || within(now, keyAt[i]); }

  // ---------------- obraz Atari ----------------
  // Novy snimek z jadra. Obrazovka se prekresli JEN kdyz se obraz opravdu
  // zmenil (napr. READY s blikajicim kurzorem je vetsinu snimku stejny).
  void setAtariFrame(const uint32_t *fb) {
    if (!fb) return;
    if (atariValid && std::memcmp(atari.data(), fb, atari.size() * 4) == 0) return;
    std::memcpy(atari.data(), fb, atari.size() * 4); atariValid = true; hrowsValid = false;
    markScreen();
  }
  void clearAtariFrame() { atariValid = false; markScreen(); }

  // ---------------- spinave obdelniky ----------------
  IRect duRect(float x, float y, float w, float h, float padCss = 14.f) const {
    float p = PX(padCss);
    return IRect{ (int)std::floor(X(x) - p), (int)std::floor(Y(y) - p), (int)std::ceil(X(x + w) + p), (int)std::ceil(Y(y + h) + p * 1.6f) };
  }
  void mark(const IRect &r) { IRect c = irIsect(r, IRect{0, 0, W, H}); if (!irEmpty(c)) dirty.push_back(c); needPresent = true; }
  void markAll() { mark(IRect{0, 0, W, H}); }
  void markKey(int k) { const KeyDef &d = KEYS[k]; mark(duRect(d.x, d.y, d.w, d.h)); }
  void markCon(int i) { mark(duRect(CONSOLE[i].x, CON_Y, CONSOLE[i].w, CON_H)); }
  void markTape() { mark(duRect(TAPE_B[0], TAPE_Y, TAPE_B[6] - TAPE_B[0], TAPE_H)); }
  void markWindow() { mark(duRect(TW_X, TW_Y, TW_W, TW_H, 26.f)); }
  void markReels() { mark(duRect(332.44f, 1317.44f, 602.22f - 332.44f, 66.5f, 3.f)); }
  void markCounter() { mark(duRect(748, 1328, 144, 48, 8.f)); }
  void markSvc(int i) { mark(duRect(SVC_X0 + i * (SVC_W + SVC_GAP), SVC_Y, SVC_W, SVC_H, 10.f)); }
  void markScreen() { mark(IRect{ (int)std::floor(X(108)) - 1, (int)std::floor(Y(105)) - 1, (int)std::ceil(X(823)) + 1, (int)std::ceil(Y(710)) + 1 }); }
  void markLegend() { mark(IRect{0, (int)std::floor(Y(DH)), W, H}); }

  // =================================================================
  //  KRESLENI
  // =================================================================
  // Pozadi stranky + stin skrine + skrin + vsechny NEMENNE prvky, ktere v
  // navrhu lezi POD vsemi menicimi se prvky (logo, ramecek obrazovky, napis,
  // barevne pruhy, deska klavesnice, telo kazety, mrizky).
  void renderBackground() {
    Canvas c; c.px = bg.data(); c.w = W; c.h = H; c.stride = W; c.clip = IRect{0, 0, W, H};
    // stranka navrhu: body{background:#0b0b0d}
    std::fill(bg.begin(), bg.end(), packRGB(11, 11, 13));
    RRect dev = deviceRR();
    // .devicebox{box-shadow:0 16px 36px rgba(0,0,0,.55)}
    shadowOut(c, dev, 0, PX(16), PX(36), 0, rgbac(0, 0, 0, .55f));
    // .devicebox{background:linear-gradient(180deg,#e4e3e2 0%,#c9c7c6 52%,#a7a5a4 100%)}
    fillRR(c, dev, Grad::three(hexc(0xe4e3e2), hexc(0xc9c7c6), .52f, hexc(0xa7a5a4)), dev.y0, dev.y1);
    drawLogo(c);
    drawBezel(c);
    drawWordmark(c);
    drawStripes(c);
    drawDeckPlate(c);
    drawTapeBody(c);
    drawGrilles(c);
  }
  RRect deviceRR() const { return mkRR(X(0), Y(0), L(DW), L(DH), L(DW) * 0.036f, L(DH) * 0.02f); }

  // Prekresli spinave oblasti do "frame" a pak slozi "out" (= frame +
  // tmavy prekryv pri vypnuti + vypinac POWER navrchu). Vsechny zmenene
  // oblasti se scitaji do presentRect - na displej se pak kopiruje JEN ta
  // (ANativeWindow_lock s dirty obdelnikem), ne cely obraz.
  std::vector<IRect> outDirty;
  IRect presentRect{0, 0, 0, 0};
  float lastOv = -1.f, lastNub = -1.f;
  float overlayAlpha(long long now) const {
    // .pwroverlay{background:#06060a;opacity:0 -> .62;transition:opacity .15s ease}
    float t = std::min(1.f, std::max(0.f, (float)(now - powerAt) / 150.f));
    float e = cubicBezier(0.25f, 0.1f, 0.25f, 1.f, t);
    return power ? (1.f - e) * 0.62f : e * 0.62f;
  }
  float nubOn(long long now) const {
    float t = std::min(1.f, std::max(0.f, (float)(now - powerAt) / 150.f));
    float e = cubicBezier(0.25f, 0.1f, 0.25f, 1.f, t);
    return power ? e : 1.f - e;
  }
  IRect powerSwitchRect() const { return duRect(8, 393, 34, 50, 8.f); }
  IRect deviceIRect() const {
    return IRect{ (int)std::floor(X(0)) - 1, (int)std::floor(Y(0)) - 1, (int)std::ceil(X(DW)) + 1, (int)std::ceil(Y(DH)) + 1 };
  }
  void render(long long now) {
    if (dirty.size() > 40) {
      IRect u{0, 0, 0, 0}; for (auto &r : dirty) u = irUnion(u, r);
      dirty.clear(); dirty.push_back(u);
    }
    std::vector<IRect> todo; todo.swap(dirty);
    for (const IRect &r0 : todo) {
      IRect r = irIsect(r0, IRect{0, 0, W, H});
      if (irEmpty(r)) continue;
      for (int y = r.y0; y < r.y1; y++)
        std::memcpy(&frame[(size_t)y * W + r.x0], &bg[(size_t)y * W + r.x0], (size_t)(r.x1 - r.x0) * 4);
      Canvas c; c.px = frame.data(); c.w = W; c.h = H; c.stride = W; c.clip = r;
      drawDynamic(c, now);
      outDirty.push_back(r);
    }
    float ov = overlayAlpha(now), nub = nubOn(now);
    if (std::fabs(ov - lastOv) > 0.001f) { outDirty.push_back(irIsect(deviceIRect(), IRect{0, 0, W, H})); lastOv = ov; }
    if (std::fabs(nub - lastNub) > 0.001f) { outDirty.push_back(irIsect(powerSwitchRect(), IRect{0, 0, W, H})); lastNub = nub; }
    if (outDirty.empty()) return;
    if (outDirty.size() > 40) {
      IRect u{0, 0, 0, 0}; for (auto &r : outDirty) u = irUnion(u, r);
      outDirty.clear(); outDirty.push_back(u);
    }
    if (out.size() != frame.size()) out.assign(frame.size(), 0xFF000000u);
    RRect dev = deviceRR();
    IRect sw = powerSwitchRect();
    for (const IRect &r : outDirty) {
      if (irEmpty(r)) continue;
      for (int y = r.y0; y < r.y1; y++)
        std::memcpy(&out[(size_t)y * W + r.x0], &frame[(size_t)y * W + r.x0], (size_t)(r.x1 - r.x0) * 4);
      Canvas c; c.px = out.data(); c.w = W; c.h = H; c.stride = W; c.clip = r;
      if (ov > 0.003f) fillRR(c, dev, Grad::solid(rgbac(6, 6, 10, 1.f)), 0, 1, ov);
      if (hits(r, sw)) drawPowerSwitch(c, now);
      presentRect = irUnion(presentRect, r);
    }
    outDirty.clear();
  }
  // Vrati (a vynuluje) obdelnik, ktery je treba poslat na displej.
  IRect takePresentRect() { IRect r = presentRect; presentRect = IRect{0, 0, 0, 0}; return r; }
  // Kopie hotoveho obrazu (out) do ciloveho bufferu displeje.
  void copyOut(uint32_t *dst, int dstStride, IRect r) const {
    r = irIsect(r, IRect{0, 0, W, H});
    if (irEmpty(r) || out.empty()) return;
    for (int y = r.y0; y < r.y1; y++)
      std::memcpy(dst + (size_t)y * dstStride + r.x0, &out[(size_t)y * W + r.x0], (size_t)(r.x1 - r.x0) * 4);
  }
  static bool hits(const IRect &clip, const IRect &b) { return !irEmpty(irIsect(clip, b)); }

  void drawDynamic(Canvas &c, long long now) {
    // poradi presne jako v DOM navrhu (pozdejsi = navrchu)
    if (hits(c.clip, IRect{ (int)X(100), (int)Y(95), (int)X(831), (int)Y(720) })) drawScreen(c);
    for (int i = 0; i < 5; i++) if (hits(c.clip, duRect(CONSOLE[i].x, CON_Y, CONSOLE[i].w, CON_H))) drawConsole(c, i, now);
    for (int i = 0; i < 57; i++) if (hits(c.clip, duRect(KEYS[i].x, KEYS[i].y, KEYS[i].w, KEYS[i].h))) drawKey(c, i, now);
    if (hits(c.clip, duRect(TW_X, TW_Y, TW_W, TW_H, 26.f))) drawWindow(c, now);
    if (hits(c.clip, duRect(748, 1328, 99, 48))) drawCounter(c);
    if (hits(c.clip, duRect(855, 1328, 37, 48))) drawResetSwitch(c, now);
    for (int i = 0; i < 6; i++) if (hits(c.clip, duRect(TAPE_B[i], TAPE_Y, TAPE_B[i + 1] - TAPE_B[i], TAPE_H))) drawTapeButton(c, i, now);
    if (hits(c.clip, duRect(25, 1523, 890, 106))) drawSvcPanel(c);
    for (int i = 0; i < 8; i++) if (hits(c.clip, duRect(SVC_X0 + i * (SVC_W + SVC_GAP), SVC_Y, SVC_W, SVC_H))) drawSvcButton(c, i, now);
    if (hits(c.clip, duRect(0, 1649, DW, 18, 2.f))) drawRainbow(c);
    if (hits(c.clip, IRect{0, (int)Y(DH), W, H})) drawLegend(c, now);
  }

  // Pro testy: cely obraz najednou.
  void present(uint32_t *dst, int dstStride, long long now) {
    render(now);
    takePresentRect();
    copyOut(dst, dstStride, IRect{0, 0, W, H});
    needPresent = false;
  }

  // ---------------- jednotlive prvky ----------------
  void drawLogo(Canvas &c) {
    // .logo{border-radius:14%;box-shadow:0 2px 5px rgba(0,0,0,.5)} pb(10,10,58,58.45)
    float x = X(10), y = Y(10), w = L(58), h = L(58.45f);
    RRect r = mkRR(x, y, w, h, w * .14f, h * .14f);
    shadowOut(c, r, 0, PX(2), PX(5), 0, rgbac(0, 0, 0, .5f));
    int x0, y0, x1, y1;
    if (!c.span(r.x0, r.y0, r.x1, r.y1, x0, y0, x1, y1)) return;
    // plocha kazdeho cil. px -> prumer zdrojovych px (plynule zmenseni)
    float sx = (float)NAP_LOGO_W / w, sy = (float)NAP_LOGO_H / h;
    for (int yy = y0; yy < y1; yy++) {
      for (int xx = x0; xx < x1; xx++) {
        float cov = covFromSd(sdRR(xx + .5f, yy + .5f, r));
        if (cov <= 0) continue;
        float u0 = (xx - x) * sx, u1 = u0 + sx, v0 = (yy - y) * sy, v1 = v0 + sy;
        float ar = 0, ag = 0, ab = 0, aw = 0;
        for (int v = (int)std::floor(v0); v < (int)std::ceil(v1); v++) {
          if (v < 0 || v >= NAP_LOGO_H) continue;
          float wy = std::min((float)v + 1, v1) - std::max((float)v, v0); if (wy <= 0) continue;
          for (int u = (int)std::floor(u0); u < (int)std::ceil(u1); u++) {
            if (u < 0 || u >= NAP_LOGO_W) continue;
            float wx = std::min((float)u + 1, u1) - std::max((float)u, u0); if (wx <= 0) continue;
            const unsigned char *p = &NAP_LOGO_RGBA[((size_t)v * NAP_LOGO_W + u) * 4];
            float ww = wx * wy; ar += p[0] * ww; ag += p[1] * ww; ab += p[2] * ww; aw += ww;
          }
        }
        if (aw > 0) blendPx(c.at(xx, yy), ar / aw, ag / aw, ab / aw, cov);
      }
    }
  }

  void drawBezel(Canvas &c) {
    // .screenbezel{border-radius:2.2%;background:#0d0d0f;box-shadow:0 3px 10px
    //  rgba(0,0,0,.5),inset 0 0 0 1px rgba(255,255,255,.04)} pb(90,80,760,660)
    float w = L(760), h = L(660);
    RRect r = mkRR(X(90), Y(80), w, h, w * .022f, h * .022f);
    shadowOut(c, r, 0, PX(3), PX(10), 0, rgbac(0, 0, 0, .5f));
    fillRR(c, r, Grad::solid(hexc(0x0d0d0f)), r.y0, r.y1);
    shadowIn(c, r, 0, 0, 0, PX(1), rgbac(255, 255, 255, .04f));
  }

  void drawWordmark(Canvas &c) {
    // .wordmark pb(60,825,500,50), flex, align-items:center
    // .fuji{width:3.2cqw;height:3.2cqw;margin-right:1cqw;border:1px solid
    //  #aca692;background:linear-gradient(160deg,#f0ece0,#cfc8b4);
    //  clip-path:polygon(50% 2%,96% 96%,4% 96%)}
    float boxY = 825, boxH = 50;
    float fujiSz = 3.2f * CQW + 2.f * CSSPX_DU;           // + ramecek 2x 1px (border-box)
    float fx = 60, fy = boxY + (boxH - fujiSz) * 0.5f;
    drawFuji(c, X(fx), Y(fy), L(fujiSz));
    // .wm-atari{italic 700;font-size:2.9cqw;color:#d8511f;margin-right:1.6cqw;text-shadow:
    //  0 1px 0 rgba(255,170,130,.5),0 -1px 0 rgba(90,20,0,.4),0 .25cqw .3cqw rgba(0,0,0,.3)}
    // .wm-130xe{700;2.9cqw;#2b2a28;text-shadow:0 1px 0 rgba(255,255,255,.55),
    //  0 -1px 0 rgba(0,0,0,.25),0 .25cqw .3cqw rgba(0,0,0,.28)}
    float fs = 2.9f * CQW;
    Font &fi = fBoldItalic(), &fb = fBold();
    float lineH = (fi.asc + fi.desc) * fs;
    float top = boxY + (boxH - lineH) * 0.5f;
    float base = baselineIn(fi, top, lineH, fs);
    float ax = fx + fujiSz + 1.f * CQW;
    float aw = textW(fi, "ATARI", fs, 0);
    float bx = ax + aw + 1.6f * CQW;
    wordmarkText(c, fi, X(ax), Y(base) + PX(TEXT_DY_WM), "ATARI", L(fs), hexc(0xd8511f),
                 rgbac(255, 170, 130, .5f), rgbac(90, 20, 0, .4f), rgbac(0, 0, 0, .3f));
    wordmarkText(c, fb, X(bx), Y(base) + PX(TEXT_DY_WM), "130XE", L(fs), hexc(0x2b2a28),
                 rgbac(255, 255, 255, .55f), rgbac(0, 0, 0, .25f), rgbac(0, 0, 0, .28f));
  }
  void wordmarkText(Canvas &c, Font &f, float x, float base, const char *s, float fs, RGBA col,
                    RGBA hiDown, RGBA loUp, RGBA blurSh) {
    float w = textW(f, s, fs, 0);
    int pad = (int)std::ceil(L(.6f * CQW) + 4);
    Mask m; m.init((int)std::floor(x) - pad, (int)std::floor(base - fs * 1.1f) - pad, (int)std::ceil(w) + 2 * pad, (int)std::ceil(fs * 1.5f) + 2 * pad);
    textToMask(m, f, x, base, s, fs, 0);
    Mask mb = m; maskBlur(mb, L(.3f * CQW) * 0.5f);
    // CSS: prvni stin je navrchu -> kreslit od posledniho
    maskBlit(c, mb, 0, L(.25f * CQW), blurSh);
    maskBlit(c, m, 0, -PX(1), loUp);
    maskBlit(c, m, 0, PX(1), hiDown);
    maskBlit(c, m, 0, 0, col);
  }
  void drawFuji(Canvas &c, float x, float y, float sz) {
    // trojuhelnik (50% 2%, 96% 96%, 4% 96%), gradient 160deg #f0ece0 -> #cfc8b4
    float ax = x + sz * .5f, ay = y + sz * .02f, bx = x + sz * .96f, by = y + sz * .96f, cx = x + sz * .04f, cy = by;
    int x0, y0, x1, y1;
    if (!c.span(x, y, x + sz, y + sz, x0, y0, x1, y1)) return;
    // CSS gradient 160deg: smer (sin160, -cos160); delka = |w sin|+|h cos|
    float ang = 160.f * 3.14159265f / 180.f;
    float dx = std::sin(ang), dy = -std::cos(ang);
    float glen = std::fabs(sz * dx) + std::fabs(sz * dy);
    float mx = x + sz * .5f, my = y + sz * .5f;
    RGBA c0 = hexc(0xf0ece0), c1 = hexc(0xcfc8b4);
    auto edgeSd = [](float px, float py, float x0, float y0, float x1, float y1) {
      float ex = x1 - x0, ey = y1 - y0, len = std::sqrt(ex * ex + ey * ey);
      return ((px - x0) * ey - (py - y0) * ex) / len;   // >0 = vpravo od hrany
    };
    for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) {
      float px = xx + .5f, py = yy + .5f;
      // vrcholy po smeru hodin -> vnitrek je vlevo... pouzijeme max ze tri polorovin
      float d1 = edgeSd(px, py, ax, ay, bx, by), d2 = edgeSd(px, py, bx, by, cx, cy), d3 = edgeSd(px, py, cx, cy, ax, ay);
      float d = std::max(d1, std::max(d2, d3));
      float cov = covFromSd(d);
      if (cov <= 0) continue;
      float t = ((px - mx) * dx + (py - my) * dy) / glen + 0.5f;
      t = std::min(1.f, std::max(0.f, t));
      blendPx(c.at(xx, yy), c0.r + (c1.r - c0.r) * t, c0.g + (c1.g - c0.g) * t, c0.b + (c1.b - c0.b) * t, cov);
    }
  }

  void drawStripes(Canvas &c) {
    // .cstripe{border-radius:2px 2px 0 0} - {277,740,40,73,#bf4310},{277,762,40,18,#0a7183},{277,783,40,30,#06308e}
    struct S { float y, h; uint32_t col; } st[3] = { {740, 73, 0xbf4310}, {762, 18, 0x0a7183}, {783, 30, 0x06308e} };
    for (auto &q : st) {
      float x = X(277), y = Y(q.y), w = L(40), h = L(q.h), r = PX(2);
      // horni rohy zaoblene, spodni ostre: slozeno ze zaobleneho + spodni pulky
      RRect top = mkRR(x, y, w, h, r, r);
      RRect bot = mkRR(x, y + h * 0.5f, w, h * 0.5f, 0, 0);
      fillRR(c, top, Grad::solid(hexc(q.col)), y, y + h);
      fillRR(c, bot, Grad::solid(hexc(q.col)), y, y + h);
    }
  }

  void drawDeckPlate(Canvas &c) {
    // .deckplate{border-radius:2.2%;background:linear-gradient(180deg,#9b9284 0%,#7e7568 100%);
    //  box-shadow:0 .4cqw .7cqw rgba(0,0,0,.35),inset 0 1px 0 rgba(255,255,255,.12)} pb(25,898,890,339)
    float w = L(890), h = L(339);
    RRect r = mkRR(X(25), Y(898), w, h, w * .022f, h * .022f);
    shadowOut(c, r, 0, L(.4f * CQW), L(.7f * CQW), 0, rgbac(0, 0, 0, .35f));
    fillRR(c, r, Grad::two(hexc(0x9b9284), hexc(0x7e7568)), r.y0, r.y1);
    shadowIn(c, r, 0, PX(1), 0, 0, rgbac(255, 255, 255, .12f));
  }

  void drawTapeBody(Canvas &c) {
    // .tapebody{border-radius:2.4%;background:linear-gradient(180deg,#e2dfd8 0%,#c7c3b8 100%);
    //  box-shadow:0 .4cqw .8cqw rgba(0,0,0,.35),inset 0 1px 0 rgba(255,255,255,.2)} pb(218,1260,497,177)
    float w = L(TB_W), h = L(TB_H);
    RRect r = mkRR(X(TB_X), Y(TB_Y), w, h, w * .024f, h * .024f);
    shadowOut(c, r, 0, L(.4f * CQW), L(.8f * CQW), 0, rgbac(0, 0, 0, .35f));
    fillRR(c, r, Grad::two(hexc(0xe2dfd8), hexc(0xc7c3b8)), r.y0, r.y1);
    shadowIn(c, r, 0, PX(1), 0, 0, rgbac(255, 255, 255, .2f));
  }

  // obdelnik v du zarovnany na cele pixely (jako prohlizec u tenkych prvku)
  RRect snapRR(float x, float y, float w, float h, float r) const {
    float x0 = std::round(X(x)), y0 = std::round(Y(y)), x1 = std::round(X(x + w)), y1 = std::round(Y(y + h));
    if (y1 <= y0) y1 = y0 + 1;
    if (x1 <= x0) x1 = x0 + 1;
    return mkRR(x0, y0, x1 - x0, y1 - y0, r, r);
  }
  void drawGrilles(Canvas &c) {
    // .grille{background:#000;opacity:.22;border-radius:1px}
    // L: x37,w155,8 car po 98/7 od y1322; R: x737,w160,5 car po 61/4 od y1393; vyska 2.6
    // prohlizec tenke pruhy zarovna na cele pixely displeje - stejne tady
    for (int i = 0; i < 8; i++) fillRR(c, snapRR(37, 1322 + i * (98.f / 7.f), 155, 2.6f, PX(1)), Grad::solid(rgbac(0, 0, 0, 1)), 0, 1, .22f);
    for (int i = 0; i < 5; i++) fillRR(c, snapRR(737, 1393 + i * (61.f / 4.f), 160, 2.6f, PX(1)), Grad::solid(rgbac(0, 0, 0, 1)), 0, 1, .22f);
  }

  // ----- obrazovka: ZIVY obraz Atari misto nakresleneho textu z navrhu -----
  // .screen{border-radius:1%;box-shadow:inset 0 0 3.5cqw rgba(0,0,0,.55),
  //  inset 0 .3cqw 1cqw rgba(0,0,0,.5)} pb(108,105,715,605)
  // .scanlines{background:repeating-linear-gradient(180deg,rgba(0,0,0,.16) 0 1px,transparent 1px 4px)}
  // Obraz Atari (384x240): orez 8 px z kazde strany (cisty okraj) -> 368
  // sloupcu na celou sirku obrazovky; radky v pomeru PAL televize, na
  // stred (nad a pod je cerna jako na skutecnem CRT).
  static const int CROP_X0 = 24, CROP_W = 336;
  int scrX0 = 0, scrY0 = 0, scrX1 = 0, scrY1 = 0;      // px rozsah obrazovky
  std::vector<uint16_t> scrMod;    // nasobitel jasu 0..256 (stiny+linky)
  std::vector<uint8_t>  scrAdd;    // odlesk skla (pridany jas 0..255)
  std::vector<uint8_t>  scrCov;    // pokryti rohu 0..255
  std::vector<int16_t> colIdx; std::vector<uint8_t> colW;   // ostre-bilinearni vzorkovani
  std::vector<int16_t> rowIdx; std::vector<uint8_t> rowW;
  float picY0 = 0, picH = 0;

  void buildScreenTables() {
    hrowsValid = false;
    float sx = X(108), sy = Y(105), sw = L(715), sh = L(605);
    scrX0 = (int)std::floor(sx); scrY0 = (int)std::floor(sy);
    scrX1 = (int)std::ceil(sx + sw); scrY1 = (int)std::ceil(sy + sh);
    int w = scrX1 - scrX0, h = scrY1 - scrY0;
    scrMod.assign((size_t)w * h, 0); scrCov.assign((size_t)w * h, 0); scrAdd.assign((size_t)w * h, 0);
    // jemny odlesk skla v miste, kde mel navrh stred zare obrazovky
    // (radial-gradient ... at 30% 18%) - jen par procent, aby obraz Atari
    // zustal verny, ale sklo vypadalo jako v navrhu
    float gx = sx + sw * .30f, gy = sy + sh * .18f, gR = sw * .85f;
    RRect r = mkRR(sx, sy, sw, sh, sw * .01f, sh * .01f);
    RRect holeA = r;                                          // inset 0 0 3.5cqw
    RRect holeB = rrMove(r, 0, L(.3f * CQW));                 // inset 0 .3cqw 1cqw
    float sigA = L(3.5f * CQW) * .5f, sigB = L(1.f * CQW) * .5f;
    float period = PX(4), band = PX(1);
    for (int y = 0; y < h; y++) {
      float fy = scrY0 + y + .5f;
      // podil tmaveho prouzku (1px z kazdych 4px) v tomto radku
      float y0 = scrY0 + y - sy, y1 = y0 + 1.f, dark = 0;
      for (float k = std::floor(y0 / period) * period; k < y1; k += period) {
        float a0 = std::max(y0, k), a1 = std::min(y1, k + band);
        if (a1 > a0) dark += a1 - a0;
      }
      float scan = 1.f - .16f * std::min(1.f, dark);
      for (int x = 0; x < w; x++) {
        float fx = scrX0 + x + .5f;
        float d = sdRR(fx, fy, r);
        float cov = covFromSd(d);
        float a1 = .55f * (1.f - gaussEdge(sdRR(fx, fy, holeA), sigA));
        float a2 = .5f * (1.f - gaussEdge(sdRR(fx, fy, holeB), sigB));
        float gd = std::sqrt((fx - gx) * (fx - gx) + (fy - gy) * (fy - gy)) / gR;
        float glow = gd >= 1.f ? 0.f : (1.f - gd) * (1.f - gd);
        float m = (1.f - a1) * (1.f - a2) * scan * (1.f + .10f * glow);
        scrMod[(size_t)y * w + x] = (uint16_t)std::lround(m * 256.f);
        scrAdd[(size_t)y * w + x] = (uint8_t)std::lround(glow * 9.f * (1.f - a1) * scan);
        scrCov[(size_t)y * w + x] = (uint8_t)std::lround(cov * 255.f);
      }
    }
    // vodorovne: CROP_W sloupcu na celou sirku
    float kx = sw / CROP_W;
    colIdx.assign(w, 0); colW.assign(w, 0);
    for (int x = 0; x < w; x++) {
      float u0 = (scrX0 + x - sx) / kx, u1 = u0 + 1.f / kx;
      int i0 = (int)std::floor(u0), i1 = (int)std::floor(u1 - 1e-4f);
      float wgt = 0.f;
      if (i1 > i0) wgt = (u1 - (float)i1) / (u1 - u0);       // podil praveho sousedniho px
      i0 = std::min(CROP_W - 1, std::max(0, i0));
      colIdx[x] = (int16_t)(CROP_X0 + i0);
      colW[x] = (uint8_t)std::lround(std::min(1.f, std::max(0.f, wgt)) * 255.f);
    }
    // svisle: vsech 240 radku tak, aby obraz vyplnil skoro celou vysku skla
    // (jako televize s dotazenou vyskou obrazu) - bod je o ~12 % vyssi nez sirsi.
    float ky = kx / 0.88f;
    picH = AH * ky; picY0 = sy + (sh - picH) * 0.5f;
    rowIdx.assign(h, -1); rowW.assign(h, 0);
    for (int y = 0; y < h; y++) {
      float v0 = (scrY0 + y - picY0) / ky, v1 = v0 + 1.f / ky;
      if (v1 <= 0.f || v0 >= AH) { rowIdx[y] = -1; continue; }
      int j0 = (int)std::floor(v0), j1 = (int)std::floor(v1 - 1e-4f);
      float wgt = 0.f;
      if (j1 > j0) wgt = (v1 - (float)j1) / (v1 - v0);
      // castecne mimo obraz (horni/dolni hrana): stmavit podilem
      rowIdx[y] = (int16_t)std::min(AH - 1, std::max(0, j0));
      if (j0 < 0) { rowIdx[y] = 0; wgt = std::min(1.f, std::max(0.f, v1)) ; rowW[y] = (uint8_t)std::lround(wgt * 255.f); rowIdx[y] = -2; continue; }
      if (j1 >= AH) { rowIdx[y] = (int16_t)(AH - 1); rowW[y] = (uint8_t)std::lround((1.f - wgt) * 255.f); rowIdx[y] = (int16_t)(-3 - (AH - 1)); continue; }
      rowW[y] = (uint8_t)std::lround(std::min(1.f, std::max(0.f, wgt)) * 255.f);
    }
  }
  static inline uint32_t lerpPx(uint32_t a, uint32_t b, int w) {   // w 0..255 (podil b)
    if (w <= 0) return a;
    if (w >= 255) return b;
    int iw = 256 - w - (w >> 7);  // ~ (255-w)/255*256
    int ww = 256 - iw;
    uint32_t r = (((a & 255) * iw + (b & 255) * ww) >> 8);
    uint32_t g = ((((a >> 8) & 255) * iw + ((b >> 8) & 255) * ww) >> 8);
    uint32_t bl = ((((a >> 16) & 255) * iw + ((b >> 16) & 255) * ww) >> 8);
    return 0xFF000000u | (bl << 16) | (g << 8) | r;
  }
  // Vodorovne zvetsene radky obrazu Atari (prepocitavaji se jen pri zmene snimku)
  std::vector<uint32_t> hrows; bool hrowsValid = false;
  void buildHRows() {
    const int w = scrX1 - scrX0;
    hrows.resize((size_t)AH * w);
    for (int j = 0; j < AH; j++) {
      const uint32_t *src = &atari[(size_t)j * AW];
      uint32_t *dst = &hrows[(size_t)j * w];
      for (int tx = 0; tx < w; tx++) {
        const int ci = colIdx[tx], cw = colW[tx];
        dst[tx] = cw ? lerpPx(src[ci], src[ci + 1], cw) : src[ci];
      }
    }
    hrowsValid = true;
  }
  void drawScreen(Canvas &c) {
    int x0 = std::max(c.clip.x0, scrX0), x1 = std::min(c.clip.x1, scrX1);
    int y0 = std::max(c.clip.y0, scrY0), y1 = std::min(c.clip.y1, scrY1);
    if (x1 <= x0 || y1 <= y0) return;
    const int w = scrX1 - scrX0;
    const bool on = power && atariValid;
    if (on && !hrowsValid) buildHRows();
    const uint32_t OFF = packRGB(8, 10, 14), BLACK = 0xFF000000u;
    for (int y = y0; y < y1; y++) {
      const int ty = y - scrY0;
      const uint16_t *modRow = &scrMod[(size_t)ty * w];
      const uint8_t *covRow = &scrCov[(size_t)ty * w];
      const uint8_t *addRow = &scrAdd[(size_t)ty * w];
      const int ri = rowIdx[ty], rw = rowW[ty];
      const uint32_t *A = nullptr, *B = nullptr;
      int mixB = 0, fade = 255;
      if (on && ri != -1) {
        if (ri >= 0) { A = &hrows[(size_t)ri * w]; B = &hrows[(size_t)std::min(AH - 1, ri + 1) * w]; mixB = rw; }
        else if (ri == -2) { A = &hrows[0]; fade = rw; }
        else { A = &hrows[(size_t)(AH - 1) * w]; fade = rw; }
      }
      const uint32_t constCol = on ? BLACK : OFF;
      uint32_t *row = &c.at(0, y);
      for (int x = x0; x < x1; x++) {
        const int tx = x - scrX0;
        uint32_t col = A ? A[tx] : constCol;
        if (mixB) col = lerpPx(col, B[tx], mixB);
        if (fade < 255) col = lerpPx(BLACK, col, fade);
        const uint32_t m = modRow[tx], ad = addRow[tx];
        uint32_t r = (((col & 255) * m) >> 8) + ad, g = ((((col >> 8) & 255) * m) >> 8) + ad, b = ((((col >> 16) & 255) * m) >> 8) + ad;
        if (r > 255) r = 255;
        if (g > 255) g = 255;
        if (b > 255) b = 255;
        const int cov = covRow[tx];
        if (cov >= 255) row[x] = 0xFF000000u | (b << 16) | (g << 8) | r;
        else if (cov) blendPx(row[x], (float)r, (float)g, (float)b, cov / 255.f);
      }
    }
  }

  // ----- konzolova lista -----
  void drawConsole(Canvas &c, int i, long long now) {
    // .cbtn{border-radius:14%/22%;background:linear-gradient(180deg,#f1efea 0%,#dad7d0 55%,#c4c0b7 100%);
    //  box-shadow:inset 0 1.5px 0 rgba(255,255,255,.7),inset 0 -2.5px 3px rgba(60,55,45,.22),
    //  0 2px 0 rgba(0,0,0,.2),0 4px 5px rgba(0,0,0,.25)}
    // .cbtn.pressed{linear-gradient(180deg,#b9b4aa 0%,#c9c5ba 60%,#d9d5c9 100%);
    //  box-shadow:inset 0 2px 5px rgba(40,36,26,.4),0 1px 0 rgba(0,0,0,.15);transform:translateY(2px)}
    // .cbtn span{700;1.55cqw;letter-spacing:.02em;#3a3630} .pressed span{translateY(1px)}
    const ConDef &d = CONSOLE[i];
    bool pr = power && (conHeld[i] || within(now, conAt[i]));
    float w = L(d.w), h = L(CON_H);
    float ty = pr ? PX(2) : 0.f;
    RRect r = mkRR(X(d.x), Y(CON_Y) + ty, w, h, w * .14f, h * .22f);
    if (pr) {
      shadowOut(c, r, 0, PX(1), 0, 0, rgbac(0, 0, 0, .15f));
      fillRR(c, r, Grad::three(hexc(0xb9b4aa), hexc(0xc9c5ba), .60f, hexc(0xd9d5c9)), r.y0, r.y1);
      shadowIn(c, r, 0, PX(2), PX(5), 0, rgbac(40, 36, 26, .4f));
    } else {
      shadowOut(c, r, 0, PX(4), PX(5), 0, rgbac(0, 0, 0, .25f));
      shadowOut(c, r, 0, PX(2), 0, 0, rgbac(0, 0, 0, .2f));
      fillRR(c, r, Grad::three(hexc(0xf1efea), hexc(0xdad7d0), .55f, hexc(0xc4c0b7)), r.y0, r.y1);
      shadowIn(c, r, 0, -PX(2.5f), PX(3), 0, rgbac(60, 55, 45, .22f));
      shadowIn(c, r, 0, PX(1.5f), 0, 0, rgbac(255, 255, 255, .7f));
    }
    Font &f = fBold();
    float fs = 1.55f * CQW, ls = .02f * fs;
    float tw = textW(f, d.lab, fs, ls);
    float lineH = (f.asc + f.desc) * fs;
    float top = CON_Y + (CON_H - lineH) * .5f;
    float base = baselineIn(f, top, lineH, fs);
    float tx = d.x + (d.w - tw) * .5f;
    drawText(c, f, X(tx), Y(base) + ty + (pr ? PX(1) : 0.f), d.lab, L(fs), L(ls), hexc(0x3a3630));
  }

  // ----- klavesy -----
  void drawKey(Canvas &c, int i, long long now) {
    // .key{border-radius:16%/20%;background:linear-gradient(180deg,#f4eee1 0%,#e3d8c4 48%,#cabea8 100%);
    //  box-shadow:inset 0 1.4px 0 rgba(255,255,255,.75),inset 0 -2.6px 3.5px rgba(95,82,58,.28),
    //  0 2px 0 rgba(0,0,0,.22),0 4px 5px rgba(0,0,0,.3)}
    // .key.pressed{linear-gradient(180deg,#c3b89f 0%,#d2c7b1 55%,#e1d6c1 100%);
    //  box-shadow:inset 0 2.2px 5px rgba(60,50,30,.42),0 1px 0 rgba(0,0,0,.18);transform:translateY(2.4px)}
    // .key.shiftLatched{linear-gradient(180deg,#ffd87a 0%,#f3b63e 55%,#e0a02a 100%);
    //  box-shadow:inset 0 1.4px 0 rgba(255,255,255,.75),inset 0 -2.6px 4px rgba(120,70,10,.32),
    //  0 0 0 2px #ffcf5c,0 2px 0 rgba(0,0,0,.22),0 4px 6px rgba(150,95,10,.4)}
    const KeyDef &d = KEYS[i];
    bool gold = (d.scan == -2 && shiftLatched) || (d.scan == -1 && ctrlLatched);
    bool pr = !gold && power && (keyHeld[i] || within(now, keyAt[i]));
    float w = L(d.w), h = L(d.h);
    float ty = pr ? PX(2.4f) : 0.f;
    RRect r = mkRR(X(d.x), Y(d.y) + ty, w, h, w * .16f, h * .20f);
    if (gold) {
      shadowOut(c, r, 0, PX(4), PX(6), 0, rgbac(150, 95, 10, .4f));
      shadowOut(c, r, 0, PX(2), 0, 0, rgbac(0, 0, 0, .22f));
      shadowOut(c, r, 0, 0, 0, PX(2), hexc(0xffcf5c));
      fillRR(c, r, Grad::three(hexc(0xffd87a), hexc(0xf3b63e), .55f, hexc(0xe0a02a)), r.y0, r.y1);
      shadowIn(c, r, 0, -PX(2.6f), PX(4), 0, rgbac(120, 70, 10, .32f));
      shadowIn(c, r, 0, PX(1.4f), 0, 0, rgbac(255, 255, 255, .75f));
    } else if (pr) {
      shadowOut(c, r, 0, PX(1), 0, 0, rgbac(0, 0, 0, .18f));
      fillRR(c, r, Grad::three(hexc(0xc3b89f), hexc(0xd2c7b1), .55f, hexc(0xe1d6c1)), r.y0, r.y1);
      shadowIn(c, r, 0, PX(2.2f), PX(5), 0, rgbac(60, 50, 30, .42f));
    } else {
      shadowOut(c, r, 0, PX(4), PX(5), 0, rgbac(0, 0, 0, .3f));
      shadowOut(c, r, 0, PX(2), 0, 0, rgbac(0, 0, 0, .22f));
      fillRR(c, r, Grad::three(hexc(0xf4eee1), hexc(0xe3d8c4), .48f, hexc(0xcabea8)), r.y0, r.y1);
      shadowIn(c, r, 0, -PX(2.6f), PX(3.5f), 0, rgbac(95, 82, 58, .28f));
      shadowIn(c, r, 0, PX(1.4f), 0, 0, rgbac(255, 255, 255, .75f));
    }
    // .key .l1{1.25cqw;600;opacity:.68;#453d2e;line-height:1.15}
    // .key .l2{1.6cqw;700;#2b2a28;line-height:1.25} (shiftLatched .l2 #4a3208)
    bool h1 = d.l1 && d.l1[0], h2 = d.l2 && d.l2[0];
    if (!h1 && !h2) return;
    Font &f1 = fSemi(), &f2 = fBold();
    float fs1 = 1.25f * CQW, fs2 = 1.6f * CQW;
    float lh1 = h1 ? fs1 * 1.15f : 0.f, lh2 = h2 ? fs2 * 1.25f : 0.f;
    float top = d.y + (d.h - (lh1 + lh2)) * .5f;
    if (h1) {
      float base = baselineIn(f1, top, lh1, fs1);
      float tw = textW(f1, d.l1, fs1, 0);
      RGBA col = hexc(0x453d2e, .68f);
      drawText(c, f1, X(d.x + (d.w - tw) * .5f), Y(base) + ty + PX(TEXT_DY_KEY), d.l1, L(fs1), 0, col);
    }
    if (h2) {
      float base = baselineIn(f2, top + lh1, lh2, fs2);
      float tw = textW(f2, d.l2, fs2, 0);
      drawText(c, f2, X(d.x + (d.w - tw) * .5f), Y(base) + ty + PX(TEXT_DY_KEY), d.l2, L(fs2), 0, gold ? hexc(0x4a3208) : hexc(0x2b2a28));
    }
  }

  // ----- okenko kazety (dvirka) -----
  // Vnitrek okenka se nakresli do vlastniho maleho platna; zavrene = rovnou
  // na misto, otevrene = pres perspektivu presne podle CSS transformace:
  // .tapewin{transform-origin:50% 0%;transition:transform .32s cubic-bezier(.34,1.45,.64,1)}
  // .tapewin.open{transform:perspective(600px) rotateX(-20deg) translateY(1.5%);
  //  box-shadow:inset 0 .3cqw .5cqw rgba(0,0,0,.6),0 10px 16px rgba(0,0,0,.5)}
  void drawWindow(Canvas &c, long long now) {
    float p = doorProgressEased(now);
    float pShadow;
    {   // stin prechazi s "ease" (.32s ease), ne s pruzinou
      float t = std::min(1.f, std::max(0.f, (float)(now - doorAt) / 320.f));
      float e = cubicBezier(0.25f, 0.1f, 0.25f, 1.f, t);
      pShadow = doorOpen ? e : 1.f - e;
    }
    float wx = X(TW_X), wy = Y(TW_Y), ww = L(TW_W), wh = L(TW_H);
    // offscreen: okenko + rezerva na stin
    int pad = (int)std::ceil(PX(10) + PX(16) * 1.6f + 4);
    int bw = (int)std::ceil(ww) + 2 * pad, bh = (int)std::ceil(wh) + 2 * pad;
    int ox = (int)std::floor(wx) - pad, oy = (int)std::floor(wy) - pad;
    if (p < 0.001f && pShadow < 0.001f) {           // zavreno: kreslit rovnou
      drawWindowContent(c, 0.f, 0.f, 0.f);
      return;
    }
    // vnitrek okna do bufferu (+ samostatne pokryti tvaru okna)
    std::vector<uint32_t> tmp((size_t)bw * bh, 0xFF000000u);
    std::vector<float> cov((size_t)bw * bh, 0.f);
    {
      Canvas o; o.px = tmp.data(); o.w = bw; o.h = bh; o.stride = bw; o.clip = IRect{0, 0, bw, bh};
      // posun souradnic: kreslime s devX/devY posunutymi o -ox,-oy
      float sdx = devX, sdy = devY; devX -= ox; devY -= oy;
      drawWindowContent(o, 0.f, 0.f, -1.f);    // bez vnejsiho stinu (ten resime zvlast)
      devX = sdx; devY = sdy;
      RRect r = mkRR(wx - ox, wy - oy, ww, wh, ww * .014f, wh * .014f);
      for (int y = 0; y < bh; y++) for (int x = 0; x < bw; x++) cov[(size_t)y * bw + x] = covFromSd(sdRR(x + .5f, y + .5f, r));
    }
    // CSS transformace (pocatek = horni stred okenka)
    float persp = 600.f * CSSPX_DU * s;
    float ang = -20.f * p * 3.14159265f / 180.f;
    float tY = 0.015f * wh * p;
    float ca = std::cos(ang), sa = std::sin(ang);
    float o0x = wx + ww * .5f, o0y = wy;
    // zobrazeni bodu okna (lx,ly relativne k pocatku) na displej
    auto fwd = [&](float lx, float ly, float &X2, float &Y2) {
      float u = ly + tY;
      float yy = u * ca, zz = u * sa;
      float wdiv = 1.f - zz / persp;
      X2 = o0x + lx / wdiv; Y2 = o0y + yy / wdiv;
    };
    // obrys transformovaneho okna (pro stin a rozsah)
    float qx[4], qy[4];
    fwd(-ww * .5f, 0, qx[0], qy[0]); fwd(ww * .5f, 0, qx[1], qy[1]);
    fwd(ww * .5f, wh, qx[2], qy[2]); fwd(-ww * .5f, wh, qx[3], qy[3]);
    float minX = std::min(std::min(qx[0], qx[1]), std::min(qx[2], qx[3]));
    float maxX = std::max(std::max(qx[0], qx[1]), std::max(qx[2], qx[3]));
    float minY = std::min(std::min(qy[0], qy[1]), std::min(qy[2], qy[3]));
    float maxY = std::max(std::max(qy[0], qy[1]), std::max(qy[2], qy[3]));
    // vnejsi stin: 0 1px 0 rgba(255,255,255,.12) -> 0 10px 16px rgba(0,0,0,.5)
    {
      RRect sr = mkRR(minX, minY, maxX - minX, maxY - minY, ww * .014f, wh * .014f);
      // lichobeznik ~ obdelnik sirky spodni hrany (stin je mekky, rozdil neni videt)
      float a = .5f * pShadow;
      if (a > 0.003f) shadowOut(c, mkRR(qx[3], minY, qx[2] - qx[3], maxY - minY, ww * .014f, wh * .014f), 0, PX(10) * pShadow, PX(16) * pShadow, 0, rgbac(0, 0, 0, a));
      if (pShadow < 0.999f) shadowOut(c, sr, 0, PX(1), 0, 0, rgbac(255, 255, 255, .12f * (1.f - pShadow)));
    }
    // inverzni mapovani kazdeho cil. px do okna (bilinearne)
    int x0, y0, x1, y1;
    if (!c.span(minX - 1, minY - 1, maxX + 1, maxY + 1, x0, y0, x1, y1)) return;
    for (int y = y0; y < y1; y++) {
      for (int x = x0; x < x1; x++) {
        // 4 vzorky na px kvuli hladkym hranam
        float ar = 0, ag = 0, ab = 0, acv = 0;
        for (int sy = 0; sy < 2; sy++) for (int sx = 0; sx < 2; sx++) {
          float Xs = x + .25f + .5f * sx - o0x, Ys = y + .25f + .5f * sy - o0y;
          float denom = ca + Ys * sa / persp;
          if (std::fabs(denom) < 1e-4f) continue;
          float u = Ys / denom;
          float wdiv = 1.f - u * sa / persp;
          float lx = Xs * wdiv, ly = u - tY;
          float bxf = lx + ww * .5f + (wx - ox) - .5f, byf = ly + (wy - oy) - .5f;
          int ix = (int)std::floor(bxf), iy = (int)std::floor(byf);
          if (ix < 0 || iy < 0 || ix + 1 >= bw || iy + 1 >= bh) continue;
          float fx = bxf - ix, fy = byf - iy;
          float w00 = (1 - fx) * (1 - fy), w10 = fx * (1 - fy), w01 = (1 - fx) * fy, w11 = fx * fy;
          size_t i00 = (size_t)iy * bw + ix;
          float c00 = cov[i00], c10 = cov[i00 + 1], c01 = cov[i00 + bw], c11 = cov[i00 + bw + 1];
          float cv = c00 * w00 + c10 * w10 + c01 * w01 + c11 * w11;
          if (cv <= 0) continue;
          uint32_t p00 = tmp[i00], p10 = tmp[i00 + 1], p01 = tmp[i00 + bw], p11 = tmp[i00 + bw + 1];
          auto ch = [&](int sh) {
            return ((p00 >> sh) & 255) * w00 * c00 + ((p10 >> sh) & 255) * w10 * c10 + ((p01 >> sh) & 255) * w01 * c01 + ((p11 >> sh) & 255) * w11 * c11;
          };
          ar += ch(0); ag += ch(8); ab += ch(16); acv += cv;
        }
        if (acv <= 0) continue;
        blendPx(c.at(x, y), ar / acv, ag / acv, ab / acv, acv / 4.f);
      }
    }
  }
  // Vnitrek okenka. shadowMode: 0 = i vnejsi stin (zavreno), -1 = bez.
  void drawWindowContent(Canvas &c, float, float, float shadowMode) {
    float wx = X(TW_X), wy = Y(TW_Y), ww = L(TW_W), wh = L(TW_H);
    RRect r = mkRR(wx, wy, ww, wh, ww * .014f, wh * .014f);
    if (shadowMode >= 0.f) shadowOut(c, r, 0, PX(1), 0, 0, rgbac(255, 255, 255, .12f));
    // .tapewin{background:linear-gradient(180deg,#332e28 0%,#121110 65%)}
    fillRR(c, r, Grad::three(hexc(0x332e28), hexc(0x121110), .65f, hexc(0x121110)), r.y0, r.y1);
    shadowIn(c, r, 0, L(.3f * CQW), L(.5f * CQW), 0, rgbac(0, 0, 0, .6f));
    // .winline{background:#d4d4d2;opacity:.55} - dve vodici cary vedle napisu
    float twinX2 = TW_X + TW_W * .04f, twinX3 = TW_X + TW_W * .96f, mid = TW_X + TW_W * .5f;
    float alY = TW_Y + TW_H * .10f, alX1 = mid - TW_W * .165f, alX2 = mid + TW_W * .165f;
    fillRR(c, snapRR(twinX2, alY, alX1 - twinX2, 1.4f, 0), Grad::solid(hexc(0xd4d4d2)), 0, 1, .55f);
    fillRR(c, snapRR(alX2, alY, twinX3 - alX2, 1.4f, 0), Grad::solid(hexc(0xd4d4d2)), 0, 1, .55f);
    // .autostop{top:7.3%;center;#d4d4d2;font-size:1.1cqw;letter-spacing:.2em} (Space Mono)
    {
      Font &f = fMono(); float fs = 1.1f * CQW, ls = .2f * fs;
      float lineH = (f.asc + f.desc) * fs;
      float top = TW_Y + TW_H * .073f;
      float base = baselineIn(f, top, lineH, fs);
      float tw = textW(f, "AUTOMATIC STOP", fs, ls);
      drawText(c, f, X(TW_X + (TW_W - tw) * .5f), Y(base), "AUTOMATIC STOP", L(fs), L(ls), hexc(0xd4d4d2));
    }
    // .ribbon - paska mezi civkami
    {
      RRect rb = mkRR(X(398.94f), Y(1346.19f), L(136.78f), L(8.98f), PX(1), PX(1));
      shadowOut(c, rb, 0, 0, 0, PX(1), rgbac(0, 0, 0, .4f));
      Grad g; g.n = 3; g.pos[0] = 0; g.pos[1] = .5f; g.pos[2] = 1; g.col[0] = hexc(0x2c2116); g.col[1] = hexc(0x161008); g.col[2] = hexc(0x2c2116);
      fillRR(c, rb, g, rb.y0, rb.y1);
    }
    drawReel(c, 332.44f, 1317.44f, 66.48f);
    drawReel(c, 535.73f, 1317.44f, 66.48f);
    // .pinch{border-radius:22%;background:#55504a;box-shadow:inset 0 1px 2px rgba(0,0,0,.5)}
    {
      float px0 = TW_X + TW_W * .4204f, py0 = TW_Y + TW_H * .3939f, pw = TW_W * .1716f, ph = TW_H * .2926f;
      RRect pr = mkRR(X(px0), Y(py0), L(pw), L(ph), L(pw) * .22f, L(ph) * .22f);
      fillRR(c, pr, Grad::solid(hexc(0x55504a)), pr.y0, pr.y1);
      shadowIn(c, pr, 0, PX(1), PX(2), 0, rgbac(0, 0, 0, .5f));
      // .pinchhole{inset:28%;border-radius:50%;background:#17140f}
      RRect ph2 = mkRR(X(px0 + pw * .28f), Y(py0 + ph * .28f), L(pw * .44f), L(ph * .44f), L(pw * .22f), L(ph * .22f));
      fillRR(c, ph2, Grad::solid(hexc(0x17140f)), ph2.y0, ph2.y1);
    }
    // .feedarrow{color:#c7c7c5;font-size:1.8cqw;opacity:.85} "->" pod kladkou
    {
      Font &f = fMedium(); float fs = 1.8f * CQW;
      const char *arrow = "\xE2\x86\x92";
      float lineH = (f.asc + f.desc) * fs;
      float top = TW_Y + TW_H * .90f;
      float base = baselineIn(f, top, lineH, fs);
      float tw = textW(f, arrow, fs, 0);
      drawText(c, f, X(TW_X + (TW_W - tw) * .5f), Y(base), arrow, L(fs), 0, hexc(0xc7c7c5, .85f));
    }
  }
  // .reel{border-radius:50%;background:#1f1b15;box-shadow:inset 0 2px 6px rgba(0,0,0,.65),0 1px 1px rgba(255,255,255,.06)}
  // .reel .teeth{inset:9%;background:repeating-conic-gradient(#433c30 0deg 9deg,#231f18 9deg 18deg);
  //  box-shadow:inset 0 0 0 1px rgba(0,0,0,.4)} (.live -> spin .6s)
  // .reel .hub{inset:38%;background:#c9c1af;box-shadow:inset 0 0 0 1.5px rgba(0,0,0,.3)}
  void drawReel(Canvas &c, float x, float y, float d) {
    float D = L(d), rx = X(x), ry = Y(y);
    RRect outer = mkRR(rx, ry, D, D, D * .5f, D * .5f);
    shadowOut(c, outer, 0, PX(1), PX(1), 0, rgbac(255, 255, 255, .06f));
    fillRR(c, outer, Grad::solid(hexc(0x1f1b15)), outer.y0, outer.y1);
    shadowIn(c, outer, 0, PX(2), PX(6), 0, rgbac(0, 0, 0, .65f));
    float cx = rx + D * .5f, cy = ry + D * .5f;
    float rT = D * .41f, rH = D * .12f;
    RGBA lt = hexc(0x433c30), dk = hexc(0x231f18);
    int x0, y0, x1, y1;
    if (c.span(cx - rT - 1, cy - rT - 1, cx + rT + 1, cy + rT + 1, x0, y0, x1, y1)) {
      for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) {
        float dx = xx + .5f - cx, dy = yy + .5f - cy;
        float rr = std::sqrt(dx * dx + dy * dy);
        float cov = covFromSd(rr - rT);
        if (cov <= 0) continue;
        // uhel od 12 hodin po smeru hodin (CSS conic), minus natoceni civky
        float ang = std::atan2(dx, -dy) * 57.2957795f - reelAngle;
        float m = std::fmod(ang, 18.f); if (m < 0) m += 18.f;
        // vyhlazeni hran klinu podle uhlove sirky px
        float aw = 57.2957795f / std::max(1.f, rr) * 0.5f;
        float t;   // podil "svetleho" klinu
        if (m < 9.f) t = std::min(1.f, std::min(m, 9.f - m) / aw * 0.5f + 0.5f);
        else t = 1.f - std::min(1.f, std::min(m - 9.f, 18.f - m) / aw * 0.5f + 0.5f);
        RGBA col{ dk.r + (lt.r - dk.r) * t, dk.g + (lt.g - dk.g) * t, dk.b + (lt.b - dk.b) * t, 1 };
        // inset 0 0 0 1px rgba(0,0,0,.4) - tmavy prstenec 1 CSS px na okraji zubu
        float edge = .4f * covFromSd((rT - PX(1)) - rr + 0.f) * 0.f + .4f * std::min(1.f, std::max(0.f, rr - (rT - PX(1)) + .5f));
        col.r *= (1 - edge); col.g *= (1 - edge); col.b *= (1 - edge);
        blendPx(c.at(xx, yy), col.r, col.g, col.b, cov);
      }
    }
    RRect hub = mkRR(cx - rH, cy - rH, 2 * rH, 2 * rH, rH, rH);
    fillRR(c, hub, Grad::solid(hexc(0xc9c1af)), hub.y0, hub.y1);
    shadowIn(c, hub, 0, 0, 0, PX(1.5f), rgbac(0, 0, 0, .3f));
  }

  // ----- pocitadlo + nulovaci spinac -----
  void drawCounter(Canvas &c) {
    // .counter{border-radius:6%;background:#0a0908;color:#d9d9d9;font-size:1.4cqw;letter-spacing:.08em;
    //  box-shadow:0 2px 4px rgba(0,0,0,.4),inset 0 1px 3px rgba(0,0,0,.6)} pb(748,1328,99,48) Space Mono
    float w = L(99), h = L(48);
    RRect r = mkRR(X(748), Y(1328), w, h, w * .06f, h * .06f);
    shadowOut(c, r, 0, PX(2), PX(4), 0, rgbac(0, 0, 0, .4f));
    fillRR(c, r, Grad::solid(hexc(0x0a0908)), r.y0, r.y1);
    shadowIn(c, r, 0, PX(1), PX(3), 0, rgbac(0, 0, 0, .6f));
    char txt[16];
    int whole = counter / 10, dec = counter % 10;
    std::snprintf(txt, sizeof(txt), "%03d.%d", whole % 1000, dec);
    Font &f = fMono(); float fs = 1.4f * CQW, ls = .08f * fs;
    float lineH = (f.asc + f.desc) * fs;
    float top = 1328 + (48 - lineH) * .5f;
    float base = baselineIn(f, top, lineH, fs);
    float tw = textW(f, txt, fs, ls);
    drawText(c, f, X(748 + (99 - tw) * .5f), Y(base), txt, L(fs), L(ls), hexc(0xd9d9d9));
  }
  void drawResetSwitch(Canvas &c, long long now) {
    // .tswitch{border-radius:16%;background:linear-gradient(180deg,#c3bdaf,#a9a294);
    //  box-shadow:0 2px 4px rgba(0,0,0,.35)} :active{transform:scale(.93)} pb(855,1328,37,48)
    // .tswitch-i{left:12%;top:8%;width:76%;height:38%;border-radius:22%;background:#fdfdfc}
    bool pr = swHeld || within(now, swAt);
    float k = pr ? .93f : 1.f;
    float cx = X(855 + 18.5f), cy = Y(1328 + 24);
    float w = L(37) * k, h = L(48) * k;
    RRect r = mkRR(cx - w * .5f, cy - h * .5f, w, h, w * .16f, h * .16f);
    shadowOut(c, r, 0, PX(2) * k, PX(4) * k, 0, rgbac(0, 0, 0, .35f));
    fillRR(c, r, Grad::two(hexc(0xc3bdaf), hexc(0xa9a294)), r.y0, r.y1);
    float iw = w * .76f, ih = h * .38f;
    RRect ri = mkRR(r.x0 + w * .12f, r.y0 + h * .08f, iw, ih, iw * .22f, ih * .22f);
    fillRR(c, ri, Grad::solid(hexc(0xfdfdfc)), ri.y0, ri.y1);
  }

  // ----- tlacitka kazety -----
  void drawTapeButton(Canvas &c, int i, long long now) {
    // .tbtn{border-radius:16%;background:linear-gradient(180deg,#ece7db 0%,#cbc5b7 55%,#b2aa98 100%);
    //  box-shadow:inset 0 1.3px 0 rgba(255,255,255,.7),inset 0 -2.2px 3px rgba(70,62,44,.25),
    //  0 2px 0 rgba(0,0,0,.2),0 3.5px 4px rgba(0,0,0,.28)}
    // .tbtn.pressed{linear-gradient(180deg,#8f8777 0%,#7d765f 65%,#6c6552 100%);
    //  box-shadow:inset 0 2px 5px rgba(20,18,10,.5);transform:scale(.92)}
    // .tbtn.play.pressed{linear-gradient(180deg,#5f8f55 0%,#4c7943 100%)}
    // ikony: font-size:2cqw; #3a3630 (REC #ad2f2f); stisknute #17140e (REC #6e2020, PLAY #122b0e)
    bool latched = (i == 0 && rec) || (i == 2 && play);
    bool pr = latched || (power && (tapeHeld[i] || within(now, tapeAt[i])));
    float bx = TAPE_B[i], bw = TAPE_B[i + 1] - TAPE_B[i];
    float k = pr ? .92f : 1.f;
    float cx = X(bx + bw * .5f), cy = Y(TAPE_Y + TAPE_H * .5f);
    float w = L(bw) * k, h = L(TAPE_H) * k;
    RRect r = mkRR(cx - w * .5f, cy - h * .5f, w, h, w * .16f, h * .16f);
    if (pr) {
      Grad g = (i == 2) ? Grad::two(hexc(0x5f8f55), hexc(0x4c7943))
                        : Grad::three(hexc(0x8f8777), hexc(0x7d765f), .65f, hexc(0x6c6552));
      fillRR(c, r, g, r.y0, r.y1);
      shadowIn(c, r, 0, PX(2) * k, PX(5) * k, 0, rgbac(20, 18, 10, .5f));
    } else {
      shadowOut(c, r, 0, PX(3.5f), PX(4), 0, rgbac(0, 0, 0, .28f));
      shadowOut(c, r, 0, PX(2), 0, 0, rgbac(0, 0, 0, .2f));
      fillRR(c, r, Grad::three(hexc(0xece7db), hexc(0xcbc5b7), .55f, hexc(0xb2aa98)), r.y0, r.y1);
      shadowIn(c, r, 0, -PX(2.2f), PX(3), 0, rgbac(70, 62, 44, .25f));
      shadowIn(c, r, 0, PX(1.3f), 0, 0, rgbac(255, 255, 255, .7f));
    }
    RGBA ic = hexc(0x3a3630);
    if (i == 0) ic = pr ? hexc(0x6e2020) : hexc(0xad2f2f);
    else if (i == 2 && pr) ic = hexc(0x122b0e);
    else if (pr) ic = hexc(0x17140e);
    drawTapeIcon(c, i, cx, cy, L(2.f * CQW) * k, ic);
  }
  void drawTapeIcon(Canvas &c, int i, float cx, float cy, float fs, RGBA col) {
    // ● (REC) neni v Chakra Petch -> kruh; ostatni glyfy primo z pisma (Medium)
    if (i == 0) {
      float rr = fs * 0.39f;        // ● ve zalozni sade prohlizece ~0.78 em
      RRect r = mkRR(cx - rr, cy - rr, 2 * rr, 2 * rr, rr, rr);
      fillRR(c, r, Grad::solid(col), r.y0, r.y1);
      return;
    }
    static const char *ICON[6] = { "", "\xE2\x97\x80\xE2\x97\x80", "\xE2\x96\xB6", "\xE2\x96\xB6\xE2\x96\xB6", "\xE2\x96\xA0", "\xE2\x96\xB2" };
    Font &f = fMedium();
    float tw = textW(f, ICON[i], fs, 0);
    // svisle na stred podle skutecneho obrysu glyfu
    int gx0, gy0, gx1, gy1;
    const char *p = ICON[i]; int cp = utf8Next(p);
    float sc = stbtt_ScaleForMappingEmToPixels(&f.info, fs);
    stbtt_GetCodepointBitmapBox(&f.info, cp, sc, sc, &gx0, &gy0, &gx1, &gy1);
    float base = cy - (gy0 + gy1) * .5f;
    drawText(c, f, cx - tw * .5f, base, ICON[i], fs, 0, col);
  }

  // ----- servisni panel + tlacitka -----
  void drawSvcPanel(Canvas &c) {
    // .svcpanel{border-radius:10%/60%;background:linear-gradient(180deg,#cbc9c8 0%,#aeaba8 100%);
    //  box-shadow:0 .3cqw .5cqw rgba(0,0,0,.3),inset 0 1px 0 rgba(255,255,255,.2)} pb(25,1523,890,106)
    float w = L(890), h = L(106);
    RRect r = mkRR(X(25), Y(1523), w, h, w * .10f, h * .60f);
    shadowOut(c, r, 0, L(.3f * CQW), L(.5f * CQW), 0, rgbac(0, 0, 0, .3f));
    fillRR(c, r, Grad::two(hexc(0xcbc9c8), hexc(0xaeaba8)), r.y0, r.y1);
    shadowIn(c, r, 0, PX(1), 0, 0, rgbac(255, 255, 255, .2f));
  }
  void drawSvcButton(Canvas &c, int i, long long now) {
    // .svcbtn{border-radius:10%;background:linear-gradient(180deg,#eeece7 0%,#cfccc6 55%,#b3afa6 100%);
    //  box-shadow:inset 0 1.3px 0 rgba(255,255,255,.7),inset 0 -2px 3px rgba(60,55,45,.22),
    //  0 1.6px 0 rgba(0,0,0,.2),0 3px 4px rgba(0,0,0,.26);flex column;center;gap:2%}
    // .svcbtn .led{width:26%;height:11%;border-radius:2px} (+ box-shadow:0 0 4px <barva>)
    // .l1/.l2{1.15cqw;700;#2b2a28} .l1.small{.92cqw}
    // .svcbtn.pressed{linear-gradient(180deg,#a8a398 0%,#938e82 60%,#827d70 100%);
    //  box-shadow:inset 0 2px 4px rgba(30,27,18,.45);transform:translateY(2px)} text #17160f
    const SvcDef &d = SVC[i];
    bool pr = power && (svcHeld[i] || within(now, svcAt[i]));
    float bx = SVC_X0 + i * (SVC_W + SVC_GAP);
    float ty = pr ? PX(2) : 0.f;
    float w = L(SVC_W), h = L(SVC_H);
    RRect r = mkRR(X(bx), Y(SVC_Y) + ty, w, h, w * .10f, h * .10f);
    if (pr) {
      fillRR(c, r, Grad::three(hexc(0xa8a398), hexc(0x938e82), .60f, hexc(0x827d70)), r.y0, r.y1);
      shadowIn(c, r, 0, PX(2), PX(4), 0, rgbac(30, 27, 18, .45f));
    } else {
      shadowOut(c, r, 0, PX(3), PX(4), 0, rgbac(0, 0, 0, .26f));
      shadowOut(c, r, 0, PX(1.6f), 0, 0, rgbac(0, 0, 0, .2f));
      fillRR(c, r, Grad::three(hexc(0xeeece7), hexc(0xcfccc6), .55f, hexc(0xb3afa6)), r.y0, r.y1);
      shadowIn(c, r, 0, -PX(2), PX(3), 0, rgbac(60, 55, 45, .22f));
      shadowIn(c, r, 0, PX(1.3f), 0, 0, rgbac(255, 255, 255, .7f));
    }
    Font &f = fBold();
    float fs = 1.15f * CQW, fsS = .92f * CQW;
    float fs1 = d.small ? fsS : fs;
    bool h1 = d.l1 && d.l1[0];
    float ledH = SVC_H * .11f, gap = SVC_H * .02f;
    float lh1 = h1 ? (f.asc + f.desc) * fs1 : 0.f, lh2 = (f.asc + f.desc) * fs;
    float total = ledH + gap + lh1 + gap + lh2;
    float top = SVC_Y + (SVC_H - total) * .5f;
    // LED se zarem
    float lw = SVC_W * .26f;
    RRect led = mkRR(X(bx + (SVC_W - lw) * .5f), Y(top) + ty, L(lw), L(ledH), PX(2), PX(2));
    shadowOut(c, led, 0, 0, PX(4), 0, hexc(d.led));
    fillRR(c, led, Grad::solid(hexc(d.led)), led.y0, led.y1);
    RGBA tc = pr ? hexc(0x17160f) : hexc(0x2b2a28);
    float y1 = top + ledH + gap;
    if (h1) {
      float base = baselineIn(f, y1, lh1, fs1);
      float tw = textW(f, d.l1, fs1, 0);
      drawText(c, f, X(bx + (SVC_W - tw) * .5f), Y(base) + ty + PX(TEXT_DY_SVC), d.l1, L(fs1), 0, tc);
    }
    float y2 = y1 + lh1 + gap;
    float base2 = baselineIn(f, y2, lh2, fs);
    float tw2 = textW(f, d.l2, fs, 0);
    drawText(c, f, X(bx + (SVC_W - tw2) * .5f), Y(base2) + ty + PX(TEXT_DY_SVC), d.l2, L(fs), 0, tc);
  }

  void drawRainbow(Canvas &c) {
    // .rbow: {x0,w313,#bf4310},{313,315,#0a7183},{628,313,#06308e} y1649 h18 (orez rohy skrine)
    struct S { float x, w; uint32_t col; } st[3] = { {0, 313, 0xbf4310}, {313, 315, 0x0a7183}, {628, 313, 0x06308e} };
    RRect dev = deviceRR();
    for (auto &q : st) {
      int x0, y0, x1, y1;
      if (!c.span(X(q.x), Y(1649), X(q.x + q.w), Y(1667), x0, y0, x1, y1)) continue;
      RGBA col = hexc(q.col);
      float fx0 = X(q.x), fx1 = X(q.x + q.w), fy0 = Y(1649), fy1 = Y(1667);
      for (int y = y0; y < y1; y++) for (int x = x0; x < x1; x++) {
        float covx = std::min((float)x + 1, fx1) - std::max((float)x, fx0);
        float covy = std::min((float)y + 1, fy1) - std::max((float)y, fy0);
        float cov = std::max(0.f, std::min(1.f, covx)) * std::max(0.f, std::min(1.f, covy));
        cov *= covFromSd(sdRR(x + .5f, y + .5f, dev));
        if (cov > 0) blendPx(c.at(x, y), col.r, col.g, col.b, cov);
      }
    }
  }

  // ----- POWER (vzdy navrchu, i nad tmavym prekryvem) -----
  void drawPowerSwitch(Canvas &c, long long now) {
    // .pwrsw{border-radius:30%/16%;background:linear-gradient(180deg,#34373f 0%,#1d1f24 100%);
    //  box-shadow:inset 0 1px 2px rgba(0,0,0,.6),inset 0 -1px 0 rgba(255,255,255,.06),0 2px 4px rgba(0,0,0,.45)}
    // .nub{left:14%;right:14%;height:38%;top:6%;border-radius:40%;
    //  background:radial-gradient(circle at 35% 30%,#8fe08a,#2c8a28);
    //  box-shadow:0 0 5px rgba(100,230,100,.8),inset 0 1px 1px rgba(255,255,255,.45)}
    // .off .nub{top:56%;radial-gradient(circle at 35% 30%,#6a6a6a,#2b2b2b);inset 0 1px 1px rgba(255,255,255,.12)}
    // transition: top/background/box-shadow .15s ease
    float w = L(34), h = L(50);
    RRect r = mkRR(X(8), Y(393), w, h, w * .30f, h * .16f);
    shadowOut(c, r, 0, PX(2), PX(4), 0, rgbac(0, 0, 0, .45f));
    fillRR(c, r, Grad::two(hexc(0x34373f), hexc(0x1d1f24)), r.y0, r.y1);
    shadowIn(c, r, 0, -PX(1), 0, 0, rgbac(255, 255, 255, .06f));
    shadowIn(c, r, 0, PX(1), PX(2), 0, rgbac(0, 0, 0, .6f));
    float on = nubOn(now);                             // 1 = zapnuto
    float topPct = .56f + (.06f - .56f) * on;
    float nx = r.x0 + w * .14f, nw = w * .72f, nh = h * .38f, ny = r.y0 + h * topPct;
    RRect nub = mkRR(nx, ny, nw, nh, nw * .40f, nh * .40f);
    if (on > 0.01f) shadowOut(c, nub, 0, 0, PX(5), 0, rgbac(100, 230, 100, .8f * on));
    // radialni gradient (circle at 35% 30%, farthest-corner)
    float gx = nx + nw * .35f, gy = ny + nh * .30f;
    float fr = std::sqrt(std::max(nw * .35f, nw * .65f) * std::max(nw * .35f, nw * .65f) + std::max(nh * .3f, nh * .7f) * std::max(nh * .3f, nh * .7f));
    RGBA a0 = hexc(0x8fe08a), a1 = hexc(0x2c8a28), b0 = hexc(0x6a6a6a), b1 = hexc(0x2b2b2b);
    int x0, y0, x1, y1;
    if (c.span(nub.x0, nub.y0, nub.x1, nub.y1, x0, y0, x1, y1)) {
      for (int yy = y0; yy < y1; yy++) for (int xx = x0; xx < x1; xx++) {
        float cov = covFromSd(sdRR(xx + .5f, yy + .5f, nub)); if (cov <= 0) continue;
        float dd = std::sqrt((xx + .5f - gx) * (xx + .5f - gx) + (yy + .5f - gy) * (yy + .5f - gy)) / fr;
        dd = std::min(1.f, dd);
        float rr = (a0.r + (a1.r - a0.r) * dd) * on + (b0.r + (b1.r - b0.r) * dd) * (1 - on);
        float gg = (a0.g + (a1.g - a0.g) * dd) * on + (b0.g + (b1.g - b0.g) * dd) * (1 - on);
        float bb = (a0.b + (a1.b - a0.b) * dd) * on + (b0.b + (b1.b - b0.b) * dd) * (1 - on);
        blendPx(c.at(xx, yy), rr, gg, bb, cov);
      }
    }
    shadowIn(c, nub, 0, PX(1), PX(1), 0, rgbac(255, 255, 255, .45f * on + .12f * (1 - on)));
  }

  // ----- stavovy radek pod pristrojem (.legend) -----
  // .legend{display:flex;gap:9px;margin:14px 2px 0;font:600 12px Chakra Petch;
  //  letter-spacing:.03em;color:#c3c9d1} .led2{9px kruh + box-shadow:0 0 7px <barva>}
  // navrh: VYPNUTO #3a3d46 / NAHRAVA — CSAVE #e3453f / PREHRAVA — CLOAD #4f9a3a / ZASTAVENO #5a5f6c
  void legendText(char *buf, size_t n, uint32_t &led) const {
    if (!power) { std::snprintf(buf, n, "VYPNUTO"); led = 0x3a3d46; return; }
    if (extraMsg[0]) { std::snprintf(buf, n, "%s", extraMsg); led = 0x4f9a3a; return; }
    if (play && rec) { std::snprintf(buf, n, "NAHRAVA \xE2\x80\x94 CSAVE"); led = 0xe3453f; return; }
    if (play) { std::snprintf(buf, n, "PREHRAVA \xE2\x80\x94 CLOAD"); led = 0x4f9a3a; return; }
    if (motorOn) { std::snprintf(buf, n, "MOTOR KAZETY BEZI"); led = 0xe3453f; return; }
    std::snprintf(buf, n, "ZASTAVENO"); led = 0x5a5f6c;
  }
  void drawLegend(Canvas &c, long long) {
    float topY = Y(DH) + PX(14);
    if (topY + PX(16) > H) return;
    char txt[128]; uint32_t led;
    legendText(txt, sizeof(txt), led);
    float x = X(0) + PX(2);
    float d = PX(9);
    Font &f = fSemi(); float fs = PX(12), ls = .03f * fs;
    float lineH = (f.asc + f.desc) * fs;
    float cy = topY + lineH * .5f;
    RRect dot = mkRR(x, cy - d * .5f, d, d, d * .5f, d * .5f);
    shadowOut(c, dot, 0, 0, PX(7), 0, hexc(led));
    fillRR(c, dot, Grad::solid(hexc(led)), dot.y0, dot.y1);
    float base = baselineIn(f, topY, lineH, fs);
    drawText(c, f, x + d + PX(9), base, txt, fs, ls, hexc(0xc3c9d1));
  }
};

} // namespace dev
} // namespace nap
