// test_overeni/b352/regrese_b352.cpp (= ../b323/regrese_her.cpp + spusteni znovu s 1088K jako appka B352)
// ====================================================================
//  B352: stejna hra ve STAREM (B351) a NOVEM jadru Atari -> otisky obrazu (ANTIC/GTIA i cely obraz s VBXE).
//  S -DAPPKA_B352 se program, ktery hleda vic bank nez 320K (mem.chce1088), spusti znovu od zacatku
//  s 1088K presne jako v appce (nap_atari_native.cpp restart1088Locked) a na konci se vypise RESTART1088.
//
//  Preklad (stare jadro = git worktree B351, nove = tahle verze; APPKA_B352 jen s novym):
//    g++ -O2 -std=c++17 -I <jadro>/app/src/main/cpp/atari regrese_b352.cpp -o regrese
//  Pouziti: regrese SOUBOR(.xex/.atr/.car) SNIMKU [ADR_PPM KAZDY]
//  Hra bezi jako v appce (XEX: studeny start s OPTION + zavadec; ATR: D1:;
//  CAR: slot), stejny scenar vstupu pro vsechny (START, FIRE, joystick),
//  vypise CRC32 obrazu kazdych 100 snimku + PC na konci. Kdyz se otisky
//  stareho a noveho jadra shoduji, zmena se hry nedotkla.
// ====================================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
using namespace nap;

static std::vector<uint8_t> nacti(const char *f) {
  std::vector<uint8_t> d; FILE *fp = fopen(f, "rb"); if (!fp) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n); fclose(fp); return d;
}
static bool konci(const std::string &s, const char *k) {
  const size_t n = strlen(k); if (s.size() < n) return false;
  for (size_t i = 0; i < n; i++) if (tolower((unsigned char)s[s.size() - n + i]) != k[i]) return false;
  return true;
}
static void ppm(const Machine &m, const std::string &fn) {
  FILE *o = fopen(fn.c_str(), "wb"); if (!o) return;
  fprintf(o, "P6\n%d %d\n255\n", AnticView::W, AnticView::H);
  for (int i = 0; i < AnticView::W * AnticView::H; i++) { uint32_t v = napAtariPalette(m.view->idx[i]); uint8_t p[3] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16)}; fwrite(p, 1, 3, o); }
  fclose(o);
}
// scenar: START 600-606, FIRE 900-906 a 1300-1306, joystick vpravo 1000-1100, vlevo 1150-1250
static void vstupy(Machine &m, int f, bool zavadi) {
  int consol = 7, joy = 0, fire = 0;
  if (f >= 600 && f <= 606) consol &= ~1;
  if ((f >= 900 && f <= 906) || (f >= 1300 && f <= 1306)) fire = 1;
  if (f >= 1000 && f <= 1100) joy |= 8;
  if (f >= 1150 && f <= 1250) joy |= 4;
  if (!zavadi) m.consol = consol;
  m.porta = 0xF0 | (~joy & 15);
  m.trig[0] = fire ? 0 : 1;
}
int main(int argc, char **argv) {
  if (argc < 3) { fprintf(stderr, "pouziti: regrese_her SOUBOR SNIMKU [ADR KAZDY]\n"); return 2; }
  const std::string jm = argv[1];
  std::vector<uint8_t> d = nacti(argv[1]);
  const int snimku = atoi(argv[2]);
  const std::string adr = argc > 4 ? argv[3] : "";
  const int kazdy = argc > 4 ? atoi(argv[4]) : 0;
  Machine *m = nullptr; AnticView *view = new AnticView();
  XexLoader xl;
  bool r1088 = false;
  int restart = -1;
  auto start = [&]() -> int {
    delete m; m = new Machine(); m->view = view;
    m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
#ifdef APPKA_B352
    m->mem.rezim1088 = r1088;
#endif
    if (konci(jm, ".xex")) {
      m->coldInit(); m->consol = 3; m->cpu.reset();
      if (!xl.priprav(d.data(), d.size(), jm)) { printf("NENI XEX\n"); return 1; }
      m->sioRychlyTimeout = true;
    } else if (konci(jm, ".atr")) {
      if (!m->disk.load(d.data(), d.size(), jm)) { printf("NENI ATR\n"); return 1; }
      m->reset(); m->consol = 7;
    } else if (konci(jm, ".car")) {
      if (m->cart.nactiCar(d.data(), d.size(), jm) != Cartridge::OK) { printf("NENI CAR\n"); return 1; }
      m->reset(); m->consol = 7; m->sioRychlyTimeout = true;
    } else { printf("NEZNAMY TYP\n"); return 1; }
    return 0;
  };
  if (start()) return 1;
  std::string otisky;
  for (int f = 1; f <= snimku; f++) {
    const bool zav = xl.aktivni();
    vstupy(*m, f, zav);
    m->runFrame(); xl.poSnimku(*m);
    float z[1000]; m->genAudio(z, 882, 44100.0);
    // otisk = barvy ANTIC/GTIA (idx) / cely obraz vcetne VBXE (fb) - B323 bral jen idx, VBXE prekryv nevidel
    if (f % 100 == 0) { char b[24]; snprintf(b, sizeof b, " %08X/%08X", napCrc32(m->view->idx, (size_t)AnticView::W * AnticView::H),
                                             napCrc32((const uint8_t *)m->view->fb, sizeof(m->view->fb))); otisky += b; }
    if (kazdy > 0 && f % kazdy == 0 && !adr.empty()) ppm(*m, adr + "/s_" + std::to_string(100000 + f).substr(1) + ".ppm");
#ifdef APPKA_B352
    if (m->mem.chce1088 && !m->mem.rezim1088) { r1088 = true; restart = f; start(); }
#endif
    if (m->cpu.jam) { printf("JAM $%04X snimek %d\n", m->cpu.jamPc, f); break; }
  }
  std::string posledni;
  for (auto &l : xl.log) posledni = l;
  printf("OTISKY%s\nKONEC PC=$%04X PORTB=$%02X jam=%d xex=%d | %s\n", otisky.c_str(), m->cpu.pc, m->mem.portB(), m->cpu.jam ? 1 : 0, (int)xl.stav, posledni.c_str());
  if (restart >= 0) printf("RESTART1088 snimek %d\n", restart);
  return 0;
}
