// test_overeni/b319/test_b319_hub.cpp
// B319: Hub ve vlaknech jako v appce - jadro 60 snimku / s (hra kresli kazdy 3. snimek, posun mapy
// v 2x Enhanced Resolution), zvuk po 10 ms, dva prohlizece soubezne: stranka B319 (volby 3: kopie +
// polovicni rozliseni) a stara (volby 0), kazdy se svou "siti" (nahodne zdrzeni zapisu = vynechane
// snimky, posun 2x / 3x vetsi). Na konci obraz chvili stoji: oba prohlizece musi mit presne posledni
// snimek (PSNR), zadna chyba v datech. Pod ThreadSanitizerem (-O1 -g -fsanitize=thread, NAP_TSAN=1)
// kratsi beh.
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb
//       -o test_b319_hub test_b319_hub.cpp && ./test_b319_hub
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <random>
#include <string>
#include <thread>
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

int main() {
  const bool tsan = std::getenv("NAP_TSAN") != nullptr;
  const int SNIMKU = tsan ? 240 : 900;            // snimku jadra (60 / s)
  std::printf("B319 - Hub ve vlaknech: jadro 60 snimku / s (posun mapy 2x), zvuk, prohlizec B319 a stary soubezne%s\n\n", tsan ? " (ThreadSanitizer)" : "");
  Hub<KodekJpeg> hub("PS1", 44100);
  hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); }, "test");
  std::atomic<bool> konec{false};
  std::vector<uint32_t> posledni;
  std::mutex mPosl;
  int W = 0, H = 0;

  // jadro: hra kresli kazdy 3. snimek, mapa se posouva (8 bodu hry), na konci 1 s klid
  std::thread jadro([&] {
    HraMapa hra;
    hra.vytvor(21);
    std::vector<uint32_t> F;
    int w = 0, h = 0;
    for (int i = 0; i < SNIMKU + 60; i++) {
      if (i % 3 == 0 && i < SNIMKU) {
        const int faze = (i / 90) % 4;
        hra.krok(faze == 0 ? 8 : (faze == 2 ? -8 : 0), faze == 1 ? 8 : (faze == 3 ? -4 : 0));
        hra.kurX = 150 + (i / 3) % 90;
        hra.kresli(F, w, h, true);
        std::lock_guard<std::mutex> l(mPosl);
        posledni = F; W = w; H = h;
      }
      if (!F.empty()) hub.snimek(F.data(), w, h);
      std::this_thread::sleep_for(std::chrono::microseconds(tsan ? 16000 : 16600));
    }
    konec = true;
  });
  // zvuk po 10 ms
  std::thread zvuk([&] {
    std::vector<int16_t> s(882);
    int t = 0;
    while (!konec) {
      for (size_t i = 0; i < s.size(); i += 2) { s[i] = s[i + 1] = (int16_t)(3000 * ((t++ / 50) & 1)); }
      hub.zvuk(s.data(), s.size());
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
  struct Vysl { ProhlizecB319 pr; long long bajtu = 0, paketu = 0; };
  Vysl v[2];
  auto prohlizec = [&](int k, int volby, unsigned seed) {
    const int id = hub.otevri(volby);
    std::mt19937 rng(seed);
    std::vector<uint8_t> pk;
    int zapis = 0;
    while (!konec) {
      const int r = hub.dalsi(id, 100, zapis, pk);
      if (r < 0) break;
      if (r == 0) continue;
      v[k].pr.prijmi(pk);
      v[k].bajtu += (long long)pk.size(); v[k].paketu++;
      // "sit": obcas zdrzi (vynechane snimky - posun 2x / 3x vetsi)
      zapis = (rng() % 10 == 0) ? 20 + (int)(rng() % 30) : (int)(rng() % 3);
      std::this_thread::sleep_for(std::chrono::milliseconds(zapis));
    }
    hub.zavri(id);
  };
  std::thread p0(prohlizec, 0, VOLBA_POSUN | VOLBA_POLOVICNI, 7u), p1(prohlizec, 1, 0, 9u);
  jadro.join();
  zvuk.join();
  p0.join();
  p1.join();
  std::string l = hub.vezmiLog();
  for (size_t p; (p = l.find('\n')) != std::string::npos; l.erase(0, p + 1)) std::printf("   log: %s\n", l.substr(0, p).c_str());
  double ps[2], ct[2];
  for (int k = 0; k < 2; k++) {
    ps[k] = (v[k].pr.W == W && v[k].pr.H == H) ? psnrB(posledni, v[k].pr.canvas, W, 0, 0, W, H) : 0;
    ct[k] = (v[k].pr.W == W && v[k].pr.H == H) ? nejhorsiCtverec(posledni, v[k].pr.canvas, W, H) : 999;
    std::printf("   prohlizec %s: paketu %lld, %.2f MB, kopii %lld, polovicnich %lld, chyb %lld; posledni snimek PSNR %.2f dB, nejhorsi ctverec %.1f\n",
                k ? "stary" : "B319 ", v[k].paketu, v[k].bajtu / 1e6, v[k].pr.kopii, v[k].pr.polovicnich, v[k].pr.chyb, ps[k], ct[k]);
  }
  over(v[0].pr.chyb == 0 && v[1].pr.chyb == 0 && v[0].pr.spatnyPriznak == 0 && v[0].pr.zvukVDalsi == 0, "1) oba prohlizece: vsechny pakety jdou rozebrat");
  over(ps[0] >= 34.0 && ct[0] <= 8.0 && ps[1] >= 34.0, "2) po chvili klidu maji oba prohlizece posledni snimek (B319 " + std::to_string(ps[0]).substr(0, 5) +
                                                            " dB, stary " + std::to_string(ps[1]).substr(0, 5) + " dB)");
  over(v[0].pr.kopii > 10 && v[0].pr.polovicnich > 10 && v[1].pr.kopii == 0 && v[1].pr.polovicnich == 0,
       "3) kopie a polovicni rozliseni jen prohlizec B319 (kopii " + std::to_string(v[0].pr.kopii) + "), stary nic");
  over(tsan || v[0].bajtu * 2 < v[1].bajtu, "4) B319 poslal mene dat nez stary (" + std::to_string(v[0].bajtu / 1024) + " kB proti " +
                                                std::to_string(v[1].bajtu / 1024) + " kB)");
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
