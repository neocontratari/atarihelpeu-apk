// test_core.cpp - B292: nove jadro (nap_atari_machine.h) na PC.
//   boot                      - studeny start s BASICem, READY
//   acid <acid800.atr>        - Acid800 (testy hardwaru od autora Altirry)
//   xex <soubor> <snimku> <prefix> [seznam]  - XEX jako v appce, ulozi obraz
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <algorithm>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
#include "nap_atari_pc_klavesnice.h"
#include "syntaz_kazeta.h"
using namespace nap;

static std::string obrazovka(Machine &m) {
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    std::string line;
    for (int c = 0; c < 40; c++) {
      int v = m.peek((uint16_t)(sav + r * 40 + c)) & 0x7F;
      int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v);
      if (a < 32 || a > 126) a = ' ';
      line.push_back((char)a);
    }
    out += line + "\n";
  }
  return out;
}
static std::vector<uint8_t> nacti(const char *f) {
  std::vector<uint8_t> d; FILE *fp = fopen(f, "rb"); if (!fp) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n); fclose(fp); return d;
}
static Machine *novy(bool basic) {
  Machine *m = new Machine();
  m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit();
  m->consol = basic ? 7 : 3;
  m->cpu.reset();
  return m;
}
static void ulozIdx(Machine &m, const std::string &fn) {
  FILE *o = fopen(fn.c_str(), "wb"); if (!o) return;
  fwrite(m.view->idx, 1, AnticView::W * AnticView::H, o); fclose(o);
}
// B296: cely RGB obraz (768x240, 4 body na barevny takt - i VBXE) jako PPM
static void ulozPpm(Machine &m, const std::string &fn) {
  FILE *o = fopen(fn.c_str(), "wb"); if (!o) return;
  fprintf(o, "P6\n%d %d\n255\n", AnticView::FW, AnticView::H);
  for (int i = 0; i < AnticView::FW * AnticView::H; i++) {
    const uint32_t v = m.view->fb[i];
    uint8_t px[3] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF), (uint8_t)((v >> 16) & 0xFF)};
    fwrite(px, 1, 3, o);
  }
  fclose(o);
}
static void vbxeStav(Machine &m, const char *kde) {
  const Vbxe &v = m.vbx;
  int radku = 0; for (int y = 0; y < AnticView::H; y++) radku += m.view->vbxeRadek[y];
  printf("VBXE %s: zapisu %lld, XDL snimku %lld, posledni overlay %d, sirka %d, XDL=$%05X zap=%d, MEMAC ctl=$%02X A=$%02X B=$%02X, "
         "blitu %lld (seznamu %lld), stav blitru %d, IRQ en=%d req=%d, paleta zapisu %lld, radku VBXE %d, extColor %d\n",
         kde, v.nWrites, v.nXdlFrames, v.lastOvMode, v.ovWidth, v.xdlBase, v.xdlEnabled, v.memacCtl, v.memacBankA, v.memacBankB,
         v.nBlits, v.nBlitLists, v.blitState, v.irqEn, v.irqReq, v.nPalWrites, radku, v.extColor);
}

// KLAVESY="snimek:kod,..." - klavesa (kod 0-63, +64 SHIFT, +128 CTRL), START = 1000, FIRE = 1001
// JOY="od-do:maska,..." - joystick 1 drzeny v snimcich od..do (1 nahoru, 2 dolu, 4 vlevo, 8 vpravo, 16 FIRE)
static void vstupy(Machine &m, int f) {
  if (getenv("KLAVESY")) {
    const char *p = getenv("KLAVESY");
    while (*p) {
      int fr = atoi(p); while (*p && *p != ':') p++; if (*p) p++;
      int kod = atoi(p); while (*p && *p != ',') p++; if (*p) p++;
      if (fr == f) { if (kod == 1000) m.consol = 6; else if (kod == 1001) m.trig[0] = 0; else m.klavesa(kod); }
      if (fr + 3 == f && kod == 1001) m.trig[0] = 1;
    }
  }
  if (getenv("JOY")) {
    int maska = 0;
    const char *p = getenv("JOY");
    while (*p) {
      int od = atoi(p); while (*p && *p != '-') p++; if (*p) p++;
      int doo = atoi(p); while (*p && *p != ':') p++; if (*p) p++;
      int mk = atoi(p); while (*p && *p != ',') p++; if (*p) p++;
      if (f >= od && f <= doo) maska |= mk;
    }
    m.porta = 0xF0 | (~maska & 15);
    if (!getenv("KLAVESY") || m.trig[0]) m.trig[0] = (maska & 16) ? 0 : 1;
  }
}

