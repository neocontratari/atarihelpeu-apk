// nap_atari_6502.h
// B292: procesor 6502 (NMOS, jako SALLY v Atari) PRESNE PO CYKLECH.
//
// Rene po B291: "pro jistotu udelej dukladnou kontrolu jadra". Stary
// procesor (nap_atari_cpu.cpp) delal celou instrukci najednou a cykly
// pricital az na konci - zapis do registru (napr. barvy) tak "prisel" o
// nekolik cyklu driv, nez na skutecnem Atari, a ANTIC nemel kdy procesoru
// brat cykly pro obraz. Tady KAZDY pristup na sbernici (i ty "zbytecne",
// ktere skutecny 6502 dela - falesna cteni a dvojity zapis u RMW instrukci)
// je jeden cyklus a jde pres Bus, ktery mezitim nechava bezet ANTIC, GTIA
// a POKEY. Poradi a adresy pristupu jsou podle dokumentace 6502
// ("6502.txt"/"64doc", J. West, M. Makela) a overuji se testem proti
// SingleStepTests (T. Harte) - 10 000 testu na kazdy opcode vcetne
// presneho seznamu cyklu (adresa, hodnota, cteni/zapis).
//
// Preruseni: 6502 rozhoduje o preruseni podle stavu linek na konci
// PREDPOSLEDNIHO cyklu instrukce. Proto kazda instrukce vola poll() tesne
// pred svym poslednim pristupem na sbernici. Vyjimky jako na skutecnem
// cipu: skok (branch) bez prechodu stranky se znovu nedotazuje, CLI/SEI/PLP
// meni priznak I az po dotazu, RTI pred nim; NMI behem BRK/IRQ "unese"
// vektor.
//
// Bus musi umet:
//   uint8_t rd(uint16_t a);            jeden cyklus - cteni
//   void    wr(uint16_t a, uint8_t v); jeden cyklus - zapis
//   bool    nmiPending();              zachycena hrana NMI (jeste neobslouzena)
//   void    nmiAck();                  procesor NMI prevzal
//   bool    irqActive();               uroven linky IRQ
#pragma once
#include <cstdint>

namespace nap {

enum : uint8_t { F_C = 0x01, F_Z = 0x02, F_I = 0x04, F_D = 0x08, F_B = 0x10, F_U = 0x20, F_V = 0x40, F_N = 0x80 };

template <class Bus>
struct Cpu6502T {
  Bus *bus = nullptr;
  uint16_t pc = 0;
  uint8_t a = 0, x = 0, y = 0, s = 0xFD, p = F_U | F_I;
  bool jam = false;              // KIL/JAM - procesor stoji (jen RESET pomuze)
  uint8_t jamOp = 0; uint16_t jamPc = 0;
  // rozhodnuti z posledniho dotazu (pred poslednim cyklem predchozi instrukce)
  bool takeNmi = false, takeIrq = false;
  // dotaz na preruseni uz v teto instrukci probehl (pro RDY/WSYNC - viz Machine)
  bool polled = false;
  // ANE/LXA: "magicka" konstanta nestabilnich instrukci (zavisi na kusu cipu)
  uint8_t magicAne = 0xEE, magicLxa = 0xEE;
  uint64_t instr = 0;            // pocet provedenych instrukci (diagnostika)

  // --- pomocne ---
  inline uint8_t rd(uint16_t ad) { return bus->rd(ad); }
  inline void wr(uint16_t ad, uint8_t v) { bus->wr(ad, v); }
  inline void nz(uint8_t v) { p = (uint8_t)((p & ~(F_N | F_Z)) | (v & F_N) | (v ? 0 : F_Z)); }
  inline void setf(uint8_t f, bool on) { p = on ? (uint8_t)(p | f) : (uint8_t)(p & ~f); }
  inline void poll() {
    polled = true;
    takeNmi = bus->nmiPending();
    takeIrq = !takeNmi && bus->irqActive() && !(p & F_I);
  }
  inline void push(uint8_t v) { wr((uint16_t)(0x100 | s), v); s--; }

