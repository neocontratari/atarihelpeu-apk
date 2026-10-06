// nap_atari_vbxe.h
// B296: VBXE (VideoBoard XE, jadro FX 1.26) - pridavna graficka karta do
// Atari 130XE. Rene: "chci zaraz otestovat i VBXE - tesim se na W3D a Popeye".
//
// Co VBXE je (fakta o hardwaru: dokumentace VBXE FX 1.2x a zdrojovy kod
// emulatoru Altirra jako zdroj FAKTU - kod je napsany znovu):
//  - vlastni pamet 512 kB (VRAM), procesor ji vidi okny MEMAC A / MEMAC B,
//  - registry na $D640-$D65F (cteni: verze $10/$26, kolize, stav blitru,
//    IRQ, MEMAC; zapis: rizeni obrazu, XDL, paleta, blitter, priority),
//  - nahrazuje GTIA: barvy jdou pres 4 palety po 256 barvach (RGB 7 bitu),
//    paleta 0 = puvodni barvy Atari,
//  - XDL (rozsireny display list ve VRAM) zapina po radcich "overlay":
//    LR 160 bodu / SR 320 bodu / HR 640 bodu (16 barev) / text 80 sloupcu,
//    atributovou mapu (barvy PF0-PF2, paleta a priorita po bunkach),
//  - blitter: kopirovani/vyplnovani bloku ve VRAM (BCB po 21 bajtech,
//    rezimy 0-6, zoom, vzor, kolize), IRQ po dokonceni.
// Obraz z VBXE ma 4 body na barevny takt (640 bodu na sirku hraciho pole).
#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace nap {

struct Vbxe {
  enum : uint8_t { OV_OFF = 0, OV_LR = 1, OV_SR = 2, OV_HR = 3, OV_TEXT = 4 };
  enum : uint8_t { W_NARROW = 0, W_NORMAL = 1, W_WIDE = 2 };
  enum : uint8_t { B_STOPPED = 0, B_RELOAD = 1, B_PROCESS = 2, B_STOPPING = 3 };
  static const uint32_t MASK = 0x7FFFF;

  std::vector<uint8_t> vram;            // 512 kB

  // ---------------- registry ----------------
  uint8_t memacCtl = 0, memacBankA = 0, memacBankB = 0;
  uint32_t xdlBase = 0, xdlAddr = 0; bool xdlActive = false, xdlEnabled = false; uint32_t xdlRepeat = 0;
  uint8_t ovMode = OV_OFF, ovWidth = W_NORMAL;
  bool ovTrans = true, ovTrans15 = false;
  uint8_t ovHscroll = 0, ovVscroll = 0;
  uint8_t ovMainPri = 0, ovPri[4] = {0, 0, 0, 0};    // "nativni" = ~pri s prohozenymi nibbly
  uint8_t collMask = 0, collState = 0;                // nativni poradi (PF dole, P nahore)
  uint32_t ovAddr = 0, ovStep = 0, ovTextRow = 0, chAddr = 0;
  uint8_t pfPal = 0, ovPal = 1;
  bool extColor = false, mapOn = false;
  uint32_t mapAddr = 0, mapStep = 0, mapW = 8, mapH = 8, mapHs = 0, mapVs = 0, mapRow = 0;
  uint8_t psel = 0, csel = 0;
  bool irqEn = false, irqReq = false;
  uint8_t configLatch = 0;
  uint32_t pal[4][256];                 // RGBA jako AnticView::fb (A<<24|B<<16|G<<8|R)
  uint32_t defPal[256];
  bool pal0Dirty = false;               // program zmenil paletu 0

  // okna MEMAC po 4 kB strankach: posun ve VRAM nebo -1
  int32_t mapCpu[16], mapAntic[16];
  bool mapAny = false;

