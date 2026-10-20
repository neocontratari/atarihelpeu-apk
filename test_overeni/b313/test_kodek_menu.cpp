// test_overeni/b313/test_kodek_menu.cpp
// ====================================================================
//  B313: KODEK "JPEG PO CASTECH" PRO HLAVNI MENU NA TV (bez telefonu, bez GPU)
//
//  Rene: "Grafika hlavniho menu je rozmazana na TV ci PC prenosu LAN."
//  Log B312: menu 854x480, kvalita 76 (az 60), "zjemneni 0" - v menu se porad neco
//  hybe (mlhovina, hvezdy, konzole na karte), takze se obraz nikdy nezjemnil.
//
//  Tady skutecny Hub + KodekJpeg (app/src/main/cpp/tvproud) s volbami menu
//  (nastavKodekMenu: vic obdelniku, zjemneni za pohybu, 4 pasy, strop 85, pri listovani 75)
//  a "prohlizec" v C++ (prohlizec_v3.h): pakety rozebere PRESNE jako stranka
//  (apPaket23 / apObraz3: hlavicka 28 B, cast, zvuk),
//  JPEG rozbali stb_image a nakresli na misto - jako canvas v prohlizeci.
//
//  Snimky 1280x720 jako v menu: pozadi s jemnym "textem" (ostre hrany - na nich je
//  rozmazani videt nejvic), uprostred se porad hybe "konzole", vedle obcas "vedlejsi
//  karta"; na zacatku 30 snimku pohyb vseho (listovani).
//  Kontroly:
//   1) vsechny pakety jdou rozebrat, dalsi casti maji priznak 8 a zadny zvuk
//   2) pri listovani (vse se hybe) jde snimek jako jeden JPEG v nizsi kvalite (75),
//      v klidu konzole v kvalite 85
//   3) v klidu (hybe se jen konzole a obcas vedlejsi karta) jdou zmeny jako vic obdelniku
//   4) stojici casti se zjemni, i kdyz se konzole porad hybe (drive "zjemneni 0"):
//      obraz v prohlizeci se s puvodnim shoduje (PSNR) jako po kvalite 94
//   5) v klidu prenos jen zlomek listovani (kB na snimek)
//   6) po listovani se ostrost vrati do ~1 s (30 snimku)
//   7) PS1 / demo (bez voleb menu): chovani jako drive - jeden obdelnik, za pohybu
//      se nezjemnuje
//   8) cast obrazu ve vic paketech: obraz v prohlizeci = posledni snimek (zadna
//      cast nechybi, poradi spravne) - i pri zmene velikosti 854x480 -> 1280x720
//
//   9) pomala sit (zapis 20 ms): za pohybu se nezjemnuje
//  10) snimek jako z glReadPixels (RGBA zdola nahoru, snimekRgbaZdola - menu preda buffer
//      bez kopie, prevod udela vlakno prohlizece): dva prohlizece, obraz spravne otoceny
//  11) totez ve vlaknech jako v appce (kresleni 60x/s, dva prohlizece soubezne): zadny snimek
//      neni roztrzeny (i pod ThreadSanitizerem: -O1 -g -fsanitize=thread)
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb
//       -o test_kodek_menu test_kodek_menu.cpp && ./test_kodek_menu
// ====================================================================
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "stb_image.h"

using namespace nap::proud;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

#include "prohlizec_v3.h"

