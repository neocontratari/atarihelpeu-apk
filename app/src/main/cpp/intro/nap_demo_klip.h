// nap_demo_klip.h
// ====================================================================
//  B307: ZABERY ZE HRY ATARI PRO UVODNI DEMO (Reneho Pitt-Kitt Platinum)
//
//  Klip nahral na PC SKUTECNY C++ emulator Atari 130XE z appky
//  (test_overeni/b307/nahraj_hru.cpp): kazdy snimek jako kody barev GTIA
//  384 x 240, ulozeny jako rozdil proti minulemu snimku (XOR) a zabaleny
//  zlib; k tomu paleta a zvuk POKEY 44,1 kHz. Tady se snimky rozbaluji
//  postupne po jednom (v pameti jen aktualni snimek), zvuk najednou.
//
//  Format "NAPKLIP1": W H pocet fps*100 vzorkovani vzorku useku (u32 LE),
//  paleta 256 x RGB, useky (prvni snimek, pocet), snimky (delka + zlib),
//  zvuk (delka + zlib int16 mono).
//  Rozbaleni zlib: stbi_zlib_decode_buffer (stb_image je v knihovne uz kvuli
//  logu - zadna dalsi knihovna).
// ====================================================================
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace napdemo {

class KlipAtari {
 public:
  int W = 0, H = 0, pocet = 0, vzorkovani = 0;
  float fps = 50.f;
  struct Usek { int prvni, pocet; };
  std::vector<Usek> useky;
  uint8_t paleta[256 * 3];
  std::vector<int16_t> zvuk;

  bool nacti(const unsigned char *d, size_t n) {
    ok = false;
    data.clear(); offs.clear(); delky.clear(); useky.clear(); zvuk.clear(); cur.clear(); curK = -1;
    if (!d || n < 36 + 768 || std::memcmp(d, "NAPKLIP1", 8) != 0) return false;
    auto u32 = [&](size_t o) { return (uint32_t)d[o] | ((uint32_t)d[o + 1] << 8) | ((uint32_t)d[o + 2] << 16) | ((uint32_t)d[o + 3] << 24); };
    W = (int)u32(8); H = (int)u32(12); pocet = (int)u32(16); fps = u32(20) / 100.f; vzorkovani = (int)u32(24);
    const uint32_t vzorku = u32(28), nUseku = u32(32);
    if (W <= 0 || H <= 0 || W > 1024 || H > 1024 || pocet <= 0 || pocet > 10000 || nUseku > 64) return false;
    size_t p = 36;
    std::memcpy(paleta, d + p, 768); p += 768;
    for (uint32_t i = 0; i < nUseku; i++) {
      if (p + 8 > n) return false;
      useky.push_back({(int)u32(p), (int)u32(p + 4)}); p += 8;
    }
    for (int i = 0; i < pocet; i++) {
      if (p + 4 > n) return false;
      const uint32_t l = u32(p); p += 4;
      if (p + l > n) return false;
      offs.push_back(p); delky.push_back(l); p += l;
    }
    if (p + 4 <= n) {
      const uint32_t l = u32(p); p += 4;
      if (p + l <= n && vzorku > 0 && vzorku < 4000000) {
        zvuk.assign(vzorku, 0);
        const int r = stbi_zlib_decode_buffer((char *)zvuk.data(), (int)(vzorku * 2), (const char *)d + p, (int)l);
        if (r != (int)(vzorku * 2)) zvuk.clear();
      }
    }
    data.assign(d, d + n);
    cur.assign((size_t)W * H, 0);
    curK = -1;
    ok = true;
    return true;
  }
  bool je() const { return ok; }

  // Snimek k (0..pocet-1) jako kody barev W x H. Rozbaluje dopredu; kdyz se
  // chce zpatky, zacne znovu od zacatku (v demu se jde jen dopredu).
  const uint8_t *snimek(int k) {
    if (!ok) return nullptr;
    if (k < 0) k = 0;
    if (k >= pocet) k = pocet - 1;
    if (k < curK) { std::fill(cur.begin(), cur.end(), 0); curK = -1; }
    std::vector<uint8_t> x((size_t)W * H);
    while (curK < k) {
      const int i = curK + 1;
      const int r = stbi_zlib_decode_buffer((char *)x.data(), (int)x.size(), (const char *)data.data() + offs[i], (int)delky[i]);
      if (r != (int)x.size()) { ok = false; return nullptr; }
      for (size_t j = 0; j < x.size(); j++) cur[j] ^= x[j];
      curK = i;
    }
    return cur.data();
  }
  // zvuk useku u od snimku "od" (v useku) na delku sekund - pro stopu dema
  bool zvukUseku(int u, int od, float sekund, std::vector<int16_t> &ven) const {
    ven.clear();
    if (!ok || zvuk.empty() || u < 0 || u >= (int)useky.size() || fps <= 0.f) return false;
    const double naSnimek = (double)vzorkovani / fps;
    const long long a = (long long)((useky[u].prvni + od) * naSnimek);
    const long long n = (long long)(sekund * vzorkovani);
    for (long long i = 0; i < n && a + i < (long long)zvuk.size(); i++) ven.push_back(zvuk[(size_t)(a + i)]);
    return !ven.empty();
  }
  // snimek k jako RGBA (R v nejnizsim bajtu) do ven (W x H)
  bool rgba(int k, std::vector<uint32_t> &ven) {
    const uint8_t *s = snimek(k);
    if (!s) return false;
    ven.resize((size_t)W * H);
    for (size_t j = 0; j < ven.size(); j++) {
      const uint8_t *c = &paleta[s[j] * 3];
      ven[j] = 0xFF000000u | ((uint32_t)c[2] << 16) | ((uint32_t)c[1] << 8) | c[0];
    }
    return true;
  }

 private:
  bool ok = false;
  std::vector<unsigned char> data;
  std::vector<size_t> offs;
  std::vector<uint32_t> delky;
  std::vector<uint8_t> cur;
  int curK = -1;
};

}  // namespace napdemo
