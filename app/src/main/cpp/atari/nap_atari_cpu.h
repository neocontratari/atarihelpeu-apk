// nap_atari_cpu.h - 6502 pro EMU-10, rozhrani shodne s Reneho CPU6502(read,write)
#pragma once
#include <cstdint>
#include <functional>

namespace nap {

struct CpuState {
  int a=0,x=0,y=0,sp=0xFF,pc=0;
  int nf=0,vf=0,df=0,if_=1,zf=0,cf=0;
  bool nmiPending=false; int irqLine=0; bool jam=false;
  long long cycles=0; int ioWriteCycle=-1;
  int jamOp=0, jamPc=0;
};

class Cpu6502 {
public:
  using RdFn = std::function<int(int)>;
  using WrFn = std::function<void(int,int)>;
  Cpu6502(RdFn r, WrFn w) : rdf(r), wrf(w) {}
  void reset();
  void step();
  CpuState c;
private:
  RdFn rdf; WrFn wrf;
  inline int  rd(int a){ return rdf(a & 0xFFFF) & 0xFF; }
  inline void wr(int a,int v){ wrf(a & 0xFFFF, v & 0xFF); }
  inline void nz(int v);
  inline void push(int v);
  inline int  pop();
  inline int  packP(int b);
  inline void unpackP(int p);
  inline void wrt(int a,int v,int cycEnd);
  void adc(int v);
  void sbc(int v);
  bool service();
  inline int br(bool cond,int off);
  inline int pg(int base,int addr);
  inline int rdIZYBase(int zp);
  inline int rdAbsBase();
};

} // namespace nap
