// B305: test kodeku prenosu Sega / PS1 na TV / PC (SKUTECNE hlavicky z appky:
// app/src/main/cpp/tvproud/nap_tv_proud_index.h a nap_tv_proud_jpeg.h).
//
//  SEGA (paleta + LZ, bez ztraty): synteticka hra jako Sonic - dve vrstvy
//  dlazdic 8x8 (popredi a pozadi s paralaxou po pasech radku, vlnici se voda),
//  postavicky, skore nahore, koloběh barev palety, stmivani a rozsvecovani,
//  prepnuti 320 -> 256 bodu, staticky titulek, nahodny sum (nejhorsi pripad).
//  Kazdy snimek se rozbali stejne jako v prohlizeci a porovna bod po bodu.
//
//  PS1 (JPEG po castech): snimky 3D sceny (uvodni demo B304 vykreslene pres
//  Mesa v 320x240 a 640x480, 30 snimku/s), rozbaleni JPEG (stb_image) a
//  skladani obdelniku jako v prohlizeci, PSNR proti originalu; staticky obraz
//  -> nic a pak zjemneni; mala zmena -> jen maly obdelnik; rizeni kvality sitm.
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb -I. -o test_kodeky test_kodeky.cpp
//   ./test_kodeky ADRESAR_320 ADRESAR_640      (PNG snimky z ../b304/demo_snimky)
//  (stb_image_write.h je tu jen pro srovnani rychlosti s puvodnim koderem)
#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <string>
#include <vector>

#include "nap_tv_proud_index.h"
#include "nap_tv_proud_jpeg.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#define STB_IMAGE_WRITE_STATIC
#define STBI_WRITE_NO_STDIO
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

using namespace nap::proud;

static int g_kontrol = 0, g_chyb = 0;
static void over(bool ok, const char *fmt, ...) {
  g_kontrol++;
  if (!ok) g_chyb++;
  char b[600];
  va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
  printf("%s %s\n", ok ? "OK    " : "CHYBA ", b);
  fflush(stdout);
}
static double ted() { return tedMsPresne(); }

#include "sega_hra.h"

struct Vysledek {
  long long snimku = 0, bajtu = 0, maxB = 0, chybnych = 0;
  double ms = 0, maxMs = 0;
};

// zakoduje snimek, hned ho rozbali jako prohlizec a porovna
static void segaSnimek(KodekIndex &k, DekoderIndex &d, const std::vector<uint32_t> &F, int W, int H, Vysledek &v, bool &spravne, unsigned *pr = nullptr) {
  std::vector<uint8_t> out;
  const double t0 = ted();
  const unsigned p = k.zakoduj(F.data(), W, H, 0, 0, out);
  const double dt = ted() - t0;
  if (pr) *pr = p;
  v.snimku++; v.bajtu += (long long)out.size(); v.maxB = std::max(v.maxB, (long long)out.size());
  v.ms += dt; v.maxMs = std::max(v.maxMs, dt);
  const size_t n = d.obraz(out.data(), out.size(), W, H, (p & 1) != 0);
  bool ok = n == out.size();
  for (int y = 0; ok && y < H; y++)
    for (int x = 0; x < W; x++)
      if (d.barva(x, y) != (F[(size_t)y * W + x] & 0xFFFFFFu)) { ok = false; break; }
  if (!ok) { v.chybnych++; spravne = false; }
}

static void tiskSega(const char *co, const Vysledek &v, double fps) {
  printf("  %-44s %4lld snimku: prumer %6.0f B, max %6lld B, %5.2f Mbit/s pri %.0f/s, kodovani prumer %.2f ms, max %.2f ms\n",
         co, v.snimku, v.bajtu / (double)std::max(1LL, v.snimku), v.maxB, v.bajtu * 8.0 / std::max(1LL, v.snimku) * fps / 1e6, fps,
         v.ms / std::max(1LL, v.snimku), v.maxMs);
}

