// vyrob_ppmd.cpp CILOVA_SLOZKA - testovaci RAR 4 s bloky PPMd (B351: "komprese textu" WinRARu)
//
// Koder PPMd jen pro testy: model je PRIMO nap::archiv::Ppmd7 z appky (nap_ppmd.h - koder z nej
// dedi), k nemu kodovani symbolu podle Ppmd7_EncodeSymbol (LZMA SDK, public domain) a rozsahovy
// koder RAR (carryless, Subbotin). Bitovy proud bloku PPM presne podle unrar (DecodeInit, Unpack29):
// bajt priznaku (0x80 PPM, 0x20 novy model, 0x40 unikovy znak, rad), MaxMB, unikovy znak, data;
// unikovy znak + 0 = konec bloku, + 1 = unikovy znak jako data, + 2 = konec souboru, + 3 = filtr (kod VM
// v bajtech PPMd), + 4 = shoda LZ, + 5 = RLE. Nezavisla kontrola: bsdtar (libarchive) archivy rozbali na
// puvodni data (../b349/over_bsdtar.sh; pevny archiv bsdtar 3.7 neumi).
// Vedle archivu zapise CILOVA_SLOZKA/obsah/<archiv>/<soubor> s puvodnimi daty.
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o vyrob_ppmd vyrob_ppmd.cpp && ./vyrob_ppmd /tmp/b351_archivy
#include <cstdio>
#include <random>
#include <string>
#include <sys/stat.h>
#include <vector>
#include "nap_archiv.h"

using nap::archiv::crc32;

class KoderPpmd : public nap::archiv::Ppmd7 {
 public:
  std::vector<uint8_t> *ven = nullptr;
  uint32_t eLow = 0, eRange = 0xFFFFFFFFu;

