// test_overeni/b335/test_b335_kodek.cpp
// ====================================================================
//  B335: PS1 NA TV - VODOROVNE CARKY V DUNE 2000 (skutecny Hub + KodekJpeg z appky, "prohlizec"
//  v C++ = stranka WEB TV; JPEG rozbali stb_image - barvy 4:2:0 dopocitava jako prohlizec:
//  "fancy upsampling", krajni radek ctverce dostane 1/4 barvy radku za hranici)
//
//  Rene (log B333, fotka B332): "v dune vodorovne carky zustaly ale mazou se" - slabe zelene carky na
//  pisku po ceste jednotek se zelenym pruhem zdravi, na radcich obrazu 16k-1.
//  Pricina (B334): cast JPEG 4:2:0 obsahuje i stojici ctverec nad jednotkou; jeho spodni radek dostane
//  1/4 zelene z pruhu pod hranici. Jednotka odejde, posle se jen jeji ctverec - stojici ctverec se
//  nezmenil, kodek ho neposle (myslí si, ze ho prohlizec ma presne) a zelena 1/4 zustane, dokud cely
//  obraz nestoji (zjemneni 94 ma barvy v plnem rozliseni) - proto se carky "mazou".
//  B335: casti do 55 000 bodu barvy 4:4:4; vetsi 4:2:0 + doplneni sousedu ze spolecneho JPEG.
//  Kontroly:
//   1) Dune 2x (736x480), slaba Wi-Fi (kvalita 55) i dobra (76): B334 carky na radcich 16k-1
//      (jako na fotce), B335 zadne; po zastaveni oboje cisto (B334 = "mazou se")
//   2) data B335 proti B334 (Dune) - nejvys +20 %, vernost (PSNR) ne horsi
//   3) 3D hra (snimky 320x240 a 640x480 z dema B304, kdyz jsou): data nejvys +25 %, PSNR ne horsi,
//      velke casti (cely obraz se meni) dal 4:2:0 - koder nema vic prace
//   4) velke casti 4:2:0 a pak male zmeny: B334 zelena 1/4 zustane, B335 doplni sousedy (cisto)
//   5) mapa s posunem (kopie v prohlizeci, B319) ve 2x i 1x: obraz po kazdem paketu spravne jako B334
//   6) rychlost kodovani B335 proti B334 (Dune, 3D)
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb
//       -I../b319 -I. -o test_b335_kodek test_b335_kodek.cpp && ./test_b335_kodek [PNG320 PNG640]
// ====================================================================
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "prohlizec_b319.h"
#include "hra_mapa.h"
#include "scena_dune.h"

using namespace nap::proud;

static int g_ok = 0, g_chyb = 0;
static FILE *g_out = nullptr;
static void tisk(const std::string &s) {
  std::printf("%s\n", s.c_str());
  std::fflush(stdout);
  if (g_out) std::fprintf(g_out, "%s\n", s.c_str());
}
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  tisk(std::string(ok ? "OK     " : "CHYBA  ") + co);
}
static std::string f1(double v) { char b[32]; snprintf(b, sizeof(b), "%.1f", v); return b; }
static std::string f2(double v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); return b; }

