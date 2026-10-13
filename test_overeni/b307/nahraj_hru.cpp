// test_overeni/b307/nahraj_hru.cpp
// ====================================================================
//  B307: zaznam zaberu ze hry Atari pro uvodni demo (Reneho Pitt-Kitt).
//
//  Hru pousti SKUTECNE jadro Atari 130XE z appky (nap_atari_machine.h,
//  stejne jako HELP v telefonu), vstupy podle scenare (START, joystick,
//  klavesy), a uklada:
//    pruzkum:  kazdy N-ty snimek jako PPM 384x240 (na prohlizeni)
//    zaznam:   snimky od..do (50 / s PAL) jako klip pro intro
//              (napdemo::KlipAtari - paleta + snimky jako rozdily, zlib)
//              + zvuk POKEY 44,1 kHz mono (16 bit) ze stejnych snimku
//
//  Pouziti:
//    nahraj_hru HRA.xex pruzkum ADR SNIMKU KAZDY "SCENAR"
//    nahraj_hru HRA.xex zaznam  KLIP.bin "OD-DO,OD-DO,..." "SCENAR" [ADR_PPM]
//      (useky snimku, ktere jdou do klipu - napr. titulek, kapitola, jizda)
//  SCENAR: "od-do:CO,..." CO = START SELECT OPTION FIRE NAHORU DOLU VLEVO
//          VPRAVO nebo K<kod> (klavesa, kod 0-63, +64 SHIFT)
// ====================================================================
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <zlib.h>
#include "nap_atari_machine.h"
#include "nap_atari_roms.h"
#include "nap_atari_runtime.h"
using namespace nap;

static std::vector<uint8_t> nacti(const char *f) {
  std::vector<uint8_t> d; FILE *fp = fopen(f, "rb"); if (!fp) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n); fclose(fp); return d;
}

struct Udalost { int od, doo; std::string co; };
static std::vector<Udalost> scenar(const char *s) {
  std::vector<Udalost> v;
  const char *p = s;
  while (p && *p) {
    Udalost u; u.od = atoi(p); while (*p && *p != '-' && *p != ':') p++;
    if (*p == '-') { p++; u.doo = atoi(p); while (*p && *p != ':') p++; } else u.doo = u.od;
    if (*p == ':') p++;
    while (*p && *p != ',') u.co.push_back(*p++);
    if (*p == ',') p++;
    v.push_back(u);
  }
  return v;
}

static void vstupy(Machine &m, const std::vector<Udalost> &sc, int f, bool zavadi) {
  int consol = 7, joy = 0, fire = 0;
  for (const auto &u : sc) {
    if (f < u.od || f > u.doo) continue;
    if (u.co == "START") consol &= ~1;
    else if (u.co == "SELECT") consol &= ~2;
    else if (u.co == "OPTION") consol &= ~4;
    else if (u.co == "FIRE") fire = 1;
    else if (u.co == "NAHORU") joy |= 1;
    else if (u.co == "DOLU") joy |= 2;
    else if (u.co == "VLEVO") joy |= 4;
    else if (u.co == "VPRAVO") joy |= 8;
    else if (u.co[0] == 'K' && f == u.od) m.klavesa(atoi(u.co.c_str() + 1));
  }
  if (!zavadi) m.consol = consol;
  m.porta = 0xF0 | (~joy & 15);
  m.trig[0] = fire ? 0 : 1;
}

static void ulozPpm(const Machine &m, const std::string &fn) {
  FILE *o = fopen(fn.c_str(), "wb"); if (!o) return;
  fprintf(o, "P6\n%d %d\n255\n", AnticView::W, AnticView::H);
  for (int i = 0; i < AnticView::W * AnticView::H; i++) {
    const uint32_t v = napAtariPalette(m.view->idx[i]);
    uint8_t px[3] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF), (uint8_t)((v >> 16) & 0xFF)};
    fwrite(px, 1, 3, o);
  }
  fclose(o);
}

