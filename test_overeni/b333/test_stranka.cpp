// B333: STRANKY APPKY NA TV Z C++ (bez Javy) - SKUTECNY nap_tv_stranka.h + Hub + KodekJpeg z appky.
// "Prohlizec" = test_overeni/b313/prohlizec_v3.h (rozebere pakety PRESNE jako stranka TV v appce, JPEG
// rozbali stb_image). Stranka = synteticka stranka OPTIONS (na vysku, text, tlacitka) jako z Javy:
// Bitmap ARGB_8888 = bajty R G B A, zmensena na 582 x 1080 (NapTvStranka.zmensi).
//  F) bez divaka se nekoduje nic, jen "stranka bezi",
//  A) prvni snimek: prohlizec ma celou stranku (PSNR), pomer stran 97:180 v hlavicce paketu,
//  B) stejny snimek znovu (stranka stoji): zjemneni (text ostrejsi), pak uz jen prazdne pakety,
//  C) zmena jednoho tlacitka: posle se jen ta cast (ne cela stranka), prohlizec ji ma,
//  D) stranka na sirku (1920 x 1034): pomer stran 255:137 v paketu, prohlizec ma novy obraz,
//  P) pomer stran pro ruzne rozmery (bajty 1-255, odchylka pod 1 %),
//  E) zvuk prehravace 48 kHz mono po kusech ruzne delky -> 44,1 kHz v paketech: 1 kHz ton,
//     zadne skoky mezi kusy, delka sedi; 44,1 kHz stereo projde beze zmeny (bit po bitu),
//  R) radkovani (stride vetsi nez sirka * 4) - obraz stejny,
//  zapise proud.bin + snimek_*.raw pro test stranky TV v Chromiu (test_b333_tv.py).
// Preklad (jeden radek): g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb
//              -I../b313 -o PRAC/test_stranka test_stranka.cpp
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "nap_tv_stranka.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "prohlizec_v3.h"

using namespace nap::proud;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string d1(double v) { char b[32]; std::snprintf(b, sizeof b, "%.1f", v); return b; }
static std::string d2(double v) { char b[32]; std::snprintf(b, sizeof b, "%.2f", v); return b; }

// stranka OPTIONS: tmave pozadi, tlacitka s "textem" (ostre hrany), jako Bitmap z PixelCopy: R G B A
static std::vector<uint8_t> stranka(int w, int h, int zvyrazni) {
  std::vector<uint8_t> p((size_t)w * h * 4);
  for (int y = 0; y < h; y++)
    for (int x = 0; x < w; x++) {
      uint8_t r = (uint8_t)(20 + y * 10 / h), g = (uint8_t)(12 + x * 6 / w), b = 6;
      const int radek = y / 54, vRadku = y % 54;
      const bool tlacitko = vRadku >= 8 && vRadku < 46 && x >= 24 && x < w - 24;
      if (tlacitko) {
        r = 59; g = 42; b = 16;
        if (radek == zvyrazni) { r = 255; g = 212; b = 121; }
        const int sl = (x - 40) % 13;
        if (vRadku >= 18 && vRadku < 36 && x > 40 && x < w / 2 + (radek * 37) % (w / 3) && sl < 2 + (x / 13 + radek) % 3) {
          r = 232; g = 201; b = 141;
          if (radek == zvyrazni) { r = 30; g = 20; b = 8; }
        }
      }
      uint8_t *d = &p[((size_t)y * w + x) * 4];
      d[0] = r; d[1] = g; d[2] = b; d[3] = 255;
    }
  return p;
}
static std::vector<uint32_t> naArgb(const std::vector<uint8_t> &p, int w, int h) {
  std::vector<uint32_t> o((size_t)w * h);
  for (size_t i = 0; i < o.size(); i++) o[i] = ((uint32_t)p[i * 4] << 16) | ((uint32_t)p[i * 4 + 1] << 8) | p[i * 4 + 2];
  return o;
}

static FILE *g_proud = nullptr;   // proud.bin pro test v Chromiu
static int g_pomerX = 0, g_pomerY = 0, g_pomerSpatne = 0;

