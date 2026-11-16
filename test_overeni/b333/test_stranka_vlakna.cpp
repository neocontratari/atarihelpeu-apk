// B333: nap_tv_stranka.h ve VLAKNECH jako v appce (preklad s -fsanitize=thread):
//  - vlakno "nap-tv-stranka" (NapTvStranka.java): snimekRgba 15x za sekundu, stranka na vysku / na sirku,
//  - vlakno JavaScriptu prehravace (pushPlayerPcm16 -> strankaZvukSafe): zvuk 48 kHz po kusech,
//  - 2 vlakna serveru TV (2 prohlizece): otevri, dalsi, zavri, znovu (prohlizec se obnovi),
//  - vlakno logu (NapTvStranka kazdych 10 s: strankaLogSafe -> vezmiLog).
// Kontrola: zadny zavod (ThreadSanitizer), kazdy prohlizec dostal pakety se spravnym pomerem stran,
// zvuk v paketech, po skonceni nikdo neceka. Vystup: VYSTUP_test_stranka_vlakna_tsan.txt
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <thread>
#include <vector>

#include "nap_tv_stranka.h"

using namespace nap::proud;

int main() {
  Stranka st;
  auto &hub = st.hub();
  std::atomic<bool> konec{false};
  std::atomic<long long> paketu{0}, spatnyPomer{0}, zvukovychVzorku{0}, otevreni{0};
  std::vector<uint8_t> vyska((size_t)582 * 1080 * 4), sirka((size_t)1920 * 1034 * 4);
  for (size_t i = 0; i < vyska.size(); i++) vyska[i] = (uint8_t)(i * 7 % 251);
  for (size_t i = 0; i < sirka.size(); i++) sirka[i] = (uint8_t)(i * 13 % 241);

  std::thread foto([&] {
    for (int n = 0; !konec.load(); n++) {
      // kazdy 4. snimek s malou zmenou, kazdych 20 otoceni telefonu
      std::vector<uint8_t> &s = (n / 20) % 2 ? sirka : vyska;
      const int w = (n / 20) % 2 ? 1920 : 582, h = (n / 20) % 2 ? 1034 : 1080;
      if (n % 4 == 0) s[(size_t)(n * 4099) % s.size()] ^= 0x55;
      st.snimekRgba(s.data(), w, h, w * 4);
      std::this_thread::sleep_for(std::chrono::milliseconds(66));
    }
  });
  std::thread zvuk([&] {
    std::vector<uint8_t> b;
    long long pos = 0;
    while (!konec.load()) {
      const int n = 1024;
      b.resize((size_t)n * 2);
      for (int i = 0; i < n; i++) {
        const int16_t v = (int16_t)std::lround(8000.0 * std::sin(2 * M_PI * 440.0 * (double)(pos + i) / 48000.0));
        b[(size_t)i * 2] = (uint8_t)(v & 255);
        b[(size_t)i * 2 + 1] = (uint8_t)((uint16_t)v >> 8);
      }
      pos += n;
      st.zvuk(b.data(), b.size(), 1, 48000);
      std::this_thread::sleep_for(std::chrono::milliseconds(21));
    }
  });
  auto prohlizec = [&](int kolo) {
    std::vector<uint8_t> pk;
    for (int r = 0; r < kolo && !konec.load(); r++) {
      const int id = hub.otevri();
      otevreni++;
      const auto t0 = std::chrono::steady_clock::now();
      while (!konec.load() && std::chrono::steady_clock::now() - t0 < std::chrono::milliseconds(2500)) {
        const int v = hub.dalsi(id, 250, 5, pk);
        if (v < 0) break;
        if (v == 0) continue;
        size_t o = 0;
        while (o + 4 <= pk.size()) {
          const uint32_t len = (uint32_t)pk[o] | ((uint32_t)pk[o + 1] << 8) | ((uint32_t)pk[o + 2] << 16) | ((uint32_t)pk[o + 3] << 24);
          const uint8_t *p = &pk[o + 4];
          const int W = p[4] | (p[5] << 8), H = p[6] | (p[7] << 8), px = p[20], py = p[21];
          if (!((W == 582 && H == 1080 && px == 97 && py == 180) || (W == 1920 && H == 1034 && px == 255 && py == 137))) spatnyPomer++;
          if (!(p[3] & 8)) {
            const size_t q = 28 + 9;
            const uint32_t dl = (uint32_t)p[q] | ((uint32_t)p[q + 1] << 8) | ((uint32_t)p[q + 2] << 16) | ((uint32_t)p[q + 3] << 24);
            zvukovychVzorku += p[q + 4 + dl] | (p[q + 5 + dl] << 8);
          }
          paketu++;
          o += 4 + len;
        }
      }
      hub.zavri(id);
    }
  };
  std::thread pr1(prohlizec, 3), pr2(prohlizec, 2);
  std::thread log([&] { while (!konec.load()) { (void)hub.vezmiLog(); std::this_thread::sleep_for(std::chrono::milliseconds(300)); } });
  pr1.join();
  pr2.join();
  konec = true;
  foto.join();
  zvuk.join();
  log.join();
  const bool ok = paketu > 50 && spatnyPomer == 0 && zvukovychVzorku > 44100 && otevreni == 5 && hub.klientu() == 0;
  std::printf("%s  vlakna (foceni 15/s s otacenim, zvuk 48 kHz, 2 prohlizece 5x pripojeni, log): paketu %lld, spatny pomer stran %lld, "
              "zvuku %lld vzorku, otevreni %lld, zbyva prohlizecu %d\n",
              ok ? "OK    " : "CHYBA ", paketu.load(), spatnyPomer.load(), zvukovychVzorku.load(), otevreni.load(), hub.klientu());
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", ok ? 1 : 0, ok ? 0 : 1);
  return ok ? 0 : 1;
}
