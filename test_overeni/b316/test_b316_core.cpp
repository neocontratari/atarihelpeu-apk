// =====================================================================
//  B316 - CARTRIDGE V JADRE ATARI 130XE (nap_atari_machine.h) NA PC
//
//  Skutecne jadro appky (procesor, ANTIC, GTIA, PIA, OS a BASIC z appky)
//  se zasunutou cartridge - jako po vyberu .car v appce (studeny start,
//  OS cartridge najde a spusti). Male cartridge si test sestavi sam
//  (strojovy kod 6502 nize), jejich program zapisuje vysledky do RAM:
//
//   A) bez cartridge: TRIG3 = 0, $D5xx = plovouci sbernice, BASIC READY
//   B) Standard 8 KB: OS ji spusti (init i start), TRIG3 = 1, zapis do ROM
//      cartridge se ztrati, ANTIC kresli display list a text primo z ROM
//      cartridge, cteni $D500 u obycejne cartridge nic neprepne
//   C) Atarimax 1 MB (typ 42 jako Reneho hra): po zapnuti banka 127, jeji
//      kod ctenim $D500 prepne na banku 0 a skoci do ni; z RAM pak cartridge
//      vypne ($D580): TRIG3 = 0, na $A000 je RAM (zapis z doby, kdy tam byla
//      cartridge, se do RAM nedostal), zapne banku 1 ($D501), TRIG3 = 1;
//      RESET banku necha, POWER vrati banku 127
//   D) Bounty Bob: cteni $8FF7 / zapis $9FF8 prepne 4 kB banky, ANTIC
//      (display list cte $8FF0-$9017) banky neprepina, peek taky ne
//   E) XEGS 32 KB: zapis $D500 = banka na $8000-$9FFF, $A000 = posledni
//      banka, cteni $D500 neprepina
//   F) Reneho Wonderboy_VBXE.car (cesta jako argument): studeny start,
//      hra bezi (prepinani bank, VBXE, obraz neni cerny), snimky PPM
//
//  Preklad:  g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b316_core test_b316_core.cpp
//  Spusteni: ./test_b316_core [Wonderboy_VBXE.car [prefix_snimku]]
// =====================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <chrono>
#include <set>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
using namespace nap;

static int g_chyb = 0, g_ok = 0;
static void over(bool ok, const char *co, long long ma, long long je) {
  if (ok) { g_ok++; std::printf("  OK     %-74s (%lld)\n", co, je); }
  else { g_chyb++; std::printf("  CHYBA  %-74s ma byt %lld, je %lld\n", co, ma, je); }
}
#define OVER_ROVNO(co, ma, je) over((long long)(ma) == (long long)(je), co, (long long)(ma), (long long)(je))

static std::vector<uint8_t> carSoubor(uint32_t typ, const std::vector<uint8_t> &data) {
  std::vector<uint8_t> f(16 + data.size());
  f[0] = 'C'; f[1] = 'A'; f[2] = 'R'; f[3] = 'T';
  f[4] = (uint8_t)(typ >> 24); f[5] = (uint8_t)(typ >> 16); f[6] = (uint8_t)(typ >> 8); f[7] = (uint8_t)typ;
  uint32_t s = 0;
  for (uint8_t b : data) s += b;
  f[8] = (uint8_t)(s >> 24); f[9] = (uint8_t)(s >> 16); f[10] = (uint8_t)(s >> 8); f[11] = (uint8_t)s;
  std::memcpy(&f[16], data.data(), data.size());
  return f;
}
static void vloz(std::vector<uint8_t> &d, size_t off, std::initializer_list<int> b) { for (int x : b) d[off++] = (uint8_t)x; }

