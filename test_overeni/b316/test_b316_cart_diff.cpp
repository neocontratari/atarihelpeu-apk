// =====================================================================
//  B316 - CARTRIDGE: nase nap::Cartridge proti emulatoru Atari800
//
//  Porovnava pristup po pristupu SKUTECNY kod appky
//  (app/src/main/cpp/atari/nap_atari_cart.h) se src/cartridge.c emulatoru
//  Atari800 (commit 98cc2dc749867c9c7317a0271ba062a6c797f886). Zdrojaky
//  Atari800 (GPL) se do repozitare neukladaji - vse.sh je stahne pri testu.
//  Pocitac pro cartridge.c = nahrady v ref_stub/ (130XE, BASIC vestaveny):
//  test vidi, ktera okna cartridge drzi (RD4 $8000-$9FFF, RD5 $A000-$BFFF)
//  a co v nich je.
//
//  1) tabulka typu: velikost a nazev vsech 161 typu jako v Atari800
//  2) nacteni .car: pro kazdy typ 0-170 (+ spatny soucet, kratky soubor,
//     soubor bez hlavicky) - Atari800 CARTRIDGE_ReadImage vs nase nactiCar
//  3) pro kazdy typ, ktery appka umi: nahodne data, vlozeni, studeny start
//     a pak tisice nahodnych cteni / zapisu $D500-$D5FF (u Bounty Bob i
//     $8FF6-$8FF9 / $9FF6-$9FF9) a obcas studeny start. Po KAZDEM kroku se
//     porovna: zapnuta okna (RD4, RD5 = TRIG3) a vsech 16 kB obsahu oken,
//     plus hodnota prectena z $D5xx.
//  4) volitelne skutecny soubor .car (napr. Reneho Wonderboy_VBXE.car):
//     ./test_b316_cart_diff --car soubor.car
//
//  Preklad (vse.sh):
//   gcc -std=gnu99 -O2 -w -I<atari800/src> -Iref_stub -c cartridge.c cartridge_info.c
//   gcc -std=gnu99 -O2 -Iref_stub -c ref_stub/ref_stub.c
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -I<atari800/src> -Iref_stub
//       test_b316_cart_diff.cpp cartridge.o cartridge_info.o ref_stub.o
// =====================================================================
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "nap_atari_cart.h"

extern "C" {
#include "atari.h"
#include "memory.h"
#include "cartridge.h"
extern int REF_coldstartu, REF_zmen_stroje;
}

static int g_chyb = 0;
static std::string g_prac = ".";

