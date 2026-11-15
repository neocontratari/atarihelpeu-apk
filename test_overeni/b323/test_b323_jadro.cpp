// test_overeni/b323/test_b323_jadro.cpp
// ====================================================================
//  B323: dve opravy jadra Atari (Rene: "v atari nefunguji 2 hry").
//   1) XEX zavadec: stav jako po zavedeni DOSu - BOOT? ($09) bit 0 = 1 a DOSINI ($0C) na RTS
//      v OS ROM, kdyz ho nic nenastavilo. Air Strike (bootovaci hra prevedena do XEX) da do
//      DOSINI svuj start a skoci na teply start OS - ten DOSINI vola jen s BOOT? bit 0.
//   2) rozsirena pamet 320K Compy Shop (16 bank, PORTB bity 2-3 + 6-7): Sonic A8 chce
//      "320K RAMBO OR LARGER". Banky 130XE (bity 6-7 = 1) lezi v ext[] tam co drive.
//  Pouziti: test_b323_jadro [Air_Strike.xex] [SONIC_A8_AI.xex]   (hry jsou Reneho - nejsou v repu)
// ====================================================================
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
using namespace nap;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co, const std::string &detail = "") {
  if (ok) g_ok++; else g_chyb++;
  std::printf("%s %s%s\n", ok ? "OK    " : "CHYBA ", co.c_str(), ok || detail.empty() ? "" : ("   -> " + detail).c_str());
}
static std::vector<uint8_t> nacti(const char *f) {
  std::vector<uint8_t> d; FILE *fp = fopen(f, "rb"); if (!fp) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n); fclose(fp); return d;
}
static Machine *stroj() {
  Machine *m = new Machine(); m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit();
  return m;
}
static void portB(Machine &m, int v) { m.piaWrite(3, 0x00); m.piaWrite(1, 0xFF); m.piaWrite(3, 0x04); m.piaWrite(1, (uint8_t)v); }
static std::string hex(int v) { char b[16]; snprintf(b, sizeof b, "$%04X", v); return b; }
// text na obrazovce (interni kody ANTIC -> ASCII), radky po 40 znacich od SAVMSC
static std::string obrazovka(const Machine &m) {
  const int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string s;
  for (int i = 0; i < 40 * 24; i++) {
    const int c = m.mem.ram[(sav + i) & 0xFFFF] & 0x7F;
    s += c < 0x40 ? (char)(c + 0x20) : c < 0x60 ? '.' : (char)c;
  }
  return s;
}
// XEX jako v appce: studeny start s OPTION, zavadec po snimcich; scenar START/FIRE
static Machine *spustXex(const std::vector<uint8_t> &d, XexLoader &xl, int snimku, std::vector<uint32_t> *otisky = nullptr,
                         std::vector<int> start = {}, std::vector<int> fire = {}, std::vector<int> vpravo = {}) {
  Machine *m = stroj(); m->consol = 3; m->cpu.reset();
  xl.priprav(d.data(), d.size(), "hra.xex"); m->sioRychlyTimeout = true;
  for (int f = 1; f <= snimku; f++) {
    int consol = 7; bool fi = false, vp = false;
    for (int s : start) if (f >= s && f <= s + 6) consol &= ~1;
    for (int s : fire) if (f >= s && f <= s + 6) fi = true;
    for (int s : vpravo) if (f >= s && f <= s + 120) vp = true;
    if (!xl.aktivni()) m->consol = consol;
    m->trig[0] = fi ? 0 : 1;
    m->porta = vp ? 0xF7 : 0xFF;
    m->runFrame(); xl.poSnimku(*m);
    float z[1000]; m->genAudio(z, 882, 44100.0);
    if (otisky && f % 50 == 0) otisky->push_back(napCrc32(m->view->idx, (size_t)AnticView::W * AnticView::H));
  }
  return m;
}

