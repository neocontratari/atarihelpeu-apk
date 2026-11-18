// nap_archiv.h
// B349: archivy ZIP a RAR v C++ (bez cizich knihoven) - Atari si hru vybali samo.
//
// Rene: "potrebuju u atari to co u ps1 - pokud to bude zabalene v rar, v zipu,
// tak at si s tim atari poradi - opravdu nehodlam davat 5 souboru ciste ven".
// Hry pro Atari chodi z webu i ze serveru N&P zabalene (Prince of Persia: RAR
// se 3 disketami + readme). Tady je cteni obou formatu cele v C++:
//  - ZIP: metody 0 (ulozeno) a 8 (deflate - vlastni inflate podle RFC 1951),
//  - RAR 4.x (signatura "Rar!" 1A 07 00): metoda 0x30 (ulozeno) a 0x31-0x35
//    s algoritmem RAR 2.9 (verze rozbaleni 29 / 36): LZ + Huffmanovy tabulky
//    + standardni filtry (E8, E8E9, Itanium, Delta, RGB, Audio); pevny archiv
//    (solid) se rozbaluje postupne od zacatku,
//  - RAR 5 (signatura "Rar!" 1A 07 01 00): pozna a poctive rekne, ze ho
//    zatim neumi (zabalit jako ZIP nebo RAR 4).
// Kazdy rozbaleny soubor se kontroluje CRC32 z archivu - kdyby se rozbaleni
// nepovedlo, appka spatna data nespusti (napise chybu).
// Neumi: heslo, archiv na vice dilu, bloky PPMd v RAR (napise to do chyby).
#pragma once
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace nap {
namespace archiv {

// ---------------------------------------------------------------------
//  CRC32 (ZIP i RAR 4: polynom EDB88320)
// ---------------------------------------------------------------------
inline uint32_t crc32(const uint8_t *d, size_t n, uint32_t c = 0) {
  static uint32_t tab[256];
  static bool hotovo = false;
  if (!hotovo) {
    for (uint32_t i = 0; i < 256; i++) {
      uint32_t r = i;
      for (int k = 0; k < 8; k++) r = (r & 1) ? (r >> 1) ^ 0xEDB88320u : r >> 1;
      tab[i] = r;
    }
    hotovo = true;
  }
  c = ~c;
  for (size_t i = 0; i < n; i++) c = tab[(c ^ d[i]) & 0xFF] ^ (c >> 8);
  return ~c;
}

inline uint16_t le16(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
inline uint32_t le32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }

// ---------------------------------------------------------------------
//  INFLATE (deflate, RFC 1951) - kanonicke Huffmanovy kody, dekodovani po
//  bitech (jednoduche a spolehlive; soubory Atari maji kB, ne MB)
// ---------------------------------------------------------------------
class Inflate {
 public:
  // vstup: surovy proud deflate; vystup: max 'limit' bajtu (ochrana pred bombou)
  static bool rozbal(const uint8_t *in, size_t n, std::vector<uint8_t> &out, size_t limit, std::string &chyba) {
    Inflate z(in, n, out, limit);
    const int r = z.bez();
    if (r == 0) return true;
    static const char *duvod[] = {"", "data skoncila driv", "spatny blok", "spatny kod", "spatna vzdalenost", "soubor je moc velky"};
    chyba = std::string("deflate: ") + duvod[r < 0 || r > 5 ? 2 : r];
    return false;
  }

 private:
  struct Huff { uint16_t pocet[16]; uint16_t symbol[320]; };
  const uint8_t *in; size_t n, pos = 0; uint32_t bitbuf = 0; int bitcnt = 0;
  std::vector<uint8_t> &out; size_t limit;
  Inflate(const uint8_t *i, size_t nn, std::vector<uint8_t> &o, size_t l) : in(i), n(nn), out(o), limit(l) {}

  int bity(int k, uint32_t &v) {
    while (bitcnt < k) {
      if (pos >= n) return 1;
      bitbuf |= (uint32_t)in[pos++] << bitcnt;
      bitcnt += 8;
    }
    v = bitbuf & ((1u << k) - 1);
    bitbuf >>= k; bitcnt -= k;
    return 0;
  }
  static int stavej(Huff &h, const uint8_t *delky, int pocet) {
    std::memset(h.pocet, 0, sizeof h.pocet);
    for (int s = 0; s < pocet; s++) h.pocet[delky[s]]++;
    if (h.pocet[0] == pocet) return 0;                     // zadny kod (prazdna tabulka)
    int zbyva = 1;
    for (int l = 1; l < 16; l++) { zbyva <<= 1; zbyva -= h.pocet[l]; if (zbyva < 0) return -1; }
    uint16_t offs[16]; offs[1] = 0;
    for (int l = 1; l < 15; l++) offs[l + 1] = (uint16_t)(offs[l] + h.pocet[l]);
    for (int s = 0; s < pocet; s++) if (delky[s]) h.symbol[offs[delky[s]]++] = (uint16_t)s;
    return zbyva;                                          // > 0 = neuplny kod (smi jen u jednoho kodu)
  }
  int dekod(const Huff &h, int &sym) {
    int kod = 0, prvni = 0, index = 0;
    for (int l = 1; l < 16; l++) {
      uint32_t b;
      if (bity(1, b)) return 1;
      kod |= (int)b;
      const int c = h.pocet[l];
      if (kod - c < prvni) { sym = h.symbol[index + (kod - prvni)]; return 0; }
      index += c; prvni += c; prvni <<= 1; kod <<= 1;
    }
    return 3;
  }
  int kody(const Huff &dl, const Huff &vz) {
    static const uint16_t zakl[29] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
    static const uint8_t ext[29] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
    static const uint16_t dzakl[30] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
    static const uint8_t dext[30] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
    for (;;) {
      int s; int r = dekod(dl, s); if (r) return r;
      if (s < 256) { if (out.size() >= limit) return 5; out.push_back((uint8_t)s); continue; }
      if (s == 256) return 0;
      s -= 257; if (s >= 29) return 3;
      uint32_t e; if (bity(ext[s], e)) return 1;
      const size_t len = zakl[s] + e;
      int ds; r = dekod(vz, ds); if (r) return r;
      if (ds >= 30) return 4;
      if (bity(dext[ds], e)) return 1;
      const size_t dist = dzakl[ds] + e;
      if (dist > out.size()) return 4;
      if (out.size() + len > limit) return 5;
      const size_t od = out.size() - dist;
      for (size_t i = 0; i < len; i++) out.push_back(out[od + i]);
    }
  }
  int bez() {
    int posledni;
    do {
      uint32_t b, typ;
      if (bity(1, b) || bity(2, typ)) return 1;
      posledni = (int)b;
      int r;
      if (typ == 0) {
        bitbuf = 0; bitcnt = 0;                            // zarovnat na bajt
        if (pos + 4 > n) return 1;
        const unsigned len = le16(in + pos), nlen = le16(in + pos + 2); pos += 4;
        if ((len ^ 0xFFFF) != nlen) return 2;
        if (pos + len > n) return 1;
        if (out.size() + len > limit) return 5;
        out.insert(out.end(), in + pos, in + pos + len); pos += len;
        r = 0;
      } else if (typ == 1) {
        struct Pevne {                                       // pevne kody (RFC 1951 3.2.6)
          Huff dl, vz;
          Pevne() {
            uint8_t d[288];
            for (int i = 0; i < 144; i++) d[i] = 8;
            for (int i = 144; i < 256; i++) d[i] = 9;
            for (int i = 256; i < 280; i++) d[i] = 7;
            for (int i = 280; i < 288; i++) d[i] = 8;
            stavej(dl, d, 288);
            for (int i = 0; i < 30; i++) d[i] = 5;
            stavej(vz, d, 30);
          }
        };
        static const Pevne pevne;                            // C++11: bezpecne i z vic vlaken
        r = kody(pevne.dl, pevne.vz);
      } else if (typ == 2) {
        uint32_t hlit, hdist, hclen;
        if (bity(5, hlit) || bity(5, hdist) || bity(4, hclen)) return 1;
        hlit += 257; hdist += 1; hclen += 4;
        if (hlit > 286 || hdist > 30) return 2;
        static const uint8_t poradi[19] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
        uint8_t d[320]; std::memset(d, 0, sizeof d);
        for (unsigned i = 0; i < hclen; i++) { uint32_t v; if (bity(3, v)) return 1; d[poradi[i]] = (uint8_t)v; }
        Huff hk; if (stavej(hk, d, 19) != 0) return 2;
        unsigned i = 0;
        while (i < hlit + hdist) {
          int s; if (dekod(hk, s)) return 3;
          if (s < 16) { d[i++] = (uint8_t)s; continue; }
          uint8_t v = 0; uint32_t opak;
          if (s == 16) { if (i == 0) return 2; v = d[i - 1]; if (bity(2, opak)) return 1; opak += 3; }
          else if (s == 17) { if (bity(3, opak)) return 1; opak += 3; }
          else { if (bity(7, opak)) return 1; opak += 11; }
          if (i + opak > hlit + hdist) return 2;
          while (opak--) d[i++] = v;
        }
        if (d[256] == 0) return 2;                           // chybi konec bloku
        Huff dl, vz;
        int e1 = stavej(dl, d, (int)hlit);
        if (e1 < 0 || (e1 > 0 && hlit - dl.pocet[0] != 1)) return 2;
        int e2 = stavej(vz, d + hlit, (int)hdist);
        if (e2 < 0 || (e2 > 0 && hdist - vz.pocet[0] != 1)) return 2;
        r = kody(dl, vz);
      } else return 2;
      if (r) return r;
    } while (!posledni);
    return 0;
  }
};

// ---------------------------------------------------------------------
//  RAR 2.9 (verze rozbaleni 29): LZ s Huffmanem + filtry
// ---------------------------------------------------------------------
class Rar29 {
 public:
  // rozbali 'velikost' bajtu z 'in'; pri pevnem archivu (solid) se volani
  // opakuje na stejnem objektu pro dalsi soubor (slovnik a tabulky zustavaji)
  bool rozbal(const uint8_t *in, size_t n, uint64_t velikost, bool solid, std::vector<uint8_t> &out, std::string &chyba) {
    vstup = in; vstupN = n; bit = 0; adr = 0; chyb = nullptr;
    if (!solid || !inicializovano) zacni();
    if (velikost > (64u << 20)) { chyba = "rar: soubor je moc velky"; return false; }
    const uint64_t start = okno.size();
    cil = start + velikost;
    if (!solid || !tabulky) {
      if (!ctiTabulky()) { chyba = chyb ? chyb : "rar: spatne tabulky"; return false; }
    }
    bool konecSouboru = false;
    while (okno.size() < cil) {
      if (adr > vstupN + 4) { chyba = "rar: data skoncila driv"; return false; }
      if (ppm) { chyba = "rar: blok PPMd (komprese textu) zatim neumim - zabal hru jako ZIP"; return false; }
      const int c = dekod(LD);
      if (c < 256) { okno.push_back((uint8_t)c); continue; }
      if (c >= 271) {
        int k = c - 271;
        uint32_t delka = kLDecode[k] + 3;
        if (kLBits[k]) { delka += bity16() >> (16 - kLBits[k]); pridej(kLBits[k]); }
        const int dn = dekod(DD);
        uint32_t vzd = dDecode[dn] + 1;
        const int db = dBits[dn];
        if (db > 0) {
          if (dn > 9) {
            if (db > 4) { vzd += ((bity16() >> (20 - db)) << 4); pridej(db - 4); }
            if (lowRep > 0) { lowRep--; vzd += prevLow; }
            else {
              const int low = dekod(LDD);
              if (low == 16) { lowRep = 15; vzd += prevLow; }    // 16 = opakovat posledni nizke bity (15x dal)
              else { vzd += (uint32_t)low; prevLow = (uint32_t)low; }
            }
          } else { vzd += bity16() >> (16 - db); pridej(db); }
        }
        if (vzd >= 0x2000) { delka++; if (vzd >= 0x40000) delka++; }
        vlozStarou(vzd);
        posledniDelka = delka;
        kopiruj(delka, vzd);
        continue;
      }
      if (c == 256) {
        // konec bloku: "1" = nova tabulka hned, "00" = konec souboru, "01" = konec souboru + nova tabulka
        const uint32_t b = bity16();
        bool novaTab, novySoubor = false;
        if (b & 0x8000) { novaTab = true; pridej(1); }
        else { novySoubor = true; novaTab = (b & 0x4000) != 0; pridej(2); }
        tabulky = !novaTab;
        if (novySoubor) { konecSouboru = true; break; }
        if (!ctiTabulky()) { chyba = chyb ? chyb : "rar: spatne tabulky"; return false; }
        continue;
      }
      if (c == 257) { if (!ctiFiltr()) { chyba = chyb ? chyb : "rar: spatny filtr"; return false; } continue; }
      if (c == 258) { if (posledniDelka) kopiruj(posledniDelka, stare[0]); continue; }
      if (c < 263) {
        const int i = c - 259;
        const uint32_t vzd = stare[i];
        for (int j = i; j > 0; j--) stare[j] = stare[j - 1];
        stare[0] = vzd;
        const int ln = dekod(RD);
        uint32_t delka = kLDecode[ln] + 2;
        if (kLBits[ln]) { delka += bity16() >> (16 - kLBits[ln]); pridej(kLBits[ln]); }
        posledniDelka = delka;
        kopiruj(delka, vzd);
        continue;
      }
      // 263..270: kratke vzdalenosti, delka 2
      {
        const int i = c - 263;
        static const uint8_t sdDecode[8] = {0, 4, 8, 16, 32, 64, 128, 192};
        static const uint8_t sdBits[8] = {2, 2, 3, 4, 5, 6, 6, 6};
        uint32_t vzd = sdDecode[i] + 1u;
        if (sdBits[i]) { vzd += bity16() >> (16 - sdBits[i]); pridej(sdBits[i]); }
        vlozStarou(vzd);
        posledniDelka = 2;
        kopiruj(2, vzd);
      }
    }
    if (okno.size() < cil) { chyba = "rar: data skoncila driv"; return false; }
    // za daty souboru pise RAR kod 256 "konec souboru" s bitem "nova tabulka" - u pevneho
    // archivu podle nej dalsi soubor zacina (nebo nezacina) tabulkami
    if (!konecSouboru && !ppm) doctiKonec();
    okno.resize(cil);                     // zbytek posledni shody patri dalsimu souboru jen u solid (nenastava)
    // vystup = okno s pouzitymi filtry (okno zustava puvodni - z nej se kopiruji dalsi shody)
    out.assign(okno.begin() + (long)start, okno.begin() + (long)cil);
    if (!pouzijFiltry(start, out)) { chyba = chyb ? chyb : "rar: filtr se nepovedl"; return false; }
    return true;
  }

 private:
  struct Dekoder { uint32_t len[16]; uint32_t pos[16]; uint16_t num[404]; uint32_t max; };
  struct Filtr { uint64_t start; uint32_t delka; int typ; uint32_t r[7]; };
  enum { F_NIC = 0, F_E8, F_E8E9, F_ITANIUM, F_DELTA, F_RGB, F_AUDIO };
  static constexpr int NC = 299, DC = 60, LDC = 17, RC = 28, BC = 20, HTS = NC + DC + LDC + RC;
  static constexpr uint8_t kLDecode[28] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 10, 12, 14, 16, 20, 24, 28, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224};
  static constexpr uint8_t kLBits[28] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5};

  const uint8_t *vstup = nullptr; size_t vstupN = 0, adr = 0; int bit = 0;
  const char *chyb = nullptr;
  std::vector<uint8_t> okno;            // cely rozbaleny proud (bez filtru)
  uint64_t cil = 0;
  bool inicializovano = false, tabulky = false, ppm = false;
  uint8_t staraTab[HTS];
  Dekoder LD, DD, LDD, RD, BD;
  uint32_t stare[4] = {0, 0, 0, 0}, posledniDelka = 0, prevLow = 0; int lowRep = 0;
  uint32_t dDecode[DC]; uint8_t dBits[DC];
  // filtry: definice (kod podle CRC -> typ) a cekajici bloky
  std::vector<int> typFiltru; std::vector<uint32_t> staraDelka;
  std::vector<Filtr> cekajici;
  int posledniFiltr = 0;

  void zacni() {
    okno.clear(); tabulky = false; ppm = false; inicializovano = true;
    std::memset(staraTab, 0, sizeof staraTab);
    stare[0] = stare[1] = stare[2] = stare[3] = 0; posledniDelka = 0; prevLow = 0; lowRep = 0;
    typFiltru.clear(); staraDelka.clear(); cekajici.clear(); posledniFiltr = 0;
    // pocet slotu vzdalenosti pro delku 0..18 bitu (celkem 60 slotu = slovnik 4 MB)
    static const int pocty[] = {4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 14, 0, 12};
    int slot = 0; uint32_t vzd = 0;
    for (int i = 0, b = 0; i < (int)(sizeof pocty / sizeof pocty[0]); i++, b++)
      for (int j = 0; j < pocty[i] && slot < DC; j++, vzd += (1u << b)) { dDecode[slot] = vzd; dBits[slot] = (uint8_t)b; slot++; }
  }
  // 16 bitu od aktualni pozice (MSB napred), za koncem dat nuly
  uint32_t bity16() const {
    const uint32_t b0 = adr < vstupN ? vstup[adr] : 0, b1 = adr + 1 < vstupN ? vstup[adr + 1] : 0, b2 = adr + 2 < vstupN ? vstup[adr + 2] : 0;
    return (((b0 << 16) | (b1 << 8) | b2) >> (8 - bit)) & 0xFFFF;
  }
  void pridej(int k) { k += bit; adr += (size_t)(k >> 3); bit = k & 7; }

  static void stavDekoder(const uint8_t *delky, int pocet, Dekoder &d) {
    uint32_t cnt[16]; std::memset(cnt, 0, sizeof cnt);
    for (int i = 0; i < pocet; i++) cnt[delky[i] & 15]++;
    cnt[0] = 0;
    std::memset(d.num, 0, sizeof d.num);
    d.pos[0] = 0; d.len[0] = 0;
    uint32_t horni = 0;
    for (int i = 1; i < 16; i++) {
      horni += cnt[i];
      d.len[i] = horni << (16 - i);
      horni *= 2;
      d.pos[i] = d.pos[i - 1] + cnt[i - 1];
    }
    uint32_t kopie[16]; std::memcpy(kopie, d.pos, sizeof kopie);
    for (int i = 0; i < pocet; i++) {
      const int l = delky[i] & 15;
      if (l) d.num[kopie[l]++] = (uint16_t)i;
    }
    d.max = (uint32_t)pocet;
  }
  int dekod(const Dekoder &d) {
    const uint32_t b = bity16() & 0xFFFE;
    int bits = 15;
    for (int i = 1; i < 15; i++) if (b < d.len[i]) { bits = i; break; }
    pridej(bits);
    const uint32_t dist = (b - d.len[bits - 1]) >> (16 - bits);
    uint32_t p = d.pos[bits] + dist;
    if (p >= d.max) p = 0;
    return d.num[p];
  }

  bool ctiTabulky() {
    if (bit) pridej(8 - bit);                       // zarovnat na bajt
    const uint32_t b = bity16();
    if (b & 0x8000) { ppm = true; chyb = "rar: blok PPMd (komprese textu) zatim neumim - zabal hru jako ZIP"; return false; }
    ppm = false;
    prevLow = 0; lowRep = 0;
    if (!(b & 0x4000)) std::memset(staraTab, 0, sizeof staraTab);
    pridej(2);
    uint8_t bl[BC];
    for (int i = 0; i < BC; i++) {
      const uint8_t l = (uint8_t)(bity16() >> 12); pridej(4);
      if (l == 15) {
        int nuly = (int)(bity16() >> 12); pridej(4);
        if (nuly == 0) bl[i] = 15;
        else { nuly += 2; while (nuly-- > 0 && i < BC) bl[i++] = 0; i--; }
      } else bl[i] = l;
    }
    stavDekoder(bl, BC, BD);
    uint8_t tab[HTS];
    for (int i = 0; i < HTS;) {
      if (adr > vstupN + 2) { chyb = "rar: data skoncila v tabulkach"; return false; }
      const int c = dekod(BD);
      if (c < 16) { tab[i] = (uint8_t)((c + staraTab[i]) & 15); i++; }
      else if (c < 18) {
        int k;
        if (c == 16) { k = (int)(bity16() >> 13) + 3; pridej(3); } else { k = (int)(bity16() >> 9) + 11; pridej(7); }
        if (i == 0) { chyb = "rar: spatna tabulka"; return false; }
        while (k-- > 0 && i < HTS) { tab[i] = tab[i - 1]; i++; }
      } else {
        int k;
        if (c == 18) { k = (int)(bity16() >> 13) + 3; pridej(3); } else { k = (int)(bity16() >> 9) + 11; pridej(7); }
        while (k-- > 0 && i < HTS) tab[i++] = 0;
      }
    }
    tabulky = true;
    stavDekoder(tab, NC, LD);
    stavDekoder(tab + NC, DC, DD);
    stavDekoder(tab + NC + DC, LDC, LDD);
    stavDekoder(tab + NC + DC + LDC, RC, RD);
    std::memcpy(staraTab, tab, sizeof staraTab);
    return true;
  }

  void doctiKonec() {
    for (int k = 0; k < 4; k++) {
      if (adr >= vstupN) return;                     // kod konce chybi (starsi RAR) - tabulky plati dal
      if (dekod(LD) != 256) return;
      const uint32_t b = bity16();
      if (b & 0x8000) { pridej(1); tabulky = false; if (!ctiTabulky()) { tabulky = false; return; } continue; }
      tabulky = !(b & 0x4000); pridej(2);
      return;
    }
  }
  void vlozStarou(uint32_t v) { stare[3] = stare[2]; stare[2] = stare[1]; stare[1] = stare[0]; stare[0] = v; }
  void kopiruj(uint32_t delka, uint32_t vzd) {
    const size_t n = okno.size();
    for (uint32_t i = 0; i < delka; i++) {
      const size_t p = n + i;
      // pred zacatkem okna jsou nuly (jako v RAR); vzdalenost 0 (stara vzdalenost z poskozenych dat) = 0
      okno.push_back(vzd != 0 && vzd <= p ? okno[p - vzd] : (uint8_t)0);
    }
  }

  // ---- filtry (kod VM ve filtru se nespousti - jen standardni filtry podle CRC) ----
  struct BitVstup {                                  // samostatny cteni bitu kodu filtru
    const uint8_t *d; size_t n; size_t a = 0; int b = 0;
    uint32_t g() const { const uint32_t b0 = a < n ? d[a] : 0, b1 = a + 1 < n ? d[a + 1] : 0, b2 = a + 2 < n ? d[a + 2] : 0; return (((b0 << 16) | (b1 << 8) | b2) >> (8 - b)) & 0xFFFF; }
    void p(int k) { k += b; a += (size_t)(k >> 3); b = k & 7; }
    uint32_t data() {                                // RarVM::ReadData
      uint32_t v = g();
      switch (v & 0xC000) {
        case 0: p(6); return (v >> 10) & 15;
        case 0x4000:
          if ((v & 0x3C00) == 0) { v = 0xFFFFFF00u | ((v >> 2) & 0xFF); p(14); }
          else { v = (v >> 6) & 0xFF; p(10); }
          return v;
        case 0x8000: p(2); v = g(); p(16); return v;
        default: { p(2); uint32_t x = g() << 16; p(16); x |= g(); p(16); return x; }
      }
    }
  };
  bool ctiFiltr() {
    const uint32_t prvni = bity16() >> 8; pridej(8);
    uint32_t delka = (prvni & 7) + 1;
    if (delka == 7) { delka = (bity16() >> 8) + 7; pridej(8); }
    else if (delka == 8) { delka = bity16(); pridej(16); }
    if (delka == 0) return false;
    std::vector<uint8_t> kod(delka);
    for (uint32_t i = 0; i < delka; i++) { kod[i] = (uint8_t)(bity16() >> 8); pridej(8); }
    BitVstup v{kod.data(), kod.size()};
    uint32_t pozice;
    if (prvni & 0x80) {
      pozice = v.data();
      if (pozice == 0) { typFiltru.clear(); staraDelka.clear(); cekajici.clear(); }   // InitFilters30
      else pozice--;
    } else pozice = (uint32_t)posledniFiltr;
    if (pozice > typFiltru.size() || pozice > 1024) { chyb = "rar: spatne cislo filtru"; return false; }
    posledniFiltr = (int)pozice;
    const bool novy = pozice == typFiltru.size();
    if (novy) { typFiltru.push_back(F_NIC); staraDelka.push_back(0); }
    Filtr f;
    uint32_t start = v.data();
    if (prvni & 0x40) start += 258;
    f.start = okno.size() + start;
    if (prvni & 0x20) { f.delka = v.data(); staraDelka[pozice] = f.delka; }
    else f.delka = staraDelka[pozice];
    std::memset(f.r, 0, sizeof f.r);
    f.r[4] = f.delka;
    if (prvni & 0x10) {
      const uint32_t maska = v.g() >> 9; v.p(7);
      for (int i = 0; i < 7; i++) if (maska & (1u << i)) f.r[i] = v.data();
    }
    if (novy) {
      const uint32_t velikost = v.data();
      if (velikost == 0 || velikost >= 0x10000 || v.a + velikost > kod.size() + 2) { chyb = "rar: spatny kod filtru"; return false; }
      std::vector<uint8_t> vm(velikost);
      for (uint32_t i = 0; i < velikost; i++) { vm[i] = (uint8_t)(v.g() >> 8); v.p(8); }
      typFiltru[pozice] = poznejFiltr(vm);
    }
    f.typ = typFiltru[pozice];
    if (cekajici.size() > 8192) { chyb = "rar: prilis mnoho filtru"; return false; }
    cekajici.push_back(f);
    return true;
  }
  static int poznejFiltr(const std::vector<uint8_t> &vm) {
    uint8_t x = 0;
    for (size_t i = 1; i < vm.size(); i++) x ^= vm[i];
    if (vm.empty() || x != vm[0]) return F_NIC;
    struct S { uint32_t delka, crc; int typ; };
    static const S std_[] = {{53, 0xad576887u, F_E8}, {57, 0x3cd7e57eu, F_E8E9}, {120, 0x3769893fu, F_ITANIUM},
                             {29, 0x0e06077du, F_DELTA}, {149, 0x1c2c5dc8u, F_RGB}, {216, 0xbc85e701u, F_AUDIO}};
    const uint32_t c = crc32(vm.data(), vm.size());
    for (const S &s : std_) if (s.delka == vm.size() && (s.crc == c || s.crc == ~c)) return s.typ;
    return F_NIC;
  }
  bool pouzijFiltry(uint64_t start, std::vector<uint8_t> &out) {
    for (size_t i = 0; i < cekajici.size(); i++) {
      Filtr &f = cekajici[i];
      if (f.start < start || f.start + f.delka > start + out.size()) continue;   // patri jinemu souboru (solid)
      if (f.typ == F_NIC) { chyb = "rar: neznamy filtr (vlastni program VM)"; return false; }
      // vstup filtru = puvodni data okna (ne vystup predchoziho filtru), ale filtry na stejnem
      // bloku se retezi (vystup jednoho = vstup dalsiho)
      std::vector<uint8_t> blok(okno.begin() + (long)f.start, okno.begin() + (long)(f.start + f.delka));
      size_t j = i;
      for (;;) {
        Filtr &g = cekajici[j];
        g.r[6] = (uint32_t)(g.start - start);            // pozice bloku v souboru
        if (!spust(g, blok)) return false;
        if (j + 1 < cekajici.size() && cekajici[j + 1].start == f.start && cekajici[j + 1].delka == f.delka) { j++; continue; }
        break;
      }
      if (!blok.empty()) std::memcpy(out.data() + (size_t)(f.start - start), blok.data(), blok.size());
      i = j;
    }
    // filtry tohoto souboru jsou hotove (u solid zustanou jen ty za jeho koncem)
    std::vector<Filtr> zbytek;
    for (const Filtr &f : cekajici) if (f.start >= start + out.size()) zbytek.push_back(f);
    cekajici.swap(zbytek);
    return true;
  }
  static uint32_t get4(const uint8_t *p) { return le32(p); }
  static void put4(uint8_t *p, uint32_t v) { p[0] = (uint8_t)v; p[1] = (uint8_t)(v >> 8); p[2] = (uint8_t)(v >> 16); p[3] = (uint8_t)(v >> 24); }
  static uint32_t itGet(const uint8_t *d, uint32_t bitpos, uint32_t pocet) {
    const uint32_t i = bitpos / 8, s = bitpos & 7;
    uint32_t v = (uint32_t)d[i] | ((uint32_t)d[i + 1] << 8) | ((uint32_t)d[i + 2] << 16) | ((uint32_t)d[i + 3] << 24);
    return (v >> s) & (0xFFFFFFFFu >> (32 - pocet));
  }
  static void itSet(uint8_t *d, uint32_t hodnota, uint32_t bitpos, uint32_t pocet) {
    const uint32_t i = bitpos / 8, s = bitpos & 7;
    const uint32_t m = ~((0xFFFFFFFFu >> (32 - pocet)) << s);
    hodnota <<= s;
    for (uint32_t k = 0; k < 4; k++) { d[i + k] &= (uint8_t)(m >> (8 * k)); d[i + k] |= (uint8_t)(hodnota >> (8 * k)); }
  }
  bool spust(const Filtr &f, std::vector<uint8_t> &b) {
    const uint32_t n = (uint32_t)b.size();
    switch (f.typ) {
      case F_E8: case F_E8E9: {
        if (n < 4) return true;
        const uint32_t velikostSouboru = 0x1000000;
        const uint8_t cmp2 = f.typ == F_E8E9 ? 0xE9 : 0xE8;
        uint8_t *d = b.data();
        for (uint32_t pos = 0; pos + 4 < n;) {
          const uint8_t c = *d++; pos++;
          if (c == 0xE8 || c == cmp2) {
            const uint32_t off = pos + f.r[6];
            const uint32_t a = get4(d);
            if (a & 0x80000000u) { if (((a + off) & 0x80000000u) == 0) put4(d, a + velikostSouboru); }
            else if (((a - velikostSouboru) & 0x80000000u) != 0) put4(d, a - off);
            d += 4; pos += 4;
          }
        }
        return true;
      }
      case F_ITANIUM: {
        if (n < 21) return true;
        uint8_t *d = b.data();
        uint32_t off = f.r[6] >> 4;
        static const uint8_t masky[16] = {4, 4, 6, 6, 0, 0, 7, 7, 4, 4, 0, 0, 4, 4, 0, 0};
        for (uint32_t pos = 0; pos < n - 21; pos += 16, d += 16, off++) {
          const int bb = (d[0] & 0x1F) - 0x10;
          if (bb < 0) continue;
          const uint8_t m = masky[bb];
          if (!m) continue;
          for (uint32_t i = 0; i <= 2; i++) {
            if (!(m & (1u << i))) continue;
            const uint32_t sp = i * 41 + 5;
            if (itGet(d, sp + 37, 4) == 5) {
              const uint32_t o = itGet(d, sp + 13, 20);
              itSet(d, (o - off) & 0xFFFFF, sp + 13, 20);
            }
          }
        }
        return true;
      }
      case F_DELTA: {
        const uint32_t kanalu = f.r[0];
        if (kanalu == 0 || kanalu > 1024) return false;
        std::vector<uint8_t> o(n);
        uint32_t src = 0;
        for (uint32_t ch = 0; ch < kanalu; ch++) {
          uint8_t prev = 0;
          for (uint32_t dst = ch; dst < n; dst += kanalu) o[dst] = (prev = (uint8_t)(prev - b[src++]));
        }
        b.swap(o);
        return true;
      }
      case F_RGB: {
        const uint32_t sirka = f.r[0] - 3, posR = f.r[1];
        if (n < 3 || sirka > n || posR > 2) return false;
        std::vector<uint8_t> o(n);
        uint32_t src = 0;
        for (uint32_t ch = 0; ch < 3; ch++) {
          uint32_t prev = 0;
          for (uint32_t i = ch; i < n; i += 3) {
            uint32_t pred;
            if (i >= sirka + 3) {
              const uint32_t horni = o[i - sirka], hl = o[i - sirka - 3];
              pred = prev + horni - hl;
              const int pa = std::abs((int)(pred - prev)), pb = std::abs((int)(pred - horni)), pc = std::abs((int)(pred - hl));
              if (pa <= pb && pa <= pc) pred = prev; else if (pb <= pc) pred = horni; else pred = hl;
            } else pred = prev;
            o[i] = (uint8_t)(prev = (uint8_t)(pred - b[src++]));
          }
        }
        for (uint32_t i = posR; i + 2 < n; i += 3) { const uint8_t g = o[i + 1]; o[i] = (uint8_t)(o[i] + g); o[i + 2] = (uint8_t)(o[i + 2] + g); }
        b.swap(o);
        return true;
      }
      case F_AUDIO: {
        const uint32_t kanalu = f.r[0];
        if (kanalu == 0 || kanalu > 128) return false;
        std::vector<uint8_t> o(n);
        uint32_t src = 0;
        for (uint32_t ch = 0; ch < kanalu; ch++) {
          uint32_t prevByte = 0, prevDelta = 0, dif[7] = {0, 0, 0, 0, 0, 0, 0};
          int d1 = 0, d2 = 0, d3, k1 = 0, k2 = 0, k3 = 0;
          for (uint32_t i = ch, cnt = 0; i < n; i += kanalu, cnt++) {
            d3 = d2; d2 = (int)prevDelta - d1; d1 = (int)prevDelta;
            uint32_t pred = 8 * prevByte + (uint32_t)(k1 * d1 + k2 * d2 + k3 * d3);
            pred = (pred >> 3) & 0xFF;
            const uint32_t cur = b[src++];
            pred -= cur;
            o[i] = (uint8_t)pred;
            prevDelta = (uint32_t)(int)(int8_t)(uint8_t)(pred - prevByte);
            prevByte = pred & 0xFF;
            int d = (int)(int8_t)(uint8_t)cur;
            d = (int)((uint32_t)d << 3);
            dif[0] += (uint32_t)std::abs(d); dif[1] += (uint32_t)std::abs(d - d1); dif[2] += (uint32_t)std::abs(d + d1);
            dif[3] += (uint32_t)std::abs(d - d2); dif[4] += (uint32_t)std::abs(d + d2); dif[5] += (uint32_t)std::abs(d - d3); dif[6] += (uint32_t)std::abs(d + d3);
            if ((cnt & 0x1F) == 0) {
              uint32_t minDif = dif[0], numMin = 0; dif[0] = 0;
              for (uint32_t j = 1; j < 7; j++) { if (dif[j] < minDif) { minDif = dif[j]; numMin = j; } dif[j] = 0; }
              switch (numMin) {
                case 1: if (k1 >= -16) k1--; break;
                case 2: if (k1 < 16) k1++; break;
                case 3: if (k2 >= -16) k2--; break;
                case 4: if (k2 < 16) k2++; break;
                case 5: if (k3 >= -16) k3--; break;
                case 6: if (k3 < 16) k3++; break;
              }
            }
          }
        }
        b.swap(o);
        return true;
      }
    }
    return false;
  }
};