// stroj jako v appce po vyberu .car (studenyStartLocked: novy stroj, cartridge, reset)
static Machine *zapni(const std::vector<uint8_t> *car, const char *jm) {
  Machine *m = new Machine();
  m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  if (car) {
    const auto e = m->cart.nactiCar(car->data(), car->size(), jm);
    if (e != Cartridge::OK) { std::printf("  CHYBA  nactiCar %s = %d\n", jm, (int)e); g_chyb++; }
  }
  m->reset();
  m->consol = 7;
  m->sioRychlyTimeout = car != nullptr;      // appka: D1: prazdna -> OS nema na co cekat
  return m;
}
static int bez(Machine &m, int snimku, uint16_t znacka = 0, uint8_t hodnota = 0) {
  for (int f = 0; f < snimku; f++) {
    m.runFrame();
    if (m.cpu.jam) { std::printf("  JAM na $%04X (op $%02X) ve snimku %lld\n", m.cpu.jamPc, m.cpu.jamOp, m.frame); return f; }
    if (znacka && m.mem.ram[znacka] == hodnota) return f;
  }
  return snimku;
}
static std::string radekObrazovky(Machine &m, int r) {
  const int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string s;
  for (int c = 0; c < 40; c++) {
    const int v = m.peek((uint16_t)(sav + r * 40 + c)) & 0x7F;
    int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v);
    if (a < 32 || a > 126) a = ' ';
    s.push_back((char)a);
  }
  return s;
}

// ---------------------------------------------------------------------
static void testBez() {
  std::printf("\nA) BEZ CARTRIDGE (jako dosud)\n");
  Machine *m = zapni(nullptr, "");
  bool ready = false;
  for (int f = 0; f < 400 && !ready; f++) {
    m->runFrame();
    for (int r = 0; r < 24; r++) if (radekObrazovky(*m, r).find("READY") != std::string::npos) ready = true;
  }
  over(ready, "BASIC po zapnuti: READY", 1, ready);
  OVER_ROVNO("TRIG3 ($D013) = 0 (prazdny slot)", 0, m->ioRead(0xD013));
  over(!m->cart.vlozena() && !m->cart.rd4() && !m->cart.rd5(), "slot prazdny, RD4 = RD5 = 0", 1, 1);
  m->busData = 0x5C;
  OVER_ROVNO("cteni $D500 = plovouci sbernice (posledni bajt na sbernici)", 0x5C, m->ioRead(0xD500));
  m->ioWrite(0xD500, 0x12);
  OVER_ROVNO("zapis $D500 nic nezmeni (cartridge zadna)", 0, m->cart.zmen);
  const bool basicVidet = !(m->mem.portB() & 2);
  OVER_ROVNO("$A000 = BASIC ROM (PORTB bit 1 = 0)", basicVidet ? NAP_BASIC_ROM[0] : m->mem.ram[0xA000], m->peek(0xA000));
  m->pokeMem(0x8000, 0x3C);
  OVER_ROVNO("$8000 = RAM (zapis se drzi)", 0x3C, m->peek(0x8000));
  delete m->view; delete m;
}

