// nap_atari_cpu.cpp
// 6502 pro EMU-10. STROJOVE PRELOZENO z Reneho JavaScriptu (machine130xe.js,
// funkce CPU6502). Zadna vlastni interpretace - kazda vetev odpovida jeho
// vetvi. Shoda se overuje diferencnim testem proti originalu.
#include <cstdint>
#include <cstring>
#include "nap_atari_cpu.h"

namespace nap {

void Cpu6502::reset() {
  c.sp = 0xFD; c.if_ = 1; c.df = 0;
  c.nmiPending = false; c.jam = false;
  c.pc = rd(0xFFFC) | (rd(0xFFFD) << 8);
}

inline void Cpu6502::nz(int v) { c.zf = (v == 0) ? 1 : 0; c.nf = (v >> 7) & 1; }
inline void Cpu6502::push(int v) { wr(0x100 | c.sp, v & 0xFF); c.sp = (c.sp - 1) & 0xFF; }
inline int  Cpu6502::pop() { c.sp = (c.sp + 1) & 0xFF; return rd(0x100 | c.sp); }
inline int  Cpu6502::packP(int b) {
  return (c.nf << 7) | (c.vf << 6) | 0x20 | (b ? 0x10 : 0) | (c.df << 3) | (c.if_ << 2) | (c.zf << 1) | c.cf;
}
inline void Cpu6502::unpackP(int p) {
  c.nf = (p >> 7) & 1; c.vf = (p >> 6) & 1; c.df = (p >> 3) & 1;
  c.if_ = (p >> 2) & 1; c.zf = (p >> 1) & 1; c.cf = p & 1;
}
inline void Cpu6502::wrt(int a, int v, int cycEnd) {
  c.ioWriteCycle = (int)(c.cycles + cycEnd); wr(a, v); c.ioWriteCycle = -1;
}

void Cpu6502::adc(int v) {
  if (!c.df) {
    int t = c.a + v + c.cf;
    c.vf = (~(c.a ^ v) & (c.a ^ t) & 0x80) ? 1 : 0;
    c.cf = t > 0xFF ? 1 : 0; c.a = t & 0xFF; nz(c.a);
  } else {                                   // NMOS decimalni ADC
    int lo = (c.a & 0x0F) + (v & 0x0F) + c.cf;
    int hi = (c.a >> 4) + (v >> 4);
    c.zf = ((c.a + v + c.cf) & 0xFF) == 0 ? 1 : 0;
    if (lo > 9) { lo += 6; hi++; }
    c.nf = (hi & 0x08) ? 1 : 0;
    c.vf = (~(c.a ^ v) & (c.a ^ (hi << 4)) & 0x80) ? 1 : 0;
    if (hi > 9) hi += 6;
    c.cf = hi > 15 ? 1 : 0;
    c.a = ((hi << 4) | (lo & 0x0F)) & 0xFF;
  }
}

void Cpu6502::sbc(int v) {
  if (!c.df) { adc(v ^ 0xFF); }
  else {                                     // NMOS decimalni SBC
    int bt = c.a - v - (1 - c.cf);
    int lo = (c.a & 0x0F) - (v & 0x0F) - (1 - c.cf);
    int hi = (c.a >> 4) - (v >> 4);
    if (lo & 0x10) { lo -= 6; hi--; }
    if (hi & 0x10) hi -= 6;
    c.vf = ((c.a ^ v) & (c.a ^ bt) & 0x80) ? 1 : 0;
    c.cf = bt >= 0 ? 1 : 0; nz(bt & 0xFF);
    c.a = ((hi << 4) | (lo & 0x0F)) & 0xFF;
  }
}

bool Cpu6502::service() {
  if (c.nmiPending) {
    c.nmiPending = false;
    push(c.pc >> 8); push(c.pc & 0xFF); push(packP(0)); c.if_ = 1;
    c.pc = rd(0xFFFA) | (rd(0xFFFB) << 8); c.cycles += 7; return true;
  }
  if (c.irqLine && !c.if_) {
    push(c.pc >> 8); push(c.pc & 0xFF); push(packP(0)); c.if_ = 1;
    c.pc = rd(0xFFFE) | (rd(0xFFFF) << 8); c.cycles += 7; return true;
  }
  return false;
}

inline int Cpu6502::br(bool cond, int off) {
  int from = c.pc;
  int to = (from + ((off & 0x80) ? off - 256 : off)) & 0xFFFF;
  if (cond) { c.pc = to; return 3 + (((from ^ to) & 0xFF00) ? 1 : 0); }
  return 2;
}
inline int Cpu6502::pg(int base, int addr) { return ((base ^ addr) & 0xFF00) ? 1 : 0; }
inline int Cpu6502::rdIZYBase(int zp) { return rd(zp) | (rd((zp + 1) & 0xFF) << 8); }
inline int Cpu6502::rdAbsBase() { return rd(c.pc) | (rd((c.pc + 1) & 0xFFFF) << 8); }

void Cpu6502::step() {
  if (service()) return;
  if (c.jam) { c.cycles += 2; return; }
  int op = rd(c.pc); c.pc = (c.pc + 1) & 0xFFFF;
  int ea = 0, t = 0, v = 0, cyc = 2;
  switch (op) {
      case 0x00:  t=(c.pc+1)&0xFFFF; push(t>>8); push(t&0xFF); push(packP(1)); c.if_=1; c.pc=rd(0xFFFE)|(rd(0xFFFF)<<8); cyc=7; break; // BRK imp
      case 0x01: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); c.a|=rd(ea); nz(c.a); cyc=6; break; // ORA izx
      case 0x05: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=3; break; // ORA zp
      case 0x06: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; nz(v); wr(ea,v); cyc=5; break; // ASL zp
      case 0x08:  push(packP(1)); cyc=3; break; // PHP imp
      case 0x09: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=2; break; // ORA imm
      case 0x0A:  v=c.a; c.cf=(v>>7)&1; v=(v<<1)&0xFF; nz(v); c.a=v; cyc=2; break; // ASL acc
      case 0x0D: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=4; break; // ORA abs
      case 0x0E: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; nz(v); wr(ea,v); cyc=6; break; // ASL abs
      case 0x10: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!c.nf,t); break; // BPL rel BUILD2IU exact branch cycles
      case 0x11: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=5+pg(v,ea); break; // ORA izy BUILD2JI page-cross
      case 0x15: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=4; break; // ORA zpx
      case 0x16: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; nz(v); wr(ea,v); cyc=6; break; // ASL zpx
      case 0x18:  c.cf=0; cyc=2; break; // CLC imp
      case 0x19: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // ORA aby BUILD2JI page-cross
      case 0x1D: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a|=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // ORA abx BUILD2JI page-cross
      case 0x1E: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; nz(v); wr(ea,v); cyc=7; break; // ASL abx
      case 0x20: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=(c.pc-1)&0xFFFF; push(t>>8); push(t&0xFF); c.pc=ea; cyc=6; break; // JSR abs
      case 0x21: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); c.a&=rd(ea); nz(c.a); cyc=6; break; // AND izx
      case 0x24: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.zf=(c.a&t)==0?1:0; c.nf=(t>>7)&1; c.vf=(t>>6)&1; cyc=3; break; // BIT zp
      case 0x25: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=3; break; // AND zp
      case 0x26: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; nz(v); wr(ea,v); cyc=5; break; // ROL zp
      case 0x28:  unpackP(pop()); cyc=4; break; // PLP imp
      case 0x29: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=2; break; // AND imm
      case 0x2A:  v=c.a; t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; nz(v); c.a=v; cyc=2; break; // ROL acc
      case 0x2C: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=rd(ea); c.zf=(c.a&t)==0?1:0; c.nf=(t>>7)&1; c.vf=(t>>6)&1; cyc=4; break; // BIT abs
      case 0x2D: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=4; break; // AND abs
      case 0x2E: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; nz(v); wr(ea,v); cyc=6; break; // ROL abs
      case 0x30: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!!c.nf,t); break; // BMI rel BUILD2IU exact branch cycles
      case 0x31: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=5+pg(v,ea); break; // AND izy BUILD2JI page-cross
      case 0x35: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=4; break; // AND zpx
      case 0x36: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; nz(v); wr(ea,v); cyc=6; break; // ROL zpx
      case 0x38:  c.cf=1; cyc=2; break; // SEC imp
      case 0x39: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // AND aby BUILD2JI page-cross
      case 0x3D: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a&=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // AND abx BUILD2JI page-cross
      case 0x3E: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; nz(v); wr(ea,v); cyc=7; break; // ROL abx
      case 0x40:  unpackP(pop()); t=pop(); c.pc=t|(pop()<<8); cyc=6; break; // RTI imp
      case 0x41: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); c.a^=rd(ea); nz(c.a); cyc=6; break; // EOR izx
      case 0x45: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=3; break; // EOR zp
      case 0x46: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; nz(v); wr(ea,v); cyc=5; break; // LSR zp
      case 0x48:  push(c.a); cyc=3; break; // PHA imp
      case 0x49: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=2; break; // EOR imm
      case 0x4A:  v=c.a; c.cf=v&1; v=v>>1; nz(v); c.a=v; cyc=2; break; // LSR acc
      case 0x4C: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.pc=ea; cyc=3; break; // JMP abs
      case 0x4D: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=4; break; // EOR abs
      case 0x4E: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; nz(v); wr(ea,v); cyc=6; break; // LSR abs
      case 0x50: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!c.vf,t); break; // BVC rel BUILD2IU exact branch cycles
      case 0x51: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=5+pg(v,ea); break; // EOR izy BUILD2JI page-cross
      case 0x55: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=4; break; // EOR zpx
      case 0x56: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; nz(v); wr(ea,v); cyc=6; break; // LSR zpx
      case 0x58:  c.if_=0; cyc=2; break; // CLI imp
      case 0x59: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // EOR aby BUILD2JI page-cross
      case 0x5D: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a^=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // EOR abx BUILD2JI page-cross
      case 0x5E: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; nz(v); wr(ea,v); cyc=7; break; // LSR abx
      case 0x60:  t=pop(); c.pc=((t|(pop()<<8))+1)&0xFFFF; cyc=6; break; // RTS imp
      case 0x61: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); adc(rd(ea)); cyc=6; break; // ADC izx
      case 0x65: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; adc(rd(ea)); cyc=3; break; // ADC zp
      case 0x66: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); nz(v); wr(ea,v); cyc=5; break; // ROR zp
      case 0x68:  c.a=pop(); nz(c.a); cyc=4; break; // PLA imp
      case 0x69: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; adc(rd(ea)); cyc=2; break; // ADC imm
      case 0x6A:  v=c.a; t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); nz(v); c.a=v; cyc=2; break; // ROR acc
      case 0x6C: t=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; ea=rd(t)|(rd((t&0xFF00)|((t+1)&0xFF))<<8); c.pc=ea; cyc=5; break; // JMP ind
      case 0x6D: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; adc(rd(ea)); cyc=4; break; // ADC abs
      case 0x6E: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); nz(v); wr(ea,v); cyc=6; break; // ROR abs
      case 0x70: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!!c.vf,t); break; // BVS rel BUILD2IU exact branch cycles
      case 0x71: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; adc(rd(ea)); cyc=5+pg(v,ea); break; // ADC izy BUILD2JI page-cross
      case 0x75: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; adc(rd(ea)); cyc=4; break; // ADC zpx
      case 0x76: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); nz(v); wr(ea,v); cyc=6; break; // ROR zpx
      case 0x78:  c.if_=1; cyc=2; break; // SEI imp
      case 0x79: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; adc(rd(ea)); cyc=4+pg(t,ea); break; // ADC aby BUILD2JI page-cross
      case 0x7D: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; adc(rd(ea)); cyc=4+pg(t,ea); break; // ADC abx BUILD2JI page-cross
      case 0x7E: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); nz(v); wr(ea,v); cyc=7; break; // ROR abx
      case 0x81: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); wrt(ea,c.a,6); cyc=6; break; // STA izx BUILD2KI io write timing
      case 0x84: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; wrt(ea,c.y,3); cyc=3; break; // STY zp BUILD2KI io write timing
      case 0x85: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; wrt(ea,c.a,3); cyc=3; break; // STA zp BUILD2KI io write timing
      case 0x86: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; wrt(ea,c.x,3); cyc=3; break; // STX zp BUILD2KI io write timing
      case 0x88:  c.y=(c.y-1)&0xFF; nz(c.y); cyc=2; break; // DEY imp
      case 0x8A:  c.a=c.x; nz(c.a); cyc=2; break; // TXA imp
      case 0x8C: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; wrt(ea,c.y,4); cyc=4; break; // STY abs BUILD2KI io write timing
      case 0x8D: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; wrt(ea,c.a,4); cyc=4; break; // STA abs BUILD2KI io write timing
      case 0x8E: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; wrt(ea,c.x,4); cyc=4; break; // STX abs BUILD2KI io write timing
      case 0x90: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!c.cf,t); break; // BCC rel BUILD2IU exact branch cycles
      case 0x91: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; wrt(ea,c.a,6); cyc=6; break; // STA izy BUILD2KI io write timing
      case 0x94: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; wr(ea,c.y); cyc=4; break; // STY zpx
      case 0x95: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; wr(ea,c.a); cyc=4; break; // STA zpx
      case 0x96: ea=(rd(c.pc)+c.y)&0xFF; c.pc=(c.pc+1)&0xFFFF; wr(ea,c.x); cyc=4; break; // STX zpy
      case 0x98:  c.a=c.y; nz(c.a); cyc=2; break; // TYA imp
      case 0x99: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; wrt(ea,c.a,5); cyc=5; break; // STA aby BUILD2KI io write timing
      case 0x9A:  c.sp=c.x; cyc=2; break; // TXS imp
      case 0x9D: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; wrt(ea,c.a,5); cyc=5; break; // STA abx BUILD2KI io write timing
      case 0xA0: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.y=rd(ea); nz(c.y); cyc=2; break; // LDY imm
      case 0xA1: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); c.a=rd(ea); nz(c.a); cyc=6; break; // LDA izx
      case 0xA2: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.x=rd(ea); nz(c.x); cyc=2; break; // LDX imm
      case 0xA4: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.y=rd(ea); nz(c.y); cyc=3; break; // LDY zp
      case 0xA5: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=3; break; // LDA zp
      case 0xA6: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.x=rd(ea); nz(c.x); cyc=3; break; // LDX zp
      case 0xA8:  c.y=c.a; nz(c.y); cyc=2; break; // TAY imp
      case 0xA9: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=2; break; // LDA imm
      case 0xAA:  c.x=c.a; nz(c.x); cyc=2; break; // TAX imp
      case 0xAC: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.y=rd(ea); nz(c.y); cyc=4; break; // LDY abs
      case 0xAD: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=4; break; // LDA abs
      case 0xAE: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.x=rd(ea); nz(c.x); cyc=4; break; // LDX abs
      case 0xB0: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!!c.cf,t); break; // BCS rel BUILD2IU exact branch cycles
      case 0xB1: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=5+pg(v,ea); break; // LDA izy BUILD2JI page-cross
      case 0xB4: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.y=rd(ea); nz(c.y); cyc=4; break; // LDY zpx
      case 0xB5: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=4; break; // LDA zpx
      case 0xB6: ea=(rd(c.pc)+c.y)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.x=rd(ea); nz(c.x); cyc=4; break; // LDX zpy
      case 0xB8:  c.vf=0; cyc=2; break; // CLV imp
      case 0xB9: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // LDA aby BUILD2JI page-cross
      case 0xBA:  c.x=c.sp; nz(c.x); cyc=2; break; // TSX imp
      case 0xBC: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.y=rd(ea); nz(c.y); cyc=4+pg(t,ea); break; // LDY abx BUILD2JI page-cross
      case 0xBD: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a=rd(ea); nz(c.a); cyc=4+pg(t,ea); break; // LDA abx BUILD2JI page-cross
      case 0xBE: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.x=rd(ea); nz(c.x); cyc=4+pg(t,ea); break; // LDX aby BUILD2JI page-cross
      case 0xC0: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.y>=t?1:0; nz((c.y-t)&0xFF); cyc=2; break; // CPY imm
      case 0xC1: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF); cyc=6; break; // CMP izx
      case 0xC4: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.y>=t?1:0; nz((c.y-t)&0xFF); cyc=3; break; // CPY zp
      case 0xC5: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF); cyc=3; break; // CMP zp
      case 0xC6: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; t=(rd(ea)-1)&0xFF; wr(ea,t); nz(t); cyc=5; break; // DEC zp
      case 0xC8:  c.y=(c.y+1)&0xFF; nz(c.y); cyc=2; break; // INY imp
      case 0xC9: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF); cyc=2; break; // CMP imm
      case 0xCA:  c.x=(c.x-1)&0xFF; nz(c.x); cyc=2; break; // DEX imp
      case 0xCC: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=rd(ea); c.cf=c.y>=t?1:0; nz((c.y-t)&0xFF); cyc=4; break; // CPY abs
      case 0xCD: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF); cyc=4; break; // CMP abs
      case 0xCE: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=(rd(ea)-1)&0xFF; wr(ea,t); nz(t); cyc=6; break; // DEC abs
      case 0xD0: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!c.zf,t); break; // BNE rel BUILD2IU exact branch cycles
      case 0xD1: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF); cyc=5+pg(v,ea); break; // CMP izy BUILD2JI page-cross
      case 0xD5: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.a>=t?1:0; nz((c.a-t)&0xFF); cyc=4; break; // CMP zpx
      case 0xD6: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; t=(rd(ea)-1)&0xFF; wr(ea,t); nz(t); cyc=6; break; // DEC zpx
      case 0xD8:  c.df=0; cyc=2; break; // CLD imp
      case 0xD9: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=c.a>=v?1:0; nz((c.a-v)&0xFF); cyc=4+pg(t,ea); break; // CMP aby BUILD2JI page-cross
      case 0xDD: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=c.a>=v?1:0; nz((c.a-v)&0xFF); cyc=4+pg(t,ea); break; // CMP abx BUILD2JI page-cross
      case 0xDE: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; t=(rd(ea)-1)&0xFF; wr(ea,t); nz(t); cyc=7; break; // DEC abx
      case 0xE0: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.x>=t?1:0; nz((c.x-t)&0xFF); cyc=2; break; // CPX imm
      case 0xE1: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); sbc(rd(ea)); cyc=6; break; // SBC izx
      case 0xE4: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; t=rd(ea); c.cf=c.x>=t?1:0; nz((c.x-t)&0xFF); cyc=3; break; // CPX zp
      case 0xE5: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; sbc(rd(ea)); cyc=3; break; // SBC zp
      case 0xE6: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; t=(rd(ea)+1)&0xFF; wr(ea,t); nz(t); cyc=5; break; // INC zp
      case 0xE8:  c.x=(c.x+1)&0xFF; nz(c.x); cyc=2; break; // INX imp
      case 0xE9: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; sbc(rd(ea)); cyc=2; break; // SBC imm
      case 0xEA:   cyc=2; break; // NOP imp
      case 0xEC: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=rd(ea); c.cf=c.x>=t?1:0; nz((c.x-t)&0xFF); cyc=4; break; // CPX abs
      case 0xED: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; sbc(rd(ea)); cyc=4; break; // SBC abs
      case 0xEE: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; t=(rd(ea)+1)&0xFF; wr(ea,t); nz(t); cyc=6; break; // INC abs
      case 0xF0: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; cyc=br(!!c.zf,t); break; // BEQ rel BUILD2IU exact branch cycles
      case 0xF1: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; sbc(rd(ea)); cyc=5+pg(v,ea); break; // SBC izy BUILD2JI page-cross
      case 0xF5: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; sbc(rd(ea)); cyc=4; break; // SBC zpx
      case 0xF6: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; t=(rd(ea)+1)&0xFF; wr(ea,t); nz(t); cyc=6; break; // INC zpx
      case 0xF8:  c.df=1; cyc=2; break; // SED imp
      case 0xF9: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; sbc(rd(ea)); cyc=4+pg(t,ea); break; // SBC aby BUILD2JI page-cross
      case 0xFD: t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; sbc(rd(ea)); cyc=4+pg(t,ea); break; // SBC abx BUILD2JI page-cross
      case 0xFE: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; t=(rd(ea)+1)&0xFF; wr(ea,t); nz(t); cyc=7; break; // INC abx
      case 0xA7: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; c.a=rd(ea); c.x=c.a; nz(c.a); cyc=3; break; // LAX zp
      case 0xB7: ea=(rd(c.pc)+c.y)&0xFF; c.pc=(c.pc+1)&0xFFFF; c.a=rd(ea); c.x=c.a; nz(c.a); cyc=4; break; // LAX zpy
      case 0xAF: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; c.a=rd(ea); c.x=c.a; nz(c.a); cyc=4; break; // LAX abs
      case 0xBF: t=rdAbsBase(); ea=(t+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; c.a=rd(ea); c.x=c.a; nz(c.a); cyc=4+pg(t,ea); break; // LAX aby BUILD2JI page-cross
      case 0xA3: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); c.a=rd(ea); c.x=c.a; nz(c.a); cyc=6; break; // LAX izx
      case 0xB3: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rdIZYBase(t); ea=(v+c.y)&0xFFFF; c.a=rd(ea); c.x=c.a; nz(c.a); cyc=5+pg(v,ea); break; // LAX izy BUILD2JI page-cross
      case 0x87: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; wr(ea, c.a&c.x); cyc=3; break; // SAX zp
      case 0x97: ea=(rd(c.pc)+c.y)&0xFF; c.pc=(c.pc+1)&0xFFFF; wr(ea, c.a&c.x); cyc=4; break; // SAX zpy
      case 0x8F: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; wr(ea, c.a&c.x); cyc=4; break; // SAX abs
      case 0x83: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); wr(ea, c.a&c.x); cyc=6; break; // SAX izx
      case 0xC7: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=5; break; // DCP zp
      case 0xD7: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=6; break; // DCP zpx
      case 0xCF: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=6; break; // DCP abs
      case 0xDF: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=7; break; // DCP abx
      case 0xDB: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=7; break; // DCP aby
      case 0xC3: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=8; break; // DCP izx
      case 0xD3: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; v=rd(ea); v=(v-1)&0xFF; t=(c.a-v)&0xFF; c.cf=c.a>=v?1:0; nz(t); wr(ea,v); cyc=8; break; // DCP izy
      case 0xE7: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=5; break; // ISB zp
      case 0xF7: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=6; break; // ISB zpx
      case 0xEF: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=6; break; // ISB abs
      case 0xFF: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=7; break; // ISB abx
      case 0xFB: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=7; break; // ISB aby
      case 0xE3: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=8; break; // ISB izx
      case 0xF3: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; v=rd(ea); v=(v+1)&0xFF; sbc(v); wr(ea,v); cyc=8; break; // ISB izy
      case 0x07: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=5; break; // SLO zp
      case 0x17: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=6; break; // SLO zpx
      case 0x0F: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=6; break; // SLO abs
      case 0x1F: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=7; break; // SLO abx
      case 0x1B: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=7; break; // SLO aby
      case 0x03: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=8; break; // SLO izx
      case 0x13: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; v=rd(ea); c.cf=(v>>7)&1; v=(v<<1)&0xFF; c.a|=v; nz(c.a); wr(ea,v); cyc=8; break; // SLO izy
      case 0x27: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=5; break; // RLA zp
      case 0x37: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=6; break; // RLA zpx
      case 0x2F: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=6; break; // RLA abs
      case 0x3F: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=7; break; // RLA abx
      case 0x3B: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=7; break; // RLA aby
      case 0x23: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=8; break; // RLA izx
      case 0x33: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; v=rd(ea); t=c.cf; c.cf=(v>>7)&1; v=((v<<1)|t)&0xFF; c.a&=v; nz(c.a); wr(ea,v); cyc=8; break; // RLA izy
      case 0x47: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=5; break; // SRE zp
      case 0x57: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=6; break; // SRE zpx
      case 0x4F: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=6; break; // SRE abs
      case 0x5F: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=7; break; // SRE abx
      case 0x5B: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=7; break; // SRE aby
      case 0x43: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=8; break; // SRE izx
      case 0x53: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; v=rd(ea); c.cf=v&1; v=v>>1; c.a^=v; nz(c.a); wr(ea,v); cyc=8; break; // SRE izy
      case 0x67: ea=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=5; break; // RRA zp
      case 0x77: ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=6; break; // RRA zpx
      case 0x6F: ea=rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8); c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=6; break; // RRA abs
      case 0x7F: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=7; break; // RRA abx
      case 0x7B: ea=((rd(c.pc)|(rd((c.pc+1)&0xFFFF)<<8))+c.y)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=7; break; // RRA aby
      case 0x63: t=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; ea=rd(t)|(rd((t+1)&0xFF)<<8); v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=8; break; // RRA izx
      case 0x73: t=rd(c.pc); c.pc=(c.pc+1)&0xFFFF; ea=((rd(t)|(rd((t+1)&0xFF)<<8))+c.y)&0xFFFF; v=rd(ea); t=c.cf; c.cf=v&1; v=(v>>1)|(t<<7); adc(v); wr(ea,v); cyc=8; break; // RRA izy
      case 0x0B: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); nz(c.a); c.cf=c.nf; cyc=2; break; // ANC imm
      case 0x2B: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); nz(c.a); c.cf=c.nf; cyc=2; break; // ANC imm
      case 0x4B: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); c.cf=c.a&1; c.a>>=1; nz(c.a); cyc=2; break; // ALR imm
      case 0x6B: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a&=rd(ea); c.a=(c.a>>1)|(c.cf<<7); nz(c.a); c.cf=(c.a>>6)&1; c.vf=((c.a>>6)^(c.a>>5))&1; cyc=2; break; // ARR imm
      case 0xCB: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; t=rd(ea); v=(c.a&c.x)-t; c.cf=v>=0?1:0; c.x=v&0xFF; nz(c.x); cyc=2; break; // SBX imm
      case 0xEB: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; sbc(rd(ea)); cyc=2; break; // SBC imm
      case 0xAB: ea=c.pc; c.pc=(c.pc+1)&0xFFFF; c.a=(c.a|0xEE)&rd(ea); c.x=c.a; nz(c.a); cyc=2; break; // LXA imm
      default:
        // undocumented: common NOPs consume operands; JAM opcodes halt
        if(op==0x02||op==0x12||op==0x22||op==0x32||op==0x42||op==0x52||
           op==0x62||op==0x72||op==0x92||op==0xB2||op==0xD2||op==0xF2){ c.jam=true; c.jamOp=op; c.jamPc=(c.pc-1)&0xFFFF; }
        else if(op==0x80||op==0x82||op==0x89||op==0xC2||op==0xE2){ c.pc=(c.pc+1)&0xFFFF; cyc=2; } // BUILD2JL: NOP immediate = 2 cykly
        else if(op==0x04||op==0x44||op==0x64){ c.pc=(c.pc+1)&0xFFFF; cyc=3; } // BUILD2JL: NOP zp = 3 cykly
        else if(op==0x14||op==0x34||op==0x54||op==0x74||op==0xD4||op==0xF4){ ea=(rd(c.pc)+c.x)&0xFF; c.pc=(c.pc+1)&0xFFFF; rd(ea); cyc=4; } // BUILD2JL: NOP zp,x = 4 cykly + dummy read
        else if(op==0x0C){ c.pc=(c.pc+2)&0xFFFF; cyc=4; } // BUILD2JL: NOP abs = 4 cykly
        else if(op==0x1C||op==0x3C||op==0x5C||op==0x7C||op==0xDC||op==0xFC){
          // BUILD2JL: neoficialni NOP abs,x cte operand na (abs+X) a pri page-cross ma +1 cyklus.
          t=rdAbsBase(); ea=(t+c.x)&0xFFFF; c.pc=(c.pc+2)&0xFFFF; rd(ea); cyc=4+pg(t,ea);
        }
        else cyc=2; // 1-byte undocumented NOPs
        break;
  }
  c.cycles += cyc;
}

} // namespace nap