  // ---------------- blitter ----------------
  uint8_t blitState = B_STOPPED;
  bool blitContinue = false;
  uint64_t blitStopTime = 0, blitEndScanTime = 0;
  bool stopEvent = false;               // IRQ/konec blitru ceka na cas blitStopTime
  uint8_t blitMode = 0;
  int32_t blitCyclesLeft = 0, blitCyclesPerRow = 0, blitSavedPerZero = 0;
  uint32_t blitListAddr = 0, blitFetch = 0;
  uint32_t blitSrc = 0, blitDst = 0; int32_t blitSrcStepX = 0, blitSrcStepY = 0, blitDstStepX = 0, blitDstStepY = 0;
  uint32_t blitWidth = 1, blitHeight = 1, blitHeightLeft = 0;
  uint8_t blitAnd = 0, blitXor = 0, blitCollMask = 0, blitActiveCollMask = 0, blitPattern = 0, blitCollCode = 0;
  uint8_t blitZoomX = 1, blitZoomY = 1, blitZoomCounterY = 0;

  // ---------------- diagnostika (log v appce) ----------------
  long long nWrites = 0, nBlits = 0, nBlitLists = 0, nXdlFrames = 0, nPalWrites = 0, nMemacSets = 0;
  int lastOvMode = 0;

  // ---------------- radek ----------------
  // atributova mapa pro radek (pul barevneho taktu = 14 MHz bod)
  struct AttrPx { uint8_t pf0, pf1, pf2, ctrl, hiresFlag, pri; };
  AttrPx attrMap[456];
  bool attrFromMap[456];
  uint8_t ovDec[912 + 16];              // dekodovany overlay (4 body na takt)
  uint8_t ovTextTrans[912 + 16];        // text: 1 = bod viditelny
  uint8_t ovPriBuf[912];                // na pul taktu: [0] priorita nad overlay, [1] kolize
  uint8_t ctrlBuf[456];                 // atribut (ovladani) na pul taktu - paleta overlay
  uint32_t lineBuf[912];                // vystup radku: 4 body na barevny takt

  Vbxe() {
    vram.assign(512 * 1024, 0);
    for (int i = 0; i < 16; i++) mapCpu[i] = mapAntic[i] = -1;
    std::memset(pal, 0, sizeof pal); std::memset(defPal, 0, sizeof defPal);
    std::memset(attrMap, 0, sizeof attrMap); std::memset(attrFromMap, 0, sizeof attrFromMap);
    std::memset(ovDec, 0, sizeof ovDec); std::memset(ovTextTrans, 0, sizeof ovTextTrans);
    std::memset(ovPriBuf, 0, sizeof ovPriBuf); std::memset(ctrlBuf, 0, sizeof ctrlBuf);
    std::memset(lineBuf, 0, sizeof lineBuf);
  }

  // priorita z registru (P0-P3 dole, PF0-PF3 nahore, 1 = overlay NAD vrstvou)
  // -> nativni tvar: 1 = vrstva NAD overlay, poradi jako bity GTIA
  static uint8_t toNative(uint8_t p) { p = (uint8_t)~p; return (uint8_t)((p << 4) | (p >> 4)); }
  // FX 1.26: PF2 a PF3 sdili bit priority, bit PF3 znamena pozadi (BAK)
  static uint8_t priTrans(uint8_t i) { return (uint8_t)((i & 0xF3) + ((i & 0x0C) ? 0x04 : 0) + (i == 0 ? 0x08 : 0)); }

  bool irqOut() const { return irqReq && irqEn; }

  // ---------------- reset ----------------
  void setDefaultPalette(const uint32_t *p) {
    std::memcpy(defPal, p, sizeof defPal);
  }
  void coldReset() {
    std::memcpy(pal[0], defPal, sizeof pal[0]);
    std::memset(pal[1], 0, sizeof(uint32_t) * 256 * 3);
    for (int k = 1; k < 4; k++) for (int i = 0; i < 256; i++) pal[k][i] = 0xFF000000u;
    pal0Dirty = false;
    psel = csel = 0; memacBankA = memacBankB = memacCtl = 0;
    xdlAddr = xdlBase = 0; xdlEnabled = xdlActive = false;
    ovMode = OV_OFF; ovWidth = W_NORMAL; ovMainPri = 0; std::memset(ovPri, 0, sizeof ovPri);
    collMask = collState = 0; ovAddr = ovStep = 0; ovTrans = true; ovTrans15 = false; ovHscroll = ovVscroll = 0;
    blitListAddr = blitFetch = 0; pfPal = 0; ovPal = 1;
    mapW = mapH = 8; mapHs = 0; mapRow = 0; chAddr = 0; configLatch = 0;
    extColor = false; mapOn = false;
    warmReset();
  }
  void warmReset() {
    xdlEnabled = false; extColor = false; ovTrans = true; ovTrans15 = false;
    memacCtl &= 0xF3; memacBankA &= 0x7F; memacBankB &= 0x3F;
    blitState = B_STOPPED; blitCollCode = 0; blitCyclesLeft = 0; stopEvent = false;
    xdlActive = false; irqEn = false; irqReq = false;
    ovWidth = W_NORMAL; ovMode = OV_OFF;
    updateMaps();
  }