// ---------------------------------------------------------------------
// B) Standard 8 KB: init $A030 (zapise $11 do $0603), start $A000
static void testStd8() {
  std::printf("\nB) STANDARD 8 KB (typ 1) - OS ji spusti, ANTIC cte z ROM cartridge\n");
  std::vector<uint8_t> d(0x2000, 0xEA);
  vloz(d, 0x0000, {0xA9, 0x42, 0x8D, 0x00, 0x06,          // LDA #$42  STA $0600   (cartridge bezi)
                   0xAD, 0x13, 0xD0, 0x8D, 0x01, 0x06,    // LDA TRIG3 STA $0601
                   0xA9, 0x5A, 0x8D, 0x00, 0xA0,          // LDA #$5A  STA $A000   (zapis do ROM)
                   0xAD, 0x00, 0xA0, 0x8D, 0x02, 0x06,    // LDA $A000 STA $0602
                   0xA9, 0x00, 0x8D, 0x30, 0x02,          // SDLSTL = $B000 (display list v ROM cartridge)
                   0xA9, 0xB0, 0x8D, 0x31, 0x02,
                   0xAD, 0x00, 0xD5, 0x8D, 0x04, 0x06,    // LDA $D500 STA $0604 (obycejna cartridge: nic)
                   0xA9, 0x77, 0x8D, 0x07, 0x06,          // hotovo
                   0x4C, 0x2B, 0xA0});                    // JMP *
  vloz(d, 0x0030, {0xA9, 0x11, 0x8D, 0x03, 0x06, 0x60});  // init: LDA #$11 STA $0603 RTS
  vloz(d, 0x1000, {0x70, 0x70, 0x70, 0x42, 0x00, 0xB1, 0x02, 0x02, 0x41, 0x00, 0xB0});   // DL: 3x mod 2 z $B100
  for (int i = 0; i < 120; i++) d[0x1100 + i] = (uint8_t)(0x21 + (i % 26));            // "ABC..." (interni kody)
  vloz(d, 0x1FFA, {0x00, 0xA0, 0x00, 0x04, 0x30, 0xA0}); // start $A000, cartridge je, flags 4 (start), init $A030
  const std::vector<uint8_t> car = carSoubor(1, d);
  Machine *m = zapni(&car, "std8.car");
  const int f = bez(*m, 400, 0x0607, 0x77);
  over(f < 400, "cartridge spustena (OS skocil na $A000)", 1, f < 400);
  OVER_ROVNO("init cartridge zavolan ($BFFE)", 0x11, m->mem.ram[0x603]);
  OVER_ROVNO("TRIG3 v programu cartridge = 1", 1, m->mem.ram[0x601]);
  OVER_ROVNO("zapis $5A do $A000 se ztratil (ROM), cte se $A9", 0xA9, m->mem.ram[0x602]);
  OVER_ROVNO("RAM pod cartridge zapis nedostala", 1, m->mem.ram[0xA000] != 0x5A);
  OVER_ROVNO("zadne prepnuti banky (obycejna cartridge)", 0, m->cart.zmen);
  bez(*m, 10);
  over(m->dlist >= 0xB000 && m->dlist <= 0xB00B, "ANTIC: display list z ROM cartridge ($B000-$B00A)", 0xB000, m->dlist);
  OVER_ROVNO("ANTIC cte ROM cartridge ($B100 = 'A')", 0x21, m->anticRead(0xB100));
  // text z ROM je opravdu na obraze: pocet bodu barvy pisma (COLPF1 jas na COLPF2)
  const uint8_t pismo = (uint8_t)((m->colpf[2] & 0xF0) | (m->colpf[1] & 0x0E));
  long bodu = 0;
  for (int i = 0; i < AnticView::W * AnticView::H; i++) if ((m->view->idx[i] & 0xFE) == pismo) bodu++;
  over(bodu > 1500, "na obraze 3 radky textu z ROM cartridge (body pisma)", 1500, bodu);
  delete m->view; delete m;
}

