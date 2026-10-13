// B306: rozbocovac prenosu (app/src/main/cpp/tvproud/nap_tv_proud.h) - novinka B306:
// kdyz se obraz zadrhne (demo na pomalem GPU, nacitani hry), ale zvuk bezi dal, posilaji
// se pakety jen se zvukem (obraz "beze zmeny"), at zvuk na TV nevypadne.
//
//   vlakno "jadra": 60 snimku/s, 1 s, pak 1 s ZADNY snimek (zvuk bezi dal), pak zase snimky
//   vlakno "zvuku": po 10 ms (480 vzorku stereo, 48 kHz), cislovane vzorky
//   vlakno "prohlizece": dalsi() dokola jako Java
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -o test_hub6 test_hub6.cpp
#include "nap_tv_proud.h"
#include "nap_tv_proud_index.h"
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
using namespace nap::proud;

int main() {
  Hub<KodekIndex> hub("TEST", 48000);
  std::atomic<bool> konec{false}, stuj{false};
  std::atomic<long long> vzorek{0};
  const int id = hub.otevri();
  std::thread jadro([&] {
    std::vector<uint32_t> F(320 * 224);
    auto t = std::chrono::steady_clock::now();
    for (unsigned n = 0; !konec.load(); n++) {
      if (!stuj.load()) {
        for (size_t i = 0; i < F.size(); i++) F[i] = ((i + n) & 15) * 0x101010u;
        hub.snimek(F.data(), 320, 224);
      }
      t += std::chrono::microseconds(16667);
      std::this_thread::sleep_until(t);
    }
  });
  std::thread zvuk([&] {
    std::vector<int16_t> b(960);
    auto t = std::chrono::steady_clock::now();
    while (!konec.load()) {
      for (int i = 0; i < 480; i++) { const long long v = vzorek++; b[2 * i] = (int16_t)(v & 0x7FFF); b[2 * i + 1] = (int16_t)((v >> 15) & 0x7FFF); }
      hub.zvuk(b.data(), b.size());
      t += std::chrono::milliseconds(10);
      std::this_thread::sleep_until(t);
    }
  });
  // prohlizec: pakety, zvuk navazuje? kolik paketu bez noveho obrazu behem zastaveni
  long long pak = 0, jenZvuk = 0, vz = 0, skoku = 0, posledni = -1, maxMezeraMs = 0;
  long long pakZastav = 0, vzZastav = 0;
  long long tPosl = tedMs();
  std::vector<uint8_t> pk;
  const long long t0 = tedMs();
  while (tedMs() - t0 < 3000) {
    const long long t = tedMs() - t0;
    stuj = (t >= 1000 && t < 2000);
    if (hub.dalsi(id, 250, 0, pk) != 1) continue;
    const long long ted = tedMs();
    if (pak > 0) maxMezeraMs = std::max(maxMezeraMs, ted - tPosl);
    tPosl = ted;
    pak++;
    const uint8_t *p = pk.data() + 4;
    const bool beze = (p[3] & 4) != 0;
    size_t o = 28;
    const unsigned pz = p[o] | (p[o + 1] << 8);
    o += 2 + 4 * pz;
    const size_t dl = (size_t)p[o] | ((size_t)p[o + 1] << 8) | ((size_t)p[o + 2] << 16) | ((size_t)p[o + 3] << 24);
    o += 4 + dl;
    const unsigned an = p[o] | (p[o + 1] << 8), kan = p[o + 2];
    o += 3;
    for (unsigned i = 0; i < an; i++) {
      const int16_t a = (int16_t)(p[o] | (p[o + 1] << 8)), b2 = kan == 2 ? (int16_t)(p[o + 2] | (p[o + 3] << 8)) : a;
      o += kan == 2 ? 4 : 2;
      const long long v = (long long)a | ((long long)b2 << 15);
      if (posledni >= 0 && v != posledni + 1) skoku++;
      posledni = v;
    }
    vz += an;
    if (t >= 1100 && t < 1950) { pakZastav++; vzZastav += an; if (beze) jenZvuk++; }
  }
  konec = true;
  jadro.join();
  zvuk.join();
  hub.zavri(id);
  std::printf("za 3 s: %lld paketu, zvuk %lld vzorku (%.0f/s), skoku ve zvuku %lld, nejdelsi mezera mezi pakety %lld ms\n",
              pak, vz, vz / 3.0, skoku, maxMezeraMs);
  std::printf("behem zastaveneho obrazu (1,1-1,95 s): %lld paketu, z toho jen se zvukem (beze zmeny) %lld, zvuk %lld vzorku\n",
              pakZastav, jenZvuk, vzZastav);
  std::printf("%s", hub.vezmiLog().c_str());
  int chyb = 0;
  // pod ThreadSanitizerem (NAP_TSAN=1) je vse ~10x pomalejsi - jen zavody, bez kontrol casu
  const bool tsan = getenv("NAP_TSAN") != nullptr;
  auto over = [&](bool ok, const char *co) { if (tsan) return; std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co); if (!ok) chyb++; };
  over(pakZastav >= 10 && jenZvuk == pakZastav, "obraz stoji -> pakety jen se zvukem (obraz beze zmeny) kazdych ~60 ms");
  over(vzZastav > 48000 * 0.85 * 0.85, "behem zastaveni prijde ~cely zvuk (nevypadne)");
  over(skoku == 0 && vz > 48000 * 2.8, "zvuk navazuje vzorek po vzorku po celou dobu (zadny skok)");
  over(maxMezeraMs < 120, "zadna mezera v paketech delsi nez 120 ms (B305: cela doba zastaveni)");
  std::printf("VYSLEDEK: %d chyb\n", chyb);
  return chyb ? 1 : 0;
}
