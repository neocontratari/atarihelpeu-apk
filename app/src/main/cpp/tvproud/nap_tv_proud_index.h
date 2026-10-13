// nap_tv_proud_index.h
// B305: SEGA NA TV / PC CELA V C++ - KODEK "INDEXY" (verze paketu 2), BEZ ZTRAT.
//
// Snimek Segy ma nejvys 192 barev naraz (64 z CRAM, s polojasem a stinem 3x),
// takze jde poslat presne, bez ztraty kvality - jako u Atari (B303). Rozdil:
// Sega se HYBE (Sonic: cela obrazovka kazdy snimek o par bodu, pozadi jinou
// rychlosti nez popredi). Atari-zpusob "jen zmenene radky" by tu poslal skoro
// cely snimek porad. Proto LZ s odkazem i do PREDCHOZIHO snimku:
//
//   buf = [ co ma prohlizec (minuly snimek) | novy snimek ]   v indexech barev
//
// a novy snimek se popise operacemi nad timhle bufferem:
//   LIT  n bajtu primo           (nove body)
//   RUN  n x stejny index        (plocha jedne barvy)
//   COPY n bajtu ze vzdalenosti d (posunuty kus minuleho snimku = scrolling,
//                                  opakujici se dlazdice v novem snimku)
//   SKIP n bajtu jako minule na stejnem miste (nic se nezmenilo)
// Pri scrollingu ma kazdy radek vrstvy stejny posun -> "COPY se stejnou
// vzdalenosti jako minule" (2 bajty). Staticka obrazovka = jeden SKIP.
//
// Paleta: barva -> index si pamatuje kazdy prohlizec zvlast (jako B303).
// Kdyz se paleta zaplni (stmivani obrazovky = kazdy snimek nove barvy),
// nove barvy dostanou mista barev, ktere v aktualnim snimku nejsou - zadny
// klicovy snimek, indexy zbytku obrazu zustanou (LZ dal odkazuje do minula).
//
// Bez Androidu - testuje se na PC (test_overeni/b305).
//
// CAST OBRAZU V PAKETU (verze 2):
//   u16 palZmen, pak palZmen x (u8 index, u8 R, u8 G, u8 B)
//   u32 delka LZ, pak operace:
//     bajt: op = b >> 6 (0 LIT, 1 RUN, 2 COPY, 3 SKIP), n = b & 63
//           delka = n + 1, pro n = 63 delka = 64 + varint
//     LIT:  delka bajtu indexu
//     RUN:  1 bajt index
//     COPY: varint vzdalenost (0 = stejna jako u minule COPY)
//     SKIP: nic (zdroj = stejne misto v minulem snimku)
//   varint = 7 bitu na bajt, horni bit = pokracuje (LEB128)
//   delka LZ 0 = obraz beze zmeny.
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>
#include <string>
#include <cstdio>
#include "nap_tv_proud_spol.h"

namespace nap {
namespace proud {

struct KodekIndex {
  static constexpr int VERZE = 2;
  static const char *nazev() { return "paleta + LZ (bez ztraty)"; }

  int W = 0, H = 0;
  size_t N = 0;
  bool ma = false;                       // prohlizec ma obraz (jinak pristi = klicovy)
  std::vector<uint8_t> buf;              // 2N: [co ma prohlizec | novy snimek]
  std::vector<uint32_t> pred;            // posledni poslany snimek RGB (beze zmeny = memcmp)
  uint32_t pal[256];
  int palPocet = 0;
  // barva -> index (otevrene adresovani; klic = barva + 1, 0 = volno)
  static constexpr int HB = 2048;
  uint32_t hK[HB];
  uint8_t hI[HB];
  uint8_t zmenaZn[256];
  std::vector<uint8_t> zmenene;          // indexy palety zmenene v tomto snimku
  // LZ
  static constexpr int LZ_BITU = 15;
  std::vector<int32_t> lzHlava, lzDalsi;
  std::vector<uint8_t> lz, lit;
  // statistika (do logu)
  long long klicovych = 0, prestaveb = 0, nejblizsich = 0, snimku = 0, beze = 0;
  long long opLit = 0, opRun = 0, opCopy = 0, opRep = 0, opSkip = 0;

  KodekIndex() { zapomen(); }

  void zapomen() {                       // prohlizec nema nic - pristi paket klicovy
    ma = false;
    pred.clear();
  }
  std::string info() const {
    char b[200];
    snprintf(b, sizeof(b), "klicovych %lld, prestaveb palety %lld, beze zmeny %lld z %lld", klicovych, prestaveb, beze, snimku);
    return b;
  }