int main(int argc, char **argv) {
  std::printf("== B323 jadro Atari: zavadec XEX jako po DOSu, pamet 320K Compy Shop\n");
  // ---------------- 320K ----------------
  {
    Machine *m = stroj();
    // zapis do vsech 256 hodnot PORTB s bitem 4 = 0 (CPU v rozsirene pameti), cteni zpet
    std::vector<int> banky;
    for (int b = 0; b < 16; b++) {
      portB(*m, 0x23 | ((b & 3) << 2) | ((~b & 0x0C) << 4));
      m->memWrite(0x4000, (uint8_t)(0xA0 + b)); m->memWrite(0x7FFF, (uint8_t)(0x50 + b));
    }
    int ruznych = 0;
    for (int b = 0; b < 16; b++) {
      portB(*m, 0x23 | ((b & 3) << 2) | ((~b & 0x0C) << 4));
      if (m->memRead(0x4000, false) == 0xA0 + b && m->memRead(0x7FFF, false) == 0x50 + b) ruznych++;
    }
    over(ruznych == 16, "T1 16 oddelenych bank po 16 kB (PORTB bity 2-3 a 6-7), kazda drzi svuj obsah", std::to_string(ruznych));
    bool stejne = true;
    for (int b = 0; b < 4; b++) {
      portB(*m, 0xE3 | (b << 2));                                   // banky 130XE
      m->memWrite(0x4123, (uint8_t)(0x10 + b));
      stejne &= m->mem.ext[b * 0x4000 + 0x123] == 0x10 + b;
    }
    over(stejne, "T2 banky 130XE ($E3/$E7/$EB/$EF) lezi v ext[0..64 kB] presne jako v B322 (vypis pameti, test xe130)");
    portB(*m, 0xE3); m->memWrite(0x5000 + 0x900, 0x77);                // banka 0 (130XE), mimo self-test
    portB(*m, 0xFF);
    const bool hlavni = m->memRead(0x5900, false) != 0x77;
    portB(*m, 0xC3 | 0x10);                                           // ANTIC bit 5 = 0, CPU bit 4 = 1
    const bool antic = m->memRead(0x5900, true) == 0x77 && m->memRead(0x5900, false) != 0x77;
    over(hlavni && antic, "T3 bit 4 = CPU, bit 5 = ANTIC jako na 130XE (ANTIC vidi banku, CPU zakladni pamet)");
    portB(*m, 0x63);                                                  // bit 7 = 0 a OS zap.: self-test ROM ma prednost
    const bool selftest = m->memRead(0x5000, false) == NAP_OS_ROM[0x1000];
    over(selftest, "T4 self-test ROM ($5000, PORTB bit 7 = 0) ma dal prednost pred bankou (jako v B322)");
    delete m->view; delete m;
  }
  // ---------------- zavadec: BOOT? a DOSINI ----------------
  {
    // nejmensi XEX: RTS na $2000 + RUNAD
    const uint8_t mini[] = {0xFF, 0xFF, 0x00, 0x20, 0x00, 0x20, 0x60, 0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20};
    std::vector<uint8_t> d(mini, mini + sizeof mini);
    XexLoader xl;
    Machine *m = spustXex(d, xl, 200);
    const int dosini = m->mem.ram[0x0C] | (m->mem.ram[0x0D] << 8);
    over((m->mem.ram[0x09] & 1) && dosini == 0xC017 && NAP_OS_ROM[dosini - 0xC000] == 0x60,
         "T5 po zavedeni XEX: BOOT? bit 0 = 1 a DOSINI = RTS v OS ROM (" + hex(dosini) + ") jako po DOSu", hex(m->mem.ram[0x09]) + " " + hex(dosini));
    over(m->cpu.pc == 0x0100 && !m->cpu.jam, "T6 program se vratil do parkovaci smycky (teply start nic nerozbil)", hex(m->cpu.pc));
    delete m->view; delete m;
    // XEX, ktery si DOSINI nastavi sam a skoci na teply start (jako obal Air Strike)
    // XEX, ktery si DOSINI nastavi sam a skoci na teply start (jako obal Air Strike):
    // $2000: LDA #$10 / STA $0C / LDA #$20 / STA $0D / JMP $E474;  $2010: INC $0600 / RTS;  RUNAD $2000
    std::vector<uint8_t> d2 = {0xFF, 0xFF, 0x00, 0x20, 0x0A, 0x20, 0xA9, 0x10, 0x85, 0x0C, 0xA9, 0x20, 0x85, 0x0D, 0x4C, 0x74, 0xE4,
                               0x10, 0x20, 0x13, 0x20, 0xEE, 0x00, 0x06, 0x60, 0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20};
    XexLoader xl2;
    Machine *m2 = spustXex(d2, xl2, 300);
    over(m2->mem.ram[0x600] == 1, "T7 program si da do DOSINI svuj start a skoci na teply start -> OS ho zavola (jako Air Strike)",
         "$0600=" + std::to_string(m2->mem.ram[0x600]) + " PC=" + hex(m2->cpu.pc));
    delete m2->view; delete m2;
  }
  // ---------------- Reneho hry (kdyz jsou) ----------------
  if (argc > 1) {
    std::vector<uint8_t> d = nacti(argv[1]);
    if (d.empty()) over(false, "Air Strike: soubor nejde nacist", argv[1]);
    else {
      XexLoader xl; std::vector<uint32_t> ot;
      Machine *m = spustXex(d, xl, 1300, &ot, {600, 800}, {900}, {1000});
      int ruznych = 0;
      for (size_t i = 17; i < ot.size(); i++) if (ot[i] != ot[i - 1]) ruznych++;
      over(m->cpu.pc != 0x0100 && (m->mem.ram[0x0C] | (m->mem.ram[0x0D] << 8)) == 0x3C20,
           "H1 Air Strike: obal nastavi DOSINI $3C20, teply start ho zavola, hra bezi (ne parkovaci smycka $0100)", hex(m->cpu.pc));
      over(ruznych >= 3, "H2 Air Strike: po START + START se hraje (obraz se meni: krajina, letadlo)", std::to_string(ruznych));
      delete m->view; delete m;
    }
  }
  if (argc > 2) {
    std::vector<uint8_t> d = nacti(argv[2]);
    if (d.empty()) over(false, "Sonic: soubor nejde nacist", argv[2]);
    else {
      XexLoader xl;
      Machine *m = spustXex(d, xl, 400);
      std::string log;
      for (auto &l : xl.log) log += l + "\n";
      const std::string obr = obrazovka(*m);
      over(log.find("nahrano: 59 segmentu") != std::string::npos && log.find("21 x INIT") != std::string::npos,
           "H3 Sonic: test pameti projde, nahraje se vsech 59 segmentu (21 x INIT, 10 bank 320K)", log);
      over(obr.find("NOT ENOUGH") == std::string::npos && m->cpu.pc != 0x97C7, "H4 Sonic: zadne NOT ENOUGH EXTENDED MEMORY, hra bezi", hex(m->cpu.pc));
      delete m->view; delete m;
      XexLoader xl2; std::vector<uint32_t> ot;
      Machine *m2 = spustXex(d, xl2, 1600, &ot, {600}, {900}, {1000});
      int ruznych = 0;
      for (size_t i = 19; i < ot.size(); i++) if (ot[i] != ot[i - 1]) ruznych++;
      over(ruznych >= 3, "H5 Sonic: po FIRE / START se hraje (obraz se meni)", std::to_string(ruznych));
      delete m2->view; delete m2;
    }
  }
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
