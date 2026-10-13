// nap_tv_proud_jpegenc.h
// B305: RYCHLY KODER JPEG pro prenos PS1 na TV / PC.
//
// Proc vlastni: stb_image_write (B305 prvni pokus) kodoval 640x480 za ~10 ms
// na PC - na Reneho Galaxy S8 (Android 9, z logu) by to bylo 25-30 ms na snimek,
// a jeho hry jedou i v 512x480 a 640x480. Systemovy koder z C++
// (AndroidBitmap_compress) je az od Androidu 11.
//
// Tenhle koder dela totez co baseline JPEG ze stb (stejne tabulky a DCT -
// prohlizec to rozbali stejne), ale:
//  - barvy primo z 0x??RRGGBB snimku (zadny mezikrok RGB), prumer 2x2 pro
//    barevne slozky se pocita z RGB jednou (ne 4x prevod),
//  - Huffman pres predpocitane kody a 64bitovy zasobnik bitu, vystup primo do
//    pameti (stb vola funkci pro kazdy bajt),
//  - obraz jde rozdelit na pasy, ktere se koduji SOUBEZNE na vic jadrech:
//    pasy se spoji znackami RST (restart) do JEDNOHO JPEG - prohlizec ho
//    rozbali jako kazdy jiny.
// Bez Androidu - testuje se na PC (test_overeni/b305).
#pragma once
#include <algorithm>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace nap {
namespace proud {

class JpegEnc {
 public:
  JpegEnc() = default;
  ~JpegEnc() { zastavPomocniky(); }
  JpegEnc(const JpegEnc &) = delete;
  JpegEnc &operator=(const JpegEnc &) = delete;

  // Zakoduje obdelnik w x h ze snimku F (radek = stride bodu) do out (pripoji).
  // casti > 1 = pasy soubezne (pomocna vlakna se vytvori jednou a cekaji).
  void koduj(const uint32_t *F, size_t stride, int w, int h, int kvalita, std::vector<uint8_t> &out, int casti = 1) {
    if (w <= 0 || h <= 0) return;
    pripravTabulky(kvalita);
    const int mcu = sub_ ? 16 : 8;
    const int mcuX = (w + mcu - 1) / mcu, mcuY = (h + mcu - 1) / mcu;
    casti = std::max(1, std::min(casti, mcuY));
    const int radkuNaCast = (mcuY + casti - 1) / casti;   // radku MCU v jednom pasu
    casti = (mcuY + radkuNaCast - 1) / radkuNaCast;
    hlavicka(out, w, h, casti > 1 ? mcuX * radkuNaCast : 0);
    if ((int)pasy_.size() < casti) pasy_.resize((size_t)casti);
    for (int i = 0; i < casti; i++) {
      Pas &p = pasy_[(size_t)i];
      p.F = F; p.stride = stride; p.w = w; p.h = h;
      p.my0 = i * radkuNaCast; p.my1 = std::min(mcuY, (i + 1) * radkuNaCast);
      p.mcuX = mcuX;
    }
    if (casti == 1) {
      kodujPas(pasy_[0]);
    } else {
      spustPomocniky(casti - 1);
      {
        std::lock_guard<std::mutex> l(pm_);
        praceCasti_ = casti;                     // pasy 1 .. casti-1 rozdava zamek (zadny "stary" index)
        dalsiPas_ = 1;
        hotovo_ = 0;
      }
      pcv_.notify_all();
      kodujPas(pasy_[0]);
      // pomoz s pasy, ktere jeste nikdo nevzal
      while (true) {
        int i;
        {
          std::lock_guard<std::mutex> l(pm_);
          if (dalsiPas_ >= praceCasti_) break;
          i = dalsiPas_++;
        }
        kodujPas(pasy_[(size_t)i]);
        std::lock_guard<std::mutex> l(pm_);
        hotovo_++;
      }
      std::unique_lock<std::mutex> l(pm_);
      hcv_.wait(l, [&] { return hotovo_ >= casti - 1; });
      praceCasti_ = 0; dalsiPas_ = 0;              // prace uzavrena - pomocnik uz nic nevezme
    }
    for (int i = 0; i < casti; i++) {
      const Pas &p = pasy_[(size_t)i];
      out.insert(out.end(), p.data.begin(), p.data.begin() + (long)p.n);
      if (i + 1 < casti) { out.push_back(0xFF); out.push_back((uint8_t)(0xD0 + (i & 7))); }   // RSTn
    }
    out.push_back(0xFF); out.push_back(0xD9);                                                   // EOI
  }