// ---------------------------------------------------------------------
//  Archiv: seznam souboru a rozbaleni jednoho z nich
// ---------------------------------------------------------------------
struct Polozka {
  std::string jmeno;           // cesta v archivu (lomitka '/')
  uint64_t velikost = 0;       // rozbalena
  bool adresar = false;
  // vnitrni
  size_t data = 0; uint64_t baleno = 0; int metoda = 0; uint32_t crc = 0; int verze = 0;
  bool solid = false, heslo = false, dily = false;
};

class Archiv {
 public:
  enum Typ { ZADNY = 0, ZIP, RAR4, RAR5 };
  Typ typ = ZADNY;
  std::vector<Polozka> polozky;
  std::string chyba;
  size_t limit = 64u << 20;    // nejvetsi rozbaleny soubor

  static Typ poznej(const uint8_t *d, size_t n) {
    if (n >= 4 && d[0] == 'P' && d[1] == 'K' && ((d[2] == 3 && d[3] == 4) || (d[2] == 5 && d[3] == 6))) return ZIP;
    if (n >= 7 && std::memcmp(d, "Rar!\x1A\x07", 6) == 0) return d[6] == 0 ? RAR4 : (n >= 8 && d[6] == 1 && d[7] == 0 ? RAR5 : ZADNY);
    return ZADNY;
  }
  static const char *nazevTypu(Typ t) { return t == ZIP ? "ZIP" : t == RAR4 ? "RAR" : t == RAR5 ? "RAR5" : "?"; }