// ---------------------------------------------------------------------
// C) Atarimax 1 MB (old) - jako Reneho Wonderboy: po zapnuti banka 127
static void testAtarimax() {
  std::printf("\nC) ATARIMAX 1 MB (typ 42, jako Wonderboy) - banky ctenim $D5xx, vypnuti, RAM pod cartridge, TRIG3\n");
  std::vector<uint8_t> d(1024 * 1024, 0xFF);
  for (int k = 0; k < 128; k++) d[(size_t)k * 0x2000 + 0x800] = (uint8_t)(0xC0 ^ k);   // podpis banky na $A800
  const size_t b127 = 127 * 0x2000;
  vloz(d, b127 + 0x1FF0, {0xAD, 0x00, 0xD5,               // $BFF0: LDA $D500 -> banka 0
                          0x4C, 0x00, 0xA0,               //        JMP $A000 (uz v bance 0)
                          0x60});                         // $BFF6: RTS (init)
  vloz(d, b127 + 0x1FFA, {0xF0, 0xBF, 0x00, 0x04, 0xF6, 0xBF});
  vloz(d, 0x1FF3, {0x4C, 0x00, 0xA0});                    // banka 0, $BFF3: dalsi instrukce se cte uz z banky 0
  // banka 0
  vloz(d, 0x0000, {0xA9, 0x42, 0x8D, 0x00, 0x06,          // LDA #$42 STA $0600
                   0xAD, 0x13, 0xD0, 0x8D, 0x01, 0x06,    // LDA TRIG3 STA $0601
                   0xA9, 0x5A, 0x8D, 0x00, 0xA0,          // STA $A000 (ROM - ztrati se)
                   0xAD, 0x00, 0xA0, 0x8D, 0x02, 0x06,    // LDA $A000 STA $0602
                   0xA2, 0x00,                            // LDX #0
                   0xBD, 0x00, 0xA1, 0x9D, 0x20, 0x06,    // LDA $A100,X STA $0620,X
                   0xE8, 0xE0, 0x40, 0xD0, 0xF5,          // INX CPX #$40 BNE
                   0x4C, 0x20, 0x06});                    // JMP $0620 (dal z RAM)
  // podprogram v RAM ($0620): cartridge vypnout a zase zapnout jinou banku
  vloz(d, 0x0100, {0xA9, 0x00, 0x8D, 0x0E, 0xD4,          // NMIEN = 0 (behem prepinani zadne VBI)
                   0x8D, 0xFA, 0x03,                      // GINTLK = 0 (OS by jinak pri zmene TRIG3 zamrzl)
                   0xAD, 0x80, 0xD5,                      // LDA $D580 -> cartridge vypnuta
                   0xAD, 0x13, 0xD0, 0x8D, 0x03, 0x06,    // TRIG3 -> $0603
                   0xAD, 0x01, 0xD3, 0x09, 0x02, 0x8D, 0x01, 0xD3,   // PORTB bit 1 = 1 (BASIC vypnout)
                   0xAD, 0x00, 0xA0, 0x8D, 0x04, 0x06,    // $A000 (RAM) -> $0604
                   0xAD, 0x01, 0xD5,                      // LDA $D501 -> banka 1
                   0xAD, 0x00, 0xA8, 0x8D, 0x05, 0x06,    // podpis banky -> $0605
                   0xAD, 0x13, 0xD0, 0x8D, 0x06, 0x06,    // TRIG3 -> $0606
                   0xA9, 0x01, 0x8D, 0xFA, 0x03,          // GINTLK = 1
                   0xA9, 0x40, 0x8D, 0x0E, 0xD4,          // NMIEN = $40
                   0xA9, 0x77, 0x8D, 0x07, 0x06,          // hotovo
                   0x4C, 0x5D, 0x06});                    // JMP $065D
  const std::vector<uint8_t> car = carSoubor(42, d);
  Machine *m = zapni(&car, "atarimax.car");
  OVER_ROVNO("po zapnuti banka 127 (stav $7F)", 0x7F, m->cart.stav);
  const int f = bez(*m, 400, 0x0607, 0x77);
  over(f < 400, "OS spustil banku 127, ta prepla na banku 0 a vse dobehlo", 1, f < 400);
  OVER_ROVNO("kod v bance 0 bezel", 0x42, m->mem.ram[0x600]);
  OVER_ROVNO("TRIG3 s cartridge = 1", 1, m->mem.ram[0x601]);
  OVER_ROVNO("zapis do ROM cartridge se ztratil", 0xA9, m->mem.ram[0x602]);
  OVER_ROVNO("po $D580 je cartridge vypnuta: TRIG3 = 0", 0, m->mem.ram[0x603]);
  OVER_ROVNO("na $A000 je pak RAM (stejna jako v pameti)", m->mem.ram[0xA000], m->mem.ram[0x604]);
  over(m->mem.ram[0xA000] != 0x5A, "zapis $5A z doby cartridge se do RAM nedostal", 1, m->mem.ram[0xA000] != 0x5A);
  OVER_ROVNO("po $D501 banka 1 (podpis $C1)", 0xC1, m->mem.ram[0x605]);
  OVER_ROVNO("TRIG3 zase 1", 1, m->mem.ram[0x606]);
  OVER_ROVNO("stav cartridge = 1", 1, m->cart.stav);
  const int rt = m->mem.ram[0x14];
  bez(*m, 50);
  OVER_ROVNO("OS bezi dal: VBI za 50 snimku 50x (RTCLOK)", 50, (m->mem.ram[0x14] - rt + 256) % 256);
  // RESET: slot 130XE nema resetovaci linku - banka zustane; POWER ji vrati
  m->reset();
  OVER_ROVNO("RESET: banka zustala (stav 1)", 1, m->cart.stav);
  m->cart.studenyStart();
  OVER_ROVNO("POWER (studeny start cartridge): banka 127", 0x7F, m->cart.stav);
  delete m->view; delete m;
}

