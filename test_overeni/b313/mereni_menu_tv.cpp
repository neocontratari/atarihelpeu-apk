// test_overeni/b313/mereni_menu_tv.cpp
// ====================================================================
//  B313: SKUTECNE HLAVNI MENU (nap_menu_gl.cpp pres Mesa) NA TV - mereni a obrazky
//
//  Menu jako na Reneho S8 (plocha 1080x2004, 60 Hz, kvalita TV VYSOKA = 1280x720),
//  prohlizec na TV = prohlizec_v3.h (pakety rozebere jako stranka v appce, JPEG
//  nakresli na misto). Prubeh: klid 4 s, listovani (3x sipka vpravo po 0,5 s), klid 5 s.
//  Meri: kB na snimek pri listovani / v klidu, casti na snimek, zjemneni; ostrost
//  obrazu v prohlizeci proti snimku menu (PSNR horniho a spodniho pasu - napisy).
//  Obrazky (PPM) do PRAC: tv_klid1.ppm, tv_listovani.ppm, tv_klid2.ppm, zdroj_klid2.ppm.
//
//   g++ -std=c++17 -O2 -pthread -w -I../b304/stub -I../b291/stub -I$INC -I$JAVA_HOME/include
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/tvproud -o mereni_menu_tv mereni_menu_tv.cpp
//       -l:libEGL.so.1 -l:libGLESv2.so.2
//   EGL_PLATFORM=surfaceless ./mereni_menu_tv PRAC [kvalita 0-2] [plochaW plochaH]
// ====================================================================
#include "../b311/tv_menu.cpp"
#include "prohlizec_v3.h"

#include <mutex>

static void menu_nastav(float hz, int k) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastav(&g_env, nullptr, hz, k); }

static void ulozPpm(const std::string &cesta, const std::vector<uint32_t> &px, int W, int H) {
  FILE *f = std::fopen(cesta.c_str(), "wb");
  if (!f) return;
  std::fprintf(f, "P6\n%d %d\n255\n", W, H);
  std::vector<unsigned char> r((size_t)W * 3);
  for (int y = 0; y < H; y++) {
    for (int x = 0; x < W; x++) {
      const uint32_t c = px[(size_t)y * W + x];
      r[(size_t)x * 3] = (unsigned char)(c >> 16); r[(size_t)x * 3 + 1] = (unsigned char)(c >> 8); r[(size_t)x * 3 + 2] = (unsigned char)c;
    }
    std::fwrite(r.data(), 1, r.size(), f);
  }
  std::fclose(f);
}

struct Faze {
  const char *jmeno;
  long long bajtu = 0, snimku = 0, casti = 0, max = 0;
  double t0 = 0, t1 = 0;
};