  bool otevri(const uint8_t *d, size_t n) {
    buf.assign(d, d + n); polozky.clear(); chyba.clear(); rar.reset(); rarZacatek = rarDalsi = 0;
    typ = poznej(d, n);
    if (typ == ZIP) return otevriZip();
    if (typ == RAR4) return otevriRar4();
    if (typ == RAR5) { chyba = "RAR5 (novy format WinRAR) zatim neumim - zabal hru jako ZIP nebo RAR4"; return false; }
    chyba = "neni to ZIP ani RAR";
    return false;
  }

  bool rozbal(size_t i, std::vector<uint8_t> &out) {
    out.clear(); chyba.clear();
    if (i >= polozky.size()) { chyba = "neni takovy soubor"; return false; }
    const Polozka &p = polozky[i];
    if (p.adresar) { chyba = "je to slozka"; return false; }
    if (p.heslo) { chyba = "soubor je zaheslovany"; return false; }
    if (p.velikost > limit) { chyba = "soubor je moc velky"; return false; }
    bool ok = typ == ZIP ? rozbalZip(p, out) : rozbalRar4(i, out);
    if (!ok) return false;
    if (out.size() != p.velikost) { chyba = "rozbaleno " + std::to_string(out.size()) + " B misto " + std::to_string(p.velikost); return false; }
    const uint32_t c = crc32(out.data(), out.size());
    if (c != p.crc) { chyba = "kontrolni soucet (CRC) nesedi - archiv je poskozeny"; return false; }
    return true;
  }