// ---- snimky jako v menu ----
// pozadi: tmavy prechod + zlate "pismo" (tenke cary, ostre hrany) + hvezdy; posun = listovani
static void kresliSnimek(std::vector<uint32_t> &F, int W, int H, int posun, int konzole, int vedlejsi) {
  F.resize((size_t)W * H);
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) {
      const int xx = x + posun;
      int r = 10 + (y * 30) / H, g = 6 + (y * 18) / H, b = 4 + (xx & 255) / 40;
      // "text": radky pismen 9 px vysoke, mezery
      const int ry = y % 40, rx = xx % 14;
      if (ry >= 8 && ry < 17 && rx < 9 && (((xx / 14) * 7 + (y / 40) * 3) % 5 != 0)) {
        const bool tah = (rx == 1 || rx == 7 || ry == 8 || ry == 12 || ry == 16);
        if (tah) { r = 250; g = 205; b = 120; }
      }
      // hvezdy
      if (((xx * 7919 + y * 104729) % 9973) == 0) { r = g = b = 255; }
      F[(size_t)y * W + x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
  // "konzole" na prostredni karte: barevny ctverec 180x150, ktery se otaci (posun vzoru)
  const int cx = W / 2 - 90, cy = H / 2 - 110;
  for (int y = 0; y < 150; y++)
    for (int x = 0; x < 180; x++) {
      const int u = (x + konzole * 3) % 180, v = (y + konzole) % 150;
      const int r = 120 + (u * 120) / 180, g = 90 + ((u ^ v) & 63), b = 60 + (v * 100) / 150;
      F[(size_t)(cy + y) * W + cx + x] = ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
  // vedlejsi karta vpravo: maly obrazek 90x80, meni se obcas
  const int vx = W * 3 / 4, vy = H / 2 - 80;
  for (int y = 0; y < 80; y++)
    for (int x = 0; x < 90; x++) {
      const int k = (x + y + vedlejsi * 5) & 31;
      F[(size_t)(vy + y) * W + vx + x] = ((uint32_t)(80 + k * 3) << 16) | ((uint32_t)(70 + k * 2) << 8) | (uint32_t)(90 + k);
    }
}

struct Vysledek {
  double kBlistovani = 0, kBklid = 0;
  double psnrStatikKonec = 0, psnrCelyKonec = 0, psnrStatik1s = 0, psnrPredTim = 0;
  int maxCastiVKlidu = 0, castiPriListovani = 0, kvalitaPriListovani = -1, kvalitaKonzoleVKlidu = -1;
  long long zjemneniZaPohybu = 0, vicecastnych = 0;
  Prohlizec pr;
  std::string info;
};

static Vysledek beh(bool menu, int W, int H, int listovat, int klid, int zapisMs = 0) {
  Hub<KodekJpeg> hub("TEST", 44100, 16, 9, false);
  if (menu)
    hub.nastavKodek(nastavKodekMenu);
  const int id = hub.otevri();
  Vysledek v;
  std::vector<uint32_t> F;
  std::vector<uint8_t> pk;
  long long bajtuList = 0, bajtuKlid = 0;
  int snList = 0, snKlid = 0;
  const int celkem = listovat + klid;
  for (int i = 0; i < celkem; i++) {
    const bool list = i < listovat;
    kresliSnimek(F, W, H, list ? i * 9 : listovat * 9, i, i / 6);
    hub.snimek(F.data(), W, H);
    const int r = hub.dalsi(id, 200, zapisMs, pk);
    if (r != 1) { v.pr.chyb++; continue; }
    v.pr.prijmi(pk);
    if (list) {
      bajtuList += (long long)pk.size(); snList++;
      if (i == listovat - 1) {
        v.castiPriListovani = v.pr.castiPoslednihoSnimku;
        v.kvalitaPriListovani = v.pr.kvalityPoslednihoSnimku.empty() ? -1 : v.pr.kvalityPoslednihoSnimku[0];
      }
    }
    else {
      bajtuKlid += (long long)pk.size(); snKlid++;
      if (i >= listovat + 40) v.maxCastiVKlidu = std::max(v.maxCastiVKlidu, v.pr.castiPoslednihoSnimku);
      if (i == celkem - 1 && !v.pr.kvalityPoslednihoSnimku.empty()) v.kvalitaKonzoleVKlidu = v.pr.kvalityPoslednihoSnimku.back();
    }
    // ostrost stojici casti (horni pas nad kartami - "napis" a hvezdy)
    if (i == listovat + 30) v.psnrStatik1s = psnr(F, v.pr.canvas, W, 0, 0, W, H / 2 - 120);
    if (i == listovat - 1) v.psnrPredTim = psnr(F, v.pr.canvas, W, 0, 0, W, H / 2 - 120);
  }
  v.kBlistovani = snList ? bajtuList / 1024.0 / snList : 0;
  v.kBklid = snKlid ? bajtuKlid / 1024.0 / snKlid : 0;
  v.psnrStatikKonec = psnr(F, v.pr.canvas, W, 0, 0, W, H / 2 - 120);
  v.psnrCelyKonec = psnr(F, v.pr.canvas, W, 0, 0, W, H);
  hub.zavri(id);
  v.info = hub.vezmiLog();
  return v;
}

int main() {
  std::printf("B313 - kodek JPEG po castech pro hlavni menu na TV (Hub + KodekJpeg z appky, prohlizec v C++)\n\n");

  // ---- menu 1280x720: 30 snimku listovani, pak 150 snimku klid (konzole se toci porad) ----
  Vysledek m = beh(true, 1280, 720, 30, 150);
  std::printf("   menu 1280x720: listovani %.1f kB/snimek, klid %.1f kB/snimek; ostrost horniho pasu: pri listovani %.1f dB, 1 s po nem %.1f dB, na konci %.1f dB; cely obraz %.1f dB\n",
              m.kBlistovani, m.kBklid, m.psnrPredTim, m.psnrStatik1s, m.psnrStatikKonec, m.psnrCelyKonec);
  {
    std::string l = m.info;
    for (size_t p; (p = l.find('\n')) != std::string::npos; l.erase(0, p + 1)) std::printf("   log: %s\n", l.substr(0, p).c_str());
  }
  over(m.pr.chyb == 0 && m.pr.spatnyPriznak == 0 && m.pr.zvukVDalsi == 0,
       "1) pakety jdou rozebrat jako ve strance (" + std::to_string(m.pr.paketu) + " paketu, " + std::to_string(m.pr.snimku) + " snimku, dalsich casti " +
           std::to_string(m.pr.dalsich) + "), dalsi casti maji priznak 8 a zadny zvuk; chyb " + std::to_string(m.pr.chyb));
  over(m.castiPriListovani == 1 && m.kvalitaPriListovani == 75 && m.kvalitaKonzoleVKlidu == 85,
       "2) pri listovani (meni se vse) jde snimek jako jeden JPEG v kvalite 75 (casti " + std::to_string(m.castiPriListovani) + ", kvalita " +
           std::to_string(m.kvalitaPriListovani) + "), v klidu konzole v kvalite " + std::to_string(m.kvalitaKonzoleVKlidu) + " (strop menu 85)");
  over(m.maxCastiVKlidu >= 2, "3) v klidu (konzole + obcas vedlejsi karta) jdou zmeny jako vic obdelniku: nejvic " + std::to_string(m.maxCastiVKlidu) + " v jednom snimku");
  over(m.psnrStatikKonec >= 40.0 && m.info.find("zjemneni za pohybu 0") == std::string::npos,
       "4) stojici casti se zjemni, i kdyz se konzole porad hybe: horni pas " + std::to_string(m.psnrStatikKonec).substr(0, 5) + " dB (>= 40 = kvalita 94)");
  over(m.kBklid < m.kBlistovani * 0.35, "5) v klidu jen zlomek prenosu: " + std::to_string(m.kBklid).substr(0, 5) + " kB proti " + std::to_string(m.kBlistovani).substr(0, 5) + " kB pri listovani");
  over(m.psnrStatik1s >= 40.0, "6) ostrost se po listovani vrati do 1 s (30 snimku): " + std::to_string(m.psnrStatik1s).substr(0, 5) + " dB (pri listovani " +
                                   std::to_string(m.psnrPredTim).substr(0, 5) + " dB)");

  // ---- stejne snimky bez voleb menu (PS1 / demo): jako drive ----
  Vysledek p = beh(false, 1280, 720, 30, 150);
  std::printf("   bez voleb menu (PS1 / demo): klid %.1f kB/snimek, horni pas na konci %.1f dB, nejvic casti %d\n", p.kBklid, p.psnrStatikKonec, p.maxCastiVKlidu);
  over(p.pr.chyb == 0 && p.pr.dalsich == 0 && p.maxCastiVKlidu <= 1 && p.info.find("B313") == std::string::npos,
       "7) PS1 / demo beze zmeny: jeden obdelnik na snimek, zadne dalsi casti, za pohybu se nezjemnuje (log bez B313)");

  // ---- 8) obraz v prohlizeci = posledni snimek; zmena velikosti 854x480 -> 1280x720 ----
  {
    Hub<KodekJpeg> hub("TEST8", 44100, 16, 9, false);
    hub.nastavKodek(nastavKodekMenu);
    const int id = hub.otevri();
    Prohlizec pr;
    std::vector<uint32_t> F;
    std::vector<uint8_t> pk;
    double nejhorsi = 99;
    int W = 854, H = 480;
    for (int i = 0; i < 160; i++) {
      if (i == 80) { W = 1280; H = 720; }
      kresliSnimek(F, W, H, i < 10 || (i >= 80 && i < 90) ? i * 9 : 9 * 9, i, i / 6);
      hub.snimek(F.data(), W, H);
      if (hub.dalsi(id, 200, 0, pk) != 1) { pr.chyb++; continue; }
      pr.prijmi(pk);
      if (i == 79 || i == 159) nejhorsi = std::min(nejhorsi, psnr(F, pr.canvas, W, 0, 0, W, H));
    }
    hub.zavri(id);
    over(pr.chyb == 0 && pr.W == 1280 && pr.H == 720 && nejhorsi >= 33.0,
         "8) obraz v prohlizeci = posledni snimek (zadna cast nechybi): nejhorsi PSNR celeho obrazu " + std::to_string(nejhorsi).substr(0, 5) +
             " dB, i po zmene 854x480 -> 1280x720 (" + std::to_string(pr.W) + "x" + std::to_string(pr.H) + ")");
  }

  // ---- 9) pomala sit: zjemneni za pohybu ceka, az sit stiha (zapis < 8 ms) ----
  {
    Vysledek s = beh(true, 1280, 720, 30, 60, 20);
    over(s.pr.chyb == 0 && s.info.find("zjemneni za pohybu 0") != std::string::npos,
         "9) sit nestiha (zapis 20 ms): za pohybu se nezjemnuje (velke pakety by sit ucpaly)");
  }

  // ---- 10) snimek jako z glReadPixels (RGBA, radky zdola) - menu tak preda buffer bez kopie ----
  {
    Hub<KodekJpeg> hub("TEST10", 44100, 16, 9, false);
    hub.nastavKodek(nastavKodekMenu);
    const int id1 = hub.otevri(), id2 = hub.otevri();          // dva prohlizece naraz
    Prohlizec p1, p2;
    std::vector<uint32_t> F;
    std::vector<uint8_t> rgba, pk;
    double nejhorsi = 99, priPohybu = 99;
    int vymen = 0;
    const int W = 1280, H = 720;
    for (int i = 0; i < 60; i++) {
      kresliSnimek(F, W, H, i < 10 ? i * 9 : 90, i, i / 6);
      rgba.resize((size_t)W * H * 4);
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
          const uint32_t c = F[(size_t)y * W + x];
          uint8_t *o = &rgba[((size_t)(H - 1 - y) * W + x) * 4];
          o[0] = (uint8_t)(c >> 16); o[1] = (uint8_t)(c >> 8); o[2] = (uint8_t)c; o[3] = 255;
        }
      const uint8_t *pred = rgba.data();
      hub.snimekRgbaZdola(rgba, W, H);
      if (rgba.data() != pred) vymen++;                         // buffer se vymenil (zadna kopie)
      if (hub.dalsi(id1, 200, 0, pk) != 1) { p1.chyb++; continue; }
      p1.prijmi(pk);
      if (hub.dalsi(id2, 200, 0, pk) != 1) { p2.chyb++; continue; }
      p2.prijmi(pk);
      const double a = std::min(psnr(F, p1.canvas, W, 0, 0, W, H), psnr(F, p2.canvas, W, 0, 0, W, H));
      if (i < 10) priPohybu = std::min(priPohybu, a);            // kvalita 75 - vzhuru nohama by bylo pod 15 dB
      if (i >= 45) nejhorsi = std::min(nejhorsi, a);             // po zjemneni
    }
    hub.zavri(id1);
    hub.zavri(id2);
    over(p1.chyb == 0 && p2.chyb == 0 && priPohybu >= 25.0 && nejhorsi >= 38.0 && vymen == 60,
         "10) snimek z glReadPixels (RGBA zdola nahoru) bez kopie: buffer vymenen " + std::to_string(vymen) + "x z 60, oba prohlizece maji spravny obraz (pri pohybu " +
             std::to_string(priPohybu).substr(0, 5) + " dB, po zjemneni " + std::to_string(nejhorsi).substr(0, 5) + " dB - ne vzhuru nohama)");
  }

  // ---- 11) vlakna jako v appce: kresleni preda snimek (snimekRgbaZdola) 60x za sekundu, dva
  //          prohlizece si ho soubezne berou a prevadeji - zadny snimek nesmi byt "roztrzeny"
  //          (prevod z bufferu, do ktereho uz kresleni zapisuje dalsi snimek) ----
  {
    Hub<KodekJpeg> hub("TEST11", 44100, 16, 9, false);
    hub.nastavKodek(nastavKodekMenu);
    const int W = 640, H = 360;
    auto snimekK = [&](unsigned k, std::vector<uint32_t> &F) { kresliSnimek(F, W, H, (int)(k * 7) % 400, (int)k, (int)k / 3); };
    std::atomic<bool> konec{false};
    std::atomic<int> chybVlakna{0}, snimkuCelkem{0};
    std::atomic<double> nejhorsi{99.0};
    const int id1 = hub.otevri(), id2 = hub.otevri();
    auto prohlizec = [&](int id) {
      Prohlizec pr;
      std::vector<uint8_t> pk;
      std::vector<uint32_t> F;
      while (!konec.load()) {
        const int r = hub.dalsi(id, 50, 0, pk);
        if (r < 0) { chybVlakna++; return; }
        if (r == 0) continue;
        pr.prijmi(pk);
        const unsigned seq = Prohlizec::u32(&pk[4 + 8]);
        snimekK(seq, F);
        const double p = psnr(F, pr.canvas, W, 0, 0, W, H);
        double n = nejhorsi.load();
        while (p < n && !nejhorsi.compare_exchange_weak(n, p)) {}
        snimkuCelkem++;
      }
      if (pr.chyb) chybVlakna += (int)pr.chyb;
    };
    std::thread a(prohlizec, id1), b(prohlizec, id2);
    std::vector<uint8_t> rgba;
    std::vector<uint32_t> F;
    for (unsigned k = 1; k <= 150; k++) {
      snimekK(k, F);
      rgba.resize((size_t)W * H * 4);
      for (int y = 0; y < H; y++)
        for (int x = 0; x < W; x++) {
          const uint32_t c = F[(size_t)y * W + x];
          uint8_t *o = &rgba[((size_t)(H - 1 - y) * W + x) * 4];
          o[0] = (uint8_t)(c >> 16); o[1] = (uint8_t)(c >> 8); o[2] = (uint8_t)c; o[3] = 255;
        }
      hub.snimekRgbaZdola(rgba, W, H);
      std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    konec.store(true);
    a.join();
    b.join();
    hub.zavri(id1);
    hub.zavri(id2);
    over(chybVlakna.load() == 0 && snimkuCelkem.load() > 60 && nejhorsi.load() >= 22.0,
         "11) vlakna: kresleni 60x/s, dva prohlizece soubezne - " + std::to_string(snimkuCelkem.load()) + " snimku, zadny roztrzeny (nejhorsi " +
             std::to_string(nejhorsi.load()).substr(0, 5) + " dB proti snimku se stejnym cislem), chyb " + std::to_string(chybVlakna.load()));
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
