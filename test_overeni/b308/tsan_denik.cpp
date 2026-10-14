// B308: ThreadSanitizer - denik PS1, klavesnice z TV, ulozeni pozice a vlakno emulace
// soubezne (jako v telefonu: vlakno emulace, zvukova karta, HTTP vlakna prohlizece
// s klavesami, stranka cte denik, tlacitko SAVE, dotyk). SKUTECNY nap_ps1_native.cpp.
#include "ps1_harness.cpp"
#include <random>

int main(int argc, char **argv) {
  const char *prac = argc > 1 ? argv[1] : "/tmp/nap_b308_tsan";
  mkdir(prac, 0755);
  std::string hra = std::string(prac) + "/hra.cue";
  if (FILE *f = fopen(hra.c_str(), "w")) { fputs("FILE \"hra.bin\" BINARY\n  TRACK 01 MODE2/2352\n", f); fclose(f); }
  printf("start: %s\n", h_start(prac, hra.c_str()));
  std::atomic<bool> bez{true};
  std::vector<std::thread> v;
  static const char *K[] = {"KeyW", "KeyA", "KeyS", "KeyD", "KeyK", "KeyL", "Enter", "Space", "KeyM", "Zije", "PustVse"};
  for (int t = 0; t < 2; t++)
    v.emplace_back([&, t] {   // HTTP vlakna prohlizece (/klavesa, /klavesy)
      std::mt19937 r(t + 1);
      while (bez) { h_klavesa(K[r() % 11], (int)(r() & 1)); std::this_thread::sleep_for(std::chrono::milliseconds(3)); }
    });
  v.emplace_back([&] { while (bez) { h_denik(); std::this_thread::sleep_for(std::chrono::milliseconds(10)); } });   // stranka: ULOZIT LOG
  v.emplace_back([&] {   // tlacitko SAVE / LOAD
    std::string p = std::string(prac) + "/pozice.state";
    int i = 0;
    while (bez) { h_pozice(p.c_str(), (i++ & 1) == 0); std::this_thread::sleep_for(std::chrono::milliseconds(40)); }
  });
  v.emplace_back([&] {   // dotyk + zmeny obrazu + hlasky jadra
    std::mt19937 r(7);
    while (bez) {
      h_dotyk((int)(r() % 16), (int)(r() & 1));
      h_obraz((r() & 1) ? 320 : 640, (r() & 1) ? 240 : 480, (int)(r() % 5 == 0), 0, 1);
      h_tenke(100);
      if (r() % 20 == 0) h_hlasky(3);
      std::this_thread::sleep_for(std::chrono::milliseconds(7));
    }
  });
  std::this_thread::sleep_for(std::chrono::seconds(7));
  bez = false;
  for (auto &t : v) t.join();
  printf("stop: %s, krokem emulace %lld, soubeznych ulozeni %d, ulozeni %d\n", h_stop(), h_behu(), h_soubeznych(), h_ulozeni());
  printf("bios: %s\n", h_start(prac, ""));
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  printf("stop: %s\n", h_stop());
  const std::string d = h_denik();
  printf("denik: %zu bajtu, ZDRAVI %s\n", d.size(), d.find("ZDRAVI") != std::string::npos ? "ano" : "NE");
  return h_soubeznych() == 0 ? 0 : 1;
}