static void chyba(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void chyba(const char *fmt, ...) {
  g_chyb++;
  if (g_chyb > 60) return;
  va_list ap;
  va_start(ap, fmt);
  std::printf("CHYBA  ");
  std::vprintf(fmt, ap);
  std::printf("\n");
  va_end(ap);
}

static std::vector<uint8_t> carSoubor(uint32_t typ, const std::vector<uint8_t> &data, bool spatnySoucet) {
  std::vector<uint8_t> f(16 + data.size());
  f[0] = 'C'; f[1] = 'A'; f[2] = 'R'; f[3] = 'T';
  f[4] = (uint8_t)(typ >> 24); f[5] = (uint8_t)(typ >> 16); f[6] = (uint8_t)(typ >> 8); f[7] = (uint8_t)typ;
  uint32_t s = 0;
  for (uint8_t b : data) s += b;
  if (spatnySoucet) s ^= 0x5A5A;
  f[8] = (uint8_t)(s >> 24); f[9] = (uint8_t)(s >> 16); f[10] = (uint8_t)(s >> 8); f[11] = (uint8_t)s;
  if (!data.empty()) std::memcpy(&f[16], data.data(), data.size());
  return f;
}

static std::string ulozit(const std::vector<uint8_t> &f, const char *jm) {
  const std::string p = g_prac + "/" + jm;
  FILE *fp = std::fopen(p.c_str(), "wb");
  if (!fp) { std::printf("nejde zapsat %s\n", p.c_str()); std::exit(2); }
  if (!f.empty()) std::fwrite(f.data(), 1, f.size(), fp);
  std::fclose(fp);
  return p;
}

// ---------------------------------------------------------------------
// 1) tabulka typu
// ---------------------------------------------------------------------
static void testTabulka() {
  int spatne = 0;
  for (int t = 0; t < CARTRIDGE_TYPE_COUNT; t++) {
    const int kbRef = CARTRIDGES[t].kb;
    if (nap::cartKb(t) != kbRef) { spatne++; chyba("typ %d: velikost %d kB, Atari800 %d kB", t, nap::cartKb(t), kbRef); }
    if (t >= 1 && kbRef > 0) {
      std::string jm = CARTRIDGES[t].description ? CARTRIDGES[t].description : "";
      const std::string k = " cartridge";
      const size_t p = jm.find(k);
      if (p != std::string::npos) jm.erase(p, k.size());
      if (jm != nap::cartJmeno(t)) { spatne++; chyba("typ %d: nazev '%s', Atari800 '%s'", t, nap::cartJmeno(t), jm.c_str()); }
    }
  }
  for (int t = CARTRIDGE_TYPE_COUNT; t < CARTRIDGE_TYPE_COUNT + 40; t++)
    if (nap::cartKb(t) != 0) { spatne++; chyba("typ %d mimo tabulku Atari800 ma velikost %d", t, nap::cartKb(t)); }
  std::printf("1) TABULKA TYPU: %d typu (0-%d) - velikost a nazev %s\n", CARTRIDGE_TYPE_COUNT, CARTRIDGE_TYPE_COUNT - 1,
              spatne ? "NESEDI" : "sedi u vsech");
}

// ---------------------------------------------------------------------
// 2) nacteni .car
// ---------------------------------------------------------------------
static const char *jmenoChyby(nap::Cartridge::Chyba c) {
  switch (c) {
    case nap::Cartridge::OK: return "OK";
    case nap::Cartridge::KRATKY: return "KRATKY";
    case nap::Cartridge::NENI_CAR: return "NENI_CAR";
    case nap::Cartridge::NEZNAMY_TYP: return "NEZNAMY_TYP";
    case nap::Cartridge::PRO_5200: return "PRO_5200";
    case nap::Cartridge::PRAVY_SLOT: return "PRAVY_SLOT";
    case nap::Cartridge::NEUMIM: return "NEUMIM";
    case nap::Cartridge::MALO_DAT: return "MALO_DAT";
  }
  return "?";
}

// ZAMERNA ODCHYLKA: typy 113-158 nejsou v Atari800 obsazene (velikost 0 kB).
// Atari800 takovou hlavicku vezme jako cartridge s 0 B dat (precte 0 B, soucet
// nesedi, cartridge pak nic nedela). My poctive rekneme "neznamy typ".
static int g_prazdnyTyp = 0;
static bool prazdnyTypAtari800(int t) { return t >= 1 && t < CARTRIDGE_TYPE_COUNT && CARTRIDGES[t].kb == 0; }

// co ma nase nactiCar vratit, kdyz Atari800 CARTRIDGE_ReadImage vrati ref (a typ t)
static bool ocekavanaShoda(int ref, int t, nap::Cartridge::Chyba moje) {
  if (prazdnyTypAtari800(t) && (ref == 0 || ref == CARTRIDGE_BAD_CHECKSUM)) {
    if (moje != nap::Cartridge::NEZNAMY_TYP) return false;
    g_prazdnyTyp++;
    return true;
  }
  if (ref == 0 || ref == CARTRIDGE_BAD_CHECKSUM) {
    // Atari800 soubor nacte - my bud taky, nebo poctive reknem proc ne
    if (nap::cartPro5200(t)) return moje == nap::Cartridge::PRO_5200;     // 130XE nema slot 5200
    if (nap::cartPravySlot(t)) return moje == nap::Cartridge::PRAVY_SLOT; // 130XE nema pravy slot
    if (!nap::cartUmim(t)) return moje == nap::Cartridge::NEUMIM;
    return moje == nap::Cartridge::OK;
  }
  if (ref == CARTRIDGE_BAD_FORMAT) return moje == nap::Cartridge::NENI_CAR || moje == nap::Cartridge::NEZNAMY_TYP || moje == nap::Cartridge::KRATKY;
  if (ref == CARTRIDGE_TOO_FEW_DATA) {
    // Atari800 testuje delku az po typu; my nejdriv rekneme, ze typ neumime / je pro 5200
    if (nap::cartPro5200(t)) return moje == nap::Cartridge::PRO_5200;
    if (nap::cartPravySlot(t)) return moje == nap::Cartridge::PRAVY_SLOT;
    if (!nap::cartUmim(t)) return moje == nap::Cartridge::NEUMIM;
    return moje == nap::Cartridge::MALO_DAT;
  }
  return false;
}

static void testNacteni() {
  std::mt19937 rng(316);
  int pokusu = 0, umime = 0, odmitnuto = 0;
  std::map<std::string, int> duvody;
  for (int t = 0; t <= 170; t++) {
    const int kb = (t < CARTRIDGE_TYPE_COUNT) ? CARTRIDGES[t].kb : 0;
    const size_t n = (size_t)(kb ? kb : 8) * 1024;
    if (n > 4u * 1024 * 1024) {
      // The!Cart 32-128 MB: jen hlavicka (data by byla zbytecne velka) - musi byt NEUMIM pred ctenim dat
      std::vector<uint8_t> f = carSoubor((uint32_t)t, std::vector<uint8_t>(16), false);
      nap::Cartridge c;
      const auto r = c.nactiCar(f.data(), f.size(), "x.car");
      pokusu++;
      if (r != nap::Cartridge::NEUMIM) chyba("typ %d (%d kB): cekal jsem NEUMIM, je %s", t, kb, jmenoChyby(r));
      else { odmitnuto++; duvody[jmenoChyby(r)]++; }
      continue;
    }
    std::vector<uint8_t> data(n);
    for (auto &b : data) b = (uint8_t)rng();
    for (int varianta = 0; varianta < 4; varianta++) {
      // 0 = spravny soubor, 1 = spatny kontrolni soucet, 2 = o 1 B kratsi data, 3 = 100 B navic za daty
      std::vector<uint8_t> f = carSoubor((uint32_t)t, data, varianta == 1);
      if (varianta == 2) f.pop_back();
      if (varianta == 3) f.insert(f.end(), 100, 0x55);
      const std::string p = ulozit(f, "nacteni.car");
      CARTRIDGE_image_t img;
      std::memset(&img, 0, sizeof img);
      const int ref = CARTRIDGE_ReadImage(p.c_str(), &img);
      if (img.image) { std::free(img.image); img.image = nullptr; }
      nap::Cartridge c;
      const auto moje = c.nactiCar(f.data(), f.size(), "x.car");
      pokusu++;
      if (!ocekavanaShoda(ref, t, moje))
        chyba("nacteni typ %d varianta %d: Atari800 %d (typ %d), my %s", t, varianta, ref, img.type, jmenoChyby(moje));
      if (moje == nap::Cartridge::OK) {
        umime++;
        const bool soucetSedi = c.soucetHlavicka == c.soucetDat;
        if (soucetSedi != (ref == 0)) chyba("typ %d varianta %d: kontrolni soucet my %s, Atari800 %s", t, varianta,
                                            soucetSedi ? "sedi" : "nesedi", ref == 0 ? "sedi" : "nesedi");
        if (c.typ != img.type) chyba("typ %d: my typ %d, Atari800 %d", t, c.typ, img.type);
        if (varianta == 3 && c.prebytek(f.size()) != 100) chyba("typ %d: prebytek %zu misto 100", t, c.prebytek(f.size()));
      } else {
        odmitnuto++;
        duvody[jmenoChyby(moje)]++;
      }
    }
  }
  // soubory bez hlavicky CART / kratke
  {
    const uint8_t xex[] = {0xFF, 0xFF, 0x00, 0x20, 0x0F, 0x20, 0xA9, 0x00, 0x8D, 0xC6, 0x02, 0x60, 0, 0, 0, 0, 0, 0, 0, 0, 0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20};
    nap::Cartridge c;
    if (c.nactiCar(xex, sizeof xex, "a.xex") != nap::Cartridge::NENI_CAR) chyba("XEX neni .car - cekal jsem NENI_CAR");
    if (c.nactiCar(xex, 3, "a.car") != nap::Cartridge::KRATKY) chyba("3 B - cekal jsem KRATKY");
    if (c.nactiCar(nullptr, 0, "a.car") != nap::Cartridge::KRATKY) chyba("prazdny soubor - cekal jsem KRATKY");
    std::vector<uint8_t> f = carSoubor(0x7FFFFFFFu, std::vector<uint8_t>(8192), false);
    if (c.nactiCar(f.data(), f.size(), "a.car") != nap::Cartridge::NEZNAMY_TYP) chyba("typ $7FFFFFFF - cekal jsem NEZNAMY_TYP");
    pokusu += 4;
  }
  std::printf("2) NACTENI .CAR: %d souboru (typy 0-170, spravne / spatny soucet / o 1 B kratsi / s bajty navic, XEX, prazdny) - "
              "nacteno %d, odmitnuto %d (", pokusu, umime, odmitnuto);
  bool prvni = true;
  for (auto &d : duvody) { std::printf("%s%s %d", prvni ? "" : ", ", d.first.c_str(), d.second); prvni = false; }
  std::printf(") - shodne s Atari800 %s\n", g_chyb ? "NE" : "ve vsech pripadech");
  std::printf("   zamerna odchylka: %d souboru s typem 113-158 (v Atari800 neobsazene, 0 kB) - Atari800 je vezme jako "
              "prazdnou cartridge, ktera nic nedela, my rekneme NEZNAMY TYP\n", g_prazdnyTyp);
}

// ---------------------------------------------------------------------
// 3) prepinani bank pristup po pristupu
// ---------------------------------------------------------------------
struct Vysledek { long long kroku = 0, cteniD5 = 0, zapisuD5 = 0, bb = 0, studenych = 0; size_t mapovani = 0; int chyb = 0; long long zmen = 0; };

// porovna, co vidi procesor: okna + obsah + TRIG3
static bool porovnej(const nap::Cartridge &c, int typ, long long krok, const char *co, int *chybTypu) {
  const int ref[4] = {REF_cart809f, REF_cart809f, REF_cartA0bf, REF_cartA0bf};
  for (int i = 0; i < 4; i++) {
    const bool moje = c.okno[i] != nap::Cartridge::NIC;
    const uint16_t base = (uint16_t)(0x8000 + i * 0x1000);
    if (moje != (ref[i] != 0)) {
      (*chybTypu)++;
      chyba("typ %d krok %lld (%s): okno $%04X my %s, Atari800 %s (stav $%X)", typ, krok, co, base,
            moje ? "ZAPNUTO" : "vypnuto", ref[i] ? "ZAPNUTO" : "vypnuto", (unsigned)c.stav);
      return false;
    }
    if (!moje) continue;
    bool sedi;
    if (c.okno[i] >= 0) sedi = std::memcmp(&c.data[(size_t)c.okno[i]], &MEMORY_mem[base], 0x1000) == 0;
    else { sedi = true; for (int k = 0; k < 0x1000; k++) if (MEMORY_mem[base + k] != 0xFF) { sedi = false; break; } }
    if (!sedi) {
      (*chybTypu)++;
      chyba("typ %d krok %lld (%s): obsah okna $%04X nesedi (stav $%X, posun $%X)", typ, krok, co, base, (unsigned)c.stav, (unsigned)c.okno[i]);
      return false;
    }
  }
  if (c.rd5() != (REF_cartA0bf != 0)) { (*chybTypu)++; chyba("typ %d krok %lld: TRIG3 (RD5) nesedi", typ, krok); return false; }
  return true;
}

static uint16_t nahodnaD5(std::mt19937 &rng) {
  static const uint8_t zajimave[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0f, 0x10, 0x11,
                                      0x17, 0x1f, 0x20, 0x3f, 0x40, 0x70, 0x71, 0x77, 0x78, 0x7f, 0x80, 0x81, 0x8f, 0x9f, 0xa0, 0xbf,
                                      0xd0, 0xd7, 0xd8, 0xdf, 0xe0, 0xe7, 0xe8, 0xec, 0xef, 0xf0, 0xf7, 0xf8, 0xfc, 0xff};
  const unsigned r = rng() % 100;
  if (r < 50) return (uint16_t)(0xD500 | (rng() & 0xff));
  return (uint16_t)(0xD500 | zajimave[rng() % sizeof zajimave]);
}
static uint8_t nahodnaHodnota(std::mt19937 &rng) {
  const unsigned r = rng() % 100;
  if (r < 50) return (uint8_t)rng();
  if (r < 75) return (uint8_t)(rng() % 0x20);
  return (uint8_t)(0x80 | (rng() % 0x10));
}

static Vysledek testTyp(int typ, const std::vector<uint8_t> *skutecna, long long kroku, unsigned seed) {
  Vysledek v;
  std::mt19937 rng(seed);
  const size_t n = (size_t)nap::cartKb(typ) * 1024;
  std::vector<uint8_t> data;
  if (skutecna) data = *skutecna;
  else {
    data.resize(n);
    // kazda 4 kB stranka jina (prvni bajty = cislo stranky), aby zamena bank nesla prehlednout
    for (size_t i = 0; i < n; i++) data[i] = (uint8_t)rng();
    for (size_t p = 0; p < n; p += 0x1000) { data[p] = (uint8_t)(p >> 12); data[p + 1] = (uint8_t)(p >> 20); }
  }
  std::vector<uint8_t> f = carSoubor((uint32_t)typ, data, false);
  const std::string p = ulozit(f, "typ.car");
  std::memset(MEMORY_mem, 0x00, sizeof MEMORY_mem);
  REF_cart809f = REF_cartA0bf = 0;
  const int r = CARTRIDGE_Insert(p.c_str());
  if (r != 0) { chyba("typ %d: Atari800 CARTRIDGE_Insert vratil %d", typ, r); v.chyb++; return v; }
  CARTRIDGE_ColdStart();
  nap::Cartridge c;
  const auto e = c.nactiCar(f.data(), f.size(), "typ.car");
  if (e != nap::Cartridge::OK) { chyba("typ %d: nactiCar %s", typ, jmenoChyby(e)); v.chyb++; return v; }
  c.studenyStart();
  std::set<std::vector<int32_t>> mapovani;
  auto zaznam = [&]() { mapovani.insert(std::vector<int32_t>(c.okno, c.okno + 4)); };
  if (!porovnej(c, typ, 0, "po zapnuti", &v.chyb)) return v;
  zaznam();
  for (long long k = 1; k <= kroku; k++) {
    const unsigned r100 = rng() % 1000;
    const char *co;
    if (r100 < 4) {                                    // studeny start (POWER)
      CARTRIDGE_ColdStart();
      c.studenyStart();
      v.studenych++;
      co = "studeny start";
    } else if (typ == 18 && r100 < 450) {              // Bounty Bob: $8FF6-$8FF9 / $9FF6-$9FF9
      const uint16_t a = (uint16_t)(((rng() & 1) ? 0x9FF6 : 0x8FF6) + (rng() % 4));
      const bool cteni = rng() & 1;
      v.bb++;
      if (cteni) {
        const UBYTE ref = (a < 0x9000) ? CARTRIDGE_BountyBob1GetByte(a, FALSE) : CARTRIDGE_BountyBob2GetByte(a, FALSE);
        c.bbPristup(a);
        const uint8_t moje = c.cti(a);
        if (moje != ref) { v.chyb++; chyba("typ 18 krok %lld: cteni $%04X my $%02X, Atari800 $%02X", k, a, moje, ref); }
        co = "Bounty Bob cteni";
      } else {
        if (a < 0x9000) CARTRIDGE_BountyBob1PutByte(a, 0); else CARTRIDGE_BountyBob2PutByte(a, 0);
        c.bbPristup(a);
        co = "Bounty Bob zapis";
      }
    } else if (r100 < 500) {                           // cteni $D5xx
      const uint16_t a = nahodnaD5(rng);
      const UBYTE ref = CARTRIDGE_GetByte(a, FALSE);
      const int moje = c.cteniD5(a);
      v.cteniD5++;
      // moje -1 = cartridge na sbernici nic nedava (u nas plovouci sbernice, Atari800 vraci $FF)
      if (moje >= 0 ? (moje != ref) : (ref != 0xFF)) { v.chyb++; chyba("typ %d krok %lld: cteni $%04X my %d, Atari800 $%02X", typ, k, a, moje, ref); }
      co = "cteni $D5xx";
    } else {                                           // zapis $D5xx
      const uint16_t a = nahodnaD5(rng);
      const uint8_t h = nahodnaHodnota(rng);
      CARTRIDGE_PutByte(a, h);
      c.zapisD5(a, h);
      v.zapisuD5++;
      co = "zapis $D5xx";
    }
    v.kroku++;
    if (!porovnej(c, typ, k, co, &v.chyb)) return v;
    zaznam();
  }
  v.mapovani = mapovani.size();
  v.zmen = c.zmen;
  if (REF_zmen_stroje) { chyba("typ %d: Atari800 prepnul typ stroje (5200?)", typ); v.chyb++; }
  return v;
}

int main(int argc, char **argv) {
  std::string car;
  long long kroku = 6000;
  for (int i = 1; i < argc; i++) {
    if (!std::strcmp(argv[i], "--car") && i + 1 < argc) car = argv[++i];
    else if (!std::strcmp(argv[i], "--kroku") && i + 1 < argc) kroku = std::atoll(argv[++i]);
    else if (!std::strcmp(argv[i], "--prac") && i + 1 < argc) g_prac = argv[++i];
  }
  std::printf("B316 CARTRIDGE - nap::Cartridge (appka) proti src/cartridge.c emulatoru Atari800 (commit 98cc2dc)\n");
  std::printf("pocitac: 130XE (XL/XE), BASIC vestaveny; porovnava se po KAZDEM kroku: okna RD4/RD5 (= TRIG3), 16 kB obsahu, cteni $D5xx\n\n");
  testTabulka();
  testNacteni();

  std::printf("\n3) PREPINANI BANK (%lld nahodnych kroku na typ: cteni / zapis $D500-$D5FF, Bounty Bob $8FF6-$9FF9, obcas POWER)\n", kroku);
  std::printf("   typ  nazev                                kB   cteni  zapisu   BB  POWER  ruznych mapovani  zmen   vysledek\n");
  int typu = 0, typuOk = 0;
  long long celkem = 0;
  for (int t = 1; t <= 112; t++) {
    if (!nap::cartUmim(t)) continue;
    const int pred = g_chyb;
    const Vysledek v = testTyp(t, nullptr, kroku, 1000u + (unsigned)t);
    typu++;
    celkem += v.kroku;
    const bool ok = v.chyb == 0 && g_chyb == pred;
    if (ok) typuOk++;
    std::printf("   %3d  %-35s %5d  %6lld  %6lld  %4lld  %4lld  %8zu          %6lld   %s\n", t, nap::cartJmeno(t), nap::cartKb(t),
                v.cteniD5, v.zapisuD5, v.bb, v.studenych, v.mapovani, v.zmen, ok ? "SHODA" : "CHYBA");
  }
  std::printf("   => %d typu, %lld kroku, shodne %d / %d\n", typu, celkem, typuOk, typu);

  if (!car.empty()) {
    FILE *fp = std::fopen(car.c_str(), "rb");
    if (!fp) { std::printf("\n4) %s nejde otevrit\n", car.c_str()); return 2; }
    std::vector<uint8_t> f;
    uint8_t b[65536];
    size_t k;
    while ((k = std::fread(b, 1, sizeof b, fp)) > 0) f.insert(f.end(), b, b + k);
    std::fclose(fp);
    nap::Cartridge c;
    const auto e = c.nactiCar(f.data(), f.size(), "skutecna.car");
    std::printf("\n4) SKUTECNY SOUBOR %s (%zu B): %s", car.c_str(), f.size(), jmenoChyby(e));
    if (e == nap::Cartridge::OK) {
      std::printf(", typ %d (%s), %d kB, kontrolni soucet %s ($%08X)\n", c.typ, nap::cartJmeno(c.typ), nap::cartKb(c.typ),
                  c.soucetHlavicka == c.soucetDat ? "sedi" : "NESEDI", (unsigned)c.soucetDat);
      const std::vector<uint8_t> data(f.begin() + 16, f.begin() + 16 + (long)nap::cartKb(c.typ) * 1024);
      const int pred = g_chyb;
      const Vysledek v = testTyp(c.typ, &data, kroku * 5, 4242u);
      std::printf("   %lld kroku (cteni %lld, zapisu %lld, POWER %lld), %zu ruznych mapovani - %s\n", v.kroku, v.cteniD5, v.zapisuD5,
                  v.studenych, v.mapovani, (v.chyb == 0 && g_chyb == pred) ? "SHODA s Atari800" : "CHYBA");
    } else {
      std::printf("\n");
      g_chyb++;
    }
  }
  std::printf("\nVYSLEDEK: %d CHYBA\n", g_chyb);
  return g_chyb ? 1 : 0;
}