// zvuk z paketu (stejne jako stranka TV): u16 vzorku, u8 kanalu (1 = L i R), int16 -> stereo
static void zvukZPaketu(const std::vector<uint8_t> &pk, std::vector<int16_t> &ven) {
  size_t o = 0;
  while (o + 4 <= pk.size()) {
    const uint32_t len = Prohlizec::u32(&pk[o]);
    const uint8_t *p = &pk[o + 4];
    size_t q = 28 + 9;
    const uint32_t dl = Prohlizec::u32(p + q);
    q += 4 + dl;
    const unsigned an = Prohlizec::u16(p + q);
    const unsigned kan = p[q + 2];
    q += 3;
    for (unsigned i = 0; i < an; i++) {
      if (kan == 2) { ven.push_back((int16_t)Prohlizec::u16(p + q + i * 4)); ven.push_back((int16_t)Prohlizec::u16(p + q + i * 4 + 2)); }
      else { const int16_t v = (int16_t)Prohlizec::u16(p + q + i * 2); ven.push_back(v); ven.push_back(v); }
    }
    o += 4 + len;
  }
}

// vsechny pakety, ktere ted ceka (jako vlakno serveru v Jave)
static int vyber(Hub<KodekJpeg> &hub, int id, Prohlizec &pr, long long *bajtu = nullptr, int ocekX = 0, int ocekY = 0) {
  int n = 0;
  std::vector<uint8_t> pk;
  for (int i = 0; i < 20; i++) {
    const int r = hub.dalsi(id, i == 0 ? 400 : 30, 0, pk);
    if (r <= 0) break;
    pr.prijmi(pk);
    if (g_proud) std::fwrite(pk.data(), 1, pk.size(), g_proud);
    if (bajtu) *bajtu += (long long)pk.size();
    g_pomerX = pk[4 + 20];
    g_pomerY = pk[4 + 21];
    if (ocekX && (g_pomerX != ocekX || g_pomerY != ocekY)) g_pomerSpatne++;
    n++;
  }
  return n;
}

static void uloz(const std::string &jmeno, const std::vector<uint32_t> &a) {
  FILE *f = std::fopen(jmeno.c_str(), "wb");
  if (f) { std::fwrite(a.data(), 4, a.size(), f); std::fclose(f); }
}

