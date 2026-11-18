// test_b348_sio.cpp - B348: jadro Atari (nap_atari_machine.h) na PC, BEZ HER.
//  A) nestabilni LXA ($AB) jako SALLY v Atari (vektory z Acid800: A = X = A & imm)
//  B) jednotka D1: na urovni bajtu SIO - vlastni zavadec (test_b348_sio.asm, jako xBIOS
//     v Prince of Persia): boot pres OS (SIO patch), pak vymena diskety bez restartu a
//     prikazy primo pres POKEY/PIA: cteni, STATUS, PUT/WRITE, NAK, spatny soucet,
//     cizi jednotka; disketa SD, DD a zamcena proti zapisu
//  C) casovac 4 v asynchronnim prijmu (SKCTL bit 4) stoji, bez nej bezi
//  D) volitelne soubory Reneho (v repozitari NEJSOU): Acid800 (ocekava se 38 z 38
//     u verze 0.1) a Prince of Persia DD (musi nahrat hru z diskety)
// Preklad (z test_overeni/b348):
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b348_sio test_b348_sio.cpp
//   ./test_b348_sio [acid800.atr] [PoP_DD.atr]                 -> VYSTUP_test_b348_sio.txt
// Stejny test prelozeny proti jadru B347 musi hlasit chyby (B + C + A) - test chybu pozna.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
#include "test_b348_program.h"
using namespace nap;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const std::string &co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); fflush(stdout); }
static std::vector<uint8_t> nacti(const char *f) {
  std::vector<uint8_t> d; FILE *fp = fopen(f, "rb"); if (!fp) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n); fclose(fp); return d;
}

// ---------------- A) LXA na samotnem procesoru ----------------
struct RamBus {
  uint8_t mem[65536];
  uint8_t rd(uint16_t a) { return mem[a]; }
  void wr(uint16_t a, uint8_t v) { mem[a] = v; }
  bool nmiPending() { return false; }
  void nmiAck() {}
  void traceNmi() {}
  bool irqActive() { return false; }
};
static void testLxa() {
  static RamBus bus; std::memset(bus.mem, 0, sizeof bus.mem);
  Cpu6502T<RamBus> cpu; cpu.bus = &bus;
  struct V { uint8_t a, x, imm, ocA, ocP; } v[] = {
    {0x00, 0xFF, 0x00, 0x00, 0x22},     // Acid800 #1: A=X=0, Z
    {0x33, 0xFF, 0x55, 0x11, 0x20},     // Acid800 #2: $33 & $55 = $11 (s konstantou $EE by bylo $55)
    {0xFF, 0xFF, 0x80, 0x80, 0xA0},     // Acid800 #3: N
  };
  int ok = 0;
  for (auto &t : v) {
    bus.mem[0x2000] = 0xAB; bus.mem[0x2001] = t.imm;
    cpu.pc = 0x2000; cpu.a = t.a; cpu.x = t.x; cpu.p = 0x20; cpu.jam = false; cpu.takeNmi = cpu.takeIrq = false;
    cpu.step();
    const bool s = cpu.a == t.ocA && cpu.x == t.ocA && (cpu.p & 0xC3) == (t.ocP & 0xC3);
    printf("      LXA #$%02X s A=$%02X -> A=$%02X X=$%02X P=$%02X (ocekavano $%02X)\n", t.imm, t.a, cpu.a, cpu.x, cpu.p, t.ocA);
    if (s) ok++;
  }
  over(ok == 3, "A LXA ($AB) jako SALLY v Atari: A = X = A & imm (3 vektory z Acid800)");
}

// ---------------- B) syntetické diskety ----------------
static uint8_t obsah(int s, int i) { return (uint8_t)(s * 7 + i * 13 + (s >> 8) * 5 + 1); }
static std::vector<uint8_t> atr(int sektoru, int velikost) {
  const size_t bajtu = velikost == 256 ? 384 + (size_t)(sektoru - 3) * 256 : (size_t)sektoru * 128;
  std::vector<uint8_t> d(16 + bajtu, 0);
  const size_t par = bajtu / 16;
  d[0] = 0x96; d[1] = 0x02; d[2] = par & 0xFF; d[3] = (par >> 8) & 0xFF; d[6] = (par >> 16) & 0xFF;
  d[4] = velikost & 0xFF; d[5] = (velikost >> 8) & 0xFF;
  for (int s = 1; s <= sektoru; s++) {
    const size_t off = (velikost == 256 && s > 3) ? 384 + (size_t)(s - 4) * 256 : (size_t)(s - 1) * 128;
    const int len = (velikost == 256 && s > 3) ? 256 : 128;
    for (int i = 0; i < len; i++) d[16 + off + i] = obsah(s, i);
  }
  return d;
}
static std::vector<uint8_t> bootDisketa() {
  std::vector<uint8_t> d = atr(720, 128);
  for (size_t i = 0; i < sizeof(B348_KOD); i++) d[16 + i] = B348_KOD[i];   // sektory 1-5 = program
  return d;
}
static uint8_t vzor(int i) { return (uint8_t)((i & 0xFF) * 3 + 7); }