static void testSega() {
  printf("\n=== SEGA: paleta + LZ (bez ztraty) ===\n");
  SegaHra hra;
  KodekIndex k;
  DekoderIndex d;
  std::vector<uint32_t> F;
  bool spravne = true;
  // 1) hra bezi: 10 s pri 60 snimcich/s
  Vysledek hraV, klicV;
  hra.kresli(F);
  segaSnimek(k, d, F, hra.W, hra.H, klicV, spravne);
  for (int i = 0; i < 600; i++) { hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, hraV, spravne); }
  tiskSega("prvni (klicovy) snimek", klicV, 60);
  tiskSega("hra: scrolling, paralaxa, postavicky (10 s)", hraV, 60);
  printf("  operace LZ: LIT %lld, RUN %lld, COPY %lld, COPY stejna vzdalenost %lld, SKIP %lld\n", k.opLit, k.opRun, k.opCopy, k.opRep, k.opSkip);
  over(spravne && hraV.chybnych == 0, "Sega hra 601 snimku: prohlizec ma po kazdem snimku PRESNE obraz jadra (vsech 71680 bodu)");
  over(hraV.bajtu * 8.0 / hraV.snimku * 60 / 1e6 < 6.0, "scrolling hry pri 60 snimcich/s pod 6 Mbit/s (%.2f Mbit/s)", hraV.bajtu * 8.0 / hraV.snimku * 60 / 1e6);
  over(k.klicovych == 1, "jen jeden klicovy snimek (na zacatku), dal jen rozdily (klicovych %lld)", k.klicovych);
  // 2) stmivani a rozsviceni (vsechny barvy nove kazdy krok)
  Vysledek stmV;
  const long long pr0 = k.prestaveb, kl0 = k.klicovych;
  for (int i = 0; i < 16; i++) { hra.ztmaveni = std::min(7, i / 2); hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, stmV, spravne); }
  for (int i = 0; i < 16; i++) { hra.ztmaveni = std::max(0, 7 - i / 2); hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, stmV, spravne); }
  tiskSega("stmivani + rozsviceni (32 snimku)", stmV, 60);
  printf("  prestaveb palety %lld (misto nich klicovy snimek: %lld)\n", k.prestaveb - pr0, k.klicovych - kl0);
  over(spravne && stmV.chybnych == 0 && k.klicovych == kl0, "stmivani: barvy se meni kazdy snimek, obraz presny, zadny klicovy snimek (paleta se prestavi)");
  // 3) staticky titulek: 120 stejnych snimku
  Vysledek statV;
  unsigned prz = 0; int beze = 0;
  for (int i = 0; i < 120; i++) { segaSnimek(k, d, F, hra.W, hra.H, statV, spravne, &prz); if (prz & 4) beze++; }
  tiskSega("staticky obraz (120 snimku)", statV, 60);
  over(beze == 120 && statV.maxB <= 8, "staticky obraz: %d/120 snimku 'beze zmeny', paket obrazu %lld B", beze, statV.maxB);
  // 4) prepnuti H40 -> H32 (256 bodu) a zpet
  Vysledek h32V;
  hra.W = 256; hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, h32V, spravne, &prz);
  const bool klic1 = (prz & 1) != 0;
  for (int i = 0; i < 60; i++) { hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, h32V, spravne); }
  hra.W = 320; hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, h32V, spravne, &prz);
  over(spravne && klic1 && (prz & 1) && h32V.chybnych == 0, "prepnuti 320 -> 256 -> 320 bodu: klicovy snimek, obraz presny");
  // 5) paleta se prepisuje (kazdy snimek 64 uplne novych barev - plna paleta,
  //    nove barvy dostanou mista barev, ktere uz ve snimku nejsou)
  Vysledek palV;
  const long long pr1 = k.prestaveb, kl1 = k.klicovych;
  Rng rp(7);
  for (int i = 0; i < 40; i++) { for (int j = 0; j < 64; j++) hra.cram[j] = rp() & 0x1FF; hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, palV, spravne); }
  tiskSega("paleta kazdy snimek nova (40 snimku)", palV, 60);
  printf("  prestaveb palety %lld, klicovych %lld\n", k.prestaveb - pr1, k.klicovych - kl1);
  over(spravne && palV.chybnych == 0 && k.prestaveb - pr1 >= 30 && k.klicovych == kl1,
       "plna paleta: nove barvy na mista nepouzitych (%lld prestaveb), obraz presny, zadny klicovy snimek", k.prestaveb - pr1);
  // 6) nejhorsi pripad: nahodny sum ze 64 barev (nic se neopakuje)
  Vysledek sumV;
  Rng r(99);
  for (int i = 0; i < 30; i++) {
    for (auto &c : F) c = mdBarva(r() & 0x1FF) & 0xFFFFFFu;
    for (size_t j = 0; j < F.size(); j++) F[j] = mdBarva(hra.cram[r() % 64]);
    segaSnimek(k, d, F, hra.W, hra.H, sumV, spravne);
  }
  tiskSega("nejhorsi pripad: nahodny sum (30 snimku)", sumV, 60);
  over(spravne && sumV.chybnych == 0, "nahodny sum: obraz presny");
  over(sumV.maxMs < 15.0, "i nejhorsi snimek se zabali do 15 ms na PC (%.1f ms)", sumV.maxMs);
  // 7) vic nez 256 barev v jednom snimku (u Segy nenastane) - nesmi spadnout, nejblizsi barva
  Vysledek moc;
  for (int y = 0; y < hra.H; y++) for (int x = 0; x < hra.W; x++) F[(size_t)y * hra.W + x] = 0xFF000000u | (uint32_t)((x * 7 + y * 131) & 0xFFFFFF);
  bool spr2 = true;
  segaSnimek(k, d, F, hra.W, hra.H, moc, spr2);
  over(k.nejblizsich > 0, "snimek s %d barvami: nespadne, barvy navic dostanou nejblizsi (nejblizsich %lld)", hra.W * hra.H, k.nejblizsich);
  // a hned zase hra - presne
  bool spr3 = true; Vysledek po;
  for (int i = 0; i < 30; i++) { hra.krok(); hra.kresli(F); segaSnimek(k, d, F, hra.W, hra.H, po, spr3); }
  over(spr3 && po.chybnych == 0, "po snimku s mnoha barvami je hra zase presna");
}

