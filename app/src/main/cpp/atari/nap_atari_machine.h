// nap_atari_machine.h
// B292: Atari 130XE (PAL) - NOVE JADRO PRESNE PO CYKLECH.
//
// Rene po B291: "nektere hry maji chybu v grafice ... pro jistotu udelej
// dukladnou kontrolu jadra". Kontrola starsiho jadra (B287-B291) nasla:
//  - ANTIC nebral procesoru zadne cykly (na skutecnem Atari si pro obraz
//    bere az ~70 % cyklu na radku) - casovani her bylo jine,
//  - preruseni DLI chodilo o radek pozde (az po celem radku),
//  - bity svisleho a vodorovneho rolovani v display listu byly PROHOZENE
//    (bit 5 = svisle, bit 4 = vodorovne) a konec svisleho rolovani chybel,
//  - kolize hracu a strel vracely vzdy "srazka se vsim" ($0F),
//  - priority PRIOR, ctvrty/paty hrac, vicebarevni hraci a rezimy GTIA
//    9/10/11 chybely, CHACTL (inverze/blikani) se ignoroval,
//  - zapis do registru se pocital od ZACATKU instrukce, ne od cyklu zapisu.
//
// Tohle jadro je postavene znovu podle chovani skutecneho hardwaru
// (Altirra Hardware Reference Manual a zdrojovy kod emulatoru Altirra jako
// zdroj FAKTU o casovani - kod je napsany znovu a jednoduse):
//  - 6502 (nap_atari_6502.h): kazdy pristup na sbernici = 1 cyklus,
//    overeno 2 560 000 testy SingleStepTests,
//  - ANTIC: DMA po jednotlivych cyklech (strely 0, DL 1, hraci 2-5, LMS
//    6-7, obnova pameti 25..57, hraci pole podle rezimu/sirky/HSCROL),
//    NMI na cyklu 8 (DLI/VBI), WSYNC uvolni na cyklu 105, VCOUNT,
//  - GTIA: kresli po barevnych taktech (228 na radek), zmena registru
//    se projevi presne na taktu, kdy ji procesor zapsal (+zpozdeni
//    GTIA), hraci/strely jako posuvne registry (HPOS, SIZE, VDELAY),
//    prioritni logika rovnicemi z GTIA, kolize, rezimy 9/10/11,
//  - POKEY: citace kanalu po cyklech (1,79 MHz / 64 kHz / 15 kHz, 16bit
//    spojeni, 3 cykly "borrow"), preruseni casovacu, seriovy port
//    (SEROUT po bitech podle casovace, dvoutonovy rezim pro kazetu,
//    prijem SERIN), klavesnice (KBCODE, SKSTAT), RANDOM z poly17/9,
//  - PIA (PORTA/PORTB, PACTL - motor kazety, PBCTL), MMU 130XE
//    (OS/BASIC/self-test ROM, 4 rozsirene banky, oddelene CPU/ANTIC).
#pragma once
#include <cstdint>
#include <cstring>
#include <cmath>
#include <vector>
#include <string>
#include <cstdio>
#include "nap_atari_6502.h"
#include "nap_atari_tape.h"

namespace nap {

// ---------------------------------------------------------------------
//  Paleta (beze zmeny od B287 - viz komentar tam; Rene ji nerozporoval)
// ---------------------------------------------------------------------
inline uint32_t napAtariPalette(int v) {
  v &= 0xFE;
  const int hue = (v >> 4) & 15;
  const int lum = v & 15;
  const double Y = (lum / 14.0) * 0.93;
  double r, g, b;
  if (hue == 0) {
    r = g = b = Y;
  } else {
    const double PI = 3.14159265358979323846;
    const double faze = (190.0 - 27.0 * hue) * PI / 180.0;
    const double SYTOST = 0.30;
    const double U = SYTOST * std::cos(faze);
    const double V = SYTOST * std::sin(faze);
    r = Y + 1.140 * V;
    g = Y - 0.395 * U - 0.581 * V;
    b = Y + 2.032 * U;
  }
  auto cl = [](double x) { int t = (int)(x * 255.0 + 0.5); return t < 0 ? 0 : (t > 255 ? 255 : t); };
  return 0xFF000000u | ((uint32_t)cl(b) << 16) | ((uint32_t)cl(g) << 8) | (uint32_t)cl(r);
}

// Obraz: 384 x 240 bodu (pul barevneho taktu = 1 bod), barevne takty 32..223
// a radky 8..247 - stejny vyrez jako Screen_atari v emulatoru atari800.
struct AnticView {
  static const int W = 384;
  static const int H = 240;
  uint32_t fb[W * H];      // RGBA (A<<24 | B<<16 | G<<8 | R)
  uint8_t idx[W * H];      // barevny kod Atari (pred paletou) - pro testy
  void vymaz(int c) { uint32_t v = napAtariPalette(c); for (int i = 0; i < W * H; i++) { fb[i] = v; idx[i] = (uint8_t)c; } }
};

struct Pia { int orA = 0, ddrA = 0, orB = 0, ddrB = 0, ctlA = 0, ctlB = 0; };

struct AtariMem {
  uint8_t ram[65536];
  uint8_t ext[65536];                 // 130XE: 4 banky po 16 kB ($4000-$7FFF)
  const uint8_t *os = nullptr;        // 16 kB OS ROM
  const uint8_t *bas = nullptr;       // 8 kB BASIC ROM
  Pia pia;
  AtariMem() { std::memset(ram, 0, sizeof ram); std::memset(ext, 0, sizeof ext); }
  inline int portB() const { return (pia.orB | (~pia.ddrB)) & 0xFF; }
};

// ---------------------------------------------------------------------
//  ATR disketa (pro mechaniku D1: - SIO na urovni prikazu)
// ---------------------------------------------------------------------
struct AtrDisk {
  std::vector<uint8_t> data;          // cely obraz bez 16B hlavicky
  int sectorSize = 128;
  int sectors = 0;
  bool mounted = false;
  bool writeProtect = false;
  std::string name;
  long long reads = 0, writes = 0;
  bool load(const uint8_t *d, size_t n, const std::string &nm) {
    mounted = false; data.clear();
    if (!d || n < 16 + 128 || d[0] != 0x96 || d[1] != 0x02) return false;
    size_t paras = (size_t)(d[2] | (d[3] << 8)) | ((size_t)d[6] << 16);
    sectorSize = d[4] | (d[5] << 8);
    if (sectorSize != 128 && sectorSize != 256) sectorSize = 128;
    size_t bytes = paras * 16;
    if (bytes > n - 16) bytes = n - 16;
    data.assign(d + 16, d + 16 + bytes);
    // sektory 1-3 jsou vzdy 128 B (i u dvojite hustoty)
    if (sectorSize == 256) sectors = (int)(bytes >= 384 ? (bytes - 384) / 256 + 3 : bytes / 128);
    else sectors = (int)(bytes / 128);
    name = nm; mounted = true; reads = writes = 0;
    return sectors > 0;
  }
  // offset a delka sektoru (1..n)
  bool sec(int s, size_t &off, int &len) const {
    if (s < 1 || s > sectors) return false;
    if (sectorSize == 256) {
      if (s <= 3) { off = (size_t)(s - 1) * 128; len = 128; }
      else { off = 384 + (size_t)(s - 4) * 256; len = 256; }
    } else { off = (size_t)(s - 1) * 128; len = 128; }
    if (off + (size_t)len > data.size()) return false;
    return true;
  }
};

// ---------------------------------------------------------------------
//  Kazetovy magnetofon (CLOAD): na pasce jsou urovne linky SIO DATA IN
//  (mark=1 / space=0) ziskane z WAV demodulaci FSK (nap_atari_tape.h).
//  Pasek se posouva jen pri zapnutem motoru (PACTL) a stisknutem PLAY.
// ---------------------------------------------------------------------
struct TapeDeck {
  TapeImage img;
  double pos = 0;                     // pozice ve vzorcich pasky
  bool loaded = false;
  bool play = false;                  // tlacitko PLAY na kazetaku
  std::string name;
  int line = 1;                       // aktualni uroven linky DATA IN
  // prijimac POKEY (seriovy vstup) - rychlost z AUDF3/AUDF4 jako na Atari
  int rxPhase = 0;                    // 0 ceka na start bit, 1 start, 2..9 data, 10 stop
  double rxNext = 0;                  // cyklus dalsiho vzorkovani bitu
  int rxByte = 0;
  long long bytes = 0, framing = 0;   // diagnostika
  void eject() { img = TapeImage(); pos = 0; loaded = false; name.clear(); line = 1; rxPhase = 0; bytes = framing = 0; }
};

class Machine {
public:
  AtariMem mem;
  Cpu6502T<Machine> cpu;
  AnticView *view = nullptr;
  const uint8_t *osRom = nullptr;     // jen pro kompatibilitu
  const uint8_t *basRom = nullptr;

  // ---------------- casovani ----------------
  uint64_t cyc = 0;                   // absolutni cyklus
  int x = 0;                          // cyklus v radku 0..113
  int line = 0;                       // radek 0..311
  long long frame = 0;
  static const int LINES = 312;
  static const int CYCLES_PER_LINE = 114;

  // ---------------- vstupy (konzole, joysticky) ----------------
  int consol = 7;                     // stisknuto = 0 v bitu (START=1, SELECT=2, OPTION=4)
  // TRIG0/1 = tlacitka joysticku 1/2. 130XE ma jen 2 porty: TRIG2 = 1 (nic),
  // TRIG3 = SNIMANI CARTRIDGE (linka RD5): 0 = ve slotu neni cartridge
  // (vestaveny BASIC se nepocita). Na Atari 800 byl TRIG3 tlacitko 4.
  // joysticku - s hodnotou 1 si programy mysli, ze je zasunuta cartridge
  // (napr. M.U.L.E. pak nefunguje).
  int trig[4] = {1, 1, 1, 0};
  int porta = 0xFF;                   // joysticky (1 = nestisknuto)

  // ---------------- kompatibilita se starym API ----------------
  long long seroutPocet = 0;
  bool sioRychlyTimeout = false;
  long long sioZkratek = 0;
  int gtiaSpeakerBit = 1;
  long long gtiaKlikPocitadlo = 0;
  int dlKroku = 0;
  int audf[4] = {0, 0, 0, 0};
  int audc[4] = {0, 0, 0, 0};
  int audctl = 0;
  AtrDisk disk;
  TapeDeck tape;
  long long sioPrikazu = 0;

  Machine() { cpu.bus = this; coldInit(); }

  // =================================================================
  //  SBERNICE PRO PROCESOR - kazde volani = jeden cyklus
  // =================================================================
  inline uint8_t rd(uint16_t a) {
    beginCpuCycle(false);
    uint8_t v = cpuRead(a);
    busData = v;
    endCycle();
    return v;
  }
  inline void wr(uint16_t a, uint8_t v) {
    beginCpuCycle(true);
    busData = v;
    cpuWrite(a, v);
    endCycle();
  }
  inline bool nmiPending() const { return nmiLatch; }
  inline void nmiAck() { nmiLatch = false; }
  inline bool irqActive() const { return irqLine; }
  inline void traceNmi() { if (traceFrame == frame) std::fprintf(stderr, "NMI y=%d x=%d\n", line, x); }

