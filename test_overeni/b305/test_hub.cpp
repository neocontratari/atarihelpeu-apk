// B305: test SKUTECNEHO rozbocovace prenosu (app/src/main/cpp/tvproud/nap_tv_proud.h)
// s vlakny jako v appce:
//   - vlakno "jadra": snimek 60x za vterinu (Sega: synteticka hra, PS1: snimky
//     3D sceny), behem behu zmena rozmeru (Sega 320 -> 256 -> 320),
//   - vlakno "zvuku": co se prehrava (48000 / 44100 Hz stereo, po 10 ms),
//   - dva prohlizece (kazdy vlastni vlakno jako Java /sega.stream): rychly a
//     pomaly (obcas ceka 100-400 ms jako spatna Wi-Fi).
// Kazdy paket se rozbali jako v prohlizeci: obraz se porovna se snimkem, ktery
// jadro opravdu poslalo (cislo snimku je zakodovane v obraze), zvuk musi
// navazovat vzorek po vzorku (krome zamerneho zahozeni u pomaleho).
//
//   g++ -std=c++17 -O2 -pthread -I../../app/src/main/cpp/tvproud -I../../app/src/main/cpp/vendor/stb -o test_hub test_hub.cpp
//   ./test_hub ADRESAR_320       (PNG snimky z ../b304/demo_snimky)
//   (to same s -O1 -g -fsanitize=thread = ThreadSanitizer)
#include <algorithm>
#include <atomic>
#include <functional>
#include <random>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "nap_tv_proud.h"
#include "nap_tv_proud_index.h"
#include "nap_tv_proud_jpeg.h"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "stb_image.h"
#include "sega_hra.h"

using namespace nap::proud;

static std::mutex g_tisk;
static int g_kontrol = 0, g_chyb = 0;
static const bool g_tsan = getenv("NAP_TSAN") != nullptr;   // pod ThreadSanitizerem je vse ~10x pomalejsi - bez kontrol rychlosti
static void over(bool ok, const char *fmt, ...) {
  char b[700];
  va_list a; va_start(a, fmt); vsnprintf(b, sizeof(b), fmt, a); va_end(a);
  std::lock_guard<std::mutex> l(g_tisk);
  g_kontrol++;
  if (!ok) g_chyb++;
  printf("%s %s\n", ok ? "OK    " : "CHYBA ", b);
  fflush(stdout);
}

// ---- paket jako v prohlizeci ----
struct Paket {
  unsigned verze = 0, pr = 0, W = 0, H = 0, ox = 0, oy = 0, ow = 0, oh = 0, px = 0, py = 0;
  uint32_t seq = 0, vzork = 0;
  const uint8_t *obraz = nullptr;
  size_t obrazMax = 0;
  std::vector<int16_t> zvuk;          // stereo
  unsigned kanalu = 0;
};
static unsigned u16(const uint8_t *p) { return p[0] | (p[1] << 8); }
static uint32_t u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
// hlavicka; obraz se rozbali zvenku (vrati delku obrazove casti), pak zvuk
template <class FnObraz>
static bool rozbal(const std::vector<uint8_t> &pk, Paket &P, FnObraz obraz) {
  if (pk.size() < 4 + 28 + 3) return false;
  const uint32_t delka = u32(pk.data());
  if (delka != pk.size() - 4) return false;
  const uint8_t *p = pk.data() + 4;
  const size_t n = delka;
  if (p[0] != 'N' || p[1] != 'A') return false;
  P.verze = p[2]; P.pr = p[3]; P.W = u16(p + 4); P.H = u16(p + 6); P.seq = u32(p + 8);
  P.ox = u16(p + 12); P.oy = u16(p + 14); P.ow = u16(p + 16); P.oh = u16(p + 18); P.px = p[20]; P.py = p[21];
  P.vzork = u32(p + 24);
  P.obraz = p + 28; P.obrazMax = n - 28;
  const size_t ob = obraz(P);
  if (!ob) return false;
  size_t o = 28 + ob;
  if (o + 3 > n) return false;
  const unsigned vz = u16(p + o); P.kanalu = p[o + 2]; o += 3;
  if (P.kanalu != 1 && P.kanalu != 2) return false;
  if (o + (size_t)vz * 2 * P.kanalu != n) return false;
  P.zvuk.resize((size_t)vz * 2);
  for (size_t i = 0; i < vz; i++) {
    const int16_t l = (int16_t)u16(p + o + i * 2 * P.kanalu);
    P.zvuk[2 * i] = l;
    P.zvuk[2 * i + 1] = P.kanalu == 2 ? (int16_t)u16(p + o + i * 4 + 2) : l;
  }
  return true;
}

