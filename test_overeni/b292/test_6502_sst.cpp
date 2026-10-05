// test_6502_sst.cpp - B292: overeni noveho 6502 (nap_atari_6502.h) proti
// SingleStepTests 65x02/6502 (Tom Harte): pro kazdy opcode 10 000 testu,
// kazdy s pocatecnim stavem, koncovym stavem a PRESNYM seznamem cyklu
// (adresa, hodnota, cteni/zapis).
//
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_6502_sst test_6502_sst.cpp
//   ./test_6502_sst soubor.json [soubor.json ...]
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "nap_atari_6502.h"

struct Access { uint16_t a; uint8_t v; bool w; };

struct TestBus {
  uint8_t mem[65536];
  std::vector<Access> log;
  uint8_t rd(uint16_t a) { log.push_back({a, mem[a], false}); return mem[a]; }
  void wr(uint16_t a, uint8_t v) { mem[a] = v; log.push_back({a, v, true}); }
  bool nmiPending() { return false; }
  void nmiAck() {}
  void traceNmi() {}
  bool irqActive() { return false; }
};

// --- maly parser presne tohoto JSON formatu ---
struct P {
  const char *s; size_t i = 0, n;
  P(const std::string &t) : s(t.c_str()), n(t.size()) {}
  void ws() { while (i < n && (s[i] == ' ' || s[i] == '\n' || s[i] == '\r' || s[i] == '\t' || s[i] == ',')) i++; }
  bool lit(char c) { ws(); if (i < n && s[i] == c) { i++; return true; } return false; }
  std::string str() { ws(); std::string r; if (s[i] != '"') return r; i++; while (i < n && s[i] != '"') r += s[i++]; i++; return r; }
  long num() { ws(); char *e; long v = strtol(s + i, &e, 10); i = e - s; return v; }
};

struct St { long pc, sp, a, x, y, p; std::vector<std::pair<long,long>> ram; };

static void readState(P &p, St &st) {
  p.lit('{');
  while (!p.lit('}')) {
    std::string k = p.str(); p.lit(':');
    if (k == "ram") {
      p.lit('[');
      while (!p.lit(']')) { p.lit('['); long ad = p.num(); long v = p.num(); p.lit(']'); st.ram.push_back({ad, v}); }
    } else {
      long v = p.num();
      if (k == "pc") st.pc = v; else if (k == "s") st.sp = v; else if (k == "a") st.a = v;
      else if (k == "x") st.x = v; else if (k == "y") st.y = v; else if (k == "p") st.p = v;
    }
  }
}

int main(int argc, char **argv) {
  long celkem = 0, chyb = 0;
  static TestBus bus;
  nap::Cpu6502T<TestBus> cpu; cpu.bus = &bus;
  for (int fi = 1; fi < argc; fi++) {
    FILE *f = fopen(argv[fi], "rb");
    if (!f) { printf("nelze otevrit %s\n", argv[fi]); return 2; }
    std::string t; char buf[1 << 16]; size_t r;
    while ((r = fread(buf, 1, sizeof buf, f)) > 0) t.append(buf, r);
    fclose(f);
    P p(t);
    p.lit('[');
    long fc = 0, fe = 0; int vypsano = 0;
    while (!p.lit(']')) {
      p.lit('{');
      std::string name; St in{}, out{}; std::vector<Access> cyc;
      while (!p.lit('}')) {
        std::string k = p.str(); p.lit(':');
        if (k == "name") name = p.str();
        else if (k == "initial") readState(p, in);
        else if (k == "final") readState(p, out);
        else if (k == "cycles") {
          p.lit('[');
          while (!p.lit(']')) {
            p.lit('['); long ad = p.num(); long v = p.num(); std::string ty = p.str(); p.lit(']');
            cyc.push_back({(uint16_t)ad, (uint8_t)v, ty == "write"});
          }
        }
      }
      // spustit
      memset(bus.mem, 0, sizeof bus.mem);
      for (auto &e : in.ram) bus.mem[e.first] = (uint8_t)e.second;
      cpu.pc = (uint16_t)in.pc; cpu.s = (uint8_t)in.sp; cpu.a = (uint8_t)in.a; cpu.x = (uint8_t)in.x;
      cpu.y = (uint8_t)in.y; cpu.p = (uint8_t)in.p; cpu.jam = false; cpu.takeNmi = cpu.takeIrq = false;
      bus.log.clear();
      cpu.step();
      bool ok = true; std::string proc;
      // B: nas procesor B v registru nedrzi (jako skutecny 6502) - porovnani bez B
      auto chk = [&](const char *nm, long a, long b) { if (a != b) { ok = false; char m[64]; snprintf(m, 64, " %s=%ld(ocek. %ld)", nm, a, b); proc += m; } };
      chk("pc", cpu.pc, out.pc); chk("s", cpu.s, out.sp); chk("a", cpu.a, out.a);
      chk("x", cpu.x, out.x); chk("y", cpu.y, out.y);
      chk("p", cpu.p | 0x30, out.p | 0x30);
      for (auto &e : out.ram) if (bus.mem[e.first] != (uint8_t)e.second) { ok = false; char m[64]; snprintf(m, 64, " ram[%04lX]=%02X(ocek. %02lX)", e.first, bus.mem[e.first], e.second); proc += m; }
      if (!cpu.jam) {
        if (bus.log.size() != cyc.size()) { ok = false; char m[64]; snprintf(m, 64, " cyklu=%zu(ocek. %zu)", bus.log.size(), cyc.size()); proc += m; }
        else for (size_t k = 0; k < cyc.size(); k++) {
          if (bus.log[k].a != cyc[k].a || bus.log[k].v != cyc[k].v || bus.log[k].w != cyc[k].w) {
            ok = false; char m[96]; snprintf(m, 96, " cyklus%zu=%04X/%02X/%c(ocek. %04X/%02X/%c)", k + 1, bus.log[k].a, bus.log[k].v, bus.log[k].w ? 'W' : 'R', cyc[k].a, cyc[k].v, cyc[k].w ? 'W' : 'R');
            proc += m; break;
          }
        }
      }
      fc++; if (!ok) { fe++; if (vypsano++ < 3) printf("  CHYBA %s:%s\n", name.c_str(), proc.c_str()); }
    }
    printf("%s: %ld testu, %ld chyb\n", argv[fi], fc, fe);
    celkem += fc; chyb += fe;
  }
  printf("CELKEM %ld testu, %ld chyb\n", celkem, chyb);
  return chyb ? 1 : 0;
}