  // ---------------- MEMAC ----------------
  void updateMaps() {
    for (int i = 0; i < 16; i++) mapCpu[i] = mapAntic[i] = -1;
    // okno B: $4000-$7FFF, banka 16 kB (bity 0-4), bit 7 procesor, bit 6 ANTIC
    if (memacBankB & 0xC0) {
      const int32_t base = (int32_t)(memacBankB & 0x1F) << 14;
      for (int p = 4; p < 8; p++) {
        if (memacBankB & 0x80) mapCpu[p] = base + (p - 4) * 0x1000;
        if (memacBankB & 0x40) mapAntic[p] = base + (p - 4) * 0x1000;
      }
    }
    // okno A (ma prednost pred B): zacatek = horni nibble MEMAC_CONTROL,
    // velikost 4/8/16/32 kB, banka po 4 kB; bit 3 procesor, bit 2 ANTIC.
    // Okno, ktere by preteklo pres $FFFF, je useknute.
    if ((memacBankA & 0x80) && (memacCtl & 0x0C)) {
      static const int kPages[4] = {1, 2, 4, 8};
      static const uint32_t kMask[4] = {0x7F000, 0x7E000, 0x7C000, 0x78000};
      const int n = kPages[memacCtl & 3];
      const int p0 = memacCtl >> 4;
      const uint32_t base = ((uint32_t)memacBankA << 12) & kMask[memacCtl & 3];
      for (int k = 0; k < n && p0 + k < 16; k++) {
        if (memacCtl & 0x08) mapCpu[p0 + k] = (int32_t)(base + (uint32_t)k * 0x1000);
        if (memacCtl & 0x04) mapAntic[p0 + k] = (int32_t)(base + (uint32_t)k * 0x1000);
      }
    }
    mapAny = false;
    for (int i = 0; i < 16; i++) if (mapCpu[i] >= 0 || mapAntic[i] >= 0) mapAny = true;
  }

