// nap_atari_selftest.h
// B292: samokontrola jadra Atari - telefon (ARM64) musi dat PRESNE stejna
// cisla jako pocitac (x86_64), kde se jadro overuje testy (SingleStepTests,
// Acid800, boot). Kdyby se preklad na ARM nekde lisil (napr. 'char' bez
// znamenka, poradi vyhodnoceni, zaokrouhleni), pozna se to tady - ne az na
// obrazu. Cista C++ logika bez Androidu: stejny soubor se pouziva v appce
// (nap_atari_native.cpp, runSelfTest) i v testu na PC
// (test_overeni/b292/test_selftest.cpp), ktery vypise ocekavana cisla pro
// NativeAtariCoreBridge.java.
//
//  1) procesor: pro kazdy z 256 opcodu 200 nahodnych stavu (registry,
//     priznaky, obsah pameti z hashe) -> jedna instrukce -> soucet stavu,
//     poctu cyklu na sbernici a vsech zapisu,
//  2) pamet 130XE: vsech 256 hodnot PORTB, cteni procesoru i ANTICu z cele
//     adresy (OS/BASIC/self-test ROM, 4 banky rozsirene pameti),
//  3) rychlost: 150 snimku skutecneho startu OS (3 s Atari) -> kolikrat
//     rychleji nez skutecne Atari to telefon zvladne.
#pragma once
#include <cstdint>
#include <cstring>
#include <ctime>
#include <map>
#include "nap_atari_machine.h"

namespace nap {
namespace selftest {

inline uint32_t &salt() { static uint32_t s = 0; return s; }
inline uint32_t &rs() { static uint32_t s = 0; return s; }
inline uint32_t genb(uint32_t a) {
  uint32_t h = (a * 0x9E3779B1u) ^ (salt() * 0x85EBCA6Bu);
  h ^= h >> 15; h *= 0xC2B2AE35u; h ^= h >> 13;
  return h & 0xFF;
}
inline uint32_t rnd() { uint32_t &s = rs(); s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
inline void mix(uint32_t &h, uint32_t v) { h ^= v; h *= 16777619u; }

struct Bus {
  std::map<int, int> ovl;
  uint32_t cycles = 0;
  uint8_t rd(uint16_t a) { cycles++; auto it = ovl.find(a); return (uint8_t)(it != ovl.end() ? it->second : (int)genb(a)); }
  void wr(uint16_t a, uint8_t v) { cycles++; ovl[a] = v; }
  bool nmiPending() { return false; }
  void nmiAck() {}
  void traceNmi() {}
  bool irqActive() { return false; }
};

// 1) procesor
inline uint32_t cpuHash(int perOp, uint32_t seed, long long *outInstr) {
  rs() = seed;
  static Bus bus;
  Cpu6502T<Bus> cpu; cpu.bus = &bus;
  uint32_t h = 2166136261u;
  long long n = 0;
  for (int op = 0; op < 256; op++) {
    for (int k = 0; k < perOp; k++) {
      salt() = rnd(); bus.ovl.clear(); bus.cycles = 0;
      cpu.a = (uint8_t)rnd(); cpu.x = (uint8_t)rnd(); cpu.y = (uint8_t)rnd();
      cpu.s = (uint8_t)rnd(); cpu.pc = (uint16_t)rnd(); cpu.p = (uint8_t)(rnd() | 0x20);
      cpu.jam = false; cpu.takeNmi = cpu.takeIrq = false;
      bus.ovl[cpu.pc] = op;
      cpu.step();
      mix(h, (uint32_t)op); mix(h, cpu.a); mix(h, cpu.x); mix(h, cpu.y); mix(h, cpu.s); mix(h, cpu.pc);
      mix(h, (uint32_t)(cpu.p | 0x30)); mix(h, bus.cycles); mix(h, cpu.jam ? 1u : 0u);
      for (auto &kv : bus.ovl) { mix(h, (uint32_t)kv.first); mix(h, (uint32_t)kv.second); }
      n++;
    }
  }
  *outInstr = n;
  return h;
}

// 2) pamet a bankovani 130XE
inline uint32_t memHash(long long *outReads) {
  static uint8_t os[16384], bas[8192];
  for (int i = 0; i < 16384; i++) os[i] = (uint8_t)((i * 31 + 5) & 0xFF);
  for (int i = 0; i < 8192; i++) bas[i] = (uint8_t)((i * 17 + 91) & 0xFF);
  static Machine *mp = nullptr;
  if (!mp) mp = new Machine();
  Machine &m = *mp;
  m.coldInit();
  m.mem.os = os; m.mem.bas = bas;
  for (int i = 0; i < 65536; i++) m.mem.ram[i] = (uint8_t)((i * 7 + 11) & 0xFF);
  for (int i = 0; i < 65536; i++) m.mem.ext[i] = 0;
  auto setPortB = [&](int v) { m.piaWrite(3, 0x00); m.piaWrite(1, 0xFF); m.piaWrite(3, 0x04); m.piaWrite(1, (uint8_t)v); };
  // zapis vzoru do vsech 4 bank pres okno $4000-$7FFF (CPU vidi banku, kdyz bit 4 = 0)
  for (int b = 0; b < 4; b++) {
    setPortB(0xE3 | (b << 2));            // OS zap., BASIC vyp., CPU banka b, ANTIC hlavni
    for (int o = 0; o < 16384; o++) m.memWrite((uint16_t)(0x4000 + o), (uint8_t)((b * 16384 + o) * 13 + 29));
  }
  uint32_t h = 2166136261u; long long n = 0;
  for (int pb = 0; pb < 256; pb++) {
    setPortB(pb);
    for (int a = 0; a < 65536; a++) {
      if (a >= 0xD000 && a < 0xD800) continue;
      mix(h, m.memRead((uint16_t)a, false));
      mix(h, m.memRead((uint16_t)a, true));
      n++;
    }
  }
  *outReads = n;
  return h;
}

// 3) rychlost celeho stroje (procesor + ANTIC + GTIA + POKEY po cyklech)
inline double speed(const uint8_t *osRom, const uint8_t *basRom, long long *outInstr, double *outAtariSec) {
  static Machine *mp = nullptr;
  if (!mp) mp = new Machine();
  Machine &m = *mp;
  m.coldInit();
  m.mem.os = osRom; m.mem.bas = basRom;
  std::memset(m.mem.ram, 0, sizeof m.mem.ram);
  m.consol = 7;
  m.reset();
  const uint64_t i0 = m.cpu.instr;
  timespec t0{}, t1{};
  clock_gettime(CLOCK_MONOTONIC, &t0);
  const int SNIMKU = 150;
  for (int f = 0; f < SNIMKU && !m.cpu.jam; f++) { m.runFrame(); float z[882]; m.genAudio(z, 882, 44100.0); }
  clock_gettime(CLOCK_MONOTONIC, &t1);
  *outInstr = (long long)(m.cpu.instr - i0);
  *outAtariSec = SNIMKU / 50.0;
  return (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;
}

}  // namespace selftest
}  // namespace nap