  // ---------- paleta ----------
  static uint32_t hs(uint32_t c) { return (c * 2654435761u) >> 21; }   // 11 bitu
  void hashZnovu() {
    std::memset(hK, 0, sizeof(hK));
    for (int i = 0; i < palPocet; i++) vlozBarvu(pal[i], i);
  }
  int hledej(uint32_t c) const {
    for (uint32_t s = hs(c);; s = (s + 1) & (HB - 1)) {
      if (hK[s] == 0) return -1;
      if (hK[s] == c + 1) return hI[s];
    }
  }
  void vlozBarvu(uint32_t c, int i) {
    for (uint32_t s = hs(c);; s = (s + 1) & (HB - 1)) {
      if (hK[s] == 0) { hK[s] = c + 1; hI[s] = (uint8_t)i; return; }
      if (hK[s] == c + 1) { hI[s] = (uint8_t)i; return; }
    }
  }
  void oznac(int i) { if (!zmenaZn[i]) { zmenaZn[i] = 1; zmenene.push_back((uint8_t)i); } }
  int nejblizsi(uint32_t c) const {
    int best = 0; long bd = 0x7FFFFFFF;
    const int r = (c >> 16) & 255, g = (c >> 8) & 255, b = c & 255;
    for (int i = 0; i < palPocet; i++) {
      const uint32_t p = pal[i];
      const long dr = r - (int)((p >> 16) & 255), dg = g - (int)((p >> 8) & 255), db = b - (int)(p & 255);
      const long d = dr * dr * 3 + dg * dg * 4 + db * db * 2;
      if (d < bd) { bd = d; best = i; }
    }
    return best;
  }
  // snimek -> indexy (dst); false = paleta plna a prisla nova barva
  bool naIndexy(const uint32_t *rgb, uint8_t *dst, size_t n, bool dovolNejblizsi) {
    uint32_t posl = 0xFFFFFFFFu; uint8_t poslI = 0;
    for (size_t i = 0; i < n; i++) {
      const uint32_t c = rgb[i] & 0xFFFFFFu;
      if (c != posl) {
        int k = hledej(c);
        if (k < 0) {
          if (palPocet < 256) { k = palPocet++; pal[k] = c; vlozBarvu(c, k); oznac(k); }
          else if (!dovolNejblizsi) return false;
          else { k = nejblizsi(c); nejblizsich++; }
        }
        posl = c; poslI = (uint8_t)k;
      }
      dst[i] = poslI;
    }
    return true;
  }
  // Paleta plna a v snimku je nova barva: barvy, ktere v tomhle snimku nejsou,
  // uvolni misto novym. Indexy barev, ktere zustaly, se nemeni.
  void prestavPaletu(const uint32_t *rgb, size_t n) {
    prestaveb++;
    static const int SB = 8192;           // mnozina barev snimku
    std::vector<uint32_t> mn(SB, 0);
    int pocet = 0;
    uint32_t posl = 0xFFFFFFFFu;
    for (size_t i = 0; i < n && pocet < SB * 3 / 4; i++) {
      const uint32_t c = rgb[i] & 0xFFFFFFu;
      if (c == posl) continue;
      posl = c;
      for (uint32_t s = (c * 2654435761u) >> 19;; s = (s + 1) & (SB - 1)) {
        if (mn[s] == 0) { mn[s] = c + 1; pocet++; break; }
        if (mn[s] == c + 1) break;
      }
    }
    auto vSnimku = [&](uint32_t c) {
      for (uint32_t s = (c * 2654435761u) >> 19;; s = (s + 1) & (SB - 1)) {
        if (mn[s] == 0) return false;
        if (mn[s] == c + 1) return true;
      }
    };
    bool drz[256];
    for (int i = 0; i < 256; i++) drz[i] = i < palPocet && vSnimku(pal[i]);
    // nove barvy (nejsou v palete) na volna mista
    int volny = 0;
    for (int s = 0; s < SB; s++) {
      if (!mn[s]) continue;
      const uint32_t c = mn[s] - 1;
      if (hledej(c) >= 0) continue;
      while (volny < 256 && drz[volny]) volny++;
      if (volny >= 256) break;                 // vic nez 256 barev naraz - zbytek nejblizsi
      if (volny >= palPocet) palPocet = volny + 1;
      pal[volny] = c; drz[volny] = true; oznac(volny);
      volny++;
    }
    hashZnovu();
  }

