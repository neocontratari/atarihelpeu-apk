// nap_tv_proud_jpeg.h
// B305: PS1 NA TV / PC CELA V C++ - KODEK "JPEG PO CASTECH" (verze paketu 3).
//
// PlayStation kresli 3D s tisici barev - bez ztraty by to bylo 10x vic dat,
// nez Wi-Fi do TV unese. Proto JPEG, ale:
//  - v puvodnim rozliseni hry (320x240, 512x240, 640x480 ...), ne zvetseny na
//    1280x720 jako driv v Jave (B301: 27-33 ms na snimek na hlavnim vlakne),
//    zvetsi az prohlizec,
//  - jen ta cast obrazu, ktera se zmenila (ctverce 16x16, obdelnik kolem nich),
//    stejny snimek (hra na 30 snimku/s, menu) = nic,
//  - kdyz obraz chvili stoji (menu, text), posle se jeste jednou ve vysoke
//    kvalite (94, barvy bez podvzorkovani) - text je pak ostry,
//  - kvalita se ridi siti: kdyz Java nestiha odesilat (zapis do site dlouho
//    ceka), kvalita klesne (mensi snimky), kdyz je sit volna, zase stoupne.
// Koduje se ve vlakne prohlizece (ne v emulaci), kazdy prohlizec zvlast;
// velky obraz (512x480, 640x480) na 2 pasy soubezne (nap_tv_proud_jpegenc.h).
//
// CAST OBRAZU V PAKETU (verze 3):
//   u16 x, u16 y, u16 w, u16 h   obdelnik, ktery se meni (w = 0: beze zmeny)
//   u8  kvalita
//   u32 delka JPEG, pak JPEG (baseline, ctverce zarovnane na 16)
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "nap_tv_proud_spol.h"
#include "nap_tv_proud_jpegenc.h"

namespace nap {
namespace proud {

struct KodekJpeg {
  static constexpr int VERZE = 3;
  static const char *nazev() { return "JPEG po castech"; }
  static constexpr int Q_KLID = 94;          // zjemneni stojiciho obrazu
  static constexpr int Q_MIN = 45;
  static constexpr int KLID_SNIMKU = 12;     // ~0,2-0,4 s beze zmeny -> zjemnit

  int W = 0, H = 0, bw = 0, bh = 0;
  bool ma = false;
  std::vector<uint32_t> pred;            // posledni poslany snimek (puvodni body)
  std::vector<uint8_t> kval;             // kvalita, ve ktere ma prohlizec ctverec 16x16
  std::vector<uint8_t> zmena;
  int klid = 0;
  int q = 0, qMax = 0;
  double zapis = 0;                      // prumerna doba zapisu do site (ms)
  long long qMs = 0;
  JpegEnc enc;
  std::vector<uint8_t> jpg;
  int casti = 1;                         // pasu soubezne (podle velikosti obdelniku)
  // statistika (do logu)
  long long klicovych = 0, snimku = 0, beze = 0, zjemneni = 0, snizeni = 0, castecnych = 0;
  double kodMsSoucet = 0, kodMsMax = 0;
  long long kodN = 0;

  void zapomen() { ma = false; }
  static int maxKvalita(int w, int h) {
    const size_t n = (size_t)w * h;
    return n <= 110000 ? 88 : n <= 200000 ? 82 : 76;
  }
  std::string info() const {
    char b[240];
    snprintf(b, sizeof(b), "JPEG kvalita %d (max %d, snizeno %lldx), kodovani prumer %.1f ms max %.1f ms (pasu %d), castecnych %lld, zjemneni %lld, beze zmeny %lld z %lld",
             q, qMax, snizeni, kodN ? kodMsSoucet / kodN : 0.0, kodMsMax, casti, castecnych, zjemneni, beze, snimku);
    return b;
  }