  // stara jmena (runtime/XEX/testy)
  int read(int a) { return peek((uint16_t)a); }
  void write(int a, int v) { pokeMem((uint16_t)a, (uint8_t)v); }
  int dmactl() const { return dmactlReg; }
  int dlistAddr() const { return dlist; }

  // =================================================================
  //  START / RESET
  // =================================================================
  // Obsah DRAM po zapnuti 130XE (zmereno na skutecnem 130XE - Altirra
  // "DRAM pattern B"): bloky po 64 bajtech se stridaji 80 FF 80 FF... a
  // 00 7F 00 7F... - plati pro zakladni i rozsirenou pamet. (800XL ma jiny
  // vzor FF 00 FF 00.)
  static void dram130xe(uint8_t *d, size_t n) {
    for (size_t i = 0; i < n; i++) {
      const bool faze = ((i >> 6) & 1) != 0;
      d[i] = (i & 1) ? (faze ? 0x7F : 0xFF) : (faze ? 0x00 : 0x80);
    }
  }
  void coldInit() {
    cyc = 0; x = 0; line = 0; frame = 0;
    dram130xe(mem.ram, sizeof mem.ram);
    dram130xe(mem.ext, sizeof mem.ext);
    mem.pia = Pia();
    // ANTIC
    dmactlReg = 0; chactl = 0; dlist = 0; hscrol = 0; vscrol = 0; pmbase = 0; chbase = 0; chbaseReg = 0;
    nmien = 0; nmist = 0x1F; nmiLatch = false;
    dlControl = dlControlPrev = 0; rowCounter = 0; rowCount = 1; rowStopUseVScroll = false;
    latchedVScroll = latchedVScroll2 = 0; dlActive = false; dlExtraLoads = false; dlDmaInTime = false;
    pfBase = 0; pfOffset = 0; pfDmaEnabled = pfDmaActive = false; hscrollEnabled = hscrollDelay = false; hscrollDmaOffset = 0;
    pfWidth = 0; wsyncPending = 0; rdyHalt = false; chbaseDelay = 0;
    pendingNMIs = 0; earlyNMIEN = 0; lateNMI = false; charInvert = 0; charBlink = 0xFF;
    std::memset(dmaPat, 0, sizeof dmaPat);
    updatePlayfieldTiming();
    // GTIA
    std::memset(gReg, 0, sizeof gReg);
    for (int i = 0; i < 8; i++) { spr[i] = Sprite(); sprPos[i] = 0; }
    prior = 0; vdelay = 0; gractl = 0; consolOut = 8; gtiaSpeakerBit = 1;
    std::memset(collP, 0, sizeof collP); std::memset(collM, 0, sizeof collM);
    for (int i = 0; i < 4; i++) { colpm[i] = colpf[i] = 0; }
    colbk = 0; rcN = 0; lastSyncCc = 0;
    // POKEY
    pokeyCold();
    // CPU
    cpu.powerOn();
    irqLine = false;
  }

  // RESET (tlacitko RESET; po POWER ho vola volajici po nastaveni ROM).
  // 130XE: tlacitko RESET je primo na resetovaci lince - resetuje procesor,
  // ANTIC (DMACTL, NMIEN), PIA a tim i MMU (FREDDIE): PORTB je po resetu
  // vstup = $FF -> OS ROM zapnuta, BASIC a rozsirena pamet vypnute, OS si je
  // pri teplem startu znovu nastavi (BASICF). (Atari 400/800 to delaly jinak:
  // RESET tam byl jen NMI pres ANTIC a PIA zustavala.)
  void reset() {
    dmactlReg = 0; nmien = 0; updatePlayfieldTiming();
    wsyncPending = 0; rdyHalt = false; nmiLatch = false;
    mem.pia = Pia();
    cpu.reset();
  }

  void breakKey() { if (irqen & 0x80) { irqst &= ~0x80; updateIrq(); } }
  // stisk klavesy (scankod 0-63 + $40 SHIFT + $80 CONTROL)
  // drzet=true: klavesa zustane stisknuta az do klavesaPustena() (prst na
  // displeji - OS pak sam opakuje znak jako na skutecne klavesnici);
  // jinak se pusti sama po ~4 snimcich (psani textu z TXT).
  void klavesa(int kod, bool drzet = false) {
    kbcode = kod & 0xFF;
    skstat &= ~0x04;                    // klavesa drzena
    keyHoldFrames = drzet ? -1 : 4;
    if (irqen & 0x40) { irqst &= ~0x40; updateIrq(); }
  }
  void klavesaPustena() { skstat |= 0x04; keyHoldFrames = 0; }
  void shiftDrzen(bool d) { if (d) skstat &= ~0x08; else skstat |= 0x08; }

  // je motor kazety zapnuty? (PACTL CA2 vystup = 0)
  bool motorOn() const { return (mem.pia.ctlA & 0x38) == 0x30; }

  // =================================================================
  //  BEH
  // =================================================================
  void runFrame() {
    const long long f = frame;
    while (frame == f) {
      if (cpu.pc == 0xE459 && !cpu.jam && (disk.mounted || sioRychlyTimeout) && !cpu.takeNmi && !cpu.takeIrq) { sioPatch(); continue; }
      cpu.step();
    }
    if (keyHoldFrames > 0 && --keyHoldFrames == 0) skstat |= 0x04;
  }
  void runScanline() { const int l = line; while (line == l) cpu.step(); }

  // =================================================================
  //  ZVUK: vzorky za posledni usek behu (volat po runFrame)
  // =================================================================
  void genAudio(float *out, int n, double sampleRateHz) {
    (void)sampleRateHz;
    integrate(audEv, audLevelStart, audFrom, cyc, out, n, true);
    audLevelStart = audLevel;
    audFrom = cyc;
    audEv.clear();
  }
  // kazetovy vystup (SIO DATA OUT v dvoutonovem rezimu) - pro CSAVE WAV.
  // Sbira se jen pri tapeCapture=true (jinak by fronta zmen rostla).
  bool tapeCapture = false;
  void genTape(float *out, int n) {
    integrate(tapeEv, tapeLevelStart, tapeFrom, cyc, out, n, false);
    tapeLevelStart = tapeLevel;
    tapeFrom = cyc;
    tapeEv.clear();
  }
  void srovnatSledovaniZvuku() { audEv.clear(); audFrom = cyc; audLevelStart = audLevel; tapeEv.clear(); tapeFrom = cyc; tapeLevelStart = tapeLevel; }

  // =================================================================
  //  PAMET
  // =================================================================
  // cteni bez vedlejsich ucinku (zavadec, testy)
  uint8_t peek(uint16_t a) {
    if (a >= 0xD000 && a < 0xD800) return 0xFF;
    return memRead(a, false);
  }
  void pokeMem(uint16_t a, uint8_t v) {
    if (a >= 0xD000 && a < 0xD800) { ioWrite(a, v); return; }
    memWrite(a, v);
  }

  // =================================================================
  //  INTERNI STAV
  // =================================================================
  // ---- sbernice / preruseni ----
  uint8_t busData = 0xFF;
  bool nmiLatch = false;
  bool irqLine = false;

  // ---- ANTIC ----
  uint8_t dmactlReg = 0, chactl = 0, hscrol = 0, vscrol = 0, pmbase = 0, chbase = 0, chbaseReg = 0, nmien = 0, nmist = 0x1F;
  uint16_t dlist = 0, dlistLatch = 0;
  uint8_t dlControl = 0, dlControlPrev = 0, dlNext = 0;
  int rowCounter = 0, rowCount = 1; bool rowStopUseVScroll = false; int latchedVScroll = 0, latchedVScroll2 = 0;
  bool dlActive = false, dlExtraLoads = false, dlDmaInTime = false;
  uint16_t pfBase = 0, pfOffset = 0;
  bool pfDmaEnabled = false, pfDmaActive = false, hscrollEnabled = false, hscrollDelay = false; int hscrollDmaOffset = 0;
  int pfWidth = 0, pfFetchWidth = 0;
  int pfDmaStart = 114, pfDmaEnd = 114, pfDmaVEnd = 114, pfDisplayStart = 110, pfDisplayEnd = 110;
  uint8_t dmaPat[116];
  uint8_t pfData[128], pfChar[128]; int pfDataW = 0, pfDataR = 0, pfCharW = 0;
  uint16_t charBase128 = 0, charBase64 = 0, charFetchPtr = 0; uint8_t charMask = 0x7F, charInvert = 0, charBlink = 0xFF;
  int pushMode = 0;                    // 0 prazdne, 1 = 160 bodu, 2 = 320 bodu (hires)
  int displayDone = 0;                 // do ktereho cyklu uz bylo hraci pole predano GTIA
  uint8_t pendingNMIs = 0, earlyNMIEN = 0; bool lateNMI = false;
  int wsyncPending = 0; bool rdyHalt = false, rdyStalled = false;
  int chbaseDelay = 0; uint8_t chbaseNew = 0;

  // ---- GTIA ----
  enum : uint8_t { PF0 = 1, PF1 = 2, PF2 = 4, PF3 = 8, P0 = 16, P1 = 32, P2 = 64, P3 = 128 };
  struct Sprite { uint8_t shift = 0, latch = 0, size = 0, state = 0; };
  Sprite spr[8];                       // 0-3 hraci, 4-7 strely
  int sprPos[8] = {0};
  uint8_t gReg[32];
  uint8_t colpm[4] = {0}, colpf[4] = {0}, colbk = 0, prior = 0, vdelay = 0, gractl = 0, consolOut = 8;
  uint8_t collP[4], collM[4];
  uint8_t merge[240];                  // na barevny takt: PF/P bity
  uint8_t anData[240];                 // na barevny takt: 2 bity (hires / GTIA rezimy)
  uint8_t lineOut[480];                // vystup radku (pul taktu = bod)
  bool lineHires = false, vblankLine = true;
  int lastSyncCc = 0;
  struct RegChange { int pos; uint8_t reg, val; };
  RegChange rc[256]; int rcN = 0;

  // ---- POKEY ----
  uint8_t audfP1[4] = {1, 1, 1, 1};
  int pcnt[4] = {1, 1, 1, 1}, pborrow[4] = {0, 0, 0, 0};
  uint8_t chOut[4] = {0, 0, 0, 0};     // vystupni klopne obvody kanalu
  uint8_t hpf[2] = {0, 0};             // horni propust (AUDCTL bity 2,1)
  uint8_t irqen = 0, irqst = 0xFF, skctl = 0, skstat = 0xFF, kbcode = 0xFF, serin = 0xFF, serout = 0;
  uint64_t last64 = 0, last15 = 0, polyBase = 0, polyShutOff = 0;
  int stimerDelay = 0;
  int serOutCounter = 0; bool serOutValid = false, serOutState = true; uint8_t serOutShift = 0;
  int serOutTickDelay = 0;
  int keyHoldFrames = 0;
  double audLevel = 0, audLevelStart = 0, tapeLevel = 0, tapeLevelStart = 0;
  uint64_t audFrom = 0, tapeFrom = 0;
  std::vector<std::pair<uint64_t, float>> audEv, tapeEv;
  double tapeTickFrac = 0;

