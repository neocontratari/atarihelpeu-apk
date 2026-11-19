// test_b352_jadro.cpp - B352: rozsirena pamet 1088K (64 bank = 1 MB jako Ultimate 1MB) v jadru Atari.
// Rene: hra TONY_ZAMEK_VBXE_DEV31_PARTIAL.xex nejde ("mame asi furt malo pameti ve VBXE"). Rozbor: VBXE hra
// nasla, ale jeji test pameti ($1A66) zkousi 32 hodnot PORTB (bity 2-3 a 5-7) a chce aspon 27 ruznych bank
// (496 kB) - nase 320K (B323) ma 16 -> "BRAK RAM - WYMAGANE 496 KB + VBXE" a smycka $1AEB.
//  T1 vychozi 320K beze zmeny: samotest memHash B0649DC5 (jako B323), banky 130XE, ANTIC bit 5
//  T2 1088K: 64 ruznych bank (PORTB bity 1-3 a 5-7), banky 130XE $E3-$EF = 0-3, ANTIC vidi totez co procesor,
//     self-test ROM ani BASIC se s rozsirenou pameti neprekryva (bity 7 a 1 = cislo banky), bez rozsirene ano
//  T3 pozna "program hleda vic bank": vic nez 16 ruznych PORTB pri zapisu do okna $4000-$7FFF (130XE 4 ani
//     Compy 16 ne), v 1088K uz nic
//  T4 testy pameti (vlastni kod 6502 z vyrob_b352.py) jako XEX pres zavadec appky, se spustenim znovu jako appka:
//     Tony (32 hodnot) -> znovu s 1088K -> 32 bank; 130XE -> 4 bez restartu; Compy -> 16 bez restartu
//  T5 totez z bootovaci diskety (ATR v D1:, SIO)
//  T6 volitelne skutecna hra: ./test_b352_jadro TONY.xex -> v 320K hlaska BRAK RAM, s 1088K se nahraje a bezi
// Preklad a spusteni (z test_overeni/b352):
//   python3 vyrob_b352.py > test_b352_program.h
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b352_jadro test_b352_jadro.cpp
//   ./test_b352_jadro [TONY_ZAMEK_VBXE_DEV31_PARTIAL.xex]           -> VYSTUP_test_b352_jadro.txt
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
#include "nap_atari_selftest.h"
#include "test_b352_program.h"
using namespace nap;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const std::string &co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); fflush(stdout); }
static std::vector<uint8_t> nacti(const char *f) {
  std::vector<uint8_t> d; FILE *fp = fopen(f, "rb"); if (!fp) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n); fclose(fp); return d;
}
static void setPortB(Machine &m, int v) { m.piaWrite(3, 0x00); m.piaWrite(1, 0xFF); m.piaWrite(3, 0x04); m.piaWrite(1, (uint8_t)v); }
static std::string text(Machine &m, int adr, int n) {
  std::string s;
  for (int i = 0; i < n; i++) {
    const int c = m.mem.ram[(adr + i) & 0xFFFF] & 0x7F;
    const char ch = c < 0x40 ? (char)(c + 0x20) : (c < 0x60 ? (char)(c - 0x40) : (char)c);
    s += (ch >= 32 && ch < 127) ? ch : '.';
  }
  while (!s.empty() && s.back() == ' ') s.pop_back();
  return s;
}

// program jako v appce: XEX pres XexLoader (studeny start s OPTION) nebo ATR v D1:; kdyz program hleda vic
// bank nez 320K (mem.chce1088), spusti se znovu od zacatku s 1088K (nap_atari_native.cpp restart1088Locked)
struct Beh {
  Machine *m = nullptr; AnticView v; XexLoader xl;
  std::vector<uint8_t> data; bool jeXex = true; bool r1088 = false; int restartu = 0; int snimekRestartu = -1; int ruznych = 0;
  void start() {
    AtrDisk disk;
    if (m && !jeXex) disk = std::move(m->disk);
    delete m; m = new Machine(); m->view = &v;
    m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
    m->mem.rezim1088 = r1088;
    if (jeXex) { m->coldInit(); m->consol = 3; m->cpu.reset(); xl.priprav(data.data(), data.size(), "test.xex"); m->sioRychlyTimeout = true; }
    else { if (!disk.mounted) disk.load(data.data(), data.size(), "test.atr"); m->disk = std::move(disk); m->reset(); m->consol = 3; }
  }
  void bez(int snimku) {
    for (int f = 1; f <= snimku; f++) {
      if (jeXex) { if (!xl.aktivni()) m->consol = 7; } else m->consol = f < 50 ? 3 : 7;
      m->runFrame(); if (jeXex) xl.poSnimku(*m);
      float z[1000]; m->genAudio(z, 882, 44100.0);
      if (m->mem.chce1088 && !m->mem.rezim1088) { ruznych = m->mem.pbRuznych; r1088 = true; restartu++; snimekRestartu = f; start(); }
      if (m->cpu.jam) break;
    }
  }
  ~Beh() { delete m; }
};

