// test_overeni/b319/test_b319_kodek.cpp
// ====================================================================
//  B319: PS1 NA TV / PC - POSUN OBRAZU A 2x ZVETSENY OBRAZ V PUVODNIM ROZLISENI
//  (skutecny Hub + KodekJpeg z appky, "prohlizec" v C++ = stranka WEB TV B319)
//
//  Rene: "mam pocit, ze doslo ke zpomaleni pri prenosu LAN". Log B318: Dune 2000 na PC 48-57
//  snimku/s misto 59, pri posunu mapy cely obraz 736x480 20x za sekundu, ~35 kB na snimek,
//  az 6 Mbit/s - Wi-Fi nestihala.
//
//  Synteticka 2D strategie jako Dune 2000 (hra_mapa.h: obraz 368x240, mapa 296x240 + panel,
//  pisek se zrnem, skaly, koreni, budovy, jednotky, kurzor), 2x Enhanced Resolution = kazdy
//  bod 2x2 (736x480). Stejne snimky jdou do kodeku jako drive (B318: bez voleb) a s volbami
//  B319 (posun + polovicni); oba proudy rozebere "prohlizec" (prohlizec_b319.h) a obraz
//  porovna s puvodnim snimkem.
//  Kontroly:
//   1) pakety jdou rozebrat, dalsi casti s priznakem 8 a bez zvuku
//   2) obraz v prohlizeci = snimek po KAZDEM paketu (zadna spatna kopie: nejhorsi ctverec 16x16
//      jako drive), vernost (PSNR) jako drive
//   3) pri posunu mapy jde kopie skoro v kazdem snimku a dat je mnohem mene nez drive
//   4) prohlizec vynechal snimky (Wi-Fi) - posun 2x / 3x vetsi se najde taky
//   5) 2x zvetseny obraz jde v polovicnim rozliseni (skoro vsechny casti)
//   6) mapa stoji (jednotky, kurzor): dat ne vic nez drive
//   7) po zastaveni se obraz zjemni (kvalita 94) jako drive
//   8) 1x (bez Enhanced Resolution): kopie ano, polovicni rozliseni ne
//   9) "3D hra" (otaceni, zoom, prechody - nic se presne neposouva, nic neni zdvojene):
//      zadna kopie, zadne polovicni rozliseni, data a vernost jako drive, hledani jen obcas
//  10) opakujici se vzor (sachovnice, pruhy) a strih sceny: obraz porad spravne
//  11) zmena rozliseni (736x480 -> video 320x240 -> 736x480) a lichy rozmer 257x193
//  12) rychlost kodovani s volbami B319 (posun mapy) proti drivejsku
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb
//       -o test_b319_kodek test_b319_kodek.cpp && ./test_b319_kodek
// ====================================================================
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "stb_image.h"

#include "prohlizec_b319.h"
#include "hra_mapa.h"

using namespace nap::proud;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string f1(double v) { char b[32]; snprintf(b, sizeof(b), "%.1f", v); return b; }
static std::string f2(double v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); return b; }

// jeden prohlizec: Hub (jako v appce) + rozbor paketu + porovnani se snimkem
struct Proud {
  Hub<KodekJpeg> hub;
  int id = 0;
  ProhlizecB319 pr;
  std::vector<uint8_t> pk;
  double psnrMin = 99, psnrSoucet = 0, nejCtverec = 0, kodMs = 0, kodMax = 0;
  long long snimku = 0, bajtu = 0;
  std::string log;
  explicit Proud(int volby, int pomerX = 4, int pomerY = 3) : hub("PS1", 44100, pomerX, pomerY) {
    hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); }, "posun + polovicni");
    id = hub.otevri(volby);
  }
  // posle snimek; vraci bajty paketu (0 = nic)
  long long posli(const std::vector<uint32_t> &F, int W, int H, bool porovnat = true, int zapisMs = 0) {
    hub.snimek(F.data(), W, H);
    const auto t0 = std::chrono::steady_clock::now();
    const int r = hub.dalsi(id, 200, zapisMs, pk);
    const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    if (r != 1) { pr.chyb++; return 0; }
    kodMs += ms; if (ms > kodMax) kodMax = ms;
    pr.prijmi(pk);
    snimku++; bajtu += (long long)pk.size();
    if (porovnat) {
      const double p = psnrB(F, pr.canvas, W, 0, 0, W, H);
      psnrMin = std::min(psnrMin, p); psnrSoucet += p;
      nejCtverec = std::max(nejCtverec, nejhorsiCtverec(F, pr.canvas, W, H));
    }
    return (long long)pk.size();
  }
  std::string zavri() { hub.zavri(id); log = hub.vezmiLog(); return log; }
};

