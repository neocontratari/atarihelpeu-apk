// test_overeni/b361/test_b361_pokey_stereo.cpp
// B361: DRUHY POKEY - STEREO (Rene 23. 11. 2026: "udelej prosim jeste POKEY Dual Stereo").
// Skutecne jadro appky (nap_atari_machine.h), Atari s BASICem, program se napise klavesnici jako v appce:
//   1) mono hry: zvuk PRESNE jako pred B361 (stare vs nove jadro bit po bitu - pokey_mono_dump), pravy = levy
//   2) program pozna stereo jako na skutecnem Atari (SKCTL druheho POKEYe nesmi zastavit RANDOM prvniho)
//      a hraje: levy kanal ton POKEY 1 (~487 Hz), pravy ton POKEY 2 (~326 Hz); do te doby pravy = levy
//   3) Atari bez upgradu (stereoPokey = false): $D210 je zrcadlo prvniho - program pozna MONO, jeden ton na obou
//   4) adresy: $D210-$D21F a zrcadla po 32 bajtech ($D230, $D2F0) = POKEY 2, $D220 = POKEY 1
//   5) druhy POKEY ma vlastni RANDOM / SKCTL a casovace s prerusenim (perioda jako prvni POKEY, B300 test)
//   6) studeny start: zase mono (pravy = levy), dokud program druhy POKEY nepouzije
//   7) OS pri zapnuti nuluje $D2xx - druhy POKEY se z nul nerozbehne (mono hry nic navic nestoji); vykon
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o test_b361_pokey_stereo test_b361_pokey_stereo.cpp
//   ./test_b361_pokey_stereo [PRAC/stary.mono PRAC/novy.mono PRAC/novy.L PRAC/novy.R]  -> VYSTUP_test_b361_pokey_stereo.txt
#include <chrono>
#include <set>
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
using namespace nap;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

static Machine *novy(bool stereo) {
  Machine *m = new Machine();
  m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->stereoPokey = stereo;
  m->coldInit();
  m->consol = 7;
  m->cpu.reset();
  return m;
}

static std::string obrazovka(Machine &m) {
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) {
    for (int c = 0; c < 40; c++) {
      int v = m.peek((uint16_t)(sav + r * 40 + c)) & 0x7F;
      int a = v < 64 ? v + 32 : (v < 96 ? v - 64 : v);
      out.push_back((a < 32 || a > 126) ? ' ' : (char)a);
    }
    out.push_back('\n');
  }
  return out;
}

// Ton kanalu POKEY v 64 kHz rezimu: 1773447 / 28 / (2 * (AUDF + 1)) Hz emulovaneho casu. Appka dava 882 vzorku na
// snimek PAL (312 x 114 = 35568 cyklu = 1/49,86 s), takze ve zvuku 44100 Hz je ton o 0,28 % vys (jako vzdy od B287).
static double tonHz(int audf) { return 1773447.0 / 28.0 / (2.0 * (audf + 1)) * 44100.0 / (882.0 * 1773447.0 / 35568.0); }
// sila tonu f (Hz) v kanalu (Goertzel), 44100 Hz
static double ton(const std::vector<float> &x, size_t od, size_t n, double f) {
  const double w = 2.0 * 3.14159265358979323846 * f / 44100.0, k = 2.0 * std::cos(w);
  double s1 = 0, s2 = 0;
  for (size_t i = od; i < od + n && i < x.size(); i++) { const double s = x[i] + k * s1 - s2; s2 = s1; s1 = s; }
  return std::sqrt(std::max(0.0, s1 * s1 + s2 * s2 - k * s1 * s2)) / (double)n;
}

static std::vector<float> nacti(const char *f) {
  std::vector<float> v;
  FILE *fp = std::fopen(f, "rb");
  if (!fp) return v;
  std::fseek(fp, 0, SEEK_END); long n = std::ftell(fp); std::fseek(fp, 0, SEEK_SET);
  v.resize((size_t)n / 4);
  if (std::fread(v.data(), 4, v.size(), fp) != v.size()) v.clear();
  std::fclose(fp);
  return v;
}