// =====================================================================
//  PS1 - JPEG po castech (snimky 3D sceny)
// =====================================================================
static std::vector<std::string> pngy(const char *adr) {
  std::vector<std::string> v;
  DIR *d = opendir(adr);
  if (!d) return v;
  while (dirent *e = readdir(d)) { std::string n = e->d_name; if (n.size() > 4 && n.substr(n.size() - 4) == ".png") v.push_back(std::string(adr) + "/" + n); }
  closedir(d);
  std::sort(v.begin(), v.end());
  return v;
}
static bool nactiPng(const std::string &f, std::vector<uint32_t> &F, int &W, int &H) {
  int n;
  unsigned char *p = stbi_load(f.c_str(), &W, &H, &n, 3);
  if (!p) return false;
  F.resize((size_t)W * H);
  for (size_t i = 0; i < F.size(); i++) F[i] = 0xFF000000u | ((uint32_t)p[i * 3] << 16) | ((uint32_t)p[i * 3 + 1] << 8) | p[i * 3 + 2];
  stbi_image_free(p);
  return true;
}
// "prohlizec": obraz, do ktereho se skladaji rozbalene JPEG obdelniky
struct PlatnoJpeg {
  int W = 0, H = 0;
  std::vector<uint32_t> px;
  bool pridej(const std::vector<uint8_t> &out, int w, int h, unsigned pr, CastJpeg &c) {
    if (w != W || h != H) { W = w; H = h; px.assign((size_t)W * H, 0); }
    if (c.cti(out.data(), out.size()) != out.size()) return false;
    if ((pr & 1) && !(c.x == 0 && c.y == 0 && (int)c.w == W && (int)c.h == H)) return false;   // klicovy = cely snimek
    if (c.w == 0) return true;
    int jw, jh, n;
    unsigned char *p = stbi_load_from_memory(c.jpeg, (int)c.delka, &jw, &jh, &n, 3);
    if (!p || jw != (int)c.w || jh != (int)c.h || c.x + c.w > (unsigned)W || c.y + c.h > (unsigned)H) { if (p) stbi_image_free(p); return false; }
    for (int y = 0; y < jh; y++)
      for (int x = 0; x < jw; x++) {
        const unsigned char *s = p + ((size_t)y * jw + x) * 3;
        px[(size_t)(c.y + y) * W + c.x + x] = 0xFF000000u | ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2];
      }
    stbi_image_free(p);
    return true;
  }
  double psnr(const std::vector<uint32_t> &F) const {
    double se = 0;
    for (size_t i = 0; i < F.size(); i++)
      for (int s = 0; s < 24; s += 8) { const double e = (double)((F[i] >> s) & 255) - (double)((px[i] >> s) & 255); se += e * e; }
    const double mse = se / (F.size() * 3.0);
    return mse <= 1e-9 ? 99.0 : 10 * std::log10(255.0 * 255.0 / mse);
  }
};