  void powerOn() {
    a = x = y = 0; s = 0xFD; p = F_U | F_I; jam = false; takeNmi = takeIrq = false;
  }
  // RESET: 7 cyklu (3 "falesne" zapisy na zasobnik jsou ve skutecnosti cteni), I=1
  void reset() {
    jam = false; takeNmi = takeIrq = false;
    rd(pc); rd(pc);
    rd((uint16_t)(0x100 | s)); s--;
    rd((uint16_t)(0x100 | s)); s--;
    rd((uint16_t)(0x100 | s)); s--;
    p |= F_I;
    uint8_t lo = rd(0xFFFC);
    uint8_t hi = rd(0xFFFD);
    pc = (uint16_t)(lo | (hi << 8));
  }

  // ---------- aritmetika (NMOS, vcetne desitkoveho rezimu) ----------
  void adc(uint8_t v) {
    const unsigned c = p & F_C;
    if (!(p & F_D)) {
      unsigned t = a + v + c;
      setf(F_V, (~(a ^ v) & (a ^ t) & 0x80) != 0);
      setf(F_C, t > 0xFF);
      a = (uint8_t)t; nz(a);
    } else {
      unsigned t = (a & 0x0F) + (v & 0x0F) + c;
      if (t > 9) t += 6;
      if (t <= 0x0F) t = (t & 0x0F) + (a & 0xF0) + (v & 0xF0);
      else t = (t & 0x0F) + (a & 0xF0) + (v & 0xF0) + 0x10;
      setf(F_Z, ((a + v + c) & 0xFF) == 0);
      setf(F_N, (t & 0x80) != 0);
      setf(F_V, ((a ^ t) & 0x80) && !((a ^ v) & 0x80));
      if ((t & 0x1F0) > 0x90) t += 0x60;
      setf(F_C, (t & 0xFF0) > 0xF0);
      a = (uint8_t)t;
    }
  }
  void sbc(uint8_t v) {
    const unsigned c = p & F_C;
    unsigned t = (unsigned)a - v - (c ? 0 : 1);
    if (!(p & F_D)) {
      setf(F_V, ((a ^ v) & (a ^ t) & 0x80) != 0);
      setf(F_C, t < 0x100);
      a = (uint8_t)t; nz(a);
    } else {
      unsigned ta = (a & 0x0F) - (v & 0x0F) - (c ? 0 : 1);
      if (ta & 0x10) ta = ((ta - 6) & 0x0F) | ((a & 0xF0) - (v & 0xF0) - 0x10);
      else ta = (ta & 0x0F) | ((a & 0xF0) - (v & 0xF0));
      if (ta & 0x100) ta -= 0x60;
      setf(F_C, t < 0x100);
      nz((uint8_t)t);
      setf(F_V, ((a ^ t) & 0x80) && ((a ^ v) & 0x80));
      a = (uint8_t)ta;
    }
  }
  inline void cmp(uint8_t r, uint8_t v) { unsigned t = (unsigned)r - v; setf(F_C, r >= v); nz((uint8_t)t); }
  inline uint8_t asl(uint8_t v) { setf(F_C, v & 0x80); v <<= 1; nz(v); return v; }
  inline uint8_t lsr(uint8_t v) { setf(F_C, v & 1); v >>= 1; nz(v); return v; }
  inline uint8_t rol(uint8_t v) { uint8_t c = p & F_C; setf(F_C, v & 0x80); v = (uint8_t)((v << 1) | c); nz(v); return v; }
  inline uint8_t ror(uint8_t v) { uint8_t c = (p & F_C) ? 0x80 : 0; setf(F_C, v & 1); v = (uint8_t)((v >> 1) | c); nz(v); return v; }
  inline void bit(uint8_t v) { p = (uint8_t)((p & ~(F_N | F_V | F_Z)) | (v & (F_N | F_V)) | ((a & v) ? 0 : F_Z)); }
  void arr(uint8_t v) {
    uint8_t t = a & v;
    if (!(p & F_D)) {
      unsigned r = (t | ((p & F_C) << 8)) >> 1;
      a = (uint8_t)r; nz(a);
      setf(F_C, a & 0x40);
      setf(F_V, ((a & 0x40) ^ ((a & 0x20) << 1)) != 0);
    } else {
      unsigned r = (t | ((p & F_C) << 8)) >> 1;
      setf(F_N, p & F_C);
      setf(F_Z, r == 0);
      setf(F_V, ((r ^ t) & 0x40) != 0);
      if (((t & 0x0F) + (t & 0x01)) > 5) r = (r & 0xF0) | ((r + 6) & 0x0F);
      if (((t & 0xF0) + (t & 0x10)) > 0x50) { r = (r & 0x0F) | ((r + 0x60) & 0xF0); setf(F_C, true); }
      else setf(F_C, false);
      a = (uint8_t)r;
    }
  }