// ---- zvuk: vzorky jsou pocitadlo (L = n mod 32768, R = L xor 0x2AAA) ----
struct ZvukZdroj {
  std::atomic<bool> bezi{true};
  void vlakno(std::function<void(const int16_t *, size_t)> do_hubu, int vzork) {
    const int ramcu = vzork / 100;               // 10 ms
    std::vector<int16_t> b((size_t)ramcu * 2);
    unsigned long long n = 0;
    auto t = std::chrono::steady_clock::now();
    while (bezi.load()) {
      for (int i = 0; i < ramcu; i++, n++) { b[2 * i] = (int16_t)(n & 0x7FFF); b[2 * i + 1] = (int16_t)((n & 0x7FFF) ^ 0x2AAA); }
      do_hubu(b.data(), b.size());
      t += std::chrono::milliseconds(10);
      std::this_thread::sleep_until(t);
    }
  }
};
struct ZvukKontrola {
  long long vzorku = 0, navazuje = 0, skoku = 0, chybnych = 0, paketuSeZvukem = 0;
  int posl = -1;
  void pridej(const std::vector<int16_t> &z) {
    if (z.empty()) return;
    paketuSeZvukem++;
    for (size_t i = 0; i < z.size(); i += 2) {
      const int l = z[i], r = z[i + 1];
      if ((r ^ 0x2AAA) != l) { chybnych++; continue; }
      if (posl >= 0) { if (((posl + 1) & 0x7FFF) == l) navazuje++; else skoku++; }
      posl = l; vzorku++;
    }
  }
};

// =====================================================================
//  SEGA
// =====================================================================
struct SegaZdroj {
  std::mutex m;
  std::map<uint32_t, std::vector<uint32_t>> snimky;   // cislo -> snimek (poslednich ~300)
  std::map<uint32_t, int> sirka;
  std::atomic<bool> bezi{true};
  std::atomic<uint32_t> poslano{0};
};

