// nap_atari_machine.h
// BUILD2SA17: stroj - hardwarove registry a snimkova smycka.
//
// Ucel: rozbehnout SELF-TEST Z ROM ATARI ($5000-$57FF). Je to Reneho vlastni
// ROM a jeho vlastni diagnostika - neni to nic, co bych si vymyslel.
// Self-test si sam nastavi display list, obrazovku i barvy a sam si otestuje
// pamet. Kdyz to nase jadro rozbehne a ukaze totez co skutecne Atari,
// je to dukaz. Kdyz ne, je videt PRESNE kde to skonci.
//
// Co uz tu je: ANTIC (DMACTL, DLIST, CHBASE, NMIEN/NMIST, VCOUNT, WSYNC),
//              GTIA (barvy, CONSOL, TRIG, PAL), PIA, POKEY jen tolik,
//              aby se ROM nezasekla (RANDOM, SKSTAT, IRQST, KBCODE).
// Co tu NENI: zvuk, hraci a strely, kolize, skrolovani, ostatni graficke
//              rezimy. Nebudu predstirat opak.
#pragma once
#include <cstdint>
#include <cstring>
#include "nap_atari_cpu.h"
#include "nap_atari_mem.h"
#include "nap_atari_video.h"

namespace nap {

struct Machine {
  AtariMem mem;
  Cpu6502  cpu;

  // ANTIC
  int dmactl = 0, chactl = 0, dlistL = 0, dlistH = 0;
  int hscrol = 0, vscrol = 0, pmbase = 0, chbase = 0;
  int nmien = 0, nmist = 0x1F;
  int line = 0;            // scanline 0..311 (PAL)
  long long frame = 0;

  // GTIA
  int hposp[4] = {0,0,0,0}, hposm[4] = {0,0,0,0};
  int sizep[4] = {0,0,0,0}, sizem = 0;
  int grafp[4] = {0,0,0,0}, grafm = 0;
  int colpm[4] = {0,0,0,0};
  int colpf[4] = {0,0,0,0};
  int colbk = 0, prior = 0, gractl = 0, vdelay = 0;
  int consol = 7;          // 0 = stisknuto; 7 = nic
  int trig[4] = {1,1,1,1};

  // POKEY - jen tolik, aby ROM nezustala viset
  int skctl = 0, irqen = 0, kbcode = 0;
  int irqst = 0xFF;        // 0 v bitu = preruseni CEKA
  int serout = 0;
  int serStav = 0;         // 0 klid, 1 bajt se posouva, 2 posunuty, ceka se na dalsi
  int serOdpocet = 0;
  uint32_t rngState = 0x2A5C1D7B;

  const uint8_t *osRom = nullptr;
  const uint8_t *basRom = nullptr;

  // Prubezne cteni display listu - ANTIC ho nezpracuje najednou, ale
  // postupne, jak sjizdi obrazovku. Bez toho by DLI nemely kdy zabrat.
  AnticView *view = nullptr;

  // SLEDOVANI PAPRSKU
  // Program meni registry GTIA UPROSTRED radky (typicky hned po WSYNC).
  // Kdyz se pro cely radek vezme jedna hodnota, multiplexovany kernel se
  // rozsype - u Decathlonu se z atletu stanou svisle bloky.
  // Proto se kazdy zapis zaznamena i s cyklem, ve kterem prisel, a pri
  // kresleni se pro kazdy bod pouzije to, co v tu chvili platilo.
  long long lineCyc0 = 0;
  static const int SEG_MAX = 192;
  int segN = 0;
  int segCyk[SEG_MAX], segReg[SEG_MAX], segHod[SEG_MAX];
  // 0x00-0x1B registry GTIA, 0x20-0x2F registry ANTIC
  int segZac[0x30];                       // stav na ZACATKU radky
  int dlPc = 0, dlScreen = -1, dlMode = 0, dlZbyva = 0, dlRadek = 0, dlKroku = 0;
  bool dlDli = false;
  bool dlKonec = false;
  bool dlHskrol = false, dlVskrol = false;
  bool stopa=false; int stopaMode[240]={0}, stopaScr[240]={0}, stopaRad[240]={0};

  Machine()
    : cpu([this](int a){ return this->read(a); },
          [this](int a,int v){ this->write(a,v); }) {}