struct Radek { uint8_t dev, cmd, a1, a2, cnt, flg, buf; const char *popis; uint8_t r0, r1, r2, r3; };

// pocet nizkourovnovych prikazu (jen nove jadro ma SioBus) - test jde prelozit i proti B347
template <class M> static auto nizke(M *m, int) -> decltype(m->sio.prikazu) { return m->sio.prikazu; }
template <class M> static long long nizke(M *, long) { return -1; }

static void scenar(const char *nazev, int velikost, bool zamek, const std::vector<Radek> &tab) {
  printf("--- %s ---\n", nazev);
  Machine *m = new Machine(); m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit(); m->consol = 3; m->cpu.reset();
  std::vector<uint8_t> boot = bootDisketa();
  m->disk.load(boot.data(), boot.size(), "b348_boot.atr");
  int f = 0;
  for (; f < 600 && m->mem.ram[0x6FE] != 0xA5; f++) { m->consol = f < 50 ? 3 : 7; m->runFrame(); }
  const long long patch = m->sioPrikazu;
  over(m->mem.ram[0x6FE] == 0xA5, std::string("B ") + nazev + ": program nabootoval pres OS (SIO patch, " + std::to_string(patch) + " prikazu SIOV, snimek " + std::to_string(f) + ")");
  // vymena diskety bez restartu (jako v appce) + tabulka testu + GO
  std::vector<uint8_t> d = atr(720, velikost);
  AtrDisk n; n.load(d.data(), d.size(), "b348_test.atr"); n.writeProtect = zamek; m->disk = std::move(n);
  int k = 0;
  for (auto &r : tab) {
    const uint8_t b[8] = {r.dev, r.cmd, r.a1, r.a2, r.cnt, r.flg, r.buf, 0};
    for (int i = 0; i < 8; i++) m->mem.ram[B348_TAB + k * 8 + i] = b[i];
    k++;
  }
  for (int i = 0; i < 8; i++) m->mem.ram[B348_TAB + k * 8 + i] = 0;
  m->mem.ram[0x6FF] = 1;
  const int f0 = f;
  for (; f < f0 + 3000 && m->mem.ram[0x6F0] != 0x5A && !m->cpu.jam; f++) { m->consol = 7; m->runFrame(); }
  over(m->mem.ram[0x6F0] == 0x5A, std::string("B ") + nazev + ": program dobehl (" + std::to_string(f - f0) + " snimku, nizkourovnovych prikazu jednotky " + std::to_string(nizke(m, 0)) + ")");
  auto zn = [](uint8_t c) { char b[8]; if (c >= 32 && c < 127) snprintf(b, sizeof b, "'%c'", c); else snprintf(b, sizeof b, "$%02X", c); return std::string(b); };
  k = 0;
  for (auto &r : tab) {
    const uint8_t *res = &m->mem.ram[0x600 + k * 4];
    bool ok = res[0] == r.r0 && res[1] == r.r1 && res[2] == r.r2 && res[3] == r.r3;
    std::string det = " [" + zn(res[0]) + " " + zn(res[1]) + " " + zn(res[2]) + " " + zn(res[3]) + "]";
    // data v bufferu
    if (ok && r.cmd == 'R' && r.r3 == 1) {
      const int len = r.cnt ? r.cnt : 256, sek = r.a1 | (r.a2 << 8);
      const bool zpet = sek == 10 || sek == 11;     // zapsany sektor = VZOR
      for (int i = 0; i < len; i++) if (m->mem.ram[(r.buf << 8) + i] != (zpet ? vzor(i) : obsah(sek, i))) { ok = false; det += " DATA NESEDI na " + std::to_string(i); break; }
    }
    if (ok && r.cmd == 'S' && r.r3 == 1) {
      const uint8_t oc0 = (uint8_t)(0x10 | (velikost == 256 ? 0x20 : 0) | (zamek ? 0x08 : 0));
      const uint8_t *s = &m->mem.ram[r.buf << 8];
      char b[48]; snprintf(b, sizeof b, " stav %02X %02X %02X %02X", s[0], s[1], s[2], s[3]); det += b;
      if (s[0] != oc0 || s[1] != 0xFF || s[2] != 0xE0 || s[3] != 0x00) ok = false;
    }
    over(ok, std::string("B ") + nazev + ": " + r.popis + det);
    k++;
  }
  // zapsane sektory na diskete
  for (auto &r : tab) {
    if (r.flg & 1) {
      const int sek = r.a1 | (r.a2 << 8);
      size_t off; int len;
      if (!m->disk.sec(sek, off, len)) continue;
      bool vz = true, puv = true;
      for (int i = 0; i < len; i++) { if (m->disk.data[off + i] != vzor(i)) vz = false; if (m->disk.data[off + i] != obsah(sek, i)) puv = false; }
      if (zamek) over(puv, std::string("B ") + nazev + ": sektor " + std::to_string(sek) + " na zamcene diskete zustal puvodni");
      else over(vz, std::string("B ") + nazev + ": sektor " + std::to_string(sek) + " je na diskete opravdu zapsany (" + std::to_string(len) + " B)");
    }
  }
  over(m->mem.ram[0x6F3] == 0, std::string("B ") + nazev + ": zadna chyba ramce ani preteceni v SKSTAT");
  over(m->mem.ram[0x6F1] == 0x04, std::string("C ") + nazev + ": casovac 4 v asynchronnim prijmu (SKCTL $13) bez start bitu stoji (IRQST bit 2 = 1)");
  over(m->mem.ram[0x6F2] == 0x00, std::string("C ") + nazev + ": casovac 4 bez asynchronniho prijmu (SKCTL $03) preruseni da");
  delete m->view; delete m;
}