int main(int argc, char **argv) {
  if (argc < 6) { fprintf(stderr, "pouziti: viz hlavicka souboru\n"); return 2; }
  std::vector<uint8_t> d = nacti(argv[1]);
  const std::string rezim = argv[2];
  Machine *m = new Machine();
  m->view = new AnticView();
  m->mem.os = NAP_OS_ROM; m->mem.bas = NAP_BASIC_ROM;
  m->coldInit();
  m->consol = 3;
  m->cpu.reset();
  XexLoader xl;
  if (!xl.priprav(d.data(), d.size(), argv[1])) { fprintf(stderr, "neni XEX\n"); return 1; }
  m->sioRychlyTimeout = true;

  if (rezim == "pruzkum") {
    const std::string adr = argv[3];
    const int snimku = atoi(argv[4]), kazdy = atoi(argv[5]);
    auto sc = scenar(argv[6]);
    for (int f = 1; f <= snimku; f++) {
      const bool zav = xl.aktivni();
      vstupy(*m, sc, f, zav);
      m->runFrame(); xl.poSnimku(*m);
      float zv[1000]; m->genAudio(zv, 882, 44100.0);
      if (f % kazdy == 0) ulozPpm(*m, adr + "/s_" + std::to_string(100000 + f).substr(1) + ".ppm");
    }
    for (auto &l : xl.log) printf("LOG %s\n", l.c_str());
    printf("PC=$%04X jam=%d\n", m->cpu.pc, m->cpu.jam);
    return 0;
  }

  if (rezim == "zaznam") {
    // klip: "NAPKLIP1", W, H, pocet, fps*100, vzorkovani, pocet vzorku, useku, paleta 256x3,
    //       useky (prvni snimek v klipu, pocet), pak pro kazdy snimek: delka zlib,
    //       zlib(snimek XOR minuly); nakonec zlib(zvuk int16 - jen zaznamenane snimky)
    const std::string klip = argv[3];
    std::vector<std::pair<int, int>> useky;
    for (const char *p = argv[4]; *p;) {
      int a = atoi(p); while (*p && *p != '-') p++; if (*p) p++;
      int b = atoi(p); while (*p && *p != ',') p++; if (*p) p++;
      useky.push_back({a, b});
    }
    int doo = 0;
    for (auto &u : useky) doo = std::max(doo, u.second);
    auto zaznamenat = [&](int f) { for (auto &u : useky) if (f >= u.first && f <= u.second) return true; return false; };
    auto sc = scenar(argv[5]);
    const std::string adrPpm = argc > 6 ? argv[6] : "";
    const int W = AnticView::W, H = AnticView::H;
    std::vector<uint8_t> minuly((size_t)W * H, 0), ted((size_t)W * H);
    std::vector<std::vector<uint8_t>> snimky;
    std::vector<int16_t> zvuk;
    double akum = 0;
    const double PAL = 1773447.0 / (312.0 * 114.0);      // snimku za vterinu (PAL)
    size_t surove = 0;
    for (int f = 1; f <= doo; f++) {
      const bool zav = xl.aktivni();
      vstupy(*m, sc, f, zav);
      m->runFrame(); xl.poSnimku(*m);
      akum += 44100.0 / PAL;
      const int n = (int)akum; akum -= n;
      float zv[1000]; m->genAudio(zv, n, 44100.0);
      if (!zaznamenat(f)) continue;
      for (int i = 0; i < n; i++) {
        float x = zv[i] * 0.9f;                            // jako v appce: <-1,1>
        if (x > 1.f) x = 1.f; if (x < -1.f) x = -1.f;
        zvuk.push_back((int16_t)(x * 32767.f));
      }
      std::memcpy(ted.data(), m->view->idx, ted.size());
      std::vector<uint8_t> x(ted.size());
      for (size_t i = 0; i < ted.size(); i++) x[i] = ted[i] ^ minuly[i];
      uLongf zl = compressBound(x.size());
      std::vector<uint8_t> z(zl);
      compress2(z.data(), &zl, x.data(), x.size(), 9);
      z.resize(zl);
      snimky.push_back(z);
      surove += x.size();
      minuly = ted;
      if (!adrPpm.empty()) ulozPpm(*m, adrPpm + "/z_" + std::to_string(100000 + f).substr(1) + ".ppm");
    }
    FILE *o = fopen(klip.c_str(), "wb");
    if (!o) return 1;
    auto u32 = [&](uint32_t v) { uint8_t b[4] = {(uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24)}; fwrite(b, 1, 4, o); };
    fwrite("NAPKLIP1", 1, 8, o);
    u32(W); u32(H); u32((uint32_t)snimky.size()); u32((uint32_t)(PAL * 100.0 + 0.5)); u32(44100); u32((uint32_t)zvuk.size());
    u32((uint32_t)useky.size());
    for (int i = 0; i < 256; i++) {
      const uint32_t v = napAtariPalette(i);
      uint8_t c[3] = {(uint8_t)(v & 0xFF), (uint8_t)((v >> 8) & 0xFF), (uint8_t)((v >> 16) & 0xFF)};
      fwrite(c, 1, 3, o);
    }
    {
      int prvni = 0;
      for (auto &u : useky) { u32((uint32_t)prvni); u32((uint32_t)(u.second - u.first + 1)); prvni += u.second - u.first + 1; }
    }
    size_t celkem = 0;
    for (auto &z : snimky) { u32((uint32_t)z.size()); fwrite(z.data(), 1, z.size(), o); celkem += z.size() + 4; }
    {
      uLongf zl = compressBound(zvuk.size() * 2);
      std::vector<uint8_t> z(zl);
      compress2(z.data(), &zl, (const Bytef *)zvuk.data(), zvuk.size() * 2, 9);
      u32((uint32_t)zl); fwrite(z.data(), 1, zl, o);
      celkem += zl + 4;
    }
    fclose(o);
    printf("KLIP %s: %zu snimku %dx%d (%.2f s pri %.3f/s), zvuk %zu vzorku, obraz %zu B surove -> %zu B celkem\n", klip.c_str(), snimky.size(),
           W, H, snimky.size() / PAL, PAL, zvuk.size(), surove, celkem);
    return 0;
  }
  fprintf(stderr, "neznamy rezim\n");
  return 2;
}
