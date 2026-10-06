// nap_atari_runtime.h
// B291: cista C++ logika "zivota" Atari v HELP, ktera nepotrebuje Android
// (proto jde poctive otestovat na pocitaci - test_overeni/b291):
//
//  1) TypeQueue  - psani textu do Atari pres klavesnicove preruseni (tlacitko
//                  BASIC/TBXL TXT). Presne stejna pravidla jako overene stare
//                  JS jadro (emu_vbxe queueText/typeStep): dalsi klavesa az
//                  kdyz OS prevzal predchozi (CH $02FC = $FF), RETURN dostane
//                  vic casu. Zadny zapis do RAM ("RAM inject") - jen klavesy.
//  2) CsaveRecorder - CSAVE -> WAV primo v C++. B292: nahrava se linka
//                  SIO DATA OUT (to, co POKEY posila do magnetofonu - FSK
//                  5327/3995 Hz v dvoutonovem rezimu), NE zvuk z reproduktoru
//                  (ten zavisi na SOUNDR a michaji se do nej dalsi kanaly).
//                  Jen kdyz bezi kazetovy motor (PACTL bit3 = 0) - presne jako
//                  skutecny magnetofon. Ulozi se jen kdyz behem behu motoru
//                  opravdu odesla data (SEROUT) - CLOAD (cteni) WAV nevytvori.
//  3) XexLoader  - spusteni XEX souboru (tlacitka XEX/MOBIL, TURBO/BASIC):
//                  studeny start s drzenym OPTION (BASIC vypnuty), pak se
//                  segmenty nahraji az ve chvili, kdy OS po startu odpocava,
//                  INIT rutiny bezi v NORMALNI emulaci (vcetne VBLANK) a na
//                  konci skok na RUNAD.
#pragma once
#include <cstdint>
#include <cstring>
#include <cstdio>
#include <deque>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include "nap_atari_machine.h"

