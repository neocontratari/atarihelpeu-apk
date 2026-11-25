// test_overeni/b363/test_b363_stav.cpp
// B363: ULOZENI A NAHRANI CELEHO STAVU ATARI 130XE (nap_atari_stav.h + nap_atari_online.h) - bez Androidu.
// Pro online hru dvou telefonu (start ze stavu, synchronizace, oprava odhadu) musi platit:
//   stroj A bezi; v snimku S se jeho stav (i s cartridge, disketou a zavadecem XEX) ulozi a nahraje do UPLNE JINEHO
//   stroje B (jina historie, jina pamet, jine nastaveni). Pak oba dostavaji stejne stisky (atariOnlineVstupy - presne
//   jako za online hry) a KAZDY snimek musi byt stejny: obraz (fb + barevne kody), zvuk (udalosti urovni POKEY 1 a 2
//   a kazetoveho vystupu - to, z ceho se pocita zvuk) a otisk stavu po castech.
//   Navic oprava odhadu: A si ulozi stav (bez cartridge a diskety, jako kruh za hry), spocita 8 snimku s jinymi
//   stisky (i jinymi daty zapsanymi na disketu), vrati se a spocita je znovu se spravnymi - musi byt stejny jako B.
// Programy: BASIC (program napsany stisky klaves, SOUND / RND / STICK / STRIG), Turbo-BASIC XL z XEX (zavadec v INIT -
// ve stavu), zatezovy program (POKEY 2 stereo, VBXE MEMAC + blitter + paleta, 16 bank pameti; i v 1088K), program
// s vlastnim SIO pres POKEY (cteni i ZAPIS sektoru na disketu D1:) a volitelne hry z prikazove radky (XEX / CAR / ATR).
// Navic: stav po zapnuti nezavisi na zbytku haldy (operator new vyplni pamet vzorem - nalezeno: konce bufferu radku
// ANTIC / GTIA a ramce SIO nikdo nezapsal, kazdy telefon mel jiny otisk; od B363 nulovane v nap_atari_machine.h).
// Preklad (z test_overeni/b363):
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b363_stav test_b363_stav.cpp -lz
//   ./test_b363_stav [hra.xex|hra.car|hra.atr ...] [1088:hra.xex]        -> VYSTUP_test_b363_stav.txt
// Prenos mezi 64bitovym a 32bitovym telefonem (arm64 <-> armeabi-v7a, napr. telefon a TV): stav ulozeny 64bitovou
// appkou musi 32bitova nahrat a pocitat dal stejne (a naopak) - format stavu nesmi zaviset na velikosti long / size_t:
//   g++ -m32 -msse2 -mfpmath=sse -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b363_stav32 test_b363_stav.cpp -lz
//   ./test_b363_stav --prenos-uloz p64.bin && ./test_b363_stav32 --prenos-over p64.bin     (a naopak)
//   (-mfpmath=sse: na PC 32 bitu jinak pocita x87 s 80bitovymi mezivysledky; ARM pocita double vzdy presne IEEE)
#include <cstdio>
#include <functional>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
#include "nap_atari_cart.h"
#include "nap_atari_stav.h"
#include "nap_atari_online.h"
#include "../b348/test_b348_program.h"
#include "test_b363_stres.h"
#include <new>
using namespace nap;