  // =================================================================
  //  CYKLUS: ANTIC/DMA -> (procesor) -> konec cyklu
  // =================================================================
  inline void beginCpuCycle(bool isWrite) {
    for (;;) {
      bool busy = anticCycle();
      if (!busy && !(rdyHalt && !isWrite)) return;
      if (!busy) rdyStalled = true;         // procesor stoji na RDY (WSYNC)
      endCycle();
    }
  }
  inline void endCycle() {
    pokeyTick();
    ++cyc;
    if (++x >= CYCLES_PER_LINE) endScanline();
  }

  // ---------------------------------------------------------------
  //  ANTIC - jeden cyklus (pred pripadnym pristupem procesoru).
  //  Vraci true, kdyz si cyklus vzal ANTIC (procesor ceka).
  // ---------------------------------------------------------------
  bool anticCycle() {
    // zpozdene CHBASE (+2 cykly) a WSYNC (RDY od 2. cyklu po zapisu)
    if (chbaseDelay && --chbaseDelay == 0) { syncGtia(0); chbase = chbaseNew; updateFont(); updateCurrentCharRow(); }
    if (wsyncPending && --wsyncPending == 0) rdyHalt = true;
    bool busy = false;
    if (x < 11 || x == 16 || x == 105) busy = anticSpecial();
    const uint8_t pat = dmaPat[x];
    if (pat & 0x06) {
      if (pat & 0x02) {                                   // jmeno znaku / bajt grafiky
        uint8_t v = anticRead((uint16_t)(pfBase | (pfOffset & 0x0FFF)));
        pfOffset = (uint16_t)((pfOffset + 1) & 0x0FFF);
        if (pfDataW < 120) pfData[pfDataW++] = v;
      }
      if (pat & 0x04) {                                   // data znaku ze znakove sady
        uint8_t c = pfDataR < pfDataW ? pfData[pfDataR] : 0;
        pfDataR++;
        uint8_t v = anticRead((uint16_t)(charFetchPtr + ((c & charMask) << 3)));
        if (pfCharW < 120) pfChar[pfCharW++] = v;
      }
    }
    if (pat & 0x01) busy = true;
    return busy;
  }

  bool anticSpecial() {
    bool busy = false;
    const bool zobrazeni = (unsigned)(line - 8) < 240;
    switch (x) {
      case 0: {
        // strely - DMA hracu zapina i strely
        if ((dmactlReg & 0x0C) && zobrazeni) {
          uint8_t b;
          if (dmactlReg & 0x10) b = anticRead((uint16_t)(((pmbase & 0xF8) << 8) + 0x300 + line));
          else b = anticRead((uint16_t)(((pmbase & 0xFC) << 8) + 0x180 + (line >> 1)));
          busy = true;
          gtiaMissileDma(b);
        }
        dlDmaInTime = (dmactlReg & 0x20) != 0;
        dlistLatch = dlist;
        break;
      }
      case 1: {
        pfDmaEnabled = false; pfDmaActive = false;
        if (line == 8) {
          dlActive = true; rowCounter = 0; rowCount = 1; rowStopUseVScroll = false;
          dlControl = dlControlPrev;
        }
        int rowStop = rowStopUseVScroll ? latchedVScroll : ((rowCount - 1) & 15);
        latchedVScroll = vscrol;
        if (rowCounter != rowStop) {
          rowCounter = (rowCounter + 1) & 15;
          pfDmaActive = true;
          if ((dlControl & 15) != 1) dlExtraLoads = false;
        } else {
          rowCounter = 0;
          rowStopUseVScroll = false;
          if (dlActive) {
            dlExtraLoads = false;
            dlControlPrev = dlControl;
            if (dlDmaInTime) {
              dlControl = anticRead(dlistLatch);
              busy = true;
              dlist = (uint16_t)((dlist & 0xFC00) | ((dlist + 1) & 0x03FF));
              dlKroku++;
            }
            const uint8_t mode = dlControl & 15;
            if (mode == 1 || (mode >= 2 && (dlControl & 0x40))) dlExtraLoads = true;
            rowCounter = 0;
            pushMode = 1; lineHiresAntic = false;
            switch (mode) {
              case 0: rowCount = ((dlControl >> 4) & 7) + 1; pushMode = 0; break;
              case 1: rowCount = 1; pushMode = 0; break;
              case 2: rowCount = 8; pushMode = 2; lineHiresAntic = true; break;
              case 3: rowCount = 10; pushMode = 2; lineHiresAntic = true; break;
              case 4: rowCount = 8; break;
              case 5: rowCount = 16; break;
              case 6: rowCount = 8; break;
              case 7: rowCount = 16; break;
              case 8: rowCount = 8; break;
              case 9: rowCount = 4; break;
              case 10: rowCount = 4; break;
              case 11: rowCount = 2; break;
              case 12: rowCount = 1; break;
              case 13: rowCount = 2; break;
              case 14: rowCount = 1; break;
              case 15: rowCount = 1; pushMode = 2; lineHiresAntic = true; break;
            }
            // svisle rolovani: bit 5 ($20). Zacatek oblasti -> start na VSCROL,
            // konec oblasti -> radek s koncem podle VSCROL.
            uint8_t sp = dlControlPrev, sc = dlControl;
            if ((sp & 15) < 2) sp = 0;
            if ((sc & 15) < 2) sc = 0;
            if ((sc ^ sp) & 0x20) {
              if (sc & 0x20) rowCounter = vscrol & 15;
              else rowStopUseVScroll = true;
            }
            // vodorovne rolovani: bit 4 ($10)
            hscrollEnabled = (mode != 1 && (sc & 0x10));
            pfDmaEnabled = true; pfDmaActive = true;
          }
        }
        hscrollDmaOffset = 0; hscrollDelay = false;
        if (hscrollEnabled) { hscrollDmaOffset = (hscrol & 14) >> 1; hscrollDelay = (hscrol & 1) != 0; }
        updateCurrentCharRow();
        updatePlayfieldTiming();
        break;
      }
      case 2: case 3: case 4: case 5: {
        if ((dmactlReg & 0x08) && zobrazeni) {
          int i = x - 2; uint8_t b;
          if (dmactlReg & 0x10) b = anticRead((uint16_t)(((pmbase & 0xF8) << 8) + 0x400 + 0x100 * i + line));
          else b = anticRead((uint16_t)(((pmbase & 0xFC) << 8) + 0x200 + 0x80 * i + (line >> 1)));
          busy = true;
          gtiaPlayerDma(i, b);
        }
        break;
      }
      case 6: {
        if (dlExtraLoads && (dmactlReg & 0x20)) {
          dlNext = anticRead(dlist); busy = true;
          dlist = (uint16_t)((dlist & 0xFC00) | ((dlist + 1) & 0x03FF));
        }
        latchedVScroll2 = vscrol;
        break;
      }
      case 7: {
        if (dlExtraLoads && (dmactlReg & 0x20)) {
          uint8_t hi = anticRead(dlist); busy = true;
          dlist = (uint16_t)((dlist & 0xFC00) | ((dlist + 1) & 0x03FF));
          uint16_t ad = (uint16_t)(dlNext | (hi << 8));
          if ((dlControl & 15) == 1) {
            dlist = ad;
            if (dlControl & 0x40) {              // JVB: konec display listu do VBLANK
              dlActive = false; dlExtraLoads = false;
              dlControl &= ~0x4F; rowCount = 1;
            }
          } else {                                // LMS
            pfBase = ad & 0xF000; pfOffset = ad & 0x0FFF;
            dlExtraLoads = false;
          }
        }
        earlyNMIEN = nmien;
        pendingNMIs = 0;
        if (line == 248) {
          pendingNMIs = 0x40; nmist |= 0x40; nmist &= ~0x80;
          dlControlPrev = dlControl; dlControl &= 0x20;
        } else {
          int rowStop = rowStopUseVScroll ? latchedVScroll2 : ((rowCount - 1) & 15);
          if ((dlControl & 0x80) && rowCounter == rowStop) { pendingNMIs = 0x80; nmist &= ~0x40; nmist |= 0x80; }
        }
        pfDataR = 0; pfCharW = 0;
        if (pfDmaEnabled) pfDataW = 0;
        break;
      }
      case 8: {
        uint8_t now = pendingNMIs & earlyNMIEN;
        uint8_t late = pendingNMIs & nmien & ~earlyNMIEN;
        lateNMI = false;
        if (now) nmiLatch = true;
        else if (late) lateNMI = true;
        break;
      }
      case 9: if (lateNMI) { nmiLatch = true; lateNMI = false; } break;
      case 10: {
        vblankLine = !zobrazeni;
        break;
      }
      case 16: {
        // GTIA: zacatek viditelne casti - 40znakovy rezim (hires) se nastavi v HBLANK
        syncGtia(-1);
        lineHires = lineHiresAntic && !(prior & 0xC0);
        break;
      }
      case 105:
        // konec WSYNC. NMI, ktere prislo, kdyz procesor stal na RDY uz PO
        // svem dotazu na preruseni (stoji na poslednim cyklu instrukce),
        // se bere hned po teto instrukci - 6502 uvolneni RDY vidi jako
        // "NMI aktivni 1 cyklus pred uvolnenim" (Altirra NegateRDY).
        if (rdyHalt && rdyStalled && cpu.polled && nmiLatch && !cpu.takeNmi) { cpu.takeNmi = true; cpu.takeIrq = false; }
        rdyHalt = false; rdyStalled = false;
        break;
    }
    return busy;
  }
  bool lineHiresAntic = false;

  void updateFont() { charBase128 = (uint16_t)((chbase & 0xFC) << 8); charBase64 = (uint16_t)((chbase & 0xFE) << 8); }
  void updateCurrentCharRow() {
    charMask = 0x7F;
    const int inv = (chactl & 4) ? 7 : 0;
    switch (dlControl & 15) {
      case 2: case 3: case 4: charFetchPtr = (uint16_t)(charBase128 + (inv ^ (rowCounter & 7))); break;
      case 5: charFetchPtr = (uint16_t)(charBase128 + (inv ^ (rowCounter >> 1))); break;
      case 6: charFetchPtr = (uint16_t)(charBase64 + (inv ^ (rowCounter & 7))); charMask = 0x3F; break;
      case 7: charFetchPtr = (uint16_t)(charBase64 + (inv ^ (rowCounter >> 1))); charMask = 0x3F; break;
      default: break;
    }
  }

  static int fetchRate(int mode) {           // cyklu na bajt
    static const uint8_t r[16] = {0, 0, 2, 2, 2, 2, 4, 4, 8, 8, 4, 4, 4, 2, 2, 2};
    return r[mode & 15];
  }