int main(int argc, char **argv) {
  printf("B352 - rozsirena pamet 1088K (64 bank) v jadru Atari, sama pro program, ktery hleda vic bank nez 320K\n\n");

  printf("T1) VYCHOZI 320K BEZE ZMENY\n");
  {
    long long n = 0;
    const uint32_t h = selftest::memHash(&n);
    printf("     memHash %08X (%lld cteni)\n", h, n);
    over(h == 0xB0649DC5u, "samotest memHash B0649DC5 = NativeAtariCoreBridge.OCEKAVANY_MEM_HASH (B323) - 320K se nezmenila");
    Machine *mp = new Machine(); Machine &m = *mp;
    m.coldInit(); m.mem.os = NAP_OS_ROM; m.mem.bas = NAP_BASIC_ROM;
    over(!m.mem.rezim1088 && m.mem.bankAktivnich() == 16, "novy stroj = 320K (16 bank), 1088K jen kdyz ho appka zapne");
    for (int b = 0; b < 4; b++) { setPortB(m, 0xE3 | (b << 2)); m.memWrite(0x4000, (uint8_t)(0x40 + b)); }
    bool xe = true;
    for (int b = 0; b < 4; b++) if (m.mem.ext[b * 16384] != 0x40 + b) xe = false;
    setPortB(m, 0xD3);                                     // CPU hlavni, ANTIC banka 0 (130XE)
    const bool antic = m.memRead(0x4000, true) == 0x40 && m.memRead(0x4000, false) != 0x40;
    over(xe && antic, "320K: banky 130XE $E3-$EF = ext[0-3], ANTIC bit 5 oddelene od procesoru (jako 130XE)");
    delete mp;
  }

  printf("\nT2) 1088K\n");
  {
    Machine *mp = new Machine(); Machine &m = *mp;
    m.coldInit(); m.mem.os = NAP_OS_ROM; m.mem.bas = NAP_BASIC_ROM;
    m.mem.rezim1088 = true;
    std::set<int> banky;
    for (int pb = 0; pb < 256; pb++) if (!(pb & 0x10)) banky.insert(AtariMem::banka1088(pb));
    over(banky.size() == 64 && AtariMem::EXT_BANK == 64, "64 ruznych bank (PORTB bity 1-3 a 5-7, bit 4 = 0)");
    // zapis do vsech 64 bank pres okno, cteni zpet
    for (int k = 0; k < 64; k++) {
      const int pb = 0x01 | ((k & 1) ? 0 : 2) | ((k >> 1 & 3) << 2) | ((k & 8) ? 0 : 0x20) | ((k & 16) ? 0 : 0x40) | ((k & 32) ? 0 : 0x80);
      setPortB(m, pb); m.memWrite(0x4000, (uint8_t)k); m.memWrite(0x7FFF, (uint8_t)(k ^ 0xFF)); m.memWrite(0x5123, (uint8_t)(k + 100));
    }
    bool vse = true; std::set<int> obsah;
    for (int k = 0; k < 64; k++) {
      const int pb = 0x01 | ((k & 1) ? 0 : 2) | ((k >> 1 & 3) << 2) | ((k & 8) ? 0 : 0x20) | ((k & 16) ? 0 : 0x40) | ((k & 32) ? 0 : 0x80);
      setPortB(m, pb);
      if (m.memRead(0x4000, false) != k || m.memRead(0x7FFF, false) != (k ^ 0xFF) || m.memRead(0x5123, false) != k + 100) vse = false;
      obsah.insert(m.memRead(0x4000, false));
    }
    over(vse && obsah.size() == 64, "do vsech 64 bank se zapise a precte zpet (i $5000-$57FF v bankach s bitem 7 = 0 - self-test ROM tam neni)");
    bool xe = true;
    for (int b = 0; b < 4; b++) { setPortB(m, 0xE3 | (b << 2)); if (AtariMem::banka1088(0xE3 | (b << 2)) != b) xe = false; }
    over(xe, "banky 130XE $E3 / $E7 / $EB / $EF = 0-3 (stejne misto jako v 320K)");
    setPortB(m, 0xC3);                                     // bit 5 = 0: v 1088K cislo banky (jina nez $E3)
    m.memWrite(0x4000, 0x77);
    setPortB(m, 0xE3);
    const bool bit5 = m.memRead(0x4000, false) != 0x77;
    setPortB(m, 0xC3);
    const bool antic = m.memRead(0x4000, true) == 0x77;    // ANTIC vidi totez co procesor
    setPortB(m, 0xD3);                                     // CPU hlavni -> ANTIC taky hlavni
    const bool antic2 = m.memRead(0x4000, true) == m.mem.ram[0x4000];
    over(bit5 && antic && antic2, "bit 5 = cislo banky; ANTIC vidi stejnou pamet jako procesor (bit 4), oddeleny pristup 130XE neni");
    setPortB(m, 0x73);                                     // bit 7 = 0, bit 4 = 1 (bez rozsirene), OS ROM zap.
    const bool st = m.memRead(0x5000, false) == NAP_OS_ROM[0x1000];
    setPortB(m, 0x63);                                     // bit 7 = 0, bit 4 = 0: rozsirena pamet ma prednost
    const bool stExt = m.memRead(0x5123, false) == m.mem.ext[AtariMem::banka1088(0x63) * 16384 + 0x1123];
    over(st && stExt, "self-test ROM $5000: bez rozsirene pameti je videt (bit 7 = 0), s rozsirenou (bit 4 = 0) je tam banka");
    m.mem.ram[0xA123] = 0x5A;
    setPortB(m, 0xFD);                                     // bit 1 = 0, bit 4 = 1: BASIC zapnuty
    const bool basic = m.memRead(0xA123, false) == NAP_BASIC_ROM[0x0123];
    setPortB(m, 0xED);                                     // bit 1 = 0, bit 4 = 0: banka s bitem 1 = 0 -> BASIC vypnuty
    const bool bezBasicu = m.memRead(0xA123, false) == 0x5A;
    m.memWrite(0xA124, 0x6B);
    over(basic && bezBasicu && m.mem.ram[0xA124] == 0x6B,
         "BASIC: bez rozsirene pameti ho bit 1 = 0 zapne, s rozsirenou je bit 1 cislo banky a BASIC vypnuty (Sonic A8 jinak bez levelu)");
    delete mp;
  }

  printf("\nT3) POZNA PROGRAM, KTERY HLEDA VIC BANK NEZ 320K\n");
  {
    Machine *mp = new Machine(); Machine &m = *mp;
    m.coldInit(); m.mem.os = NAP_OS_ROM; m.mem.bas = NAP_BASIC_ROM;
    for (int b = 0; b < 4; b++) for (int k = 0; k < 3; k++) { setPortB(m, 0xE3 | (b << 2)); m.memWrite(0x4000, 1); setPortB(m, 0xFF); }
    const bool xe = !m.mem.chce1088 && m.mem.pbRuznych == 4;
    for (int b = 0; b < 16; b++) { setPortB(m, 0x23 | ((b & 3) << 2) | ((~b & 0x0C) << 4)); m.memWrite(0x4000, 2); }
    const int poCompy = m.mem.pbRuznych; const bool compy = !m.mem.chce1088;
    setPortB(m, 0x83); m.memWrite(0x4000, 3);              // 17. ruzna hodnota (bit 5 = 0)
    printf("     130XE: %d ruzne PORTB, + Compy 16 bank: %d, + $83: %d -> chce1088 %d\n", 4, poCompy, m.mem.pbRuznych, (int)m.mem.chce1088);
    over(xe && compy && poCompy == 16 && m.mem.chce1088, "130XE (4) ani Compy 320K (16 ruznych PORTB) nic, 17. ruzna hodnota = program hleda vic bank");
    m.mem.chce1088 = false; m.mem.rezim1088 = true;
    for (int pb = 0; pb < 256; pb += 4) { setPortB(m, pb & ~0x10); m.memWrite(0x4000, 4); }
    over(!m.mem.chce1088, "v 1088K uz se nic nehlida");
    delete mp;
  }

  printf("\nT4) TESTY PAMETI JAKO XEX (zavadec appky, spusteni znovu jako appka)\n");
  struct P { const char *jm; const uint8_t *d; size_t n; int bank; bool restart; };
  const P prog[] = {{"Tony (32 hodnot PORTB, bity 2-3 a 5-7)", B352_XEX_TONY, sizeof B352_XEX_TONY, 32, true},
                    {"130XE (4 banky)", B352_XEX_130XE, sizeof B352_XEX_130XE, 4, false},
                    {"320K Compy Shop (16 bank)", B352_XEX_COMPY, sizeof B352_XEX_COMPY, 16, false}};
  for (const P &p : prog) {
    Beh b; b.data.assign(p.d, p.d + p.n); b.jeXex = true;
    b.start(); b.bez(300);
    const int n = b.m->mem.ram[0x600], hotovo = b.m->mem.ram[0x601];
    printf("     %-40s: bank %2d, hotovo $%02X, restartu %d (snimek %d, ruznych PORTB %d), 1088K %d\n", p.jm, n, hotovo, b.restartu, b.snimekRestartu,
           b.ruznych, (int)b.m->mem.rezim1088);
    over(hotovo == 0xA5 && n == p.bank && (b.restartu == (p.restart ? 1 : 0)) && b.m->mem.rezim1088 == p.restart,
         std::string(p.jm) + (p.restart ? ": spusten znovu s 1088K, napocita 32 bank" : ": zustava 320K, napocita " + std::to_string(p.bank)));
  }

  printf("\nT5) TEST PAMETI Z BOOTOVACI DISKETY (D1:, SIO)\n");
  {
    Beh b; b.jeXex = false;
    b.data.assign(B352_ATR_TONY_ZACATEK, B352_ATR_TONY_ZACATEK + sizeof B352_ATR_TONY_ZACATEK);
    b.data.resize(16 + (size_t)B352_ATR_SEKTORU * 128, 0);
    b.start(); b.bez(600);
    const int n = b.m->mem.ram[0x600], hotovo = b.m->mem.ram[0x601];
    printf("     disketa: bank %d, hotovo $%02X, restartu %d (snimek %d), 1088K %d, nacteno sektoru %lld\n", n, hotovo, b.restartu, b.snimekRestartu,
           (int)b.m->mem.rezim1088, b.m->disk.reads);
    over(hotovo == 0xA5 && n == 32 && b.restartu == 1 && b.m->mem.rezim1088, "disketa s testem Tony: boot, spusteni znovu s 1088K (disketa zustava v D1:), 32 bank");
  }

  if (argc > 1) {
    printf("\nT6) SKUTECNA HRA: %s\n", argv[1]);
    std::vector<uint8_t> d = nacti(argv[1]);
    {
      Beh b; b.data = d; b.jeXex = true;
      b.start();
      b.m->mem.chce1088 = false;
      // nejdriv bez 1088K (jako B351): hlaska hry
      for (int f = 0; f < 300; f++) { if (!b.xl.aktivni()) b.m->consol = 7; b.m->runFrame(); b.xl.poSnimku(*b.m); float z[1000]; b.m->genAudio(z, 882, 44100.0); }
      const std::string t = text(*b.m, 0x1250, 40);
      printf("     jen 320K (jako B351): PC=$%04X, $1250: '%s'\n", b.m->cpu.pc, t.c_str());
      over(t.find("BRAK RAM") != std::string::npos && b.m->cpu.pc >= 0x1AEB && b.m->cpu.pc <= 0x1AED, "320K: hra hlasi \"BRAK RAM - WYMAGANE 496 KB + VBXE\" a stoji ($1AEB)");
    }
    {
      Beh b; b.data = d; b.jeXex = true;
      b.start(); b.bez(1800);
      std::string posledni; for (auto &l : b.xl.log) posledni = l;
      printf("     appka: restartu %d (snimek %d, ruznych PORTB %d), 1088K %d, PC=$%04X, zavadec: %s\n", b.restartu, b.snimekRestartu, b.ruznych,
             (int)b.m->mem.rezim1088, b.m->cpu.pc, posledni.c_str());
      printf("     VBXE: XDL %s, blitru %lld, MEMAC A $%02X/$%02X\n", b.m->vbx.xdlEnabled ? "ZAP" : "vyp", b.m->vbx.nBlits, b.m->vbx.memacCtl, b.m->vbx.memacBankA);
      over(b.restartu == 1 && b.m->mem.rezim1088 && posledni.find("nahrano: 444 segmentu") != std::string::npos && posledni.find("RUNAD") != std::string::npos,
           "s 1088K: hra se spusti znovu, nahraje vsech 444 segmentu (27 bank rozsirene pameti + VRAM VBXE) a spusti se ($1000)");
      over(b.m->vbx.xdlEnabled && text(*b.m, 0x1250, 40).find("BRAK") == std::string::npos, "titulni obrazovka hry ve VBXE (XDL zapnuto), zadna hlaska BRAK RAM");
    }
  }
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