int main(int argc, char **argv) {
  const std::string prac = argc > 1 ? argv[1] : ".";
  const int kvalita = argc > 2 ? std::atoi(argv[2]) : 2;
  pripravEnv();
  g_okno.w = argc > 4 ? std::atoi(argv[3]) : 1080;
  g_okno.h = argc > 4 ? std::atoi(argv[4]) : 2004;
  std::printf("B313 - mereni: skutecne menu (Mesa) na TV, plocha %dx%d, kvalita TV %d\n", g_okno.w, g_okno.h, kvalita);
  const int n = menu_assety(nullptr);
  menu_nastav(60.f, kvalita);
  menu_prah(8.f);
  menu_ukaz(1);
  menu_plocha(true);
  for (int i = 0; i < 600 && !menu_pripraveno(); i++) std::this_thread::sleep_for(std::chrono::milliseconds(50));
  std::printf("karet %d, pripraveno %s\n", n, menu_pripraveno() ? "ano" : "NE");
  if (!menu_pripraveno()) return 2;

  const int id = proud_otevri();
  Prohlizec pr;
  std::mutex pm;
  std::atomic<bool> konec{false};
  std::atomic<int> faze{0};
  Faze fz[3] = {{"klid 1 (4 s)"}, {"listovani (3x sipka)"}, {"klid 2 (5 s)"}};
  const auto start = std::chrono::steady_clock::now();
  auto ted = [&] { return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count(); };
  std::thread prijem([&] {
    std::vector<uint8_t> b(8u << 20);
    while (!konec.load()) {
      std::vector<uint8_t> pk(b.size());
      const int r = proud_dalsi(id, pk, 250);
      if (r <= 0) continue;
      pk.resize((size_t)r);
      std::lock_guard<std::mutex> l(pm);
      pr.prijmi(pk);
      Faze &f = fz[faze.load()];
      f.bajtu += r; f.snimku++; f.casti += pr.castiPoslednihoSnimku; f.max = std::max<long long>(f.max, r);
      if (std::getenv("VYPIS") && (faze.load() != 1)) {
        std::printf("   [%.2f s faze %d] %d B:", ted(), faze.load(), r);
        for (const auto &c : pr.castiSnimku) std::printf(" (%d,%d %dx%d q%d %zu B)", c.x, c.y, c.w, c.h, c.kv, c.bajtu);
        std::printf("\n");
      }
    }
  });
  auto grab = [&](std::vector<uint32_t> &zdroj, int &W, int &H) {
    std::vector<jint> px((size_t)1280 * 720);
    int r = 0;
    for (int i = 0; i < 100 && r <= 0; i++) {
      r = menu_grab(px);
      if (r < 0) px.assign((size_t)((-r) >> 16) * ((-r) & 0xFFFF), 0);
      if (r <= 0) std::this_thread::sleep_for(std::chrono::milliseconds(30));
    }
    W = r >> 16; H = r & 0xFFFF;
    zdroj.assign(px.begin(), px.begin() + (size_t)W * H);
    for (auto &c : zdroj) c &= 0xFFFFFFu;
  };
  auto ostrost = [&](const char *co, const std::string &soubor) {
    std::vector<uint32_t> zdroj;
    int W = 0, H = 0;
    grab(zdroj, W, H);
    std::lock_guard<std::mutex> l(pm);
    if (W != pr.W || H != pr.H) { std::printf("   %s: rozmer zdroje %dx%d, prohlizec %dx%d\n", co, W, H, pr.W, pr.H); return; }
    const double horni = psnr(zdroj, pr.canvas, W, 0, 0, W, H / 5);
    const double spodni = psnr(zdroj, pr.canvas, W, 0, H * 3 / 4, W, H);
    const double cely = psnr(zdroj, pr.canvas, W, 0, 0, W, H);
    std::printf("   %s: prohlizec %dx%d, ostrost proti snimku menu: horni pas %.1f dB, spodni pas (popis, tlacitko) %.1f dB, cely %.1f dB\n", co, W, H, horni, spodni, cely);
    ulozPpm(prac + "/tv_" + soubor + ".ppm", pr.canvas, W, H);
    ulozPpm(prac + "/zdroj_" + soubor + ".ppm", zdroj, W, H);
  };

  fz[0].t0 = ted();
  std::this_thread::sleep_for(std::chrono::seconds(4));
  fz[0].t1 = ted();
  ostrost("po klidu 1", "klid1");
  faze.store(1);
  fz[1].t0 = ted();
  for (int i = 0; i < 3; i++) {
    menu_klavesa(napmenu::KL_VPRAVO);
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
  }
  fz[1].t1 = ted();
  {
    std::lock_guard<std::mutex> l(pm);
    ulozPpm(prac + "/tv_listovani.ppm", pr.canvas, pr.W, pr.H);
  }
  faze.store(2);
  fz[2].t0 = ted();
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  ostrost("1,5 s po listovani", "po15s");
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));
  fz[2].t1 = ted();
  ostrost("po klidu 2 (5 s)", "klid2");
  konec.store(true);
  prijem.join();
  proud_zavri(id);
  for (const Faze &f : fz) {
    const double s = std::max(0.001, f.t1 - f.t0);
    std::printf("   %-22s %5.1f snimku/s, %6.1f kB na snimek (nejvic %6.1f kB), %.2f casti na snimek, %.2f Mbit/s\n", f.jmeno, f.snimku / s,
                f.snimku ? f.bajtu / 1024.0 / f.snimku : 0.0, f.max / 1024.0, f.snimku ? (double)f.casti / f.snimku : 0.0, f.bajtu * 8.0 / 1e6 / s);
  }
  std::string l = proud_log();
  for (size_t p; (p = l.find('\n')) != std::string::npos; l.erase(0, p + 1)) std::printf("   log: %s\n", l.substr(0, p).c_str());
  std::printf("   menu: %s\n", menu_info().c_str());
  std::printf("   chyb v paketech %lld, paketu %lld, snimku %lld\n", pr.chyb, pr.paketu, pr.snimku);
  menu_plocha(false);
  return pr.chyb ? 1 : 0;
}