  void updatePlayfieldTiming() {
    pfWidth = dmactlReg & 3;
    pfFetchWidth = pfWidth;
    if (hscrollEnabled && pfFetchWidth != 0 && pfFetchWidth != 3) pfFetchWidth++;
    switch (pfWidth) {
      case 0: pfDisplayStart = 110; pfDisplayEnd = 110; break;
      case 1: pfDisplayStart = 32; pfDisplayEnd = 96; break;
      case 2: pfDisplayStart = 24; pfDisplayEnd = 104; break;
      case 3: pfDisplayStart = 22; pfDisplayEnd = 112; break;
    }
    pfDmaStart = pfDmaEnd = pfDmaVEnd = 114;
    const int mode = dlControl & 15;
    if (mode >= 2) {
      switch (pfFetchWidth) {
        case 0: break;
        case 1: pfDmaStart = mode < 8 ? 26 : 28; pfDmaEnd = pfDmaStart + 64; break;
        case 2: pfDmaStart = mode < 8 ? 18 : 20; pfDmaEnd = pfDmaStart + 80; break;
        case 3: pfDmaStart = mode < 8 ? 10 : 12; pfDmaEnd = pfDmaStart + 96; break;
      }
      if (pfFetchWidth) {
        pfDmaStart += hscrollDmaOffset; pfDmaEnd += hscrollDmaOffset;
        pfDmaVEnd = pfDmaEnd;
        if (pfDmaEnd > 106) pfDmaEnd = 106;
      }
    }
    if (!(pfDmaActive && x <= std::max(10, pfDmaStart - (mode < 8 ? 2 : 4)))) {
      // DMA zacatek uz probehl (zmena DMACTL uprostred radku) - nic dalsiho
      if (x > 10 && pfFetchWidth == 0) pfDmaActive = false;
    }
    updateDmaPattern();
  }

  void updateDmaPattern() {
    // od aktualniho cyklu dal (zmena uprostred radku nesmi prepsat minulost)
    const int from = (x < 11) ? 0 : x + 1;
    for (int i = from; i < 115; i++) dmaPat[i] = 0;
    const int mode = dlControl & 15;
    if (mode >= 2 && pfDmaActive && pfDmaStart < pfDmaVEnd && pfFetchWidth) {
      const int r = fetchRate(mode);
      const bool text = mode < 8;
      // bajt grafiky / jmeno znaku jen na 1. radku rezimu (pfDmaEnabled)
      if (pfDmaEnabled) {
        for (int c = pfDmaStart; c < pfDmaVEnd && c < 115; c += r) if (c >= from) dmaPat[c] |= 0x02 | (c < 106 ? 0x01 : 0);
      }
      if (text) {
        for (int c = pfDmaStart + 3; c < pfDmaVEnd + 3 && c < 115; c += r) if (c >= from) dmaPat[c] |= 0x04 | (c < 106 ? 0x01 : 0);
      }
    }
    // obnova pameti: 9x po 4 cyklech od 25; obsazeny cyklus -> nejblizsi volny
    {
      int rr = 24;
      for (int c = 25; c < 61; c += 4) {
        if (rr >= c) continue;
        rr = c;
        while (rr < 107) { if (!(dmaPat[rr] & 1)) { if (rr >= from) dmaPat[rr] |= 0x01 | 0x08; rr++; break; } rr++; }
      }
    }
    dmaPat[0] &= 0x08; // cyklus 0 resi anticSpecial (strely)
  }

  // ---------------------------------------------------------------
  //  Konec radku
  // ---------------------------------------------------------------
  void endScanline() {
    syncGtia(1000);                      // dokreslit cely radek
    gtiaEndLine();
    if (tape.loaded) tapeAdvance();      // kazeta bezi v realnem case (motor + PLAY)
    x = 0;
    if (++line >= LINES) {
      line = 0; frame++;
      dlActive = false;
    } else if (line >= 248) {
      if (line == 248) frameDone();
      dlActive = false; dlExtraLoads = false;
    }
    pfDataR = 0; pfCharW = 0;
    displayDone = 0;
  }

  void frameDone() {
    if (!view) return;
    static uint32_t lut[256]; static bool lutOk = false;
    if (!lutOk) { for (int i = 0; i < 256; i++) lut[i] = napAtariPalette(i); lutOk = true; }
    for (int i = 0; i < AnticView::W * AnticView::H; i++) view->fb[i] = lut[view->idx[i]];
  }

  // =================================================================
  //  ANTIC -> GTIA: dekodovani hraciho pole po cyklech
  // =================================================================
  // Vyplni merge[]/anData[] pro cykly [displayDone, xEnd) - hodnota v bunce
  // cyklu c patri barevnym taktum 2c a 2c+1.
  void pushPlayfield(int xEnd) {
    if (xEnd > pfDisplayEnd) xEnd = pfDisplayEnd;
    int c0 = displayDone; if (c0 < pfDisplayStart) c0 = pfDisplayStart;
    if (c0 >= xEnd) { if (displayDone < xEnd) displayDone = xEnd; return; }
    const int mode = dlControl & 15;
    const bool active = pushMode != 0 && pfWidth != 0 && (dmactlReg & 3);
    for (int c = c0; c < xEnd; c++) {
      uint8_t a0 = 0, a1 = 0;     // merge (PF bity) pro takt 2c, 2c+1
      uint8_t h0 = 0, h1 = 0;     // 2 bity (hires/AN) pro takt 2c, 2c+1
      if (active) {
        if (hscrollDelay) {
          uint8_t pa0, pa1, ph0, ph1, qa0, qa1, qh0, qh1;
          decodeCycle(mode, c - 1, pa0, pa1, ph0, ph1);
          decodeCycle(mode, c, qa0, qa1, qh0, qh1);
          a0 = pa1; a1 = qa0; h0 = ph1; h1 = qh0;
        } else decodeCycle(mode, c, a0, a1, h0, h1);
      }
      const int cc = c * 2;
      if (pushMode == 2) {
        merge[cc] = merge[cc + 1] = PF2;
        anData[cc] = h0; anData[cc + 1] = h1;
      } else {
        merge[cc] = a0; merge[cc + 1] = a1;
        anData[cc] = h0; anData[cc + 1] = h1;
      }
    }
    displayDone = xEnd;
  }

  // Dekodovani jednoho cyklu c (2 barevne takty).
  // a0/a1: PF bity (lores), h0/h1: 2bitove AN hodnoty (hires bity / GTIA)
  void decodeCycle(int mode, int c, uint8_t &a0, uint8_t &a1, uint8_t &h0, uint8_t &h1) {
    a0 = a1 = 0; h0 = h1 = 0;
    const int D = (mode < 8) ? 6 : 4;
    const int r = fetchRate(mode);
    if (!r) return;
    const int rel = c - D - pfDmaStart;
    if (rel < 0 || c - D >= pfDmaVEnd) return;
    const int k = rel / r, s = rel % r;
    if (k >= 120) return;
    static const uint8_t onehot[4] = {0, PF0, PF1, PF2};
    switch (mode) {
      case 2: case 3: {
        uint8_t ch = pfData[k], d = pfChar[k];
        uint8_t himask = (ch & 0x80) ? 0xFF : 0;
        uint8_t inv = himask & charInvert;
        if (mode == 2) {
          if ((rowCounter & 14) == 8 && (ch & 0x60) != 0x60) d = 0;
          d &= (uint8_t)(~himask | charBlink);
          d ^= inv;
        } else {
          uint8_t mask = rowCounter >= 2 ? 0xFF : 0x00;
          if ((rowCounter & 6) == 0) { if ((ch & 0x60) != 0x60) mask ^= 0xFF; }
          d &= (uint8_t)(~himask | charBlink);
          d = (uint8_t)(inv ^ (mask & d));
        }
        uint8_t nib = s ? (d & 15) : (d >> 4);
        h0 = (nib >> 2) & 3; h1 = nib & 3;
        break;
      }
      case 4: case 5: {
        uint8_t ch = pfData[k], d = pfChar[k];
        uint8_t nib = s ? (d & 15) : (d >> 4);
        uint8_t p0 = (nib >> 2) & 3, p1 = nib & 3;
        a0 = (p0 == 3 && (ch & 0x80)) ? PF3 : onehot[p0];
        a1 = (p1 == 3 && (ch & 0x80)) ? PF3 : onehot[p1];
        h0 = p0; h1 = p1;
        break;
      }
      case 6: case 7: {
        uint8_t ch = pfData[k], d = pfChar[k];
        uint8_t col = (uint8_t)(PF0 << (ch >> 6));
        uint8_t b0 = (d >> (7 - s * 2)) & 1, b1 = (d >> (6 - s * 2)) & 1;
        a0 = b0 ? col : 0; a1 = b1 ? col : 0;
        h0 = b0 ? (uint8_t)((ch >> 6) + 1 > 3 ? 3 : (ch >> 6) + 1) : 0; h1 = b1 ? h0 : 0;
        break;
      }
      case 8: {   // 4 body po 4 taktech, 2 bity
        uint8_t d = pfData[k];
        uint8_t p = (d >> (6 - (s >> 1) * 2)) & 3;
        a0 = a1 = onehot[p]; h0 = h1 = p;
        break;
      }
      case 9: {   // 8 bodu po 2 taktech, 1 bit
        uint8_t d = pfData[k];
        uint8_t b = (d >> (7 - s)) & 1;
        a0 = a1 = b ? PF0 : 0; h0 = h1 = b;
        break;
      }
      case 10: {  // 4 body po 2 taktech, 2 bity
        uint8_t d = pfData[k];
        uint8_t p = (d >> (6 - s * 2)) & 3;
        a0 = a1 = onehot[p]; h0 = h1 = p;
        break;
      }
      case 11: case 12: {  // 8 bodu po 1 taktu, 1 bit
        uint8_t d = pfData[k];
        uint8_t b0 = (d >> (7 - s * 2)) & 1, b1 = (d >> (6 - s * 2)) & 1;
        a0 = b0 ? PF0 : 0; a1 = b1 ? PF0 : 0; h0 = b0; h1 = b1;
        break;
      }
      case 13: case 14: {  // 4 body po 1 taktu, 2 bity
        uint8_t d = pfData[k];
        uint8_t nib = s ? (d & 15) : (d >> 4);
        uint8_t p0 = (nib >> 2) & 3, p1 = nib & 3;
        a0 = onehot[p0]; a1 = onehot[p1]; h0 = p0; h1 = p1;
        break;
      }
      case 15: {  // 8 hires bodu
        uint8_t d = pfData[k];
        uint8_t nib = s ? (d & 15) : (d >> 4);
        h0 = (nib >> 2) & 3; h1 = nib & 3;
        break;
      }
    }
  }

  // =================================================================
  //  GTIA
  // =================================================================
  inline int xclock() const { return x * 2; }

  void gtiaAddChange(int pos, uint8_t reg, uint8_t val) {
    if (rcN >= 256) syncGtia(1000);
    int i = rcN;
    while (i > 0 && rc[i - 1].pos > pos) { rc[i] = rc[i - 1]; i--; }
    rc[i].pos = pos; rc[i].reg = reg; rc[i].val = val; rcN++;
  }

  void gtiaPlayerDma(int i, uint8_t b) {
    if (gractl & 2) {
      if ((line & 1) || !(vdelay & (0x10 << i))) gtiaAddChange(xclock() + 3, (uint8_t)(0x0D + i), b);
    }
  }
  void gtiaMissileDma(uint8_t b) {
    if (gractl & 1) gtiaAddChange(xclock() + 3, 0x20, b);
  }

  // Dokreslit radek do barevneho taktu (x*2 + offset + 2)
  void syncGtia(int offset) {
    int xEnd = (offset >= 1000) ? 114 : x + offset + 1;
    if (xEnd > 114) xEnd = 114;
    if (xEnd < 0) xEnd = 0;
    pushPlayfield(xEnd);
    int ccEnd = (offset >= 1000) ? 228 : x * 2 + offset * 2 + 2;
    if (ccEnd > 228) ccEnd = 228;
    renderTo(ccEnd);
  }

