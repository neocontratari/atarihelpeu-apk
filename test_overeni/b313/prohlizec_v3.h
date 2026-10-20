// test_overeni/b313/prohlizec_v3.h
// B313: "prohlizec" v C++ pro testy proudu na TV - pakety verze 3 (JPEG po castech)
// rozebere PRESNE jako stranka v appce (apPaket23 / apObraz3 v MainActivity: hlavicka
// 28 B, cast x y w h kvalita delka JPEG, zvuk), JPEG rozbali stb_image a nakresli na
// jeho misto do "canvasu". Pred include: STB_IMAGE_IMPLEMENTATION + stb_image.h.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

struct Prohlizec {
  int W = 0, H = 0;
  std::vector<uint32_t> canvas;
  long long paketu = 0, snimku = 0, casti = 0, dalsich = 0, chyb = 0, zvukVDalsi = 0, spatnyPriznak = 0;
  std::vector<int> kvalityPoslednihoSnimku;
  int castiPoslednihoSnimku = 0;
  struct Cast { int x, y, w, h, kv; size_t bajtu; };
  std::vector<Cast> castiSnimku;            // casti posledniho snimku (pro vypis)

  static unsigned u16(const uint8_t *p) { return p[0] | (p[1] << 8); }
  static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

  // pk = 4 B delka + paket, mozna nekolik za sebou (jak Java posle jednim zapisem)
  void prijmi(const std::vector<uint8_t> &pk) {
    size_t o = 0;
    bool prvni = true;
    while (o + 4 <= pk.size()) {
      const uint32_t len = u32(&pk[o]);
      if (o + 4 + len > pk.size()) { chyb++; return; }
      paket(&pk[o + 4], len, prvni);
      prvni = false;
      o += 4 + len;
    }
    if (o != pk.size()) chyb++;
  }
  void paket(const uint8_t *p, size_t n, bool prvni) {
    paketu++;
    if (n < 28 || p[0] != 'N' || p[1] != 'A' || p[2] != 3) { chyb++; return; }
    const unsigned pr = p[3];
    const int w = (int)u16(p + 4), h = (int)u16(p + 6);
    if (w != W || h != H) { W = w; H = h; canvas.assign((size_t)W * H, 0); }
    if (prvni) { snimku++; castiPoslednihoSnimku = 0; kvalityPoslednihoSnimku.clear(); castiSnimku.clear(); }
    if (prvni == ((pr & 8) != 0)) spatnyPriznak++;      // prvni cast bez priznaku 8, dalsi s nim
    size_t o = 28;
    // cast v3: x y w h kvalita delka JPEG
    if (o + 13 > n) { chyb++; return; }
    const int x = (int)u16(p + o), y = (int)u16(p + o + 2), rw = (int)u16(p + o + 4), rh = (int)u16(p + o + 6);
    const int kv = p[o + 8];
    const uint32_t dl = u32(p + o + 9);
    o += 13;
    if (o + dl > n) { chyb++; return; }
    if (rw > 0 && dl > 0) {
      int jw = 0, jh = 0, jc = 0;
      unsigned char *img = stbi_load_from_memory(p + o, (int)dl, &jw, &jh, &jc, 3);
      if (!img || jw != rw || jh != rh || x + rw > W || y + rh > H) { chyb++; if (img) stbi_image_free(img); return; }
      for (int yy = 0; yy < rh; yy++)
        for (int xx = 0; xx < rw; xx++) {
          const unsigned char *s = img + ((size_t)yy * rw + xx) * 3;
          canvas[(size_t)(y + yy) * W + x + xx] = ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2];
        }
      stbi_image_free(img);
      casti++;
      castiPoslednihoSnimku++;
      castiSnimku.push_back({x, y, rw, rh, kv, (size_t)dl});
      kvalityPoslednihoSnimku.push_back(kv);
      if (!prvni) dalsich++;
    }
    o += dl;
    if (o + 3 > n) { chyb++; return; }
    const unsigned an = u16(p + o);
    if (!prvni && an != 0) zvukVDalsi++;
    o += 3 + (size_t)an * (p[o + 2] == 2 ? 4 : 2);
    if (o != n) chyb++;
  }
};

// PSNR obdelniku (x0,y0)-(x1,y1) mezi snimkem a canvasem
static double psnr(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b, int W, int x0, int y0, int x1, int y1) {
  double se = 0;
  long long n = 0;
  for (int y = y0; y < y1; y++)
    for (int x = x0; x < x1; x++) {
      const uint32_t p = a[(size_t)y * W + x], q = b[(size_t)y * W + x];
      for (int s = 0; s < 24; s += 8) {
        const double d = (double)((p >> s) & 255) - (double)((q >> s) & 255);
        se += d * d;
      }
      n += 3;
    }
  if (n == 0) return 99;
  const double mse = se / n;
  return mse <= 1e-9 ? 99.0 : 10.0 * std::log10(255.0 * 255.0 / mse);
}