// carka = usek 8 bodu v radku, kde je prohlizec o hodne zelenejsi nez hra, a ve hre tam (ani 3 radky
// kolem) zadna zelena neni - fantom (JPEG sum na pisku dela nejvys ~17, carka z pruhu ~40-55)
struct Carky { int useku = 0, radek15 = 0; double nej = 0; };
static Carky carky(const std::vector<uint32_t> &C, const std::vector<uint32_t> &F, int W, int H, int x1) {
  Carky m;
  for (int y = 3; y < H - 3; y++)
    for (int x0 = 0; x0 + 8 <= x1; x0 += 8) {
      bool zelena = false;
      for (int yy = y - 3; yy <= y + 3 && !zelena; yy++)
        for (int x = x0; x < x0 + 8; x++) {
          const uint32_t p = F[(size_t)yy * W + x];
          if (((p >> 8) & 255) > ((p >> 16) & 255) + 20) { zelena = true; break; }
        }
      if (zelena) continue;
      double g = 0;
      for (int x = x0; x < x0 + 8; x++) {
        const uint32_t a = C[(size_t)y * W + x], b = F[(size_t)y * W + x];
        const double dr = (double)((a >> 16) & 255) - ((b >> 16) & 255), dg = (double)((a >> 8) & 255) - ((b >> 8) & 255),
                     db = (double)(a & 255) - (b & 255);
        g += dg - (dr + db) / 2;
      }
      g /= 8;
      if (g > m.nej) m.nej = g;
      if (g > 24) { m.useku++; if ((y & 15) == 15) m.radek15++; }
    }
  return m;
}

// jeden prohlizec: Hub (jako v appce) + rozbor paketu
struct Proud {
  Hub<KodekJpeg> hub;
  int id = 0;
  ProhlizecB319 pr;
  std::vector<uint8_t> pk;
  double kodMs = 0, psSoucet = 0;
  long long n = 0;
  long long casti444 = 0, sousedu = 0;
  std::string info;
  // b335 = false: jako B334 (plneBarvyDo 0)
  Proud(bool b335, int qStrop, int volby = 3) : hub("PS1", 44100) {
    hub.nastavKodek([qStrop](KodekJpeg &k) { if (qStrop > 0) k.qStrop = qStrop; });
    hub.nastavVolby([b335](KodekJpeg &k, int v) { nastavKodekPs1(k, v); if (!b335) k.plneBarvyDo = 0; }, "test");
    id = hub.otevri(volby);
  }
  void snimek(const std::vector<uint32_t> &F, int W, int H) {
    hub.snimek(F.data(), W, H);
    const auto t0 = std::chrono::steady_clock::now();
    if (hub.dalsi(id, 200, 0, pk) != 1) { pr.chyb++; return; }
    kodMs += std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
    pr.prijmi(pk);
    psSoucet += psnrB(pr.canvas, F, W, 0, 0, W, H);
    n++;
  }
  void konec() {
    hub.zavri(id);
    const std::string l = hub.vezmiLog();
    const size_t i = l.rfind("odpojen");
    if (i != std::string::npos) info = l.substr(i);
  }
  double psnr() const { return n ? psSoucet / n : 0; }
};

// cislo z logu odpojeni ("casti 123)" za danym textem)
static long long zLogu(const std::string &s, const char *za) {
  const size_t i = s.find(za);
  return i == std::string::npos ? -1 : std::atoll(s.c_str() + i + std::strlen(za));
}

static std::vector<std::vector<uint32_t>> nactiPng(const char *adr, int &W, int &H) {
  std::vector<std::vector<uint32_t>> v;
  DIR *d = adr ? opendir(adr) : nullptr;
  if (!d) return v;
  std::vector<std::string> jm;
  while (struct dirent *e = readdir(d)) if (std::strstr(e->d_name, ".png")) jm.push_back(e->d_name);
  closedir(d);
  std::sort(jm.begin(), jm.end());
  for (const std::string &s : jm) {
    int c = 0;
    unsigned char *p = stbi_load((std::string(adr) + "/" + s).c_str(), &W, &H, &c, 3);
    if (!p) continue;
    std::vector<uint32_t> f((size_t)W * H);
    for (size_t i = 0; i < f.size(); i++) f[i] = ((uint32_t)p[i * 3] << 16) | ((uint32_t)p[i * 3 + 1] << 8) | p[i * 3 + 2];
    stbi_image_free(p);
    v.push_back(f);
  }
  return v;
}

