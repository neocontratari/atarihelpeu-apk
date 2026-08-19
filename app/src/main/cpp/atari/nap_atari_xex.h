// nap_atari_xex.h
// BUILD2SA20: zavadec programu ve formatu XEX (binarni soubor Atari DOS).
//
// Dela to same, co zavadec v OS: cte segmenty, kazdy nasype na svou adresu,
// po kazdem segmentu zavola INITAD ($02E2), kdyz se zmenil, a na konci
// skoci na RUNAD ($02E0).
// Zadna zkratka - segmenty se zapisuji pres normalni zapis do pameti,
// takze RAM pod ROM se plni stejne jako na zeleze.
#pragma once
#include <cstdint>
#include "nap_atari_machine.h"

namespace nap {

struct XexVysledek {
  bool ok = false;
  int segmentu = 0;
  int bajtu = 0;
  int runad = -1;
  int initVolani = 0;
  const char *chyba = nullptr;
};

/** Necha bezet CPU, dokud se nevrati z podprogramu na zarazku. */
inline void nap_zavolej(Machine &M, int adr, int maxCyklu = 4000000) {
  const int ZARAZKA = 0x0100;              // sem "skoci" navrat
  M.mem.ram[ZARAZKA] = 0x00;               // BRK - sem se nikdy nedostane
  M.cpu.c.sp = (M.cpu.c.sp - 0) & 0xFF;
  // rucne polozit navratovou adresu (RTS pak skoci na ZARAZKA)
  const int navrat = ZARAZKA - 1;
  M.mem.ram[0x100 | M.cpu.c.sp] = (navrat >> 8) & 0xFF; M.cpu.c.sp = (M.cpu.c.sp - 1) & 0xFF;
  M.mem.ram[0x100 | M.cpu.c.sp] = navrat & 0xFF;        M.cpu.c.sp = (M.cpu.c.sp - 1) & 0xFF;
  M.cpu.c.pc = adr & 0xFFFF;
  const long long konec = M.cpu.c.cycles + maxCyklu;
  while (M.cpu.c.cycles < konec && !M.cpu.c.jam) {
    M.cpu.step();
    if (M.cpu.c.pc == ZARAZKA) return;     // vratil se
  }
}

inline XexVysledek nap_nahrajXex(Machine &M, const uint8_t *d, int n) {
  XexVysledek v;
  if (n < 6) { v.chyba = "soubor je prilis kratky"; return v; }

  int p = 0;
  if (d[0] == 0xFF && d[1] == 0xFF) p = 2;          // uvodni znacka
  else { v.chyba = "chybi znacka $FFFF na zacatku"; return v; }

  int initPredtim = M.mem.ram[0x2E2] | (M.mem.ram[0x2E3] << 8);
  M.mem.ram[0x2E0] = 0; M.mem.ram[0x2E1] = 0;       // RUNAD vynulovat

  while (p + 3 < n) {
    // dalsi $FFFF mezi segmenty se preskakuje
    while (p + 1 < n && d[p] == 0xFF && d[p+1] == 0xFF) p += 2;
    if (p + 3 >= n) break;

    const int od = d[p] | (d[p+1] << 8);
    const int doo = d[p+2] | (d[p+3] << 8);
    p += 4;
    if (doo < od) { v.chyba = "segment ma konec pred zacatkem"; return v; }
    const int delka = doo - od + 1;
    if (p + delka > n) {
      // posledni segment muze byt oriznuty - nacteme, co je
      const int zbytek = n - p;
      for (int i = 0; i < zbytek; i++) M.write((od + i) & 0xFFFF, d[p + i]);
      v.bajtu += zbytek; v.segmentu++;
      p = n;
      break;
    }
    for (int i = 0; i < delka; i++) M.write((od + i) & 0xFFFF, d[p + i]);
    p += delka;
    v.bajtu += delka; v.segmentu++;

    // INITAD: kdyz se zmenil, OS ho po segmentu zavola
    const int initTed = M.mem.ram[0x2E2] | (M.mem.ram[0x2E3] << 8);
    if (initTed != initPredtim && initTed != 0) {
      nap_zavolej(M, initTed);
      v.initVolani++;
      initPredtim = initTed;
    }
  }

  v.runad = M.mem.ram[0x2E0] | (M.mem.ram[0x2E1] << 8);
  v.ok = (v.segmentu > 0);
  if (!v.ok) v.chyba = "zadny segment";
  return v;
}

} // namespace nap