  void applyChange(const RegChange &c) {
    const uint8_t v = c.val;
    switch (c.reg) {
      case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: sprPos[c.reg] = v; break;
      case 0x08: case 0x09: case 0x0A: case 0x0B: spr[c.reg & 3].size = v & 3; break;
      case 0x0C: for (int i = 0; i < 4; i++) spr[4 + i].size = (v >> (2 * i)) & 3; break;
      case 0x0D: case 0x0E: case 0x0F: case 0x10: spr[c.reg - 0x0D].latch = v; break;
      case 0x11:
        spr[4].latch = (uint8_t)((v << 6) & 0xC0); spr[5].latch = (uint8_t)((v << 4) & 0xC0);
        spr[6].latch = (uint8_t)((v << 2) & 0xC0); spr[7].latch = (uint8_t)(v & 0xC0);
        break;
      case 0x12: case 0x13: case 0x14: case 0x15: colpm[c.reg - 0x12] = v & 0xFE; break;
      case 0x16: case 0x17: case 0x18: case 0x19: colpf[c.reg - 0x16] = v & 0xFE; break;
      case 0x1A: colbk = v & 0xFE; break;
      case 0x1B: prior = v; if (v & 0xC0) lineHires = false; break;
      case 0x1E: std::memset(collP, 0, 4); std::memset(collM, 0, 4); break;
      case 0x20: {   // strely z DMA (VDELAY po strelach)
        uint8_t mask = 0x0F;
        if (!(line & 1)) mask = (uint8_t)~vdelay;
        if (mask & 1) spr[4].latch = (uint8_t)((v << 6) & 0xC0);
        if (mask & 2) spr[5].latch = (uint8_t)((v << 4) & 0xC0);
        if (mask & 4) spr[6].latch = (uint8_t)((v << 2) & 0xC0);
        if (mask & 8) spr[7].latch = (uint8_t)(v & 0xC0);
        break;
      }
    }
  }

  // barva vystupu: prioritni logika GTIA (rovnice z GTIA, viz Altirra gtiatables)
  static uint8_t priorityDecode(int prior5, uint8_t m, uint8_t col[9]) {
    // col: 0-3 P0..P3, 4-7 PF0..PF3, 8 BAK
    static const uint8_t kPfPri[8] = {0, 1, 2, 2, 4, 4, 4, 4};
    const uint8_t v = kPfPri[m & 7];
    const bool pf0 = v & 1, pf1 = (v & 2) != 0, pf2 = (v & 4) != 0, pf3 = (m & 8) != 0;
    const bool p0 = (m & 16) != 0, p1 = (m & 32) != 0, p2 = (m & 64) != 0, p3 = (m & 128) != 0;
    const bool multi = (prior5 & 16) != 0;
    const bool pri0 = prior5 & 1, pri1 = (prior5 & 2) != 0, pri2 = (prior5 & 4) != 0, pri3 = (prior5 & 8) != 0;
    const bool pri01 = pri0 | pri1, pri12 = pri1 | pri2, pri23 = pri2 | pri3, pri03 = pri0 | pri3;
    const bool p01 = p0 | p1, p23 = p2 | p3, pf01 = pf0 | pf1, pf23 = pf2 | pf3;
    const bool sp0 = p0 && !(pf01 && pri23) && !(pri2 && pf23);
    const bool sp1 = p1 && !(pf01 && pri23) && !(pri2 && pf23) && (!p0 || multi);
    const bool sp2 = p2 && !p01 && !(pf23 && pri12) && !(pf01 && !pri0);
    const bool sp3 = p3 && !p01 && !(pf23 && pri12) && !(pf01 && !pri0) && (!p2 || multi);
    const bool sf3 = pf3 && !(p23 && pri03) && !(p01 && !pri2);
    const bool sf2 = pf2 && !(p23 && pri03) && !(p01 && !pri2) && !sf3;
    const bool sf1 = pf1 && !(p23 && pri0) && !(p01 && pri01) && !sf3;
    const bool sf0 = pf0 && !(p23 && pri0) && !(p01 && pri01) && !sf3;
    const bool sb = !p01 && !p23 && !pf01 && !pf23;
    uint8_t c = 0;
    if (sf0) c |= col[4];
    if (sf1) c |= col[5];
    if (sf2) c |= col[6];
    if (sf3) c |= col[7];
    if (sp0) c |= col[0];
    if (sp1) c |= col[1];
    if (sp2) c |= col[2];
    if (sp3) c |= col[3];
    if (sb) c |= col[8];
    return c;
  }

  // Vykresleni barevnych taktu [lastSyncCc, ccEnd)
  void renderTo(int ccEnd) {
    int cc = lastSyncCc;
    if (cc >= ccEnd) return;
    int ri = 0;
    while (cc < ccEnd) {
      while (ri < rcN && rc[ri].pos <= cc) { applyChange(rc[ri]); ri++; }
      renderCc(cc);
      cc++;
    }
    while (ri < rcN && rc[ri].pos <= cc) { applyChange(rc[ri]); ri++; }
    if (ri) { for (int i = ri; i < rcN; i++) rc[i - ri] = rc[i]; rcN -= ri; }
    lastSyncCc = cc;
  }

  inline void sprStep(Sprite &s) {
    static const uint8_t tr[4][4] = {{0, 0, 0, 0}, {1, 0, 1, 0}, {0, 2, 2, 0}, {1, 2, 3, 0}};
    s.state = tr[s.size][s.state];
    if (s.state == 0) s.shift <<= 1;
  }

  void renderCc(int cc) {
    // spousteni hracu/strel na pozici HPOS (i v okrajich - posuvny registr bezi)
    for (int i = 0; i < 8; i++) {
      Sprite &s = spr[i];
      if (sprPos[i] == cc && (s.latch | s.shift)) {
        if (s.state) { s.state = 0; s.shift <<= 1; }
        s.shift |= s.latch;
      }
    }
    if (vblankLine || cc < 34 || cc >= 222) {
      for (int i = 0; i < 8; i++) if (spr[i].shift) sprStep(spr[i]);
      if (cc >= 32 && cc < 224) lineOut[cc * 2] = lineOut[cc * 2 + 1] = 0;
      return;
    }
    uint8_t m = 0;
    const bool inPf = (cc >> 1) >= pfDisplayStart && (cc >> 1) < pfDisplayEnd && pushMode != 0;
    uint8_t hb = 0;
    if (inPf) { m = merge[cc]; hb = anData[cc]; }
    const int gmode = prior & 0xC0;
    // "pseudo rezim E": ANTIC posila hires (rezim 2/3/F), ale klopny obvod
    // 40 sloupcu v GTIA uz byl na tomto radku shozen zapnutim GTIA rezimu
    // (PRIOR bit 6/7) - nahodit ho umi jen HBLANK. GTIA pak cte dvojice
    // hires bitu jako lores: 00..11 = PF0..PF3 (Altirra HW Reference;
    // pouziva napr. Postcard - obrazky v ramecich).
    if (gmode == 0 && pushMode == 2 && !lineHires && (m & PF2)) m = (uint8_t)(1 << hb);
    uint8_t pfColl = m & 15;
    if (gmode == 0 && lineHires && inPf) pfColl = hb ? PF2 : 0;   // kolize v hires: PF2 kdyz bit
    if (gmode == 0x40 || gmode == 0xC0) { m = 0; pfColl = 0; }
    if (gmode == 0x80) {
      // rezim 10: dvojice taktu -> 4bitovy index; hraci barvy z hraciho pole
      int base = (cc - 1) & ~1;
      uint8_t l = (uint8_t)(((base >= 0 ? anData[base] : 0) << 2) | anData[base + 1]);
      static const uint8_t k10[16] = {P0, P1, P2, P3, PF0, PF1, PF2, PF3, 0, 0, 0, 0, PF0, PF1, PF2, PF3};
      if (!inPf) l = 8;
      m = k10[l];
      pfColl = m & 15;
    }
    // hraci
    uint8_t pm = 0;
    for (int i = 0; i < 4; i++) {
      Sprite &s = spr[i];
      if (s.shift & 0x80) {
        collP[i] |= (uint8_t)(pfColl | pm);
        pm |= (uint8_t)(P0 << i);
      }
      if (s.shift) sprStep(s);
    }
    // P->P kolize oboustranne
    for (int i = 0; i < 4; i++) if (pm & (P0 << i)) collP[i] |= (uint8_t)(pm & ~(P0 << i));
    // strely
    uint8_t mm = 0;
    for (int i = 0; i < 4; i++) {
      Sprite &s = spr[4 + i];
      if (s.shift & 0x80) { collM[i] |= (uint8_t)(pfColl | pm); mm |= (uint8_t)(1 << i); }
      if (s.shift) sprStep(s);
    }
    uint8_t all = (uint8_t)(m | pm);
    if (mm) {
      if (prior & 0x10) all |= PF3;                 // paty hrac
      else for (int i = 0; i < 4; i++) if (mm & (1 << i)) all |= (uint8_t)(P0 << i);
    }
    uint8_t col[9] = {colpm[0], colpm[1], colpm[2], colpm[3], colpf[0], colpf[1], colpf[2], colpf[3], colbk};
    uint8_t c0, c1;
    if (gmode == 0) {
      uint8_t c = priorityDecode(prior & 0x1F, all, col);
      if (lineHires && inPf) {
        // 40znakovy rezim: PF2 pozadi, nastaveny bit = jas PF1 (i pres hrace)
        c0 = (hb & 2) ? (uint8_t)((c & 0xF0) | (colpf[1] & 0x0F)) : c;
        c1 = (hb & 1) ? (uint8_t)((c & 0xF0) | (colpf[1] & 0x0F)) : c;
      } else c0 = c1 = c;
    } else if (gmode == 0x40) {
      uint8_t c = priorityDecode(prior & 0x1F, (uint8_t)(all & (P0 | P1 | P2 | P3 | PF3)), col);
      int base = cc & ~1;
      uint8_t l = inPf ? (uint8_t)((anData[base] << 2) | anData[base + 1]) : 0;
      if (!(all & 0xF0)) c |= l;
      c0 = c1 = c;
    } else if (gmode == 0xC0) {
      uint8_t c = priorityDecode(prior & 0x1F, (uint8_t)(all & (P0 | P1 | P2 | P3 | PF3)), col);
      int base = cc & ~1;
      uint8_t l0 = inPf ? (uint8_t)((anData[base] << 6) | (anData[base + 1] << 4)) : 0;
      if (!(all & 0xF0)) { c |= l0; if (l0 == 0) c &= 0xF0; }
      c0 = c1 = c;
    } else {
      uint8_t c = priorityDecode(prior & 0x1F, all, col);
      c0 = c1 = c;
    }
    if (cc >= 32 && cc < 224) { lineOut[cc * 2] = c0; lineOut[cc * 2 + 1] = c1; }
  }