int main(int argc, char **argv) {
  const std::string prac = argc > 1 ? argv[1] : ".";
  std::printf("B333 - stranky appky na TV z C++ (SKUTECNY nap_tv_stranka.h + Hub + KodekJpeg, prohlizec jako stranka TV)\n");
  Stranka st;
  auto &hub = st.hub();
  g_proud = std::fopen((prac + "/proud.bin").c_str(), "wb");

  // ---- F) bez divaka ----
  const int W = 582, H = 1080;
  auto s0 = stranka(W, H, -1);
  st.snimekRgba(s0.data(), W, H, W * 4);
  over(hub.klientu() == 0 && hub.bezi(2000), "F) bez divaka: nic se nekoduje, jen \"stranka bezi\" (Java bez divaka ani nefoti)");

  const int id = hub.otevri();
  Prohlizec pr;
  // ---- A) prvni snimek ----
  int px, py;
  Stranka::pomer(W, H, px, py);
  st.snimekRgba(s0.data(), W, H, W * 4);
  long long bA = 0;
  const int nA = vyber(hub, id, pr, &bA, px, py);
  const auto a0 = naArgb(s0, W, H);
  const double pA = pr.W == W && pr.H == H ? psnr(pr.canvas, a0, W, 0, 0, W, H) : 0.0;
  over(nA >= 1 && pr.W == W && pr.H == H && pA > 30.0 && pr.chyb == 0,
       "A) prvni snimek: prohlizec ma celou stranku " + std::to_string(pr.W) + "x" + std::to_string(pr.H) + ", PSNR " + d1(pA) +
       " dB, " + std::to_string(bA / 1024) + " kB, casti " + std::to_string(pr.castiPoslednihoSnimku) + ", chyb " + std::to_string(pr.chyb));
  over(px == 97 && py == 180 && g_pomerX == 97 && g_pomerY == 180 && g_pomerSpatne == 0,
       "A) pomer stran 582 x 1080 -> " + std::to_string(g_pomerX) + ":" + std::to_string(g_pomerY) + " v hlavicce paketu (na TV na vysku, ne 16:9)");
  uloz(prac + "/snimek_A.raw", a0);

  // ---- B) stranka stoji ----
  long long bB = 0;
  int nB = 0;
  for (int i = 0; i < 16; i++) { st.snimekRgba(s0.data(), W, H, W * 4); nB += vyber(hub, id, pr, &bB, px, py); }
  const double pB = psnr(pr.canvas, a0, W, 0, 0, W, H);
  over(nB >= 1 && pB > pA + 1.0, "B) stranka stoji (16 snimku): zjemneni " + std::to_string(bB / 1024) + " kB celkem, PSNR " + d1(pA) + " -> " + d1(pB) + " dB (text ostrejsi)");
  long long bB2 = 0;
  for (int i = 0; i < 8; i++) { st.snimekRgba(s0.data(), W, H, W * 4); vyber(hub, id, pr, &bB2, px, py); }
  over(bB2 < 8 * 200, "B) po zjemneni uz jen prazdne pakety: " + std::to_string(bB2) + " B za 8 snimku (H.264 pres Javu posilalo 30 celych snimku/s dokola)");

  // ---- C) zmena jednoho tlacitka ----
  auto s1 = stranka(W, H, 5);
  st.snimekRgba(s1.data(), W, H, W * 4);
  long long bC = 0;
  vyber(hub, id, pr, &bC, px, py);
  const auto a1 = naArgb(s1, W, H);
  const double pC = psnr(pr.canvas, a1, W, 0, 5 * 54, W, 6 * 54);
  const double pCcela = psnr(pr.canvas, a1, W, 0, 0, W, H);
  int maxV = 0;
  for (const auto &c : pr.castiSnimku) maxV = std::max(maxV, c.h);
  over(pC > 30.0 && maxV <= 64 && bC < bA / 3 && pCcela > 30.0,
       "C) zmena jednoho tlacitka: poslana jen ta cast (" + std::to_string(bC / 1024) + " kB misto " + std::to_string(bA / 1024) + " kB, vyska casti " +
       std::to_string(maxV) + " bodu), PSNR tlacitka " + d1(pC) + " dB, cele stranky " + d1(pCcela) + " dB");
  for (int i = 0; i < 16; i++) { st.snimekRgba(s1.data(), W, H, W * 4); vyber(hub, id, pr, nullptr, px, py); }
  uloz(prac + "/snimek_C.raw", a1);

  // ---- D) na sirku ----
  const int W2 = 1920, H2 = 1034;
  auto s2 = stranka(W2, H2, 2);
  int px2, py2;
  Stranka::pomer(W2, H2, px2, py2);
  st.snimekRgba(s2.data(), W2, H2, W2 * 4);
  vyber(hub, id, pr, nullptr, px2, py2);
  const auto a2 = naArgb(s2, W2, H2);
  const double pD = pr.W == W2 && pr.H == H2 ? psnr(pr.canvas, a2, W2, 0, 0, W2, H2) : 0.0;
  over(pr.W == W2 && pr.H == H2 && pD > 30.0 && g_pomerX == 255 && g_pomerY == 137 && g_pomerSpatne == 0,
       "D) stranka na sirku 1920 x 1034: prohlizec " + std::to_string(pr.W) + "x" + std::to_string(pr.H) + ", PSNR " + d1(pD) + " dB, pomer v paketu " +
       std::to_string(g_pomerX) + ":" + std::to_string(g_pomerY) + " (" + d2(255.0 / 137) + " misto " + d2(1920.0 / 1034) + ")");
  for (int i = 0; i < 16; i++) { st.snimekRgba(s2.data(), W2, H2, W2 * 4); vyber(hub, id, pr, nullptr, px2, py2); }
  uloz(prac + "/snimek_D.raw", a2);
  // zpet na vysku (otoceni telefonu zpet) - pro test v Chromiu; znacka = kde v proud.bin zacina
  // (server v testu posle proud az sem, prohlizec ukaze stranku na sirku, pak zbytek)
  if (g_proud) {
    std::fflush(g_proud);
    FILE *f = std::fopen((prac + "/proud_znacka.txt").c_str(), "w");
    if (f) { std::fprintf(f, "%ld\n", std::ftell(g_proud)); std::fclose(f); }
  }
  st.snimekRgba(s0.data(), W, H, W * 4);
  vyber(hub, id, pr, nullptr, px, py);
  for (int i = 0; i < 16; i++) { st.snimekRgba(s0.data(), W, H, W * 4); vyber(hub, id, pr, nullptr, px, py); }
  const double pZ = pr.W == W && pr.H == H ? psnr(pr.canvas, a0, W, 0, 0, W, H) : 0.0;
  over(pr.W == W && pr.H == H && pZ > 35.0 && g_pomerX == 97 && g_pomerY == 180 && pr.chyb == 0,
       "D) zpet na vysku: prohlizec " + std::to_string(pr.W) + "x" + std::to_string(pr.H) + ", PSNR " + d1(pZ) + " dB, pomer " + std::to_string(g_pomerX) + ":" +
       std::to_string(g_pomerY) + ", chyb prohlizece " + std::to_string(pr.chyb));
  if (g_proud) { std::fclose(g_proud); g_proud = nullptr; }
  hub.zavri(id);

  // ---- P) pomer stran ----
  {
    const int rozm[][2] = {{582, 1080}, {1920, 1034}, {1080, 1920}, {1920, 1080}, {608, 1080}, {1918, 1080}, {2, 2}, {3998, 2}, {1080, 582}, {1, 1}};
    std::string vypis;
    bool ok = true;
    for (const auto &r : rozm) {
      int a, b;
      Stranka::pomer(r[0], r[1], a, b);
      const double chybaPomeru = std::fabs((double)a / b / ((double)r[0] / r[1]) - 1.0);
      const bool dobre = a >= 1 && b >= 1 && a <= 255 && b <= 255 && (chybaPomeru < 0.01 || (r[0] == 3998));
      ok = ok && dobre;
      vypis += " " + std::to_string(r[0]) + "x" + std::to_string(r[1]) + "->" + std::to_string(a) + ":" + std::to_string(b);
    }
    over(ok, "P) pomer stran (1-255, odchylka pod 1 %):" + vypis);
  }

  // ---- E) zvuk prehravace ----
  {
    Stranka s3;
    auto &h3 = s3.hub();
    const int id3 = h3.otevri();
    auto sp = stranka(64, 64, -1);
    std::vector<uint8_t> pk;
    std::vector<int16_t> ven;
    s3.snimekRgba(sp.data(), 64, 64, 64 * 4);
    h3.dalsi(id3, 300, 0, pk);                       // prvni paket: zvuk zacina od ted
    long long pos = 0;
    const long long celkem = 48000 * 2;              // 2 s, 1 kHz ton, 48 kHz mono (JS prehravace: pushPlayerPcm16)
    const int kusy[] = {1000, 2400, 1700, 512, 2048};
    int ki = 0;
    std::vector<uint8_t> b;
    while (pos < celkem) {
      const int n = (int)std::min<long long>(kusy[ki++ % 5], celkem - pos);
      b.resize((size_t)n * 2);
      for (int i = 0; i < n; i++) {
        const int16_t v = (int16_t)std::lround(12000.0 * std::sin(2 * M_PI * 1000.0 * (double)(pos + i) / 48000.0));
        b[(size_t)i * 2] = (uint8_t)(v & 255);
        b[(size_t)i * 2 + 1] = (uint8_t)((uint16_t)v >> 8);
      }
      s3.zvuk(b.data(), b.size(), 1, 48000);
      pos += n;
      s3.snimekRgba(sp.data(), 64, 64, 64 * 4);       // obraz stranky (paket nese zvuk od minula)
      if (h3.dalsi(id3, 300, 0, pk) > 0) zvukZPaketu(pk, ven);
    }
    const size_t ramcu = ven.size() / 2;
    int pruchodu = 0, skok = 0;
    bool stejneKanaly = true;
    for (size_t i = 1; i < ramcu; i++) {
      if ((ven[(i - 1) * 2] < 0) != (ven[i * 2] < 0)) pruchodu++;
      skok = std::max(skok, std::abs((int)ven[i * 2] - (int)ven[(i - 1) * 2]));
      if (ven[i * 2] != ven[i * 2 + 1]) stejneKanaly = false;
    }
    const double sekund = (double)ramcu / 44100.0, hz = sekund > 0 ? pruchodu / 2.0 / sekund : 0;
    // nejvetsi skok 1 kHz tonu 12000 pri 44,1 kHz: 12000 * 2 * pi * 1000 / 44100 = 1710
    over(std::labs((long)ramcu - 88200) <= 2 && std::fabs(hz - 1000.0) < 5.0 && skok <= 1712 && stejneKanaly,
         "E) zvuk prehravace 48 kHz mono po kusech 1000 / 2400 / 1700 / 512 / 2048 -> " + std::to_string(ramcu) + " ramcu 44,1 kHz (" + d2(sekund) +
         " s), ton " + d1(hz) + " Hz, nejvetsi skok mezi vzorky " + std::to_string(skok) + " (1 kHz ton max 1710 - bez lupnuti mezi kusy)");

    // 44,1 kHz stereo (YouTube): projde bit po bitu
    std::vector<int16_t> ven2, zdroj;
    for (int kus = 0; kus < 6; kus++) {
      const int n = 735 + kus * 100;
      b.resize((size_t)n * 4);
      for (int i = 0; i < n; i++) {
        const int16_t L = (int16_t)((kus * 7919 + i * 31) % 20000 - 10000), R = (int16_t)(-L / 2);
        zdroj.push_back(L);
        zdroj.push_back(R);
        b[(size_t)i * 4] = (uint8_t)(L & 255); b[(size_t)i * 4 + 1] = (uint8_t)((uint16_t)L >> 8);
        b[(size_t)i * 4 + 2] = (uint8_t)(R & 255); b[(size_t)i * 4 + 3] = (uint8_t)((uint16_t)R >> 8);
      }
      s3.zvuk(b.data(), b.size(), 2, 44100);
      s3.snimekRgba(sp.data(), 64, 64, 64 * 4);
      if (h3.dalsi(id3, 300, 0, pk) > 0) zvukZPaketu(pk, ven2);
    }
    over(ven2 == zdroj, "E) zvuk 44,1 kHz stereo (YouTube): " + std::to_string(ven2.size() / 2) + " ramcu, bit po bitu stejny jako z prehravace");
    h3.zavri(id3);
  }

  // ---- R) radkovani (stride) ----
  {
    Stranka s4;
    auto &h4 = s4.hub();
    const int id4 = h4.otevri();
    const int w = 100, h = 60, stride = w * 4 + 64;
    auto zdroj = stranka(w, h, 0);
    std::vector<uint8_t> sRadky((size_t)stride * h, 0xEE);
    for (int y = 0; y < h; y++) std::memcpy(&sRadky[(size_t)y * stride], &zdroj[(size_t)y * w * 4], (size_t)w * 4);
    Prohlizec p4;
    for (int i = 0; i < 20; i++) { s4.snimekRgba(sRadky.data(), w, h, stride); vyber(h4, id4, p4); }
    const double pR = p4.W == w && p4.H == h ? psnr(p4.canvas, naArgb(zdroj, w, h), w, 0, 0, w, h) : 0.0;
    over(pR > 35.0, "R) radky s mezerou (stride " + std::to_string(stride) + " misto " + std::to_string(w * 4) + "): obraz stejny, PSNR " + d1(pR) + " dB");
    // spatny vstup: nic se nestane
    s4.snimekRgba(nullptr, w, h, stride);
    s4.snimekRgba(sRadky.data(), w, h, w * 4 - 1);
    s4.zvuk(nullptr, 100, 1, 48000);
    s4.zvuk(sRadky.data(), 100, 3, 48000);
    s4.zvuk(sRadky.data(), 100, 1, 1000);
    over(true, "R) spatny vstup (prazdny obraz, kratky radek, 3 kanaly, 1000 Hz) - bez padu");
    h4.zavri(id4);
  }

  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