  // ---------------- registry ----------------
  bool blitterActive(uint64_t now) const {
    if (blitState == B_STOPPED) return false;
    if (blitState == B_STOPPING) return now < blitStopTime;
    return blitState == B_PROCESS;
  }
  // -1 = nic neodpovida (plovouci sbernice)
  int read(uint8_t lo, uint64_t now) const {
    switch (lo) {
      case 0x40: return 0x10;                              // CORE_VERSION: FX
      case 0x41: return 0x26;                              // MINOR_REVISION: 1.26
      case 0x4A: return (uint8_t)((collState << 4) | (collState >> 4));   // COLDETECT
      case 0x50: return blitCollCode;                      // BLT_COLLISION_CODE
      case 0x53: return (blitterActive(now) ? 0x02 : 0x00) | (blitState == B_RELOAD ? 0x01 : 0x00);
      case 0x54: return irqReq ? 0x01 : 0x00;              // IRQ_STATUS
      case 0x5E: return memacCtl;
      case 0x5F: return memacBankA;
      default: if (lo >= 0xC0) return configLatch; return -1;
    }
  }
  void write(uint8_t lo, uint8_t v, uint64_t now) {
    nWrites++;
    switch (lo) {
      case 0x40:                                           // VIDEO_CONTROL
        xdlEnabled = (v & 0x01) != 0; extColor = (v & 0x02) != 0;
        ovTrans = (v & 0x04) == 0; ovTrans15 = (v & 0x08) != 0;
        break;
      case 0x41: xdlBase = (xdlBase & 0x7FF00) | v; break;
      case 0x42: xdlBase = (xdlBase & 0x700FF) | ((uint32_t)v << 8); break;
      case 0x43: xdlBase = (xdlBase & 0x0FFFF) | ((uint32_t)(v & 7) << 16); break;
      case 0x44: csel = v; break;
      case 0x45: psel = v & 3; break;
      case 0x46: case 0x47: case 0x48: {                   // CR / CG / CB (7 bitu)
        const uint32_t c = (uint32_t)((v & 0xFE) | (v >> 7));
        uint32_t &e = pal[psel][csel];
        if (lo == 0x46) e = (e & 0xFFFFFF00u) | c;
        else if (lo == 0x47) e = (e & 0xFFFF00FFu) | (c << 8);
        else e = (e & 0xFF00FFFFu) | (c << 16);
        e |= 0xFF000000u;
        if (psel == 0) pal0Dirty = true;
        nPalWrites++;
        if (lo == 0x48) csel++;
        break;
      }
      case 0x49: collMask = v; break;                      // COLMASK
      case 0x4A: collState = 0; break;                     // COLCLR
      case 0x50: blitListAddr = (blitListAddr & 0x7FF00) | v; break;
      case 0x51: blitListAddr = (blitListAddr & 0x700FF) | ((uint32_t)v << 8); break;
      case 0x52: blitListAddr = (blitListAddr & 0x0FFFF) | ((uint32_t)(v & 7) << 16); break;
      case 0x53:                                           // BLITTER_START
        if (v & 1) {
          if (!blitterActive(now)) {
            blitState = B_RELOAD; blitContinue = true; blitFetch = blitListAddr;
            // spusteni uprostred radku: blitter ma jen zbytek radku
            int64_t maxT = ((int64_t)blitEndScanTime - (int64_t)now) * 8;
            if (maxT < 0) maxT = 0;
            if (blitCyclesLeft > maxT) blitCyclesLeft = (int32_t)maxT;
            nBlitLists++;
            // prvni BCB se nacte hned (nektera dema ho hned prepisuji)
            loadBlitter();
            runBlitter(now);
          }
        } else {
          blitState = B_STOPPED; stopEvent = false;
        }
        break;
      case 0x54:                                           // IRQ_CONTROL
        irqReq = false;
        irqEn = (v & 1) != 0;
        break;
      case 0x55: case 0x56: case 0x57: case 0x58: ovPri[lo - 0x55] = toNative(v); break;
      case 0x5D: if (memacBankB != v) { memacBankB = v; updateMaps(); nMemacSets++; } break;
      case 0x5E: if (memacCtl != v) { memacCtl = v; updateMaps(); nMemacSets++; } break;
      case 0x5F: if (memacBankA != v) { memacBankA = v; updateMaps(); nMemacSets++; } break;
      default: if (lo >= 0xC0) configLatch = v & 7; break;
    }
  }

  // ---------------- snimek / radek ----------------
  void beginFrame() {
    xdlActive = xdlEnabled;
    xdlAddr = xdlBase;
    xdlRepeat = 1;
    ovWidth = W_NORMAL; ovMode = OV_OFF;
    pfPal = 0; ovPal = 1;
    mapOn = false; mapW = mapH = 8; mapHs = mapVs = 0;
    ovHscroll = ovVscroll = 0;
    ovMainPri = 0;                       // overlay nad vsim ($FF)
    if (xdlActive) nXdlFrames++;
  }
  void endFrame() { xdlActive = false; xdlRepeat = 1; }