  unsigned zakoduj(const uint32_t *F, int w, int h, int zapisMs, long long ted, std::vector<uint8_t> &out) {
    snimku++;
    const size_t n = (size_t)w * h;
    const bool klic = !ma || w != W || h != H || pred.size() != n;
    if (klic) {
      W = w; H = h; bw = (W + 15) / 16; bh = (H + 15) / 16;
      kval.assign((size_t)bw * bh, 0);
      qMax = maxKvalita(W, H);
      if (q <= 0 || q > qMax) q = qMax;
      klicovych++;
    }
    // kvalita podle site: Java hlasi, jak dlouho cekal zapis predchoziho paketu
    zapis = zapis * 0.8 + std::max(0, zapisMs) * 0.2;
    if (ted - qMs >= 600) {
      if (zapis > 12.0 && q > Q_MIN) { q = std::max(Q_MIN, q - 6); qMs = ted; snizeni++; }
      else if (zapis < 3.0 && q < qMax && ted - qMs >= 1500) { q = std::min(qMax, q + 4); qMs = ted; }
    }
    int x0 = 0, y0 = 0, x1 = W, y1 = H, kvalita = q;
    if (!klic) {
      zmena.assign((size_t)bw * bh, 0);
      bool neco = false;
      for (int y = 0; y < H; y++) {
        const uint32_t *a = F + (size_t)y * W, *b = &pred[(size_t)y * W];
        if (std::memcmp(a, b, (size_t)W * 4) == 0) continue;
        uint8_t *zr = &zmena[(size_t)(y / 16) * bw];
        for (int bx = 0; bx < bw; bx++) {
          if (zr[bx]) continue;
          const int xa = bx * 16, nx = std::min(16, W - xa);
          if (std::memcmp(a + xa, b + xa, (size_t)nx * 4)) { zr[bx] = 1; neco = true; }
        }
      }
      bool zjemnit = false;
      if (!neco) {
        if (++klid < KLID_SNIMKU) { beze++; p16(out, 0); p16(out, 0); p16(out, 0); p16(out, 0); p8(out, 0); p32(out, 0); return 4u; }
        for (size_t i = 0; i < kval.size(); i++) if (kval[i] < Q_KLID) { zmena[i] = 1; zjemnit = true; }
        if (!zjemnit) { beze++; p16(out, 0); p16(out, 0); p16(out, 0); p16(out, 0); p8(out, 0); p32(out, 0); return 4u; }
        kvalita = Q_KLID;
        zjemneni++;
      } else {
        klid = 0;
      }
      int bx0 = bw, by0 = bh, bx1 = -1, by1 = -1;
      for (int by = 0; by < bh; by++)
        for (int bx = 0; bx < bw; bx++)
          if (zmena[(size_t)by * bw + bx]) {
            bx0 = std::min(bx0, bx); bx1 = std::max(bx1, bx);
            by0 = std::min(by0, by); by1 = std::max(by1, by);
          }
      x0 = bx0 * 16; y0 = by0 * 16;
      x1 = std::min(W, (bx1 + 1) * 16); y1 = std::min(H, (by1 + 1) * 16);
      if (x0 > 0 || y0 > 0 || x1 < W || y1 < H) castecnych++;
    }
    const int rw = x1 - x0, rh = y1 - y0;
    const size_t plocha = (size_t)rw * rh;
    casti = plocha <= 110000 ? 1 : 2;     // 512x480, 640x480: 2 jadra (ne vic - emulace PS1 jich potrebuje par sama)
    jpg.clear();
    const double t0 = tedMsPresne();
    enc.koduj(F + (size_t)y0 * W + x0, (size_t)W, rw, rh, kvalita, jpg, casti);
    const double dt = tedMsPresne() - t0;
    kodMsSoucet += dt; kodN++; if (dt > kodMsMax) kodMsMax = dt;
    p16(out, (unsigned)x0); p16(out, (unsigned)y0); p16(out, (unsigned)rw); p16(out, (unsigned)rh);
    p8(out, (unsigned)kvalita);
    p32(out, (uint32_t)jpg.size());
    out.insert(out.end(), jpg.begin(), jpg.end());
    if (klic) pred.assign(F, F + n);
    else for (int y = y0; y < y1; y++) std::memcpy(&pred[(size_t)y * W + x0], F + (size_t)y * W + x0, (size_t)rw * 4);
    for (int by = y0 / 16; by < (y1 + 15) / 16; by++)
      for (int bx = x0 / 16; bx < (x1 + 15) / 16; bx++) kval[(size_t)by * bw + bx] = (uint8_t)kvalita;
    ma = true;
    return klic ? 1u : 0u;
  }
};

// Prectene casti obrazu verze 3 (pro testy na PC; JPEG si rozbali test sam).
struct CastJpeg {
  unsigned x = 0, y = 0, w = 0, h = 0, kvalita = 0;
  const uint8_t *jpeg = nullptr;
  size_t delka = 0;
  // vrati pocet prectenych bajtu, 0 = chyba
  size_t cti(const uint8_t *p, size_t n) {
    if (n < 13) return 0;
    x = p[0] | (p[1] << 8); y = p[2] | (p[3] << 8); w = p[4] | (p[5] << 8); h = p[6] | (p[7] << 8);
    kvalita = p[8];
    delka = (size_t)p[9] | ((size_t)p[10] << 8) | ((size_t)p[11] << 16) | ((size_t)p[12] << 24);
    if (13 + delka > n) return 0;
    jpeg = p + 13;
    return 13 + delka;
  }
};

}  // namespace proud
}  // namespace nap