static void vypisLog(const std::string &s) {
  std::string l = s;
  for (size_t p; (p = l.find('\n')) != std::string::npos; l.erase(0, p + 1)) std::printf("   log: %s\n", l.substr(0, p).c_str());
}

// ---- "3D hra": otaceni + zoom textury s bilinearnim vzorkovanim, prechod barev - nic neni zdvojene ----
static void kresli3D(std::vector<uint32_t> &F, int W, int H, int t) {
  F.resize((size_t)W * H);
  const double a = t * 0.031, z = 1.0 + 0.3 * std::sin(t * 0.05), ca = std::cos(a) / z, sa = std::sin(a) / z;
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      const double u = (x - W / 2) * ca - (y - H / 2) * sa + 1000, v = (x - W / 2) * sa + (y - H / 2) * ca + 1000;
      const int iu = (int)u, iv = (int)v;
      const double fu = u - iu, fv = v - iv;
      auto tx = [](int uu, int vv) { return (((uu >> 4) ^ (vv >> 4)) & 1) ? 200.0 : 60.0 + ((uu * 7 + vv * 3) & 31); };
      const double c = tx(iu, iv) * (1 - fu) * (1 - fv) + tx(iu + 1, iv) * fu * (1 - fv) + tx(iu, iv + 1) * (1 - fu) * fv + tx(iu + 1, iv + 1) * fu * fv;
      const int r = (int)c, g = (int)(c * 0.8 + y * 40.0 / H), b = (int)(255 - c * 0.5 + x * 30.0 / W);
      F[(size_t)y * W + x] = HraMapa::c15(r, g, b);
    }
}

// ---- opakujici se vzor: sachovnice 8x8 a pruhy, posun ----
static void kresliVzor(std::vector<uint32_t> &F, int W, int H, int ox, int oy, int druh) {
  F.resize((size_t)W * H);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      const int xx = x + ox, yy = y + oy;
      uint32_t c;
      if (druh == 0) c = (((xx >> 3) ^ (yy >> 3)) & 1) ? 0xF8F8F8u : 0x101010u;
      else c = ((xx / 6) % 3 == 0) ? 0xF83000u : (((xx / 6) % 3 == 1) ? 0x00F800u : 0x2020F8u);
      F[(size_t)y * W + x] = c;
    }
}