// program: stereo detekce jako ve hrach (SKCTL druheho POKEYe = 0, pak 2x RANDOM prvniho), pak ton na kazdem POKEYi
static const char *STEREO_PROGRAM =
    "10 POKE 53791,0:A=PEEK(53770):B=PEEK(53770):POKE 53791,3:POKE 53775,3\n"
    "20 IF A=B THEN PRINT \"MONO\":GOTO 40\n"
    "30 PRINT \"STEREO\"\n"
    "40 FOR I=1 TO 40:NEXT I\n"
    "50 SOUND 0,64,10,8\n"
    "60 POKE 53784,0:POKE 53776,96:POKE 53777,166\n"
    "70 GOTO 70\n"
    "RUN\n";

struct Beh { std::vector<float> L, R; std::string obraz; Machine *m = nullptr; int stereoOdSnimku = -1; };
static Beh hraj(bool stereo, const char *program, int snimku, bool resetVeSnimku = false) {
  Beh b; b.m = novy(stereo);
  TypeQueue q;
  b.L.assign((size_t)snimku * 882, 0); b.R.assign((size_t)snimku * 882, 0);
  for (int f = 0; f < snimku; f++) {
    if (f == 120) q.addText(program);
    if (resetVeSnimku && f == snimku - 200) { b.m->coldInit(); b.m->cpu.reset(); q.clear(); }
    q.step(*b.m);
    b.m->runFrame();
    b.m->genAudioStereo(b.L.data() + (size_t)f * 882, b.R.data() + (size_t)f * 882, 882, 44100.0);
    if (b.stereoOdSnimku < 0 && b.m->stereoAktivni()) b.stereoOdSnimku = f;
  }
  b.obraz = obrazovka(*b.m);
  return b;
}