 private:
  std::vector<uint8_t> buf;
  std::unique_ptr<Rar29> rar; size_t rarZacatek = 0, rarDalsi = 0;   // pevny RAR: retez od rarZacatek, dalsi na rade rarDalsi

  // ---------------- ZIP ----------------
  bool otevriZip() {
    const uint8_t *d = buf.data(); const size_t n = buf.size();
    // konec centralniho adresare (od konce, komentar max 64 kB)
    size_t eocd = SIZE_MAX;
    for (size_t i = n >= 22 ? n - 22 : 0; ; i--) {
      if (n >= 22 && le32(d + i) == 0x06054b50) { eocd = i; break; }
      if (i == 0 || n - i > 65557) break;
    }
    if (eocd == SIZE_MAX) { chyba = "ZIP: chybi seznam souboru (poskozeny nebo nedokonceny ZIP)"; return false; }
    const unsigned pocet = le16(d + eocd + 10);
    size_t p = le32(d + eocd + 16);
    if (p >= n) { chyba = "ZIP: spatny seznam souboru"; return false; }
    for (unsigned k = 0; k < pocet; k++) {
      if (p + 46 > n || le32(d + p) != 0x02014b50) { chyba = "ZIP: spatny zaznam v seznamu"; return false; }
      Polozka it;
      const unsigned flags = le16(d + p + 8);
      it.metoda = le16(d + p + 10);
      it.crc = le32(d + p + 16);
      it.baleno = le32(d + p + 20);
      it.velikost = le32(d + p + 24);
      const unsigned ln = le16(d + p + 28), le = le16(d + p + 30), lc = le16(d + p + 32);
      const size_t lokal = le32(d + p + 42);
      if (p + 46 + ln > n) { chyba = "ZIP: spatne jmeno"; return false; }
      it.jmeno.assign((const char *)d + p + 46, ln);
      for (char &c : it.jmeno) if (c == '\\') c = '/';
      it.adresar = !it.jmeno.empty() && it.jmeno.back() == '/';
      it.heslo = (flags & 1) != 0;
      // ZIP64: velikosti ve vlastnim poli
      if (it.velikost == 0xFFFFFFFFu || it.baleno == 0xFFFFFFFFu) {
        size_t e = p + 46 + ln, ek = e + le;
        while (e + 4 <= ek && ek <= n) {
          const unsigned id = le16(d + e), sz = le16(d + e + 2);
          if (e + 4 + sz > ek) break;                       // pole za koncem rozsireni (poskozeny ZIP)
          if (id == 1) {
            size_t q = e + 4;
            if (it.velikost == 0xFFFFFFFFu && q + 8 <= e + 4 + sz) { it.velikost = le32(d + q) | ((uint64_t)le32(d + q + 4) << 32); q += 8; }
            if (it.baleno == 0xFFFFFFFFu && q + 8 <= e + 4 + sz) { it.baleno = le32(d + q) | ((uint64_t)le32(d + q + 4) << 32); q += 8; }
          }
          e += 4 + sz;
        }
      }
      if (lokal + 30 > n || le32(d + lokal) != 0x04034b50) { chyba = "ZIP: spatna hlavicka souboru " + it.jmeno; return false; }
      it.data = lokal + 30 + le16(d + lokal + 26) + le16(d + lokal + 28);
      polozky.push_back(it);
      p += 46 + ln + le + lc;
    }
    return true;
  }
  bool rozbalZip(const Polozka &p, std::vector<uint8_t> &out) {
    if (p.data > buf.size() || p.baleno > buf.size() - p.data) { chyba = "ZIP: data souboru za koncem archivu"; return false; }
    const uint8_t *d = buf.data() + p.data;
    if (p.metoda == 0) { out.assign(d, d + p.baleno); return true; }
    if (p.metoda == 8) return Inflate::rozbal(d, (size_t)p.baleno, out, (size_t)p.velikost + 1, chyba);
    chyba = "ZIP: metoda " + std::to_string(p.metoda) + " (jen 0 a 8 = deflate)";
    return false;
  }