  inline int rnd() {
    rngState ^= rngState << 13; rngState ^= rngState >> 17; rngState ^= rngState << 5;
    return (int)(rngState >> 16) & 0xFF;
  }

  int read(int a) {
    a &= 0xFFFF;
    if (a >= 0xD000 && a < 0xD800) return hwRead(a);
    return mem.cpuRead(a);
  }
  void write(int a, int v) {
    a &= 0xFFFF; v &= 0xFF;
    if (a >= 0xD000 && a < 0xD800) { hwWrite(a, v); return; }
    mem.cpuWrite(a, v);
  }

  int hwRead(int a) {
    const int page = a & 0xFF00;
    if (page == 0xD000) {                       // GTIA
      const int r = a & 0x1F;
      if (r == 0x10) return trig[0];            // TRIG0
      if (r == 0x11) return trig[1];
      if (r == 0x12) return trig[2];
      if (r == 0x13) return trig[3];
      if (r == 0x14) return 0x01;               // PAL: 1 = PAL stroj
      if (r == 0x1F) return consol & 7;         // CONSOL
      return 0x0F;                              // kolize - zatim nic
    }
    if (page == 0xD200) {                       // POKEY
      const int r = a & 0x0F;
      if (r == 0x09) return kbcode;             // KBCODE
      if (r == 0x0A) return rnd();              // RANDOM
      if (r == 0x0E) return irqst;              // IRQST
      if (r == 0x0F) return 0xFF;               // SKSTAT - klid
      return 0xFF;
    }
    if (page == 0xD300) {                       // PIA
      const int r = a & 0x03;
      if (r == 0) return (mem.pia.ctlA & 4) ? 0xFF : mem.pia.ddrA;
      if (r == 1) return (mem.pia.ctlB & 4) ? mem.portB() : mem.pia.ddrB;
      if (r == 2) return mem.pia.ctlA;
      return mem.pia.ctlB;
    }
    if (page == 0xD400) {                       // ANTIC
      const int r = a & 0x0F;
      if (r == 0x0B) return (line >> 1) & 0xFF; // VCOUNT
      if (r == 0x0F) return nmist;              // NMIST
      return 0xFF;
    }
    return 0xFF;
  }

