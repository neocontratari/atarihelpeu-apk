// B291: test ciste C++ casti behu Atari v HELP (bez Androidu):
//  - psani textu klavesnici (TypeQueue) - BASIC program se opravdu zapise
//    (overeno prectenim obrazovky Atari po LIST)
//  - CSAVE -> WAV (CsaveRecorder) - zaznam jen pri bezicim motoru a jen
//    kdyz odesla data (SEROUT)
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b291_runtime test_b291_runtime.cpp
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
using namespace nap;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const char *co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co); }

// Precte textovou obrazovku GR.0 primo z pameti Atari (SAVMSC $58/$59),
// internal kod -> ASCII. Tim se overuje, co OS skutecne zapsal.
static std::string obrazovka(Machine &m) {
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    std::string line;
    for (int c = 0; c < 40; c++) {
      int v = m.mem.ram[(sav + r * 40 + c) & 0xFFFF] & 0x7F;
      int a;
      if (v < 64) a = v + 32; else if (v < 96) a = v - 64; else a = v;
      if (a < 32 || a > 126) a = '.';
      line.push_back((char)a);
    }
    while (!line.empty() && line.back() == ' ') line.pop_back();
    out += line + "\n";
  }
  return out;
}

int main() {
  Machine *m = new Machine(); AnticView *v = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM; m->view = v;
  std::memset(m->mem.ram, 0, sizeof(m->mem.ram));
  m->reset(); m->consol = 7;
  std::vector<float> a(882);
  CsaveRecorder rec;
  auto snimek = [&](TypeQueue *tq) {
    if (tq) tq->step(*m);
    m->runFrame();
    if (m->cpu.jam) std::fill(a.begin(), a.end(), 0.f); else m->genAudio(a.data(), 882, 44100.0);
    rec.snimek(*m, a.data(), 882);
  };
  int f = 0;
  for (; f < 1500; f++) { snimek(nullptr); if (obrazovka(*m).find("READY") != std::string::npos && f > 50) break; }
  over(obrazovka(*m).find("READY") != std::string::npos, "po startu je na obrazovce READY");
  for (int i = 0; i < 30; i++) snimek(nullptr);

  // 1) psani programu - vcetne cislic, uvozovek, stejnych pismen za sebou (LL, 00)
  TypeQueue tq;
  int n = tq.addText("10 PRINT \"HELLO 1000\"\n20 GOTO 10\nLIST");
  over(n > 30 && tq.preskoceno == 0, "text prevedeny na klavesy (zadny neznamy znak)");
  int fr = 0;
  while (!tq.empty() && fr < 3000) { snimek(&tq); fr++; }
  for (int i = 0; i < 60; i++) snimek(&tq);
  std::string scr = obrazovka(*m);
  printf("--- obrazovka po LIST (%d snimku psani) ---\n%s---\n", fr, scr.c_str());
  over(tq.empty(), "fronta klaves se vyprazdnila");
  over(scr.find("10 PRINT \"HELLO 1000\"") != std::string::npos, "LIST ukazuje radek 10 PRESNE (vc. LL a 000)");
  over(scr.find("20 GOTO 10") != std::string::npos, "LIST ukazuje radek 20");

  // 2) CSAVE: napsat CSAVE + RETURN, po pipnuti dalsi RETURN -> motor + data
  TypeQueue t2; t2.addText("CSAVE");
  while (!t2.empty()) snimek(&t2);
  bool motorBezel = false; int fmax = 0;
  for (int i = 0; i < 400; i++) { snimek(&t2); }           // pipnuti, ceka na klavesu
  t2.q.push_back(12);                                       // RETURN = stisk klavesy po pipnuti
  for (fmax = 0; fmax < 50 * 120; fmax++) {
    snimek(&t2);
    if (CsaveRecorder::motor(*m)) motorBezel = true;
    if (motorBezel && !CsaveRecorder::motor(*m)) break;
  }
  for (auto &l : rec.log) printf("LOG: %s\n", l.c_str());
  over(motorBezel, "po CSAVE se zapnul kazetovy motor");
  over(rec.maHotovo, "po vypnuti motoru je hotovy WAV zaznam");
  // pri studenem startu (PIA=0 vypada jako zapnuty motor) se NIC nesmi ulozit
  int falesnych = 0; for (auto &l : rec.log) if (l.find("CSAVE_HOTOVO") != std::string::npos) falesnych++;
  over(falesnych == 1, "presne jeden CSAVE zaznam (zadny falesny pri startu)");
  double sek = rec.hotovo.size() / 44100.0;
  printf("CSAVE trvalo %d snimku, WAV %.1f s, SEROUT celkem %lld\n", fmax, sek, m->seroutPocet);
  over(sek > 10.0 && sek < 60.0, "delka WAV odpovida CSAVE (10-60 s)");
  // signal neni ticho: spocitat pruchody nulou (FSK tony ~5 kHz)
  long long nul = 0; for (size_t i = 1; i < rec.hotovo.size(); i++) if ((rec.hotovo[i - 1] < 0) != (rec.hotovo[i] < 0)) nul++;
  printf("pruchodu nulou: %lld (%.0f za s)\n", nul, nul / std::max(1.0, sek));
  over(nul / std::max(1.0, sek) > 2000, "WAV obsahuje kazetove tony (tisice pruchodu nulou za s)");
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