  void gtiaEndLine() {
    // zbyle zmeny registru (s pozici za koncem radku) posunout do dalsiho radku
    for (int i = 0; i < rcN; i++) rc[i].pos -= 228;
    int k = 0;
    for (int i = 0; i < rcN; i++) { if (rc[i].pos < 0) applyChange(rc[i]); else rc[k++] = rc[i]; }
    rcN = k;
    lastSyncCc = 0;
    if (view && line >= 8 && line < 248) {
      std::memcpy(&view->idx[(line - 8) * AnticView::W], &lineOut[64], AnticView::W);
    }
    std::memset(merge, 0, sizeof merge);
    std::memset(anData, 0, sizeof anData);
  }

  // =================================================================
  //  I/O
  // =================================================================
  inline uint8_t cpuRead(uint16_t a) {
    if ((a & 0xF800) == 0xD000) return ioRead(a);
    return memRead(a, false);
  }
  inline void cpuWrite(uint16_t a, uint8_t v) {
    if ((a & 0xF800) == 0xD000) { ioWrite(a, v); return; }
    memWrite(a, v);
  }
  inline uint8_t memRead(uint16_t a, bool antic) {
    const int pb = mem.portB();
    if (a >= 0xC000) { if ((pb & 1) && mem.os) return mem.os[a - 0xC000]; return mem.ram[a]; }
    if (a >= 0xA000) { if (!(pb & 2) && mem.bas) return mem.bas[a - 0xA000]; return mem.ram[a]; }
    if (a >= 0x4000 && a < 0x8000) {
      // self-test ROM ($5000-$57FF, PORTB bit 7 = 0, jen se zapnutou OS ROM):
      // MMU dekoduje adresu bez ohledu na to, kdo je na sbernici - vidi ji
      // procesor i ANTIC a ma prednost pred rozsirenou pameti
      if (a >= 0x5000 && a < 0x5800 && !(pb & 0x80) && (pb & 1) && mem.os) return mem.os[a - 0x5000 + 0x1000];
      if (!(pb & (antic ? 0x20 : 0x10))) return mem.ext[((pb >> 2) & 3) * 0x4000 + (a - 0x4000)];
    }
    return mem.ram[a];
  }
  inline uint8_t anticRead(uint16_t a) {
    // DMA ANTIC jde po stejne datove sbernici - posledni hodnota na ni zustane
    // (130XE ma "plovouci" sbernici, viz ioRead)
    const uint8_t v = ((a & 0xF800) == 0xD000) ? 0xFF : memRead(a, true);
    busData = v;
    return v;
  }
  inline void memWrite(uint16_t a, uint8_t v) {
    const int pb = mem.portB();
    if (a >= 0xC000) { if (pb & 1) return; mem.ram[a] = v; return; }
    if (a >= 0xA000) { if (!(pb & 2) && mem.bas) return; mem.ram[a] = v; return; }
    if (a >= 0x4000 && a < 0x8000) {
      if (a >= 0x5000 && a < 0x5800 && !(pb & 0x80) && (pb & 1)) return;
      if (!(pb & 0x10)) { mem.ext[((pb >> 2) & 3) * 0x4000 + (a - 0x4000)] = v; return; }
    }
    mem.ram[a] = v;
  }

  uint8_t ioRead(uint16_t a) {
    switch (a & 0xFF00) {
      case 0xD000: return gtiaRead(a & 0x1F);
      case 0xD200: return pokeyRead(a & 0x0F);
      case 0xD300: return piaRead(a & 3);
      case 0xD400: return anticRegRead(a & 0x0F);
      // $D100 (PBI), $D500 (cartridge), $D600-$D7FF: na 130XE tam bez
      // pripojenych zarizeni nic neodpovida a datova sbernice "plave" - cte
      // se posledni hodnota, ktera na ni byla (typicky horni bajt adresy z
      // predchoziho cyklu, napr. LDA $D5xx -> $D5). 800XL ma pull-upy ($FF).
      default: return busData;
    }
  }
  void ioWrite(uint16_t a, uint8_t v) {
    switch (a & 0xFF00) {
      case 0xD000: gtiaWrite(a & 0x1F, v); break;
      case 0xD200: pokeyWrite(a & 0x0F, v); break;
      case 0xD300: piaWrite(a & 3, v); break;
      case 0xD400: anticRegWrite(a & 0x0F, v); break;
      default: break;
    }
  }

  // ---------------- ANTIC registry ----------------
  uint8_t anticRegRead(int r) {
    switch (r) {
      case 0x0B: {
        int y = line;
        if (x >= 111) { y++; if (x >= 112 && y >= LINES) y = 0; }
        return (uint8_t)(y >> 1);
      }
      case 0x0C: return 0;       // PENH
      case 0x0D: return 0xFF;    // PENV
      case 0x0E: return 0xFF;
      case 0x0F: return nmist;
      default: return 0xFF;
    }
  }
  void anticRegWrite(int r, uint8_t v) {
    if (traceFrame == frame) std::fprintf(stderr, "A y=%d x=%d reg=%02X v=%02X\n", line, x, r, v);
    switch (r) {
      case 0x00:
        v &= 0x3F;
        if (v != dmactlReg) { syncGtia(0); dmactlReg = v; updatePlayfieldTiming(); }
        break;
      case 0x01:
        v &= 7;
        if (v != chactl) { syncGtia(0); chactl = v; charInvert = (chactl & 2) ? 0xFF : 0; charBlink = (chactl & 1) ? 0 : 0xFF; updateCurrentCharRow(); }
        break;
      case 0x02: dlist = (uint16_t)((dlist & 0xFF00) | v); break;
      case 0x03: dlist = (uint16_t)((dlist & 0x00FF) | (v << 8)); break;
      case 0x04: {
        v &= 15;
        if (v != hscrol) {
          if (hscrollEnabled) {
            uint8_t d = (uint8_t)(hscrol ^ v);
            if (d & 1) { syncGtia(0); hscrollDelay = (v & 1) != 0; }
            if (d & 14) { hscrollDmaOffset = (v & 14) >> 1; hscrol = v; updatePlayfieldTiming(); }
          }
          hscrol = v;
        }
        break;
      }
      case 0x05: vscrol = v & 15; if (x >= 1 && x < 109) latchedVScroll = vscrol; break;
      case 0x07: pmbase = v & 0xFC; break;
      case 0x09: chbaseNew = v; chbaseReg = v; chbaseDelay = 2; break;
      case 0x0A:
        if (!wsyncPending || (wsyncPending == 1 && x == 104)) wsyncPending = 2;
        break;
      case 0x0E: nmien = v & 0xC0; break;
      case 0x0F:
        nmist = 0x1F;
        if (x == 7 && pendingNMIs) nmist |= pendingNMIs;
        break;
    }
  }

  // ---------------- GTIA registry ----------------
  uint8_t gtiaRead(int r) {
    switch (r) {
      case 0x10: case 0x11: case 0x12: case 0x13: return (uint8_t)(trig[r - 0x10] & 1);
      case 0x14: return 0x01;                       // PAL
      case 0x1F: {
        uint8_t in = (uint8_t)(0x08 | (consol & 7));
        return (uint8_t)((~consolOut) & in & 0x0F);
      }
      default: break;
    }
    if (r >= 0x15) return 0x0F;
    syncGtia(0);
    switch (r) {
      case 0x00: case 0x01: case 0x02: case 0x03: return collM[r] & 15;
      case 0x04: case 0x05: case 0x06: case 0x07: return collP[r - 4] & 15;
      case 0x08: case 0x09: case 0x0A: case 0x0B: return (collM[r - 8] >> 4) & 15;
      case 0x0C: case 0x0D: case 0x0E: case 0x0F: return (uint8_t)((collP[r - 0x0C] >> 4) & 15 & ~(1 << (r - 0x0C)));
    }
    return 0;
  }
  long long traceFrame = -1;          // testy: vypis zapisu barev / NMI v tomto snimku
  void gtiaWrite(int r, uint8_t v) {
    gReg[r] = v;
    if (traceFrame == frame && r >= 0x12 && r <= 0x1B) std::fprintf(stderr, "W y=%d x=%d reg=%02X v=%02X\n", line, x, r, v);
    const int xp = xclock();
    switch (r) {
      case 0x12: case 0x13: case 0x14: case 0x15:
      case 0x16: case 0x17: case 0x18: case 0x19: case 0x1A:
        gtiaAddChange(xp + 1, (uint8_t)r, v); return;
      case 0x1B: gtiaAddChange(xp + 1, (uint8_t)r, v); return;
      case 0x1C: vdelay = v; return;
      case 0x1D: gractl = v; return;
      case 0x1F: {
        uint8_t n = v & 0x0F;
        if ((n ^ consolOut) & 8) setSpeaker((n & 8) != 0);
        consolOut = n;
        return;
      }
      case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07:
        gtiaAddChange(xp + 5, (uint8_t)r, v); return;
      case 0x08: case 0x09: case 0x0A: case 0x0B: case 0x0C:
      case 0x0D: case 0x0E: case 0x0F: case 0x10: case 0x11:
        gtiaAddChange(xp + 3, (uint8_t)r, v); return;
      case 0x1E: gtiaAddChange(xp + 3, (uint8_t)r, v); return;
    }
  }

  // ---------------- PIA ----------------
  uint8_t piaRead(int r) {
    switch (r) {
      case 0: if (mem.pia.ctlA & 4) return (uint8_t)((porta & ~mem.pia.ddrA) | (mem.pia.orA & mem.pia.ddrA)); return (uint8_t)mem.pia.ddrA;
      case 1: if (mem.pia.ctlB & 4) return (uint8_t)mem.portB(); return (uint8_t)mem.pia.ddrB;
      case 2: return (uint8_t)(mem.pia.ctlA & 0x3F);
      default: return (uint8_t)(mem.pia.ctlB & 0x3F);
    }
  }
  void piaWrite(int r, uint8_t v) {
    switch (r) {
      case 0: if (mem.pia.ctlA & 4) mem.pia.orA = v; else mem.pia.ddrA = v; break;
      case 1: if (mem.pia.ctlB & 4) mem.pia.orB = v; else mem.pia.ddrB = v; break;
      case 2: mem.pia.ctlA = v & 0x3F; break;
      default: mem.pia.ctlB = v & 0x3F; break;
    }
  }

  // =================================================================
  //  POKEY
  // =================================================================
  void pokeyCold() {
    for (int i = 0; i < 4; i++) { audf[i] = 0; audc[i] = 0; audfP1[i] = 1; pcnt[i] = 1; pborrow[i] = 0; chOut[i] = 0; }
    audctl = 0; irqen = 0; irqst = 0xFF; skctl = 0; skstat = 0xFF; kbcode = 0xFF; serin = 0xFF;
    hpf[0] = hpf[1] = 0; stimerDelay = 0;
    serOutCounter = 0; serOutValid = false; serOutState = true; serOutShift = 0; serOutTickDelay = 0;
    last64 = last15 = polyBase = polyShutOff = 0;
    audLevel = audLevelStart = 0; audEv.clear(); audFrom = 0;
    tapeLevel = tapeLevelStart = 0; tapeEv.clear(); tapeFrom = 0;
  }
  void updateIrq() { irqLine = ((~irqst) & irqen & 0xFF) != 0; }

