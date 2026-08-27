// nap_atari_mem.h - pametova vrstva EMU-10 (RAM, rozsirene banky, ROM, PIA)
// Prelozeno z Reneho machine130xe.js: portB, xe130Bank, rambo320Bank,
// cpuExtAdr, anticExtAdr, cpuRead, cpuWrite, anticRead.
// Hardwarove okno $D000-$D7FF a VBXE se sem zamerne netahaji - obsluhuji
// je vrstvy 3 a 5. Sem prijdou jako odbocka ven.
#pragma once
#include <cstdint>
#include <cstring>
#include <functional>

namespace nap {

struct Pia { int orA=0xFF, ddrA=0, orB=0xFF, ddrB=0, ctlA=0, ctlB=0; };

class AtariMem {
public:
  static const int EXT_SIZE = 320*1024;      // 20 banku po 16K

  uint8_t ram[65536];
  uint8_t ext[EXT_SIZE];
  const uint8_t* os  = nullptr;              // 16K, $C000-$FFFF
  const uint8_t* bas = nullptr;              // 8K,  $A000-$BFFF
  Pia pia;

  // odbocky ven - hardware a VBXE resi jine vrstvy
  std::function<int(int)>        hwRead   = nullptr;   // $D000-$D7FF
  std::function<void(int,int)>   hwWrite  = nullptr;
  std::function<int(int)>        vbxeCpuMap   = nullptr;  // <0 = nemapovano
  std::function<int(int)>        vbxeAnticMap = nullptr;
  std::function<int(int)>        vramRead  = nullptr;
  std::function<void(int,int)>   vramWrite = nullptr;

  AtariMem(){ std::memset(ram,0,sizeof(ram)); std::memset(ext,0,sizeof(ext)); }

  inline int portB() const { return (pia.orB | (~pia.ddrB)) & 0xFF; }

  static inline int xe130Bank(int pb){ return ((pb>>2)&3)&3; }
  static inline int rambo320Bank(int pb){
    return (((pb>>2)&3) | ((pb>>3)&4) | ((pb>>3)&8)) & 15;
  }
  static inline int cpuExtAdr(int pb,int a){
    int bank = ((pb&0x40)==0) ? rambo320Bank(pb) : xe130Bank(pb);
    return ((bank<<14) | (a-0x4000)) & (EXT_SIZE-1);
  }
  static inline int anticExtAdr(int pb,int a){
    // ANTIC drzime 130XE-kompatibilni na prvnich 4 bankach (jako v JS)
    return ((xe130Bank(pb)<<14) | (a-0x4000)) & (EXT_SIZE-1);
  }

  int cpuRead(int a){
    a &= 0xFFFF;
    if(a>=0xD000 && a<0xD800) return hwRead ? hwRead(a) : 0xFF;
    if(vbxeCpuMap){ int va=vbxeCpuMap(a); if(va>=0) return vramRead? vramRead(va):0xFF; }
    const int pb = portB();
    if(a>=0xC000){ if(pb&1) return os[a-0xC000]; return ram[a]; }
    if(a>=0xA000){ if(!(pb&2)) return bas[a-0xA000]; return ram[a]; }
    if(a>=0x5000 && a<0x5800 && !(pb&0x80) && (pb&1)) return os[a-0x5000+0x1000]; // self test
    if(a>=0x4000 && a<0x8000 && !(pb&0x10)) return ext[cpuExtAdr(pb,a)];
    return ram[a];
  }

  void cpuWrite(int a,int v){
    a &= 0xFFFF; v &= 0xFF;
    if(a>=0xD000 && a<0xD800){ if(hwWrite) hwWrite(a,v); return; }
    if(vbxeCpuMap){ int va=vbxeCpuMap(a); if(va>=0){ if(vramWrite) vramWrite(va,v); return; } }
    const int pb = portB();
    if(a>=0x4000 && a<0x8000 && !(pb&0x10)){ ext[cpuExtAdr(pb,a)]=(uint8_t)v; return; }
    ram[a]=(uint8_t)v;                       // RAM pod ROM je vzdy zapisovatelna
  }

  int anticRead(int a){
    a &= 0xFFFF;
    if(vbxeAnticMap){ int va=vbxeAnticMap(a); if(va>=0) return vramRead? vramRead(va):0xFF; }
    const int pb = portB();
    if(a>=0xC000){ if(pb&1) return os[a-0xC000]; return ram[a]; }
    if(a>=0xA000 && !(pb&2)) return bas[a-0xA000];
    if(a>=0x5000 && a<0x5800 && !(pb&0x80) && (pb&1)) return os[a-0x5000+0x1000];
    if(a>=0x4000 && a<0x8000 && !(pb&0x20)) return ext[anticExtAdr(pb,a)];
    return ram[a];
  }

  // PIA zapis, presne jako v hwWrite pro stranku $D300
  void piaWrite(int a,int v){
    switch(a&0x03){
      case 0: if(pia.ctlA&0x04) pia.orA=v&0xFF; else pia.ddrA=v&0xFF; return;
      case 1: if(pia.ctlB&0x04) pia.orB=v&0xFF; else pia.ddrB=v&0xFF; return;
      case 2: pia.ctlA=v&0xFF; return;
      default: pia.ctlB=v&0xFF; return;
    }
  }
};

} // namespace nap
