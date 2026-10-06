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
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
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
struct PutHook { int put = -1; bool on = false; std::string out; };
static void snimekH(Machine &m, PutHook &h) {
  const long long f = m.frame;
  while (m.frame == f) {
    if (h.on && m.cpu.pc == h.put && !m.cpu.takeNmi && !m.cpu.takeIrq) h.out.push_back((char)m.cpu.a);
    // presne jako Machine::runFrame (SIO patch jen disk, kazeta $60 nikdy)
    if (m.cpu.pc == 0xE459 && !m.cpu.jam && (m.disk.mounted || m.sioRychlyTimeout) && m.mem.ram[0x300] != 0x60 && !m.cpu.takeNmi && !m.cpu.takeIrq) { m.sioPatch(); continue; }
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

int main(int argc, char **argv) {
  std::string mode = argc > 1 ? argv[1] : "boot";
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