  // ---------------- RAR 4 ----------------
  bool otevriRar4() {
    const uint8_t *d = buf.data(); const size_t n = buf.size();
    size_t p = 7;
    bool solid = false;
    while (p + 7 <= n) {
      const unsigned typBloku = d[p + 2], fl = le16(d + p + 3), sz = le16(d + p + 5);
      if (sz < 7) { chyba = "RAR: spatna hlavicka"; return false; }
      uint64_t pridano = 0;
      if (typBloku == 0x74 || typBloku == 0x7A || (fl & 0x8000)) { if (p + 11 > n) break; pridano = le32(d + p + 7); }
      if (typBloku == 0x73) {
        if (fl & 0x0080) { chyba = "RAR: zaheslovany archiv (i seznam souboru)"; return false; }
        if (fl & 0x0001) { chyba = "RAR na vice dilu (part1, part2 ...) zatim neumim"; return false; }
        solid = (fl & 0x0008) != 0;
      } else if (typBloku == 0x74) {
        if (p + 32 > n) { chyba = "RAR: zkracena hlavicka souboru"; return false; }
        Polozka it;
        it.baleno = le32(d + p + 7);
        it.velikost = le32(d + p + 11);
        it.crc = le32(d + p + 16);
        it.verze = d[p + 24];
        it.metoda = d[p + 25];
        const unsigned ln = le16(d + p + 26);
        size_t q = p + 32;
        if (fl & 0x100) {                                  // velke soubory
          if (q + 8 > n) break;
          it.baleno |= (uint64_t)le32(d + q) << 32; it.velikost |= (uint64_t)le32(d + q + 4) << 32; q += 8;
          pridano = it.baleno;
        }
        if (q + ln > n) { chyba = "RAR: spatne jmeno souboru"; return false; }
        std::string jm((const char *)d + q, ln);
        const size_t nula = jm.find('\0');                 // Unicode jmeno za nulou - staci ASCII cast
        if (nula != std::string::npos) jm.resize(nula);
        for (char &c : jm) if (c == '\\') c = '/';
        it.jmeno = jm;
        it.adresar = (fl & 0xE0) == 0xE0;
        it.heslo = (fl & 0x04) != 0;
        it.dily = (fl & 0x03) != 0;
        it.solid = solid && (fl & 0x10) != 0;
        it.data = p + sz;
        if (it.data + it.baleno > n) { chyba = "RAR: archiv je zkraceny (" + it.jmeno + ")"; return false; }
        polozky.push_back(it);
      } else if (typBloku == 0x7B) break;
      p += sz + (size_t)pridano;
    }
    if (polozky.empty() && chyba.empty()) chyba = "RAR: zadne soubory";
    return !polozky.empty();
  }
  bool rozbalRar4(size_t i, std::vector<uint8_t> &out) {
    const Polozka &p = polozky[i];
    if (p.dily) { chyba = "RAR: soubor pokracuje v dalsim dilu archivu"; return false; }
    if (p.metoda == 0x30) { out.assign(buf.begin() + (long)p.data, buf.begin() + (long)(p.data + p.baleno)); return true; }
    if (p.verze != 29 && p.verze != 36) {
      chyba = "RAR: stary format komprese (verze " + std::to_string(p.verze) + ") zatim neumim";
      return false;
    }
    // pevny (solid) archiv: soubor navazuje na slovnik predchozich -> rozbalit retez od jeho
    // zacatku (soubor bez priznaku navaznosti); pri postupnem rozbalovani 0, 1, 2 ... se jen pokracuje
    size_t zacatek = i;
    while (zacatek > 0 && polozky[zacatek].solid) zacatek--;
    if (!rar || rarZacatek != zacatek || rarDalsi > i) { rar.reset(new Rar29()); rarZacatek = zacatek; rarDalsi = zacatek; }
    for (; rarDalsi <= i; rarDalsi++) {
      const Polozka &q = polozky[rarDalsi];
      if (q.adresar || q.baleno == 0 || q.metoda == 0x30) continue;   // slozka / prazdny / ulozeny: slovnik beze zmeny
      if (q.heslo || q.dily || (q.verze != 29 && q.verze != 36)) {
        chyba = "RAR: pevny archiv - predchozi soubor " + q.jmeno + " nejde rozbalit";
        rar.reset();
        return false;
      }
      std::vector<uint8_t> o;
      if (!rar->rozbal(buf.data() + q.data, (size_t)q.baleno, q.velikost, rarDalsi != rarZacatek, o, chyba)) { rar.reset(); return false; }
      if (rarDalsi == i) { out.swap(o); rarDalsi++; return true; }
    }
    if (p.baleno == 0) { out.clear(); return true; }
    chyba = "RAR: pevny archiv - soubor nenalezen";
    return false;
  }
};

}  // namespace archiv
}  // namespace nap
