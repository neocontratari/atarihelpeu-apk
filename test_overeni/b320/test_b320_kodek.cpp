// test_overeni/b320/test_b320_kodek.cpp
// ====================================================================
//  B320: POSUN OBRAZU A POLOVICNI ROZLISENI I PRO HRU S DITHERINGEM (Dune 2000)
//  (skutecny Hub + KodekJpeg z appky, "prohlizec" v C++ = stranka WEB TV B319 - beze zmeny)
//
//  Log B319 z telefonu (Dune 2000 po F5): posun obrazu zkousen 322x, nalezen 0x; polovicni
//  rozliseni jen 304 casti z 1137. Umela mapa v testu B319 dithering nemela.
//  PS1 kresli textury s ditheringem (v logu "dithering ZAPNUTO"): mrizka 4x4 (gpu_neon
//  dither_table) je pevne na OBRAZOVCE - po posunu mapy ma tentyz kus mapy jine body
//  (+-1 odstin), ve 2x Enhanced Resolution se lisi i 4 body jednoho bodu hry.
//
//  Tady: stejna umela Dune jako B319 (hra_mapa.h), ale teren kresleny jako v gpu_neon:
//  textura x barva vrcholu 0x70 (= 7/8), + dithering podle bodu VE 2x OBRAZE (y & 3, x & 3,
//  presne tabulka z psx_gpu.c), oriznuti na 5 bitu, RGB565 jako jadro. Jednotky, kurzor a panel
//  bez ditheringu (sprity). Stejne snimky jdou do tri kodeku:
//    drive  = bez voleb (B318),  B319 = posun + polovicni PRESNE (tolerance 0),
//    B320   = nastavKodekPs1 (tolerance ditheringu 10)
//  Kontroly:
//   1) B319 na ditheringu posun nenajde, kdyz se mapa posune o lichy pocet bodu hry (jina faze
//      mrizky ditheringu - to je chyba z logu; o 2, 4 body nebo sikmo 1,1 je faze stejna)
//   2) B320 ho najde skoro vzdy, ve vsech smerech, i liche posuny
//   3) obraz v prohlizeci = snimek po kazdem paketu (nejhorsi ctverec jako drive, PSNR jako drive)
//   4) dat pri posunu mapy mnohem mene nez drive i nez B319
//   5) 2x obraz v polovicnim rozliseni (s toleranci) - skoro vsechny casti; vernost jako drive
//   6) po zastaveni se obraz zjemni (Q94) jako drive - i kusy z kopie s toleranci (presne)
//   7) 3D hra s ditheringem (Gouraud): zadna kopie, polovicni ne, data a vernost jako drive,
//      kodovani skoro stejne rychle
//   8) hra BEZ ditheringu (B319 umela Dune): posun jako B319, rovny posun bajt po bajtu stejne; sikmy
//      (novy pruh nahore i vlevo = tvar L) mene dat - uz ne obdelnik pres skoro cely obraz
//   9) Dune s ditheringem v puvodnim rozliseni 368x240 (bez Enhanced Resolution): posun i tak
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb
//       -I../b319 -o test_b320_kodek test_b320_kodek.cpp && ./test_b320_kodek
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
static std::string t(long long v) { return std::to_string(v); }

// druh kodeku
enum { DRIVE = 0, B319 = 1, B320 = 2 };
struct Proud {
  Hub<KodekJpeg> hub;
  int id = 0;
  ProhlizecB319 pr;
  std::vector<uint8_t> pk;
  double psnrMin = 99, psnrSoucet = 0, nejCtverec = 0, kodMs = 0, kodMax = 0;
  long long snimku = 0, bajtu = 0;
  std::string log;
  explicit Proud(int druh) : hub("PS1", 44100, 4, 3) {
    if (druh == B319) hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); k.tolerance = 0; }, "B319 presne");
    else hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); }, "B320 tolerance ditheringu");
    id = hub.otevri(druh == DRIVE ? 0 : (VOLBA_POSUN | VOLBA_POLOVICNI));
  }
  long long posli(const std::vector<uint32_t> &F, int W, int H, bool porovnat = true) {
    hub.snimek(F.data(), W, H);
    const auto t0 = std::chrono::steady_clock::now();
    const int r = hub.dalsi(id, 200, 0, pk);
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
  double nejCtverecKonec(const std::vector<uint32_t> &F, int W, int H) { return nejhorsiCtverec(F, pr.canvas, W, H); }
};