// ---- 1, 2, 6) Dune ----
struct VysledekDune { Carky pohyb, klid; Carky nejvic; long long bajtu = 0; double psnr = 0, kodMs = 0; long long casti444 = 0, chyb = 0; std::vector<uint32_t> canvas599, frame599; };
static VysledekDune dune(bool b335, int q) {
  Proud p(b335, q);
  ScenaDune s;
  s.vytvor(5);
  std::vector<uint32_t> F;
  VysledekDune v;
  for (int i = 0; i < 900; i++) {
    s.stoji = i >= 600;
    s.krok();
    s.kresli(F);
    p.snimek(F, ScenaDune::W, ScenaDune::H);
    if (i % 50 == 49 && i < 600) {
      const Carky c = carky(p.pr.canvas, F, ScenaDune::W, ScenaDune::H, ScenaDune::VW);
      if (c.useku > v.nejvic.useku) v.nejvic = c;
    }
    if (i == 599) { v.pohyb = carky(p.pr.canvas, F, ScenaDune::W, ScenaDune::H, ScenaDune::VW); v.canvas599 = p.pr.canvas; v.frame599 = F; }
  }
  v.klid = carky(p.pr.canvas, F, ScenaDune::W, ScenaDune::H, ScenaDune::VW);
  v.bajtu = p.pr.bajtu;
  v.psnr = p.psnr();
  v.kodMs = p.kodMs;
  v.chyb = p.pr.chyb;
  p.konec();
  v.casti444 = zLogu(p.info, "barvy 4:4:4 v castech do 55000 bodu (casti ");
  return v;
}