namespace nap {

// ---------------------------------------------------------------------
//  ASCII -> scankod (presna kopie tabulek KEY/SHIFTED ze stareho JS jadra)
// ---------------------------------------------------------------------
inline int scanZakladni(int c) {
  switch (c) {
    case 'l': return 0;  case 'j': return 1;  case ';': return 2;  case 'k': return 5;
    case '+': return 6;  case '*': return 7;  case 'o': return 8;  case 'p': return 10;
    case 'u': return 11; case 'i': return 13; case '-': return 14; case '=': return 15;
    case 'v': return 16; case 'c': return 18; case 'b': return 21; case 'x': return 22;
    case 'z': return 23; case '4': return 24; case '3': return 26; case '6': return 27;
    case '5': return 29; case '2': return 30; case '1': return 31; case ',': return 32;
    case ' ': return 33; case '.': return 34; case 'n': return 35; case 'm': return 37;
    case '/': return 38; case 'r': return 40; case 'e': return 42; case 'y': return 43;
    case 't': return 45; case 'w': return 46; case 'q': return 47; case '9': return 48;
    case '0': return 50; case '7': return 51; case '8': return 53; case 'f': return 56;
    case 'h': return 57; case 'd': return 58; case 'g': return 61; case 's': return 62;
    case 'a': return 63; case '<': return 54; case '>': return 55;
    default: return -1;
  }
}
inline int asciiNaScan(int c) {
  if (c >= 'A' && c <= 'Z') return scanZakladni(c - 'A' + 'a');   // BASIC pise velka bez SHIFT (caps)
  int z = scanZakladni(c);
  if (z >= 0) return z;
  const char *SH = "!\"#$%&'@():?^_|\\[]";
  const char *BASE = "1234567890;/*-=+,.";
  for (int i = 0; SH[i]; i++) if (SH[i] == c) { int b = scanZakladni(BASE[i]); return b >= 0 ? (b | 0x40) : -1; }
  return -1;
}

struct TypeQueue {
  std::deque<int> q;       // scankody; 12 = RETURN
  int last = -1;           // posledni poslany scankod
  int odPosledni = 999;    // snimku od posledni klavesy
  int preskoceno = 0;      // znaky, ktere na Atari klavesnici nejsou
  void clear() { q.clear(); last = -1; odPosledni = 999; }
  // \n = RETURN. Posledni radek dostane RETURN automaticky (jako JS R11).
  int addText(const std::string &t) {
    int n = 0;
    std::string s;
    for (char ch : t) if (ch != '\r') s.push_back(ch);
    if (!s.empty() && s.back() != '\n') s.push_back('\n');
    for (char ch : s) {
      if (ch == '\n') { q.push_back(12); n++; continue; }
      int sc = asciiNaScan((unsigned char)ch);
      if (sc < 0) { preskoceno++; continue; }
      q.push_back(sc); n++;
    }
    return n;
  }
  bool empty() const { return q.empty(); }
  // Vola se JEDNOU ZA SNIMEK pred runFrame(). Vraci true, kdyz poslal klavesu.
  bool step(Machine &m) {
    if (odPosledni < 100000) odPosledni++;
    if (q.empty()) return false;
    const int sc = q.front();
    // minimalni odstup: RETURN necha BASIC radek zpracovat; STEJNA klavesa
    // hned po sobe: OS ji ignoruje, dokud KEYDEL (3) neodpocita - a ten
    // odpocitava jen ve VBI, kdyz uz je klavesa PUSTENA (OS $C1A1: SKSTAT
    // bit 2). Klavesa je drzena 4 snimky + 3 snimky KEYDEL -> 9 (rezerva).
    int gap = (last == 12) ? 12 : 2;
    if (sc == last) gap = std::max(gap, 9);
    if (odPosledni < gap) return false;
    // OS jeste nevyzvedl predchozi klavesu (CH $02FC != $FF) - pockat
    if (m.mem.ram[0x2FC] != 0xFF) return false;
    q.pop_front();
    m.klavesa(sc & 0xFF);
    last = sc; odPosledni = 0;
    return true;
  }
};

// ---------------------------------------------------------------------
//  CSAVE -> WAV
// ---------------------------------------------------------------------
struct CsaveRecorder {
  bool armed = false;                // az OS jednou motor VYPNE (po studenem startu je PIA na 0 = "zapnuto")
  bool active = false;
  long long serout0 = 0;
  std::vector<int16_t> pcm;          // mono 44100 Hz
  std::vector<int16_t> hotovo;       // posledni dokonceny zaznam (ceka na ulozeni)
  bool maHotovo = false;
  std::vector<std::string> log;
  static const size_t MAX_VZORKU = (size_t)44100 * 60 * 10;   // pojistka 10 minut
  static bool motor(const Machine &m) { return m.motorOn(); }
  // po studenem startu / vypnuti: zacit znovu "nenatazeny"
  void reset() { armed = false; active = false; pcm.clear(); }
  // vola se po kazdem snimku s presne temi vzorky, co sly do reproduktoru
  void snimek(const Machine &m, const float *mono, int n) {
    bool mot = motor(m);
    if (!armed) { if (!mot) armed = true; return; }
    if (mot && !active) {
      active = true; pcm.clear(); serout0 = m.seroutPocet;
      log.push_back("B292 KAZETA motor ZAPNUT (PACTL bit3=0) - nahravam linku SIO DATA OUT");
    }
    if (active) {
      if (pcm.size() + (size_t)n <= MAX_VZORKU) {
        for (int i = 0; i < n; i++) {
          float f = mono[i];
          if (f > 1.f) f = 1.f; else if (f < -1.f) f = -1.f;
          pcm.push_back((int16_t)std::lround(f * 32767.0));
        }
      }
    }
    if (!mot && active) {
      active = false;
      long long bajtu = m.seroutPocet - serout0;
      double sek = pcm.size() / 44100.0;
      char b[220];
      // skutecny CSAVE posle aspon jeden 132-bajtovy zaznam a trva vteriny
      if (bajtu >= 100 && sek >= 2.0) {
        hotovo.swap(pcm); pcm.clear(); maHotovo = true;
        std::snprintf(b, sizeof(b), "B292 CSAVE_HOTOVO motor VYPNUT, SEROUT bajtu=%lld, zvuk=%.1fs -> ukladam WAV", bajtu, sek);
      } else {
        pcm.clear();
        std::snprintf(b, sizeof(b), "B292 KAZETA motor VYPNUT (SEROUT bajtu=%lld, %.1fs) - neni to CSAVE, WAV se nevytvari", bajtu, sek);
      }
      log.push_back(b);
    }
  }
};


// ---------------------------------------------------------------------
//  B298: kazeta s bootem (hry na kazete, ne BASIC). Na skutecnem 130XE:
//  pri zapnuti drzet START (= boot z kazety) a OPTION (= BASIC vypnuty),
//  OS jednou pipne a ceka na klavesu, pak se zapne motor a hra se nahraje.
//  Tohle dela totez: drzi START+OPTION od studeneho startu, pozna pipnuti
//  (reproduktor v GTIA - rada cvaknuti), po jeho skonceni stiskne RETURN
//  a konzoli pusti. Kdyby OS nepipnul do 12 s, RETURN se stiskne stejne.
// ---------------------------------------------------------------------
struct KazetaBoot {
  enum Stav { NIC, CEKA_PIP, PIPA, HOTOVO } stav = NIC;
  int snimku = 0, tichoSnimku = 0;
  long long klikPosl = 0, klikPiskStart = 0;
  std::vector<std::string> log;
  bool aktivni() const { return stav == CEKA_PIP || stav == PIPA; }
  // konzole behem bootu: START + OPTION drzene (bity 0 a 2 = 0)
  static int konzole(int c) { return c & ~5; }
  void start(const Machine &m) { stav = CEKA_PIP; snimku = 0; tichoSnimku = 0; klikPosl = m.gtiaKlikPocitadlo; klikPiskStart = klikPosl; }
  void zrus() { stav = NIC; }
  // vola se po kazdem snimku; vraci true, kdyz prave stiskl RETURN
  bool poSnimku(Machine &m) {
    if (!aktivni()) return false;
    snimku++;
    const long long k = m.gtiaKlikPocitadlo;
    if (stav == CEKA_PIP) {
      // pipnuti = desitky prepnuti reproduktoru za par snimku (jednotliva
      // cvaknuti pri inicializaci OS se nepocitaji)
      if (k - klikPosl >= 20) { stav = PIPA; klikPiskStart = klikPosl; klikPosl = k; tichoSnimku = 0; return false; }
      if (snimku % 5 == 0) klikPosl = k;
      if (snimku > 50 * 12) {
        m.klavesa(12);
        log.push_back("B298 KAZETA BOOT: OS do 12 s nepipnul - RETURN stisknut stejne");
        stav = HOTOVO; return true;
      }
      return false;
    }
    // PIPA: ceka se, az pipnuti skonci (10 snimku bez cvaknuti)
    if (k != klikPosl) { klikPosl = k; tichoSnimku = 0; return false; }
    if (++tichoSnimku >= 10) {
      m.klavesa(12);
      char b[200];
      std::snprintf(b, sizeof(b), "B298 KAZETA BOOT: OS pipnul (%lld cvaknuti, snimek %d) - RETURN stisknut, motor bezi, hra se nahrava",
                    klikPosl - klikPiskStart, snimku);
      log.push_back(b);
      stav = HOTOVO; return true;
    }
    return false;
  }
};

// ---------------------------------------------------------------------
//  XEX zavadec (tlacitka XEX/MOBIL a TURBO/BASIC na pristroji)
//
//  Postup (stejna myslenka jako zavadeni XEX bez DOSu v jinych emulatorech):
//   1) studeny start s drzenym OPTION -> OS vypne BASIC (BASICF=1, RAMTOP $C0)
//   2) OS se pokusi zavest disk D1: - tady neni, proto SIO hned vrati 138
//      (misto ~10 s cekani) - viz Machine::sioRychlyTimeout
//   3) na konci startu OS skoci pres DOSVEC; ten presmerujeme na "parkovaci"
//      smycku $0100: JMP $0100. Procesor v ni beha s plne nastavenym OS,
//      preruseni (VBLANK) bezi normalne.
//   4) segmenty XEX se zapisou do pameti; kdyz segment nastavi INITAD
//      ($02E2), zavola se INIT jako JSR - BEZI V NORMALNI EMULACI snimek po
//      snimku (vcetne VBLANK), a az se vrati do parkovaci smycky, pokracuje
//      se dalsim segmentem (presne jako DOS).
//   5) nakonec JSR na RUNAD ($02E0) - program bezi.
// ---------------------------------------------------------------------
struct XexLoader {
  // INIT_DLOUHO (B296): INIT bezi uz pres 10 s - bud prevzal rizeni (TBXL,
  // Decathlon), nebo ceka na hrace (titulka "stiskni klavesu" - Night Driver
  // VBXE). Uzivatel uz ma konzoli; kdyz se INIT vrati, nahraje se zbytek.
  enum Stav { NIC, START, SEGMENT, INIT, INIT_DLOUHO, BEZI, CHYBA } stav = NIC;
  std::vector<uint8_t> data;
  std::string jmeno;
  size_t pos = 0;
  int prvniStart = -1, segmentu = 0, bajtu = 0, initu = 0, snimku = 0, initSnimku = 0;
  std::vector<std::string> log;
  static const int PARK = 0x0100;
  bool aktivni() const { return stav == START || stav == SEGMENT || stav == INIT; }