static void testSega() {
  printf("\n=== SEGA: rozbocovac + kodek indexy, vlakna jako v appce ===\n");
  Hub<KodekIndex> hub("SEGA", 48000);
  SegaZdroj zdroj;
  ZvukZdroj zvuk;
  std::thread tj([&] {
    SegaHra hra;
    std::vector<uint32_t> F;
    auto t = std::chrono::steady_clock::now();
    for (uint32_t n = 1; zdroj.bezi.load(); n++) {
      if (n == 150) hra.W = 256;                       // hra prepne na 256 bodu
      if (n == 250) hra.W = 320;
      hra.krok(); hra.kresli(F);
      F[0] = 0xFF000000u | (n & 0xFFFFFF);             // cislo snimku v prvnim bodu
      {
        std::lock_guard<std::mutex> l(zdroj.m);
        zdroj.snimky[n] = F; zdroj.sirka[n] = hra.W;
        if (zdroj.snimky.size() > 300) { zdroj.sirka.erase(zdroj.snimky.begin()->first); zdroj.snimky.erase(zdroj.snimky.begin()); }
      }
      hub.snimek(F.data(), hra.W, hra.H);
      zdroj.poslano = n;
      t += std::chrono::microseconds(16667);
      std::this_thread::sleep_until(t);
    }
  });
  std::thread tz([&] { zvuk.vlakno([&](const int16_t *s, size_t n) { hub.zvuk(s, n); }, 48000); });
  struct Vysl { long long paketu = 0, presnych = 0, chybnych = 0, starych = 0, klicovych = 0, bezeZmeny = 0, chybPaketu = 0, rozmeru = 0, odlozenych = 0; ZvukKontrola zv; uint32_t poslSeq = 0; long long seqPreskoku = 0; double cekalMs = 0; };
  auto prohlizec = [&](bool pomaly, Vysl &v) {
    const int id = hub.otevri();
    DekoderIndex d;
    std::vector<uint8_t> pk;
    std::mt19937 rnd(pomaly ? 5 : 7);
    const auto konec = std::chrono::steady_clock::now() + std::chrono::seconds(6);
    unsigned predW = 0;
    while (std::chrono::steady_clock::now() < konec) {
      const int r = hub.dalsi(id, 250, 0, pk);
      if (r < 0) { v.chybPaketu++; break; }
      if (r == 0) continue;
      v.paketu++;
      if (!pomaly && v.paketu == 100 && !v.odlozenych) { v.odlozenych++; v.paketu--; hub.odloz(id, pk); continue; }   // paket se do Javy nevesel (pole male) - prijde znovu
      Paket P;
      const bool ok = rozbal(pk, P, [&](Paket &Q) { return d.obraz(Q.obraz, Q.obrazMax, (int)Q.W, (int)Q.H, (Q.pr & 1) != 0); });
      if (!ok || P.verze != 2 || P.vzork != 48000 || P.px != 4 || P.py != 3) { v.chybPaketu++; continue; }
      if (P.pr & 1) v.klicovych++;
      if (P.pr & 4) v.bezeZmeny++;
      if (predW && P.W != predW) v.rozmeru++;
      predW = P.W;
      if (v.poslSeq && P.seq != v.poslSeq + 1) v.seqPreskoku += P.seq - v.poslSeq - 1;
      v.poslSeq = P.seq;
      v.zv.pridej(P.zvuk);
      // obraz = presne snimek, ktery jadro poslalo?
      const uint32_t n = d.barva(0, 0) & 0xFFFFFF;
      std::vector<uint32_t> F;
      {
        std::lock_guard<std::mutex> l(zdroj.m);
        auto it = zdroj.snimky.find(n);
        if (it == zdroj.snimky.end()) { v.starych++; continue; }
        F = it->second;
      }
      bool stejny = F.size() == (size_t)P.W * P.H;
      for (unsigned y = 0; stejny && y < P.H; y++)
        for (unsigned x = 0; x < P.W; x++) if (d.barva((int)x, (int)y) != (F[(size_t)y * P.W + x] & 0xFFFFFF)) { stejny = false; break; }
      if (stejny) v.presnych++; else v.chybnych++;
      if (pomaly && rnd() % 6 == 0) {
        const int ms = (rnd() % 10 == 0) ? 400 : 100 + (int)(rnd() % 120);
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
        v.cekalMs += ms;
      }
    }
    hub.zavri(id);
  };
  Vysl A, B;
  std::thread pa([&] { prohlizec(false, A); });
  std::thread pb([&] { prohlizec(true, B); });
  pa.join(); pb.join();
  zdroj.bezi = false; zvuk.bezi = false;
  tj.join(); tz.join();
  const std::string log = hub.vezmiLog();
  for (int k = 0; k < 2; k++) {
    Vysl &v = k ? B : A;
    printf("  prohlizec %s: %lld paketu (%.1f/s), presnych snimku %lld, chybnych %lld, starych %lld, klicovych %lld, beze zmeny %lld, preskoceno snimku %lld, zmen rozmeru %lld\n",
           k ? "POMALY" : "rychly", v.paketu, v.paketu / 6.0, v.presnych, v.chybnych, v.starych, v.klicovych, v.bezeZmeny, v.seqPreskoku, v.rozmeru);
    printf("     zvuk: %lld vzorku (%.0f/s), navazuje %lld, skoku %lld, chybnych %lld\n", v.zv.vzorku, v.zv.vzorku / 6.0, v.zv.navazuje, v.zv.skoku, v.zv.chybnych);
  }
  over(A.chybPaketu == 0 && B.chybPaketu == 0, "Sega: vsechny pakety se rozbali (hlavicka, obraz, zvuk do posledniho bajtu, verze 2, 48000 Hz, 4:3)");
  over(A.chybnych == 0 && B.chybnych == 0 && A.presnych > (g_tsan ? 50 : 250) && B.presnych > (g_tsan ? 10 : 20),
       "Sega: obraz v obou prohlizecich je vzdy PRESNE snimek jadra (rychly %lld, pomaly %lld snimku)", A.presnych, B.presnych);
  if (!g_tsan) over(A.paketu >= 6 * 55, "Sega: rychly prohlizec dostava kazdy snimek (%.1f/s z 60)", A.paketu / 6.0);
  over(B.seqPreskoku > 0 && B.chybnych == 0, "Sega: pomaly prohlizec dostava vzdy POSLEDNI snimek (preskoceno %lld, zadna fronta)", B.seqPreskoku);
  over(A.klicovych == 3 && A.rozmeru == 2, "Sega: klicovy snimek jen na zacatku a pri zmene rozmeru 320 -> 256 -> 320 (%lld klicove, %lld zmeny)", A.klicovych, A.rozmeru);
  over(A.odlozenych == 1 && A.chybnych == 0, "Sega: paket, ktery se nevesel do pole Javy, prisel znovu pri dalsim volani (zadny klicovy snimek, zadny ztraceny zvuk)");
  over(A.zv.chybnych == 0 && A.zv.skoku == 0 && A.zv.vzorku > 48000 * 5, "Sega: zvuk rychleho prohlizece navazuje vzorek po vzorku (%lld vzorku, 0 skoku)", A.zv.vzorku);
  over(B.zv.chybnych == 0 && B.zv.skoku >= 1 && B.zv.skoku <= 12, "Sega: pomaly prohlizec - kdyz zaostane o vic nez 0,25 s, stary zvuk se zahodi (skoku %lld)", B.zv.skoku);
  printf("  log:\n%s", log.c_str());
  over(log.find("prohlizec 1 pripojen") != std::string::npos && log.find("prohlizec 2 odpojen") != std::string::npos && log.find("klicovych") != std::string::npos,
       "Sega: log appky - pripojen / odpojen se statistikou");
  over(hub.klientu() == 0 && hub.bezi(1000), "Sega: po odpojeni zadny prohlizec, jadro hlasi beh");
}