static void stbPiste(void *ctx, void *data, int size) {
  std::vector<uint8_t> *v = (std::vector<uint8_t> *)ctx;
  v->insert(v->end(), (const uint8_t *)data, (const uint8_t *)data + size);
}
static double psnrJpeg(const std::vector<uint8_t> &j, const std::vector<uint32_t> &F, int W, int H) {
  int jw, jh, n;
  unsigned char *p = stbi_load_from_memory(j.data(), (int)j.size(), &jw, &jh, &n, 3);
  if (!p || jw != W || jh != H) { if (p) stbi_image_free(p); return -1; }
  double se = 0;
  for (size_t i = 0; i < F.size(); i++)
    for (int c = 0; c < 3; c++) { const double e = (double)((F[i] >> (16 - 8 * c)) & 255) - p[i * 3 + c]; se += e * e; }
  stbi_image_free(p);
  return 10 * std::log10(255.0 * 255.0 / std::max(1e-9, se / (F.size() * 3.0)));
}
// rychlost: puvodni stb_image_write proti vlastnimu koderu (1, 2, 3 pasy)
static void srovnaniKoderu(const std::vector<uint32_t> &F, int W, int H, int q) {
  std::vector<uint8_t> rgb((size_t)W * H * 3), j;
  for (size_t i = 0; i < F.size(); i++) { rgb[i * 3] = (uint8_t)(F[i] >> 16); rgb[i * 3 + 1] = (uint8_t)(F[i] >> 8); rgb[i * 3 + 2] = (uint8_t)F[i]; }
  const int OPAK = 20;
  double t0 = ted();
  for (int i = 0; i < OPAK; i++) { j.clear(); stbi_write_jpg_to_func(stbPiste, &j, W, H, 3, rgb.data(), q); }
  const double msStb = (ted() - t0) / OPAK;
  const double psStb = psnrJpeg(j, F, W, H);
  const size_t bStb = j.size();
  printf("     %dx%d q%d  stb_image_write: %.2f ms, %zu B, PSNR %.2f dB\n", W, H, q, msStb, bStb, psStb);
  JpegEnc e;
  for (int casti = 1; casti <= 3; casti++) {
    j.clear(); e.koduj(F.data(), (size_t)W, W, H, q, j, casti);   // zahrati (vlakna)
    t0 = ted();
    for (int i = 0; i < OPAK; i++) { j.clear(); e.koduj(F.data(), (size_t)W, W, H, q, j, casti); }
    const double ms = (ted() - t0) / OPAK;
    const double ps = psnrJpeg(j, F, W, H);
    printf("     %dx%d q%d  vlastni koder, %d pas%s: %.2f ms (%.1fx rychleji), %zu B, PSNR %.2f dB\n", W, H, q, casti, casti == 1 ? "" : "y",
           ms, msStb / ms, j.size(), ps);
    over(ps > 0 && std::fabs(ps - psStb) < 0.05 && (double)j.size() < bStb * 1.02 + 64,
         "%dx%d, %d pas%s: platny JPEG (i se znackami RST), stejna kvalita jako stb (PSNR %.2f / %.2f dB)", W, H, casti, casti == 1 ? "" : "y", ps, psStb);
    if (casti == 1) over(ms < msStb * 0.8, "%dx%d: vlastni koder rychlejsi nez stb (%.2f / %.2f ms)", W, H, ms, msStb);
  }
}

