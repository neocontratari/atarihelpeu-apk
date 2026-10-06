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

// ---------------------------------------------------------------------
//  B299: pomocne veci pro diagnostiku (LOG/CHYBA) a psani programu
// ---------------------------------------------------------------------
inline uint32_t napCrc32(const uint8_t *d, size_t n, uint32_t crc = 0) {
  crc = ~crc;
  for (size_t i = 0; i < n; i++) {
    crc ^= d[i];
    for (int k = 0; k < 8; k++) crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
  }
  return ~crc;
}

// Radky programu BASIC / Turbo-BASIC XL v pameti (STMTAB $88): pocet a cisla
// radku s chybou syntaxe (BASIC je ulozi jako prikaz "ERROR-" = token $37).
inline int napRadkyProgramu(const Machine &m, std::vector<int> *sChybou) {
  int p = m.mem.ram[0x88] | (m.mem.ram[0x89] << 8), n = 0;
  for (int i = 0; i < 6000; i++) {
    const int ln = m.mem.ram[p & 0xFFFF] | (m.mem.ram[(p + 1) & 0xFFFF] << 8);
    const int len = m.mem.ram[(p + 2) & 0xFFFF];
    if (ln >= 32768 || len < 4) break;
    n++;
    if (sChybou && m.mem.ram[(p + 4) & 0xFFFF] == 0x37) sChybou->push_back(ln);
    p += len;
  }
  return n;
}

// Ukazatele BASICu / TBXL a kontrolni soucty oblasti pameti (LOG/CHYBA, "B299 PAMET");
// stejna funkce bezi v testu na PC, takze se cisla z telefonu daji porovnat.
inline std::string napDiagPameti(const Machine &m) {
  const uint8_t *r = m.mem.ram;
  auto w = [&](int a) { return r[a] | (r[a + 1] << 8); };
  auto crc = [&](int od, int doo) { return napCrc32(r + od, (size_t)(doo - od + 1)); };
  char b[900];
  std::string out;
  const int vntp = w(0x82), starp = w(0x8C);
  const uint32_t crcProg = (vntp > 0 && vntp < starp && starp - vntp < 0xB000) ? napCrc32(r + vntp, (size_t)(starp - vntp)) : 0;
  std::vector<int> chyby;
  const int radku = napRadkyProgramu(m, &chyby);
  std::snprintf(b, sizeof(b), "B299 PAMET BASIC: LOMEM $%04X VNTP $%04X VNTD $%04X VVTP $%04X STMTAB $%04X STMCUR $%04X STARP $%04X MEMTOP $%04X; "
                "program VNTP..STARP %d B crc32 %08X, radku %d, s chybou syntaxe %d",
                w(0x80), vntp, w(0x84), w(0x86), w(0x88), w(0x8A), starp, w(0x90), starp - vntp, crcProg, radku, (int)chyby.size());
  out += b; out += "\n";
  std::snprintf(b, sizeof(b), "B299 PAMET CRC32: $00-$7F %08X, $80-$FF %08X, $0100-$01FF %08X, $0200-$03FF %08X, $0400-$06FF %08X, "
                "$0700-$1FFF %08X, $2000-$3FFF %08X, $4000-$7FFF %08X, $8000-$BFFF %08X, RAM pod ROM $C000-$CFFF %08X, $D800-$FFFF %08X, "
                "rozsirena 64 kB %08X; VBXE MEMAC A $%02X/$%02X B $%02X",
                crc(0x00, 0x7F), crc(0x80, 0xFF), crc(0x100, 0x1FF), crc(0x200, 0x3FF), crc(0x400, 0x6FF), crc(0x700, 0x1FFF),
                crc(0x2000, 0x3FFF), crc(0x4000, 0x7FFF), crc(0x8000, 0xBFFF), crc(0xC000, 0xCFFF), crc(0xD800, 0xFFFF),
                napCrc32(m.mem.ext, sizeof(m.mem.ext)), m.vbx.memacCtl, m.vbx.memacBankA, m.vbx.memacBankB);
  out += b;
  return out;
}