  // ---------- LZ ----------
  static uint32_t lzHash(const uint8_t *p) {
    uint32_t v; std::memcpy(&v, p, 4);
    return (v * 2654435761u) >> (32 - LZ_BITU);
  }
  void lzVloz(size_t i) {
    const uint32_t h = lzHash(&buf[i]);
    lzDalsi[i] = lzHlava[h];
    lzHlava[h] = (int32_t)i;
  }
  static void op(std::vector<uint8_t> &o, unsigned kod, size_t n) {
    if (n < 64) o.push_back((uint8_t)((kod << 6) | (unsigned)(n - 1)));
    else { o.push_back((uint8_t)((kod << 6) | 63u)); pVarint(o, n - 64); }
  }
  static size_t opCena(size_t n) { return n < 64 ? 1 : 1 + varintDelka(n - 64); }
  void litVen() {
    if (lit.empty()) return;
    op(lz, 0, lit.size());
    lz.insert(lz.end(), lit.begin(), lit.end());
    opLit++;
    lit.clear();
  }
  void lzKoduj(bool klic) {
    lz.clear(); lit.clear();
    const size_t N2 = 2 * N;
    const uint8_t *b = buf.data();
    if (lzHlava.size() != ((size_t)1 << LZ_BITU)) lzHlava.resize((size_t)1 << LZ_BITU);
    std::fill(lzHlava.begin(), lzHlava.end(), -1);
    lzDalsi.resize(N2);
    // minuly snimek do slovniku (u klicoveho je to nula - stejne i v prohlizeci)
    if (!klic) for (size_t i = 0; i + 4 <= N; i++) lzVloz(i);
    size_t p = N, rep = 0;
    while (p < N2) {
      const size_t zbyva = N2 - p;
      // kandidati: SKIP (stejne misto minule), RUN, COPY se stejnou vzdalenosti, COPY z hashe
      size_t bestN = 0; long bestZisk = 0; int bestOp = -1; size_t bestD = 0;
      auto zkus = [&](int o, size_t n, size_t d, size_t cena) {
        const long zisk = (long)n - (long)cena;
        if (zisk > bestZisk) { bestZisk = zisk; bestN = n; bestOp = o; bestD = d; }
      };
      const size_t s = klic ? 0 : shoda(b + p, b + p - N, zbyva);
      if (s) zkus(3, s, 0, opCena(s));
      if (s < zbyva) {
        size_t r = 1; while (r < zbyva && b[p + r] == b[p]) r++;
        if (r >= 3) zkus(1, r, 0, opCena(r) + 1);
        if (rep && rep <= p) {
          const size_t n = shoda(b + p, b + p - rep, zbyva);
          if (n >= 3) zkus(2, n, 0, opCena(n) + 1);
        }
        if (bestN < 48 && bestN < zbyva && zbyva >= 4) {
          int hloubka = 24;
          for (int32_t q = lzHlava[lzHash(b + p)]; q >= 0 && hloubka-- > 0; q = lzDalsi[(size_t)q]) {
            const size_t d = p - (size_t)q;
            if (bestN && b[(size_t)q + bestN] != b[p + bestN]) continue;   // rychle vyrazeni (delsi nebude)
            const size_t n = shoda(b + p, b + q, zbyva);
            if (n >= 4) zkus(2, n, d, opCena(n) + varintDelka(d));
            if (n == zbyva) break;
          }
        }
      }
      if (bestOp < 0) {                          // zadna vyhodna shoda - bod primo
        lit.push_back(b[p]);
        if (p + 4 <= N2) lzVloz(p);
        p++;
        continue;
      }
      litVen();
      switch (bestOp) {
        case 3: op(lz, 3, bestN); opSkip++; break;
        case 1: op(lz, 1, bestN); lz.push_back(b[p]); opRun++; break;
        default:
          op(lz, 2, bestN);
          if (bestD == 0 || bestD == rep) { pVarint(lz, 0); opRep++; }
          else { pVarint(lz, bestD); rep = bestD; opCopy++; }
          break;
      }
      // shoda do slovniku (dalsi radky mohou odkazovat sem); dlouhe useky jen kazdy 4. bod
      const size_t konec = p + bestN;
      const size_t krok = bestN > 64 ? 4 : 1;
      for (size_t i = p; i < konec && i + 4 <= N2; i += krok) lzVloz(i);
      p = konec;
    }
    litVen();
  }

