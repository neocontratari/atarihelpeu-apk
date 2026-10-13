// B305: ThreadSanitizer nad SKUTECNYMI nap_sega_native_proof.cpp / nap_ps1_native.cpp
// (tv_sega.cpp / tv_ps1.cpp): vlakno jadra, vlakno zvuku a dva prohlizece soubezne.
//   g++ -std=c++17 -O1 -g -fsanitize=thread ... -DJADRO_SEGA (nebo -DJADRO_PS1) tsan_jadra.cpp
#include <cstdio>
#include <thread>
#include <vector>
#ifdef JADRO_SEGA
#include "tv_sega.cpp"
#define J(x) sega_##x
#else
#include "tv_ps1.cpp"
#define J(x) ps1_##x
#endif
int main(int argc, char **argv) {
#ifdef JADRO_SEGA
  J(start)();
#else
  printf("start: %s\n", J(start)(argc > 1 ? argv[1] : "ps1_320", argc > 2 ? argv[2] : "."));
#endif
  std::atomic<long long> paketu{0};
  auto prohlizec = [&](bool pomaly) {
    const int id = J(otevri)();
    std::vector<uint8_t> b(1 << 20);
    const auto konec = std::chrono::steady_clock::now() + std::chrono::seconds(4);
    while (std::chrono::steady_clock::now() < konec) {
      const int n = J(dalsi)(id, b.data(), (int)b.size(), 250, pomaly ? 20 : 0);
      if (n > 0) paketu++;
      if (pomaly) std::this_thread::sleep_for(std::chrono::milliseconds(70));
      J(stav)();
    }
    J(zavri)(id);
  };
  std::thread a(prohlizec, false), b(prohlizec, true);
  a.join(); b.join();
  printf("paketu %lld\n%s", paketu.load(), J(log)());
  J(stop)();
  return 0;
}