  // zacatek radku: XDL, atributova mapa, cykly pro blitter
  void beginScanline(uint64_t now) {
    blitEndScanTime = now + 114;
    bool reloadMap = false;
    uint32_t xdlCycles = 0;
    if (--xdlRepeat) {
      ovTextRow = (ovTextRow + 1) & 7;
    } else if (!xdlActive) {
      xdlRepeat = 0xFFFFFFFFu;
      mapOn = false;
      ovMode = OV_OFF;
    } else {
      const uint32_t start = xdlAddr;
      const uint8_t x1 = fetch(xdlAddr++), x2 = fetch(xdlAddr++);
      if (x1 & 4) ovMode = OV_OFF;
      else if (x1 & 3) {
        static const uint8_t kMode[3][4] = {
          {OV_TEXT, OV_TEXT, OV_TEXT, OV_TEXT},
          {OV_SR, OV_HR, OV_LR, OV_OFF},
          {OV_OFF, OV_OFF, OV_OFF, OV_OFF}};
        ovMode = kMode[(x1 & 3) - 1][(x2 >> 4) & 3];
      }
      if (x1 & 0x10) mapOn = false;
      else if (x1 & 0x08) { mapOn = true; reloadMap = true; }
      if (x1 & 0x20) xdlRepeat = fetch(xdlAddr++);          // RPTL
      ++xdlRepeat;
      if (x1 & 0x40) {                                      // OVADR + krok
        const uint32_t a0 = fetch(xdlAddr++), a1 = fetch(xdlAddr++), a2 = fetch(xdlAddr++);
        const uint32_t s0 = fetch(xdlAddr++), s1 = fetch(xdlAddr++);
        ovAddr = a0 | (a1 << 8) | (a2 << 16);
        ovStep = (s0 | (s1 << 8)) & 0xFFF;
      }
      if (x1 & 0x80) {                                      // OVSCRL
        ovHscroll = fetch(xdlAddr++) & 7;
        ovVscroll = fetch(xdlAddr++) & 7;
      }
      if (x2 & 0x01) chAddr = (uint32_t)fetch(xdlAddr++) << 11;   // CHBASE
      if (x2 & 0x02) {                                      // MAPADR + krok
        const uint32_t a0 = fetch(xdlAddr++), a1 = fetch(xdlAddr++), a2 = fetch(xdlAddr++);
        const uint32_t s0 = fetch(xdlAddr++), s1 = fetch(xdlAddr++);
        mapAddr = a0 | (a1 << 8) | (a2 << 16);
        mapStep = (s0 | (s1 << 8)) & 0xFFF;
        reloadMap = true;
      }
      if (x2 & 0x04) {                                      // MAPPAR
        mapHs = fetch(xdlAddr++) & 0x1F;
        mapVs = fetch(xdlAddr++) & 0x1F;
        mapW = (uint32_t)(fetch(xdlAddr++) & 0x1F) + 1;
        mapH = (uint32_t)(fetch(xdlAddr++) & 0x1F) + 1;
      }
      if (x2 & 0x08) {                                      // ATT: sirka, palety, priorita
        const uint8_t ctl = fetch(xdlAddr++), pri = fetch(xdlAddr++);
        ovWidth = ((ctl & 3) == 3) ? W_NARROW : (uint8_t)(ctl & 3);
        pfPal = ctl >> 6;
        ovPal = (ctl >> 4) & 3;
        ovMainPri = toNative(pri);
      }
      if (x2 & 0x80) xdlActive = false;                     // END
      ovTextRow = ovVscroll & 7;
      if (reloadMap) mapRow = mapVs % mapH;
      xdlCycles = xdlAddr - start;
      if (ovMode) lastOvMode = ovMode;
    }
    uint32_t mapCycles = 0;
    if (mapOn && (reloadMap || mapRow == 0)) mapCycles = 43 * 4;
    static const uint32_t kOvCycles[5][3] = {{0, 0, 0}, {128, 160, 168}, {256, 320, 336}, {256, 320, 336}, {195, 243, 255}};
    const uint32_t ovCycles = kOvCycles[ovMode][ovWidth];
    blitCyclesLeft += (int32_t)(8 * 114) - (int32_t)(xdlCycles + mapCycles + ovCycles);
    if (blitCyclesLeft > 8 * 114) blitCyclesLeft = 8 * 114;
    runBlitter(now);
  }
  // ma radek jit pres VBXE (jinak je vystup stejny jako GTIA s paletou 0)
  bool needsRender() const { return ovMode != OV_OFF || mapOn || extColor || pfPal != 0 || pal0Dirty; }

  void endScanline() {
    if (ovMode != OV_OFF) {
      if (ovMode != OV_TEXT || xdlRepeat == 1 || ovTextRow == 7) ovAddr += ovStep;
    }
    if (mapOn && ++mapRow >= mapH) { mapRow = 0; mapAddr += mapStep; }
  }