int main(int argc, char **argv) {
  std::printf("B361 - druhy POKEY (stereo): skutecne jadro appky nap_atari_machine.h\n");

  std::printf("== 1) mono hry: zvuk presne jako pred B361\n");
  if (argc >= 5) {
    const std::vector<float> st = nacti(argv[1]), no = nacti(argv[2]), l = nacti(argv[3]), r = nacti(argv[4]);
    double rms = 0; for (float x : st) rms += (double)x * x; rms = st.empty() ? 0 : std::sqrt(rms / st.size());
    over(!st.empty() && st == no && no == l && l == r && rms > 0.01,
         "program jen pro prvni POKEY (SOUND, 1,79 MHz, 16 bitu, horni propust, 15 kHz, klapani klaves), 1500 snimku: stare jadro = nove "
         "jadro = levy = pravy kanal, bit po bitu (" + std::to_string(st.size()) + " vzorku, rms " + std::to_string(rms).substr(0, 6) + ")");
  } else {
    over(false, "chybi vystupy pokey_mono_dump (stary / novy) - viz JAK_SPUSTIT.txt");
  }

  std::printf("== 2) program pro stereo na Atari se dvema POKEYi\n");
  Beh s = hraj(true, STEREO_PROGRAM, 1300);
  const size_t konec = s.L.size() - 44100;
  const double f1 = tonHz(64), f2 = tonHz(96);   // ~488,6 Hz a ~327,4 Hz
  const double l487 = ton(s.L, konec, 44100, f1), l326 = ton(s.L, konec, 44100, f2);
  const double r487 = ton(s.R, konec, 44100, f1), r326 = ton(s.R, konec, 44100, f2);
  over(s.obraz.find("STEREO") != std::string::npos, "program na obrazovce: STEREO (SKCTL druheho POKEYe nezastavil RANDOM prvniho)");
  // amplituda zakladni harmonicke obdelniku: hlasitost/60/2 * 4/pi, Goertzel ukaze polovinu
  const double ocekL = 8 / 60.0 / 2 * 4 / 3.14159265 / 2, ocekR = 6 / 60.0 / 2 * 4 / 3.14159265 / 2;
  over(l487 > 20 * l326 && r326 > 20 * r487 && l487 > 0.8 * ocekL && r326 > 0.8 * ocekR,
       "levy kanal = POKEY 1: ton " + std::to_string(f1).substr(0, 5) + " Hz sila " + std::to_string(l487).substr(0, 6) + " (ocekavano " +
           std::to_string(ocekL).substr(0, 6) + ", druhy ton " + std::to_string(l326).substr(0, 7) + "), pravy = POKEY 2: ton " +
           std::to_string(f2).substr(0, 5) + " Hz sila " + std::to_string(r326).substr(0, 6) + " (ocekavano " + std::to_string(ocekR).substr(0, 6) +
           ", prvni ton " + std::to_string(r487).substr(0, 7) + ")");
  bool stejnePred = s.stereoOdSnimku > 120;
  for (size_t i = 0; stejnePred && i < (size_t)s.stereoOdSnimku * 882; i++) if (s.L[i] != s.R[i]) stejnePred = false;
  over(stejnePred, "dokud program na druhy POKEY nehral (do snimku " + std::to_string(s.stereoOdSnimku) + "), pravy kanal = levy (bit po bitu)");

  std::printf("== 3) Atari bez upgradu (stereoPokey = false): $D210 = zrcadlo prvniho POKEYe\n");
  Beh m = hraj(false, STEREO_PROGRAM, 1300);
  const double ml487 = ton(m.L, konec, 44100, f1), ml326 = ton(m.L, konec, 44100, f2);
  over(m.obraz.find("MONO") != std::string::npos && m.L == m.R && ml326 > 20 * ml487 && !m.m->stereoAktivni() && !m.m->pk2.bezi,
       "program pozna MONO, zapis do $D210 prepise ton prvniho POKEYe (326 Hz " + std::to_string(ml326).substr(0, 6) + ", 487 Hz " +
           std::to_string(ml487).substr(0, 7) + "), oba kanaly stejne - jako skutecne Atari bez upgradu");

  std::printf("== 4) adresy\n");
  {
    Machine *x = novy(true);
    x->ioWrite(0xD210, 0x11); x->ioWrite(0xD230, 0x22); x->ioWrite(0xD2F2, 0x33); x->ioWrite(0xD220, 0x44); x->ioWrite(0xD2E2, 0x55);
    over(x->pk2.audf[0] == 0x22 && x->pk2.audf[1] == 0x33 && x->audf[0] == 0x44 && x->audf[1] == 0x55,
         "$D210 / $D230 -> POKEY 2 AUDF1, $D2F2 -> POKEY 2 AUDF2, $D220 -> POKEY 1 AUDF1, $D2E2 -> POKEY 1 AUDF2 (bit A4 vybira cip)");
    Machine *y = novy(false);
    y->ioWrite(0xD210, 0x66); y->ioWrite(0xD21E, 0x00);
    over(y->audf[0] == 0x66 && y->pk2.audf[0] == 0 && !y->pk2.bezi, "bez upgradu: $D210 -> POKEY 1 (zrcadlo), druhy POKEY nic");
  }

  std::printf("== 5) RANDOM, SKCTL a casovace druheho POKEYe\n");
  {
    Machine *x = novy(true);
    for (int f = 0; f < 60; f++) x->runFrame();   // OS bezi, POKEY 1 normalne
    const uint8_t r1a = x->ioRead(0xD21A), r1b = x->ioRead(0xD21A);
    x->ioWrite(0xD21F, 0x03);
    for (int i = 0; i < 20; i++) { x->pokey2Tick(); x->cyc++; }
    std::set<int> ruzne;
    for (int i = 0; i < 64; i++) { ruzne.insert(x->ioRead(0xD21A)); x->pokey2Tick(); x->cyc += 7; }
    const uint8_t p1a = x->ioRead(0xD20A);
    x->cyc += 3;
    const uint8_t p1b = x->ioRead(0xD20A);
    over(r1a == 0xFF && r1b == 0xFF && ruzne.size() > 40 && p1a != p1b,
         "RANDOM druheho: v init rezimu $FF, po SKCTL=3 bezi (" + std::to_string(ruzne.size()) + " ruznych z 64); RANDOM prvniho bezi dal");
    // casovac 1 druheho POKEYe s prerusenim - perioda (AUDF+1) * 28 cyklu (64 kHz), jako prvni POKEY (B300)
    int chyb = 0; std::string pr;
    for (int audf : {0, 1, 63, 255}) {
      x->pokey2Write(0x0F, 0x03); x->pokey2Write(0x08, 0x00); x->pokey2Write(0x00, (uint8_t)audf);
      x->pokey2Write(0x0E, 0x01); x->pokey2Write(0x09, 0x00);
      std::vector<uint64_t> kdy;
      for (int i = 0; i < 40000 && kdy.size() < 4; i++) {
        x->pokey2Tick(); x->cyc++;
        if (!(x->pk2.irqst & 1)) { kdy.push_back(x->cyc); if (!x->irqLine) chyb++; x->pokey2Write(0x0E, 0x00); x->pokey2Write(0x0E, 0x01); }
      }
      const uint64_t per = kdy.size() >= 3 ? kdy[2] - kdy[1] : 0;
      if (per != (uint64_t)(audf + 1) * 28) chyb++;
      pr += " AUDF " + std::to_string(audf) + " -> " + std::to_string(per);
    }
    x->pokey2Write(0x0E, 0x00);
    over(chyb == 0 && !x->irqLine, "casovac 1 druheho POKEYe: preruseni s periodou (AUDF+1) x 28 cyklu:" + pr + " (linka IRQ spolecna)");
  }

  std::printf("== 6) studeny start\n");
  {
    Beh z = hraj(true, STEREO_PROGRAM, 1300, true);
    bool stejne = true;
    for (size_t i = z.L.size() - 150 * 882; i < z.L.size(); i++) if (z.L[i] != z.R[i]) stejne = false;
    over(z.stereoOdSnimku > 0 && !z.m->stereoAktivni() && stejne, "stereo hralo od snimku " + std::to_string(z.stereoOdSnimku) +
                                                                       ", po studenem startu zase mono: pravy = levy");
  }

  std::printf("== 7) mono hry nic navic nestoji\n");
  {
    Machine *x = novy(true);
    for (int f = 0; f < 200; f++) x->runFrame();
    over(!x->pk2.bezi && x->pk2.zapisu >= 128, "po zapnuti: OS vynuloval $D2xx (" + std::to_string(x->pk2.zapisu) +
                                                   " zapisu do adres druheho POKEYe), druhy POKEY se z nul nerozbehl - nepocita se");
    auto cas = [](bool stereo, const char *prog) {
      Machine *y = novy(stereo);
      TypeQueue q; q.addText(prog);
      std::vector<float> l(882), r(882);
      const auto t0 = std::chrono::steady_clock::now();
      for (int f = 0; f < 1500; f++) { q.step(*y); y->runFrame(); y->genAudioStereo(l.data(), r.data(), 882, 44100.0); }
      return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count() / 1500.0;
    };
    const double mono = cas(true, "10 SOUND 0,64,10,8\n20 GOTO 20\nRUN\n"), st = cas(true, STEREO_PROGRAM);
    over(st < mono * 1.6, "snimek na PC: mono hra " + std::to_string(mono).substr(0, 5) + " ms, stereo hra (oba POKEYe + 2 kanaly FIR) " +
                              std::to_string(st).substr(0, 5) + " ms");
  }

  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