  // ---------- jeden snimek ----------
  // rgb = 0x??RRGGBB (W*H). Pripoji cast obrazu paketu do out, vrati priznaky
  // (bit0 klicovy, bit2 beze zmeny).
  unsigned zakoduj(const uint32_t *rgb, int w, int h, int /*zapisMs*/, long long /*tedMs*/, std::vector<uint8_t> &out) {
    snimku++;
    const size_t n = (size_t)w * h;
    bool klic = !ma || w != W || h != H || pred.size() != n;
    if (!klic && std::memcmp(rgb, pred.data(), n * 4) == 0) {
      beze++;
      p16(out, 0); p32(out, 0);
      return 4u;
    }
    zmenene.clear(); std::memset(zmenaZn, 0, sizeof(zmenaZn));
    if (klic) {
      W = w; H = h; N = n;
      buf.assign(2 * N, 0);
      palPocet = 0; std::memset(hK, 0, sizeof(hK));
      klicovych++;
    } else {
      std::memcpy(buf.data(), buf.data() + N, N);   // co ted ma prohlizec
    }
    uint8_t *cur = buf.data() + N;
    if (!naIndexy(rgb, cur, N, false)) {
      prestavPaletu(rgb, N);
      naIndexy(rgb, cur, N, true);
    }
    lzKoduj(klic);
    p16(out, (unsigned)zmenene.size());
    for (uint8_t i : zmenene) {
      const uint32_t c = pal[i];
      p8(out, i); p8(out, (c >> 16) & 255); p8(out, (c >> 8) & 255); p8(out, c & 255);
    }
    p32(out, (uint32_t)lz.size());
    out.insert(out.end(), lz.begin(), lz.end());
    if (pred.size() != n) pred.resize(n);
    std::memcpy(pred.data(), rgb, n * 4);
    ma = true;
    return klic ? 1u : 0u;
  }
};

// Rozbaleni casti obrazu (stejne jako prohlizec) - pro testy na PC.
struct DekoderIndex {
  int W = 0, H = 0;
  size_t N = 0;
  std::vector<uint8_t> buf;            // 2N jako v kodeku
  uint32_t pal[256];
  std::vector<uint8_t> radekZmenen;    // ktere radky se v poslednim snimku prepsaly (ne SKIP)
  bool palZmena = false;
  // p ukazuje na cast obrazu, vrati pocet prectenych bajtu, 0 = chyba
  size_t obraz(const uint8_t *p, size_t n, int w, int h, bool klic) {
    size_t o = 0;
    auto r16 = [&](unsigned &v) { if (o + 2 > n) return false; v = p[o] | (p[o + 1] << 8); o += 2; return true; };
    unsigned pz;
    if (klic || w != W || h != H) {
      W = w; H = h; N = (size_t)w * h;
      buf.assign(2 * N, 0);
      std::memset(pal, 0, sizeof(pal));
    }
    radekZmenen.assign((size_t)H, 0);
    if (!r16(pz)) return 0;
    if (o + (size_t)pz * 4 > n) return 0;
    palZmena = pz > 0;
    for (unsigned i = 0; i < pz; i++) {
      pal[p[o]] = ((uint32_t)p[o + 1] << 16) | ((uint32_t)p[o + 2] << 8) | p[o + 3];
      o += 4;
    }
    if (o + 4 > n) return 0;
    const size_t dl = (size_t)p[o] | ((size_t)p[o + 1] << 8) | ((size_t)p[o + 2] << 16) | ((size_t)p[o + 3] << 24);
    o += 4;
    if (o + dl > n) return 0;
    if (dl == 0) return o;                      // beze zmeny
    std::memcpy(buf.data(), buf.data() + N, N);
    const uint8_t *z = p + o, *ke = p + o + dl;
    size_t q = N, rep = 0;
    const size_t N2 = 2 * N;
    auto varint = [&](size_t &v) {
      v = 0; unsigned sh = 0;
      while (true) { if (z >= ke || sh > 35) return false; const uint8_t c = *z++; v |= (size_t)(c & 0x7F) << sh; if (!(c & 0x80)) return true; sh += 7; }
    };
    auto oznac = [&](size_t a, size_t len) {
      for (size_t y = (a - N) / (size_t)W, y1 = (a + len - 1 - N) / (size_t)W; y <= y1; y++) radekZmenen[y] = 1;
    };
    while (z < ke) {
      const uint8_t b = *z++;
      const unsigned kod = b >> 6;
      size_t len = (b & 63) + 1;
      if ((b & 63) == 63) { size_t v; if (!varint(v)) return 0; len = 64 + v; }
      if (q + len > N2) return 0;
      if (kod == 0) {
        if (z + len > ke) return 0;
        std::memcpy(&buf[q], z, len); z += len; oznac(q, len);
      } else if (kod == 1) {
        if (z >= ke) return 0;
        std::memset(&buf[q], *z++, len); oznac(q, len);
      } else if (kod == 2) {
        size_t d; if (!varint(d)) return 0;
        if (d == 0) d = rep; else rep = d;
        if (d == 0 || d > q) return 0;
        for (size_t i = 0; i < len; i++) buf[q + i] = buf[q - d + i];
        oznac(q, len);
      } else {
        std::memcpy(&buf[q], &buf[q - N], len);
      }
      q += len;
    }
    if (q != N2) return 0;
    return o + dl;
  }
  uint32_t barva(int x, int y) const { return pal[buf[N + (size_t)y * W + x]]; }
};

}  // namespace proud
}  // namespace nap