// ---------------------------------------------------------------------
// D) Bounty Bob Strikes Back (typ 18): 4x4 kB na $8000, 4x4 kB na $9000, 8 kB na $A000
static void testBountyBob() {
  std::printf("\nD) BOUNTY BOB 40 KB (typ 18) - procesor prepina banky na $8FF6-$9FF9, ANTIC ne\n");
  std::vector<uint8_t> d(40 * 1024, 0x00);
  for (int k = 0; k < 4; k++) {
    d[(size_t)k * 0x1000] = (uint8_t)(0x80 + k);
    d[0x4000 + (size_t)k * 0x1000] = (uint8_t)(0x90 + k);
    d[0x4000 + (size_t)k * 0x1000 + 0xFFC] = 0xFF;     // $9FFC != 0: neni "cartridge B"
  }
  vloz(d, 0x8000, {0xA9, 0x42, 0x8D, 0x00, 0x06,
                   0xAD, 0x00, 0x80, 0x8D, 0x01, 0x06,    // $8000 -> $0601 (banka 0)
                   0xAD, 0x00, 0x90, 0x8D, 0x02, 0x06,    // $9000 -> $0602
                   0xAD, 0xF7, 0x8F,                      // LDA $8FF7 -> $8000 banka 1
                   0xAD, 0x00, 0x80, 0x8D, 0x03, 0x06,
                   0x8D, 0xF8, 0x9F,                      // STA $9FF8 -> $9000 banka 2
                   0xAD, 0x00, 0x90, 0x8D, 0x04, 0x06,
                   0xA9, 0x00, 0x8D, 0x30, 0x02, 0xA9, 0xB0, 0x8D, 0x31, 0x02,   // DL na $B000
                   0xA9, 0x77, 0x8D, 0x07, 0x06,
                   0x4C, 0x32, 0xA0});
  vloz(d, 0x8035, {0x60});
  vloz(d, 0x9000, {0x70, 0x70, 0x70, 0x42, 0xF0, 0x8F, 0x41, 0x00, 0xB0});   // mod 2 z $8FF0: ANTIC cte $8FF6-$8FF9
  vloz(d, 0x9FFA, {0x00, 0xA0, 0x00, 0x04, 0x35, 0xA0});
  const std::vector<uint8_t> car = carSoubor(18, d);
  Machine *m = zapni(&car, "bountybob.car");
  const int f = bez(*m, 400, 0x0607, 0x77);
  over(f < 400, "cartridge spustena", 1, f < 400);
  OVER_ROVNO("po zapnuti $8000 = banka 0", 0x80, m->mem.ram[0x601]);
  OVER_ROVNO("po zapnuti $9000 = banka 0", 0x90, m->mem.ram[0x602]);
  OVER_ROVNO("cteni $8FF7 -> $8000 banka 1", 0x81, m->mem.ram[0x603]);
  OVER_ROVNO("zapis $9FF8 -> $9000 banka 2", 0x92, m->mem.ram[0x604]);
  const long long zmenPred = m->cart.zmen;
  bez(*m, 30);
  over(m->dlist >= 0xB000 && m->dlist <= 0xB008, "ANTIC kresli radek z $8FF0-$9017 (display list $B000)", 0xB000, m->dlist);
  OVER_ROVNO("ANTIC (30 snimku cteni $8FF6-$8FF9) banku neprepnul - stav $09", 0x09, m->cart.stav);
  OVER_ROVNO("zadne dalsi prepnuti", zmenPred, m->cart.zmen);
  m->peek(0x8FF9); m->peek(0x9FF6);
  OVER_ROVNO("peek (LOG/CHYBA, zavadec) banku neprepne", 0x09, m->cart.stav);
  delete m->view; delete m;
}

