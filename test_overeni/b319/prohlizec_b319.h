// test_overeni/b319/prohlizec_b319.h
// B319: "prohlizec" v C++ pro testy proudu PS1 na TV / PC - pakety verze 3 rozebere PRESNE jako
// stranka WEB TV B319 (apPaket23 / apObraz3 / apJdalsi v MainActivity): hlavicka 28 B, cast
// x y w h kvalita delka data, zvuk. Nove v B319:
//   kvalita 0xFF  = KOPIE: obdelnik w x h z canvasu z (zdroj x, zdroj y) na (x, y) - stejne jako
//                   stranka: pres pomocny canvas (prekryv zdroje a cile nevadi)
//   kvalita bit7  = JPEG v polovicnim rozliseni (w/2 x h/2), nakreslit 2x bez vyhlazeni
// JPEG rozbali stb_image. Pred include: STB_IMAGE_IMPLEMENTATION + stb_image.h.
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

struct ProhlizecB319 {
  int W = 0, H = 0;
  std::vector<uint32_t> canvas, pomocny;
  long long paketu = 0, snimku = 0, casti = 0, kopii = 0, polovicnich = 0, dalsich = 0;
  long long chyb = 0, zvukVDalsi = 0, spatnyPriznak = 0, bajtu = 0, bajtuJpeg = 0, kopiiBodu = 0;
  int castiPoslednihoSnimku = 0, kopiiPoslednihoSnimku = 0, polovicnichPoslednihoSnimku = 0;

  static unsigned u16(const uint8_t *p) { return p[0] | (p[1] << 8); }
  static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

  // pk = 4 B delka + paket, mozna nekolik za sebou (jak Java posle jednim zapisem)
  void prijmi(const std::vector<uint8_t> &pk) {
    size_t o = 0;
    bool prvni = true;
    bajtu += (long long)pk.size();
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
    if (prvni) { snimku++; castiPoslednihoSnimku = 0; kopiiPoslednihoSnimku = 0; polovicnichPoslednihoSnimku = 0; }
    if (prvni == ((pr & 8) != 0)) spatnyPriznak++;      // prvni cast bez priznaku 8, dalsi s nim
    size_t o = 28;
    if (o + 13 > n) { chyb++; return; }
    const int x = (int)u16(p + o), y = (int)u16(p + o + 2), rw = (int)u16(p + o + 4), rh = (int)u16(p + o + 6);
    const int kv = p[o + 8];
    const uint32_t dl = u32(p + o + 9);
    o += 13;
    if (o + dl > n) { chyb++; return; }
    if (kv == 0xFF) {
      // KOPIE (B319)
      if (dl != 4) { chyb++; return; }
      const int sx = (int)u16(p + o), sy = (int)u16(p + o + 2);
      if (rw <= 0 || rh <= 0 || x + rw > W || y + rh > H || sx + rw > W || sy + rh > H) { chyb++; return; }
      pomocny.resize((size_t)rw * rh);
      for (int yy = 0; yy < rh; yy++) std::memcpy(&pomocny[(size_t)yy * rw], &canvas[(size_t)(sy + yy) * W + sx], (size_t)rw * 4);
      for (int yy = 0; yy < rh; yy++) std::memcpy(&canvas[(size_t)(y + yy) * W + x], &pomocny[(size_t)yy * rw], (size_t)rw * 4);
      kopii++; kopiiPoslednihoSnimku++; kopiiBodu += (long long)rw * rh;
    } else if (rw > 0 && dl > 0) {
      const bool pol = (kv & 0x80) != 0;
      int jw = 0, jh = 0, jc = 0;
      unsigned char *img = stbi_load_from_memory(p + o, (int)dl, &jw, &jh, &jc, 3);
      const int ew = pol ? rw / 2 : rw, eh = pol ? rh / 2 : rh;
      if (!img || jw != ew || jh != eh || x + rw > W || y + rh > H || (pol && ((rw | rh) & 1))) { chyb++; if (img) stbi_image_free(img); return; }
      for (int yy = 0; yy < rh; yy++)
        for (int xx = 0; xx < rw; xx++) {
          const unsigned char *s = pol ? img + ((size_t)(yy / 2) * jw + xx / 2) * 3 : img + ((size_t)yy * rw + xx) * 3;
          canvas[(size_t)(y + yy) * W + x + xx] = ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2];
        }
      stbi_image_free(img);
      casti++; castiPoslednihoSnimku++; bajtuJpeg += dl;
      if (pol) { polovicnich++; polovicnichPoslednihoSnimku++; }
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
static double psnrB(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b, int W, int x0, int y0, int x1, int y1) {
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

// nejhorsi ctverec 16x16: prumerna absolutni chyba (spatna kopie = velka chyba v celych ctvercich,
// JPEG jen par jednotek)
static double nejhorsiCtverec(const std::vector<uint32_t> &a, const std::vector<uint32_t> &b, int W, int H, int *bxOut = nullptr, int *byOut = nullptr) {
  double nej = 0;
  for (int by = 0; by < (H + 15) / 16; by++)
    for (int bx = 0; bx < (W + 15) / 16; bx++) {
      double s = 0;
      int n = 0;
      for (int y = by * 16; y < std::min(H, by * 16 + 16); y++)
        for (int x = bx * 16; x < std::min(W, bx * 16 + 16); x++) {
          const uint32_t p = a[(size_t)y * W + x], q = b[(size_t)y * W + x];
          for (int k = 0; k < 24; k += 8) s += std::fabs((double)((p >> k) & 255) - (double)((q >> k) & 255));
          n += 3;
        }
      if (n && s / n > nej) { nej = s / n; if (bxOut) *bxOut = bx; if (byOut) *byOut = by; }
    }
  return nej;
}