  // Polynomialni citace POKEY (fakta o hardwaru - Altirra HW Reference):
  // vsechny jsou typu XNOR (stav "same nuly" je platny, init je nuluje),
  // 4bit: zpetna vazba z bitu 2^3, 5bit: 2^4, 9bit: 8^3, 17bit: 16^11;
  // RANDOM cte INVERTOVANE bity 17bit registru (bity 8..15) resp. 9bit.
  // Tabulka: bit3 = poly4, bit2 = poly5, bit1 = poly9, bit0 = poly17,
  // 2x 131071 polozek (aby slo cist 8 po sobe jdoucich bez preteceni).
  static const uint8_t *polyBuf() {
    static uint8_t *t = nullptr;
    if (!t) {
      t = new uint8_t[131071 * 2];
      uint32_t p4 = 0, p5 = 0, p9 = 0, p17 = 0;
      for (int i = 0; i < 131071; i++) {
        p4 = (p4 >> 1) + (~((p4 << 2) ^ (p4 << 3)) & 8);
        p5 = (p5 >> 1) + (~((p5 << 2) ^ (p5 << 4)) & 16);
        p9 = (p9 >> 1) + (~((p9 << 8) ^ (p9 << 3)) & 0x100);
        p17 = (p17 >> 1) + (~((p17 << 16) ^ (p17 << 11)) & 0x10000);
        t[i] = (uint8_t)(((p4 & 1) << 3) | ((p5 & 1) << 2) | ((p9 & 1) << 1) | ((p17 >> 8) & 1));
      }
      std::memcpy(t + 131071, t, 131071);
    }
    return t;
  }
  // pozice poly citacu v cyklu (citace bezi od posledniho opusteni init rezimu)
  inline uint32_t polyPos(uint32_t period) const {
    int64_t d = (int64_t)(cyc - polyBase);
    int64_t m = d % (int64_t)period;
    if (m < 0) m += period;
    return (uint32_t)m;
  }
  inline int polyBit4() const { return (polyBuf()[polyPos(15)] >> 3) & 1; }
  inline int polyBit5() const { return (polyBuf()[polyPos(31)] >> 2) & 1; }
  inline int polyBit9() const { return (polyBuf()[polyPos(511)] >> 1) & 1; }
  inline int polyBit17() const { return polyBuf()[polyPos(131071)] & 1; }

  uint8_t pokeyRead(int r) {
    switch (r) {
      case 0x00: case 0x01: case 0x02: case 0x03: case 0x04: case 0x05: case 0x06: case 0x07: return 228; // paddle: bez paddlu
      case 0x08: return 0;          // ALLPOT
      case 0x09: return kbcode;
      case 0x0A: {
        // init rezim: posuvny registr se plni jednickami (do 10 cyklu), pak $FF
        uint8_t force = 0;
        if (!(skctl & 3)) {
          const uint64_t off = cyc - polyShutOff;
          if (off > 10) return 0xFF;
          if (off) force = (uint8_t)(0xFFE00 >> (int)off);
        }
        const uint8_t *pb = polyBuf();
        uint8_t v = 0;
        if (audctl & 0x80) { const uint32_t o = polyPos(511); for (int i = 7; i >= 0; i--) v = (uint8_t)((v << 1) | ((pb[o + i] >> 1) & 1)); }
        else { const uint32_t o = polyPos(131071); for (int i = 7; i >= 0; i--) v = (uint8_t)((v << 1) | (pb[o + i] & 1)); }
        return (uint8_t)(~v | force);
      }
      case 0x0D: return serin;
      case 0x0E: return irqst;
      case 0x0F: {
        // bit 4 = primy stav seriove vstupni linky (kazeta: demodulovana FSK)
        uint8_t s = skstat;
        if (tapeDataLevel()) s |= 0x10; else s &= ~0x10;
        return s;
      }
      default: return 0xFF;
    }
  }

  void pokeyWrite(int r, uint8_t v) {
    switch (r) {
      case 0x00: case 0x02: case 0x04: case 0x06: {
        int ch = r >> 1;
        audfMark();
        audf[ch] = v; audfP1[ch] = (uint8_t)(v + 1);
        break;
      }
      case 0x01: case 0x03: case 0x05: case 0x07: audfMark(); audc[r >> 1] = v; updateAudioLevel(); break;
      case 0x08: audfMark(); audctl = v; updateAudioLevel(); break;
      case 0x09: stimerDelay = 4; break;            // STIMER: restart citacu za 4 cykly
      case 0x0A: skstat |= 0xE0; break;             // SKRES
      case 0x0B: break;                             // POTGO
      case 0x0D:
        serout = v; seroutPocet++;
        if (!serOutCounter) serOutCounter = 1;
        serOutValid = true;
        break;
      case 0x0E:
        irqen = v;
        irqst |= (uint8_t)(~v & 0xF7);
        updateIrq();
        break;
      case 0x0F: {
        const bool prvInit = (skctl & 3) == 0, newInit = (v & 3) == 0;
        if (prvInit != newInit) {
          if (!newInit) { last15 = cyc + 81 - 114; last64 = cyc + 22 - 28; polyBase = cyc + 1; }
          else polyShutOff = cyc;     // poly citace bezi dal (dobeh posuvu), pak RANDOM = $FF
          serOutCounter = 0; serOutValid = false; serOutState = false;
          // zmena init rezimu nuluje i prijimaci posuvny registr - rozpracovany
          // bajt z kazety se zahodi (OS tim po zmereni rychlosti zahodi druhy $55)
          tape.rxPhase = 0;
          skstat |= 0x02;
          irqst &= ~0x08; updateIrq();
        }
        if ((skctl ^ v) & 0x02) { if (!(v & 2)) skstat |= 0x04; }
        skctl = v;
        updateAudioLevel();
        break;
      }
    }
  }

  inline bool slowTick15() const { return (int64_t)(cyc - last15) >= 0 && ((cyc - last15) % 114) == 0; }
  inline bool slowTick64() const { return (int64_t)(cyc - last64) >= 0 && ((cyc - last64) % 28) == 0; }

  // jeden cyklus POKEY
  inline void pokeyTick() {
    if (stimerDelay && --stimerDelay == 0) {
      for (int i = 0; i < 4; i++) { pcnt[i] = audfP1[i]; pborrow[i] = 0; }
      if (audctl & 0x10) pcnt16[0] = (audf[1] << 8 | audf[0]) + 1;
      if (audctl & 0x08) pcnt16[1] = (audf[3] << 8 | audf[2]) + 1;
    }
    // init rezim (SKCTL bity 0-1 = 0): stoji jen delicky 64/15 kHz (a poly
    // citace) - kanaly na 1,79 MHz bezi dal (Altirra HW Reference)
    const bool fast1 = audctl & 0x40, fast3 = audctl & 0x20, link12 = audctl & 0x10, link34 = audctl & 0x08;
    const bool slow = (skctl & 3) ? ((audctl & 1) ? slowTick15() : slowTick64()) : false;
    // kanaly 1+2
    if (link12) tickLinked(0, fast1 || slow, fast1);
    else { tickCh(0, fast1 || slow); tickCh(1, slow); }
    if (link34) tickLinked(1, fast3 || slow, fast3);
    else { tickCh(2, fast3 || slow); tickCh(3, slow); }
    serialTickDelayed();
  }
  int pcnt16[2] = {1, 1}, pborrow16[2] = {0, 0};

  inline void tickCh(int ch, bool clk) {
    if (pborrow[ch]) { if (--pborrow[ch] == 0) { pcnt[ch] = audfP1[ch]; timerFire(ch); } return; }
    if (clk && --pcnt[ch] == 0) pborrow[ch] = 3;
  }
  inline void tickLinked(int pair, bool clk, bool fast) {
    (void)fast;
    if (pborrow16[pair]) {
      if (--pborrow16[pair] == 0) {
        pcnt16[pair] = (pair == 0 ? ((audf[1] << 8) | audf[0]) : ((audf[3] << 8) | audf[2])) + 1;
        timerFire(pair * 2 + 1);
      }
      return;
    }
    if (clk) {
      --pcnt16[pair];
      if ((pcnt16[pair] & 0xFF) == 0) timerFire(pair * 2, true);   // nizsi kanal (jen zvuk)
      if (pcnt16[pair] == 0) pborrow16[pair] = 6;
    }
  }

  void timerFire(int ch, bool lowOfLinked = false) {
    // vystup kanalu (zvuk)
    const int ac = audc[ch];
    bool change = false;
    const bool init = !(skctl & 3);         // init: vystupy poly citacu = 1
    if ((ac & 0x80) || init || polyBit5()) {
      uint8_t o;
      if (ac & 0x20) o = chOut[ch] ^ 1;
      else if (init) o = 1;
      else if (ac & 0x40) o = (uint8_t)polyBit4();
      else o = (uint8_t)((audctl & 0x80) ? polyBit9() : polyBit17());
      if (o != chOut[ch]) { chOut[ch] = o; change = true; }
    }
    // horni propusti: kanal 3 vzorkuje kanal 1, kanal 4 kanal 2
    if (ch == 2 && (audctl & 0x04)) { hpf[0] = chOut[0]; change = true; }
    if (ch == 3 && (audctl & 0x02)) { hpf[1] = chOut[1]; change = true; }
    if (lowOfLinked) { if (change) updateAudioLevel(); return; }
    // preruseni casovacu 1, 2, 4
    if (ch == 0 && (irqen & 0x01)) { irqst &= ~0x01; updateIrq(); }
    if (ch == 1 && (irqen & 0x02)) { irqst &= ~0x02; updateIrq(); }
    if (ch == 3 && (irqen & 0x04)) { irqst &= ~0x04; updateIrq(); }
    // dvoutonovy rezim: tony se srovnaji
    if ((skctl & 0x08) && ((ch == 0 && serOutState) || ch == 1)) twoToneReset = 2;
    // seriovy vystup: hodiny = casovac 4 (SKCTL $20/$40) nebo 2 ($60)
    if (serOutCounter) {
      const int m = skctl & 0x60;
      if ((ch == 3 && (m == 0x20 || m == 0x40)) || (ch == 1 && m == 0x60)) serOutTickDelay = 2;
    }
    if (change) updateAudioLevel();
  }
  int twoToneReset = 0;

  inline void serialTickDelayed() {
    if (twoToneReset && --twoToneReset == 0) {
      pcnt[0] = audfP1[0]; pcnt[1] = audfP1[1]; pborrow[0] = pborrow[1] = 0;
    }
    if (serOutTickDelay && --serOutTickDelay == 0) serialOutTick();
  }

  void serialOutTick() {
    if (!serOutCounter) return;
    --serOutCounter;
    // 20 pul-bitu: start (0), 8 datovych LSB napred, stop (1)
    if (serOutCounter) {
      int half = 20 - serOutCounter;       // 1..19
      int bit = half / 2;                  // 0 start, 1-8 data, 9 stop
      bool st = bit == 0 ? false : (bit <= 8 ? ((serOutShift >> (bit - 1)) & 1) != 0 : true);
      if (st != serOutState) { serOutState = st; updateAudioLevel(); }
    } else {
      if (serOutValid) {
        serOutCounter = 20;
        serOutShift = serout;
        serOutValid = false;
        bool st = false;                   // start bit
        if (st != serOutState) { serOutState = st; updateAudioLevel(); }
        irqst |= 0x08;
        if (irqen & 0x10) irqst &= ~0x10;
      } else {
        if (!serOutState) { serOutState = true; updateAudioLevel(); }
        irqst &= ~0x08;
      }
      updateIrq();
    }
  }