// Scankod (+ $40 SHIFT, $80 CONTROL) -> text pro log (co clovek napsal)
inline std::string napScanText(int kod) {
  const int sc = kod & 0x3F;
  switch (sc) {
    case 12: return "<RETURN>";
    case 52: return (kod & 0x40) ? "<SMAZ.RADEK>" : (kod & 0x80) ? "<DEL>" : "<BKSP>";
    case 28: return "<ESC>";
    case 44: return "<TAB>";
    case 60: return "<CAPS>";
    case 39: return "<INVERZE>";
    case 17: return "<HELP>";
    default: break;
  }
  static const char *zaklad = "abcdefghijklmnopqrstuvwxyz0123456789,. ;+*-=/<>";
  for (const char *c = zaklad; *c; c++) {
    if (scanZakladni(*c) != sc) continue;
    std::string s;
    if (kod & 0x80) s += "^";
    char ch = *c;
    if (kod & 0x40) {
      const char *SH = "!\"#$%&'@():?^_|\\[]", *BASE = "1234567890;/*-=+,.";
      const char *p = std::strchr(BASE, ch);
      if (p) ch = SH[p - BASE];
      else if (ch == '<') { return s + "<CLEAR>"; }
      else if (ch == '>') { return s + "<INSERT>"; }
    }
    if (ch >= 'a' && ch <= 'z') ch = (char)(ch - 'a' + 'A');       // OS po startu pise velka (CAPS)
    return s + ch;
  }
  char b[12]; std::snprintf(b, sizeof(b), "<%02X>", kod & 0xFF);
  return b;
}

// ---------------------------------------------------------------------
//  B299: PROGRAM Z TXT SOUBORU -> ATARI BASIC / TURBO-BASIC XL
//  (tlacitko BASIC/TBXL TXT -> TXT SOUBOR). Rene: "abych si mohl vybrat txt
//  soubor s kodem jak pro turbobasic tak pro basic a aby ho aplikace
//  automaticky prepsala do obrazovky, kde sel spustit a pote i ulozit a nahrat".
//
//  Dela to, co clovek u klavesnice, jen rychle:
//   - pocka na READY (BASIC / TBXL ceka na radek),
//   - napise NEW a pak radek po radku - jen radky s cislem (ostatni se
//     preskoci a reknou se v logu),
//   - klavesy jdou do CH ($02FC) - tam, kam je dava klavesnicove preruseni;
//     OS je cte stejne (K: -> E: -> BASIC) a BASIC kazdy radek zpracuje, jako
//     by ho nekdo napsal. Dalsi klavesa az kdyz OS predchozi prevzal (CH=$FF),
//   - behem psani: LMARGN = 0 (radek az 120 znaku, jako POKE 82,0), bez
//     klapani klaves (NOCLIK), mala/velka pismena presne podle souboru
//     (SHFLOK), inverzni znaky (INVFLG), ridici znaky v retezcich s ESC -
//     jako to pise clovek. Pak se vse vrati.
//   - nakonec projde program v pameti: radky s chybou syntaxe vypise cislem.
// ---------------------------------------------------------------------
struct PsaniProgramu {
  enum Stav { NIC, CEKA_READY, PISE, DOBEH, HOTOVO, CHYBA } stav = NIC;
  struct Klav { uint8_t kod; int8_t inv; };   // kod: scankod | $40 SHIFT | $80 CONTROL; inv 1/0 = INVFLG, -1 = jedno
  std::deque<Klav> q;
  bool tbxl = false, rychlyTimeoutVypnout = false;
  std::string jmeno;
  int radku = 0, prazdnych = 0, bezCisla = 0, dlouhych = 0, znakuNelze = 0, klaves = 0;
  std::vector<int> dlouhe;
  std::vector<std::string> bezCislaUkazka;
  int snimku = 0, klid = 0, cekaInv = 0, snimkuPsani = 0;
  uint8_t puvLmargn = 2, puvShflok = 0x40, puvNoclik = 0;
  std::vector<std::string> log;
  std::string status; int statusMs = 0;        // zprava pod pristroj (vyzvedne vlakno emulace)
  bool aktivni() const { return stav == CEKA_READY || stav == PISE || stav == DOBEH; }
  bool pise() const { return stav == PISE; }
  void zrus() {
    if (aktivni()) log.push_back("B299 TXT PROGRAM: psani preruseno (POWER / RESET / jiny program)");
    stav = NIC; q.clear();
  }