static void vypisLog(const std::string &s) {
  std::string l = s;
  for (size_t p; (p = l.find('\n')) != std::string::npos; l.erase(0, p + 1)) std::printf("   log: %s\n", l.substr(0, p).c_str());
}

// ---- PS1 dithering jako gpu_neon (psx_gpu.c dither_table, radek y & 3, sloupec x & 3) ----
static const int DITHER[4][4] = {{-4, 0, -3, 1}, {2, -2, 3, -1}, {-3, 1, -4, 0}, {3, -1, 2, -2}};
// jeden kanal: 8bitova hodnota (uz po modulaci) + dither -> 5 bitu -> 8 bitu jako RGB565 z jadra
static inline int kanal(int v8, int d) {
  int v = v8 + d;
  v = v < 0 ? 0 : (v > 255 ? 255 : v);
  const int v5 = v >> 3;
  return (v5 << 3) | (v5 >> 2);
}
// textura (15 bit, expandovana) x barva vrcholu 0x70/0x80 + dithering v bode (x, y) 2x obrazu
static inline uint32_t texDither(uint32_t texel, int x, int y) {
  const int d = DITHER[y & 3][x & 3];
  const int r = (int)((texel >> 16) & 255) * 7 / 8, g = (int)((texel >> 8) & 255) * 7 / 8, b = (int)(texel & 255) * 7 / 8;
  return ((uint32_t)kanal(r, d) << 16) | ((uint32_t)kanal(g, d) << 8) | (uint32_t)kanal(b, d);
}
// bez ditheringu (sprity: jednotky, kurzor, panel) - jen 5 bitu jako jadro
static inline uint32_t bezDitheru(uint32_t c) {
  return ((uint32_t)kanal((int)((c >> 16) & 255), 0) << 16) | ((uint32_t)kanal((int)((c >> 8) & 255), 0) << 8) | (uint32_t)kanal((int)(c & 255), 0);
}

// Dune s ditheringem: snimek 736x480 (2x, m = 2) nebo 368x240 (m = 1). Teren (body vyhledu, ktere
// jsou z mapy) s ditheringem podle bodu obrazu (jako gpu_neon - v Enhanced Resolution podle bodu 2x
// obrazu, 2x2 body nejsou stejne), zbytek bez.
static void kresliDither(HraMapa &hra, std::vector<uint32_t> &F, int &W, int &H, int m = 2) {
  std::vector<uint32_t> obr;
  int w1, h1;
  hra.kresli(obr, w1, h1, false);           // 368x240 (body hry)
  W = w1 * m; H = h1 * m;
  F.resize((size_t)W * H);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      const int hx = x / m, hy = y / m;
      const uint32_t c = obr[(size_t)hy * w1 + hx];
      const bool teren = hx < HraMapa::VW && c == hra.svet[(size_t)(hra.camY + hy) * HraMapa::MW + hra.camX + hx];
      F[(size_t)y * W + x] = teren ? texDither(c, x, y) : bezDitheru(c);
    }
}

// "3D hra" s ditheringem (Gouraud + textura, otaceni, zoom) - nic se presne neposouva
static void kresli3D(std::vector<uint32_t> &F, int W, int H, int t) {
  F.resize((size_t)W * H);
  const double a = t * 0.031, z = 1.0 + 0.3 * std::sin(t * 0.05), ca = std::cos(a) / z, sa = std::sin(a) / z;
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      const double u = (x - W / 2) * ca - (y - H / 2) * sa + 1000, v = (x - W / 2) * sa + (y - H / 2) * ca + 1000;
      const int iu = (int)u, iv = (int)v;
      const double c = ((((iu >> 4) ^ (iv >> 4)) & 1) ? 200.0 : 60.0 + ((iu * 7 + iv * 3) & 31));
      const int r = (int)c, g = (int)(c * 0.8 + y * 40.0 / H), b = (int)(255 - c * 0.5 + x * 30.0 / W);
      const int d = DITHER[y & 3][x & 3];
      F[(size_t)y * W + x] = ((uint32_t)kanal(r, d) << 16) | ((uint32_t)kanal(g, d) << 8) | (uint32_t)kanal(b, d);
    }
}