int main(int argc, char **argv) {
  g_out = std::fopen("VYSTUP_test_b335_kodek.txt", "w");
  tisk("B335 - PS1 na TV: vodorovne carky v Dune 2000 (barvy 4:2:0 dopocitane pres hranici ctvercu)");

  // ===== 1) 2) 6) Dune =====
  for (int q : {55, 76}) {
    const VysledekDune a = dune(false, q), b = dune(true, q);
    tisk("   Dune 2x, kvalita " + std::to_string(q) + ": B334 carek " + std::to_string(a.pohyb.useku) + " (z toho na radku 16k-1 " + std::to_string(a.pohyb.radek15) +
         ", nejsilnejsi " + f1(a.pohyb.nej) + ", nejvic behem pohybu " + std::to_string(a.nejvic.useku) + "), B335 " + std::to_string(b.pohyb.useku) +
         " (nejsilnejsi " + f1(b.pohyb.nej) + ", nejvic behem pohybu " + std::to_string(b.nejvic.useku) + ")");
    over(a.pohyb.useku >= 50 && a.pohyb.radek15 * 10 >= a.pohyb.useku * 8 && b.nejvic.useku == 0 && a.chyb == 0 && b.chyb == 0,
         "1) Dune kvalita " + std::to_string(q) + ": B334 carky po ceste jednotek (" + std::to_string(a.pohyb.useku) + " useku, " +
         std::to_string(a.pohyb.radek15 * 100 / std::max(1, a.pohyb.useku)) + " % na radcich 16k-1 jako na fotce), B335 zadna (nejvic " +
         std::to_string(b.nejvic.useku) + " v 12 kontrolach)");
    over(a.klid.useku == 0 && b.klid.useku == 0,
         "1) Dune kvalita " + std::to_string(q) + ": jednotky stoji 5 s -> B334 carky zmizi (zjemneni 94 = barvy v plnem rozliseni - \"mazou se\"), B335 cisto");
    over(b.bajtu * 100 <= a.bajtu * 120 && b.psnr >= a.psnr - 0.05,
         "2) Dune kvalita " + std::to_string(q) + ": dat B335 " + f2(b.bajtu / 1e6) + " MB proti B334 " + f2(a.bajtu / 1e6) + " MB (+" +
         f1(100.0 * (b.bajtu - a.bajtu) / a.bajtu) + " %), PSNR " + f2(b.psnr) + " proti " + f2(a.psnr) + " dB, casti 4:4:4 " + std::to_string(b.casti444));
    tisk("   6) kodovani Dune (900 snimku, PC): B334 " + f1(a.kodMs) + " ms, B335 " + f1(b.kodMs) + " ms");
    // NAP_B335_PRAC=slozka: canvas po snimku 599 (B334, B335) a snimek hry pro oci (0x00RRGGBB 736x480)
    const char *prac = std::getenv("NAP_B335_PRAC");
    if (q == 55 && prac) {
      const std::string d = prac;
      const std::vector<uint32_t> *co[3] = {&a.canvas599, &b.canvas599, &a.frame599};
      const char *jm[3] = {"/dune_b334_canvas599.raw", "/dune_b335_canvas599.raw", "/dune_hra599.raw"};
      for (int k = 0; k < 3; k++)
        if (FILE *f = std::fopen((d + jm[k]).c_str(), "wb")) { std::fwrite(co[k]->data(), 4, co[k]->size(), f); std::fclose(f); }
    }
  }

  // ===== 3) 3D hra =====
  for (int k = 0; k < 2; k++) {
    int W = 0, H = 0;
    const char *adr = argc > 1 + k ? argv[1 + k] : nullptr;
    const auto S = nactiPng(adr, W, H);
    if (S.empty()) { tisk(std::string("   (3D ") + (k ? "640x480" : "320x240") + ": snimky nejsou - preskoceno)"); continue; }
    for (int q : {0, 55}) {
      Proud a(false, q), b(true, q);
      for (const auto &F : S) { a.snimek(F, W, H); b.snimek(F, W, H); }
      a.konec(); b.konec();
      const long long c444 = zLogu(b.info, "barvy 4:4:4 v castech do 55000 bodu (casti ");
      const double proc = 100.0 * (b.pr.bajtu - a.pr.bajtu) / a.pr.bajtu;
      over(b.pr.bajtu * 100 <= a.pr.bajtu * 125 && b.psnr() >= a.psnr() - 0.05 && a.pr.chyb == 0 && b.pr.chyb == 0 &&
               c444 < (long long)S.size(),
           "3) 3D " + std::to_string(W) + "x" + std::to_string(H) + " (" + std::to_string(S.size()) + " snimku, kvalita " + (q ? std::to_string(q) : std::string("max")) +
           "): dat B335 " + f2(b.pr.bajtu / 1e6) + " MB proti " + f2(a.pr.bajtu / 1e6) + " MB (" + (proc >= 0 ? "+" : "") + f1(proc) + " %), PSNR " +
           f2(b.psnr()) + " proti " + f2(a.psnr()) + " dB, casti 4:4:4 " + std::to_string(c444) + ", kodovani " + f1(b.kodMs) + " proti " + f1(a.kodMs) + " ms");
    }
  }

  // ===== 4) velke casti 4:2:0 (cely obraz se meni) a pak male zmeny =====
  {
    const int W = 640, H = 480;
    std::vector<uint32_t> F((size_t)W * H), pozadi((size_t)W * H);
    uint32_t rs = 7;
    auto rnd = [&]() { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; };
    auto pruh = [&](std::vector<uint32_t> &G, int x0, int y0) {
      for (int y = y0; y < y0 + 4; y++)
        for (int x = x0; x < x0 + 48; x++) if (x >= 0 && x < W) G[(size_t)y * W + x] = ScenaDune::ZELENA;
    };
    Proud a(false, 70), b(true, 70);
    Carky ca, cb;
    for (int i = 0; i < 90; i++) {
      if (i < 20) {   // "3D": cely obraz jiny (pisek s posunutym zrnem a vlnami), zeleny pruh stoji pod hranici radku 160
        for (int y = 0; y < H; y++)
          for (int x = 0; x < W; x++) {
            const int z = (int)(rnd() % 9) - 4, v = (int)(20 * std::sin((x + i * 7) * 0.05) + 20 * std::cos((y - i * 5) * 0.04));
            pozadi[(size_t)y * W + x] = ScenaDune::c15(200 + z + v, 170 + z + v, 110 + z);
          }
        F = pozadi;
        pruh(F, 200, 160);
      } else {        // obraz stoji, pruh jede doprava (male casti)
        F = pozadi;
        pruh(F, 200 + (i - 19) * 6, 160);
      }
      a.snimek(F, W, H);
      b.snimek(F, W, H);
      if (i == 60) { ca = carky(a.pr.canvas, F, W, H, W); cb = carky(b.pr.canvas, F, W, H, W); }
    }
    a.konec(); b.konec();
    const long long dopl = zLogu(b.info, "ctvercu doplnenych kvuli sousedovi z JPEG 4:2:0 ");
    over(ca.useku >= 4 && ca.radek15 == ca.useku && cb.useku == 0 && dopl > 0,
         "4) cely obraz se menil (casti 640x480 = 4:2:0) a pak jen pruh: B334 zelena 1/4 nad puvodnim pruhem zustala (" + std::to_string(ca.useku) +
         " useku na radku 159), B335 cisto - doplneno " + std::to_string(dopl) + " ctvercu (soused ze spolecneho JPEG 4:2:0)");
  }

  // ===== 5) mapa s posunem (kopie) ve 2x a 1x =====
  for (const bool dvakrat : {true, false}) {
    Proud a(false, 0), b(true, 0);
    HraMapa hra;
    hra.vytvor(7);
    std::vector<uint32_t> F;
    int W = 0, H = 0;
    double nejA = 0, nejB = 0, psMinA = 99, psMinB = 99;
    const int faze[][3] = {{8, 0, 0}, {30, 8, 0}, {20, 0, 8}, {20, -8, -8}, {20, 0, 0}, {20, 4, 2}, {10, 0, 0}};
    int i = 0;
    for (const auto &f : faze)
      for (int k = 0; k < f[0]; k++, i++) {
        hra.kurX = 150 + (int)(60 * std::sin(i * 0.11));
        hra.kurY = 120 + (int)(40 * std::cos(i * 0.07));
        hra.krok(f[1], f[2]);
        hra.kresli(F, W, H, dvakrat);
        a.snimek(F, W, H);
        b.snimek(F, W, H);
        nejA = std::max(nejA, nejhorsiCtverec(a.pr.canvas, F, W, H));
        nejB = std::max(nejB, nejhorsiCtverec(b.pr.canvas, F, W, H));
        psMinA = std::min(psMinA, psnrB(a.pr.canvas, F, W, 0, 0, W, H));
        psMinB = std::min(psMinB, psnrB(b.pr.canvas, F, W, 0, 0, W, H));
      }
    a.konec(); b.konec();
    const long long dopl = zLogu(b.info, "ctvercu doplnenych kvuli sousedovi z JPEG 4:2:0 ");
    over(a.pr.kopii > 50 && b.pr.kopii > 50 && nejB <= std::max(15.0, nejA + 1.0) && psMinB >= psMinA - 0.3 && b.pr.chyb == 0,
         std::string("5) mapa s posunem (B319) ") + (dvakrat ? "2x 736x480" : "1x 368x240") + ": kopii B335 " + std::to_string(b.pr.kopii) + " (B334 " + std::to_string(a.pr.kopii) + "), polovicnich " +
         std::to_string(b.pr.polovicnich) + ", nejhorsi ctverec " + f1(nejB) + " (B334 " + f1(nejA) + "), PSNR nejmene " + f2(psMinB) + " (B334 " + f2(psMinA) +
         ") dB, dat " + f2(b.pr.bajtu / 1e6) + " MB (B334 " + f2(a.pr.bajtu / 1e6) + "), doplneno sousedu " + std::to_string(dopl));
  }

  tisk("VYSLEDEK: " + std::to_string(g_ok + g_chyb) + " kontrol, " + std::to_string(g_chyb) + " CHYBA");
  if (g_out) std::fclose(g_out);
  return g_chyb ? 1 : 0;
}