  void hwWrite(int a, int v) {
    const int page = a & 0xFF00;
    if (page == 0xD000) {                       // GTIA
      const int r = a & 0x1F;
      if (r < 0x1C && segN < SEG_MAX) {         // poznamenat CYKLUS zapisu
        segCyk[segN] = (int)(cpu.c.cycles - lineCyc0);
        segReg[segN] = r; segHod[segN] = v & 0xFF; segN++;
      }
      if (r <= 0x03) { hposp[r] = v; return; }
      if (r <= 0x07) { hposm[r - 4] = v; return; }
      if (r <= 0x0B) { sizep[r - 8] = v; return; }
      if (r == 0x0C) { sizem = v; return; }
      if (r >= 0x0D && r <= 0x10) { grafp[r - 0x0D] = v; return; }
      if (r == 0x11) { grafm = v; return; }
      if (r >= 0x12 && r <= 0x15) { colpm[r - 0x12] = v; return; }
      if (r >= 0x16 && r <= 0x19) { colpf[r - 0x16] = v; return; }
      if (r == 0x1A) { colbk = v; return; }
      if (r == 0x1B) { prior = v; return; }
      if (r == 0x1C) { vdelay = v; return; }
      if (r == 0x1D) { gractl = v & 7; return; }
      if (r == 0x1F) {
        // POZOR: zapis do CONSOL ovlada REPRODUKTOR a vystupni zapadku.
        // NESMI mazat stav tlacitek - OS sem pise pri KAZDEM snimku
        // (pro klapnuti reproduktoru), takze by tim smazal kazdy stisk
        // START/SELECT/OPTION driv, nez si ho program stihne precist.
        return;
      }
      return;
    }
    if (page == 0xD200) {                       // POKEY
      const int r = a & 0x0F;
      if (r == 0x0D) {                          // SEROUT
        // POKEY hlasi DVE ruzne veci a NE naraz:
        //   bit4 = "posunul jsem bajt, dej dalsi"
        //   bit3 = "uz nic neposilam, vysilani skoncilo"
        // Kdyz se nastavi obe zaroven, SIO rutina si mysli, ze je hotovo
        // hned po prvnim bajtu, a zustane viset. Musi to jit po sobe.
        serout = v;
        serStav = 1;
        serOdpocet = 2;                         // ~2 radky na bajt
        return;
      }
      if (r == 0x0E) {                          // IRQEN
        irqen = v;
        irqst |= (~v) & 0xFF;                   // zakazane se rovnou zahodi
        obnovIrq();
        return;
      }
      if (r == 0x0F) { skctl = v; return; }
      return;
    }
    if (page == 0xD300) { mem.piaWrite(a, v); return; }
    if (page == 0xD400) {                       // ANTIC
      {
        // CHBASE, HSCROL a VSCROL se meni PRES DLI uprostred obrazu.
        // Kdyz se ctou az na konci snimku, cte se znakova sada z mista,
        // kde uz zadna neni - u Decathlonu vyslo CHBASE=$00 a misto
        // tabule byla kase ze systemovych promennych.
        const int r = 0x20 + (a & 0x0F);
        if (segN < SEG_MAX) {
          segCyk[segN] = (int)(cpu.c.cycles - lineCyc0);
          segReg[segN] = r; segHod[segN] = v & 0xFF; segN++;
        }
      }
      switch (a & 0x0F) {
        case 0x00: dmactl = v; return;
        case 0x01: chactl = v; return;
        case 0x02: dlistL = v; return;
        case 0x03: dlistH = v; return;
        case 0x04: hscrol = v; return;
        case 0x05: vscrol = v; return;
        case 0x07: pmbase = v; return;
        case 0x09: chbase = v; return;
        case 0x0A: {                            // WSYNC
          // Zapis do WSYNC ZASTAVI procesor az do konce radky. Bez toho
          // se rozsype casovani vsech kernelu, ktere synchronizuji na
          // paprsek - a to je skoro kazda hra.
          const long long cyk = cpu.c.cycles - lineCyc0;
          if (cyk < 105) cpu.c.cycles = lineCyc0 + 105;
          return;
        }
        case 0x0E: nmien = v; return;
        case 0x0F: nmist = 0x1F; return;        // NMIRES
      }
      return;
    }
  }

  void obnovIrq() {
    cpu.c.irqLine = (((~irqst) & irqen) & 0xFF) ? 1 : 0;
  }

  /** Stisk klavesy: OS ji prevezme pres preruseni z POKEY. */
  void klavesa(int kod) {
    kbcode = kod & 0xFF;
    irqst &= ~0x40;
    obnovIrq();
  }

  int dlistAddr() const { return (dlistL | (dlistH << 8)) & 0xFFFF; }

  void reset() {
    mem.pia = Pia();
    cpu.c = CpuState();
    cpu.reset();
  }

  void dalsiDlInstrukce() {
    for (int ochrana = 0; ochrana < 8; ochrana++) {
      if (++dlKroku > 300) { dlMode = 0; dlZbyva = 240; dlDli = false; return; }
      const int op = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
      const int m = op & 0x0F;
      dlDli = (op & 0x80) != 0;
      if (m == 0) { dlMode = 0; dlZbyva = ((op >> 4) & 7) + 1; dlRadek = 0;
                    dlHskrol = false; dlVskrol = false; return; }
      // POZOR NA PORADI: bit4 je SVISLE, bit5 je VODOROVNE skrolovani.
      // Mel jsem to prohozene, takze se u rezimu s vodorovnym skrolem
      // menil pocatecni radek znaku misto sirsiho nataceni z pameti.
      dlVskrol = (op & 0x10) != 0;      // bit4 = svisle
      dlHskrol = (op & 0x20) != 0;      // bit5 = vodorovne

      if (m == 1) {
        // Skok. Adresa je u OBOU variant - to jsem drive u $01 vubec necetl.
        const int lo = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        const int hi = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        dlPc = lo | (hi << 8);
        if (op & 0x40) {                            // $41 = JVB: KONEC SNIMKU
          dlKonec = true; dlMode = 0; dlZbyva = 240; dlRadek = 0;
          return;                                   // dal uz se nekresli
        }
        continue;                                   // $01 = JMP: cti dal
      }
      if (op & 0x40) {                              // LMS
        const int lo = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        const int hi = mem.cpuRead(dlPc); dlPc = (dlPc + 1) & 0xFFFF;
        dlScreen = lo | (hi << 8);
      }
      dlMode = m; dlRadek = dlVskrol ? (vscrol & 15) : 0;
      dlZbyva = AnticView::popisRezimu(m).scanline - dlRadek;
      if (dlZbyva <= 0) dlZbyva = 1;
      return;
    }
    dlMode = 0; dlZbyva = 240; dlDli = false;
  }

