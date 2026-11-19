// test_b351_ppmd.cpp - B351: RAR 4 s kompresi textu PPMd ("nejlepsi" ve starem WinRARu) - cisty C++
// (nap_ppmd.h = model PPMd var.H + rozsahovy dekoder RAR, nap_archiv.h = bloky PPMd v RAR 2.9)
//  A) archivy z vyrob_ppmd.cpp: text 200 kB (model 1 MB se cestou restartuje), unikove prikazy (esc 1 / 4 / 5),
//     konec bloku PPMd + novy blok bez / s novym modelem (rad 64), pevny archiv 3 soubory jednim modelem
//     (i na preskacku); filtry v PPMd (esc 3); bsdtar rozbali nepevne na puvodni data (../b349/over_bsdtar.sh)
//  B) Atari: pevny RAR "nejlepsi" - readme (PPMd) pred hrou: readme se nenabizi, hra se rozbali pres nej
//  C) odolnost: poskozene bloky PPMd (prehozene bity v datech modelu, zkraceni) - zadny pad ani zaseknuti
//     (dlouhy fuzz primo dekoderu: fuzz_b351.cpp)
//  D) volitelne "--cizi SLOZKA ...": skutecne RAR s PPMd (libarchive compress_best, rarfile unicode.rar ...)
// Spusteni (z test_overeni/b351):
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/atari -o vyrob_ppmd vyrob_ppmd.cpp && ./vyrob_ppmd /tmp/b351_archivy
//   g++ -std=c++17 -O2 -Wall -Wextra -I../../app/src/main/cpp/atari -o test_b351_ppmd test_b351_ppmd.cpp
//   ./test_b351_ppmd /tmp/b351_archivy [--cizi SLOZKA_libarchive SLOZKA_rarfile]   -> VYSTUP_test_b351_ppmd.txt
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <random>
#include <string>
#include <vector>
#include "nap_atari_archiv.h"

static int chyb = 0, kontrol = 0;
static void over(bool ok, const std::string &co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); fflush(stdout); }
static std::vector<uint8_t> nacti(const std::string &c) {
  std::vector<uint8_t> d; FILE *f = fopen(c.c_str(), "rb"); if (!f) return d;
  uint8_t b[65536]; size_t n; while ((n = fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n); fclose(f); return d;
}
static bool existuje(const std::string &c) { FILE *f = fopen(c.c_str(), "rb"); if (f) fclose(f); return f != nullptr; }
static double ms(std::chrono::steady_clock::time_point t0) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }
static std::string D;

struct Vysledek {
  bool otevreno = false; std::string chyba;
  std::vector<std::pair<std::string, std::string>> polozky;
  int ok() const { int k = 0; for (auto &p : polozky) if (p.second == "OK") k++; return k; }
};
static Vysledek rozbalVse(const std::string &jm, const std::vector<int> &poradi = {}) {
  Vysledek v;
  const std::vector<uint8_t> d = nacti(D + "/" + jm);
  nap::archiv::Archiv a;
  v.otevreno = a.otevri(d.data(), d.size());
  if (!v.otevreno) { v.chyba = a.chyba; return v; }
  std::vector<int> idx = poradi;
  if (idx.empty()) for (size_t i = 0; i < a.polozky.size(); i++) idx.push_back((int)i);
  for (int i : idx) {
    const nap::archiv::Polozka &p = a.polozky[(size_t)i];
    if (p.adresar) continue;
    std::vector<uint8_t> o;
    std::string r = a.rozbal((size_t)i, o) ? "OK" : a.chyba;
    if (r == "OK") { const std::string ref = D + "/obsah/" + jm + "/" + p.jmeno; if (existuje(ref) && nacti(ref) != o) r = "JINY OBSAH nez puvodni data"; }
    v.polozky.push_back({p.jmeno, r});
  }
  return v;
}
static std::string popis(const Vysledek &v) {
  if (!v.otevreno) return "NEOTEVRENO: " + v.chyba;
  std::string s; for (auto &p : v.polozky) s += (s.empty() ? "" : ", ") + p.first + " " + p.second; return s;
}

