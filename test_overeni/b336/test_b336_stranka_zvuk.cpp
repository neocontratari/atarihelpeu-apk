// test_overeni/b336/test_b336_stranka_zvuk.cpp
// ====================================================================
//  B336: ZVUK YOUTUBE V PROUDU STRANKY (C++ Stranka + Hub z appky) - DAVKA PO ZADRHNUTI STRANKY
//
//  Odbocka B336 (MediaStreamTrackProcessor) nic neztrati, ale po zadrhnuti stranky YouTube posle
//  zvuk DAVKOU (co se behem zadrhnuti nasbiralo). Hub do B335 pustil mezi dvema pakety nejvys 250 ms
//  zvuku - vic zahodil ("zvuk preskoku" v logu) = dira ve zvuku na TV. B336: Stranka 600 ms.
//   Z1) zadrhnuti 400 ms: B335 (250 ms) zahodi davku, B336 (600 ms) projde vsechno bez skoku
//   Z2) zadrhnuti 800 ms (vic nez 600): B336 zahodi jen to stare (jako drive), proud dal plynule
//   (PS1 / menu: Hub bez nastaveni dal 250 ms - kontrola_b336.py)
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb -I../b313 -o test_b336_stranka_zvuk test_b336_stranka_zvuk.cpp
// ====================================================================
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_jpeg.h"
#include "nap_tv_stranka.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "stb_image.h"
#include "prohlizec_v3.h"

using namespace nap::proud;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s%s\n", ok ? "OK     " : "CHYBA  ", co.c_str());
  std::fflush(stdout);
}

// zvuk z paketu (stejne jako stranka TV): u16 vzorku, u8 kanalu (1 = L i R) -> stereo int16
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

struct Vysledek { long long vstup = 0, vystup = 0; int skoku = 0; double nejSkok = 0; };

// YouTube: 48 kHz stereo po 2048 ramcich (jako odbocka). Paket se bere po kazdych ~60 ms zvuku (jako
// vlakno serveru: novy snimek / paket jen se zvukem). Zadrhnuti: "zadrh" ms nic neprijde, pak to cele
// najednou (davka) a az pak dalsi paket.
static Vysledek beh(int zvukMaxMs, int zadrhMs) {
  Stranka s;
  if (zvukMaxMs > 0) s.hub().zvukMaxMs(zvukMaxMs);
  auto &h = s.hub();
  const int id = h.otevri();
  std::vector<uint8_t> sp(64 * 64 * 4, 40), pk, b;
  std::vector<int16_t> ven;
  s.snimekRgba(sp.data(), 64, 64, 64 * 4);
  h.dalsi(id, 300, 0, pk);                         // prvni paket: zvuk zacina od ted
  Vysledek v;
  long long pos = 0;
  const int KUS = 2048;
  const double F = 440.0;
  auto posli = [&](int ramcu) {
    for (int done = 0; done < ramcu;) {
      const int n = std::min(KUS, ramcu - done);
      b.resize((size_t)n * 4);
      for (int i = 0; i < n; i++) {
        const int16_t x = (int16_t)std::lround(14000.0 * std::sin(2 * M_PI * F * (double)(pos + i) / 48000.0));
        b[(size_t)i * 4] = (uint8_t)(x & 255); b[(size_t)i * 4 + 1] = (uint8_t)((uint16_t)x >> 8);
        b[(size_t)i * 4 + 2] = b[(size_t)i * 4]; b[(size_t)i * 4 + 3] = b[(size_t)i * 4 + 1];
      }
      s.zvuk(b.data(), b.size(), 2, 48000);
      pos += n;
      done += n;
    }
  };
  auto paket = [&]() {
    s.snimekRgba(sp.data(), 64, 64, 64 * 4);
    if (h.dalsi(id, 300, 0, pk) > 0) zvukZPaketu(pk, ven);
  };
  // 3 s: kazdych 60 ms zvuku paket, v 1. a 2. vterine zadrhnuti
  const int R60 = 48000 * 60 / 1000;
  for (int t = 0; t < 3000; t += 60) {
    if (zadrhMs > 0 && (t == 900 || t == 1980)) {
      posli(48000 * zadrhMs / 1000);               // co se behem zadrhnuti nasbiralo - najednou
      t += zadrhMs;
    }
    posli(R60);
    paket();
  }
  h.zavri(id);
  v.vstup = pos;
  v.vystup = (long long)ven.size() / 2;
  // skoky: ton 440 Hz 14000 pri 44,1 kHz - rozdil sousednich vzorku nejvys 14000 * 2 pi 440 / 44100 = 878
  for (size_t i = 1; i < ven.size() / 2; i++) {
    const double d = std::fabs((double)ven[i * 2] - (double)ven[(i - 1) * 2]);
    if (d > v.nejSkok) v.nejSkok = d;
    if (d > 900) v.skoku++;
  }
  return v;
}

int main() {
  std::printf("B336 - zvuk YouTube v proudu stranky (C++): davka po zadrhnuti stranky\n");
  const double pomer = 44100.0 / 48000.0;
  const Vysledek a = beh(250, 400), b = beh(0, 400);
  over(a.vystup < (long long)(a.vstup * pomer * 0.95) && a.skoku >= 1 &&
           std::llabs(b.vystup - (long long)(b.vstup * pomer)) <= 4 && b.skoku == 0,
       "Z1) 2x zadrhnuti 400 ms: B335 (Hub 250 ms) na TV " + std::to_string(a.vystup) + " z " + std::to_string((long long)(a.vstup * pomer)) +
           " vzorku, skoku " + std::to_string(a.skoku) + "; B336 (Stranka 600 ms) " + std::to_string(b.vystup) + " vzorku - vsechno, skoku " +
           std::to_string(b.skoku) + " (nejvetsi rozdil vzorku " + std::to_string((int)b.nejSkok) + ", ton max 878)");
  const Vysledek c = beh(0, 800);
  over(c.vystup < (long long)(c.vstup * pomer) && c.skoku <= 2,
       "Z2) zadrhnuti 800 ms (vic nez 600): B336 zahodi jen stare (" + std::to_string((long long)(c.vstup * pomer) - c.vystup) +
           " vzorku), skoku " + std::to_string(c.skoku) + " - dal plynule");
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