// Halda vyplnena vzorem: kazdy telefon ma v nove pameti jiny "zbytek" - stav po zapnuti na nem nesmi zaviset
// (pole, ktera emulace nikdy nezapise, ale jsou ve stavu: lisil by se otisk telefonu, ktere maji stejnou hru).
static unsigned char g_vzorHaldy = 0xCD;
void *operator new(std::size_t n) {
  void *p = std::malloc(n ? n : 1);
  if (!p) throw std::bad_alloc();
  std::memset(p, g_vzorHaldy, n);
  return p;
}
#if defined(__GNUC__) && !defined(__clang__)
#pragma GCC diagnostic ignored "-Wmismatched-new-delete"   // new i delete jsou tady oba nase (malloc / free)
#endif
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, std::size_t) noexcept { std::free(p); }

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::vector<uint8_t> nacti(const std::string &f) {
  std::vector<uint8_t> d;
  FILE *fp = std::fopen(f.c_str(), "rb");
  if (!fp) return d;
  uint8_t b[65536];
  size_t n;
  while ((n = std::fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n);
  std::fclose(fp);
  return d;
}
static std::string jmenoSouboru(const std::string &c) { const size_t s = c.find_last_of('/'); return s == std::string::npos ? c : c.substr(s + 1); }

// ------------------------------------------------------------------------------------------- telefon (stroj)
struct Tel {
  std::unique_ptr<Machine> m;
  std::unique_ptr<AnticView> v;
  XexLoader xl;
  AtariOnlineKontext k;
  float zl[882], zr[882];
  Tel() { k.xex = &xl; }
};
// jako appka (zaloz + studenyStartLocked): novy stroj, ROM, obraz, RESET, konzole
static void zapni(Tel &t, bool r1088, int consol) {
  t.m.reset(new Machine());
  if (!t.v) t.v.reset(new AnticView());
  t.m->mem.os = NAP_OS_ROM;
  t.m->mem.bas = NAP_BASIC_ROM;
  t.m->view = t.v.get();
  t.m->tapeCapture = true;
  t.m->mem.rezim1088 = r1088;
  t.m->reset();
  t.m->consol = consol & 7;
}

static uint64_t hashUdalosti(const std::vector<std::pair<uint64_t, float>> &e, uint64_t h) {
  for (const auto &x : e) {
    uint32_t f;
    std::memcpy(&f, &x.second, 4);
    h = atariOtisk64(&x.first, 8, h);
    h = atariOtisk64(&f, 4, h);
  }
  return h;
}
struct Vysledek { uint64_t obraz = 0, zvuk = 0, stav = 0; uint64_t casti[ATARI_STAV_CASTI] = {}; };

// jeden snimek jako za online hry: stisky obou -> snimek -> (zvuk) -> zavadec XEX; vysledek pro porovnani
static Vysledek snimek(Tel &t, uint32_t p1, uint32_t p2, bool vstupy, bool otisk) {
  Machine &m = *t.m;
  if (vstupy) atariOnlineVstupy(m, p1, p2, t.k.obaNaJ1);
  else if (!t.xl.aktivni()) m.consol = 7;
  m.runFrame();
  Vysledek r;
  r.zvuk = hashUdalosti(m.audEv, 1469598103934665603ULL);
  r.zvuk = hashUdalosti(m.audEv2, r.zvuk);
  r.zvuk = hashUdalosti(m.tapeEv, r.zvuk);
  m.genAudioStereo(t.zl, t.zr, 882, 44100.0);
  m.genTape(t.zl, 882);
  t.xl.poSnimku(m);
  t.xl.log.clear();
  if (m.mem.chce1088) m.mem.chce1088 = false;   // za online hry appka stroj nerestartuje (oba telefony stejne)
  r.obraz = atariOtisk64(t.v->fb, sizeof t.v->fb, 7);
  r.obraz = atariOtisk64(t.v->idx, sizeof t.v->idx, r.obraz);
  if (otisk) r.stav = atariOnlineOtisk(m, t.k, r.casti);
  return r;
}

// stisky hrace pro snimek f (deterministicke, stejne pro oba stroje): joystick + FIRE se meni po 6 snimcich,
// obcas START / SELECT / OPTION, obcas klavesa (drzena 3 snimky), BREAK jednou za cas
static uint32_t mix(uint32_t x) { x ^= x >> 16; x *= 0x7feb352dU; x ^= x >> 15; x *= 0x846ca68bU; x ^= x >> 16; return x; }
static uint32_t stisky(uint32_t seed, uint32_t f, int hrac, bool konzole, bool klavesy) {
  const uint32_t h = mix(seed * 1000003u + (f / 6) * 7919u + (uint32_t)hrac * 104729u);
  uint32_t b = h & 0x1F;
  if ((b & 3) == 3) b &= ~2u;            // nahoru i dolu naraz ne
  if ((b & 12) == 12) b &= ~8u;
  if (konzole && ((f / 6) % 29) == 7) b |= AV_SELECT;
  if (konzole && ((f / 6) % 41) == 11) b |= AV_OPTION;
  if (klavesy && (f % 50) < 3) {
    static const int KL[6] = {0x21, 0x0C, 0x3F, 0x15, 0x12, 0x3A};   // mezera, RETURN, A, B, C, D
    b |= AV_KLAVESA | ((uint32_t)KL[(f / 50 + (uint32_t)hrac) % 6] << AV_KOD_POSUN);
  }
  if (klavesy && f % 397 == 200) b |= AV_BREAK;
  return b;
}

// psani textu stisky klaves (jako hrac na klavesnici pristroje): klavesa drzena 4 snimky, pak 5 pustena, RETURN 12
struct Psani {
  std::vector<std::pair<int, int>> plan;   // (od snimku, kod) - kod -1 = nic
  void text(const std::string &s, int od) {
    int f = od;
    for (char c : s) {
      const int sc = c == '\n' ? 12 : asciiNaScan((unsigned char)c);
      if (sc < 0) continue;
      plan.push_back({f, sc});
      plan.push_back({f + 4, -1});
      f += c == '\n' ? 16 : 9;
    }
  }
  int konec() const { return plan.empty() ? 0 : plan.back().first; }
  uint32_t bity(int f) const {
    int k = -1;
    for (const auto &p : plan) { if (p.first <= f) k = p.second; else break; }
    return k >= 0 ? (AV_KLAVESA | ((uint32_t)k << AV_KOD_POSUN)) : 0;
  }
};

// ------------------------------------------------------------------------------------------- scenar
struct Scenar {
  std::string nazev;
  int predem = 300;        // snimku stroje A pred ulozenim
  int potom = 400;         // snimku obou po nahrani
  uint32_t seed = 1;
  bool konzole = true, klavesy = true, obaNaJ1 = false;
  int prenosu = 12;        // kolikrat prenest A do jineho stroje (kazdych 29 snimku)
  std::function<void(Tel &)> priprav;              // nahrat program do A (studeny start apod.)
  std::function<void(Tel &, int)> predSnimkem;     // A pred snimkem f (napr. psani, GO pro SIO program)
  std::function<uint32_t(int)> psani;              // stisky klaves misto nahodnych (BASIC)
  std::function<void(Tel &)> odbocka;              // oprava: co A udela jinak pri spatnem odhadu (krome stisku)
  std::function<std::string(Tel &)> popis;
};

static std::vector<uint8_t> xexStres();
static void spustXex(Tel &t, const std::vector<uint8_t> &d, const std::string &jm, bool r1088);

// "Spinavy" cilovy stroj: jiny program (zatez: VBXE, POKEY 2, banky, hraci a kolize) s jinymi stisky - vse, co stav
// NEULOZI, ma pak v cili jinou hodnotu nez ve zdroji (zapomenute pole = rozdil v nasledujicich snimcich).
static void zaspin(Tel &t, uint32_t seed, bool r1088) {
  spustXex(t, xexStres(), "spina.xex", r1088);
  t.m->stereoPokey = (seed & 1) != 0;
  t.k.obaNaJ1 = (seed & 2) != 0;
  const int n = 40 + (int)(seed % 23);
  for (int i = 0; i < n; i++) {
    const bool zavadi = t.xl.aktivni();
    snimek(t, stisky(seed, (uint32_t)i, 1, true, true), stisky(seed + 9, (uint32_t)i, 2, true, true), !zavadi, false);
  }
  std::vector<uint8_t> d(16 + 720 * 128, (uint8_t)seed);
  d[0] = 0x96; d[1] = 0x02; d[2] = (720 * 8) & 0xFF; d[3] = ((720 * 8) >> 8) & 0xFF; d[4] = 128;
  t.m->disk.load(d.data(), d.size(), "spina.atr");
}

static void spust(const Scenar &s) {
  std::printf("== %s\n", s.nazev.c_str());
  Tel A;
  A.k.obaNaJ1 = s.obaNaJ1;
  s.priprav(A);
  auto vstup = [&](int f, int hrac) -> uint32_t {
    uint32_t b = stisky(s.seed, (uint32_t)f, hrac, s.konzole, s.klavesy && !s.psani);
    if (s.psani && hrac == 1) { b &= ~(AV_KLAVESA | 0xFF00u); b |= s.psani(f); }
    return b;
  };
  int f = 0;
  for (; f < s.predem; f++) {
    if (s.predSnimkem) s.predSnimkem(A, f);
    const bool zavadi = A.xl.aktivni();
    snimek(A, vstup(f, 1), vstup(f, 2), !zavadi, false);
  }
  // ---- ulozit A (s mediem) a nahrat do jineho stroje B
  std::vector<uint8_t> surova, zabaleno;
  const bool zab = atariOnlineZabal(*A.m, A.k, surova, zabaleno);
  Tel B;
  zapni(B, !A.m->mem.rezim1088, 7);                 // B: opacny rezim pameti, jina historie
  B.m->stereoPokey = !A.m->stereoPokey;
  for (int i = 0; i < 77; i++) snimek(B, (uint32_t)i * 37u, 0, true, false);
  {   // B ma jinou disketu a cartridge, nez A (start ze stavu je musi prepsat)
    std::vector<uint8_t> d(16 + 720 * 128, 0x5A);
    d[0] = 0x96; d[1] = 0x02; d[2] = (720 * 8) & 0xFF; d[3] = ((720 * 8) >> 8) & 0xFF; d[4] = 128;
    B.m->disk.load(d.data(), d.size(), "jina.atr");
  }
  std::vector<uint8_t> surovaB;
  const bool nahr = zab && atariOnlineRozbal(*B.m, B.k, zabaleno, surovaB);
  over(zab && nahr, s.nazev + ": stav ulozen v snimku " + std::to_string(f) + " (" + std::to_string(surova.size() / 1024) + " KB, zlib " +
                         std::to_string(zabaleno.size() / 1024) + " KB) a nahran do jineho stroje" + (s.popis ? " - " + s.popis(A) : ""));
  if (!nahr) return;
  {
    uint64_t ca[ATARI_STAV_CASTI], cb[ATARI_STAV_CASTI];
    const uint64_t ha = atariOnlineOtisk(*A.m, A.k, ca), hb = atariOnlineOtisk(*B.m, B.k, cb);
    std::string ruzne;
    for (int i = 0; i < ATARI_STAV_CASTI; i++) if (ca[i] != cb[i]) ruzne += std::string(" ") + atariStavNazvyCasti()[i];
    over(ha == hb, s.nazev + ": hned po nahrani stejny otisk" + (ruzne.empty() ? "" : " - LISI SE:" + ruzne));
  }
  // ---- oba dal se stejnymi stisky; uprostred oprava odhadu na A
  AtariOnlineKruh<10> kruh;
  const int oprava = s.potom / 2;
  int prvniRozdil = -1, rozdilObraz = -1, rozdilZvuk = -1;
  std::string castiRozdilu;
  int oprav = 0;
  for (int i = 0; i < s.potom; i++, f++) {
    if (i == oprava) {
      // A: ulozit (jako pred snimkem s odhadem), 8 snimku jinak, zpet, znovu spravne - B nic z toho nedela
      kruh.uloz(*A.m, A.k, (uint32_t)f);
      if (s.odbocka) s.odbocka(A);
      const long long zapisuPred = A.m->disk.writes;
      const uint64_t diskPred = atariOtisk64(A.m->disk.data.data(), A.m->disk.data.size(), 1);
      for (int j = 0; j < 8; j++) snimek(A, vstup(f + j, 1) ^ 0x1Fu, (vstup(f + j, 2) ^ 0x1Bu) | AV_START, true, false);
      const long long vOdbocce = A.m->disk.writes - zapisuPred;
      const bool diskJiny = atariOtisk64(A.m->disk.data.data(), A.m->disk.data.size(), 1) != diskPred;
      const bool vr = kruh.vrat(*A.m, A.k, (uint32_t)f);
      const bool diskZpet = atariOtisk64(A.m->disk.data.data(), A.m->disk.data.size(), 1) == diskPred && A.m->disk.writes == zapisuPred;
      over(vr && diskZpet, s.nazev + ": oprava - stav pred snimkem " + std::to_string(f) + " vracen (kruh " + std::to_string(kruh.bajtu() / 1024) + " KB)" +
                               (vOdbocce > 0 ? ", v 8 snimcich se spatnym odhadem zapsano na disketu " + std::to_string(vOdbocce) + "x" +
                                                   (diskJiny ? " (jina data)" : "") + " - disketa vracena" : ""));
      oprav++;
    }
    if (s.predSnimkem) s.predSnimkem(A, f), s.predSnimkem(B, f);
    const Vysledek ra = snimek(A, vstup(f, 1), vstup(f, 2), true, true);
    const Vysledek rb = snimek(B, vstup(f, 1), vstup(f, 2), true, true);
    if (ra.stav != rb.stav && prvniRozdil < 0) {
      prvniRozdil = f;
      for (int c = 0; c < ATARI_STAV_CASTI; c++) if (ra.casti[c] != rb.casti[c]) castiRozdilu += std::string(" ") + atariStavNazvyCasti()[c];
    }
    if (ra.obraz != rb.obraz && rozdilObraz < 0) rozdilObraz = f;
    if (ra.zvuk != rb.zvuk && rozdilZvuk < 0) rozdilZvuk = f;
  }
  over(prvniRozdil < 0 && rozdilObraz < 0 && rozdilZvuk < 0,
       s.nazev + ": " + std::to_string(s.potom) + " snimku po nahrani (s " + std::to_string(oprav) + " opravou odhadu) - otisk stavu, obraz a zvuk " +
           (prvniRozdil < 0 && rozdilObraz < 0 && rozdilZvuk < 0 ? std::string("KAZDY SNIMEK STEJNE")
                                                                 : "LISI SE: stav od " + std::to_string(prvniRozdil) + castiRozdilu + ", obraz od " +
                                                                       std::to_string(rozdilObraz) + ", zvuk od " + std::to_string(rozdilZvuk)) +
           (s.popis ? " | " + s.popis(A) : ""));
  // ---- opakovane: A -> nove "spinavy" stroj kazdych 29 snimku (ruzne okamziky: radek, instrukce, DMA, SIO, blitter ...)
  int prenosu = 0, spatne = -1;
  std::string spatneCasti;
  for (int j = 0; j < s.prenosu && spatne < 0; j++) {
    Tel C;
    zaspin(C, s.seed * 31u + (uint32_t)j, (j & 1) != 0);
    std::vector<uint8_t> sz, z, sc2;
    if (!atariOnlineZabal(*A.m, A.k, sz, z) || !atariOnlineRozbal(*C.m, C.k, z, sc2)) { spatne = f; spatneCasti = " (nenahrano)"; break; }
    prenosu++;
    for (int i = 0; i < 29; i++, f++) {
      if (s.predSnimkem) s.predSnimkem(A, f), s.predSnimkem(C, f);
      const Vysledek ra = snimek(A, vstup(f, 1), vstup(f, 2), true, true);
      const Vysledek rc = snimek(C, vstup(f, 1), vstup(f, 2), true, true);
      if ((ra.stav != rc.stav || ra.obraz != rc.obraz || ra.zvuk != rc.zvuk) && spatne < 0) {
        spatne = f;
        for (int c = 0; c < ATARI_STAV_CASTI; c++) if (ra.casti[c] != rc.casti[c]) spatneCasti += std::string(" ") + atariStavNazvyCasti()[c];
        if (ra.obraz != rc.obraz) spatneCasti += " (obraz)";
        if (ra.zvuk != rc.zvuk) spatneCasti += " (zvuk)";
      }
    }
  }
  if (s.prenosu > 0)
    over(spatne < 0, s.nazev + ": " + std::to_string(prenosu) + "x prenos do jineho (spinaveho) stroje, pokazde 29 snimku stejne" +
                         (spatne < 0 ? std::string() : " - LISI SE ve snimku " + std::to_string(spatne) + ":" + spatneCasti));
}

// ------------------------------------------------------------------------------------------- programy
static std::vector<uint8_t> xexStres() {
  std::vector<uint8_t> x = {0xFF, 0xFF, (uint8_t)(B363S_ORG & 0xFF), (uint8_t)(B363S_ORG >> 8)};
  const int konec = B363S_ORG + (int)sizeof(B363S_KOD) - 1;
  x.push_back((uint8_t)(konec & 0xFF)); x.push_back((uint8_t)(konec >> 8));
  x.insert(x.end(), B363S_KOD, B363S_KOD + sizeof(B363S_KOD));
  const uint8_t run[6] = {0xE0, 0x02, 0xE1, 0x02, (uint8_t)(B363S_START & 0xFF), (uint8_t)(B363S_START >> 8)};
  x.insert(x.end(), run, run + 6);
  return x;
}
static void spustXex(Tel &t, const std::vector<uint8_t> &d, const std::string &jm, bool r1088) {
  t.xl.priprav(d.data(), d.size(), jm);
  zapni(t, r1088, 3);                    // jako devLoadXexNative: OPTION drzene = BASIC vypnuty
  t.m->sioRychlyTimeout = true;
}
static uint8_t obsahSektoru(int s, int i) { return (uint8_t)(s * 7 + i * 13 + (s >> 8) * 5 + 1); }
static std::vector<uint8_t> atr720(bool boot) {
  std::vector<uint8_t> d(16 + 720 * 128, 0);
  const size_t par = 720 * 128 / 16;
  d[0] = 0x96; d[1] = 0x02; d[2] = par & 0xFF; d[3] = (par >> 8) & 0xFF; d[4] = 128;
  for (int s = 1; s <= 720; s++) for (int i = 0; i < 128; i++) d[16 + (s - 1) * 128 + i] = obsahSektoru(s, i);
  if (boot) for (size_t i = 0; i < sizeof(B348_KOD); i++) d[16 + i] = B348_KOD[i];
  return d;
}

// ---- prenos stavu mezi 64 a 32 bity: uloz = stav ve snimku predem + vysledky dalsich snimku; over = nahrat a porovnat
static void wu32(FILE *f, uint32_t v) { std::fwrite(&v, 4, 1, f); }
static void wu64(FILE *f, uint64_t v) { std::fwrite(&v, 8, 1, f); }
static bool ru32(FILE *f, uint32_t &v) { return std::fread(&v, 4, 1, f) == 1; }
static bool ru64(FILE *f, uint64_t &v) { return std::fread(&v, 8, 1, f) == 1; }
static void prenos(const Scenar &s, FILE *fu, FILE *fo) {
  auto vstup = [&](int f, int hrac) -> uint32_t {
    uint32_t b = stisky(s.seed, (uint32_t)f, hrac, s.konzole, s.klavesy && !s.psani);
    if (s.psani && hrac == 1) { b &= ~(AV_KLAVESA | 0xFF00u); b |= s.psani(f); }
    return b;
  };
  const char *bity = sizeof(void *) == 8 ? "64 bitu" : "32 bitu";
  if (fu) {
    Tel A;
    A.k.obaNaJ1 = s.obaNaJ1;
    s.priprav(A);
    int f = 0;
    for (; f < s.predem; f++) {
      if (s.predSnimkem) s.predSnimkem(A, f);
      const bool zavadi = A.xl.aktivni();
      snimek(A, vstup(f, 1), vstup(f, 2), !zavadi, false);
    }
    std::vector<uint8_t> surova, z;
    const bool ok = atariOnlineZabal(*A.m, A.k, surova, z);
    wu32(fu, (uint32_t)s.nazev.size()); std::fwrite(s.nazev.data(), 1, s.nazev.size(), fu);
    wu32(fu, (uint32_t)f); wu32(fu, (uint32_t)z.size()); std::fwrite(z.data(), 1, z.size(), fu);
    wu32(fu, (uint32_t)s.potom);
    for (int i = 0; i < s.potom; i++, f++) {
      if (s.predSnimkem) s.predSnimkem(A, f);
      const Vysledek r = snimek(A, vstup(f, 1), vstup(f, 2), true, true);
      wu64(fu, r.obraz); wu64(fu, r.zvuk); wu64(fu, r.stav);
    }
    over(ok, s.nazev + ": stav ulozen (" + bity + ", " + std::to_string(z.size() / 1024) + " KB zlib) + " + std::to_string(s.potom) + " snimku vysledku");
    return;
  }
  uint32_t n = 0, f0 = 0, zn = 0, potom = 0;
  std::string jm;
  std::vector<uint8_t> z;
  if (!ru32(fo, n) || n > 1000) { over(false, s.nazev + ": v souboru prenosu nic neni"); return; }
  jm.resize(n);
  if (std::fread(&jm[0], 1, n, fo) != n || !ru32(fo, f0) || !ru32(fo, zn) || zn > (64u << 20)) { over(false, s.nazev + ": spatny soubor prenosu"); return; }
  z.resize(zn);
  if (std::fread(z.data(), 1, zn, fo) != zn || !ru32(fo, potom)) { over(false, s.nazev + ": spatny soubor prenosu"); return; }
  std::vector<uint64_t> ocek((size_t)potom * 3);
  for (auto &x : ocek) if (!ru64(fo, x)) { over(false, s.nazev + ": useknuty soubor prenosu"); return; }
  if (jm != s.nazev) { over(false, "prenos: jiny scenar (" + jm + " misto " + s.nazev + ")"); return; }
  Tel B;
  zapni(B, false, 7);
  for (int i = 0; i < 33; i++) snimek(B, (uint32_t)i * 5u, 0, true, false);
  std::vector<uint8_t> sb;
  if (!atariOnlineRozbal(*B.m, B.k, z, sb)) { over(false, s.nazev + ": stav z druhe architektury NEJDE nahrat (" + bity + ")"); return; }
  int f = (int)f0, rozdil = -1;
  std::string co;
  for (uint32_t i = 0; i < potom; i++, f++) {
    if (s.predSnimkem) s.predSnimkem(B, f);
    const Vysledek r = snimek(B, vstup(f, 1), vstup(f, 2), true, true);
    if (rozdil < 0 && (r.obraz != ocek[i * 3] || r.zvuk != ocek[i * 3 + 1] || r.stav != ocek[i * 3 + 2])) {
      rozdil = f;
      co = std::string(r.obraz != ocek[i * 3] ? " obraz" : "") + (r.zvuk != ocek[i * 3 + 1] ? " zvuk" : "") + (r.stav != ocek[i * 3 + 2] ? " stav" : "");
    }
  }
  over(rozdil < 0, s.nazev + ": stav z druhe architektury nahran (" + bity + "), " + std::to_string(potom) + " snimku: obraz, zvuk a otisk stavu " +
                       (rozdil < 0 ? std::string("KAZDY SNIMEK STEJNE") : "LISI SE od snimku " + std::to_string(rozdil) + ":" + co));
  // a vlastni stroj od zapnuti (bez prenosu): stejne vysledky jako druha architektura = emulace nezavisi na 32 / 64 bitech
  Tel A;
  A.k.obaNaJ1 = s.obaNaJ1;
  s.priprav(A);
  int g = 0;
  for (; g < (int)f0; g++) {
    if (s.predSnimkem) s.predSnimkem(A, g);
    const bool zavadi = A.xl.aktivni();
    snimek(A, vstup(g, 1), vstup(g, 2), !zavadi, false);
  }
  int rozdilA = -1;
  for (uint32_t i = 0; i < potom; i++, g++) {
    if (s.predSnimkem) s.predSnimkem(A, g);
    const Vysledek r = snimek(A, vstup(g, 1), vstup(g, 2), true, true);
    if (rozdilA < 0 && (r.obraz != ocek[i * 3] || r.zvuk != ocek[i * 3 + 1] || r.stav != ocek[i * 3 + 2])) rozdilA = g;
  }
  over(rozdilA < 0, s.nazev + ": i vlastni stroj od zapnuti (" + bity + ") pocita stejne jako druha architektura" +
                        (rozdilA < 0 ? std::string() : " - LISI SE od snimku " + std::to_string(rozdilA)));
}

int main(int argc, char **argv) {
  std::printf("B363 - ulozeni a nahrani celeho stavu Atari 130XE (online hra: start ze stavu, synchronizace, oprava odhadu)\n");
  FILE *prenosU = nullptr, *prenosO = nullptr;
  if (argc == 3 && std::string(argv[1]) == "--prenos-uloz") prenosU = std::fopen(argv[2], "wb");
  if (argc == 3 && std::string(argv[1]) == "--prenos-over") prenosO = std::fopen(argv[2], "rb");
  const bool prenosRezim = argc >= 2 && std::string(argv[1]).rfind("--prenos", 0) == 0;
  if (prenosRezim && !prenosU && !prenosO) { std::printf("CHYBA soubor prenosu nejde otevrit\n"); return 1; }
  if (prenosRezim) { argc = 1; std::printf("PRENOS STAVU (%s): %s\n", sizeof(void *) == 8 ? "64 bitu" : "32 bitu", prenosU ? "ukladam" : "nahravam a porovnavam"); }
  std::vector<Scenar> sc;
  {   // 1) BASIC: program napsany klavesami, pak RUN (SOUND, RND, STICK, STRIG, SETCOLOR)
    Scenar s;
    s.nazev = "BASIC (program napsany klavesami)";
    auto ps = std::make_shared<Psani>();
    ps->text("10 SOUND 0,RND(0)*255,10,8:SETCOLOR 2,STICK(0),4\n20 ? STICK(0);STRIG(0);PEEK(53770):GOTO 10\nRUN\n", 160);
    s.predem = ps->konec() + 60;
    s.potom = 500;
    s.klavesy = false;
    s.konzole = false;
    s.seed = 11;
    s.priprav = [](Tel &t) { zapni(t, false, 7); };
    s.psani = [ps](int f) { return ps->bity(f); };
    s.popis = [](Tel &t) { return std::string("radku programu ") + std::to_string(napRadkyProgramu(*t.m, nullptr)); };
    sc.push_back(s);
  }
  {   // 2) Turbo-BASIC XL z XEX: INIT prevezme rizeni - zavadec XEX je ve stavu (INIT_DLOUHO)
    const std::vector<uint8_t> tbxl = nacti("../../app/src/main/assets/emu_atari_cpp/turbo_basic_xl.xex");
    if (!tbxl.empty()) {
      Scenar s;
      s.nazev = "Turbo-BASIC XL (XEX, zavadec ve stavu)";
      s.predem = 620;
      s.potom = 400;
      s.seed = 22;
      s.konzole = false;
      s.priprav = [tbxl](Tel &t) { spustXex(t, tbxl, "turbo_basic_xl.xex", false); };
      s.popis = [](Tel &t) { return std::string("zavadec XEX ve stavu ") + std::to_string((int)t.xl.stav) + (t.xl.stav == XexLoader::INIT_DLOUHO ? " (INIT bezi - po navratu nahraje zbytek)" : ""); };
      sc.push_back(s);
    } else over(false, "turbo_basic_xl.xex z assets nejde nacist");
  }
  for (int r = 0; r < 2; r++) {   // 3) zatezovy program: POKEY 2, VBXE, rozsirena pamet (320K i 1088K)
    Scenar s;
    s.nazev = std::string("zatez POKEY 2 + VBXE + banky ") + (r ? "1088K" : "320K") + (r ? ", oba hraci na joysticku 1" : "");
    s.predem = 300;
    s.potom = 400;
    s.seed = 33 + r;
    s.obaNaJ1 = r == 1;
    const bool r1088 = r == 1;
    s.priprav = [r1088](Tel &t) { spustXex(t, xexStres(), "b363_stres.xex", r1088); t.m->stereoPokey = true; };
    s.popis = [](Tel &t) {
      char b[200];
      std::snprintf(b, sizeof b, "VBXE zapisu %lld, blitru %lld, POKEY 2 %s, $0605=%02X $0606=%02X", t.m->vbx.nWrites, t.m->vbx.nBlitLists,
                    t.m->pk2.slyset ? "hraje" : "ticho", t.m->mem.ram[0x605], t.m->mem.ram[0x606]);
      return std::string(b);
    };
    sc.push_back(s);
  }
  {   // 4) vlastni SIO pres POKEY (B348): cteni a ZAPIS sektoru na disketu D1: - oprava vraci i data diskety
    Scenar s;
    s.nazev = "disketa D1: - vlastni SIO, zapis sektoru (kopie diskety pri oprave)";
    s.predem = 150;
    s.potom = 700;
    s.seed = 44;
    s.konzole = false;
    s.klavesy = false;
    s.priprav = [](Tel &t) {
      zapni(t, false, 3);
      const std::vector<uint8_t> d = atr720(true);
      t.m->disk.load(d.data(), d.size(), "b363_sio.atr");
    };
    // program nabootuje a ceka na GO; GO az 2 snimky pred opravou (snimek 498): zapis sektoru pak bezi prave v 8 snimcich
    // spocitanych se spatnym odhadem (jina data na diskete) - po navratu musi byt disketa zase jako u B
    s.predSnimkem = [](Tel &t, int f) {
      Machine &m = *t.m;
      if (f < 50) m.consol = 3;
      if (f == 498 && m.mem.ram[0x6FE] == 0xA5 && m.mem.ram[0x6FF] == 0) {
        const uint8_t tab[5][8] = {{0x31, 'P', 10, 0, 128, 1, 0x34, 0}, {0x31, 'R', 10, 0, 128, 2, 0x35, 0}, {0x31, 'P', 11, 0, 128, 1, 0x34, 0},
                                   {0x31, 'S', 0, 0, 4, 2, 0x32, 0}, {0, 0, 0, 0, 0, 0, 0, 0}};
        for (int k = 0; k < 5; k++) for (int i = 0; i < 8; i++) m.mem.ram[B348_TAB + k * 8 + i] = tab[k][i];
        m.mem.ram[0x6FF] = 1;
      }
    };
    // spatny odhad: program zapise jina data (jine VZOR) - po oprave musi byt na diskete spravna
    s.odbocka = [](Tel &t) { for (int i = 0; i < 256; i++) t.m->mem.ram[0x3400 + i] ^= 0xA5; };
    s.popis = [](Tel &t) {
      return "zapisu na disketu " + std::to_string(t.m->disk.writes) + ", prikazu SIO pres POKEY " + std::to_string(t.m->sio.prikazu) +
             ", DONE=" + std::to_string(t.m->mem.ram[0x6F0]);
    };
    sc.push_back(s);
  }
  // 5) volitelne: hry z prikazove radky (jen na PC - hry nejsou v repozitari)
  for (int a = 1; a < argc; a++) {
    std::string cesta = argv[a];
    bool r1088 = false;
    if (cesta.rfind("1088:", 0) == 0) { r1088 = true; cesta = cesta.substr(5); }
    const std::vector<uint8_t> d = nacti(cesta);
    if (d.empty()) { over(false, "nejde nacist " + jmenoSouboru(cesta)); continue; }
    const std::string jm = jmenoSouboru(cesta);
    std::string pr = jm.size() > 4 ? jm.substr(jm.size() - 4) : jm;
    for (char &c : pr) c = (char)std::tolower((unsigned char)c);
    Scenar s;
    s.nazev = "hra " + jm + (r1088 ? " (1088K)" : "");
    s.predem = 900;
    s.potom = 600;
    s.seed = 100 + (uint32_t)a;
    s.klavesy = false;
    if (pr == ".car") {
      s.priprav = [d, jm](Tel &t) {
        zapni(t, false, 7);
        const Cartridge::Chyba ch = t.m->cart.nactiCar(d.data(), d.size(), jm);
        if (ch != Cartridge::OK) std::printf("   cartridge %s: chyba %d\n", jm.c_str(), (int)ch);
        t.m->reset();
      };
    } else if (pr == ".atr") {
      s.priprav = [d, jm, r1088](Tel &t) { zapni(t, r1088, 3); t.m->disk.load(d.data(), d.size(), jm); };
      s.predSnimkem = [](Tel &t, int f) { if (f < 50) t.m->consol = 3; };
    } else {
      s.priprav = [d, jm, r1088](Tel &t) { spustXex(t, d, jm, r1088); };
    }
    s.popis = [](Tel &t) {
      char b[200];
      std::snprintf(b, sizeof b, "PC=$%04X, VBXE zapisu %lld, banky %d, cart typ %d banka %d, D1: %s", t.m->cpu.pc, t.m->vbx.nWrites,
                    t.m->mem.bankAktivnich(), t.m->cart.typ, t.m->cart.stav, t.m->disk.mounted ? "ano" : "ne");
      return std::string(b);
    };
    sc.push_back(s);
  }
  if (prenosRezim) {
    for (const Scenar &s : sc) prenos(s, prenosU, prenosO);
    std::fclose(prenosU ? prenosU : prenosO);
    std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
    return g_chyb ? 1 : 0;
  }
  for (const Scenar &s : sc) spust(s);

  // ---- poskozena / cizi data se nenahraji
  {
    Tel A, B;
    zapni(A, false, 7);
    zapni(B, false, 7);
    for (int i = 0; i < 100; i++) snimek(A, 0, 0, true, false);
    std::vector<uint8_t> s1, z1, s2;
    atariOnlineZabal(*A.m, A.k, s1, z1);
    std::vector<uint8_t> zk = z1;
    zk[zk.size() / 2] ^= 0x40;
    const bool poskozena = atariOnlineRozbal(*B.m, B.k, zk, s2);
    std::vector<uint8_t> v = s1;
    v[4] ^= 0x01;   // jina verze
    const bool verze = atariOnlineNahraj(*B.m, B.k, v.data(), v.size());
    std::vector<uint8_t> kr(s1.begin(), s1.begin() + (long)(s1.size() - 7));
    const bool kratka = atariOnlineNahraj(*B.m, B.k, kr.data(), kr.size());
    over(!poskozena && !verze && !kratka, "poskozena data (zlib), jina verze stavu a useknuta data se NENAHRAJI");
  }
  // ---- stav po zapnuti nezavisi na zbytku haldy (dva telefony se stejnou hrou = stejny otisk)
  for (int k = 0; k < 3; k++) {
    const char *jm[3] = {"BASIC", "zatez 1088K", "disketa (SIO)"};
    std::vector<uint8_t> st[2];
    uint64_t ca[2][ATARI_STAV_CASTI];
    for (int i = 0; i < 2; i++) {
      g_vzorHaldy = i ? 0xA5 : 0x00;
      std::unique_ptr<Tel> t(new Tel());
      if (k == 0) zapni(*t, false, 7);
      else if (k == 1) { spustXex(*t, xexStres(), "b363_stres.xex", true); t->m->stereoPokey = true; }
      else { zapni(*t, false, 3); const std::vector<uint8_t> d = atr720(true); t->m->disk.load(d.data(), d.size(), "b363_sio.atr"); }
      g_vzorHaldy = i ? 0x5A : 0xFF;
      for (int f = 0; f < 400; f++) {
        if (k == 2 && f < 50) t->m->consol = 3;
        const bool zavadi = t->xl.aktivni();
        snimek(*t, stisky(77, (uint32_t)f, 1, k != 2, k == 1), stisky(78, (uint32_t)f, 2, k != 2, false), !zavadi, false);
      }
      atariOnlineUloz(*t->m, t->k, st[i], ATARI_STAV_MEDIA);
      atariOnlineOtisk(*t->m, t->k, ca[i]);
    }
    g_vzorHaldy = 0xCD;
    std::string ruzne;
    for (int c = 0; c < ATARI_STAV_CASTI; c++) if (ca[0][c] != ca[1][c]) ruzne += std::string(" ") + atariStavNazvyCasti()[c];
    over(st[0] == st[1], std::string(jm[k]) + ": stav po 400 snimcich od zapnuti nezavisi na zbytku haldy (halda 00/FF vs A5/5A, " +
                             std::to_string(st[0].size() / 1024) + " KB bajt po bajtu" + (ruzne.empty() ? ")" : ") - LISI SE:" + ruzne));
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