// ---- B298: dlouhy program a zachyceni vystupu LIST (vse, co jde pres E: PUT) ----
static bool g_staryPatch = false;   // B299: true = SIO patch i v RAM pod ROM (chyba do B298)
struct PutHook { int put = -1; bool on = false; std::string out; long long pcPage1 = 0, pcPark = 0; };
static void snimekH(Machine &m, PutHook &h) {
  const long long f = m.frame;
  while (m.frame == f) {
    if (h.on && m.cpu.pc == h.put && !m.cpu.takeNmi && !m.cpu.takeIrq) h.out.push_back((char)m.cpu.a);
    if (m.cpu.pc >= 0x100 && m.cpu.pc < 0x200) { h.pcPage1++; if (m.cpu.pc == 0x100) h.pcPark++; }
    // presne jako Machine::runFrame (SIO patch jen disk, kazeta $60 nikdy)
    // B299: jen se zapnutou ROM OS (g_staryPatch = chovani B298 pro dukaz chyby TBXL + disketa)
    if (m.cpu.pc == 0xE459 && !m.cpu.jam && (g_staryPatch || (m.mem.portB() & 1)) && (m.disk.mounted || m.sioRychlyTimeout) && m.mem.ram[0x300] != 0x60 && !m.cpu.takeNmi && !m.cpu.takeIrq) { m.sioPatch(); continue; }
    m.cpu.step();
  }
  if (m.keyHoldFrames > 0 && --m.keyHoldFrames == 0) m.skstat |= 0x04;
}
static Machine *startBasic(bool tbxl, PutHook &h) {
  Machine *m = novy(!tbxl);
  if (!tbxl) {
    for (int f = 0; f < 400; f++) { snimekH(*m, h); if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
  } else {
    std::vector<uint8_t> d = nacti("../../app/src/main/assets/emu_atari_cpp/turbo_basic_xl.xex");
    XexLoader xl; xl.priprav(d.data(), d.size(), "turbo_basic_xl.xex");
    m->sioRychlyTimeout = true;
    for (int f = 0; f < 1500; f++) {
      if (!xl.aktivni()) m->consol = 7;
      snimekH(*m, h); xl.poSnimku(*m);
      if (f > 200 && obrazovka(*m).find("READY") != std::string::npos) break;
    }
  }
  h.put = (m->peek(0xE406) | (m->peek(0xE407) << 8)) + 1;   // E: PUT (vektor v tabulce editoru $E400)
  return m;
}
static void pisH(Machine &m, PutHook &h, const std::string &t, int dobeh) {
  TypeQueue tq; tq.addText(t);
  for (int f = 0; f < 400000 && !tq.empty(); f++) { tq.step(m); snimekH(m, h); }
  for (int k = 0; k < dobeh; k++) snimekH(m, h);
}
static std::string listH(Machine &m, PutHook &h) {
  h.out.clear(); h.on = true;
  pisH(m, h, "LIST", 0);
  for (int k = 0; k < 50 * 300; k++) { snimekH(m, h); if (h.out.size() > 8 && h.out.rfind("READY") != std::string::npos && h.out.rfind("READY") + 6 >= h.out.size()) break; }
  h.on = false;
  std::string s; for (char c : h.out) s.push_back(c == (char)0x9B ? '\n' : c);
  return s;
}
static std::vector<uint8_t> programPamet(Machine &m) {
  const int od = m.mem.ram[0x82] | (m.mem.ram[0x83] << 8), doo = m.mem.ram[0x8C] | (m.mem.ram[0x8D] << 8);   // VNTP..STARP
  std::vector<uint8_t> v; for (int a = od; a < doo; a++) v.push_back(m.peek((uint16_t)a));
  return v;
}
static std::string dlouhyProgram(bool tbxl, int opak) {
  std::string p; int ln = 10;
  auto L = [&](const std::string &s) { p += std::to_string(ln) + " " + s + "\n"; ln += 10; };
  L("REM *** DLOUHY PROGRAM - TEST KAZETY B298 ***");
  L("DIM A$(120),B(60),M$(30)");
  L("X=10:Y=5:SC=0:LV=1:A$=\"ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\"");
  for (int r = 0; r < opak; r++) {
    const std::string R = std::to_string(r);
    L("REM ---- BLOK " + R + " ----");
    L("FOR I=0 TO 19:B(I)=I*3+" + R + ":NEXT I");
    L("POSITION 2," + std::to_string(r % 20) + ":PRINT \"SKORE: \";SC;\"  LEVEL: \";LV;\"  \";" + std::to_string(r * 7));
    L("IF SC>=" + std::to_string(1000 + r) + " THEN PRINT \"BONUS\":LV=LV+1:SC=SC-500");
    L("SOUND 0," + std::to_string(100 + r) + ",10,8:SOUND 0,0,0,0");
    L("M$=\"N&P EDITION " + R + "\":PRINT M$(1,3);LEN(M$);ASC(M$);CHR$(65+" + std::to_string(r % 20) + ")");
    L("Z=INT(3.75*" + std::to_string(r + 1) + ")+ABS(-" + R + ")+SQR(16)+SIN(0)+COS(0)+ATN(1)*4");
    L("DATA " + R + ",1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25");
    if (tbxl) {
      L("IF X>" + R + ":PRINT \"VELKE X\":ELSE :PRINT \"MALE X\":ENDIF");
      L("REPEAT :X=X+1:UNTIL X>" + std::to_string(20 + r));
      L("WHILE Y<" + std::to_string(10 + r) + ":Y=Y+1:WEND");
      L("DPOKE 1536,DPEEK(88):MOVE ADR(A$),1600,10:Q=$FF&15!16");
      L("PRINT HEX$(" + R + ");\" \";DEC(\"FF\");\" \";INSTR(A$,\"XYZ\");\" \";17 DIV 5;\" \";17 MOD 5");
      L("W=FRAC(3.75)+TRUNC(9.99)+%3+RAND(10):PAUSE 0");
    } else {
      L("IF X>" + R + " THEN PRINT \"VELKE X\"");
      L("X=X+1:IF X<" + std::to_string(20 + r) + " THEN " + std::to_string(ln - 10));
    }
  }
  L("END");
  return p;
}
static std::string normalizuj(const std::string &s) {
  // jen radky programu, mezery sjednotit (TBXL odsazuje bloky, za ENDIF/WEND mezera)
  std::string out, rad;
  auto konec = [&]() {
    std::string r; bool mez = false;
    for (char c : rad) { if (c == ' ') { mez = true; continue; } if (mez && !r.empty()) r.push_back(' '); mez = false; r.push_back(c); }
    if (!r.empty() && r.compare(0, 5, "READY") != 0) out += r + "\n";
    rad.clear();
  };
  for (char c : s) { if (c == '\n') konec(); else rad.push_back(c); }
  konec();
  return out;
}

static PutHook g_hookStat;
// B299: CLOAD ciziho WAV v BASICu / Turbo-BASICu a zachyceny vystup LIST
static std::string cloadList(const std::vector<uint8_t> &wav, bool tbxl, Machine **ven) {
  TapeImage img; std::string err;
  if (!napTapeFromWav(wav.data(), wav.size(), img, err)) return "WAV: " + err;
  PutHook h;
  Machine *b = startBasic(tbxl, h);
  b->tape.img = img; b->tape.loaded = true; b->tape.pos = 0; b->tape.play = true; b->tape.name = "wav";
  h.pcPage1 = h.pcPark = 0;
  TypeQueue tq; tq.addText("CLOAD");
  bool enter = false; int cekej = 0;
  for (int f = 0; f < 50 * 900; f++) {
    tq.step(*b); snimekH(*b, h);
    if (tq.empty() && !enter && ++cekej == 100) { b->klavesa(12); enter = true; cekej = 0; }
    if (enter && !b->motorOn() && ++cekej > 100) {
      const std::string sc = obrazovka(*b);
      if (sc.find("READY", sc.rfind("CLOAD")) != std::string::npos || sc.find("ERROR") != std::string::npos) break;
    }
  }
  std::string l = listH(*b, h);
  if (ven) *ven = b;
  g_hookStat = h;
  return l;
}

int main(int argc, char **argv) {
  std::string mode = argc > 1 ? argv[1] : "boot";
  if (mode == "pc-klavesnice") {
    // B302: ./test_core pc-klavesnice - klavesnice pocitace (prohlizec na TV/PC) -> Atari 130XE:
    // PcKlavesnice (mapovani, rezim PSANI/HRANI, drzene klavesy) + PcPrehravac (tempo pro OS).
    // Po siti muze prijit nekolik klaves NARAZ - tady schvalne vsechny udalosti radku najednou.
    // "naivne" = bez PcPrehravac: kazda udalost hned do stroje (jako kdyby se
    // klavesy predavaly tak, jak prijdou) - dukaz, ze bez tempa se znaky ztraceji
    const bool naivne = argc > 2 && std::string(argv[2]) == "naivne";
    int chyb = 0;
    auto over = [&](bool ok, const std::string &co) { if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); };
    Machine *m = novy(true);
    for (int f = 0; f < 400; f++) { m->runFrame(); if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
    if (naivne) printf("NAIVNE: klavesy jdou do stroje hned, jak prijdou (bez PcPrehravac)\n");
    PcKlavesnice kl;
    PcPrehravac hr;
    int joy = 0, kon = 0;
    std::vector<int> stisky;
    auto ud = [&](const std::string &code, const std::string &znak, int mod, bool dolu) {
      std::vector<PcAkce> a;
      const std::string r = kl.udalost(code, znak, mod, dolu, a);
      for (auto &x : a) {
        if (x.t == PcAkce::JOY) joy = x.v;
        else if (x.t == PcAkce::KONZOLE) kon = x.v;
        else if (!naivne) hr.pridej(x);
        else if (x.t == PcAkce::KLAVESA) { m->klavesa(x.v, true); stisky.push_back(x.v); }
        else if (x.t == PcAkce::PUSTIT) m->klavesaPustena();
        else m->breakKey();
      }
      return r;
    };
    long long snimku = 0;
    auto snimky = [&](int n) { for (int i = 0; i < n; i++) { if (!naivne) hr.krok(*m, &stisky); m->runFrame(); snimku++; } };
    auto dobeh = [&](int navic) { for (int i = 0; i < 3000 && !hr.prazdny(); i++) snimky(1); snimky(navic); };
    // americka klavesnice: znak -> e.code a Shift
    auto usKod = [](char c, int &mod) -> std::string {
      mod = 0;
      if (c >= 'a' && c <= 'z') return std::string("Key") + (char)(c - 'a' + 'A');
      if (c >= 'A' && c <= 'Z') { mod = PcKlavesnice::SHIFT; return std::string("Key") + c; }
      if (c >= '0' && c <= '9') return std::string("Digit") + c;
      switch (c) {
        case ' ': return "Space";
        case '\n': return "Enter";
        case '"': mod = PcKlavesnice::SHIFT; return "Quote";
        case ';': return "Semicolon";
        case ':': mod = PcKlavesnice::SHIFT; return "Semicolon";
        case '+': mod = PcKlavesnice::SHIFT; return "Equal";
        case '=': return "Equal";
        case '-': return "Minus";
        case '*': mod = PcKlavesnice::SHIFT; return "Digit8";
        case '?': mod = PcKlavesnice::SHIFT; return "Slash";
        case ',': return "Comma";
        case '.': return "Period";
        default: return "Unidentified";
      }
    };
    auto napisNaraz = [&](const std::string &t) {
      for (char c : t) {
        int mod; const std::string code = usKod(c, mod);
        const std::string z = c == '\n' ? std::string("Enter") : std::string(1, c);
        ud(code, z, mod, true); ud(code, z, mod, false);
      }
    };
    const int S = PcKlavesnice::SHIFT, C = PcKlavesnice::CTRL, A = PcKlavesnice::ALT, O = PcKlavesnice::OPAKOVANI;
    // 1) radek naraz: velka/mala pismena, uvozovky, strednik, dvojice "00", ? = PRINT
    long long s0 = snimku;
    napisNaraz("10 PRINT \"Ahoj\";100;:? 2*3\n");
    dobeh(30);
    printf("radek 10 (26 klaves naraz) napsany za %lld snimku\n", snimku - s0 - 30);
    // 2) CapsLock = Atari CAPS (mala pismena), pak zpet velka
    over(ud("CapsLock", "CapsLock", 0, true).rfind("OK:CapsLock=60", 0) == 0, "CapsLock = klavesa CAPS (60)");
    over(ud("CapsLock", "CapsLock", O, true) == "DRZENO", "drzeny CapsLock neprepina dokola");
    ud("CapsLock", "CapsLock", 0, false);
    napisNaraz("20 ? \"male\"\n");
    ud("CapsLock", "CapsLock", 0, true); ud("CapsLock", "CapsLock", 0, false);
    dobeh(10);
    // 3) ceska klavesnice: diakritika bez hacku a carek, AltGr (Windows: Ctrl+Alt) neni CONTROL
    {
      napisNaraz("30 REM ");
      ud("KeyP", "P", S, true); ud("KeyP", "P", S, false);
      const char *cz[][2] = {{"Digit5", "\xC5\x99"}, {"Digit9", "\xC3\xAD"}, {"KeyL", "l"}, {"KeyI", "i"}, {"Digit3", "\xC5\xA1"}};
      for (auto &k : cz) { ud(k[0], k[1], 0, true); ud(k[0], k[1], 0, false); }
      napisNaraz(" ");
      over(ud("KeyX", "#", C | A, true) == "OK:KeyX=90", "AltGr+X (#) = SHIFT+3, ne CONTROL");
      ud("KeyX", "#", C | A, false);
      napisNaraz("\n");
    }
    // 4) dvojice a trojice stejne klavesy naraz (OS: KEYDEL)
    napisNaraz("40 REM AABBB\n");
    // 5) Backspace
    napisNaraz("50 ? 7X");
    ud("Backspace", "Backspace", 0, true); ud("Backspace", "Backspace", 0, false);
    napisNaraz("\n");
    // 6) drzena klavesa: opakovani od systemu nic neprida (opakuje OS Atari sam)
    napisNaraz("60 REM ");
    ud("KeyQ", "q", 0, true);
    bool drz = true; for (int i = 0; i < 5; i++) drz = drz && ud("KeyQ", "q", O, true) == "DRZENO";
    over(drz, "opakovani drzene klavesy od systemu = DRZENO");
    ud("KeyQ", "q", 0, false);
    napisNaraz("\n");
    // 7) prekryv: A dolu, B dolu, A nahoru, B nahoru
    napisNaraz("70 REM ");
    ud("KeyA", "a", 0, true); ud("KeyB", "b", 0, true); ud("KeyA", "a", 0, false); ud("KeyB", "b", 0, false);
    napisNaraz("\n");
    s0 = snimku;
    dobeh(30);
    printf("radky 30-70 napsany za %lld snimku\n", snimku - s0 - 30);
    // 7b) normalni psani: klavesa jde do Atari hned v nejblizsim snimku, rychly pisar
    //     (drzi 2 snimky, mezera 1 snimek, dvojice i trojice) nic neztrati
    stisky.clear();
    ud("Digit7", "7", 0, true); snimky(1);
    over(stisky.size() == 1 && stisky[0] == 51, "klavesa jde do Atari hned v nejblizsim snimku (20 ms)");
    snimky(2); ud("Digit7", "7", 0, false); snimky(2);
    for (char c : std::string("5 REM ZZZ 1001 MISSISSIPPI\n")) {
      int mod; const std::string code = usKod(c, mod);
      const std::string z = c == '\n' ? std::string("Enter") : std::string(1, c);
      ud(code, z, mod, true); snimky(2); ud(code, z, mod, false); snimky(1);
    }
    dobeh(20);
    napisNaraz("LIST\n");
    dobeh(150);
    const std::string sc = obrazovka(*m);
    printf("--- LIST ---\n%s---\n", sc.c_str());
    over(sc.find("10 PRINT \"AHOJ\";100;:? 2*3") != std::string::npos, "radek 10 presne (26 klaves naraz, 00, uvozovky, Shift)");
    over(sc.find("20 ? \"male\"") != std::string::npos, "radek 20: CapsLock -> mala pismena");
    over(sc.find("30 REM PRILIS #") != std::string::npos, "radek 30: cesky Příliš -> PRILIS, AltGr # ");
    over(sc.find("40 REM AABBB") != std::string::npos, "radek 40: AA a BBB naraz - zadna klavesa neztracena");
    over(sc.find("50 ? 7 ") != std::string::npos && sc.find("7X") == std::string::npos, "radek 50: Backspace smazal X");
    over(sc.find("60 REM Q") != std::string::npos && sc.find("60 REM QQ") == std::string::npos, "radek 60: drzena klavesa jednou");
    over(sc.find("70 REM AB") != std::string::npos, "radek 70: prekryv A/B - obe klavesy");
    over(sc.find("75 REM ZZZ 1001 MISSISSIPPI") != std::string::npos, "radek 75: rychly pisar (ZZZ, 1001, MISSISSIPPI) - nic neztraceno");
    over(sc.find("ERROR") == std::string::npos, "zadna chyba syntaxe");
    napisNaraz("RUN\n");
    dobeh(100);
    const std::string sr = obrazovka(*m);
    over(sr.find("AHOJ1006") != std::string::npos && sr.find("male") != std::string::npos, "RUN: AHOJ1006 a male");
    // 8) BREAK (prohlizec posila F2 jako "Break")
    napisNaraz("80 GOTO 80\nRUN\n");
    dobeh(50);
    over(ud("Break", "", 0, true) == "BREAK", "F2 = BREAK");
    over(ud("Break", "", O, true) == "DRZENO", "drzene F2 = jen jeden BREAK");
    ud("F2", "F2", 0, false);
    dobeh(50);
    { const std::string sb = obrazovka(*m); over(sb.find("STOPPED") != std::string::npos && sb.find("AT LINE 80") != std::string::npos, "BREAK zastavil program (STOPPED AT LINE 80)"); }
    // 9) sipky = kurzor (CONTROL + - = + *): nahoru na radek 80, prepsat na 90 GOTO 90
    {
      napisNaraz("LIST 80\n");
      dobeh(60);
      // kurzor je pod READY; nahoru na radek "80 GOTO 80" (ROWCRS $54 - radek na obrazovce)
      const std::string s8 = obrazovka(*m);
      int r80 = -1, r = 0;
      for (size_t p = 0, q; (q = s8.find('\n', p)) != std::string::npos; p = q + 1, r++)
        if (q - p >= 12 && s8.compare(p, 12, "  80 GOTO 80") == 0) r80 = r;
      const int nahoru = r80 >= 0 ? m->mem.ram[0x54] - r80 : 0;
      printf("radek 80 na obrazovce %d, kurzor %d, COLCRS %d -> %dx nahoru\n", r80, m->mem.ram[0x54], m->mem.ram[0x55], nahoru);
      for (int i = 0; i < nahoru; i++) { ud("ArrowUp", "ArrowUp", 0, true); ud("ArrowUp", "ArrowUp", 0, false); }
      napisNaraz("9");
      for (int i = 0; i < 7; i++) { ud("ArrowRight", "ArrowRight", 0, true); ud("ArrowRight", "ArrowRight", 0, false); }
      napisNaraz("9\n");
      dobeh(30);
      napisNaraz("LIST 80,90\n");
      dobeh(60);
      const std::string sl = obrazovka(*m);
      printf("--- sipky ---\n%s---\n", sl.c_str());
      over(sl.find("90 GOTO 90") != std::string::npos, "sipky nahoru/vpravo = kurzor, radek 80 prepsan na 90 GOTO 90");
    }
    // 10) HRANI: WASD/sipky = joystick, K/mezernik = skok, L = FIRE
    over(ud("F9", "F9", 0, true) == "HRANI", "F9 = HRANI");
    over(ud("F9", "F9", O, true) == "DRZENO", "drzene F9 neprepina dokola");
    ud("F9", "F9", 0, false);
    over(ud("KeyW", "w", 0, true) == "SMER:KeyW" && joy == 1, "W = joystick nahoru");
    snimky(5);
    over(m->kbcode == 46 && !(m->skstat & 4), "W je i klavesa W drzena (PEEK(764) v BASICu)");
    over(ud("KeyL", "l", 0, true) == "VYSTREL" && joy == 17, "L = FIRE (s drzenym W)");
    over(ud("ArrowLeft", "ArrowLeft", 0, true) == "SMER:ArrowLeft" && joy == 21, "sipka vlevo = joystick vlevo (diagonala)");
    ud("KeyW", "w", 0, false);
    snimky(5);
    over(joy == 20 && (m->skstat & 4), "W pusteno: joystick bez nahoru, klavesa pustena");
    ud("KeyL", "l", 0, false); ud("ArrowLeft", "ArrowLeft", 0, false);
    over(joy == 0, "vse pusteno: joystick v klidu");
    over(ud("Space", " ", 0, true) == "SKOK" && joy == 1, "mezernik = skok (nahoru + MEZERA)");
    ud("Space", " ", 0, false);
    over(joy == 0, "mezernik pusten");
    over(ud("F9", "F9", 0, true) == "PSANI", "F9 = zpet PSANI");
    ud("F9", "F9", 0, false);
    // 11) START / SELECT / OPTION drzene jako na skrini
    over(ud("F1", "F1", 0, true) == "START" && kon == 1, "F1 = START drzeny");
    over(ud("F4", "F4", 0, true) == "OPTION" && kon == 5, "F4 = OPTION (s drzenym START)");
    ud("F1", "F1", 0, false);
    over(kon == 4, "F1 pusteno, OPTION drzi");
    ud("F4", "F4", 0, false);
    over(kon == 0, "konzole pustena");
    // 12) ostatni mapovani
    over(ud("KeyC", "c", C, true) == "OK:KeyC=146", "Ctrl+C = CONTROL+C (graficky znak)"); ud("KeyC", "c", C, false);
    over(ud("KeyV", "@", C | A, true) == "OK:KeyV=117", "AltGr+V (@) = SHIFT+8"); ud("KeyV", "@", C | A, false);
    over(ud("Delete", "Delete", 0, true) == "OK:Delete=180", "Delete = DELETE (CONTROL+BACK S)"); ud("Delete", "Delete", 0, false);
    over(ud("Insert", "Insert", S, true) == "OK:Insert=119", "Shift+Insert = vlozit radek (SHIFT+>)"); ud("Insert", "Insert", S, false);
    over(ud("Home", "Home", 0, true) == "OK:Home=118", "Home = CLEAR (SHIFT+<)"); ud("Home", "Home", 0, false);
    over(ud("F6", "F6", 0, true) == "OK:F6=17", "F6 = HELP"); ud("F6", "F6", 0, false);
    over(ud("F8", "F8", 0, true) == "OK:F8=39", "F8 = INVERZE (logo Atari)"); ud("F8", "F8", 0, false);
    over(ud("Quote", "\xC2\xA7", 0, true).rfind("NEZNAMA:Quote", 0) == 0, "paragraf (Atari ho nema) = NEZNAMA, nic se nenapise");
    over(ud("ShiftLeft", "Shift", 0, true) == "NIC", "Shift sam nic nepise");
    over(ud("KeyE", "Unidentified", 0, true) == "OK:KeyE=42", "klavesa bez znaku -> podle mista (E)"); ud("KeyE", "Unidentified", 0, false);
    // 12b) hlidani spojeni: drzena klavesa a prohlizec se 2,5 s neozve -> vse pustit
    {
      kl.zprava(10000);
      ud("KeyM", "m", 0, true); ud("F4", "F4", 0, true);
      std::vector<PcAkce> a1, a2;
      const bool drzi = !kl.hlidej(12400, a1) && a1.empty();          // 2,4 s ticho - jeste drzi
      kl.zprava(12400);                                                 // "Zije" od prohlizece
      const bool drzi2 = !kl.hlidej(14800, a2);                         // 2,4 s od "Zije"
      std::vector<PcAkce> a3;
      const bool pustil = kl.hlidej(15000, a3);                         // 2,6 s ticho
      bool m = false, k0 = false;
      for (auto &x : a3) { if (x.t == PcAkce::PUSTIT && x.v == 37) m = true; if (x.t == PcAkce::KONZOLE && x.v == 0) k0 = true; }
      over(drzi && drzi2 && pustil && m && k0 && kl.drzene.empty(), "hlidani spojeni: 2,5 s ticho -> pusteno (M i OPTION), \"Zije\" drzeni prodlouzi");
      for (auto &x : a3) { if (x.t == PcAkce::KONZOLE) kon = x.v; else if (x.t != PcAkce::JOY) hr.pridej(x); }
      std::vector<PcAkce> a4;
      over(!kl.hlidej(99999, a4) && a4.empty(), "hlidani spojeni: bez drzene klavesy nic");
      dobeh(5);
    }
    // 13) PustVse (okno prohlizece ztratilo zamereni): vse pustit
    ud("KeyZ", "z", 0, true); ud("F3", "F3", 0, true);
    snimky(5);
    over(kon == 2 && !(m->skstat & 4), "Z a SELECT drzene");
    over(ud("PustVse", "", 0, false) == "PUSTENO_VSE" && kon == 0 && joy == 0, "PustVse = konzole a joystick pusteny");
    dobeh(5);
    over((m->skstat & 4) != 0, "PustVse = klavesa Z pustena");
    printf("pc-klavesnice: chyb %d\n", chyb);
    return chyb ? 1 : 0;
  }
  if (mode == "kazeta-run-list") {
    // B299: CLOAD (TBXL), RUN, po N snimcich BREAK nebo RESET, pak LIST
    std::vector<uint8_t> wav = nacti(argv[2]);
    const std::string jak = argc > 3 ? argv[3] : "break";
    const int snimku = argc > 4 ? atoi(argv[4]) : 500;
    Machine *b = nullptr;
    cloadList(wav, true, &b);   // CLOAD + LIST (ok)
    PutHook h; h.put = (b->peek(0xE406) | (b->peek(0xE407) << 8)) + 1;
    pisH(*b, h, "RUN", 0);
    for (int k = 0; k < snimku; k++) snimekH(*b, h);
    if (jak == "reset") { b->reset(); b->consol = 7; } else b->breakKey();
    for (int k = 0; k < 200; k++) snimekH(*b, h);
    std::string l = listH(*b, h);
    if (argc > 5) { FILE *o = fopen(argv[5], "wb"); fputs(l.c_str(), o); fclose(o); }
    printf("po RUN + %s: LIST %zu znaku, radku %d\n%s", jak.c_str(), l.size(), (int)std::count(l.begin(), l.end(), '\n'), obrazovka(*b).c_str());
    return 0;
  }
  if (mode == "pokey-casovac") {
    // B300: ./test_core pokey-casovac - perioda preruseni casovace 1 (8 bitu) pro AUDF 0..255
    // na 1,79 MHz (AUDF+4 cyklu) a na 64 kHz ((AUDF+1)*28). Do B299 AUDF=$FF (256) v uint8_t
    // = 0 -> casovac uz nikdy nevystrelil (Ghostbusters: hudba z preruseni, zustal na titulce).
    int chyb = 0;
    for (int rychly = 1; rychly >= 0; rychly--) {
      for (int audf : {0, 1, 0x7F, 0xFE, 0xFF}) {
        Machine *m = novy(true);
        m->pokeyWrite(0x0F, 0x03);                  // SKCTL: normalni rezim (bezi delicky 64/15 kHz)
        m->pokeyWrite(0x08, rychly ? 0x40 : 0x00);  // AUDCTL: kanal 1 na 1,79 MHz / 64 kHz
        m->pokeyWrite(0x00, (uint8_t)audf);         // AUDF1
        m->pokeyWrite(0x0E, 0x01);                  // IRQEN: casovac 1
        m->pokeyWrite(0x09, 0x00);                  // STIMER
        long long t = 0, posl = -1, perioda = -1; int vystrelu = 0;
        const long long limit = rychly ? 5000 : 200000;
        for (; t < limit && vystrelu < 4; t++) {
          m->cyc++;
          m->pokeyTick();
          if (!(m->irqst & 0x01)) {               // preruseni casovace 1
            if (posl >= 0) perioda = t - posl;
            posl = t; vystrelu++;
            m->pokeyWrite(0x0E, 0x00); m->pokeyWrite(0x0E, 0x01);   // potvrdit jako obsluha
          }
        }
        const long long ocek = rychly ? audf + 4 : (long long)(audf + 1) * 28;
        const bool ok = vystrelu >= 4 && perioda == ocek;
        if (!ok) chyb++;
        printf("%s  AUDCTL=$%02X AUDF1=$%02X: preruseni %d, perioda %lld cyklu (ocekavano %lld)\n", ok ? "OK   " : "CHYBA",
               rychly ? 0x40 : 0, audf, vystrelu, perioda, ocek);
        delete m;
      }
    }
    printf("pokey-casovac: chyb %d\n", chyb);
    return chyb ? 1 : 0;
  }
  if (mode == "txt-program") {
    // B299: ./test_core txt-program soubor.txt|zkouska basic|tbxl [vystup_list.txt]
    // TXT soubor -> klavesy do Atari (nap::PsaniProgramu jako v appce), pak LIST a srovnani
    const bool tbxl = argc > 3 && std::string(argv[3]) == "tbxl";
    std::string text;
    const bool zkouska = std::string(argv[2]) == "zkouska";
    if (zkouska) {
      // schvalne zlobive veci: mala pismena, CRLF, prazdne radky, radek bez cisla, retezce s malymi
      // pismeny a ridicimi znaky, REM s malymi pismeny, dlouhy radek (>120), chyba syntaxe
      text = "Program pro zkousku B299\r\n"
             "10 rem Tady je komentar s malymi pismeny\r\n"
             "\r\n"
             "20 dim a$(40):a$=\"Ahoj Atari 130xe!\":print \"}\";a$\r\n"
             "30 for i=1 to 3:print i;\" \";:next i:print\r\n"
             "   40 PRINT \"PRVNI \",\"druhy\";\"|{`~\"\r\n"
             "50 x=5:IF X>3 THEN PRINT \"VETSI NEZ 3\"\r\n"
             "60 PRIMT \"tady je chyba\"\r\n"
             "70 data abc,Def,12\r\n"
             "80 read b$:print b$\r\n";
      text += "90 PRINT \"" + std::string(130, 'X') + "\"\r\n";
      text += "100 END";
      if (tbxl) text += "\r\n110 TEXT 20,25,\"Tbxl\":DPOKE 1536,1234:? DPEEK(1536)";
    } else {
      std::vector<uint8_t> v = nacti(argv[2]);
      text.assign(v.begin(), v.end());
    }
    PutHook h;
    Machine *m;
    if (tbxl) m = startBasic(true, h);
    else {
      m = novy(true);
      m->sioRychlyTimeout = true;
      h.put = -1;
    }
    PsaniProgramu ps;
    ps.start((const uint8_t *)text.data(), text.size(), tbxl, zkouska ? "zkouska.txt" : argv[2], !tbxl);
    int f = 0;
    for (; f < 50 * 900 && (ps.aktivni() || f < 2); f++) {
      if (ps.pise()) m->runFrameRadky([&](Machine &mm) { ps.radek(mm); });
      else snimekH(*m, h);
      ps.poSnimku(*m);
    }
    for (auto &s : ps.log) printf("%s\n", s.c_str());
    h.put = (m->peek(0xE406) | (m->peek(0xE407) << 8)) + 1;
    const std::string l = listH(*m, h);
    if (argc > 4) { FILE *o = fopen(argv[4], "wb"); fputs(l.c_str(), o); fclose(o); }
    printf("--- LIST (%zu znaku) ---\n%s", l.size(), l.size() < 4000 ? l.c_str() : (l.substr(0, 1500) + "\n...\n").c_str());
    printf("%s", obrazovka(*m).c_str());
    printf("LMARGN=%d SHFLOK=$%02X NOCLIK=$%02X INVFLG=$%02X, snimku %d, stav %d\n", m->mem.ram[0x52], m->mem.ram[0x2BE],
           m->mem.ram[0x2DB], m->mem.ram[0x2B6], f, (int)ps.stav);
    if (!zkouska) {
      // srovnat s puvodnim textem (radky s cislem, mezery sjednocene, mala pismena mimo uvozovky velka)
      const std::string a = normalizuj(l);
      std::string ocek; { std::string t2; for (char c : text) if (c != '\r') t2.push_back(c); ocek = normalizuj(t2); }
      const bool shoda = a == ocek;
      printf("txt-program: LIST %s s puvodnim souborem (%zu / %zu znaku)\n", shoda ? "SOUHLASI" : "NESOUHLASI", a.size(), ocek.size());
      if (!shoda) {
        size_t d = 0; while (d < a.size() && d < ocek.size() && a[d] == ocek[d]) d++;
        printf("rozdil na znaku %zu:\n--- ocekavano ---\n%.200s\n--- LIST ---\n%.200s\n", d, ocek.substr(d > 60 ? d - 60 : 0).c_str(), a.substr(d > 60 ? d - 60 : 0).c_str());
      }
      return shoda && ps.stav == PsaniProgramu::HOTOVO ? 0 : 1;
    }
    return ps.stav == PsaniProgramu::HOTOVO ? 0 : 1;
  }
  if (mode == "tbxl-disketa") {
    // B299: ./test_core tbxl-disketa [stary] - Turbo-BASIC XL a disketa v D1: (DDEVIC $31).
    // TBXL bezi s vypnutou ROM a na $E459 (v RAM pod ROM) ma vlastni kod; SIO patch
    // ho do B298 "prepadal" -> LIST/RUN rozsypane. "stary" = chovani B298 (dukaz chyby).
    g_staryPatch = argc > 2 && std::string(argv[2]) == "stary";
    PutHook h;
    Machine *m = startBasic(true, h);
    std::vector<uint8_t> atr(16 + 720 * 128, 0);
    atr[0] = 0x96; atr[1] = 0x02; const int par = 720 * 128 / 16;
    atr[2] = par & 0xFF; atr[3] = (par >> 8) & 0xFF; atr[4] = 128; atr[6] = (par >> 16) & 0xFF;
    m->disk.load(atr.data(), atr.size(), "prazdna.atr");
    printf("TBXL: DDEVIC=$%02X, disketa v D1: %s, PORTB=$%02X (%s)\n", m->mem.ram[0x300], m->disk.mounted ? "ano" : "ne",
           m->mem.portB(), g_staryPatch ? "STARY patch jako B298" : "patch jen se zapnutou ROM (B299)");
    auto pis = [&](const std::string &t, int dobeh) {     // jako pisH, ale s limitem (rozsypany TBXL klavesy nebere)
      TypeQueue tq; tq.addText(t);
      for (int f = 0; f < 3000 && !tq.empty(); f++) { tq.step(*m); snimekH(*m, h); }
      for (int k = 0; k < dobeh; k++) snimekH(*m, h);
    };
    pis("10 A=5:IF A>3 THEN PRINT \"VELKE\";A*2\n20 FOR I=1 TO 3:PRINT I*10;:NEXT I:PRINT\n30 TEXT 20,25,\"BODY\":DIM B$(5):B$=\"OK\":PRINT B$", 100);
    h.out.clear(); h.on = true;
    pis("LIST", 400);
    h.on = false;
    std::string l; for (char c : h.out) l.push_back(c == (char)0x9B ? '\n' : c);
    h.out.clear(); h.on = true;
    pis("RUN", 300);
    h.on = false;
    std::string r; for (char c : h.out) r.push_back(c == (char)0x9B ? '\n' : c);
    printf("--- LIST ---\n%s--- RUN ---\n%s\n", l.c_str(), r.c_str());
    const bool okL = l.find("10 A=5:IF A>3 THEN PRINT \"VELKE\";A*2") != std::string::npos && l.find("20 FOR I=1 TO 3:PRINT I*10;:NEXT I:PRINT") != std::string::npos;
    const bool okR = r.find("VELKE10") != std::string::npos && r.find("102030") != std::string::npos && r.find("\nOK") != std::string::npos && r.find("ERROR") == std::string::npos;
    printf("tbxl-disketa: LIST %s, RUN %s, zkratek SIO %lld\n", okL ? "OK" : "CHYBA", okR ? "OK" : "CHYBA", m->sioZkratek + m->sioPrikazu);
    return okL && okR ? 0 : 1;
  }
  if (mode == "kazeta-fuzz-cload") {
    // B299: ./test_core kazeta-fuzz-cload program.wav pokusu seed vzor.txt - cely postup jako na telefonu
    // (TBXL, CLOAD napsany v nahodnem okamziku, nahodne drzeni klaves, kazeta vlozena az po pipnuti,
    // RETURN, nahravani, LIST) a srovnani s vypisem vzor.txt
    std::vector<uint8_t> wav = nacti(argv[2]);
    const int pokusu = argc > 3 ? atoi(argv[3]) : 5;
    unsigned long long seed = argc > 4 ? strtoull(argv[4], nullptr, 10) : 1;
    std::string vzor; { std::vector<uint8_t> v = nacti(argv[5]); vzor.assign(v.begin(), v.end()); }
    TapeImage img; std::string err;
    if (!napTapeFromWav(wav.data(), wav.size(), img, err)) { printf("WAV: %s\n", err.c_str()); return 1; }
    auto rnd = [&](int n) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return (int)((seed >> 33) % (unsigned long long)n); };
    int chyb = 0;
    for (int t = 0; t < pokusu; t++) {
      PutHook h;
      Machine *m = startBasic(true, h);
      const int put = h.put;
      std::string out; bool zapis = false;
      auto krok = [&](int snimku) {
        for (int s = 0; s < snimku; s++) {
          const long long fr = m->frame;
          while (m->frame == fr) {
            if (zapis && m->cpu.pc == put && !m->cpu.takeNmi && !m->cpu.takeIrq) out.push_back((char)m->cpu.a);
            m->cpu.step();
          }
          if (m->keyHoldFrames > 0 && --m->keyHoldFrames == 0) m->skstat |= 0x04;
        }
      };
      auto klav = [&](int kod, int drz) { m->klavesa(kod, true); krok(drz); m->klavesaPustena(); };
      krok(rnd(300));
      for (int c = rnd(30000); c > 0; c--) m->cpu.step();
      const int cload[5] = {18 /*C*/, 0 /*L*/, 8 /*O*/, 63 /*A*/, 58 /*D*/};
      for (int k = 0; k < 5; k++) { krok(2 + rnd(25)); for (int c = rnd(30000); c > 0; c--) m->cpu.step(); klav(cload[k], 1 + rnd(15)); }
      krok(2 + rnd(25)); klav(12, 1 + rnd(150));                       // RETURN (CLOAD)
      krok(50 + rnd(400));                                             // pipnuti, EJECT, vyber kazety
      m->tape.img = img; m->tape.loaded = true; m->tape.pos = 0; m->tape.play = true; m->tape.name = "wav";
      krok(rnd(100)); for (int c = rnd(30000); c > 0; c--) m->cpu.step();
      klav(12, 1 + rnd(150));                                          // RETURN po pipnuti -> motor
      bool motor = false;
      for (int f = 0; f < 50 * 200; f++) { krok(1); if (m->motorOn()) motor = true; if (motor && !m->motorOn()) break; }
      krok(100 + rnd(300));
      m->tape.play = false;                                            // STOP
      krok(rnd(200)); for (int c = rnd(30000); c > 0; c--) m->cpu.step();
      const int kody[4] = {0x00, 0x0D, 0x3E, 0x2D};
      for (int k = 0; k < 4; k++) { krok(2 + rnd(25)); klav(kody[k], 1 + rnd(15)); }
      krok(2 + rnd(25)); zapis = true; klav(12, 1 + rnd(150));
      for (int f = 0; f < 50 * 40; f++) { krok(1); const size_t r = out.rfind("READY"); if (r != std::string::npos && r > 10) break; }
      std::string s; for (char c : out) s.push_back(c == (char)0x9B ? '\n' : c);
      const size_t z1 = s.find("1 GOTO"), z2 = vzor.find("1 GOTO");
      const std::string a = z1 == std::string::npos ? s : s.substr(z1);
      const std::string e = z2 == std::string::npos ? vzor : vzor.substr(z2);
      const size_t n = std::min(a.size(), e.size());
      const bool ok = a.substr(0, n) == e.substr(0, n) && a.size() + 2 >= e.size();
      printf("POKUS %d: %s (motor %s, LIST %zu znaku)\n", t, ok ? "OK" : "ROZDIL", motor ? "jel" : "NEJEL", a.size());
      if (!ok) {
        chyb++;
        size_t d = 0; while (d < a.size() && d < e.size() && a[d] == e[d]) d++;
        printf("--- ocekavano od znaku %zu ---\n%.200s\n--- dostal ---\n%.300s\n", d, e.substr(d > 80 ? d - 80 : 0).c_str(), a.substr(d > 80 ? d - 80 : 0).c_str());
      }
      fflush(stdout);
      delete m;
    }
    printf("kazeta-fuzz-cload: %d pokusu, chyb %d\n", pokusu, chyb);
    return chyb ? 1 : 0;
  }
  if (mode == "kazeta-fuzz") {
    // B299: ./test_core kazeta-fuzz program.wav pokusu [seed] - stav po CLOAD (TBXL), pak LIST
    // v nahodnem okamziku a s nahodnym drzenim klaves (jako prst na displeji telefonu)
    std::vector<uint8_t> wav = nacti(argv[2]);
    const int pokusu = argc > 3 ? atoi(argv[3]) : 100;
    unsigned long long seed = argc > 4 ? strtoull(argv[4], nullptr, 10) : 1;
    Machine *b = nullptr;
    const std::string vzor = cloadList(wav, true, &b);
    const int put = (b->peek(0xE406) | (b->peek(0xE407) << 8)) + 1;
    auto rnd = [&](int n) { seed = seed * 6364136223846793005ULL + 1442695040888963407ULL; return (int)((seed >> 33) % (unsigned long long)n); };
    int chyb = 0;
    for (int t = 0; t < pokusu; t++) {
      Machine *m = new Machine(*b); m->cpu.bus = m;
      std::string out; bool zapis = false;
      auto krok = [&](int snimku) {
        for (int s = 0; s < snimku; s++) {
          const long long fr = m->frame;
          while (m->frame == fr) {
            if (zapis && m->cpu.pc == put && !m->cpu.takeNmi && !m->cpu.takeIrq) out.push_back((char)m->cpu.a);
            m->cpu.step();
          }
          if (m->keyHoldFrames > 0 && --m->keyHoldFrames == 0) m->skstat |= 0x04;
        }
      };
      // nahodna faze: 0..1 s po READY, pak jeste nahodny pocet cyklu
      krok(rnd(50));
      for (int c = rnd(30000); c > 0; c--) m->cpu.step();
      const int kody[5] = {0x00 /*L*/, 0x0D /*I*/, 0x3E /*S*/, 0x2D /*T*/, 12 /*RETURN*/};
      for (int k = 0; k < 5; k++) {
        krok(2 + rnd(25));
        for (int c = rnd(30000); c > 0; c--) m->cpu.step();
        if (k == 4) zapis = true;
        m->klavesa(kody[k], true);
        krok(1 + rnd(k == 4 ? 150 : 15));             // RETURN nekdy dlouho (opakovani klavesy)
        m->klavesaPustena();
      }
      for (int f = 0; f < 50 * 40; f++) {
        krok(1);
        const size_t r = out.rfind("READY");
        if (r != std::string::npos && r > 10) break;
      }
      std::string s; for (char c : out) s.push_back(c == (char)0x9B ? '\n' : c);
      // porovnat od prvniho cisla radku
      const size_t z1 = s.find("1 GOTO"), z2 = vzor.find("1 GOTO");
      const std::string a = z1 == std::string::npos ? s : s.substr(z1);
      const std::string e = z2 == std::string::npos ? vzor : vzor.substr(z2);
      const bool ok = a.substr(0, std::min(a.size(), e.size())) == e.substr(0, std::min(a.size(), e.size())) && a.size() + 2 >= e.size();
      if (!ok) {
        chyb++;
        size_t d = 0; while (d < a.size() && d < e.size() && a[d] == e[d]) d++;
        printf("POKUS %d: ROZDIL na znaku %zu (delka %zu / %zu)\n--- ocekavano ---\n%.200s\n--- dostal ---\n%.300s\n", t, d, a.size(), e.size(),
               e.substr(d > 80 ? d - 80 : 0).c_str(), a.substr(d > 80 ? d - 80 : 0).c_str());
        if (chyb > 3) break;
      }
      delete m;
    }
    printf("kazeta-fuzz: %d pokusu, chyb %d\n", pokusu, chyb);
    return chyb ? 1 : 0;
  }
  if (mode == "kazeta-trace") {
    // B299: ./test_core kazeta-trace program.wav "LIST 1000" - kudy jde TBXL pri LIST (adresy mimo ROM OS, volani E: PUT)
    std::vector<uint8_t> wav = nacti(argv[2]);
    Machine *b = nullptr;
    cloadList(wav, true, &b);
    const std::string prikaz = argc > 3 ? argv[3] : "LIST 1000";
    const int put = (b->peek(0xE406) | (b->peek(0xE407) << 8)) + 1;
    std::map<int, long long> pcs;
    std::string out;
    TypeQueue tq; tq.addText(prikaz);
    bool zapis = false;
    for (int f = 0; f < 3000; f++) {
      tq.step(*b);
      const long long fr = b->frame;
      while (b->frame == fr) {
        const int pc = b->cpu.pc;
        if (tq.empty() && !zapis) zapis = true;
        if (zapis) {
          const bool rom = (b->mem.portB() & 1) && pc >= 0xC000 && !(pc >= 0xD000 && pc < 0xD800);
          if (!rom && !(pc >= 0x5000 && pc < 0x5800 && !(b->mem.portB() & 0x80))) pcs[pc]++;
          if (pc == put && !b->cpu.takeNmi && !b->cpu.takeIrq) {
            char t[200]; int s = b->cpu.s;
            std::string st;
            for (int k = 1; k <= 12 && s + k <= 0xFF; k++) { snprintf(t, sizeof t, "%02X ", b->mem.ram[0x100 + s + k]); st += t; }
            snprintf(t, sizeof t, "PUT %02X '%c' S=%02X stack: %s\n", b->cpu.a, (b->cpu.a >= 32 && b->cpu.a < 127) ? b->cpu.a : '.', s, st.c_str());
            out += t;
          }
        }
        b->cpu.step();
      }
      if (b->keyHoldFrames > 0 && --b->keyHoldFrames == 0) b->skstat |= 0x04;
      if (zapis && f > 50 && obrazovka(*b).rfind("READY") != std::string::npos && obrazovka(*b).rfind("READY") > obrazovka(*b).rfind(prikaz)) break;
    }
    printf("%s", out.c_str());
    printf("--- PC mimo ROM OS (pocet) ---\n");
    for (auto &kv : pcs) printf("%04X %lld\n", kv.first, kv.second);
    printf("%s", obrazovka(*b).c_str());
    return 0;
  }
  if (mode == "kazeta-diag") {
    // B299: ./test_core kazeta-diag program.wav basic|tbxl - CLOAD + LIST a radky "B299 PAMET" (jako LOG/CHYBA
    // v appce) - cisla pro srovnani s logem z telefonu
    std::vector<uint8_t> wav = nacti(argv[2]);
    const bool tbxl = argc > 3 && std::string(argv[3]) == "tbxl";
    Machine *b = nullptr;
    std::string l = cloadList(wav, tbxl, &b);
    printf("LIST %zu znaku\n%s\n", l.size(), napDiagPameti(*b).c_str());
    return 0;
  }
  if (mode == "kazeta-dump") {
    // B299: ./test_core kazeta-dump program.wav basic|tbxl ram.bin - CLOAD, LIST a vypis RAM (64 kB vcetne RAM pod ROM)
    std::vector<uint8_t> wav = nacti(argv[2]);
    const bool tbxl = argc > 3 && std::string(argv[3]) == "tbxl";
    Machine *b = nullptr;
    std::string l = cloadList(wav, tbxl, &b);
    if (argc > 4) { FILE *o = fopen(argv[4], "wb"); fwrite(b->mem.ram, 1, 65536, o); fclose(o); }
    printf("LIST %zu znaku; LOMEM $%04X VNTP $%04X VNTD $%04X VVTP $%04X STMTAB $%04X STMCUR $%04X STARP $%04X MEMTOP $%04X PORTB $%02X\n",
           l.size(), b->peek(0x80) | b->peek(0x81) << 8, b->peek(0x82) | b->peek(0x83) << 8, b->peek(0x84) | b->peek(0x85) << 8,
           b->peek(0x86) | b->peek(0x87) << 8, b->peek(0x88) | b->peek(0x89) << 8, b->peek(0x8A) | b->peek(0x8B) << 8,
           b->peek(0x8C) | b->peek(0x8D) << 8, b->peek(0x90) | b->peek(0x91) << 8, b->mem.portB());
    return 0;
  }
  if (mode == "kazeta-list") {
    // B299: ./test_core kazeta-list program.wav basic|tbxl vystup.txt - CLOAD a LIST (cely vystup E:)
    std::vector<uint8_t> wav = nacti(argv[2]);
    const bool tbxl = argc > 3 && std::string(argv[3]) == "tbxl";
    Machine *b = nullptr;
    std::string l = cloadList(wav, tbxl, &b);
    if (argc > 4) { FILE *o = fopen(argv[4], "wb"); fputs(l.c_str(), o); fclose(o); }
    printf("%s: LIST %zu znaku, radku %d\n%s", tbxl ? "TURBO-BASIC XL" : "ATARI BASIC", l.size(), (int)std::count(l.begin(), l.end(), '\n'), b ? obrazovka(*b).c_str() : "");
    printf("instrukci v page 1: %lld, z toho PC=$0100: %lld\n", g_hookStat.pcPage1, g_hookStat.pcPark);
    return 0;
  }
  if (mode == "kazeta-dlouha") {
    // B298: ./test_core kazeta-dlouha [basic|tbxl] [bloku]
    // dlouhy program se napise na klavesnici -> LIST (porovnani se zdrojem) ->
    // CSAVE -> WAV -> novy stroj -> CLOAD -> pamet programu a LIST musi souhlasit
    const bool tbxl = argc > 2 && std::string(argv[2]) == "tbxl";
    const int opak = argc > 3 ? atoi(argv[3]) : 10;
    PutHook h;
    Machine *m = startBasic(tbxl, h);
    const std::string src = dlouhyProgram(tbxl, opak);
    pisH(*m, h, src, 100);
    const std::vector<uint8_t> pa = programPamet(*m);
    const std::string listA = listH(*m, h);
    const bool listOk = normalizuj(listA) == normalizuj(src);
    printf("%s: program %zu znaku, v pameti %zu B, LIST %zu znaku - %s se zdrojem\n", tbxl ? "TURBO-BASIC XL" : "ATARI BASIC",
           src.size(), pa.size(), listA.size(), listOk ? "SOUHLASI" : "NESOUHLASI");
    m->tapeCapture = true;
    CsaveRecorder rec;
    {
      float t[882]; m->genTape(t, 882);
      TypeQueue tq; tq.addText("CSAVE");
      bool enterPoslan = false; int poBeep = 0;
      for (int f = 0; f < 50 * 600 && !rec.maHotovo; f++) {
        tq.step(*m); snimekH(*m, h);
        m->genTape(t, 882);
        for (int i = 0; i < 882; i++) t[i] *= 0.8f;
        rec.snimek(*m, t, 882);
        if (tq.empty() && !enterPoslan && ++poBeep == 150) { m->klavesa(12); enterPoslan = true; }
      }
    }
    if (!rec.maHotovo) { printf("CSAVE: WAV nevznikl\n"); return 5; }
    std::vector<uint8_t> wav;
    {
      const uint32_t nb = (uint32_t)rec.hotovo.size() * 2;
      uint8_t hd[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
      uint32_t r = 36 + nb; std::memcpy(hd + 4, &r, 4); std::memcpy(hd + 40, &nb, 4);
      wav.assign(hd, hd + 44);
      for (int16_t v : rec.hotovo) { wav.push_back((uint8_t)(v & 0xFF)); wav.push_back((uint8_t)((v >> 8) & 0xFF)); }
    }
    TapeImage img; std::string err;
    if (!napTapeFromWav(wav.data(), wav.size(), img, err)) { printf("WAV: %s\n", err.c_str()); return 6; }
    printf("CSAVE -> WAV %.1f s: %lld zaznamu (kontrolni soucet OK %lld, spatne %lld), chyb ramce v zaznamech %lld, sum v mezerach %lld, druh: %s\n",
           img.seconds(), img.records600, img.recordsOk600, img.recordsBad600, img.framingRec600, img.framing600 - img.framingRec600, napTapeDruh(img));
    PutHook h2;
    Machine *b = startBasic(tbxl, h2);
    b->tape.img = img; b->tape.loaded = true; b->tape.pos = 0; b->tape.play = true; b->tape.name = "dlouha";
    {
      TypeQueue tq; tq.addText("CLOAD");
      bool enter = false; int cekej = 0;
      for (int f = 0; f < 50 * 900; f++) {
        tq.step(*b); snimekH(*b, h2);
        if (tq.empty() && !enter && ++cekej == 100) { b->klavesa(12); enter = true; cekej = 0; }
        if (enter && !b->motorOn() && ++cekej > 100) {
          const std::string sc = obrazovka(*b);
          if (sc.find("READY", sc.rfind("CLOAD")) != std::string::npos || sc.find("ERROR") != std::string::npos) break;
        }
      }
    }
    const std::vector<uint8_t> pb = programPamet(*b);
    size_t rozdil = 0;
    for (size_t i = 0; i + 1 < pa.size() && i < pb.size(); i++) if (pa[i] != pb[i]) rozdil++;   // posledni bajt = zacatek pole retezcu, muze se lisit
    const std::string listB = listH(*b, h2);
    printf("CLOAD: OS precetl %lld B, chyb ramce pri cteni %lld; program v pameti %zu B, rozdilnych bajtu %zu; LIST po CLOAD %s\n",
           b->tape.bytesRx, b->tape.framingRx, pb.size(), rozdil, normalizuj(listB) == normalizuj(listA) ? "SHODNY" : "JINY");
    if (getenv("LISTY")) {
      FILE *o = fopen("list_pred.txt", "wb"); fputs(listA.c_str(), o); fclose(o);
      o = fopen("list_po.txt", "wb"); fputs(listB.c_str(), o); fclose(o);
      o = fopen("list_zdroj.txt", "wb"); fputs(src.c_str(), o); fclose(o);
    }
    const bool ok = listOk && rozdil == 0 && normalizuj(listB) == normalizuj(listA) && img.framingRec600 == 0 && b->tape.framingRx == 0;
    printf("%s\n", ok ? "KAZETA DLOUHA OK: LIST = zdroj, CSAVE -> CLOAD bez chyby, LIST po nahrani stejny"
                      : "KAZETA DLOUHA CHYBA");
    return ok ? 0 : 7;
  }
  if (mode == "boot") {
    Machine *m = novy(true);
    int f;
    int pevne = argc > 3 ? atoi(argv[3]) : 0;
    for (f = 0; f < 3000; f++) { m->runFrame(); if (pevne) { if (f + 1 >= pevne) break; } else if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
    printf("READY po %d snimcich (%.2f s), PC=$%04X jam=%d cyc=%llu\n", f, f / 50.0, m->cpu.pc, m->cpu.jam, (unsigned long long)m->cyc);
    printf("%s", obrazovka(*m).c_str());
    if (argc > 2) ulozIdx(*m, argv[2]);
    return 0;
  }
  if (mode == "acid") {
    Machine *m = novy(false);
    std::vector<uint8_t> d = nacti(argv[2]);
    if (!m->disk.load(d.data(), d.size(), "acid800.atr")) { printf("ATR nelze nacist\n"); return 2; }
    m->consol = 7;   // Acid800: BASIC neni treba drzet (XL bez cartridge)
    m->consol = 3;   // OPTION drzet pri startu = BASIC vypnuty
    std::set<std::string> videno; int pass = 0, fail = 0, skip = 0;
    for (int f = 0; f < 60000; f++) {
      if (f == 100) m->consol = 7;
      m->runFrame();
      if (f % 8) continue;
      std::string s = obrazovka(*m); std::string flat;
      for (char c : s) if (c != '\n') flat.push_back(c);
      if (getenv("TEXTLOG")) {
        // cely textovy vystup testu (radky v poradi prvniho vyskytu)
        static std::vector<std::string> radky; static std::set<std::string> zname;
        for (int r = 0; r < 24; r++) {
          std::string rr = flat.substr(r * 40, 40);
          while (!rr.empty() && rr.back() == ' ') rr.pop_back();
          if (rr.empty()) continue;
          if (zname.insert(rr).second) { radky.push_back(rr); fprintf(stderr, "%s\n", rr.c_str()); }
        }
      }
      for (size_t i = 0; i + 10 < flat.size(); i++) {
        if (flat.compare(i, 3, "...") != 0) continue;
        char v = 0;
        if (flat.compare(i + 3, 4, "Pass") == 0) v = 'P'; else if (flat.compare(i + 3, 4, "FAIL") == 0) v = 'F'; else if (flat.compare(i + 3, 7, "Skipped") == 0) v = 'S';
        if (!v) continue;
        size_t st = i; while (st > 1 && i - st < 64 && !(flat[st - 1] == ' ' && flat[st - 2] == ' ')) st--;
        while (flat[st] == ' ') st++;
        std::string nm = flat.substr(st, i - st);
        if (nm.find(':') == std::string::npos) continue;
        if (videno.insert(nm).second) {
          printf("%s\t%s\n", v == 'P' ? "PASS" : v == 'F' ? "FAIL" : "SKIP", nm.c_str()); fflush(stdout);
          if (v == 'P') pass++; else if (v == 'F') fail++; else skip++;
          if (v == 'F' && getenv("DETAIL")) {
            // obrazovka kolem chyby (zprava testu je nad radkem s FAIL)
            size_t radek = st / 40;
            for (size_t rr = (radek >= 4 ? radek - 4 : 0); rr <= radek && rr < 24; rr++) printf("      | %s\n", flat.substr(rr * 40, 40).c_str());
          }
        }
      }
      if (flat.find("All tests complete.") != std::string::npos) {
        // Acid800 sam vypise souhrn o radek niz - par snimku pockat
        for (int k = 0; k < 120 && obrazovka(*m).find("Passed:") == std::string::npos; k++) m->runFrame();
        std::string sc = obrazovka(*m); size_t q = sc.find("Passed:");
        printf("KONEC snimek %d: pass %d fail %d skip %d | Acid800: %s\n", f, pass, fail, skip, q != std::string::npos ? sc.substr(q, sc.find('\n', q) - q).c_str() : "?");
        return 0;
      }
      if (m->cpu.jam) { printf("JAM na $%04X op $%02X (snimek %d)\n", m->cpu.jamPc, m->cpu.jamOp, f); printf("%s", s.c_str()); return 3; }
    }
    printf("TIMEOUT: pass %d fail %d skip %d\n%s", pass, fail, skip, obrazovka(*m).c_str());
    return 4;
  }
  if (mode == "atr") {
    // ./test_core atr disketa.atr <snimku> <prefix> [seznam]  - boot z D1: (OPTION drzene 150 snimku)
    std::vector<uint8_t> d = nacti(argv[2]);
    int frames = atoi(argv[3]); std::string pref = argv[4];
    std::set<int> dump; if (argc > 5) { const char *p = argv[5]; while (*p) { dump.insert(atoi(p)); while (*p && *p != ',') p++; if (*p) p++; } }
    Machine *m = novy(false);
    if (!m->disk.load(d.data(), d.size(), argv[2])) { printf("ATR nelze nacist\n"); return 2; }
    printf("ATR: %d sektoru po %d B\n", m->disk.sectors, m->disk.sectorSize);
    for (int f = 1; f <= frames; f++) {
      m->consol = f < 150 ? 3 : 7;
      vstupy(*m, f);
      m->runFrame();
      if (dump.count(f)) {
        ulozIdx(*m, pref + "_" + std::to_string(f) + ".idx");
        ulozPpm(*m, pref + "_" + std::to_string(f) + ".ppm");
        vbxeStav(*m, ("snimek " + std::to_string(f)).c_str());
      }
    }
    printf("PC=$%04X jam=%d SIO prikazu=%lld cteni=%lld zapisu=%lld PORTB=$%02X\n", m->cpu.pc, m->cpu.jam, m->sioPrikazu, m->disk.reads, m->disk.writes, m->mem.portB());
    vbxeStav(*m, "konec");
    return 0;
  }
  if (mode == "basic") {
    // ./test_core basic "prikazy"  - napise prikazy do BASICu (\n = RETURN) a vypise obrazovku
    Machine *m = novy(true);
    TypeQueue tq; int f = 0;
    for (; f < 400; f++) { m->runFrame(); if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
    std::string t = argc > 2 ? argv[2] : "?FRE(0)";
    for (size_t i = 0; (i = t.find("\\n", i)) != std::string::npos; ) t.replace(i, 2, "\n");
    tq.addText(t);
    int cekej = argc > 3 ? atoi(argv[3]) : 300;
    for (f = 0; f < 20000 && !tq.empty(); f++) { tq.step(*m); m->runFrame(); }
    for (int k = 0; k < cekej; k++) m->runFrame();
    printf("%s", obrazovka(*m).c_str());
    return 0;
  }
  if (mode == "xe130") {
    // Kontrola veci, ktere ma 130XE jinak nez Atari 800 / 800XL
    int chyb = 0, kontrol = 0;
    auto over = [&](bool ok, const char *co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co); };
    Machine *m = novy(true);
    TypeQueue tq;
    int f = 0;
    for (; f < 400; f++) { m->runFrame(); if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
    // OS a BASIC
    over(m->peek(0xC000) == 0x11 && m->peek(0xC001) == 0x92 && m->peek(0xFFF7) == 2, "OS ROM = XL/XE OS rev. 2 (soucet $9211) - ROM 130XE");
    // 1) 4 banky rozsirene pameti (PORTB bity 2-3, bit 4 = procesor) z BASICu
    tq.addText("10 FOR B=0 TO 3:POKE 54017,225+B*4:POKE 16384,B+10:NEXT B\n20 POKE 54017,253:POKE 16384,99\n"
               "30 FOR B=0 TO 3:POKE 54017,225+B*4:? PEEK(16384);\" \";:NEXT B\n40 POKE 54017,253:? PEEK(16384)\n"
               "50 ? \"TRIG3=\";PEEK(53267);\" D500=\";PEEK(54528)\nRUN\n");
    for (f = 0; f < 3000 && !tq.empty(); f++) { tq.step(*m); m->runFrame(); }
    for (int k = 0; k < 150; k++) m->runFrame();
    std::string sc = obrazovka(*m);
    printf("%s", sc.c_str());
    over(sc.find("10 11 12 13 99") != std::string::npos, "4 banky rozsirene pameti 130XE + zakladni pamet jsou oddelene (10 11 12 13 / 99)");
    over(sc.find("TRIG3=0") != std::string::npos, "TRIG3 = 0 (zadna cartridge ve slotu - 130XE)");
    over(sc.find("D500=213") != std::string::npos || sc.find("D500=") != std::string::npos, "cteni $D500 (neobsazeno) - plovouci sbernice 130XE");
    // 2) ANTIC vidi rozsirenou pamet samostatne (PORTB bit 5) - jen 130XE
    {
      uint8_t pb0 = (uint8_t)m->mem.pia.orB;
      m->mem.ext[2 * 0x4000 + 0x123] = 0x5A;                  // banka 2
      m->mem.ram[0x4123] = 0xA5;
      m->piaWrite(1, (uint8_t)(0xC1 | (2 << 2) | 0x10));       // bit5=0 ANTIC banka, bit4=1 CPU hlavni
      uint8_t cpuV = m->memRead(0x4123, false), anV = m->memRead(0x4123, true);
      m->piaWrite(1, (uint8_t)(0xC1 | (2 << 2) | 0x20));       // bit4=0 CPU banka, bit5=1 ANTIC hlavni
      uint8_t cpuV2 = m->memRead(0x4123, false), anV2 = m->memRead(0x4123, true);
      m->piaWrite(1, pb0);
      printf("ANTIC/CPU: bit5=0 -> CPU $%02X ANTIC $%02X | bit4=0 -> CPU $%02X ANTIC $%02X\n", cpuV, anV, cpuV2, anV2);
      over(cpuV == 0xA5 && anV == 0x5A && cpuV2 == 0x5A && anV2 == 0xA5, "ANTIC a procesor maji oddeleny pristup k rozsirene pameti (PORTB bit 4 / bit 5)");
    }
    // 3) self-test ROM v $5000 (PORTB bit 7 = 0) - vidi ji procesor i ANTIC
    {
      uint8_t pb0 = (uint8_t)m->mem.pia.orB;
      m->piaWrite(1, (uint8_t)(pb0 & 0x7F));
      uint8_t c = m->memRead(0x5000, false), a = m->memRead(0x5000, true);
      m->piaWrite(1, pb0);
      over(c == NAP_OS_ROM[0x1000] && a == NAP_OS_ROM[0x1000], "self-test ROM $5000-$57FF (PORTB bit 7) vidi procesor i ANTIC");
    }
    // 4) RESET = teply start: program zustane, OS znovu zapne BASIC
    m->reset();
    for (int k = 0; k < 200; k++) m->runFrame();
    tq.addText("LIST 10\n? PEEK(54017)\n");
    for (f = 0; f < 2000 && !tq.empty(); f++) { tq.step(*m); m->runFrame(); }
    for (int k = 0; k < 100; k++) m->runFrame();
    sc = obrazovka(*m);
    printf("%s", sc.c_str());
    over(sc.find("10 FOR B=0 TO 3") != std::string::npos, "po RESET (130XE: reset i PIA/MMU) program v pameti zustal (teply start)");
    over(sc.find("READY") != std::string::npos && (m->mem.portB() & 2) == 0, "po RESET OS znovu zapnul BASIC (PORTB bit 1 = 0)");
    // 5) obsah pameti po zapnuti = vzor DRAM 130XE
    {
      Machine *z = new Machine();
      bool ok = z->mem.ram[0] == 0x80 && z->mem.ram[1] == 0xFF && z->mem.ram[64] == 0x00 && z->mem.ram[65] == 0x7F && z->mem.ext[0] == 0x80;
      over(ok, "pamet po zapnuti = vzor DRAM 130XE (80 FF.. / 00 7F.. po 64 B)");
      delete z;
    }
    printf("\nVYSLEDEK 130XE: %d kontrol, %d chyb\n", kontrol, chyb);
    return chyb ? 1 : 0;
  }
  if (mode == "kazeta") {
    // CSAVE -> WAV (linka SIO DATA OUT) -> CLOAD z toho WAV -> LIST
    // ./test_core kazeta [vystup.wav] [vstup.wav]  (vstup = jen CLOAD ciziho WAV)
    std::string wavOut = argc > 2 ? argv[2] : "csave_test.wav";
    std::vector<uint8_t> wav;
    const char *program = "10 PRINT \"ATARIHELP B292\"\n20 FOR I=1 TO 3:? I*7:NEXT I\n30 GOTO 10\n";
    if (argc > 3) wav = nacti(argv[3]);
    else {
      Machine *m = novy(true);
      m->tapeCapture = true;
      TypeQueue tq; CsaveRecorder rec;
      int f = 0;
      for (; f < 400; f++) { m->runFrame(); float t[882]; m->genTape(t, 882); if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
      tq.addText(program); tq.addText("CSAVE");
      bool enterPoslan = false; int poBeep = 0;
      for (f = 0; f < 50 * 120; f++) {
        tq.step(*m);
        m->runFrame();
        float t[882]; m->genTape(t, 882);
        for (int i = 0; i < 882; i++) t[i] *= 0.8f;
        rec.snimek(*m, t, 882);
        // po CSAVE OS pipne a ceka na klavesu (RETURN = "pasek pripraven")
        if (tq.empty() && !enterPoslan) { if (++poBeep == 150) { m->klavesa(12); enterPoslan = true; } }
        if (rec.maHotovo) break;
      }
      for (auto &l : rec.log) printf("%s\n", l.c_str());
      if (!rec.maHotovo) { printf("CSAVE: WAV nevznikl (snimek %d)\n%s", f, obrazovka(*m).c_str()); return 5; }
      printf("CSAVE hotovo za %d snimku, %zu vzorku (%.1f s), SEROUT=%lld\n", f, rec.hotovo.size(), rec.hotovo.size() / 44100.0, m->seroutPocet);
      // WAV soubor
      const uint32_t nb = (uint32_t)rec.hotovo.size() * 2;
      uint8_t h[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
      uint32_t r = 36 + nb; std::memcpy(h + 4, &r, 4); std::memcpy(h + 40, &nb, 4);
      wav.assign(h, h + 44);
      for (int16_t v : rec.hotovo) { wav.push_back((uint8_t)(v & 0xFF)); wav.push_back((uint8_t)((v >> 8) & 0xFF)); }
      FILE *o = fopen(wavOut.c_str(), "wb"); if (o) { fwrite(wav.data(), 1, wav.size(), o); fclose(o); }
      delete m;
    }
    TapeImage img; std::string err;
    if (!napTapeFromWav(wav.data(), wav.size(), img, err)) { printf("WAV: %s\n", err.c_str()); return 6; }
    printf("PASKA: %.1f s, %d Hz, kanalu %d (FSK v kanalu %d), pri 600 Bd: %lld bajtu, %lld zaznamu, %lld chyb ramce\n",
           img.seconds(), img.srcRate, img.channels, img.usedChannel, img.bytes600, img.records600, img.framing600);
    Machine *m = novy(true);
    TypeQueue tq;
    int f = 0;
    for (; f < 400; f++) { m->runFrame(); if (f > 30 && obrazovka(*m).find("READY") != std::string::npos) break; }
    m->tape.img = img; m->tape.loaded = true; m->tape.pos = 0; m->tape.name = "test";
    std::vector<int> slog; if (getenv("SERIN")) m->serinLog = &slog;
    tq.addText("CLOAD");
    bool enter = false; int cekej = 0; bool listPoslan = false; int hotovo = -1;
    for (f = 0; f < 50 * 400; f++) {
      tq.step(*m);
      m->runFrame();
      if (tq.empty() && !enter) { if (++cekej == 100) { m->tape.play = true; m->klavesa(12); enter = true; cekej = 0; } }
      if (enter && !listPoslan && !m->motorOn() && ++cekej > 100) {
        std::string sc = obrazovka(*m);
        if (sc.find("READY", sc.find("CLOAD")) != std::string::npos || sc.find("ERROR") != std::string::npos) { tq.addText("LIST"); listPoslan = true; hotovo = f; }
      }
      if (listPoslan && tq.empty() && f > hotovo + 150) break;
    }
    if (getenv("SERIN")) { for (size_t k = 0; k < slog.size(); k++) printf("%02X%s%s%s ", slog[k] & 0xFF, (slog[k] & 0x100) ? "!" : "", (slog[k] & 0x200) ? "i" : "", (slog[k] & 0x400) ? "" : "-"); printf("\n"); }
    printf("CLOAD: snimek %d, prijato bajtu %lld, chyb ramce %lld, pozice pasky %.1f s\n%s", f, m->tape.bytes, m->tape.framing, m->tape.pos / img.rate, obrazovka(*m).c_str());
    std::string sc = obrazovka(*m);
    bool ok = sc.find("10 PRINT \"ATARIHELP B292\"") != std::string::npos && sc.find("30 GOTO 10") != std::string::npos;
    printf("%s\n", ok ? "KAZETA OK: program po CLOAD souhlasi" : "KAZETA CHYBA: program po CLOAD nesouhlasi");
    return ok ? 0 : 7;
  }
  if (mode == "bootkazeta") {
    // B298: kazeta s bootem (hra): ./test_core bootkazeta boot.wav | synt | synt-stereo
    // START+OPTION drzene od zapnuti, po pipnuti OS RETURN (KazetaBoot jako v appce)
    // synt = synteticka kazeta ze syntaz_kazeta.h (synt-stereo: + zvukova stopa 440 Hz)
    const std::string co = argc > 2 ? argv[2] : "synt";
    std::vector<uint8_t> wav = co == "synt" ? syntazBootKazetaWav(false) : co == "synt-stereo" ? syntazBootKazetaWav(true) : nacti(co.c_str());
    TapeImage img; std::string err;
    if (!napTapeFromWav(wav.data(), wav.size(), img, err)) { printf("WAV: %s\n", err.c_str()); return 6; }
    printf("PASKA: %.1f s, kanalu %d (data v %d), druh: %s, zaznamu %lld (OK %lld, spatne %lld), chyb ramce v zaznamech %lld (vsech %lld), zvukova stopa %zu vzorku @ %.0f Hz\n",
           img.seconds(), img.channels, img.usedChannel, napTapeDruh(img), img.records600, img.recordsOk600, img.recordsBad600,
           img.framingRec600, img.framing600, img.audio.size(), img.audioRate);
    if (img.druh != TapeImage::BOOT) { printf("BOOT KAZETA CHYBA: kazeta nebyla poznana jako bootovaci\n"); return 9; }
    Machine *m = new Machine();
    m->view = new AnticView();
    m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
    m->coldInit();
    m->consol = KazetaBoot::konzole(7);
    m->cpu.reset();
    m->tape.img = img; m->tape.loaded = true; m->tape.play = true; m->tape.pos = 0; m->tape.name = "boot";
    KazetaBoot kb; kb.start(*m);
    int f, motorOd = -1; double zvukStopa = 0;
    for (f = 0; f < 50 * 150; f++) {
      m->consol = kb.aktivni() ? KazetaBoot::konzole(7) : 7;
      m->runFrame();
      float z[882]; m->genAudio(z, 882, 44100);
      if (m->motorOn() && motorOd < 0) motorOd = f;
      if (m->motorOn()) for (int i = 0; i < 882; i++) zvukStopa += z[i] * z[i];
      kb.poSnimku(*m);
      if (m->peek(0x600) == 0x42) break;
    }
    for (auto &l : kb.log) printf("%s\n", l.c_str());
    printf("snimek %d: motor od snimku %d, $0600=$%02X COLOR4=$%02X PC=$%04X, prijato z pasky %lld B, chyb ramce pri cteni %lld, energie zvuku s motorem %.1f\n",
           f, motorOd, m->peek(0x600), m->peek(0x2C8), m->cpu.pc, m->tape.bytes, m->tape.framingRx, zvukStopa);
    const bool ok = m->peek(0x600) == 0x42 && m->peek(0x2C8) == 0x34;
    printf("%s\n", ok ? "BOOT KAZETA OK: hra z kazety nabootovala (START+OPTION pri zapnuti, RETURN po pipnuti)" : "BOOT KAZETA CHYBA");
    return ok ? 0 : 8;
  }
  if (mode == "xex") {
    std::vector<uint8_t> d = nacti(argv[2]);
    int frames = atoi(argv[3]); std::string pref = argv[4];
    std::set<int> dump; if (argc > 5) { const char *p = argv[5]; while (*p) { dump.insert(atoi(p)); while (*p && *p != ',') p++; if (*p) p++; } }
    Machine *m = novy(false);
    XexLoader xl; xl.priprav(d.data(), d.size(), argv[2]);
    if (getenv("TRACE")) m->traceFrame = atoi(getenv("TRACE"));
    m->sioRychlyTimeout = true;
    for (int f = 1; f <= frames; f++) {
      if (!xl.aktivni()) m->consol = 7;
      vstupy(*m, f);
      m->runFrame(); xl.poSnimku(*m);
      if (dump.count(f)) {
        ulozIdx(*m, pref + "_" + std::to_string(f) + ".idx");
        ulozPpm(*m, pref + "_" + std::to_string(f) + ".ppm");
        vbxeStav(*m, ("snimek " + std::to_string(f)).c_str());
        if (getenv("DUMPMEM")) {
          FILE *o = fopen((pref + "_" + std::to_string(f) + ".mem").c_str(), "wb");
          for (int a = 0; a < 65536; a++) { uint8_t b = m->peek((uint16_t)a); fwrite(&b, 1, 1, o); }
          fclose(o);
          printf("snimek %d: DLIST=$%04X DMACTL=$%02X PRIOR=$%02X GRACTL=$%02X CHBASE=$%02X PMBASE=$%02X NMIEN=$%02X\n", f, m->dlist, m->dmactlReg, m->prior, m->gractl, m->chbaseReg, m->pmbase, m->nmien);
          printf("  HPOS:"); for (int i = 0; i < 8; i++) printf(" %02X", m->sprPos[i]);
          printf("  SIZE:"); for (int i = 0; i < 8; i++) printf(" %d", m->spr[i].size);
          printf("  LATCH:"); for (int i = 0; i < 8; i++) printf(" %02X", m->spr[i].latch);
          printf("\n  COLPM: %02X %02X %02X %02X COLPF: %02X %02X %02X %02X COLBK %02X VDELAY %02X\n", m->colpm[0], m->colpm[1], m->colpm[2], m->colpm[3], m->colpf[0], m->colpf[1], m->colpf[2], m->colpf[3], m->colbk, m->vdelay);
        }
      }
    }
    for (auto &l : xl.log) printf("LOG %s\n", l.c_str());
    printf("PC=$%04X jam=%d\n", m->cpu.pc, m->cpu.jam);
    vbxeStav(*m, "konec");
    return 0;
  }
  return 1;
}