int main(int argc, char **argv) {
  if (argc < 2) { printf("pouziti: %s SLOZKA_Z_vyrob_ppmd [--cizi SLOZKA ...]\n", argv[0]); return 2; }
  D = argv[1];
  std::vector<std::string> cizi;
  for (int i = 2; i < argc; i++) if (std::string(argv[i]) != "--cizi") cizi.push_back(argv[i]);
  printf("B351 - RAR 4 s kompresi textu PPMd: model PPMd var.H + rozsahovy dekoder RAR v C++ (nap_ppmd.h)\n");
  if (!existuje(D + "/ppmd_text.rar")) { printf("CHYBA: v %s nejsou archivy - nejdriv ./vyrob_ppmd %s\n", D.c_str(), D.c_str()); return 2; }

  printf("\nA) ROZBALENI BLOKU PPMd (obsah se porovna s puvodnimi daty z vyrob_ppmd)\n");
  auto t0 = std::chrono::steady_clock::now();
  Vysledek v = rozbalVse("ppmd_text.rar");
  printf("     %s  (%.0f ms)\n", popis(v).c_str(), ms(t0));
  over(v.otevreno && v.ok() == 1, "200 kB textu v PPMd (rad 6, model 1 MB - cestou se zaplni a restartuje)");
  v = rozbalVse("ppmd_prikazy.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 1, "unikove prikazy: znak = unikovy znak (esc 1), shoda LZ (esc 4), RLE (esc 5); konec bloku + novy blok bez noveho modelu (jiny unikovy znak) a s novym modelem (rad 64, 4 MB)");
  v = rozbalVse("ppmd_pevny.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 3, "pevny RAR: 3 soubory jednim modelem PPMd (konec souboru = esc 2, dalsi soubor: hlavicka bez noveho modelu)");
  v = rozbalVse("ppmd_pevny.rar", {2, 0, 1, 2});
  printf("     na preskacku 3,1,2,3: %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 4, "pevny RAR s PPMd na preskacku (model se postavi znovu od 1. souboru)");
  v = rozbalVse("ppmd_filtr.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.polozky.size() == 2 && v.polozky[0].second == "OK" && v.polozky[1].second.find("neznamy filtr") != std::string::npos,
       "filtry v PPMd (esc 3, kod 6 / 16 / 5 B): blok za koncem souboru se nepouzije (CRC sedi), neznamy filtr uvnitr souboru appka odmitne");

  printf("\nB) ATARI: PEVNY RAR \"NEJLEPSI\" - README (PPMd) PRED HROU\n");
  {
    const std::vector<uint8_t> d = nacti(D + "/vyber_ppmd.rar");
    nap::AtariArchiv a;
    const bool ok = a.otevri(d.data(), d.size(), "vyber_ppmd.rar");
    printf("     vyber_ppmd.rar (%s): %s%s | preskoceno: %s\n", a.typ.c_str(), ok ? a.soubory[0].jmeno.c_str() : ("NIC: " + a.chyba).c_str(),
           ok ? (" [" + std::string(1, a.soubory[0].druh) + " " + std::to_string(a.soubory[0].data.size()) + " B]").c_str() : "",
           a.preskoceno.empty() ? "-" : a.preskoceno[0].c_str());
    over(ok && a.soubory.size() == 1 && a.soubory[0].jmeno == "hra.xex" && a.soubory[0].druh == 'X' && a.soubory[0].data == nacti(D + "/obsah/vyber_ppmd.rar/hra.xex"),
         "hra.xex se rozbali pres readme v PPMd (pevny archiv), readme se nenabizi");
  }

  printf("\nC) ODOLNOST: poskozene bloky PPMd nesmi appku shodit ani zaseknout\n");
  {
    const char *vzory[] = {"ppmd_text.rar", "ppmd_prikazy.rar", "ppmd_pevny.rar", "vyber_ppmd.rar", "ppmd_filtr.rar"};
    std::mt19937 rng(351);
    int pokusu = 0, chybnych = 0;
    double nejdele = 0;
    t0 = std::chrono::steady_clock::now();
    for (const char *jm : vzory) {
      const std::vector<uint8_t> puv = nacti(D + "/" + jm);
      for (int k = 0; k < 250; k++) {
        std::vector<uint8_t> d = puv;
        const size_t z = 100, kon = d.size() > 120 ? d.size() - 20 : d.size();   // data za prvni hlavickou
        if (k % 3 == 0) { for (int j = 0; j < 1 + (int)(rng() % 3); j++) d[z + rng() % (kon - z)] ^= (uint8_t)(1 << (rng() % 8)); }
        else if (k % 3 == 1) { d[z + rng() % (kon - z)] = (uint8_t)rng(); }
        else d.resize(z + rng() % (kon - z));
        auto t1 = std::chrono::steady_clock::now();
        nap::archiv::Archiv a;
        if (a.otevri(d.data(), d.size()))
          for (size_t i = 0; i < a.polozky.size(); i++) { std::vector<uint8_t> o; if (!a.rozbal(i, o)) chybnych++; }
        nejdele = std::max(nejdele, ms(t1));
        pokusu++;
      }
    }
    printf("     %d poskozenych archivu s PPMd za %.0f ms (nejdelsi %.1f ms), chyb rozbaleni %d (CRC / poskozena data PPMd)\n", pokusu, ms(t0), nejdele, chybnych);
    over(pokusu == 5 * 250 && nejdele < 3000, "zadny pad ani zaseknuti (s -fsanitize=address,undefined i bez chyb pameti)");
  }

  printf("\nD) SKUTECNE RAR S PPMd (volitelne: libarchive, rarfile - jen lokalne)\n");
  if (cizi.empty()) printf("     (zadne - \"--cizi SLOZKA\")\n");
  else {
    int ppmdSoubory = 0, ppmdOk = 0;
    for (const std::string &s : cizi) {
      DIR *dir = opendir(s.c_str());
      if (!dir) continue;
      std::vector<std::string> v2;
      while (dirent *e = readdir(dir)) { const std::string n = e->d_name; if (n.size() > 4 && n.rfind(".rar") == n.size() - 4) v2.push_back(n); }
      closedir(dir);
      std::sort(v2.begin(), v2.end());
      for (const std::string &n : v2) {
        const std::vector<uint8_t> d = nacti(s + "/" + n);
        if (d.size() < 7 || std::memcmp(d.data(), "Rar!\x1A\x07\x00", 7) != 0) continue;   // jen RAR 4
        nap::archiv::Archiv a;
        if (!a.otevri(d.data(), d.size())) continue;
        std::string r;
        int k = 0, kOk = 0;
        for (size_t i = 0; i < a.polozky.size(); i++) {
          const nap::archiv::Polozka &p = a.polozky[i];
          if (p.adresar || p.metoda != 0x35 || p.heslo || p.dily) continue;   // jen "nejlepsi" (tam byva PPMd)
          if (p.velikost > (64u << 20)) { r += " " + p.jmeno + ": " + std::to_string(p.velikost >> 20) + " MB nad limit appky 64 MB"; continue; }
          std::vector<uint8_t> o;
          k++;
          const bool ok = a.rozbal(i, o);
          if (ok) kOk++; else r += " " + p.jmeno + ": " + a.chyba;
        }
        if (k == 0 && r.empty()) continue;
        ppmdSoubory += k; ppmdOk += kOk;
        for (char &c : r) if ((unsigned char)c < 0x20 || (unsigned char)c >= 0x7F) c = '?';
        printf("     %-50s metoda 0x35: %d/%d OK%s\n", n.c_str(), kOk, k, r.c_str());
      }
    }
    over(ppmdSoubory > 0 && ppmdOk == ppmdSoubory, "skutecne RAR \"nejlepsi\" (PPMd i LZ, pevny unicode.rar): vsechny soubory rozbalene, CRC sedi");
  }
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