  // ATASCII znak -> klavesy Atari. false = na klavesnici Atari nejde napsat.
  static bool klavesy(int c, std::vector<Klav> &out) {
    const int ESC = 28;
    auto k = [&](int kod, int inv = 0) { out.push_back({(uint8_t)kod, (int8_t)inv}); };
    if (c < 0 || c > 0xFF) return false;
    if (c >= 0x80) {
      switch (c) {                                       // editacni kody (v retezci s ESC)
        case 0x9B: return false;                         // EOL = konec radku
        case 0x9C: k(ESC, -1); k(52 | 0x40); return true;  // smazat radek
        case 0x9D: k(ESC, -1); k(55 | 0x40); return true;  // vlozit radek
        case 0x9E: k(ESC, -1); k(44 | 0x80); return true;  // CTRL-TAB
        case 0x9F: k(ESC, -1); k(44 | 0x40); return true;  // SHIFT-TAB
        case 0xFD: k(ESC, -1); k(30 | 0x80); return true;  // zvonek
        case 0xFE: k(ESC, -1); k(52 | 0x80); return true;  // smazat znak
        case 0xFF: k(ESC, -1); k(55 | 0x80); return true;  // vlozit znak
        default: break;
      }
      std::vector<Klav> z;                               // inverzni znak = zakladni znak s INVFLG
      if (!klavesy(c & 0x7F, z)) return false;
      for (auto &x : z) { if (x.inv >= 0) x.inv = 1; out.push_back(x); }
      return true;
    }
    if (c >= 'a' && c <= 'z') { k(scanZakladni(c)); return true; }                    // SHFLOK=0: mala
    if (c >= 'A' && c <= 'Z') { k(scanZakladni(c - 'A' + 'a') | 0x40); return true; } // SHIFT: velka
    switch (c) {
      case 0x00: k(32 | 0x80); return true;              // srdce = CTRL-,
      case 0x1B: k(ESC, -1); k(ESC, -1); return true;     // ESC v retezci
      case 0x1C: k(ESC, -1); k(14 | 0x80); return true;   // kurzor nahoru
      case 0x1D: k(ESC, -1); k(15 | 0x80); return true;   // kurzor dolu
      case 0x1E: k(ESC, -1); k(6 | 0x80); return true;    // kurzor vlevo
      case 0x1F: k(ESC, -1); k(7 | 0x80); return true;    // kurzor vpravo
      case 0x60: k(34 | 0x80); return true;              // kara = CTRL-.
      case 0x7B: k(2 | 0x80); return true;               // pika = CTRL-;
      case 0x7C: k(15 | 0x40); return true;              // |
      case 0x7D: k(ESC, -1); k(54 | 0x40); return true;   // CLEAR (smazani obrazovky) v retezci
      case 0x7E: k(ESC, -1); k(52); return true;          // BACKSPACE v retezci
      case 0x7F: k(ESC, -1); k(44); return true;          // TAB v retezci
      default: break;
    }
    if (c >= 0x01 && c <= 0x1A) { k(scanZakladni('a' + c - 1) | 0x80); return true; }   // CTRL-A..Z (grafika)
    const int s = asciiNaScan(c);                        // cislice, mezera, interpunkce
    if (s < 0) return false;
    k(s); return true;
  }