// ---------------------------------------------------------------------
// E) XEGS 32 KB (typ 12): $8000 = banka podle zapisu $D500, $A000 = banka 3
static void testXegs() {
  std::printf("\nE) XEGS 32 KB (typ 12) - zapis $D500 vybere banku na $8000, cteni neprepina\n");
  std::vector<uint8_t> d(32 * 1024, 0x00);
  for (int k = 0; k < 3; k++) { d[(size_t)k * 0x2000] = (uint8_t)(0xE0 + k); d[(size_t)k * 0x2000 + 0x1FFC] = 0xFF; }
  const size_t b3 = 3 * 0x2000;
  vloz(d, b3, {0xA9, 0x42, 0x8D, 0x00, 0x06,
               0xAD, 0x00, 0x80, 0x8D, 0x01, 0x06,        // $8000 -> $0601 (banka 0)
               0xA9, 0x02, 0x8D, 0x00, 0xD5,              // STA $D500 = 2
               0xAD, 0x00, 0x80, 0x8D, 0x02, 0x06,        // -> $0602 (banka 2)
               0xAD, 0x00, 0xD5,                          // LDA $D500 (neprepina)
               0xAD, 0x00, 0x80, 0x8D, 0x03, 0x06,
               0xAD, 0x13, 0xD0, 0x8D, 0x04, 0x06,
               0xA9, 0x77, 0x8D, 0x07, 0x06,
               0x4C, 0x2A, 0xA0});
  vloz(d, b3 + 0x40, {0x60});
  vloz(d, b3 + 0x1FFA, {0x00, 0xA0, 0x00, 0x04, 0x40, 0xA0});
  const std::vector<uint8_t> car = carSoubor(12, d);
  Machine *m = zapni(&car, "xegs.car");
  const int f = bez(*m, 400, 0x0607, 0x77);
  over(f < 400, "cartridge spustena (kod v posledni bance na $A000)", 1, f < 400);
  OVER_ROVNO("po zapnuti $8000 = banka 0", 0xE0, m->mem.ram[0x601]);
  OVER_ROVNO("zapis $D500 = 2 -> $8000 banka 2", 0xE2, m->mem.ram[0x602]);
  OVER_ROVNO("cteni $D500 neprepina (porad banka 2)", 0xE2, m->mem.ram[0x603]);
  OVER_ROVNO("TRIG3 = 1", 1, m->mem.ram[0x604]);
  over(m->cart.rd4() && m->cart.rd5(), "RD4 i RD5 (16 kB okno $8000-$BFFF)", 1, m->cart.rd4() && m->cart.rd5());
  delete m->view; delete m;
}