  /** Hodnota registru r v okamziku, kdy paprsek prochazel bodem x. */
  int regNaX(int r, int x) const {
    // 1 cyklus procesoru = 2 barvove takty = 4 body ve framebufferu,
    // a bod x=0 odpovida barvovemu taktu 32
    int v = segZac[r];
    for (int i = 0; i < segN; i++) {
      if (segReg[i] != r) continue;
      const int xz = segCyk[i] * 4 - 64;
      if (x >= xz) v = segHod[i];
    }
    return v;
  }

  // Jedna scanline. PAL: 312 radku po 114 cyklech.
  void runScanline() {
    lineCyc0 = cpu.c.cycles;
    segN = 0;
    for (int p = 0; p < 4; p++) {
      segZac[p] = hposp[p]; segZac[4+p] = hposm[p];
      segZac[8+p] = sizep[p]; segZac[0x0D+p] = grafp[p];
      segZac[0x12+p] = colpm[p]; segZac[0x16+p] = colpf[p];
    }
    segZac[0x0C] = sizem; segZac[0x11] = grafm;
    segZac[0x1A] = colbk; segZac[0x1B] = prior;
    segZac[0x20] = dmactl; segZac[0x21] = chactl;
    segZac[0x24] = hscrol; segZac[0x25] = vscrol;
    segZac[0x27] = pmbase; segZac[0x29] = chbase;

    bool dliTed = false;
    if (line == 8) {
      dlPc = dlistAddr(); dlScreen = -1; dlZbyva = 0; dlKroku = 0; dlRadek = 0; dlKonec = false;
      if (view) view->vymaz(colbk);
    }
    // ANTIC si nacte instrukci display listu na zacatku radky
    int mode = 0, screen = -1, radek = 0;
    const bool viditelna = (line >= 8 && line < 248 && view);
    if (viditelna && (dmactl & 3) && !dlKonec) {
      if (dlZbyva == 0) dalsiDlInstrukce();
      mode = dlMode; screen = dlScreen; radek = dlRadek;
    }

    // TEPRVE TED procesor - behem nej se zaznamenavaji zapisy do GTIA
    const long long konec = lineCyc0 + 114;
    while (cpu.c.cycles < konec && !cpu.c.jam) cpu.step();

    if (viditelna) {
      const int y = line - 8;
      // ANTIC vyrobi proud kodu, GTIA z nej udela obraz
      if (stopa && y < 240) { stopaMode[y] = mode; stopaScr[y] = screen; stopaRad[y] = radek; }
      if ((dmactl & 3) && !dlKonec && mode != 0)
        view->anRadek(mem, mode, screen, radek, segZac[0x20], segZac[0x29],
                      dlHskrol, segZac[0x24]);
      else
        view->anPozadi();
      view->gtiaRadek(mem, y, line, *this);
      if ((dmactl & 3) && !dlKonec) {
        dlRadek++;
        if (--dlZbyva == 0) {
          if (dlMode != 0 && dlScreen >= 0) dlScreen += AnticView::bajtuNaRadek(dlMode, dmactl, dlHskrol);
          dliTed = dlDli;
        }
      }
    }
    if (dliTed && (nmien & 0x80)) {
      nmist = (nmist & 0x3F) | 0x80;
      cpu.c.nmiPending = true;
    }
    // POKEY: posun serioveho bajtu
    if (serStav && --serOdpocet <= 0) {
      if (serStav == 1) { irqst &= ~0x10; serStav = 2; serOdpocet = 2; }
      else              { irqst &= ~0x08; serStav = 0; }
      obnovIrq();
    }
    line++;
    if (line == 248) {
      nmist = (nmist & 0x3F) | 0x40;
      if (nmien & 0x40) cpu.c.nmiPending = true;
    }
    if (line >= 312) { line = 0; frame++; }
  }

  void runFrame() { const long long f = frame; while (frame == f) runScanline(); }
};

} // namespace nap