  // ---------- adresovani: vraci efektivni adresu, provede vsechny cykly
  //            krome posledniho (ten dela samotna instrukce) ----------
  inline uint16_t fetch16() { uint8_t lo = rd(pc++); uint8_t hi = rd(pc++); return (uint16_t)(lo | (hi << 8)); }
  inline uint16_t zpx(uint8_t r) { uint8_t z = rd(pc++); rd(z); return (uint8_t)(z + r); }
  inline uint16_t izx() { uint8_t z = rd(pc++); rd(z); z = (uint8_t)(z + x); uint8_t lo = rd(z); uint8_t hi = rd((uint8_t)(z + 1)); return (uint16_t)(lo | (hi << 8)); }
  inline uint16_t izyBase() { uint8_t z = rd(pc++); uint8_t lo = rd(z); uint8_t hi = rd((uint8_t)(z + 1)); return (uint16_t)(lo | (hi << 8)); }

  // cteni s indexem: falesne cteni z neopravene adresy jen pri prechodu stranky
  inline uint8_t rdIdx(uint16_t base, uint8_t r) {
    uint16_t ea = (uint16_t)(base + r);
    if ((ea ^ base) & 0xFF00) { rd((uint16_t)((base & 0xFF00) | (ea & 0x00FF))); poll(); return rd(ea); }
    poll(); return rd(ea);
  }
  // zapis/RMW s indexem: falesne cteni z neopravene adresy VZDY
  inline uint16_t eaIdxW(uint16_t base, uint8_t r) {
    uint16_t ea = (uint16_t)(base + r);
    rd((uint16_t)((base & 0xFF00) | (ea & 0x00FF)));
    return ea;
  }
  // RMW: cteni, falesny zapis puvodni hodnoty, zapis nove
  template <class F> inline void rmw(uint16_t ea, F f) {
    uint8_t v = rd(ea);
    wr(ea, v);
    uint8_t n = f(v);
    poll();
    wr(ea, n);
  }

  // ---------- preruseni (IRQ/NMI/BRK) ----------
  void interrupt(bool fromBrk) {
    // BRK: druhy bajt instrukce se cte a PC se posune; IRQ/NMI: falesna cteni
    if (fromBrk) { rd(pc); pc++; }
    else { rd(pc); rd(pc); }
    push((uint8_t)(pc >> 8));
    push((uint8_t)pc);
    // NMI, ktere prijde nejpozdeji tady, "unese" vektor (i u BRK a IRQ)
    bool nmi = bus->nmiPending();
    push((uint8_t)(p | F_U | (fromBrk ? F_B : 0)));
    // chyba NMOS 6502: NMI, ktere prijde PRESNE v cyklu zapisu P na
    // zasobnik (vcetne cyklu DMA pred nim), se ztrati - vektor uz je
    // rozhodnuty jako IRQ a hrana NMI je "spotrebovana" (Acid800 "Blocked NMIs")
    if (!nmi && bus->nmiPending()) bus->nmiAck();
    p |= F_I;
    uint16_t vec = nmi ? 0xFFFA : 0xFFFE;
    if (nmi) bus->nmiAck();
    uint8_t lo = rd(vec);
    uint8_t hi = rd((uint16_t)(vec + 1));
    pc = (uint16_t)(lo | (hi << 8));
    // po vstupu do preruseni se dalsi preruseni nebere hned
    takeNmi = false; takeIrq = false;
  }