  // overi format; true = lze zavest (pak volajici udela studeny start s OPTION)
  bool priprav(const uint8_t *d, size_t n, const std::string &nm) {
    stav = NIC; data.clear(); log.clear();
    jmeno = nm; pos = 0; prvniStart = -1; segmentu = bajtu = initu = snimku = initSnimku = 0;
    char b[200];
    if (!d || n < 7 || d[0] != 0xFF || d[1] != 0xFF) {
      std::snprintf(b, sizeof(b), "B291 XEX %s: neni to XEX (chybi znacka $FFFF na zacatku) - nic se nespousti", nm.c_str());
      log.push_back(b); stav = CHYBA; return false;
    }
    data.assign(d, d + n); pos = 2; stav = START;
    std::snprintf(b, sizeof(b), "B291 XEX %s: %u bajtu - studeny start s OPTION (BASIC vypnuty), pak segmenty", nm.c_str(), (unsigned)n);
    log.push_back(b);
    return true;
  }
  void zrus() { if (aktivni()) log.push_back("B291 XEX zavadeni preruseno"); stav = NIC; data.clear(); }

  static void jsr(Machine &m, int adr) {
    const int navrat = PARK - 1;     // RTS -> $0100
    m.mem.ram[0x100 | m.cpu.s] = (navrat >> 8) & 0xFF; m.cpu.s = (uint8_t)(m.cpu.s - 1);
    m.mem.ram[0x100 | m.cpu.s] = navrat & 0xFF;        m.cpu.s = (uint8_t)(m.cpu.s - 1);
    m.cpu.pc = (uint16_t)(adr & 0xFFFF);
  }
  static void parkKod(Machine &m) { m.mem.ram[PARK] = 0x4C; m.mem.ram[PARK + 1] = PARK & 0xFF; m.mem.ram[PARK + 2] = PARK >> 8; }