  // prijem bajtu z kazety/SIO zarizeni (na konci stop bitu). cyclesPerBit=0:
  // rychlost se nekontroluje (kazeta uz bajt slozila rychlosti POKEY).
  std::vector<int> *serinLog = nullptr;     // testy: zaznam prijatych bajtu
  void receiveSerialByte(uint8_t c, double cyclesPerBit, bool framingError = false) {
    if (serinLog) serinLog->push_back((int)c | (framingError ? 0x100 : 0) | (!(skctl & 3) ? 0x200 : 0) | ((irqen & 0x20) ? 0x400 : 0));
    if (!(skctl & 3)) return;                 // init rezim: POKEY nic neprijima
    if (framingError) skstat &= ~0x80;
    if (cyclesPerBit > 0) {
      const double expect = 2.0 * serialHalfBitCycles();
      const double margin = expect / 8.0;
      if (cyclesPerBit < expect - margin || cyclesPerBit > expect + margin) { c = 0xFF; skstat &= ~0x80; }
    }
    if (irqen & 0x20) {
      if (!(irqst & 0x20)) skstat &= ~0x20;   // predchozi bajt nikdo nevyzvedl -> preteceni
      irqst &= ~0x20; updateIrq();
    }
    serin = c;
  }

  // ---------------- zvukovy vystup ----------------
  void setSpeaker(bool on) { audfMark(); gtiaSpeakerBit = on ? 1 : 0; gtiaKlikPocitadlo++; updateAudioLevel(); }
  void audfMark() {}
  void updateAudioLevel() {
    double s = 0;
    for (int ch = 0; ch < 4; ch++) {
      const int ac = audc[ch];
      const int vol = ac & 15;
      if (!vol) continue;
      if (ac & 0x10) { s += vol / 60.0; continue; }
      if ((ch == 0 && (audctl & 0x10)) || (ch == 2 && (audctl & 0x08))) { /* nizsi kanal 16bit */ }
      uint8_t o = chOut[ch];
      if (ch == 0 && (audctl & 0x04)) o ^= hpf[0];
      if (ch == 1 && (audctl & 0x02)) o ^= hpf[1];
      if (o) s += vol / 60.0;
    }
    s += gtiaSpeakerBit ? 0.25 : 0.0;
    if (s != audLevel) {
      audLevel = s; audEv.emplace_back(cyc, (float)s);
      if (audEv.size() > 400000) { audEv.clear(); audFrom = cyc; audLevelStart = audLevel; }   // nikdo zvuk neodebira
    }
    // kazetovy vystup (SIO DATA OUT): dvoutonovy rezim = vystup casovace 1/2
    double tl;
    if (skctl & 0x08) tl = (serOutState ? chOut[0] : chOut[1]) ? 1.0 : -1.0;
    else tl = serOutState ? 1.0 : -1.0;
    if (tl != tapeLevel) {
      tapeLevel = tl;
      if (tapeCapture) tapeEv.emplace_back(cyc, (float)tl);
      if (tapeEv.size() > 400000) { tapeEv.clear(); tapeFrom = cyc; tapeLevelStart = tapeLevel; }
    }
  }

  // Integrace urovni do n vzorku za usek [from, to)
  static void integrate(const std::vector<std::pair<uint64_t, float>> &ev, double levelStart, uint64_t from, uint64_t to,
                        float *out, int n, bool dc) {
    if (n <= 0) return;
    if (to <= from) { for (int i = 0; i < n; i++) out[i] = 0; return; }
    const double span = (double)(to - from);
    const double per = span / n;
    size_t ei = 0;
    double level = levelStart;
    double tcur = (double)from;
    static double dcx = 0, dcy = 0;
    for (int i = 0; i < n; i++) {
      const double tEnd = (double)from + per * (i + 1);
      double acc = 0;
      while (ei < ev.size() && (double)ev[ei].first < tEnd) {
        double te = (double)ev[ei].first;
        if (te > tcur) { acc += level * (te - tcur); tcur = te; }
        level = ev[ei].second; ei++;
      }
      acc += level * (tEnd - tcur); tcur = tEnd;
      double s = acc / per;
      if (dc) { double y = s - dcx + 0.995 * dcy; dcx = s; dcy = y; s = y; }
      out[i] = (float)s;
    }
  }

  // ---------------- kazeta -> datova linka -> POKEY ----------------
  // Volano na konci kazdeho radku a pri cteni SKSTAT: posune pasku o ubehly
  // cas (jen motor ZAP + PLAY) a prijimac POKEY vzorkuje uroven linky
  // uprostred bitu rychlosti z AUDF3/AUDF4 (jak ji nastavil OS po zmereni
  // uvodnich $55 $55) - start bit, 8 datovych bitu (LSB napred), stop bit.
  int tapeDataLevel() { tapeAdvance(); return tape.line; }
  uint64_t tapeLastCyc = 0;
  // pul bitu v cyklech = perioda casovace 4 (seriovy vstup)
  double serialHalfBitCycles() const {
    double per;
    if (audctl & 0x08) {
      const int f = (audf[3] << 8) | audf[2];
      per = (audctl & 0x20) ? (double)(f + 7) : (double)(f + 1) * ((audctl & 1) ? 114 : 28);
    } else per = (double)(audf[3] + 1) * ((audctl & 1) ? 114 : 28);
    if (per < 20) per = 1478;                         // nesmysl -> 600 baudu
    return per;
  }
  void tapeAdvance() {
    const uint64_t now = cyc;
    const uint64_t d = now - tapeLastCyc; tapeLastCyc = now;
    const bool bezi = tape.loaded && tape.play && motorOn();
    if (!bezi || d == 0) {
      if (!bezi) { tape.line = 1; tape.rxPhase = 0; }
      return;
    }
    const double perCyc = tape.img.rate / 1773447.0;   // vzorku pasky za cyklus
    const double p0 = tape.pos, p1 = tape.pos + d * perCyc;
    const double c0 = (double)(now - d);               // cyklus odpovidajici p0
    // projit hranice vzorku pasky v [p0, p1) a vzorkovani UART mezi nimi
    size_t i = (size_t)std::floor(p0) + 1;
    const size_t iEnd = (size_t)std::floor(p1);
    for (;;) {
      if (i > iEnd) {                                  // zbytek useku do "ted"
        while (tape.rxPhase && tape.rxNext <= (double)now) tapeUartSample();
        break;
      }
      const double tSeg = c0 + ((double)i - p0) / perCyc;
      // vzorkovani prijimace pred zmenou urovne
      while (tape.rxPhase && tape.rxNext <= tSeg) tapeUartSample();
      const int lvl = tape.img.bit(i);
      if (lvl != tape.line) {
        if (tape.line == 1 && lvl == 0 && tape.rxPhase == 0) {
          // hrana start bitu: stred start bitu za pul bitu
          tape.rxPhase = 1; tape.rxByte = 0;
          tape.rxNext = tSeg + serialHalfBitCycles();
          skstat &= ~0x02;                             // seriovy vstup prijima
        }
        tape.line = lvl;
      }
      i++;
    }
    tape.pos = p1;
    if ((size_t)tape.pos >= tape.img.n) tape.line = 1; // konec pasky
  }
  void tapeUartSample() {
    const int v = tape.line;
    const double bit = 2.0 * serialHalfBitCycles();
    if (tape.rxPhase == 1) {
      if (v == 0) { tape.rxPhase = 2; tape.rxNext += bit; }
      else { tape.rxPhase = 0; skstat |= 0x02; }       // jen zakmit
      return;
    }
    if (tape.rxPhase <= 9) {
      if (v) tape.rxByte |= 1 << (tape.rxPhase - 2);
      tape.rxPhase++; tape.rxNext += bit;
      return;
    }
    // stop bit
    tape.rxPhase = 0; skstat |= 0x02;
    if (v) tape.bytes++; else tape.framing++;
    receiveSerialByte((uint8_t)tape.rxByte, 0, !v);
  }

  // =================================================================
  //  SIO D1: na urovni prikazu (jako "SIO patch" v emulatorech)
  // =================================================================
  void sioPatch() {
    sioPrikazu++;
    const uint8_t ddevic = mem.ram[0x300], dunit = mem.ram[0x301], dcomnd = mem.ram[0x302];
    const uint16_t dbuf = (uint16_t)(mem.ram[0x304] | (mem.ram[0x305] << 8));
    const int dbyt = mem.ram[0x308] | (mem.ram[0x309] << 8);
    const int daux = mem.ram[0x30A] | (mem.ram[0x30B] << 8);
    uint8_t st = 138;                         // timeout = zarizeni neodpovida
    const int dev = (ddevic + dunit - 1) & 0xFF;
    if (dev == 0x31 && disk.mounted) {
      switch (dcomnd) {
        case 0x52: {                          // READ SECTOR
          size_t off; int len;
          if (disk.sec(daux, off, len)) {
            for (int i = 0; i < dbyt; i++) pokeMem((uint16_t)(dbuf + i), i < len ? disk.data[off + i] : 0);
            st = 1; disk.reads++;
          } else st = 144;
          break;
        }
        case 0x57: case 0x50: {               // WRITE (with verify)
          size_t off; int len;
          if (disk.writeProtect) st = 144;
          else if (disk.sec(daux, off, len)) {
            for (int i = 0; i < len && i < dbyt; i++) disk.data[off + i] = peek((uint16_t)(dbuf + i));
            st = 1; disk.writes++;
          } else st = 144;
          break;
        }
        case 0x53: {                          // STATUS
          uint8_t s[4] = {(uint8_t)(0x10 | (disk.sectorSize == 256 ? 0x20 : 0) | (disk.writeProtect ? 0x08 : 0)), 0xFF, 0xE0, 0x00};
          for (int i = 0; i < dbyt && i < 4; i++) pokeMem((uint16_t)(dbuf + i), s[i]);
          st = 1;
          break;
        }
        case 0x21: case 0x22: {               // FORMAT
          if (disk.writeProtect) st = 144;
          else { std::fill(disk.data.begin(), disk.data.end(), 0); for (int i = 0; i < dbyt; i++) pokeMem((uint16_t)(dbuf + i), 0xFF); st = 1; }
          break;
        }
        default: st = 139; break;             // NAK
      }
    } else if (!sioRychlyTimeout) {
      st = 138;
    }
    mem.ram[0x303] = st;
    cpu.y = st;
    cpu.p = (uint8_t)((cpu.p & ~(F_N | F_Z)) | (st & 0x80) | (st ? 0 : F_Z));
    if (st == 1) cpu.p &= ~F_C;
    // RTS (navrat na volajiciho SIOV) - zaroven nekolik cyklu "behu"
    cpu.s++; uint8_t lo = mem.ram[0x100 | cpu.s];
    cpu.s++; uint8_t hi = mem.ram[0x100 | cpu.s];
    cpu.pc = (uint16_t)(((hi << 8) | lo) + 1);
    for (int i = 0; i < 6; i++) { beginCpuCycle(false); endCycle(); }
    if (sioRychlyTimeout) sioZkratek++;
  }
};

} // namespace nap