int main() {
  std::printf("B320 - PS1 na TV / PC: posun obrazu a polovicni rozliseni i s ditheringem PS1 (Dune 2000)\n");
  std::printf("       (Hub + KodekJpeg z appky, prohlizec v C++ podle stranky WEB TV B319 - stranka se nemeni)\n\n");

  // ================= A) Dune s ditheringem v 2x Enhanced Resolution =================
  {
    HraMapa hra;
    hra.vytvor(7);
    Proud drive(DRIVE), b319(B319), b320(B320);
    struct Faze { const char *jmeno; int snimku, dx, dy; };
    // posun v BODECH HRY za obraz hry (20 / s) - liche posuny = jina faze ditheringu
    const Faze faze[] = {
        {"mapa stoji", 15, 0, 0},
        {"doprava 1 bod", 30, 1, 0},
        {"dolu 1", 30, 0, 1},
        {"sikmo 1,1", 25, 1, 1},
        {"doleva 3", 25, -3, 0},
        {"nahoru 2", 25, 0, -2},
        {"sikmo 3,-1", 25, 3, -1},
        {"doprava 4", 25, 4, 0},
        {"mapa stoji", 20, 0, 0},
    };
    std::vector<uint32_t> F;
    int W = 0, H = 0, celkem = 0;
    long long bPosun[3] = {0, 0, 0}, bStoji[3] = {0, 0, 0};
    int snPosun = 0, snStoji = 0, sKopii[3] = {0, 0, 0}, moznych = 0;
    int lichKopii319 = 0, lichMoznych = 0, nejhorsiFaze = 100;
    double kodPosun[3] = {0, 0, 0};
    std::string radky;
    for (const Faze &f : faze) {
      int fazeKopii[3] = {0, 0, 0}, fazeMoznych = 0;
      // jina faze ditheringu ve 2x obrazu: posun o lichy pocet bodu hry jen v jednom smeru
      // (o 1 bod hry = 2 body obrazu; mrizka PS1 je stejna po posunu o (2, 2) a o 4)
      const bool jinaFaze = ((f.dx & 1) != (f.dy & 1));
      for (int i = 0; i < f.snimku; i++, celkem++) {
        hra.kurX = 150 + (int)(60 * std::sin(celkem * 0.11));
        hra.kurY = 120 + (int)(40 * std::cos(celkem * 0.07));
        hra.krok(f.dx, f.dy);
        kresliDither(hra, F, W, H);
        const bool posun = f.dx != 0 || f.dy != 0;
        Proud *p[3] = {&drive, &b319, &b320};
        for (int k = 0; k < 3; k++) {
          const double k0 = p[k]->kodMs;
          const long long b = p[k]->posli(F, W, H);
          if (posun) { bPosun[k] += b; kodPosun[k] += p[k]->kodMs - k0; } else bStoji[k] += b;
          if (posun && i > 0 && p[k]->pr.kopiiPoslednihoSnimku > 0) { sKopii[k]++; fazeKopii[k]++; }
        }
        if (posun) { snPosun++; if (i > 0) { moznych++; fazeMoznych++; } } else snStoji++;
      }
      if (f.dx || f.dy) {
        char b[200];
        snprintf(b, sizeof(b), "      %-16s kopie: B319 %2d z %2d, B320 %2d z %2d%s\n", f.jmeno, fazeKopii[B319], fazeMoznych, fazeKopii[B320], fazeMoznych,
                 jinaFaze ? "   (jina faze ditheringu)" : "");
        radky += b;
        if (jinaFaze) { lichKopii319 += fazeKopii[B319]; lichMoznych += fazeMoznych; }
        nejhorsiFaze = std::min(nejhorsiFaze, 100 * fazeKopii[B320] / std::max(1, fazeMoznych));
      }
    }
    // zastaveni: zjemneni
    kresliDither(hra, F, W, H);
    for (int i = 0; i < 20; i++) { drive.posli(F, W, H); b319.posli(F, W, H); b320.posli(F, W, H); }
    const double psD = psnrB(F, drive.pr.canvas, W, 0, 0, W, H), ps20 = psnrB(F, b320.pr.canvas, W, 0, 0, W, H);
    drive.zavri(); b319.zavri();
    const std::string log20 = b320.zavri();
    std::printf("   A) Dune s ditheringem 736x480 (2x), %d obrazu hry:\n%s", celkem, radky.c_str());
    std::printf("      posun mapy: drive %.1f kB/obraz, B319 %.1f kB, B320 %.1f kB (%.0f %% drivejska)\n", bPosun[0] / 1024.0 / snPosun,
                bPosun[1] / 1024.0 / snPosun, bPosun[2] / 1024.0 / snPosun, 100.0 * bPosun[2] / bPosun[0]);
    std::printf("      pri 20 obrazech / s: drive %.2f Mbit/s, B319 %.2f Mbit/s, B320 %.2f Mbit/s\n", bPosun[0] * 8.0 / 1e6 / snPosun * 20,
                bPosun[1] * 8.0 / 1e6 / snPosun * 20, bPosun[2] * 8.0 / 1e6 / snPosun * 20);
    std::printf("      mapa stoji: drive %.1f kB, B319 %.1f kB, B320 %.1f kB\n", bStoji[0] / 1024.0 / snStoji, bStoji[1] / 1024.0 / snStoji,
                bStoji[2] / 1024.0 / snStoji);
    std::printf("      vernost PSNR prumer / nejhur / nejhorsi ctverec: drive %.2f / %.2f / %.1f, B319 %.2f / %.2f / %.1f, B320 %.2f / %.2f / %.1f\n",
                drive.psnrSoucet / drive.snimku, drive.psnrMin, drive.nejCtverec, b319.psnrSoucet / b319.snimku, b319.psnrMin, b319.nejCtverec,
                b320.psnrSoucet / b320.snimku, b320.psnrMin, b320.nejCtverec);
    std::printf("      polovicni rozliseni: B319 %lld z %lld casti, B320 %lld z %lld\n", b319.pr.polovicnich, b319.pr.casti, b320.pr.polovicnich, b320.pr.casti);
    std::printf("      kodovani pri posunu: drive %.2f ms, B319 %.2f ms, B320 %.2f ms (nejdele %.1f ms)\n", kodPosun[0] / snPosun, kodPosun[1] / snPosun,
                kodPosun[2] / snPosun, b320.kodMax);
    vypisLog(log20);
    over(drive.pr.chyb == 0 && b319.pr.chyb == 0 && b320.pr.chyb == 0 && b320.pr.spatnyPriznak == 0 && b320.pr.zvukVDalsi == 0,
         "A0) vsechny pakety jdou rozebrat jako ve strance B319 (B320 " + t(b320.pr.paketu) + " paketu)");
    over(lichMoznych > 0 && lichKopii319 * 10 <= lichMoznych,
         "A1) chyba z logu B319 zopakovana: pri jine fazi ditheringu najde presne porovnani kopii jen v " + t(lichKopii319) + " z " + t(lichMoznych) + " obrazu posunu");
    over(sKopii[B320] * 10 >= moznych * 9 && nejhorsiFaze >= 85,
         "A2) B320 (tolerance ditheringu) najde posun v " + t(sKopii[B320]) + " z " + t(moznych) + " obrazu, v kazde fazi aspon " + t(nejhorsiFaze) + " % - i liche posuny");
    over(b320.nejCtverec <= drive.nejCtverec * 1.25 + 1.0 && b320.psnrMin >= drive.psnrMin - 1.0 && b320.psnrSoucet / b320.snimku >= drive.psnrSoucet / drive.snimku - 0.5,
         "A3) obraz v prohlizeci = snimek po kazdem paketu: nejhorsi ctverec " + f1(b320.nejCtverec) + " (drive " + f1(drive.nejCtverec) + "), PSNR prumer " +
             f2(b320.psnrSoucet / b320.snimku) + " dB (drive " + f2(drive.psnrSoucet / drive.snimku) + "), nejhur " + f2(b320.psnrMin) + " (drive " + f2(drive.psnrMin) + ")");
    over(bPosun[B320] * 10 <= bPosun[DRIVE] * 4 && bPosun[B320] * 10 <= bPosun[B319] * 6,
         "A4) dat pri posunu mapy: B320 " + t(100 * bPosun[B320] / bPosun[DRIVE]) + " % drivejska (<= 40 %), " + t(100 * bPosun[B320] / bPosun[B319]) + " % B319 (<= 60 %)");
    over(b320.pr.polovicnich * 10 >= b320.pr.casti * 8,
         "A5) 2x obraz s ditheringem v polovicnim rozliseni: " + t(b320.pr.polovicnich) + " z " + t(b320.pr.casti) + " casti (B319 " + t(b319.pr.polovicnich) + " z " + t(b319.pr.casti) + ")");
    over(ps20 >= psD - 1.0 && ps20 >= 34.0 && b320.nejCtverecKonec(F, W, H) <= drive.nejCtverecKonec(F, W, H) + 1.0,
         "A6) po zastaveni se obraz zjemni jako drive: " + f2(ps20) + " dB (drive " + f2(psD) + "), nejhorsi ctverec " +
             f1(b320.nejCtverecKonec(F, W, H)) + " (drive " + f1(drive.nejCtverecKonec(F, W, H)) + ") - i kusy z kopie s toleranci");
    over(log20.find("B320: tolerance ditheringu 10 - posunu nalezenych jen s toleranci") != std::string::npos,
         "A7) log Hubu: \"B320: tolerance ditheringu 10 - posunu nalezenych jen s toleranci ..., shodnych jen s toleranci: ctvercu ...\"");
  }

  // ================= B) 3D hra s ditheringem =================
  {
    Proud drive(DRIVE), b320(B320);
    std::vector<uint32_t> F;
    long long b0 = 0, b1 = 0;
    const int W = 640, H = 480, N = 120;
    for (int i = 0; i < N; i++) {
      kresli3D(F, W, H, i);
      b0 += drive.posli(F, W, H); b1 += b320.posli(F, W, H);
    }
    drive.zavri(); b320.zavri();
    std::printf("   B) 3D s ditheringem 640x480: drive %.1f kB, B320 %.1f kB; kopii %lld, polovicnich %lld; PSNR %.2f / %.2f dB; kodovani %.2f / %.2f ms\n",
                b0 / 1024.0 / N, b1 / 1024.0 / N, b320.pr.kopii, b320.pr.polovicnich, drive.psnrSoucet / drive.snimku, b320.psnrSoucet / b320.snimku,
                drive.kodMs / drive.snimku, b320.kodMs / b320.snimku);
    over(b320.pr.chyb == 0 && b320.pr.kopii == 0 && b320.pr.polovicnich == 0 && b1 <= b0 * 1.02 + 2048 &&
             b320.psnrSoucet / b320.snimku >= drive.psnrSoucet / drive.snimku - 0.2,
         "B8) 3D hra s ditheringem: zadna kopie, zadne polovicni rozliseni, data a vernost jako drive");
    over(b320.kodMs / b320.snimku <= drive.kodMs / drive.snimku * 1.2 + 0.4,
         "B9) 3D hra: kodovani skoro stejne rychle: " + f2(b320.kodMs / b320.snimku) + " ms (drive " + f2(drive.kodMs / drive.snimku) + " ms)");
  }

  // ================= C) hra bez ditheringu (umela Dune z B319) =================
  // posun stejne jako B319; jen sikmy posun (novy pruh nahore i vlevo = tvar L) uz nejde JPEGem
  // jako obdelnik pres skoro cely obraz (B320 rozdelSkupinu)
  {
    HraMapa hra;
    hra.vytvor(7);
    Proud b319(B319), b320(B320);
    std::vector<uint32_t> F;
    int W = 0, H = 0;
    long long ruznych = 0, n = 0, d319 = 0, d320 = 0, rovne = 0, rovneStejne = 0;
    for (int i = 0; i < 160; i++) {
      hra.kurX = 150 + (int)(60 * std::sin(i * 0.11));
      hra.krok(i < 60 ? 8 : (i < 100 ? 0 : -4), i < 40 ? 4 : 0);
      hra.kresli(F, W, H, true);
      d319 += b319.posli(F, W, H);
      const std::vector<uint8_t> a = b319.pk;
      d320 += b320.posli(F, W, H);
      n++;
      if (a != b320.pk) ruznych++;
      if (i >= 40) { rovne++; if (a == b320.pk) rovneStejne++; }   // bez sikmeho posunu
    }
    b319.zavri(); b320.zavri();
    std::printf("   C) umela Dune BEZ ditheringu (B319): %lld paketu, lisi se %lld (bez sikmeho posunu %lld z %lld stejnych); kopii B319 %lld, B320 %lld; dat B319 %.1f kB, B320 %.1f kB; nejhorsi ctverec %.1f / %.1f\n",
                n, ruznych, rovneStejne, rovne, b319.pr.kopii, b320.pr.kopii, d319 / 1024.0 / n, d320 / 1024.0 / n, b319.nejCtverec, b320.nejCtverec);
    // (par paketu hned po sikmem posunu se lisi i pri rovnem - prohlizec ma po nem jine kusy v jine kvalite)
    over(b320.pr.chyb == 0 && b320.pr.kopii == b319.pr.kopii && rovneStejne * 10 >= rovne * 9 && d320 <= d319 &&
             b320.nejCtverec <= b319.nejCtverec + 0.5 && b320.psnrMin >= b319.psnrMin - 0.3,
         "C10) hra bez ditheringu: posun jako B319 (kopii " + t(b320.pr.kopii) + "), rovny posun skoro vzdy bajt po bajtu stejne (" + t(rovneStejne) + " z " + t(rovne) +
             "), sikmy mene dat: celkem " + t(100 * d320 / d319) + " % B319, vernost stejna");
  }

  // ================= D) Dune s ditheringem v puvodnim rozliseni 368x240 =================
  {
    HraMapa hra;
    hra.vytvor(11);
    Proud drive(DRIVE), b319(B319), b320(B320);
    std::vector<uint32_t> F;
    int W = 0, H = 0, moznych = 0, k319 = 0, k320 = 0;
    long long b0 = 0, b2 = 0;
    const int smery[6][2] = {{1, 0}, {0, 1}, {-1, -1}, {2, 0}, {-3, 1}, {0, -2}};
    for (int f = 0; f < 6; f++)
      for (int i = 0; i < 25; i++) {
        hra.kurX = 150 + (int)(60 * std::sin(i * 0.11));
        hra.kurY = 120;
        hra.krok(smery[f][0], smery[f][1]);
        kresliDither(hra, F, W, H, 1);
        b0 += drive.posli(F, W, H);
        b319.posli(F, W, H);
        b2 += b320.posli(F, W, H);
        if (i > 0) {
          moznych++;
          if (b319.pr.kopiiPoslednihoSnimku > 0) k319++;
          if (b320.pr.kopiiPoslednihoSnimku > 0) k320++;
        }
      }
    drive.zavri(); b319.zavri(); b320.zavri();
    std::printf("   D) Dune s ditheringem 368x240 (bez Enhanced Resolution): kopie B319 %d z %d, B320 %d z %d; dat drive %.1f kB, B320 %.1f kB; nejhorsi ctverec %.1f (drive %.1f)\n",
                k319, moznych, k320, moznych, b0 / 1024.0 / 150, b2 / 1024.0 / 150, b320.nejCtverec, drive.nejCtverec);
    over(b320.pr.chyb == 0 && k320 * 10 >= moznych * 9 && b2 * 10 <= b0 * 6 && b320.nejCtverec <= drive.nejCtverec * 1.25 + 1.0,
         "D11) puvodni rozliseni s ditheringem: posun v " + t(k320) + " z " + t(moznych) + " obrazu (B319 " + t(k319) + "), dat " + t(100 * b2 / b0) +
             " % drivejska, nejhorsi ctverec " + f1(b320.nejCtverec) + " (drive " + f1(drive.nejCtverec) + ")");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