  void zacniKoder(std::vector<uint8_t> *v) { ven = v; eLow = 0; eRange = 0xFFFFFFFFu; spatne = false; }
  void dokonci() { for (int i = 0; i < 4; i++) { ven->push_back((uint8_t)(eLow >> 24)); eLow <<= 8; } }
  void usek(uint32_t start, uint32_t vel, uint32_t celkem) {
    eLow += start * (eRange /= celkem);
    eRange *= vel;
    for (;;) {
      if ((eLow ^ (eLow + eRange)) >= (1u << 24)) {
        if (eRange >= 0x8000) break;
        eRange = (0u - eLow) & (0x8000 - 1);
      }
      ven->push_back((uint8_t)(eLow >> 24));
      eRange <<= 8; eLow <<= 8;
    }
  }
  // Ppmd7_EncodeSymbol (LZMA SDK 9.20 / libarchive) nad modelem appky
  void koduj(int symbol) {
    int8_t maska[256];
    const unsigned ns0 = NumStats(MinContext);
    if (ns0 != 1) {
      uint32_t s = Stats(MinContext);
      if ((int)Sym(s) == symbol) {
        usek(0, Freq(s), SummFreq(MinContext));
        FoundState = s; Update1_0(); return;
      }
      PrevSuccess = 0;
      uint32_t sum = Freq(s);
      unsigned i = ns0 - 1;
      do {
        s += 6;
        if ((int)Sym(s) == symbol) {
          usek(sum, Freq(s), SummFreq(MinContext));
          FoundState = s; Update1(); return;
        }
        sum += Freq(s);
      } while (--i);
      HiBitsFlag = HB2Flag[Sym(FoundState)];
      std::memset(maska, -1, sizeof maska);
      maska[Sym(s)] = 0;
      i = ns0 - 1;
      do { s -= 6; maska[Sym(s)] = 0; } while (--i);
      usek(sum, SummFreq(MinContext) - sum, SummFreq(MinContext));
    } else {
      const uint32_t one = OneState(MinContext);
      HiBitsFlag = HB2Flag[Sym(FoundState)];
      const unsigned idx = PrevSuccess + NS2BSIndx[NumStats(Suffix(MinContext)) - 1] + HiBitsFlag + 2 * HB2Flag[Sym(one)] + (unsigned)((RunLength >> 26) & 0x20);
      uint16_t &prob = BinSumm[Freq(one) - 1][idx];
      if ((int)Sym(one) == symbol) {
        usek(0, prob, BIN_SCALE);
        prob = (uint16_t)(prob + (1 << INT_BITS) - ((prob + (1 << (PERIOD_BITS - 2))) >> PERIOD_BITS));
        FoundState = one; UpdateBin(); return;
      }
      usek(prob, BIN_SCALE - prob, BIN_SCALE);
      prob = (uint16_t)(prob - ((prob + (1 << (PERIOD_BITS - 2))) >> PERIOD_BITS));
      static const uint8_t kExpEscape[16] = {25, 14, 9, 7, 5, 5, 4, 4, 4, 3, 3, 3, 2, 2, 2, 2};
      InitEsc = kExpEscape[prob >> 10];
      std::memset(maska, -1, sizeof maska);
      maska[Sym(one)] = 0;
      PrevSuccess = 0;
    }
    for (;;) {
      const unsigned numMasked = NumStats(MinContext);
      do {
        OrderFall++;
        if (!Suffix(MinContext)) { std::fprintf(stderr, "koder: symbol %d nenalezen\n", symbol); std::exit(1); }
        MinContext = Suffix(MinContext);
      } while (NumStats(MinContext) == numMasked);
      uint32_t escFreq;
      See *see = MakeEscFreq(numMasked, escFreq);
      uint32_t s = Stats(MinContext);
      uint32_t sum = 0;
      unsigned i = NumStats(MinContext);
      do {
        const unsigned cur = Sym(s);
        if ((int)cur == symbol) {
          const uint32_t low = sum, s1 = s;
          do { sum += (Freq(s) & (unsigned)maska[Sym(s)]); s += 6; } while (--i);
          usek(low, Freq(s1), sum + escFreq);
          if (see->Shift < PERIOD_BITS && --see->Count == 0) { see->Summ = (uint16_t)(see->Summ << 1); see->Count = (uint8_t)(3 << see->Shift++); }
          FoundState = s1; Update2(); return;
        }
        sum += (Freq(s) & (unsigned)maska[cur]);
        maska[cur] = 0;
        s += 6;
      } while (--i);
      usek(sum, escFreq, sum + escFreq);
      see->Summ = (uint16_t)(see->Summ + sum + escFreq);
    }
  }
};

// ---------- proud PPM (prikazy) ----------
// typ: 'L' bajt a, 'M' shoda (vzd a, delka b), 'R' RLE (delka b), 'K' konec bloku + nova hlavicka (a = priznaky, b = MaxMB),
// 'F' filtr: esc + 3 + bajty (prvni bajt, delka, kod filtru - unrar ReadVMCodePPM) bez dalsiho kodovani
struct Prikaz { int typ; uint32_t a, b; std::vector<uint8_t> bajty = {}; };
static std::string D;

static void uloz(const std::string &jm, const std::vector<uint8_t> &d) { FILE *f = fopen(jm.c_str(), "wb"); fwrite(d.data(), 1, d.size(), f); fclose(f); }
static void ulozObsah(const std::string &archiv, const std::string &jm, const std::vector<uint8_t> &d) {
  mkdir((D + "/obsah").c_str(), 0755);
  mkdir((D + "/obsah/" + archiv).c_str(), 0755);
  uloz(D + "/obsah/" + archiv + "/" + jm, d);
}