// ---------------------------------------------------------------------
// F) skutecna hra z .car (Reneho Wonderboy_VBXE.car)
static void ulozPpm(Machine &m, const std::string &fn) {
  FILE *o = std::fopen(fn.c_str(), "wb");
  if (!o) return;
  std::fprintf(o, "P6\n%d %d\n255\n", AnticView::FW, AnticView::H);
  for (int i = 0; i < AnticView::FW * AnticView::H; i++) {
    const uint32_t v = m.view->fb[i];
    const uint8_t px[3] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF), (uint8_t)((v >> 16) & 0xFF)};
    std::fwrite(px, 1, 3, o);
  }
  std::fclose(o);
}
static void testSkutecna(const char *cesta, const char *prefix) {
  std::printf("\nF) SKUTECNA HRA: %s\n", cesta);
  std::vector<uint8_t> f;
  {
    FILE *fp = std::fopen(cesta, "rb");
    if (!fp) { std::printf("  CHYBA  soubor nejde otevrit\n"); g_chyb++; return; }
    uint8_t b[65536]; size_t n;
    while ((n = std::fread(b, 1, sizeof b, fp)) > 0) f.insert(f.end(), b, b + n);
    std::fclose(fp);
  }
  Machine *m = zapni(&f, "Wonderboy_VBXE.car");
  std::printf("  typ %d (%s), %d kB, kontrolni soucet %s, po zapnuti stav $%X, RD4 %d RD5 %d\n", m->cart.typ, cartJmeno(m->cart.typ),
              cartKb(m->cart.typ), m->cart.soucetHlavicka == m->cart.soucetDat ? "sedi" : "NESEDI", (unsigned)m->cart.stav, m->cart.rd4(), m->cart.rd5());
  // snimky: kdy se co deje; FIRE / START jako hrac (titulek -> hra)
  const int SNIMKU = 3000;
  std::set<int> uloz = {50, 150, 300, 600, 900, 1200, 1500, 2000, 2500, 3000};
  long long pcCart = 0, pcRam = 0, pcOs = 0;
  std::set<int> banky;
  long long vbxeZapisuPred = 0;
  const auto t0 = std::chrono::steady_clock::now();
  for (int fr = 1; fr <= SNIMKU; fr++) {
    // hrac: START ve 400, FIRE 700-705, 1000-1005 ..., joystick doprava 1100-1400, nahoru 1450-1460
    m->consol = (fr >= 400 && fr < 406) ? 6 : 7;
    m->trig[0] = ((fr % 300) >= 100 && (fr % 300) < 106 && fr >= 700) ? 0 : 1;
    int joy = 0;
    if (fr >= 1100 && fr < 1400) joy |= 8;
    if (fr >= 1450 && fr < 1460) joy |= 1;
    if (fr >= 1700 && fr < 2100) joy |= 8;
    m->porta = 0xF0 | (~joy & 15);
    const long long f0 = m->frame;
    while (m->frame == f0) {
      const uint16_t pc = m->cpu.pc;
      if (pc >= 0x8000 && pc < 0xC000 && m->cart.okno[(pc >> 12) - 8] != Cartridge::NIC) pcCart++;
      else if (pc >= 0xC000 || (pc >= 0x5000 && pc < 0x5800)) pcOs++;
      else pcRam++;
      if (m->sioPatchTed()) { m->sioPatch(); continue; }
      m->cpu.step();
      if (m->cpu.jam) break;
    }
    if (m->keyHoldFrames > 0 && --m->keyHoldFrames == 0) m->skstat |= 0x04;
    banky.insert(m->cart.stav);
    if (m->cpu.jam) { std::printf("  JAM na $%04X (op $%02X) ve snimku %d\n", m->cpu.jamPc, m->cpu.jamOp, fr); break; }
    if (uloz.count(fr)) {
      long svetlych = 0; std::set<uint32_t> barev;
      for (int i = 0; i < AnticView::FW * AnticView::H; i++) {
        const uint32_t v = m->view->fb[i] & 0xFFFFFF;
        barev.insert(v);
        if (((v & 0xFF) + ((v >> 8) & 0xFF) + ((v >> 16) & 0xFF)) > 60) svetlych++;
      }
      std::printf("  snimek %4d: banka $%02X (prepnuti %lld), VBXE zapisu %lld (+%lld), XDL %s, blitu %lld, barev na obraze %zu, "
                  "svetlych bodu %ld, PC $%04X\n", fr, (unsigned)m->cart.stav, m->cart.zmen, m->vbx.nWrites, m->vbx.nWrites - vbxeZapisuPred,
                  m->vbx.xdlEnabled ? "zap" : "vyp", m->vbx.nBlits, barev.size(), svetlych, m->cpu.pc);
      vbxeZapisuPred = m->vbx.nWrites;
      if (prefix) ulozPpm(*m, std::string(prefix) + "_" + std::to_string(fr) + ".ppm");
    }
  }
  const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
  std::printf("  %d snimku za %.1f s (%.0f snimku/s; Atari ma 50/s)\n", SNIMKU, s, SNIMKU / s);
  std::printf("  procesor: v ROM cartridge %lld krat, v RAM %lld, v OS %lld; ruznych stavu banky %zu\n", pcCart, pcRam, pcOs, banky.size());
  over(!m->cpu.jam, "procesor nezamrzl (zadny JAM)", 1, !m->cpu.jam);
  over(m->cart.zmen > 0, "hra prepina banky cartridge (ctenim/zapisem $D5xx)", 1, m->cart.zmen);
  over(pcCart > 1000, "procesor bezi v ROM cartridge", 1000, pcCart);
  over(m->vbx.nWrites > 100, "hra pouziva VBXE (zapisy do registru $D640-)", 100, m->vbx.nWrites);
  long svetlych = 0; std::set<uint32_t> barev;
  for (int i = 0; i < AnticView::FW * AnticView::H; i++) {
    const uint32_t v = m->view->fb[i] & 0xFFFFFF;
    barev.insert(v);
    if (((v & 0xFF) + ((v >> 8) & 0xFF) + ((v >> 16) & 0xFF)) > 60) svetlych++;
  }
  over(barev.size() > 8 && svetlych > 5000, "obraz na konci neni cerny (barvy / svetle body)", 8, (long long)barev.size());
  delete m->view; delete m;
}

int main(int argc, char **argv) {
  std::printf("B316 - cartridge v jadre Atari 130XE (skutecne nap_atari_machine.h, OS a BASIC z appky)\n");
  testBez();
  testStd8();
  testAtarimax();
  testBountyBob();
  testXegs();
  if (argc > 1) testSkutecna(argv[1], argc > 2 ? argv[2] : nullptr);
  else std::printf("\nF) skutecna hra: preskoceno (cesta k .car jako 1. argument)\n");
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