// ---------------- D) volitelne: Acid800 a Prince of Persia ----------------
static std::string obrazovka(Machine &m) {
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    for (int c = 0; c < 40; c++) {
      int v = m.peek((uint16_t)(sav + r * 40 + c)) & 0x7F;
      int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v);
      if (a < 32 || a > 126) a = ' ';
      out.push_back((char)a);
    }
  }
  return out;
}
static void acid(const char *soubor) {
  std::vector<uint8_t> d = nacti(soubor);
  Machine *m = new Machine(); m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit(); m->consol = 3; m->cpu.reset();
  if (!m->disk.load(d.data(), d.size(), "acid800.atr")) { over(false, "D Acid800: ATR nejde nacist"); return; }
  std::string verze, souhrn;
  for (int f = 0; f < 60000; f++) {
    m->consol = f < 50 ? 3 : 7;
    m->runFrame();
    if (f % 8) continue;
    const std::string s = obrazovka(*m);
    size_t v = s.find("Acid800 test, V");
    if (v != std::string::npos && verze.empty()) { verze = s.substr(v + 14, 5); while (!verze.empty() && verze.back() == ' ') verze.pop_back(); }
    size_t q = s.find("Passed:");
    if (q != std::string::npos && s.find("All tests complete.") != std::string::npos) {
      for (int k = 0; k < 60; k++) m->runFrame();                 // radek se souhrnem se dopise
      const std::string s2 = obrazovka(*m); q = s2.find("Passed:");
      souhrn = s2.substr(q, 40 - (q % 40));
      while (!souhrn.empty() && souhrn.back() == ' ') souhrn.pop_back();
      break;
    }
    if (m->cpu.jam) break;
  }
  printf("      Acid800 %s: %s\n", verze.c_str(), souhrn.c_str());
  const bool nula = souhrn.find("Failed: 0") != std::string::npos || (souhrn.find("Failed") == std::string::npos && !souhrn.empty());
  if (verze == "V0.1") over(souhrn.find("Passed: 38") != std::string::npos && nula, "D Acid800 V0.1: vsech 38 testu proslo");
  else over(!souhrn.empty(), "D Acid800 " + verze + " dobehl (" + souhrn + ")");
  delete m->view; delete m;
}
static void prince(const char *soubor) {
  std::vector<uint8_t> d = nacti(soubor);
  Machine *m = new Machine(); m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit(); m->consol = 3; m->cpu.reset();
  if (!m->disk.load(d.data(), d.size(), soubor)) { over(false, "D Prince of Persia: ATR nejde nacist"); return; }
  for (int f = 1; f <= 4400; f++) {
    m->consol = f < 50 ? 3 : 7;
    if (f == 1500) m->klavesa(33);                   // MEZERA = preskocit uvodni demo
    m->runFrame();
    if (m->cpu.jam) break;
  }
  printf("      Prince of Persia: nacteno sektoru %lld, prikazu SIOV %lld, nizkourovnovych %lld, PC=$%04X jam=%d\n",
         m->disk.reads, m->sioPrikazu, nizke(m, 0), m->cpu.pc, m->cpu.jam ? 1 : 0);
  over(m->disk.reads >= 400 && !m->cpu.jam, "D Prince of Persia DD: vlastni zavadec (xBIOS) nahral hru z diskety (>= 400 sektoru)");
  delete m->view; delete m;
}