// soubor jako proud PPM: hlavicka (priznaky, MaxMB, esc), prikazy, esc + 2; okno = rozbalena data (vystup)
struct Proud { std::vector<uint8_t> data; std::vector<uint8_t> okno; };
static void hlavicka(KoderPpmd &k, std::vector<uint8_t> &out, uint8_t prizn, uint8_t maxMB, uint8_t &esc, uint8_t noveEsc) {
  out.push_back(prizn);
  if (prizn & 0x20) out.push_back(maxMB);
  if (prizn & 0x40) { out.push_back(noveEsc); esc = noveEsc; }
  if (prizn & 0x20) {
    unsigned rad = (prizn & 0x1F) + 1;
    if (rad > 16) rad = 16 + (rad - 16) * 3;
    if (!k.alokuj(((uint32_t)maxMB + 1) << 20)) { std::fprintf(stderr, "alokace\n"); std::exit(1); }
    k.init(rad);
  }
  k.zacniKoder(&out);
}
static void kodujProud(KoderPpmd &k, Proud &p, uint8_t &esc, const std::vector<Prikaz> &prikazy, bool konecSouboru) {
  for (const Prikaz &c : prikazy) {
    if (c.typ == 'L') {
      k.koduj((int)c.a);
      if (c.a == esc) k.koduj(1);
      p.okno.push_back((uint8_t)c.a);
    } else if (c.typ == 'M') {                        // esc 4: vzdalenost - 2 (3 B), delka - 32 (1 B)
      if (c.b < 32 || c.b > 287 || c.a < 2 || c.a > p.okno.size()) { std::fprintf(stderr, "spatna shoda\n"); std::exit(1); }
      k.koduj(esc); k.koduj(4);
      const uint32_t v = c.a - 2;
      k.koduj((int)((v >> 16) & 0xFF)); k.koduj((int)((v >> 8) & 0xFF)); k.koduj((int)(v & 0xFF)); k.koduj((int)(c.b - 32));
      for (uint32_t i = 0; i < c.b; i++) p.okno.push_back(p.okno[p.okno.size() - c.a]);
    } else if (c.typ == 'R') {                        // esc 5: delka - 4, vzdalenost 1
      k.koduj(esc); k.koduj(5); k.koduj((int)(c.b - 4));
      for (uint32_t i = 0; i < c.b; i++) p.okno.push_back(p.okno.back());
    } else if (c.typ == 'F') {                        // esc 3: filtr - bajty jdou primo (unrar SafePPMDecodeChar)
      k.koduj(esc); k.koduj(3);
      for (uint8_t x : c.bajty) k.koduj(x);
    } else if (c.typ == 'K') {                        // esc 0: konec bloku, nova hlavicka PPM
      k.koduj(esc); k.koduj(0); k.dokonci();
      hlavicka(k, p.data, (uint8_t)c.a, (uint8_t)c.b, esc, (uint8_t)(c.b + 0x30));
    }
  }
  if (konecSouboru) { k.koduj(esc); k.koduj(2); }
  k.dokonci();
}

// ---------- kod filtru RAR 2.9 (bitovy proud, cisla jako RarVM::ReadData) ----------
struct Bity {
  std::vector<uint8_t> v; int n = 0;
  void put(uint32_t x, int k) { for (int i = k - 1; i >= 0; i--) { if (n % 8 == 0) v.push_back(0); if ((x >> i) & 1) v.back() |= (uint8_t)(0x80 >> (n % 8)); n++; } }
  void data(uint32_t x) {                              // 0-15: 00+4 b, 16-255: 01+8 b, do $FFFF: 10+16 b, jinak 11+32 b
    if (x < 16) { put(0, 2); put(x, 4); }
    else if (x < 256) { put(1, 2); put(x, 8); }
    else if (x <= 0xFFFF) { put(2, 2); put(x, 16); }
    else { put(3, 2); put(x >> 16, 16); put(x & 0xFFFF, 16); }
  }
};
// filtr pro esc 3: prvni bajt (0x80 = cislo filtru, 0x20 = delka bloku, 0x10 = registry, bity 0-2 = delka kodu),
// [delka], kod (cislo 0 = nove filtry, start bloku od konce okna, delka bloku, maska registru 0, u noveho filtru
// velikost + bajty programu VM).
// Program VM tu neni standardni (E8 / DELTA ... se poznaji podle CRC skutecneho kodu z WinRARu) = neznamy filtr.
static std::vector<uint8_t> filtr(int prvni, bool novy, uint32_t start, uint32_t delka, uint32_t vm) {
  Bity b;
  if (prvni & 0x80) b.data(0);
  b.data(start);
  if (prvni & 0x20) b.data(delka);
  if (prvni & 0x10) b.put(0, 7);                       // maska 7 registru: zadny
  if (novy) { b.data(vm); for (uint32_t i = 0; i < vm; i++) b.put((i * 37 + 11) & 0xFF, 8); }
  std::vector<uint8_t> o;
  const uint32_t n = (uint32_t)b.v.size();
  const int d = prvni & 7;
  if (d < 6) { if (n != (uint32_t)d + 1) { std::fprintf(stderr, "filtr: kod %u B, prvni bajt chce %d\n", n, d + 1); std::exit(1); } o.push_back((uint8_t)prvni); }
  else if (d == 6) { o.push_back((uint8_t)prvni); o.push_back((uint8_t)(n - 7)); }
  else { o.push_back((uint8_t)prvni); o.push_back((uint8_t)(n >> 8)); o.push_back((uint8_t)n); }
  o.insert(o.end(), b.v.begin(), b.v.end());
  return o;
}

