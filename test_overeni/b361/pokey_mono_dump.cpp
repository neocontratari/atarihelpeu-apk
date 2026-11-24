// test_overeni/b361/pokey_mono_dump.cpp
// B361: MONO HRY HRAJI PRESNE JAKO PRED B361 - stejny program prelozeny dvakrat:
//   se STARYM nap_atari_machine.h (git: posledni verze pred B361, -I na slozku s nim PRED cestou jadra)
//   a s NOVYM (dva POKEYe). Atari s BASICem napise a spusti program, ktery hraje jen na prvni POKEY
// (SOUND, AUDCTL 1,79 MHz, 16 bitu, horni propust, 15 kHz, klapani klaves pri psani = reproduktor GTIA).
// Zvuk 1500 snimku se zapise do souboru (float32); nove jadro navic tymz programem na DRUHEM stroji
// pres genAudioStereo -> levy a pravy kanal (L, R). Porovnani dela test_b361_pokey_stereo.
//   g++ -std=c++17 -O2 -I<stary> -I../../app/src/main/cpp/atari -o dump_stary pokey_mono_dump.cpp   (-DNAP_STARY)
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o dump_novy pokey_mono_dump.cpp
//   ./dump_stary PRAC/stary   ./dump_novy PRAC/novy      -> PRAC/*.mono (a novy .L .R)
#include <cstdio>
#include <string>
#include <vector>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
using namespace nap;

static const char *PROGRAM =
    "10 SOUND 0,121,10,8:SOUND 1,96,10,6:SOUND 2,81,12,4:SOUND 3,64,2,4\n"
    "20 FOR I=1 TO 25:NEXT I\n"
    "30 POKE 53768,80:POKE 53760,40:POKE 53762,1:POKE 53761,0:POKE 53763,170\n"
    "40 FOR I=1 TO 25:NEXT I\n"
    "50 POKE 53768,4:POKE 53760,50:POKE 53761,170:POKE 53764,55:POKE 53765,160\n"
    "60 FOR I=1 TO 25:NEXT I\n"
    "70 POKE 53768,1:POKE 53760,20:POKE 53761,136:POKE 53762,30:POKE 53763,72\n"
    "80 FOR I=1 TO 25:NEXT I\n"
    "90 POKE 53768,0:SOUND 0,0,0,0:SOUND 1,0,0,0:SOUND 2,0,0,0:SOUND 3,0,0,0\n"
    "100 GOTO 10\n"
    "RUN\n";

static Machine *novy() {
  Machine *m = new Machine();
  m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit();
  m->consol = 7;          // BASIC zapnuty
  m->cpu.reset();
  return m;
}

int main(int argc, char **argv) {
  const std::string pref = argc > 1 ? argv[1] : "dump";
  const int SNIMKU = 1500;
  Machine *a = novy();
#ifndef NAP_STARY
  Machine *b = novy();
#endif
  TypeQueue qa, qb;
  std::vector<float> mono((size_t)SNIMKU * 882), L((size_t)SNIMKU * 882), R((size_t)SNIMKU * 882);
  for (int f = 0; f < SNIMKU; f++) {
    if (f == 120) { qa.addText(PROGRAM); qb.addText(PROGRAM); }
    qa.step(*a);
    a->runFrame();
    a->genAudio(mono.data() + (size_t)f * 882, 882, 44100.0);
#ifndef NAP_STARY
    qb.step(*b);
    b->runFrame();
    b->genAudioStereo(L.data() + (size_t)f * 882, R.data() + (size_t)f * 882, 882, 44100.0);
#endif
  }
  auto zapis = [&](const std::string &jm, const std::vector<float> &v) {
    FILE *o = std::fopen(jm.c_str(), "wb");
    if (!o) return;
    std::fwrite(v.data(), sizeof(float), v.size(), o);
    std::fclose(o);
  };
  zapis(pref + ".mono", mono);
#ifndef NAP_STARY
  zapis(pref + ".L", L);
  zapis(pref + ".R", R);
  std::printf("dump %s: %d snimku, PC=$%04X, klapani reproduktoru %lld, zapisu do POKEY 2: %lld, stereo %d\n", pref.c_str(), SNIMKU, a->cpu.pc,
              a->gtiaKlikPocitadlo, b->pk2.zapisu, b->stereoAktivni() ? 1 : 0);
#else
  std::printf("dump %s: %d snimku, PC=$%04X, klapani reproduktoru %lld (stare jadro)\n", pref.c_str(), SNIMKU, a->cpu.pc, a->gtiaKlikPocitadlo);
#endif
  return 0;
}