 private:
  struct Pas {
    const uint32_t *F = nullptr;
    size_t stride = 0;
    int w = 0, h = 0, my0 = 0, my1 = 0, mcuX = 0;
    std::vector<uint8_t> data;
    size_t n = 0;
  };

  // ---------------- tabulky ----------------
  int qAkt_ = -1;
  bool sub_ = true;
  uint8_t qtY_[64], qtC_[64];                // zigzag (do hlavicky)
  float fdY_[64], fdC_[64];                  // po DCT (transponovane poradi), vcetne meritek AAN
  uint8_t zzT_[64];                          // pozice po DCT -> poradi v zigzagu
  uint16_t dcYc_[12], dcCc_[12], acYc_[256], acCc_[256];
  uint8_t dcYl_[12], dcCl_[12], acYl_[256], acCl_[256];
  bool huff_ = false;

  static const uint8_t *zigzag() {           // prirozeny index -> poradi v zigzagu (jako stb)
    static const uint8_t z[64] = {0, 1, 5, 6, 14, 15, 27, 28, 2, 4, 7, 13, 16, 26, 29, 42, 3, 8, 12, 17, 25, 30, 41, 43, 9, 11, 18,
                                  24, 31, 40, 44, 53, 10, 19, 23, 32, 39, 45, 52, 54, 20, 22, 33, 38, 46, 51, 55, 60, 21, 34, 37, 47, 50, 56, 59, 61, 35, 36, 48, 49, 57, 58, 62, 63};
    return z;
  }
  static const uint8_t *dcYbits() { static const uint8_t b[17] = {0, 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0}; return b; }
  static const uint8_t *dcVals() { static const uint8_t v[12] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11}; return v; }
  static const uint8_t *acYbits() { static const uint8_t b[17] = {0, 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d}; return b; }
  static const uint8_t *acYvals() {
    static const uint8_t v[162] = {
        0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
        0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
        0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
        0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
        0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
        0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
        0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
    return v;
  }
  static const uint8_t *dcCbits() { static const uint8_t b[17] = {0, 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0}; return b; }
  static const uint8_t *acCbits() { static const uint8_t b[17] = {0, 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77}; return b; }
  static const uint8_t *acCvals() {
    static const uint8_t v[162] = {
        0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
        0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
        0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
        0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
        0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
        0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
        0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa};
    return v;
  }
  // kody Huffmana z poctu delek (JPEG, priloha C)
  static void kody(const uint8_t *bits, const uint8_t *vals, uint16_t *kod, uint8_t *del) {
    unsigned k = 0, c = 0;
    for (int l = 1; l <= 16; l++) {
      for (int i = 0; i < bits[l]; i++, k++) { kod[vals[k]] = (uint16_t)c; del[vals[k]] = (uint8_t)l; c++; }
      c <<= 1;
    }
  }
  void pripravTabulky(int q) {
    if (!huff_) {
      std::memset(acYl_, 0, sizeof(acYl_)); std::memset(acCl_, 0, sizeof(acCl_));
      kody(dcYbits(), dcVals(), dcYc_, dcYl_);
      kody(dcCbits(), dcVals(), dcCc_, dcCl_);
      kody(acYbits(), acYvals(), acYc_, acYl_);
      kody(acCbits(), acCvals(), acCc_, acCl_);
      huff_ = true;
    }
    q = std::max(1, std::min(100, q));
    if (q == qAkt_) return;
    qAkt_ = q;
    sub_ = q <= 90;                           // jako stb: nad 90 barvy v plnem rozliseni
    static const int YQT[] = {16, 11, 10, 16, 24, 40, 51, 61, 12, 12, 14, 19, 26, 58, 60, 55, 14, 13, 16, 24, 40, 57, 69, 56, 14, 17, 22, 29, 51, 87, 80, 62, 18, 22,
                              37, 56, 68, 109, 103, 77, 24, 35, 55, 64, 81, 104, 113, 92, 49, 64, 78, 87, 103, 121, 120, 101, 72, 92, 95, 98, 112, 100, 103, 99};
    static const int UVQT[] = {17, 18, 24, 47, 99, 99, 99, 99, 18, 21, 26, 66, 99, 99, 99, 99, 24, 26, 56, 99, 99, 99, 99, 99, 47, 66, 99, 99, 99, 99, 99, 99,
                               99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99, 99};
    static const float aasf[] = {1.0f * 2.828427125f, 1.387039845f * 2.828427125f, 1.306562965f * 2.828427125f, 1.175875602f * 2.828427125f,
                                 1.0f * 2.828427125f, 0.785694958f * 2.828427125f, 0.541196100f * 2.828427125f, 0.275899379f * 2.828427125f};
    const int s = q < 50 ? 5000 / q : 200 - q * 2;
    const uint8_t *zz = zigzag();
    for (int i = 0; i < 64; i++) {
      const int y = (YQT[i] * s + 50) / 100, c = (UVQT[i] * s + 50) / 100;
      qtY_[zz[i]] = (uint8_t)std::max(1, std::min(255, y));
      qtC_[zz[i]] = (uint8_t)std::max(1, std::min(255, c));
    }
    // DCT (dct2d) vraci koeficient (radek v, sloupec u) na pozici u*8+v
    for (int v = 0; v < 8; v++)
      for (int u = 0; u < 8; u++) {
        const int j = v * 8 + u, k = u * 8 + v;
        fdY_[k] = 1.0f / (qtY_[zz[j]] * aasf[v] * aasf[u]);
        fdC_[k] = 1.0f / (qtC_[zz[j]] * aasf[v] * aasf[u]);
        zzT_[k] = zz[j];
      }
  }

  void hlavicka(std::vector<uint8_t> &o, int w, int h, int restart) {
    static const uint8_t soi[] = {0xFF, 0xD8, 0xFF, 0xE0, 0, 0x10, 'J', 'F', 'I', 'F', 0, 1, 1, 0, 0, 1, 0, 1, 0, 0};
    o.insert(o.end(), soi, soi + sizeof(soi));
    o.push_back(0xFF); o.push_back(0xDB); o.push_back(0); o.push_back(0x84);
    o.push_back(0); o.insert(o.end(), qtY_, qtY_ + 64);
    o.push_back(1); o.insert(o.end(), qtC_, qtC_ + 64);
    const uint8_t sof[] = {0xFF, 0xC0, 0, 0x11, 8, (uint8_t)(h >> 8), (uint8_t)h, (uint8_t)(w >> 8), (uint8_t)w,
                           3, 1, (uint8_t)(sub_ ? 0x22 : 0x11), 0, 2, 0x11, 1, 3, 0x11, 1};
    o.insert(o.end(), sof, sof + sizeof(sof));
    o.push_back(0xFF); o.push_back(0xC4); o.push_back(0x01); o.push_back(0xA2);
    o.push_back(0x00); o.insert(o.end(), dcYbits() + 1, dcYbits() + 17); o.insert(o.end(), dcVals(), dcVals() + 12);
    o.push_back(0x10); o.insert(o.end(), acYbits() + 1, acYbits() + 17); o.insert(o.end(), acYvals(), acYvals() + 162);
    o.push_back(0x01); o.insert(o.end(), dcCbits() + 1, dcCbits() + 17); o.insert(o.end(), dcVals(), dcVals() + 12);
    o.push_back(0x11); o.insert(o.end(), acCbits() + 1, acCbits() + 17); o.insert(o.end(), acCvals(), acCvals() + 162);
    if (restart > 0) { const uint8_t dri[] = {0xFF, 0xDD, 0, 4, (uint8_t)(restart >> 8), (uint8_t)restart}; o.insert(o.end(), dri, dri + 6); }
    static const uint8_t sos[] = {0xFF, 0xDA, 0, 0x0C, 3, 1, 0, 2, 0x11, 3, 0x11, 0, 0x3F, 0};
    o.insert(o.end(), sos, sos + sizeof(sos));
  }

  // ---------------- kodovani ----------------
  struct Bity {
    uint8_t *p; uint64_t acc = 0; int n = 0;
    inline void put(uint32_t kod, int len) {
      acc = (acc << len) | kod; n += len;
      while (n >= 8) {
        n -= 8;
        const uint8_t b = (uint8_t)(acc >> n);
        *p++ = b;
        if (b == 0xFF) *p++ = 0;
      }
    }
    inline void konec() { if (n > 0) put((1u << (8 - n)) - 1, 8 - n); }   // doplnit jednickami
  };

  // AAN (stejne jako stb_image_write) - pro float i pro 4 floaty naraz (SIMD:
  // NEON v telefonu, SSE na PC; vektory GCC/Clang, zadne zvlastni instrukce)
  typedef float f4 __attribute__((vector_size(16)));
  template <class T>
  static inline void dct8(T &d0, T &d1, T &d2, T &d3, T &d4, T &d5, T &d6, T &d7) {
    T tmp0 = d0 + d7, tmp7 = d0 - d7, tmp1 = d1 + d6, tmp6 = d1 - d6;
    T tmp2 = d2 + d5, tmp5 = d2 - d5, tmp3 = d3 + d4, tmp4 = d3 - d4;
    T tmp10 = tmp0 + tmp3, tmp13 = tmp0 - tmp3, tmp11 = tmp1 + tmp2, tmp12 = tmp1 - tmp2;
    d0 = tmp10 + tmp11; d4 = tmp10 - tmp11;
    T z1 = (tmp12 + tmp13) * 0.707106781f;
    d2 = tmp13 + z1; d6 = tmp13 - z1;
    tmp10 = tmp4 + tmp5; tmp11 = tmp5 + tmp6; tmp12 = tmp6 + tmp7;
    T z5 = (tmp10 - tmp12) * 0.382683433f;
    T z2 = tmp10 * 0.541196100f + z5;
    T z4 = tmp12 * 1.306562965f + z5;
    T z3 = tmp11 * 0.707106781f;
    T z11 = tmp7 + z3, z13 = tmp7 - z3;
    d5 = z13 + z2; d3 = z13 - z2; d1 = z11 + z4; d7 = z11 - z4;
  }
  static inline f4 ld4(const float *p) { f4 v; std::memcpy(&v, p, 16); return v; }
  static inline void st4(float *p, f4 v) { std::memcpy(p, &v, 16); }
  // 2D DCT bloku 8x8 (radek = s floatu): sloupce 4 naraz, transpozice, znovu.
  // Vysledek: koeficient (v, u) na out[u*8+v].
  static inline void dct2d(const float *src, int s, float *out) {
    f4 a0 = ld4(src), a1 = ld4(src + s), a2 = ld4(src + 2 * s), a3 = ld4(src + 3 * s);
    f4 a4 = ld4(src + 4 * s), a5 = ld4(src + 5 * s), a6 = ld4(src + 6 * s), a7 = ld4(src + 7 * s);
    f4 b0 = ld4(src + 4), b1 = ld4(src + s + 4), b2 = ld4(src + 2 * s + 4), b3 = ld4(src + 3 * s + 4);
    f4 b4 = ld4(src + 4 * s + 4), b5 = ld4(src + 5 * s + 4), b6 = ld4(src + 6 * s + 4), b7 = ld4(src + 7 * s + 4);
    dct8(a0, a1, a2, a3, a4, a5, a6, a7);
    dct8(b0, b1, b2, b3, b4, b5, b6, b7);
    alignas(16) float t[64];
    const f4 *ra[8] = {&a0, &a1, &a2, &a3, &a4, &a5, &a6, &a7}, *rb[8] = {&b0, &b1, &b2, &b3, &b4, &b5, &b6, &b7};
    for (int r = 0; r < 8; r++) {
      const f4 x = *ra[r], y = *rb[r];
      t[0 * 8 + r] = x[0]; t[1 * 8 + r] = x[1]; t[2 * 8 + r] = x[2]; t[3 * 8 + r] = x[3];
      t[4 * 8 + r] = y[0]; t[5 * 8 + r] = y[1]; t[6 * 8 + r] = y[2]; t[7 * 8 + r] = y[3];
    }
    a0 = ld4(t); a1 = ld4(t + 8); a2 = ld4(t + 16); a3 = ld4(t + 24); a4 = ld4(t + 32); a5 = ld4(t + 40); a6 = ld4(t + 48); a7 = ld4(t + 56);
    b0 = ld4(t + 4); b1 = ld4(t + 12); b2 = ld4(t + 20); b3 = ld4(t + 28); b4 = ld4(t + 36); b5 = ld4(t + 44); b6 = ld4(t + 52); b7 = ld4(t + 60);
    dct8(a0, a1, a2, a3, a4, a5, a6, a7);
    dct8(b0, b1, b2, b3, b4, b5, b6, b7);
    st4(out, a0); st4(out + 8, a1); st4(out + 16, a2); st4(out + 24, a3); st4(out + 32, a4); st4(out + 40, a5); st4(out + 48, a6); st4(out + 56, a7);
    st4(out + 4, b0); st4(out + 12, b1); st4(out + 20, b2); st4(out + 28, b3); st4(out + 36, b4); st4(out + 44, b5); st4(out + 52, b6); st4(out + 60, b7);
  }

  // jeden blok 8x8 (radek = s floatu); vrati novou DC
  inline int blok(Bity &bt, const float *b, int s, const float *fd, int dc, const uint16_t *dcc, const uint8_t *dcl, const uint16_t *acc, const uint8_t *acl) {
    alignas(16) float d[64];
    dct2d(b, s, d);
    int z[64];
    uint64_t nenul = 0;                        // bit i = z[i] neni nula (poradi zigzag)
    for (int k = 0; k < 64; k++) {
      const float v = d[k] * fd[k];
      const int q = (int)(v < 0 ? v - 0.5f : v + 0.5f);
      const int i = zzT_[k];
      z[i] = q;
      nenul |= (uint64_t)(q != 0) << i;
    }
    // DC
    const int diff = z[0] - dc;
    {
      const int a = diff < 0 ? -diff : diff;
      const int nb = a ? 32 - __builtin_clz((unsigned)a) : 0;
      const unsigned extra = (unsigned)(diff < 0 ? diff - 1 : diff) & ((1u << nb) - 1);
      bt.put(((uint32_t)dcc[nb] << nb) | extra, dcl[nb] + nb);
    }
    // AC: jen nenulove (preskoceni nul pres masku)
    uint64_t m = nenul & ~(uint64_t)1;
    int pred = 0;
    while (m) {
      const int i = __builtin_ctzll(m);
      m &= m - 1;
      int nul = i - pred - 1;
      while (nul >= 16) { bt.put(acc[0xF0], acl[0xF0]); nul -= 16; }
      const int v = z[i];
      const int a = v < 0 ? -v : v;
      const int nb = 32 - __builtin_clz((unsigned)a);
      const unsigned extra = (unsigned)(v < 0 ? v - 1 : v) & ((1u << nb) - 1);
      const int sym = (nul << 4) | nb;
      bt.put(((uint32_t)acc[sym] << nb) | extra, acl[sym] + nb);
      pred = i;
    }
    if (pred != 63) bt.put(acc[0x00], acl[0x00]);
    return z[0];
  }

  void kodujPas(Pas &p) {
    const int mcu = sub_ ? 16 : 8;
    const size_t mcuPocet = (size_t)p.mcuX * (size_t)(p.my1 - p.my0);
    const size_t maxB = mcuPocet * (sub_ ? 6 : 3) * 450 + 64;  // nejhorsi blok 64 x 27 bitu = 216 B, se zdvojenim 0xFF 432 B
    if (p.data.size() < maxB) p.data.resize(maxB);
    Bity bt; bt.p = p.data.data();
    int dcY = 0, dcU = 0, dcV = 0;
    alignas(16) float Y[256], U[64], V[64];
    for (int my = p.my0; my < p.my1; my++) {
      const int y0 = my * mcu;
      for (int mx = 0; mx < p.mcuX; mx++) {
        const int x0 = mx * mcu;
        const bool cely = x0 + mcu <= p.w && y0 + mcu <= p.h;
        if (sub_ && cely) {
          // cely MCU uvnitr obrazu: primocare smycky (prekladac je zvektorizuje)
          alignas(16) float R[256], G[256], B[256];
          for (int yy = 0; yy < 16; yy++) {
            const uint32_t *r = p.F + (size_t)(y0 + yy) * p.stride + x0;
            float *rr = R + yy * 16, *gg = G + yy * 16, *bb = B + yy * 16, *y = Y + yy * 16;
            for (int xx = 0; xx < 16; xx++) {
              const uint32_t c = r[xx];
              rr[xx] = (float)((c >> 16) & 255); gg[xx] = (float)((c >> 8) & 255); bb[xx] = (float)(c & 255);
            }
            for (int xx = 0; xx < 16; xx++) y[xx] = 0.29900f * rr[xx] + 0.58700f * gg[xx] + 0.11400f * bb[xx] - 128.f;
          }
          for (int yy = 0; yy < 8; yy++)
            for (int xx = 0; xx < 8; xx++) {
              const int a = yy * 32 + xx * 2;
              const float r = (R[a] + R[a + 1] + R[a + 16] + R[a + 17]) * 0.25f;
              const float g = (G[a] + G[a + 1] + G[a + 16] + G[a + 17]) * 0.25f;
              const float b = (B[a] + B[a + 1] + B[a + 16] + B[a + 17]) * 0.25f;
              U[yy * 8 + xx] = -0.16874f * r - 0.33126f * g + 0.50000f * b;
              V[yy * 8 + xx] = 0.50000f * r - 0.41869f * g - 0.08131f * b;
            }
          dcY = blok(bt, Y, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcY = blok(bt, Y + 8, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcY = blok(bt, Y + 128, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcY = blok(bt, Y + 136, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcU = blok(bt, U, 8, fdC_, dcU, dcCc_, dcCl_, acCc_, acCl_);
          dcV = blok(bt, V, 8, fdC_, dcV, dcCc_, dcCl_, acCc_, acCl_);
        } else if (sub_) {
          for (int yy = 0; yy < 16; yy += 2) {
            const uint32_t *r0, *r1;
            const int ra = cely ? y0 + yy : std::min(y0 + yy, p.h - 1), rb = cely ? y0 + yy + 1 : std::min(y0 + yy + 1, p.h - 1);
            r0 = p.F + (size_t)ra * p.stride; r1 = p.F + (size_t)rb * p.stride;
            for (int xx = 0; xx < 16; xx += 2) {
              const int ca = cely ? x0 + xx : std::min(x0 + xx, p.w - 1), cb = cely ? x0 + xx + 1 : std::min(x0 + xx + 1, p.w - 1);
              const uint32_t c00 = r0[ca], c01 = r0[cb], c10 = r1[ca], c11 = r1[cb];
              float *y = &Y[yy * 16 + xx];
              const float R00 = (float)((c00 >> 16) & 255), G00 = (float)((c00 >> 8) & 255), B00 = (float)(c00 & 255);
              const float R01 = (float)((c01 >> 16) & 255), G01 = (float)((c01 >> 8) & 255), B01 = (float)(c01 & 255);
              const float R10 = (float)((c10 >> 16) & 255), G10 = (float)((c10 >> 8) & 255), B10 = (float)(c10 & 255);
              const float R11 = (float)((c11 >> 16) & 255), G11 = (float)((c11 >> 8) & 255), B11 = (float)(c11 & 255);
              y[0] = 0.29900f * R00 + 0.58700f * G00 + 0.11400f * B00 - 128.f;
              y[1] = 0.29900f * R01 + 0.58700f * G01 + 0.11400f * B01 - 128.f;
              y[16] = 0.29900f * R10 + 0.58700f * G10 + 0.11400f * B10 - 128.f;
              y[17] = 0.29900f * R11 + 0.58700f * G11 + 0.11400f * B11 - 128.f;
              const float R = (R00 + R01 + R10 + R11) * 0.25f, G = (G00 + G01 + G10 + G11) * 0.25f, B = (B00 + B01 + B10 + B11) * 0.25f;
              const int k = (yy >> 1) * 8 + (xx >> 1);
              U[k] = -0.16874f * R - 0.33126f * G + 0.50000f * B;
              V[k] = 0.50000f * R - 0.41869f * G - 0.08131f * B;
            }
          }
          dcY = blok(bt, Y, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcY = blok(bt, Y + 8, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcY = blok(bt, Y + 128, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcY = blok(bt, Y + 136, 16, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcU = blok(bt, U, 8, fdC_, dcU, dcCc_, dcCl_, acCc_, acCl_);
          dcV = blok(bt, V, 8, fdC_, dcV, dcCc_, dcCl_, acCc_, acCl_);
        } else {
          for (int yy = 0; yy < 8; yy++) {
            const int ra = cely ? y0 + yy : std::min(y0 + yy, p.h - 1);
            const uint32_t *r0 = p.F + (size_t)ra * p.stride;
            for (int xx = 0; xx < 8; xx++) {
              const uint32_t c = r0[cely ? x0 + xx : std::min(x0 + xx, p.w - 1)];
              const float R = (float)((c >> 16) & 255), G = (float)((c >> 8) & 255), B = (float)(c & 255);
              const int k = yy * 8 + xx;
              Y[k] = 0.29900f * R + 0.58700f * G + 0.11400f * B - 128.f;
              U[k] = -0.16874f * R - 0.33126f * G + 0.50000f * B;
              V[k] = 0.50000f * R - 0.41869f * G - 0.08131f * B;
            }
          }
          dcY = blok(bt, Y, 8, fdY_, dcY, dcYc_, dcYl_, acYc_, acYl_);
          dcU = blok(bt, U, 8, fdC_, dcU, dcCc_, dcCl_, acCc_, acCl_);
          dcV = blok(bt, V, 8, fdC_, dcV, dcCc_, dcCl_, acCc_, acCl_);
        }
      }
    }
    bt.konec();
    p.n = (size_t)(bt.p - p.data.data());
  }

  // ---------------- pomocna vlakna (pasy soubezne) ----------------
  std::vector<Pas> pasy_;
  std::vector<std::thread> pomocnici_;
  std::mutex pm_;
  std::condition_variable pcv_, hcv_;
  int praceCasti_ = 0, dalsiPas_ = 0, hotovo_ = 0;   // pod pm_
  bool konci_ = false;

  // Pomocnik si pod zamkem vezme dalsi pas (index a kontrola meze naraz - nemuze
  // vzit pas z jineho snimku), zakoduje ho mimo zamek a ohlasi hotovo.
  void spustPomocniky(int kolik) {
    while ((int)pomocnici_.size() < kolik) {
      pomocnici_.emplace_back([this] {
        while (true) {
          int i;
          {
            std::unique_lock<std::mutex> l(pm_);
            pcv_.wait(l, [&] { return konci_ || dalsiPas_ < praceCasti_; });
            if (konci_) return;
            i = dalsiPas_++;
          }
          kodujPas(pasy_[(size_t)i]);
          { std::lock_guard<std::mutex> l(pm_); hotovo_++; }
          hcv_.notify_all();
        }
      });
    }
  }
  void zastavPomocniky() {
    {
      std::lock_guard<std::mutex> l(pm_);
      konci_ = true;
    }
    pcv_.notify_all();
    for (auto &t : pomocnici_) if (t.joinable()) t.join();
    pomocnici_.clear();
  }
};

}  // namespace proud
}  // namespace nap