  inline uint8_t fetch(uint32_t a) const { return vram[a & MASK]; }

  // hranice overlay (barevne takty): uzky $40-$BF, normalni $30-$CF, siroky $2C-$D4
  static int boundL(int w) { static const int b[3] = {64, 48, 44}; return b[w]; }
  static int boundR(int w) { static const int b[3] = {192, 208, 212}; return b[w]; }

  // Atributova mapa pro cely radek (bunky ve VRAM, 4 bajty: PF0 PF1 PF2 CTRL)
  void prepareAttrLine() {
    std::memset(attrFromMap, 0, sizeof attrFromMap);
    if (!mapOn) return;
    const int xlh = boundL(ovWidth) * 2;
    int xrh = boundR(ovWidth) * 2;
    const int xrh2 = (xlh - (int)mapHs) + 43 * (int)mapW;    // nejvys 43 bunek
    if (xrh > xrh2) xrh = xrh2;
    const uint8_t cm = extColor ? 0xFF : 0xFE;
    const int hiresShift = mapW > 16 ? 2 : mapW > 8 ? 1 : 0;
    const uint32_t hs = mapHs % mapW;
    for (int xh = xlh; xh < xrh; xh++) {
      const uint32_t rel = (uint32_t)(xh - xlh) + hs;
      const uint32_t cell = rel / mapW, off = rel % mapW;
      const uint32_t a = mapAddr + cell * 4;
      AttrPx &p = attrMap[xh];
      p.pf0 = fetch(a) & cm; p.pf1 = fetch(a + 1) & cm; p.pf2 = fetch(a + 2) & cm; p.ctrl = fetch(a + 3);
      p.pri = ovPri[p.ctrl & 3];
      p.hiresFlag = (uint8_t)(((int8_t)(uint8_t)(p.pf0 << (off >> hiresShift))) >> 7);
      attrFromMap[xh] = true;
    }
  }

  // ---------------- overlay ----------------
  void decodeOverlay() {
    const int xl = boundL(ovWidth), xr = boundR(ovWidth);
    const int hs = ovMode == OV_TEXT ? ovHscroll : 0;
    const int xr2 = hs ? xr + 2 : xr;
    for (int x = xl; x < xr2; x++) {
      uint8_t *d = &ovDec[x * 4];
      const uint32_t rel = (uint32_t)(x - xl);
      switch (ovMode) {
        case OV_LR: { const uint8_t p = fetch(ovAddr + rel); d[0] = d[1] = d[2] = d[3] = p; break; }
        case OV_SR: { const uint8_t b0 = fetch(ovAddr + rel * 2), b1 = fetch(ovAddr + rel * 2 + 1); d[0] = d[1] = b0; d[2] = d[3] = b1; break; }
        case OV_HR: { const uint8_t b0 = fetch(ovAddr + rel * 2), b1 = fetch(ovAddr + rel * 2 + 1); d[0] = b0 >> 4; d[1] = b0 & 15; d[2] = b1 >> 4; d[3] = b1 & 15; break; }
        case OV_TEXT: {
          const uint32_t fa = (ovAddr + (rel & ~1u)) & MASK;
          const uint8_t ch = fetch(fa), at = fetch(fa + 1);
          const uint8_t data = vram[(chAddr + ovTextRow + ((uint32_t)ch << 3)) & MASK];
          const uint8_t nib = (rel & 1) ? (data & 15) : (data >> 4);
          const uint8_t base = at & 0x7F;
          uint8_t *t = &ovTextTrans[x * 4];
          for (int k = 0; k < 4; k++) {
            const bool on = (nib >> (3 - k)) & 1;
            if (at & 0x80) { d[k] = on ? base : (uint8_t)(0x80 + base); t[k] = 1; }
            else if (ovTrans) { d[k] = on ? base : 0; t[k] = on ? 1 : 0; }
            else { d[k] = on ? base : 0x80; t[k] = 1; }
          }
          break;
        }
        default: d[0] = d[1] = d[2] = d[3] = 0; break;
      }
    }
  }
  // slozeni overlay s hracim polem/hraci (lineBuf + ovPriBuf vyplnene po taktech)
  void composeOverlay() {
    if (ovMode == OV_OFF) return;
    decodeOverlay();
    const int xl = boundL(ovWidth), xr = boundR(ovWidth);
    const int hs = ovMode == OV_TEXT ? ovHscroll : 0;
    const bool text = ovMode == OV_TEXT;
    uint8_t cs = collState;
    for (int xh = xl * 2; xh < xr * 2; xh++) {
      const uint8_t pri = ovPriBuf[xh * 2], coll = ovPriBuf[xh * 2 + 1];
      const uint32_t *pp = pal[(ctrlBuf[xh] >> 4) & 3];
      for (int k = 0; k < 2; k++) {
        const int di = xh * 2 + k + hs;
        const uint8_t v = ovDec[di];
        if (ovTrans) {
          if (pri) continue;
          const bool vis = text ? ovTextTrans[di] != 0 : v != 0;
          if (!vis || (ovTrans15 && (v & 15) == 15)) continue;
          lineBuf[xh * 2 + k] = pp[v];
          if (collMask & (1 << (v >> 5))) cs |= coll;
        } else {
          if (collMask & (1 << (v >> 5))) cs |= coll;
          if (!pri) lineBuf[xh * 2 + k] = pp[v];
        }
      }
    }
    collState = cs;
  }