// ---------- RAR 4 ----------
static void p16(std::vector<uint8_t> &v, unsigned x) { v.push_back((uint8_t)x); v.push_back((uint8_t)(x >> 8)); }
static void p32(std::vector<uint8_t> &v, uint32_t x) { p16(v, x & 0xFFFF); p16(v, x >> 16); }
struct Clen { std::string jmeno; std::vector<uint8_t> data, zabaleno; bool ulozeno = false; };
static std::vector<uint8_t> rar4(const std::vector<Clen> &c, bool pevny) {
  std::vector<uint8_t> o = {'R', 'a', 'r', '!', 0x1A, 0x07, 0x00};
  std::vector<uint8_t> h = {0x73}; p16(h, pevny ? 0x0008 : 0); p16(h, 13); p16(h, 0); p32(h, 0);
  p16(o, crc32(h.data(), h.size()) & 0xFFFF); o.insert(o.end(), h.begin(), h.end());
  for (size_t i = 0; i < c.size(); i++) {
    const std::vector<uint8_t> &bal = c[i].ulozeno ? c[i].data : c[i].zabaleno;
    h = {0x74};
    p16(h, 0x8000 | 0x0080 | (pevny && i > 0 ? 0x0010 : 0));
    p16(h, (unsigned)(32 + c[i].jmeno.size()));
    p32(h, (uint32_t)bal.size()); p32(h, (uint32_t)c[i].data.size()); h.push_back(2);
    p32(h, crc32(c[i].data.data(), c[i].data.size())); p32(h, 0x5D726000u);
    h.push_back(c[i].ulozeno ? 20 : 29); h.push_back(c[i].ulozeno ? 0x30 : 0x35);
    p16(h, (unsigned)c[i].jmeno.size()); p32(h, 0x20);
    h.insert(h.end(), c[i].jmeno.begin(), c[i].jmeno.end());
    p16(o, crc32(h.data(), h.size()) & 0xFFFF); o.insert(o.end(), h.begin(), h.end());
    o.insert(o.end(), bal.begin(), bal.end());
  }
  h = {0x7B}; p16(h, 0x4000); p16(h, 7);
  p16(o, crc32(h.data(), h.size()) & 0xFFFF); o.insert(o.end(), h.begin(), h.end());
  return o;
}

// ---------- testovaci data ----------
static std::vector<Prikaz> text(std::mt19937 &rng, size_t kolik) {
  static const char *slova[] = {"atari", "disketa", "hra", "joystick", "prince", "persia", "strana", "kazeta", "basic", "turbo",
                                "pamet", "sektor", "rychle", "pomalu", "nahrava", "zavadec", "obrazovka", "zvuk", "pokey", "antic"};
  std::vector<Prikaz> v;
  size_t n = 0;
  while (n < kolik) {
    std::string w = slova[rng() % 20];
    if (rng() % 7 == 0) w[0] = (char)(w[0] - 32);
    w += (rng() % 11 == 0) ? ".\r\n" : " ";
    for (char ch : w) { v.push_back({'L', (uint8_t)ch, 0}); n++; }
  }
  return v;
}