  // soubor -> radky jako ATASCII kody (-1 = znak, ktery v ATASCII neni)
  static std::vector<std::vector<int>> radkySouboru(const uint8_t *d, size_t n, bool &atascii) {
    std::vector<std::vector<int>> out;
    atascii = false;
    for (size_t i = 0; i < n; i++) if (d[i] == 0x9B) { atascii = true; break; }
    size_t i = 0;
    if (!atascii && n >= 3 && d[0] == 0xEF && d[1] == 0xBB && d[2] == 0xBF) i = 3;   // UTF-8 BOM
    // platne UTF-8? (jinak 8bitove ATASCII s PC konci radku)
    bool utf8 = !atascii;
    for (size_t j = i; utf8 && j < n; ) {
      const uint8_t b = d[j];
      int dl = b < 0x80 ? 1 : (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : (b & 0xF8) == 0xF0 ? 4 : 0;
      if (!dl || j + dl > n) { utf8 = false; break; }
      for (int k = 1; k < dl; k++) if ((d[j + k] & 0xC0) != 0x80) { utf8 = false; break; }
      j += dl;
    }
    std::vector<int> r;
    auto konec = [&]() { out.push_back(r); r.clear(); };
    while (i < n) {
      const uint8_t b = d[i];
      if (atascii) { if (b == 0x9B) konec(); else r.push_back(b); i++; continue; }
      if (b == '\r') { konec(); i++; if (i < n && d[i] == '\n') i++; continue; }
      if (b == '\n') { konec(); i++; continue; }
      if (b == '\t') { r.push_back(' '); i++; continue; }
      if (b < 0x80 || !utf8) { r.push_back(b); i++; continue; }
      int dl = (b & 0xE0) == 0xC0 ? 2 : (b & 0xF0) == 0xE0 ? 3 : 4;
      r.push_back(-1); i += dl;                          // ceske znaky, emoji... na Atari nejsou
    }
    if (!r.empty()) konec();
    return out;
  }

  // velka pismena mimo uvozovky (BASIC zna jen velke prikazy a promenne),
  // ale za REM / DATA zustane text tak, jak je
  static void velkaPismena(std::vector<int> &r, size_t od) {
    bool uvoz = false, zacatek = true;
    for (size_t i = od; i < r.size(); i++) {
      const int c = r[i];
      if (c == '"') { uvoz = !uvoz; zacatek = false; continue; }
      if (uvoz) continue;
      if (c == ':') { zacatek = true; continue; }
      if (zacatek && c == ' ') continue;
      if (zacatek) {
        zacatek = false;
        auto slovo = [&](const char *w) {
          size_t k = 0;
          for (; w[k]; k++) {
            if (i + k >= r.size()) return false;
            int x = r[i + k]; if (x >= 'a' && x <= 'z') x -= 32;
            if (x != w[k]) return false;
          }
          return true;
        };
        const char *kom[] = {"REM", "DATA", "R.", "D.", "."};
        for (const char *w : kom) {
          if (!slovo(w)) continue;
          for (size_t k = 0; w[k]; k++) if (r[i + k] >= 'a' && r[i + k] <= 'z') r[i + k] -= 32;
          return;                                          // zbytek radku beze zmeny
        }
      }
      if (c >= 'a' && c <= 'z') r[i] = c - 32;
    }
  }

  // pripravi frontu klaves; tbxl jen do logu (TBXL nahrava volajici pres XexLoader)
  void start(const uint8_t *d, size_t n, bool tb, const std::string &nm, bool rychlyTimeout) {
    q.clear(); log.clear(); status.clear();
    tbxl = tb; jmeno = nm; rychlyTimeoutVypnout = rychlyTimeout;
    radku = prazdnych = bezCisla = dlouhych = znakuNelze = klaves = 0;
    dlouhe.clear(); bezCislaUkazka.clear();
    snimku = klid = cekaInv = snimkuPsani = 0;
    bool atascii = false;
    std::vector<std::vector<int>> radky = radkySouboru(d, n, atascii);
    std::vector<Klav> k;
    auto pridej = [&](const std::vector<Klav> &v) { for (const auto &x : v) q.push_back(x); };
    // NEW
    k.clear(); for (int c : {'N', 'E', 'W'}) klavesy(c, k); k.push_back({12, -1}); pridej(k);
    for (auto &r : radky) {
      size_t a = 0; while (a < r.size() && r[a] == ' ') a++;
      size_t b = r.size();
      int uvozovek = 0; for (int c : r) if (c == '"') uvozovek++;
      if (uvozovek % 2 == 0) while (b > a && r[b - 1] == ' ') b--;
      std::vector<int> t(r.begin() + (long)a, r.begin() + (long)b);
      if (t.empty()) { prazdnych++; continue; }
      if (t[0] < '0' || t[0] > '9') {
        bezCisla++;
        if (bezCislaUkazka.size() < 3) {
          std::string s; for (size_t x = 0; x < t.size() && x < 30; x++) s.push_back(t[x] >= 32 && t[x] < 127 ? (char)t[x] : '?');
          bezCislaUkazka.push_back(s);
        }
        continue;
      }
      long cislo = 0; size_t p = 0;
      while (p < t.size() && t[p] >= '0' && t[p] <= '9') { cislo = cislo * 10 + (t[p] - '0'); p++; if (cislo > 99999) break; }
      if (cislo > 32767) { bezCisla++; continue; }
      velkaPismena(t, p);
      k.clear();
      for (int c : t) if (!klavesy(c, k)) znakuNelze++;
      int viditelnych = 0; for (int c : t) if (c >= 0) viditelnych++;
      if (viditelnych > 120) { dlouhych++; if (dlouhe.size() < 8) dlouhe.push_back((int)cislo); continue; }
      k.push_back({12, -1});
      pridej(k);
      radku++;
    }
    klaves = (int)q.size();
    stav = CEKA_READY;
    char b[260];
    std::snprintf(b, sizeof(b), "B299 TXT PROGRAM %s -> %s: %d radku programu, %d stisku klaves (%s soubor); cekam na READY",
                  nm.c_str(), tb ? "TURBO-BASIC XL" : "ATARI BASIC", radku, klaves, atascii ? "ATASCII" : "textovy");
    log.push_back(b);
    status = tb ? "NAČÍTÁM TURBO-BASIC, PAK PÍŠU PROGRAM" : "ATARI BASIC - PAK PÍŠU PROGRAM"; statusMs = 6000;
  }

  // "READY" na radku nad kurzorem textove obrazovky a OS ceka na klavesu
  static bool ready(const Machine &m) {
    if (m.mem.ram[0x57] != 0 || m.mem.ram[0x2FC] != 0xFF) return false;   // DINDEX = textova obrazovka, CH prazdne
    const int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
    const int row = m.mem.ram[0x54];
    if (row < 1 || row > 23) return false;
    static const uint8_t R[5] = {0x32, 0x25, 0x21, 0x24, 0x39};             // READY ve vnitrnim kodu
    const int a = sav + (row - 1) * 40;
    for (int c = 0; c + 5 <= 40; c++) {
      int i = 0;
      while (i < 5 && (m.mem.ram[(a + c + i) & 0xFFFF] & 0x7F) == R[i]) i++;
      if (i == 5) return true;
    }
    return false;
  }

  // vola se na zacatku kazdeho radku obrazu behem psani (PISE)
  void radek(Machine &m) {
    if (stav != PISE || q.empty()) return;
    if (m.mem.ram[0x2FC] != 0xFF) { cekaInv = 0; return; }   // OS predchozi klavesu jeste nevzal
    const Klav k = q.front();
    if (k.inv >= 0 && ((m.mem.ram[0x2B6] & 0x80) != 0) != (k.inv != 0)) {
      // INVFLG prepnout az OS predchozi klavesu cele zpracuje (snimek pockat)
      if (++cekaInv < 312) return;
      m.mem.ram[0x2B6] = k.inv ? 0x80 : 0x00;
    }
    cekaInv = 0;
    m.mem.ram[0x2FC] = k.kod;      // CH - jako klavesnicove preruseni
    m.mem.ram[0x4D] = 0;           // ATRACT (setric obrazovky) vynulovat jako pri stisku
    q.pop_front();
  }

  // vola se po kazdem snimku
  void poSnimku(Machine &m) {
    if (!aktivni()) return;
    snimku++;
    char b[400];
    if (stav == CEKA_READY) {
      klid = ready(m) ? klid + 1 : 0;
      if (klid >= 8) {
        if (rychlyTimeoutVypnout) m.sioRychlyTimeout = false;
        puvLmargn = m.mem.ram[0x52]; puvShflok = m.mem.ram[0x2BE]; puvNoclik = m.mem.ram[0x2DB];
        m.mem.ram[0x52] = 0;       // LMARGN 0: radek az 120 znaku
        m.mem.ram[0x2BE] = 0x00;   // SHFLOK: mala pismena, velka se pisou se SHIFT
        m.mem.ram[0x2DB] = 0xFF;   // NOCLIK: bez klapani klaves (rychleji)
        stav = PISE; snimkuPsani = 0; klid = 0;
        std::snprintf(b, sizeof(b), "B299 TXT PROGRAM: READY po %d snimcich - pisu NEW a %d radku", snimku, radku);
        log.push_back(b);
        status = "PÍŠU PROGRAM: " + std::to_string(radku) + " ŘÁDKŮ"; statusMs = 60000;
      } else if (snimku > 50 * 45) {
        stav = CHYBA;
        log.push_back("B299 TXT PROGRAM: Atari do 45 s neukazalo READY - nic se nepise (bezi hra nebo jiny program?)");
        status = "ATARI NEUKÁZALO READY - NIC NEPÍŠU"; statusMs = 8000;
      }
      return;
    }
    if (stav == PISE) {
      snimkuPsani++;
      if (q.empty()) { stav = DOBEH; klid = 0; }
      return;
    }
    // DOBEH: posledni RETURN si OS vzal (CH = $FF) a BASIC radek zpracoval,
    // pak kontrola programu v pameti
    klid = (m.mem.ram[0x2FC] == 0xFF) ? klid + 1 : 0;
    if (klid >= 15 || ++snimkuPsani > 50 * 600) {
      m.mem.ram[0x52] = puvLmargn; m.mem.ram[0x2BE] = puvShflok; m.mem.ram[0x2DB] = puvNoclik; m.mem.ram[0x2B6] = 0;
      std::vector<int> chyby;
      const int vPameti = napRadkyProgramu(m, &chyby);
      std::string ch;
      for (size_t i = 0; i < chyby.size() && i < 12; i++) ch += (i ? "," : " ") + std::to_string(chyby[i]);
      std::string dl;
      for (size_t i = 0; i < dlouhe.size(); i++) dl += (i ? "," : " ") + std::to_string(dlouhe[i]);
      std::string bc;
      for (size_t i = 0; i < bezCislaUkazka.size(); i++) bc += (i ? " | " : " (napr. ") + bezCislaUkazka[i] + (i + 1 == bezCislaUkazka.size() ? ")" : "");
      std::snprintf(b, sizeof(b), "B299 TXT PROGRAM HOTOVO za %.1f s: napsano %d radku, v pameti %d radku, s chybou syntaxe %d%s; "
                    "preskoceno: prazdnych %d, bez cisla radku %d%s, delsich nez 120 znaku %d%s; znaku, ktere na Atari nejsou: %d",
                    snimkuPsani / 50.0, radku, vPameti, (int)chyby.size(), ch.c_str(), prazdnych, bezCisla, bc.c_str(),
                    dlouhych, dl.c_str(), znakuNelze);
      log.push_back(b);
      if (!chyby.empty()) status = "NAPSÁNO, CHYBA V " + std::to_string(chyby.size()) + " ŘÁDCÍCH (LIST)";
      else status = "HOTOVO " + std::to_string(vPameti) + " ŘÁDKŮ - RUN / CSAVE";
      statusMs = 9000;
      stav = HOTOVO;
    }
  }
};

}  // namespace nap