static void testPs1(const char *a320, const char *a640) {
  printf("\n=== PS1: JPEG po castech ===\n");
  for (int kolo = 0; kolo < 2; kolo++) {
    const auto soubory = pngy(kolo == 0 ? a320 : a640);
    if (soubory.empty()) { over(false, "chybi PNG snimky v %s (../b304/demo_snimky)", kolo == 0 ? a320 : a640); continue; }
    KodekJpeg k;
    PlatnoJpeg pl;
    std::vector<uint32_t> F;
    int W = 0, H = 0;
    double psnrMin = 99, psnrSum = 0, msSum = 0, msMax = 0;
    long long bajtu = 0, maxB = 0, n = 0;
    bool ok = true;
    CastJpeg c;
    for (const auto &f : soubory) {
      if (!nactiPng(f, F, W, H)) { ok = false; break; }
      std::vector<uint8_t> out;
      const double t0 = ted();
      const unsigned pr = k.zakoduj(F.data(), W, H, 0, 0, out);
      const double dt = ted() - t0;
      msSum += dt; msMax = std::max(msMax, dt);
      bajtu += (long long)out.size(); maxB = std::max(maxB, (long long)out.size()); n++;
      if (!pl.pridej(out, W, H, pr, c)) { ok = false; break; }
      const double ps = pl.psnr(F);
      psnrMin = std::min(psnrMin, ps); psnrSum += ps;
    }
    printf("  %dx%d: %lld snimku 3D sceny (30/s): prumer %.1f kB, max %.1f kB, %.2f Mbit/s pri 30/s (%.2f pri 60/s), kodovani prumer %.2f ms, max %.2f ms\n",
           W, H, n, bajtu / 1024.0 / std::max(1LL, n), maxB / 1024.0, bajtu * 8.0 / std::max(1LL, n) * 30 / 1e6, bajtu * 8.0 / std::max(1LL, n) * 60 / 1e6,
           msSum / std::max(1LL, n), msMax);
    printf("     PSNR prohlizece proti originalu: prumer %.1f dB, nejhorsi %.1f dB, kvalita %d, pasu %d\n", psnrSum / std::max(1LL, n), psnrMin, k.q, k.casti);
    srovnaniKoderu(F, W, H, k.q);
    over(ok, "%dx%d: kazdy paket se rozbali a slozi (obdelniky, klicovy = cely snimek)", W, H);
    over(psnrMin >= 30.0, "%dx%d: obraz v prohlizeci verny originalu (nejhorsi PSNR %.1f dB >= 30)", W, H, psnrMin);
    // stoji: 30x stejny snimek -> nic, pak jednou zjemneni
    long long nic = 0, zjem = 0, bajtuKlid = 0;
    double psPred = pl.psnr(F), psPo = 0;
    for (int i = 0; i < 30; i++) {
      std::vector<uint8_t> out;
      const unsigned pr = k.zakoduj(F.data(), W, H, 0, 0, out);
      if (!pl.pridej(out, W, H, pr, c)) ok = false;
      if (pr & 4) nic++; else { zjem++; bajtuKlid += (long long)out.size(); }
    }
    psPo = pl.psnr(F);
    printf("     stojici obraz 30 snimku: %lld beze zmeny, %lld zjemneni (%.1f kB), PSNR %.1f -> %.1f dB\n", nic, zjem, bajtuKlid / 1024.0, psPred, psPo);
    over(ok && zjem == 1 && nic == 29 && psPo > psPred + 1.0, "%dx%d: stojici obraz = nic se neposila, jednou zjemneni ve vysoke kvalite (PSNR +%.1f dB)", W, H, psPo - psPred);
    // mala zmena (blikajici kurzor 10x10) -> jen maly obdelnik
    std::vector<uint32_t> G = F;
    for (int y = 100; y < 110; y++) for (int x = 150; x < 160; x++) G[(size_t)y * W + x] ^= 0xFFFFFFu;
    std::vector<uint8_t> out;
    const unsigned pr = k.zakoduj(G.data(), W, H, 0, 0, out);
    const bool slozeno = pl.pridej(out, W, H, pr, c);
    printf("     mala zmena 10x10: obdelnik %ux%u na %u,%u, paket %zu B\n", c.w, c.h, c.x, c.y, out.size());
    over(slozeno && c.w == 16 && c.h <= 32 && out.size() < 3000, "%dx%d: mala zmena = maly obdelnik (%ux%u, %zu B)", W, H, c.w, c.h, out.size());
    // site nestiha (zapis 40 ms) -> kvalita dolu, sit volna -> zase nahoru
    const int q0 = k.q;
    long long t = 1000000;
    for (int i = 0; i < 60; i++) { std::vector<uint8_t> o2; G[(size_t)i] ^= 1; t += 33; k.zakoduj(G.data(), W, H, 40, t, o2); }
    const int qDole = k.q;
    for (int i = 0; i < 400; i++) { std::vector<uint8_t> o2; G[(size_t)i] ^= 1; t += 33; k.zakoduj(G.data(), W, H, 0, t, o2); }
    printf("     rizeni kvality siti: %d -> pomala sit %d -> volna sit %d\n", q0, qDole, k.q);
    over(qDole < q0 && k.q == k.qMax, "%dx%d: pomala sit = nizsi kvalita (%d), volna sit = zpet nahoru (%d)", W, H, qDole, k.q);
  }
}