int main(int argc, char **argv) {
  D = argc > 1 ? argv[1] : "archivy_ppmd";
  mkdir(D.c_str(), 0755);
  std::mt19937 rng(351);
  KoderPpmd k;

  // 1) text 200 kB, rad 6, model 1 MB (oblast textu modelu ~128 kB -> model se cestou restartuje)
  {
    Proud p; uint8_t esc = 2;
    hlavicka(k, p.data, 0x80 | 0x20 | 0x40 | (6 - 1), 0, esc, 0x7E);
    kodujProud(k, p, esc, text(rng, 200000), true);
    uloz(D + "/ppmd_text.rar", rar4({{"text.txt", p.okno, p.data}}, false));
    ulozObsah("ppmd_text.rar", "text.txt", p.okno);
    std::printf("ppmd_text.rar: %zu B textu -> %zu B (rad 6, model 1 MB)\n", p.okno.size(), p.data.size());
  }
  // 2) prikazy: unikovy znak v datech (esc 1), shody LZ (esc 4), RLE (esc 5), konec bloku + nova hlavicka
  //    bez noveho modelu / s novym modelem (rad 64, 4 MB), binarni data vsech hodnot
  {
    Proud p; uint8_t esc = 2;
    hlavicka(k, p.data, 0x80 | 0x20 | (8 - 1), 1, esc, 0);
    std::vector<Prikaz> v = text(rng, 3000);
    for (int i = 0; i < 500; i++) v.push_back({'L', (uint32_t)(rng() % 4 == 0 ? 2 : rng() % 256), 0});   // i unikove znaky (2)
    v.push_back({'M', 1000, 200}); v.push_back({'M', 2, 32}); v.push_back({'M', 3499, 287});
    v.push_back({'R', 0, 4}); v.push_back({'R', 0, 259});
    std::vector<Prikaz> t = text(rng, 2000); v.insert(v.end(), t.begin(), t.end());
    v.push_back({'K', 0x80 | 0x40 | 5, 0});                                    // novy blok, model zustava, esc = 0x30
    t = text(rng, 3000); v.insert(v.end(), t.begin(), t.end());
    for (int i = 0; i < 300; i++) v.push_back({'L', 0x30, 0});                  // novy unikovy znak v datech
    v.push_back({'K', 0x80 | 0x20 | 0x40 | 0x1F, 3});                           // novy model: rad 64, 4 MB, esc = 0x33
    t = text(rng, 5000); v.insert(v.end(), t.begin(), t.end());
    v.push_back({'M', 4000, 255 + 32});
    kodujProud(k, p, esc, v, true);
    uloz(D + "/ppmd_prikazy.rar", rar4({{"prikazy.bin", p.okno, p.data}}, false));
    ulozObsah("ppmd_prikazy.rar", "prikazy.bin", p.okno);
    std::printf("ppmd_prikazy.rar: %zu B -> %zu B (esc 1 / 4 / 5, 2x konec bloku, rad 64)\n", p.okno.size(), p.data.size());
  }
  // 3) pevny archiv: 3 soubory jednim modelem PPMd (dalsi soubor: hlavicka bez noveho modelu)
  {
    std::vector<Clen> c;
    uint8_t esc = 2;
    Proud p1; hlavicka(k, p1.data, 0x80 | 0x20 | (6 - 1), 1, esc, 0); kodujProud(k, p1, esc, text(rng, 30000), true);
    Proud p2; hlavicka(k, p2.data, 0x80 | (6 - 1), 0, esc, 0); kodujProud(k, p2, esc, text(rng, 20000), true);
    Proud p3; hlavicka(k, p3.data, 0x80 | 0x40 | (6 - 1), 0, esc, 0x55); kodujProud(k, p3, esc, text(rng, 10000), true);
    c.push_back({"pevny1.txt", p1.okno, p1.data}); c.push_back({"pevny2.txt", p2.okno, p2.data}); c.push_back({"pevny3.txt", p3.okno, p3.data});
    uloz(D + "/ppmd_pevny.rar", rar4(c, true));
    for (auto &x : c) ulozObsah("ppmd_pevny.rar", x.jmeno, x.data);
    std::printf("ppmd_pevny.rar: 3 soubory (%zu + %zu + %zu B) jednim modelem PPMd\n", p1.okno.size(), p2.okno.size(), p3.okno.size());
  }
  // 4) Atari: pevny archiv "nejlepsi" - readme (PPMd) pred hrou (XEX v PPMd)
  {
    std::vector<uint8_t> xex = {0xFF, 0xFF, 0x00, 0x20, 0xFF, 0x20};
    for (int i = 0; i < 256; i++) xex.push_back((uint8_t)(i * 5 + 3));
    const uint8_t run[] = {0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20};
    xex.insert(xex.end(), run, run + 6);
    std::vector<Prikaz> vx;
    for (uint8_t b : xex) vx.push_back({'L', b, 0});
    uint8_t esc = 2;
    Proud p1; hlavicka(k, p1.data, 0x80 | 0x20 | (4 - 1), 0, esc, 0); kodujProud(k, p1, esc, text(rng, 5000), true);
    Proud p2; hlavicka(k, p2.data, 0x80 | (4 - 1), 0, esc, 0); kodujProud(k, p2, esc, vx, true);
    std::vector<Clen> c = {{"readme.txt", p1.okno, p1.data}, {"hra.xex", p2.okno, p2.data}};
    uloz(D + "/vyber_ppmd.rar", rar4(c, true));
    for (auto &x : c) ulozObsah("vyber_ppmd.rar", x.jmeno, x.data);
    std::printf("vyber_ppmd.rar: pevny RAR, readme (PPMd) pred hrou hra.xex (PPMd)\n");
  }
  // 5) filtry v PPMd (esc 3, unrar ReadVMCodePPM): vsechny 3 tvary delky kodu (1-6 / 7 + bajt / 2 bajty);
  //    a) filtry s blokem az za koncem souboru = nepouziji se, soubor se rozbali (CRC sedi) - overi, ze esc 3
  //       spotrebuje presne svoje bajty; b) neznamy filtr uvnitr souboru = appka ho odmitne ("neznamy filtr")
  {
    uint8_t esc = 2;
    Proud p1; hlavicka(k, p1.data, 0x80 | 0x20 | (6 - 1), 0, esc, 0);
    std::vector<Prikaz> v = text(rng, 1000);
    v.push_back({'F', 0, 0, filtr(0x80 | 0x10 | 5, true, 50000, 0, 1)});         // kod 6 B (delka v prvnim bajtu)
    std::vector<Prikaz> t = text(rng, 1000); v.insert(v.end(), t.begin(), t.end());
    v.push_back({'F', 0, 0, filtr(0x80 | 0x20 | 6, true, 60000, 500, 10)});      // kod 16 B (7 + bajt)
    v.push_back({'F', 0, 0, filtr(0x20 | 7, false, 55000, 300, 0)});             // kod 5 B (2 bajty), stary filtr c. 0
    t = text(rng, 1000); v.insert(v.end(), t.begin(), t.end());
    kodujProud(k, p1, esc, v, true);
    Proud p2; hlavicka(k, p2.data, 0x80 | 0x20 | (6 - 1), 0, esc, 0);
    v = text(rng, 500);
    v.push_back({'F', 0, 0, filtr(0x80 | 0x20 | 6, true, 10, 100, 10)});         // blok uvnitr souboru
    t = text(rng, 2000); v.insert(v.end(), t.begin(), t.end());
    kodujProud(k, p2, esc, v, true);
    std::vector<Clen> c = {{"filtr_za_koncem.bin", p1.okno, p1.data}, {"filtr_neznamy.bin", p2.okno, p2.data}};
    uloz(D + "/ppmd_filtr.rar", rar4(c, false));
    for (auto &x : c) ulozObsah("ppmd_filtr.rar", x.jmeno, x.data);
    std::printf("ppmd_filtr.rar: esc 3 - 3 filtry za koncem souboru (kod 6 / 16 / 5 B), neznamy filtr uvnitr souboru\n");
  }
  std::printf("hotovo: 5 archivu RAR s PPMd v %s\n", D.c_str());
  return 0;
}