// =====================================================================
//  PS1
// =====================================================================
static void vlozCislo(std::vector<uint32_t> &F, int W, uint32_t n) {   // 16 bitu jako ctverecky 4x4 (preziji JPEG)
  for (int b = 0; b < 16; b++)
    for (int y = 0; y < 4; y++)
      for (int x = 0; x < 4; x++) F[(size_t)((b / 4) * 4 + y) * W + (b % 4) * 4 + x] = ((n >> b) & 1) ? 0xFFFFFFFFu : 0xFF000000u;
}
static uint32_t prectiCislo(const std::vector<uint32_t> &P, int W) {
  uint32_t n = 0;
  for (int b = 0; b < 16; b++) {
    const uint32_t c = P[(size_t)((b / 4) * 4 + 1) * W + (b % 4) * 4 + 1];
    if (((c >> 8) & 255) > 128) n |= 1u << b;
  }
  return n;
}

static void testPs1(const char *adr) {
  printf("\n=== PS1: rozbocovac + JPEG po castech, vlakna jako v appce ===\n");
  std::vector<std::vector<uint32_t>> src;
  int SW = 0, SH = 0;
  {
    std::vector<std::string> v;
    if (DIR *d = opendir(adr)) { while (dirent *e = readdir(d)) { std::string s = e->d_name; if (s.size() > 4 && s.substr(s.size() - 4) == ".png") v.push_back(std::string(adr) + "/" + s); } closedir(d); }
    std::sort(v.begin(), v.end());
    for (auto &f : v) {
      int n; unsigned char *p = stbi_load(f.c_str(), &SW, &SH, &n, 3);
      if (!p) continue;
      std::vector<uint32_t> F((size_t)SW * SH);
      for (size_t i = 0; i < F.size(); i++) F[i] = 0xFF000000u | ((uint32_t)p[i * 3] << 16) | ((uint32_t)p[i * 3 + 1] << 8) | p[i * 3 + 2];
      stbi_image_free(p);
      src.push_back(F);
    }
  }
  if (src.empty()) { over(false, "PS1: chybi snimky v %s", adr); return; }
  Hub<KodekJpeg> hub("PS1", 44100);
  std::mutex zm;
  std::map<uint32_t, std::vector<uint32_t>> poslane;
  std::atomic<bool> bezi{true};
  ZvukZdroj zvuk;
  std::thread tj([&] {
    auto t = std::chrono::steady_clock::now();
    for (uint32_t n = 1; bezi.load(); n++) {
      std::vector<uint32_t> F = src[(n / 2) % src.size()];   // hra na 30 snimku/s, jadro posila 60/s (kazdy 2x)
      vlozCislo(F, SW, n / 2);
      {
        std::lock_guard<std::mutex> l(zm);
        poslane[n / 2] = F;
        if (poslane.size() > 200) poslane.erase(poslane.begin());
      }
      hub.snimek(F.data(), SW, SH);
      t += std::chrono::microseconds(16667);
      std::this_thread::sleep_until(t);
    }
  });
  std::thread tz([&] { zvuk.vlakno([&](const int16_t *s, size_t n) { hub.zvuk(s, n); }, 44100); });
  struct Vysl { long long paketu = 0, obrazu = 0, bezeZmeny = 0, chybPaketu = 0, nesedi = 0; double psMin = 99, psSum = 0; long long psN = 0; ZvukKontrola zv; int id = 0; };
  auto prohlizec = [&](bool pomaly, Vysl &v) {
    const int id = hub.otevri();
    v.id = id;
    std::vector<uint32_t> platno;
    std::vector<uint8_t> pk;
    const auto konec = std::chrono::steady_clock::now() + std::chrono::seconds(6);
    int zapis = 0;
    while (std::chrono::steady_clock::now() < konec) {
      const int r = hub.dalsi(id, 250, zapis, pk);
      if (r < 0) { v.chybPaketu++; break; }
      if (r == 0) continue;
      v.paketu++;
      Paket P;
      CastJpeg c;
      const bool ok = rozbal(pk, P, [&](Paket &Q) { return c.cti(Q.obraz, Q.obrazMax); });
      if (!ok || P.verze != 3 || P.vzork != 44100) { v.chybPaketu++; continue; }
      v.zv.pridej(P.zvuk);
      if (platno.size() != (size_t)P.W * P.H) platno.assign((size_t)P.W * P.H, 0);
      if (c.w == 0) { v.bezeZmeny++; continue; }
      int jw, jh, nn;
      unsigned char *px = stbi_load_from_memory(c.jpeg, (int)c.delka, &jw, &jh, &nn, 3);
      if (!px || jw != (int)c.w || jh != (int)c.h) { v.chybPaketu++; if (px) stbi_image_free(px); continue; }
      for (int y = 0; y < jh; y++)
        for (int x = 0; x < jw; x++) {
          const unsigned char *s = px + ((size_t)y * jw + x) * 3;
          platno[(size_t)(c.y + y) * P.W + c.x + x] = 0xFF000000u | ((uint32_t)s[0] << 16) | ((uint32_t)s[1] << 8) | s[2];
        }
      stbi_image_free(px);
      v.obrazu++;
      const uint32_t n = prectiCislo(platno, (int)P.W);
      std::vector<uint32_t> F;
      { std::lock_guard<std::mutex> l(zm); auto it = poslane.find(n); if (it != poslane.end()) F = it->second; }
      if (F.empty()) { v.nesedi++; continue; }
      double se = 0;
      for (size_t i = 0; i < F.size(); i++)
        for (int s2 = 0; s2 < 24; s2 += 8) { const double e = (double)((F[i] >> s2) & 255) - (double)((platno[i] >> s2) & 255); se += e * e; }
      const double ps = 10 * std::log10(255.0 * 255.0 / std::max(1e-9, se / (F.size() * 3.0)));
      v.psMin = std::min(v.psMin, ps); v.psSum += ps; v.psN++;
      if (pomaly) { zapis = 30; std::this_thread::sleep_for(std::chrono::milliseconds(60)); }   // pomala sit: zapis ceka
    }
    hub.zavri(id);
  };
  Vysl A, B;
  std::thread pa([&] { prohlizec(false, A); });
  std::thread pb([&] { prohlizec(true, B); });
  pa.join(); pb.join();
  bezi = false; zvuk.bezi = false;
  tj.join(); tz.join();
  const std::string log = hub.vezmiLog();
  for (int k = 0; k < 2; k++) {
    Vysl &v = k ? B : A;
    printf("  prohlizec %s: %lld paketu (%.1f/s), obrazu %lld (%.1f/s), beze zmeny %lld, PSNR prumer %.1f dB nejhorsi %.1f dB, nesedi %lld, chyb %lld\n",
           k ? "POMALY" : "rychly", v.paketu, v.paketu / 6.0, v.obrazu, v.obrazu / 6.0, v.bezeZmeny, v.psN ? v.psSum / v.psN : 0, v.psMin, v.nesedi, v.chybPaketu);
    printf("     zvuk: %lld vzorku (%.0f/s), navazuje %lld, skoku %lld, chybnych %lld\n", v.zv.vzorku, v.zv.vzorku / 6.0, v.zv.navazuje, v.zv.skoku, v.zv.chybnych);
  }
  over(A.chybPaketu == 0 && B.chybPaketu == 0, "PS1: vsechny pakety se rozbali (verze 3, 44100 Hz, JPEG se da otevrit)");
  if (!g_tsan) over(A.obrazu >= 6 * 27 && A.bezeZmeny >= 6 * 25, "PS1: hra na 30 snimku/s = 30 obrazu/s, druhy (stejny) snimek jen zvuk (%lld obrazu, %lld beze zmeny)", A.obrazu, A.bezeZmeny);
  over(A.psMin >= 30 && B.psMin >= 28 && A.nesedi == 0 && B.nesedi == 0, "PS1: obraz v prohlizeci odpovida snimku jadra (PSNR nejhorsi %.1f / %.1f dB)", A.psMin, B.psMin);
  over(A.zv.chybnych == 0 && A.zv.skoku == 0 && A.zv.vzorku > 44100 * 5, "PS1: zvuk navazuje vzorek po vzorku (%lld vzorku)", A.zv.vzorku);
  printf("  log:\n%s", log.c_str());
  over(log.find("JPEG kvalita") != std::string::npos && log.find("snizeno") != std::string::npos, "PS1: log appky s kvalitou JPEG");
  const std::string pomalyOdpojen = "prohlizec " + std::to_string(B.id) + " odpojen", rychlyOdpojen = "prohlizec " + std::to_string(A.id) + " odpojen";
  const size_t ib = log.find(pomalyOdpojen), ia = log.find(rychlyOdpojen);
  const size_t ibk = ib == std::string::npos ? ib : log.find('\n', ib), iak = ia == std::string::npos ? ia : log.find('\n', ia);
  over(ib != std::string::npos && ia != std::string::npos && log.substr(ib, ibk - ib).find("snizeno 0x") == std::string::npos &&
       log.substr(ia, iak - ia).find("kvalita 88 (max 88, snizeno 0x)") != std::string::npos,
       "PS1: pomaly prohlizec (zapis do site 30 ms) dostal nizsi kvalitu JPEG, rychly plnou (88)");
}

int main(int argc, char **argv) {
  printf("B305: rozbocovac prenosu Sega / PS1 (nap_tv_proud.h) - vlakna jako v appce\n");
  testSega();
  testPs1(argc > 1 ? argv[1] : "ps1_320");
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", g_kontrol, g_chyb);
  return g_chyb ? 1 : 0;
}