  // Vola se po kazdem snimku (pod zamkem stroje).
  void poSnimku(Machine &m) {
    if (!aktivni() && stav != INIT_DLOUHO) return;
    snimku++;
    char b[220];
    if (stav == INIT_DLOUHO) {
      if (m.cpu.pc != PARK) { initSnimku++; return; }  // INIT (program) bezi dal
      std::snprintf(b, sizeof(b), "B296 XEX %s: INIT c.%d se vratil po %d s - nahravam dalsi segmenty", jmeno.c_str(), initu, initSnimku / 50);
      log.push_back(b);
      stav = SEGMENT;
    }
    if (stav == START) {
      parkKod(m);
      int dv = m.mem.ram[0x0A] | (m.mem.ram[0x0B] << 8);
      if (dv != 0 && dv != PARK) { m.mem.ram[0x0A] = PARK & 0xFF; m.mem.ram[0x0B] = PARK >> 8; }
      if (m.cpu.pc == PARK && snimku > 10) {
        m.sioRychlyTimeout = false;
        m.consol = 7;                                  // OPTION pustit (hry ho ctou na titulce)
        m.mem.ram[0x2E0] = 0; m.mem.ram[0x2E1] = 0;    // RUNAD
        std::snprintf(b, sizeof(b), "B291 XEX OS nastartovan za %d snimku (BASIC=%s, RAMTOP=$%02X) - nahravam segmenty",
                      snimku, (m.mem.portB() & 2) ? "VYPNUTY" : "ZAPNUTY", m.mem.ram[0x6A]);
        log.push_back(b);
        stav = SEGMENT;
      } else if (snimku > 50 * 25) {
        m.sioRychlyTimeout = false; m.consol = 7;
        log.push_back("B291 XEX CHYBA: OS se do 25 s nedostal do zavadece - zruseno"); stav = CHYBA; return;
      } else return;
    }
    if (stav == INIT) {
      if (m.cpu.pc != PARK) {
        initSnimku++;
        // INIT, ktery se nevrati, SPUSTIL program sam (napr. Turbo-BASIC XL,
        // Decathlon) - presne tak by to dopadlo i s DOSem. Po 10 s dostane
        // konzoli uzivatel; zavadec ale dal hlida navrat z INIT (B296: titulka
        // "stiskni klavesu" muze cekat libovolne dlouho - pak se nahraje zbytek).
        if (initSnimku > 50 * 10) {
          std::snprintf(b, sizeof(b), "B291 XEX %s: INIT c.%d prevzal rizeni a bezi (program spusten z INIT; kdyz se vrati, nahraju zbytek)", jmeno.c_str(), initu);
          log.push_back(b); stav = INIT_DLOUHO;
        }
        return;                                        // INIT jeste bezi (normalni emulace)
      }
      stav = SEGMENT;
    }
    if (stav == SEGMENT) {
      // vsechny segmenty bez INIT najednou; s INIT -> zavolat a cekat na navrat
      while (true) {
        while (pos + 1 < data.size() && data[pos] == 0xFF && data[pos + 1] == 0xFF) pos += 2;
        if (pos + 4 > data.size()) {
          int run = m.mem.ram[0x2E0] | (m.mem.ram[0x2E1] << 8);
          bool zRunad = run != 0;
          if (!zRunad) run = prvniStart;
          if (run < 0) { log.push_back("B291 XEX CHYBA: zadny segment ani RUNAD"); stav = CHYBA; return; }
          std::snprintf(b, sizeof(b), "B291 XEX %s nahrano: %d segmentu, %d bajtu, %d x INIT -> start $%04X (%s)",
                        jmeno.c_str(), segmentu, bajtu, initu, run, zRunad ? "RUNAD" : "prvni segment, RUNAD chybi");
          log.push_back(b);
          jsr(m, run);
          stav = BEZI; data.clear();
          return;
        }
        int od = data[pos] | (data[pos + 1] << 8), doo = data[pos + 2] | (data[pos + 3] << 8);
        pos += 4;
        if (doo < od) { std::snprintf(b, sizeof(b), "B291 XEX CHYBA: segment $%04X-$%04X ma konec pred zacatkem", od, doo); log.push_back(b); stav = CHYBA; return; }
        int delka = doo - od + 1;
        if (pos + (size_t)delka > data.size()) delka = (int)(data.size() - pos);   // oriznuty posledni segment
        m.mem.ram[0x2E2] = 0; m.mem.ram[0x2E3] = 0;                              // INITAD pred segmentem
        for (int i = 0; i < delka; i++) m.write((od + i) & 0xFFFF, data[pos + i]);
        pos += delka; segmentu++; bajtu += delka;
        if (prvniStart < 0) prvniStart = od;
        int init = m.mem.ram[0x2E2] | (m.mem.ram[0x2E3] << 8);
        if (init != 0) {
          m.mem.ram[0x2E2] = 0; m.mem.ram[0x2E3] = 0;
          initu++; initSnimku = 0;
          jsr(m, init);
          stav = INIT;
          return;                                      // INIT pobezi v dalsich snimcich
        }
      }
    }
  }
};

}  // namespace nap