int main() {
  std::printf("B319 - PS1 na TV / PC: posun obrazu (kopie v prohlizeci) a 2x zvetseny obraz v puvodnim rozliseni\n");
  std::printf("       (Hub + KodekJpeg z appky, prohlizec v C++ podle stranky WEB TV B319)\n\n");

  // ================= A) "Dune 2000" v 2x Enhanced Resolution: posun mapy =================
  {
    HraMapa hra;
    hra.vytvor(7);
    Proud stary(0), novy(VOLBA_POSUN | VOLBA_POLOVICNI);
    struct Faze { const char *jmeno; int snimku, dx, dy; bool vynechavat; };
    const Faze faze[] = {
        {"mapa stoji (jednotky, kurzor)", 20, 0, 0, false},
        {"posun doprava 8 b/snimek", 40, 8, 0, false},
        {"posun dolu 8", 30, 0, 8, false},
        {"posun sikmo -8,-8", 30, -8, -8, false},
        {"posun doleva 16 (prohlizec obcas vynecha snimek)", 20, -16, 0, true},
        {"posun nahoru 16 (prohlizec obcas vynecha snimek)", 20, 0, -16, true},
        {"mapa stoji", 40, 0, 0, false},
        {"maly posun 4,2", 30, 4, 2, false},
        {"mapa stoji", 20, 0, 0, false},
    };
    std::vector<uint32_t> F;
    int W = 0, H = 0, celkem = 0;
    long long bStaryPosun = 0, bNovyPosun = 0, bStaryStoji = 0, bNovyStoji = 0;
    int snPosun = 0, snStoji = 0, snPosunSKopii = 0, snPosunMoznych = 0, vynechano = 0;
    double kodStaryPosun = 0, kodNovyPosun = 0;
    for (const Faze &f : faze) {
      for (int i = 0; i < f.snimku; i++, celkem++) {
        hra.kurX = 150 + (int)(60 * std::sin(celkem * 0.11));
        hra.kurY = 120 + (int)(40 * std::cos(celkem * 0.07));
        hra.krok(f.dx, f.dy);
        if (f.vynechavat && (i % 4) == 2) { vynechano++; continue; }   // prohlizec nestihl - snimek vynechan
        hra.kresli(F, W, H, true);
        const bool posun = f.dx != 0 || f.dy != 0;
        const double k0 = stary.kodMs, k1 = novy.kodMs;
        const long long b0 = stary.posli(F, W, H), b1 = novy.posli(F, W, H);
        if (posun) {
          bStaryPosun += b0; bNovyPosun += b1; snPosun++;
          kodStaryPosun += stary.kodMs - k0; kodNovyPosun += novy.kodMs - k1;
          if (i > 0) { snPosunMoznych++; if (novy.pr.kopiiPoslednihoSnimku > 0) snPosunSKopii++; }
        } else { bStaryStoji += b0; bNovyStoji += b1; snStoji++; }
      }
    }
    // zastaveni: nic se nehybe -> zjemneni na 94 (jako drive)
    hra.kresli(F, W, H, true);
    for (int i = 0; i < 20; i++) { stary.posli(F, W, H); novy.posli(F, W, H); }
    const double psStary = psnrB(F, stary.pr.canvas, W, 0, 0, W, H), psNovy = psnrB(F, novy.pr.canvas, W, 0, 0, W, H);
    const std::string logS = stary.zavri(), logN = novy.zavri();
    std::printf("   A) Dune-like 736x480 (2x), %d snimku hry, vynechano %d:\n", celkem, vynechano);
    std::printf("      posun mapy: drive %.1f kB/snimek, B319 %.1f kB/snimek (%.0f %%); kopie v %d z %d snimku posunu\n",
                bStaryPosun / 1024.0 / snPosun, bNovyPosun / 1024.0 / snPosun, 100.0 * bNovyPosun / bStaryPosun, snPosunSKopii, snPosunMoznych);
    std::printf("      mapa stoji: drive %.1f kB/snimek, B319 %.1f kB/snimek\n", bStaryStoji / 1024.0 / snStoji, bNovyStoji / 1024.0 / snStoji);
    std::printf("      pri 20 snimcich hry / s: drive %.2f Mbit/s, B319 %.2f Mbit/s (jen obraz, bez zvuku)\n",
                bStaryPosun * 8.0 / 1e6 / snPosun * 20, bNovyPosun * 8.0 / 1e6 / snPosun * 20);
    std::printf("      vernost: drive PSNR prumer %.2f dB (nejhur %.2f), nejhorsi ctverec %.1f; B319 prumer %.2f dB (nejhur %.2f), nejhorsi ctverec %.1f\n",
                stary.psnrSoucet / stary.snimku, stary.psnrMin, stary.nejCtverec, novy.psnrSoucet / novy.snimku, novy.psnrMin, novy.nejCtverec);
    std::printf("      kodovani pri posunu: drive %.2f ms/snimek, B319 %.2f ms/snimek (nejdele %.1f ms)\n",
                kodStaryPosun / snPosun, kodNovyPosun / snPosun, novy.kodMax);
    std::printf("      casti: JPEG %lld, z toho v polovicnim rozliseni %lld; kopii %lld (%.1f Mbodu)\n", novy.pr.casti, novy.pr.polovicnich, novy.pr.kopii, novy.pr.kopiiBodu / 1e6);
    vypisLog(logN);
    over(stary.pr.chyb == 0 && novy.pr.chyb == 0 && novy.pr.spatnyPriznak == 0 && novy.pr.zvukVDalsi == 0,
         "A1) pakety jdou rozebrat jako ve strance B319 (paketu " + std::to_string(novy.pr.paketu) + ", dalsich casti " + std::to_string(novy.pr.dalsich) +
             "), dalsi casti s priznakem 8 a bez zvuku");
    over(novy.nejCtverec <= stary.nejCtverec * 1.25 + 1.0 && novy.psnrMin >= stary.psnrMin - 1.0,
         "A2) obraz v prohlizeci = snimek po kazdem paketu: nejhorsi ctverec 16x16 " + f1(novy.nejCtverec) + " (drive " + f1(stary.nejCtverec) +
             "), nejhorsi PSNR " + f2(novy.psnrMin) + " dB (drive " + f2(stary.psnrMin) + ")");
    over(novy.psnrSoucet / novy.snimku >= stary.psnrSoucet / stary.snimku - 0.5,
         "A2b) vernost jako drive: prumer " + f2(novy.psnrSoucet / novy.snimku) + " dB (drive " + f2(stary.psnrSoucet / stary.snimku) + ")");
    over(snPosunSKopii * 10 >= snPosunMoznych * 9 && bNovyPosun * 10 <= bStaryPosun * 3,
         "A3) posun mapy: kopie v " + std::to_string(snPosunSKopii) + " z " + std::to_string(snPosunMoznych) + " snimku, dat " +
             std::to_string((int)(100.0 * bNovyPosun / bStaryPosun)) + " % proti drivejsku (<= 30 %)");
    over(logN.find("B319: posun obrazu") != std::string::npos && novy.pr.kopii >= (long long)snPosunSKopii,
         "A4) i kdyz prohlizec vynechal snimek (posun 2x vetsi), kopie jde - log: posun obrazu");
    over(novy.pr.polovicnich * 10 >= novy.pr.casti * 9,
         "A5) 2x zvetseny obraz jde v polovicnim rozliseni: " + std::to_string(novy.pr.polovicnich) + " z " + std::to_string(novy.pr.casti) + " casti");
    over(bNovyStoji <= bStaryStoji,
         "A6) mapa stoji (jednotky, kurzor): " + f1(bNovyStoji / 1024.0 / snStoji) + " kB/snimek, drive " + f1(bStaryStoji / 1024.0 / snStoji));
    over(psNovy >= 38.0 && psNovy >= psStary - 1.0,
         "A7) po zastaveni se obraz zjemni jako drive: " + f2(psNovy) + " dB (drive " + f2(psStary) + ")");
    over(kodNovyPosun <= kodStaryPosun * 1.2,
         "A12) kodovani pri posunu mapy ne pomalejsi nez drive: " + f2(kodNovyPosun / snPosun) + " ms (drive " + f2(kodStaryPosun / snPosun) + " ms)");
  }

  // ================= B) 1x (bez Enhanced Resolution) =================
  {
    HraMapa hra;
    hra.vytvor(11);
    Proud stary(0), novy(VOLBA_POSUN | VOLBA_POLOVICNI);
    std::vector<uint32_t> F;
    int W = 0, H = 0;
    long long b0 = 0, b1 = 0;
    for (int i = 0; i < 80; i++) {
      hra.krok(i < 40 ? 8 : 0, i < 40 ? 4 : 0);
      hra.kresli(F, W, H, false);
      b0 += stary.posli(F, W, H); b1 += novy.posli(F, W, H);
    }
    stary.zavri(); novy.zavri();
    std::printf("   B) 1x 368x240: drive %.1f kB/snimek, B319 %.1f kB/snimek, kopii %lld, polovicnich %lld, nejhorsi ctverec %.1f (drive %.1f)\n",
                b0 / 1024.0 / 80, b1 / 1024.0 / 80, novy.pr.kopii, novy.pr.polovicnich, novy.nejCtverec, stary.nejCtverec);
    over(novy.pr.chyb == 0 && novy.pr.kopii >= 30 && novy.pr.polovicnich == 0 && novy.nejCtverec <= stary.nejCtverec * 1.25 + 1.0 && b1 < b0,
         "B8) 1x: kopie ano (" + std::to_string(novy.pr.kopii) + "), polovicni rozliseni ne, obraz spravne, dat mene");
  }

  // ================= C) "3D hra" =================
  {
    Proud stary(0), novy(VOLBA_POSUN | VOLBA_POLOVICNI);
    std::vector<uint32_t> F;
    long long b0 = 0, b1 = 0;
    const int W = 640, H = 480, N = 90;
    for (int t = 0; t < N; t++) {
      kresli3D(F, W, H, t);
      b0 += stary.posli(F, W, H); b1 += novy.posli(F, W, H);
    }
    const std::string logN = novy.zavri();
    stary.zavri();
    std::printf("   C) 3D 640x480: drive %.1f kB/snimek, B319 %.1f kB/snimek, kopii %lld, polovicnich %lld; PSNR %.2f / %.2f dB; kodovani %.2f / %.2f ms\n",
                b0 / 1024.0 / N, b1 / 1024.0 / N, novy.pr.kopii, novy.pr.polovicnich, stary.psnrSoucet / stary.snimku, novy.psnrSoucet / novy.snimku,
                stary.kodMs / stary.snimku, novy.kodMs / novy.snimku);
    vypisLog(logN);
    const size_t p = logN.find("zkouseno ");
    const long long zkouseno = p == std::string::npos ? -1 : std::atoll(logN.c_str() + p + 9);
    over(novy.pr.chyb == 0 && novy.pr.kopii == 0 && novy.pr.polovicnich == 0 && b1 <= b0 * 1.02 + 2048 &&
             novy.psnrSoucet / novy.snimku >= stary.psnrSoucet / stary.snimku - 0.2,
         "C9) 3D hra: zadna kopie, zadne polovicni rozliseni, data a vernost jako drive");
    over(novy.kodMs / novy.snimku <= stary.kodMs / stary.snimku * 1.15 + 0.3,
         "C9b) 3D hra: kodovani skoro stejne rychle (hledani posunu jen obcas): " + f2(novy.kodMs / novy.snimku) + " ms (drive " +
             f2(stary.kodMs / stary.snimku) + " ms), log 'zkouseno' " + std::to_string(zkouseno));
  }

  // ================= D) opakujici se vzor, strih sceny =================
  // (sytě barevne pruhy maji v JPEG 4:2:0 velkou chybu i drive - srovnava se s drivejskem)
  {
    Proud stary(0), novy(VOLBA_POSUN | VOLBA_POLOVICNI);
    std::vector<uint32_t> F;
    const int W = 736, H = 480;
    auto obe = [&](int w, int h) { stary.posli(F, w, h); novy.posli(F, w, h); };
    for (int i = 0; i < 40; i++) { kresliVzor(F, W, H, i * 8, i * 2, 0); obe(W, H); }     // sachovnice
    for (int i = 0; i < 40; i++) { kresliVzor(F, W, H, i * 6, 0, 1); obe(W, H); }         // pruhy (perioda 18)
    HraMapa hra; hra.vytvor(3);
    int w2, h2;
    for (int i = 0; i < 10; i++) { hra.krok(8, 0); hra.kresli(F, w2, h2, true); obe(w2, h2); }
    HraMapa hra2; hra2.vytvor(99);                                                         // strih sceny
    for (int i = 0; i < 10; i++) { hra2.krok(0, 8); hra2.kresli(F, w2, h2, true); obe(w2, h2); }
    stary.zavri(); novy.zavri();
    std::printf("   D) vzor + strih: kopii %lld, nejhorsi ctverec %.1f (drive %.1f), nejhorsi PSNR %.2f dB (drive %.2f)\n", novy.pr.kopii, novy.nejCtverec,
                stary.nejCtverec, novy.psnrMin, stary.psnrMin);
    over(novy.pr.chyb == 0 && novy.nejCtverec <= stary.nejCtverec * 1.25 + 1.0 && novy.psnrMin >= stary.psnrMin - 1.0,
         "D10) opakujici se vzor a strih sceny: obraz porad spravne (nejhorsi ctverec " + f1(novy.nejCtverec) + ", drive " + f1(stary.nejCtverec) + ")");
  }

  // ================= E) zmena rozliseni, lichy rozmer =================
  {
    Proud stary(0), novy(VOLBA_POSUN | VOLBA_POLOVICNI);
    HraMapa hra; hra.vytvor(5);
    std::vector<uint32_t> F;
    int W, H;
    auto obe = [&](int w, int h) { stary.posli(F, w, h); novy.posli(F, w, h); };
    for (int i = 0; i < 15; i++) { hra.krok(8, 8); hra.kresli(F, W, H, true); obe(W, H); }
    for (int i = 0; i < 10; i++) { kresli3D(F, 320, 240, i); obe(320, 240); }               // video 320x240
    for (int i = 0; i < 15; i++) { hra.krok(-8, 0); hra.kresli(F, W, H, true); obe(W, H); }
    for (int i = 0; i < 20; i++) { kresliVzor(F, 257, 193, i * 3, i, 1); obe(257, 193); }    // lichy rozmer
    stary.zavri(); novy.zavri();
    std::printf("   E) 736x480 -> 320x240 -> 736x480 -> 257x193: chyb %lld, nejhorsi ctverec %.1f (drive %.1f), kopii %lld, polovicnich %lld\n",
                novy.pr.chyb, novy.nejCtverec, stary.nejCtverec, novy.pr.kopii, novy.pr.polovicnich);
    over(novy.pr.chyb == 0 && novy.nejCtverec <= stary.nejCtverec * 1.25 + 1.0 && novy.psnrMin >= stary.psnrMin - 1.0,
         "E11) zmena rozliseni a lichy rozmer: obraz porad spravne (nejhorsi ctverec " + f1(novy.nejCtverec) + ", drive " + f1(stary.nejCtverec) + ")");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