int main(int argc, char **argv) {
  printf("B348 - jednotka D1: na urovni bajtu SIO, casovac 4 v asynchronnim prijmu, LXA (jadro na PC, bez her)\n\n");
  testLxa();
  scenar("SD disketa (128 B)", 128, false, {
    {0x31, 'R', 4, 0, 128, 2, 0x30, "cteni sektoru 4 -> 'A' 'C' + 128 B + soucet", 'A', 0, 'C', 1},
    {0x31, 'R', 0xD0, 2, 128, 2, 0x31, "cteni posledniho sektoru 720", 'A', 0, 'C', 1},
    {0x31, 'R', 0xD1, 2, 128, 2, 0x32, "sektor 721 neexistuje -> 'N' (NAK)", 'N', 0, 0, 0},
    {0x31, 'S', 0, 0, 4, 2, 0x33, "STATUS -> 'A' 'C' $10 $FF $E0 $00", 'A', 0, 'C', 1},
    {0x31, 'P', 10, 0, 128, 1, 0x34, "PUT sektoru 10 -> 'A', datovy ramec, 'A' 'C'", 'A', 'A', 'C', 0},
    {0x31, 'R', 10, 0, 128, 2, 0x35, "sektor 10 precten zpet = zapsana data", 'A', 0, 'C', 1},
    {0x31, 'R', 4, 0, 128, 0x82, 0x36, "spatny soucet ramce prikazu -> jednotka mlci", 0, 0, 0, 0},
    {0x32, 'R', 4, 0, 128, 2, 0x37, "prikaz pro D2: -> nikdo neodpovi", 0, 0, 0, 0},
    {0x31, '?', 0, 0, 0, 0, 0x38, "'?' (vysoka rychlost) -> 'N' - jednotka umi jen 19200 Bd", 'N', 0, 0, 0},
    {0x31, 'W', 11, 0, 128, 1, 0x34, "WRITE (s kontrolou) sektoru 11 -> 'A' 'A' 'C'", 'A', 'A', 'C', 0},
  });
  scenar("DD disketa (256 B)", 256, false, {
    {0x31, 'R', 4, 0, 0, 2, 0x30, "cteni sektoru 4 = 256 B", 'A', 0, 'C', 1},
    {0x31, 'R', 1, 0, 128, 2, 0x31, "cteni sektoru 1 = 128 B (sektory 1-3 jsou i u DD po 128 B)", 'A', 0, 'C', 1},
    {0x31, 'S', 0, 0, 4, 2, 0x32, "STATUS -> $30 (dvojita hustota)", 'A', 0, 'C', 1},
    {0x31, 'P', 10, 0, 0, 1, 0x34, "PUT sektoru 10 (256 B)", 'A', 'A', 'C', 0},
    {0x31, 'R', 10, 0, 0, 2, 0x35, "sektor 10 precten zpet = zapsana data", 'A', 0, 'C', 1},
    {0x31, 'R', 0xD1, 2, 0, 2, 0x36, "sektor 721 -> 'N'", 'N', 0, 0, 0},
  });
  scenar("zamcena SD disketa", 128, true, {
    {0x31, 'P', 10, 0, 128, 1, 0x34, "PUT na zamcenou disketu -> 'A' 'A' 'E' (chyba)", 'A', 'A', 'E', 0},
    {0x31, 'S', 0, 0, 4, 2, 0x33, "STATUS -> $18 (bit 3 = zamceno)", 'A', 0, 'C', 1},
    {0x31, 'R', 4, 0, 128, 2, 0x30, "cteni jde i ze zamcene", 'A', 0, 'C', 1},
  });
  auto jmeno = [](const char *c) { const char *s = std::strrchr(c, '/'); return s ? s + 1 : c; };
  if (argc > 1) { printf("--- volitelne: %s ---\n", jmeno(argv[1])); acid(argv[1]); }
  if (argc > 2) { printf("--- volitelne: %s ---\n", jmeno(argv[2])); prince(argv[2]); }
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