  inline void branch(bool cond) {
    // 2. cyklus: operand (dotaz na preruseni je pred nim)
    poll();
    int8_t off = (int8_t)rd(pc++);
    if (!cond) return;
    // 3. cyklus: falesne cteni dalsiho opcode, bez noveho dotazu (zvlastnost 6502)
    rd(pc);
    uint16_t t = (uint16_t)(pc + off);
    if ((t ^ pc) & 0xFF00) {
      // 4. cyklus: oprava stranky - tady se dotaz zase dela
      poll();
      rd((uint16_t)((pc & 0xFF00) | (t & 0x00FF)));
    }
    pc = t;
  }

  // ---------- jedna instrukce (nebo vstup do preruseni) ----------
  void step() {
    polled = false;
    if (jam) { rd(0xFFFF); return; }
    if (takeNmi || takeIrq) {
      bool n = takeNmi;
      if (n) { bus->nmiAck(); bus->traceNmi(); }
      // NMI se zjisti znovu uvnitr interrupt() (unos vektoru) - kdyz uz bylo
      // prevzate, vektor je NMI.
      if (n) { interruptNmi(); return; }
      interrupt(false);
      return;
    }
    instr++;
    uint8_t op = rd(pc++);
    uint16_t ea; uint8_t v;
    switch (op) {
#define IMM()  { poll(); v = rd(pc++); }
#define ZP_R() { uint8_t z = rd(pc++); poll(); v = rd(z); }
#define ZPX_R(r) { uint16_t e = zpx(r); poll(); v = rd(e); }
#define ABS_R() { ea = fetch16(); poll(); v = rd(ea); }
#define ABX_R(r) { uint16_t b = fetch16(); v = rdIdx(b, r); }
#define IZX_R() { ea = izx(); poll(); v = rd(ea); }
#define IZY_R() { uint16_t b = izyBase(); v = rdIdx(b, y); }
#define IMPL() { poll(); rd(pc); }

      // ---- LDA ----
      case 0xA9: IMM(); a = v; nz(a); break;
      case 0xA5: ZP_R(); a = v; nz(a); break;
      case 0xB5: ZPX_R(x); a = v; nz(a); break;
      case 0xAD: ABS_R(); a = v; nz(a); break;
      case 0xBD: ABX_R(x); a = v; nz(a); break;
      case 0xB9: ABX_R(y); a = v; nz(a); break;
      case 0xA1: IZX_R(); a = v; nz(a); break;
      case 0xB1: IZY_R(); a = v; nz(a); break;
      // ---- LDX ----
      case 0xA2: IMM(); x = v; nz(x); break;
      case 0xA6: ZP_R(); x = v; nz(x); break;
      case 0xB6: ZPX_R(y); x = v; nz(x); break;
      case 0xAE: ABS_R(); x = v; nz(x); break;
      case 0xBE: ABX_R(y); x = v; nz(x); break;
      // ---- LDY ----
      case 0xA0: IMM(); y = v; nz(y); break;
      case 0xA4: ZP_R(); y = v; nz(y); break;
      case 0xB4: ZPX_R(x); y = v; nz(y); break;
      case 0xAC: ABS_R(); y = v; nz(y); break;
      case 0xBC: ABX_R(x); y = v; nz(y); break;
      // ---- LAX (neoficialni) ----
      case 0xA7: ZP_R(); a = x = v; nz(a); break;
      case 0xB7: ZPX_R(y); a = x = v; nz(a); break;
      case 0xAF: ABS_R(); a = x = v; nz(a); break;
      case 0xBF: ABX_R(y); a = x = v; nz(a); break;
      case 0xA3: IZX_R(); a = x = v; nz(a); break;
      case 0xB3: IZY_R(); a = x = v; nz(a); break;
      // ---- ORA AND EOR ADC SBC CMP ----
#define ALU_GROUP(o_imm,o_zp,o_zpx,o_abs,o_abx,o_aby,o_izx,o_izy, OPER) \
      case o_imm: IMM(); OPER; break; \
      case o_zp: ZP_R(); OPER; break; \
      case o_zpx: ZPX_R(x); OPER; break; \
      case o_abs: ABS_R(); OPER; break; \
      case o_abx: ABX_R(x); OPER; break; \
      case o_aby: ABX_R(y); OPER; break; \
      case o_izx: IZX_R(); OPER; break; \
      case o_izy: IZY_R(); OPER; break;
      ALU_GROUP(0x09,0x05,0x15,0x0D,0x1D,0x19,0x01,0x11, (a |= v, nz(a)))
      ALU_GROUP(0x29,0x25,0x35,0x2D,0x3D,0x39,0x21,0x31, (a &= v, nz(a)))
      ALU_GROUP(0x49,0x45,0x55,0x4D,0x5D,0x59,0x41,0x51, (a ^= v, nz(a)))
      ALU_GROUP(0x69,0x65,0x75,0x6D,0x7D,0x79,0x61,0x71, adc(v))
      ALU_GROUP(0xE9,0xE5,0xF5,0xED,0xFD,0xF9,0xE1,0xF1, sbc(v))
      ALU_GROUP(0xC9,0xC5,0xD5,0xCD,0xDD,0xD9,0xC1,0xD1, cmp(a, v))
#undef ALU_GROUP
      case 0xEB: IMM(); sbc(v); break;                 // SBC # (neoficialni)
      case 0xE0: IMM(); cmp(x, v); break;
      case 0xE4: ZP_R(); cmp(x, v); break;
      case 0xEC: ABS_R(); cmp(x, v); break;
      case 0xC0: IMM(); cmp(y, v); break;
      case 0xC4: ZP_R(); cmp(y, v); break;
      case 0xCC: ABS_R(); cmp(y, v); break;
      case 0x24: ZP_R(); bit(v); break;
      case 0x2C: ABS_R(); bit(v); break;

      // ---- zapisy ----
#define ST_ZP(val)  { uint8_t z = rd(pc++); poll(); wr(z, (val)); }
#define ST_ZPX(r,val) { uint16_t e = zpx(r); poll(); wr(e, (val)); }
#define ST_ABS(val) { ea = fetch16(); poll(); wr(ea, (val)); }
#define ST_ABX(r,val) { uint16_t b = fetch16(); ea = eaIdxW(b, r); poll(); wr(ea, (val)); }
#define ST_IZX(val) { ea = izx(); poll(); wr(ea, (val)); }
#define ST_IZY(val) { uint16_t b = izyBase(); ea = eaIdxW(b, y); poll(); wr(ea, (val)); }
      case 0x85: ST_ZP(a); break;
      case 0x95: ST_ZPX(x, a); break;
      case 0x8D: ST_ABS(a); break;
      case 0x9D: ST_ABX(x, a); break;
      case 0x99: ST_ABX(y, a); break;
      case 0x81: ST_IZX(a); break;
      case 0x91: ST_IZY(a); break;
      case 0x86: ST_ZP(x); break;
      case 0x96: ST_ZPX(y, x); break;
      case 0x8E: ST_ABS(x); break;
      case 0x84: ST_ZP(y); break;
      case 0x94: ST_ZPX(x, y); break;
      case 0x8C: ST_ABS(y); break;
      // SAX (neoficialni)
      case 0x87: ST_ZP((uint8_t)(a & x)); break;
      case 0x97: ST_ZPX(y, (uint8_t)(a & x)); break;
      case 0x8F: ST_ABS((uint8_t)(a & x)); break;
      case 0x83: ST_IZX((uint8_t)(a & x)); break;

      // ---- RMW: ASL LSR ROL ROR INC DEC + neoficialni SLO RLA SRE RRA DCP ISC ----
#define RMW_GROUP(o_zp,o_zpx,o_abs,o_abx, FN) \
      case o_zp:  { uint8_t z = rd(pc++); rmw(z, FN); break; } \
      case o_zpx: { uint16_t e = zpx(x); rmw(e, FN); break; } \
      case o_abs: { ea = fetch16(); rmw(ea, FN); break; } \
      case o_abx: { uint16_t b = fetch16(); ea = eaIdxW(b, x); rmw(ea, FN); break; }
#define RMW_GROUP_ILL(o_zp,o_zpx,o_abs,o_abx,o_aby,o_izx,o_izy, FN) \
      RMW_GROUP(o_zp,o_zpx,o_abs,o_abx, FN) \
      case o_aby: { uint16_t b = fetch16(); ea = eaIdxW(b, y); rmw(ea, FN); break; } \
      case o_izx: { ea = izx(); rmw(ea, FN); break; } \
      case o_izy: { uint16_t b = izyBase(); ea = eaIdxW(b, y); rmw(ea, FN); break; }
      RMW_GROUP(0x06,0x16,0x0E,0x1E, [this](uint8_t q){ return asl(q); })
      RMW_GROUP(0x46,0x56,0x4E,0x5E, [this](uint8_t q){ return lsr(q); })
      RMW_GROUP(0x26,0x36,0x2E,0x3E, [this](uint8_t q){ return rol(q); })
      RMW_GROUP(0x66,0x76,0x6E,0x7E, [this](uint8_t q){ return ror(q); })
      RMW_GROUP(0xE6,0xF6,0xEE,0xFE, [this](uint8_t q){ q++; nz(q); return q; })
      RMW_GROUP(0xC6,0xD6,0xCE,0xDE, [this](uint8_t q){ q--; nz(q); return q; })
      RMW_GROUP_ILL(0x07,0x17,0x0F,0x1F,0x1B,0x03,0x13, [this](uint8_t q){ q = asl(q); a |= q; nz(a); return q; })
      RMW_GROUP_ILL(0x27,0x37,0x2F,0x3F,0x3B,0x23,0x33, [this](uint8_t q){ q = rol(q); a &= q; nz(a); return q; })
      RMW_GROUP_ILL(0x47,0x57,0x4F,0x5F,0x5B,0x43,0x53, [this](uint8_t q){ q = lsr(q); a ^= q; nz(a); return q; })
      RMW_GROUP_ILL(0x67,0x77,0x6F,0x7F,0x7B,0x63,0x73, [this](uint8_t q){ q = ror(q); adc(q); return q; })
      RMW_GROUP_ILL(0xC7,0xD7,0xCF,0xDF,0xDB,0xC3,0xD3, [this](uint8_t q){ q--; cmp(a, q); return q; })
      RMW_GROUP_ILL(0xE7,0xF7,0xEF,0xFF,0xFB,0xE3,0xF3, [this](uint8_t q){ q++; sbc(q); return q; })
#undef RMW_GROUP
#undef RMW_GROUP_ILL

      // ---- akumulator / implikovane ----
      case 0x0A: IMPL(); a = asl(a); break;
      case 0x4A: IMPL(); a = lsr(a); break;
      case 0x2A: IMPL(); a = rol(a); break;
      case 0x6A: IMPL(); a = ror(a); break;
      case 0xAA: IMPL(); x = a; nz(x); break;
      case 0x8A: IMPL(); a = x; nz(a); break;
      case 0xA8: IMPL(); y = a; nz(y); break;
      case 0x98: IMPL(); a = y; nz(a); break;
      case 0xBA: IMPL(); x = s; nz(x); break;
      case 0x9A: IMPL(); s = x; break;
      case 0xE8: IMPL(); x++; nz(x); break;
      case 0xCA: IMPL(); x--; nz(x); break;
      case 0xC8: IMPL(); y++; nz(y); break;
      case 0x88: IMPL(); y--; nz(y); break;
      case 0x18: IMPL(); p &= ~F_C; break;
      case 0x38: IMPL(); p |= F_C; break;
      case 0xD8: IMPL(); p &= ~F_D; break;
      case 0xF8: IMPL(); p |= F_D; break;
      case 0xB8: IMPL(); p &= ~F_V; break;
      case 0x58: IMPL(); p &= ~F_I; break;            // CLI - dotaz pred zmenou I
      case 0x78: IMPL(); p |= F_I; break;             // SEI - dotaz pred zmenou I
      case 0xEA: IMPL(); break;
      // 1bajtove NOP (neoficialni)
      case 0x1A: case 0x3A: case 0x5A: case 0x7A: case 0xDA: case 0xFA: IMPL(); break;

      // ---- zasobnik ----
      case 0x48: rd(pc); poll(); push(a); break;                         // PHA
      case 0x08: rd(pc); poll(); push((uint8_t)(p | F_B | F_U)); break;  // PHP
      case 0x68: rd(pc); rd((uint16_t)(0x100 | s)); s++; poll(); a = rd((uint16_t)(0x100 | s)); nz(a); break; // PLA
      case 0x28: rd(pc); rd((uint16_t)(0x100 | s)); s++; poll(); p = (uint8_t)((rd((uint16_t)(0x100 | s)) & ~F_B) | F_U); break; // PLP
      case 0x20: {                                                      // JSR
        uint8_t lo = rd(pc++);
        rd((uint16_t)(0x100 | s));
        push((uint8_t)(pc >> 8)); push((uint8_t)pc);
        poll();
        uint8_t hi = rd(pc);
        pc = (uint16_t)(lo | (hi << 8));
        break;
      }
      case 0x60: {                                                      // RTS
        rd(pc); rd((uint16_t)(0x100 | s)); s++;
        uint8_t lo = rd((uint16_t)(0x100 | s)); s++;
        uint8_t hi = rd((uint16_t)(0x100 | s));
        pc = (uint16_t)(lo | (hi << 8));
        poll();
        rd(pc); pc++;
        break;
      }
      case 0x40: {                                                      // RTI
        rd(pc); rd((uint16_t)(0x100 | s)); s++;
        p = (uint8_t)((rd((uint16_t)(0x100 | s)) & ~F_B) | F_U); s++;
        uint8_t lo = rd((uint16_t)(0x100 | s)); s++;
        poll();                                       // uz s obnovenym I
        uint8_t hi = rd((uint16_t)(0x100 | s));
        pc = (uint16_t)(lo | (hi << 8));
        break;
      }
      case 0x00: interrupt(true); break;                                 // BRK

      // ---- skoky ----
      case 0x4C: { uint8_t lo = rd(pc++); poll(); uint8_t hi = rd(pc); pc = (uint16_t)(lo | (hi << 8)); break; }
      case 0x6C: {
        uint16_t t = fetch16();
        uint8_t lo = rd(t);
        poll();
        uint8_t hi = rd((uint16_t)((t & 0xFF00) | ((t + 1) & 0x00FF)));   // chyba 6502: bez prenosu stranky
        pc = (uint16_t)(lo | (hi << 8));
        break;
      }
      case 0x10: branch(!(p & F_N)); break;
      case 0x30: branch((p & F_N) != 0); break;
      case 0x50: branch(!(p & F_V)); break;
      case 0x70: branch((p & F_V) != 0); break;
      case 0x90: branch(!(p & F_C)); break;
      case 0xB0: branch((p & F_C) != 0); break;
      case 0xD0: branch(!(p & F_Z)); break;
      case 0xF0: branch((p & F_Z) != 0); break;

      // ---- neoficialni s okamzitym operandem ----
      case 0x0B: case 0x2B: IMM(); a &= v; nz(a); setf(F_C, a & 0x80); break;  // ANC
      case 0x4B: IMM(); a &= v; a = lsr(a); break;                            // ALR
      case 0x6B: IMM(); arr(v); break;                                       // ARR
      case 0xCB: IMM(); { unsigned t = (unsigned)(a & x) - v; setf(F_C, (a & x) >= v); x = (uint8_t)t; nz(x); } break; // SBX
      case 0x8B: IMM(); a = (uint8_t)((a | magicAne) & x & v); nz(a); break; // ANE (nestabilni)
      case 0xAB: IMM(); a = x = (uint8_t)((a | magicLxa) & v); nz(a); break; // LXA (nestabilni)
      case 0xBB: { uint16_t b = fetch16(); v = rdIdx(b, y); a = x = s = (uint8_t)(v & s); nz(a); break; } // LAS

      // ---- neoficialni zapisy "& (H+1)" (nestabilni) ----
      case 0x9C: { // SHY abs,X
        uint16_t b = fetch16(); uint16_t e = (uint16_t)(b + x);
        rd((uint16_t)((b & 0xFF00) | (e & 0x00FF)));
        uint8_t val = (uint8_t)(y & ((b >> 8) + 1));
        if ((e ^ b) & 0xFF00) e = (uint16_t)((val << 8) | (e & 0xFF));
        poll(); wr(e, val); break;
      }
      case 0x9E: { // SHX abs,Y
        uint16_t b = fetch16(); uint16_t e = (uint16_t)(b + y);
        rd((uint16_t)((b & 0xFF00) | (e & 0x00FF)));
        uint8_t val = (uint8_t)(x & ((b >> 8) + 1));
        if ((e ^ b) & 0xFF00) e = (uint16_t)((val << 8) | (e & 0xFF));
        poll(); wr(e, val); break;
      }
      case 0x9F: { // SHA abs,Y
        uint16_t b = fetch16(); uint16_t e = (uint16_t)(b + y);
        rd((uint16_t)((b & 0xFF00) | (e & 0x00FF)));
        uint8_t val = (uint8_t)(a & x & ((b >> 8) + 1));
        if ((e ^ b) & 0xFF00) e = (uint16_t)((val << 8) | (e & 0xFF));
        poll(); wr(e, val); break;
      }
      case 0x93: { // SHA (zp),Y
        uint16_t b = izyBase(); uint16_t e = (uint16_t)(b + y);
        rd((uint16_t)((b & 0xFF00) | (e & 0x00FF)));
        uint8_t val = (uint8_t)(a & x & ((b >> 8) + 1));
        if ((e ^ b) & 0xFF00) e = (uint16_t)((val << 8) | (e & 0xFF));
        poll(); wr(e, val); break;
      }
      case 0x9B: { // TAS abs,Y
        uint16_t b = fetch16(); uint16_t e = (uint16_t)(b + y);
        rd((uint16_t)((b & 0xFF00) | (e & 0x00FF)));
        s = (uint8_t)(a & x);
        uint8_t val = (uint8_t)(s & ((b >> 8) + 1));
        if ((e ^ b) & 0xFF00) e = (uint16_t)((val << 8) | (e & 0xFF));
        poll(); wr(e, val); break;
      }

      // ---- NOP s operandem (neoficialni) ----
      case 0x80: case 0x82: case 0x89: case 0xC2: case 0xE2: IMM(); break;
      case 0x04: case 0x44: case 0x64: ZP_R(); break;
      case 0x14: case 0x34: case 0x54: case 0x74: case 0xD4: case 0xF4: ZPX_R(x); break;
      case 0x0C: ABS_R(); break;
      case 0x1C: case 0x3C: case 0x5C: case 0x7C: case 0xDC: case 0xFC: ABX_R(x); break;

      // ---- JAM / KIL ----
      case 0x02: case 0x12: case 0x22: case 0x32: case 0x42: case 0x52:
      case 0x62: case 0x72: case 0x92: case 0xB2: case 0xD2: case 0xF2:
        rd(pc); jam = true; jamOp = op; jamPc = (uint16_t)(pc - 1);
        rd(0xFFFF); rd(0xFFFE); rd(0xFFFE);
        break;
#undef IMM
#undef ZP_R
#undef ZPX_R
#undef ABS_R
#undef ABX_R
#undef IZX_R
#undef IZY_R
#undef IMPL
#undef ST_ZP
#undef ST_ZPX
#undef ST_ABS
#undef ST_ABX
#undef ST_IZX
#undef ST_IZY
    }
  }

  void interruptNmi() {
    // NMI uz prevzate - stejny prubeh jako IRQ, vektor $FFFA
    rd(pc); rd(pc);
    push((uint8_t)(pc >> 8));
    push((uint8_t)pc);
    push((uint8_t)(p | F_U));
    p |= F_I;
    uint8_t lo = rd(0xFFFA);
    uint8_t hi = rd(0xFFFB);
    pc = (uint16_t)(lo | (hi << 8));
    takeNmi = false; takeIrq = false;
  }
};

} // namespace nap