  // ---------------- blitter ----------------
  void assertIrq() {
    blitState = B_STOPPED; stopEvent = false;
    if (!irqReq) irqReq = true;
  }
  // volano z jadra po kazdem cyklu, kdyz stopEvent
  inline bool tick(uint64_t now) {
    if (stopEvent && now >= blitStopTime) { assertIrq(); return true; }
    return false;
  }
  void loadBlitter() {
    uint8_t b[21];
    for (int i = 0; i < 21; i++) b[i] = fetch(blitFetch + (uint32_t)i);
    blitFetch += 21;
    blitSrc = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16);
    blitSrcStepY = (int32_t)((((uint32_t)b[3] | ((uint32_t)b[4] << 8)) & 0x1FFF) ^ 0x1000) - 0x1000;
    blitSrcStepX = (int8_t)b[5];
    blitDst = (uint32_t)b[6] | ((uint32_t)b[7] << 8) | ((uint32_t)b[8] << 16);
    blitDstStepY = (int32_t)((((uint32_t)b[9] | ((uint32_t)b[10] << 8)) & 0x1FFF) ^ 0x1000) - 0x1000;
    blitDstStepX = (int8_t)b[11];
    blitWidth = (uint32_t)b[12] + ((uint32_t)(b[13] & 1) << 8) + 1;
    blitHeight = (uint32_t)b[14] + 1;
    blitAnd = b[15]; blitXor = b[16];
    blitCollMask = blitActiveCollMask = b[17];
    blitZoomX = (uint8_t)((b[18] & 7) + 1); blitZoomY = (uint8_t)(((b[18] >> 4) & 7) + 1);
    blitPattern = b[19];
    blitContinue = (b[20] & 0x08) != 0;
    blitMode = b[20] & 7;
    // pametove cykly na radek (rychlost blitru)
    const int32_t srcB = (int32_t)blitWidth, dstB = (int32_t)(blitWidth * blitZoomX);
    blitCyclesPerRow = dstB; blitSavedPerZero = 0;
    if (blitAnd) blitCyclesPerRow += srcB;
    if (blitAnd || blitXor) {
      if (blitMode == 1) {
        if (blitCollMask) { blitCyclesPerRow += dstB; blitSavedPerZero = blitAnd ? blitZoomX * 2 : blitZoomX; }
        else blitSavedPerZero = blitAnd ? blitZoomX : 0;
      } else if (blitMode != 0) {
        blitCyclesPerRow += dstB;
        blitSavedPerZero = blitAnd ? blitZoomX * 2 : blitZoomX;
      }
    }
    blitState = B_PROCESS;
    blitHeightLeft = blitHeight;
    blitZoomCounterY = 0;
    blitCollCode = 0;
    blitCyclesLeft -= 21;
    if (blitAnd == 0) --blitCyclesLeft;
    nBlits++;
  }
  void runBlitter(uint64_t now) {
    if (blitState == B_STOPPED || blitState == B_STOPPING) return;
    while (blitCyclesLeft > 0) {
      if (blitState == B_RELOAD) {
        if (!blitContinue) {
          blitState = B_STOPPING;
          blitStopTime = blitEndScanTime - (uint64_t)(int64_t)(blitCyclesLeft >> 3);
          stopEvent = false;
          if (now >= blitStopTime) assertIrq();
          else stopEvent = true;
          break;
        }
        loadBlitter();
        if (blitCyclesLeft <= 0) break;
      }
      const uint32_t zeros = runRow();
      if (blitMode != 0 && blitAnd != 0) blitCyclesLeft += blitSavedPerZero * (int32_t)zeros;
      blitCyclesLeft -= blitCyclesPerRow;
      blitDst += (uint32_t)blitDstStepY;
      if (++blitZoomCounterY >= blitZoomY) {
        blitZoomCounterY = 0;
        blitSrc += (uint32_t)blitSrcStepY;
        if (!--blitHeightLeft) blitState = B_RELOAD;
      }
    }
  }
  uint32_t runRow() {
    uint32_t s = blitSrc, d = blitDst;
    const uint32_t patW = (blitPattern & 0x80) ? (uint32_t)(blitPattern & 0x3F) + 1 : 0xFFFFFu;
    uint32_t patC = patW;
    const uint32_t sx = (uint32_t)blitSrcStepX, dx = (uint32_t)blitDstStepX;
    const uint32_t dxZ = (uint32_t)(blitDstStepX * blitZoomX);
    uint32_t zeros = 0;
    uint8_t *m = vram.data();
    if (blitMode == 0 || blitMode == 7) {          // 7 se chova jako 0 (kopie/vypln)
      for (uint32_t x = 0; x < blitWidth; x++) {
        const uint8_t c = (uint8_t)((m[s & MASK] & blitAnd) ^ blitXor);
        for (uint8_t i = 0; i < blitZoomX; i++) { m[d & MASK] = c; d += dx; }
        s += sx;
        if (!--patC) { patC = patW; s = blitSrc; }
      }
      return 0;
    }
    for (uint32_t x = 0; x < blitWidth; x++) {
      const uint8_t c = (uint8_t)((m[s & MASK] & blitAnd) ^ blitXor);
      if (c) {
        for (uint8_t i = 0; i < blitZoomX; i++) {
          const uint8_t dv = m[d & MASK];
          if (blitMode == 6) {
            const uint8_t cl = c & 0x0F, chh = c & 0xF0;
            uint8_t dl = dv & 0x0F, dh = dv & 0xF0;
            if (cl) {
              if (dl && ((1 << ((dv >> 1) & 7)) & blitActiveCollMask)) { blitCollCode = (uint8_t)((blitCollCode & 0xF0) + dl); blitActiveCollMask = 0; }
              dl = cl;
            }
            if (chh) {
              if (dh && ((1 << ((dv >> 5) & 7)) & blitActiveCollMask)) { blitCollCode = (uint8_t)((blitCollCode & 0x0F) + dh); blitActiveCollMask = 0; }
              dh = chh;
            }
            m[d & MASK] = (uint8_t)(dl + dh);
          } else {
            if (dv && ((1 << (dv >> 5)) & blitActiveCollMask)) { blitCollCode = dv; blitActiveCollMask = 0; }
            uint8_t r;
            switch (blitMode) {
              case 1: r = c; break;
              case 2: r = (uint8_t)(c + dv); break;
              case 3: r = (uint8_t)(c | dv); break;
              case 4: r = (uint8_t)(c & dv); break;
              default: r = (uint8_t)(c ^ dv); break;   // 5: XOR
            }
            m[d & MASK] = r;
          }
          d += dx;
        }
      } else {
        ++zeros;
        if (blitMode == 4) { for (uint8_t i = 0; i < blitZoomX; i++) { m[d & MASK] = 0; d += dx; } }
        else d += dxZ;
      }
      s += sx;
      if (!--patC) { patC = patW; s = blitSrc; }
    }
    return zeros;
  }
};

} // namespace nap