// PS1 ma i "liche" rozmery (368x240, 512x480, 640x512, prekryvy ...) - JPEG musi
// sedet i na kraji, kde ctverce 16x16 presahuji obraz, a s pasy RST
static void testRozmery() {
  printf("\n=== PS1: JPEG pro ruzne rozmery a obdelniky na kraji ===\n");
  const int roz[][2] = {{368, 240}, {512, 480}, {640, 512}, {257, 193}, {256, 224}, {33, 17}};
  for (auto &r : roz) {
    const int W = r[0], H = r[1];
    std::vector<uint32_t> F((size_t)W * H);
    for (int y = 0; y < H; y++)
      for (int x = 0; x < W; x++)
        F[(size_t)y * W + x] = 0xFF000000u | ((uint32_t)(x * 255 / W) << 16) | ((uint32_t)(y * 255 / H) << 8) | (uint32_t)(((x / 8 + y / 8) & 1) ? 200 : 40);
    KodekJpeg k;
    PlatnoJpeg pl;
    CastJpeg c;
    std::vector<uint8_t> out;
    unsigned pr = k.zakoduj(F.data(), W, H, 0, 0, out);
    bool ok = pl.pridej(out, W, H, pr, c) && (pr & 1);
    const double ps1 = pl.psnr(F);
    // zmena v pravem dolnim rohu (obdelnik konci na kraji obrazu, ne na nasobku 16)
    std::vector<uint32_t> G = F;
    for (int y = H - 5; y < H; y++) for (int x = W - 7; x < W; x++) G[(size_t)y * W + x] = 0xFFFF00FFu;
    out.clear();
    pr = k.zakoduj(G.data(), W, H, 0, 0, out);
    ok = ok && pl.pridej(out, W, H, pr, c);
    const bool kraj = c.x + c.w == (unsigned)W && c.y + c.h == (unsigned)H && c.w <= 16 + 15 && c.h <= 16 + 15;
    const double ps2 = pl.psnr(G);
    // srovnani: stejny obraz a kvalita pres stb_image_write (vzor) - ostre sachovnice
    // modre jsou pro JPEG tezke (podvzorkovane barvy), vlastni koder ma byt stejny
    std::vector<uint8_t> rgb((size_t)W * H * 3), j;
    for (size_t i = 0; i < F.size(); i++) { rgb[i * 3] = (uint8_t)(F[i] >> 16); rgb[i * 3 + 1] = (uint8_t)(F[i] >> 8); rgb[i * 3 + 2] = (uint8_t)F[i]; }
    stbi_write_jpg_to_func(stbPiste, &j, W, H, 3, rgb.data(), k.maxKvalita(W, H));
    const double psStb = psnrJpeg(j, F, W, H);
    printf("  %dx%d: klicovy PSNR %.2f dB (stb %.2f dB), zmena v rohu -> obdelnik %ux%u na %u,%u, PSNR %.2f dB\n", W, H, ps1, psStb, c.w, c.h, c.x, c.y, ps2);
    // (u maleho 33x17 je novy ostry fialovy ctverec 6 % obrazu - PSNR celku klesne o ~1 dB)
    over(ok && kraj && std::fabs(ps1 - psStb) < 0.05 && ps2 > ps1 - 1.5, "%dx%d: JPEG cely (stejny jako stb) i obdelnik na kraji obrazu se rozbali spravne", W, H);
  }
}

int main(int argc, char **argv) {
  printf("B305: kodeky prenosu Sega / PS1 na TV / PC (skutecne hlavicky z appky)\n");
  testSega();
  testPs1(argc > 1 ? argv[1] : "ps1_320", argc > 2 ? argv[2] : "ps1_640");
  testRozmery();
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", g_kontrol, g_chyb);
  return g_chyb ? 1 : 0;
}
