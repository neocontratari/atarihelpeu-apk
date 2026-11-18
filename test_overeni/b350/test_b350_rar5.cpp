// test_b350_rar5.cpp - B350: hra pro Atari v RAR 5 (novy WinRAR) - cisty C++ (nap_archiv.h + nap_atari_archiv.h)
//
// B349 RAR5 jen poznal ("zatim neumim"). B350 ho rozbali: hlavicky s cisly promenne delky (vint) a CRC32,
// ulozeno i komprese RAR 5 (bloky, Huffman bez delty, filtry DELTA / E8 / E8E9 / ARM), pevny archiv.
//  A) rozbaleni archivu z vyrob_rar5.py (vlastni koder RAR 5 - bsdtar je rozbali na puvodni data):
//     ulozeny (+ slozka), vsechny kody a filtry (300 kB, 14 bloku, vzdalenost nad 256 kB, shoda 4097 B),
//     pevny (2. soubor bez tabulek, 3. prazdny bez dat, 4. s novymi tabulkami) i na preskacku,
//     soubor bez CRC32 (jako s otiskem BLAKE2) + odkaz (symlink) se preskoci
//  B) vyber pro Atari: Prince of Persia v RAR5 (DD, sada SDA / SDB), pevny RAR5, strany A / B
//  C) odolnost: poskozene RAR5 (prehozene bajty, zkraceni, vlozene bajty, nahodna data) - zadny pad
//  D) volitelne "--cizi SLOZKA ...": testovaci RAR5 libarchive a rarfile (jen lokalne, v repozitari nejsou)
// Spusteni (z test_overeni/b350):
//   python3 vyrob_rar5.py /tmp/b350_archivy
//   g++ -std=c++17 -O2 -Wall -Wextra -I../../app/src/main/cpp/atari -o test_b350_rar5 test_b350_rar5.cpp
//   ./test_b350_rar5 /tmp/b350_archivy [--cizi SLOZKA_libarchive SLOZKA_rarfile]   -> VYSTUP_test_b350_rar5.txt
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
  std::string co(const std::string &j) const { for (auto &p : polozky) if (p.first == j) return p.second; return "(chybi)"; }
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

static nap::AtariArchiv vyber(const std::string &cesta, bool &ok) {
  const std::vector<uint8_t> d = nacti(cesta);
  nap::AtariArchiv a;
  auto t0 = std::chrono::steady_clock::now();
  ok = a.otevri(d.data(), d.size(), cesta.substr(cesta.find_last_of('/') + 1));
  const double t = ms(t0);
  std::string s;
  for (size_t i = 0; i < a.soubory.size(); i++) {
    const nap::AtariSoubor &f = a.soubory[i];
    char b[300];
    snprintf(b, sizeof b, "%s%s[%c%s%s %zu B]", i ? ", " : "", f.jmeno.c_str(), f.druh, f.druh == 'A' ? (f.boot ? " boot" : " data") : "",
             f.sada >= 0 ? " sada" : "", f.data.size());
    s += b;
  }
  std::string p;
  for (auto &x : a.preskoceno) p += (p.empty() ? "" : ", ") + x;
  printf("     %s (%s, %.1f ms): %s%s%s%s\n", cesta.substr(cesta.find_last_of('/') + 1).c_str(), a.typ.c_str(), t, ok ? s.c_str() : ("NIC: " + a.chyba).c_str(),
         ok ? (" -> SPUSTI " + a.soubory[(size_t)a.vybrany].jmeno).c_str() : "", p.empty() ? "" : " | preskoceno: ", p.c_str());
  return a;
}
static std::vector<std::string> jmena(const nap::AtariArchiv &a) { std::vector<std::string> v; for (auto &f : a.soubory) v.push_back(f.jmeno); return v; }
static bool stejnaSada(const nap::AtariArchiv &a, size_t i, size_t j) { return i < a.soubory.size() && j < a.soubory.size() && a.soubory[i].sada >= 0 && a.soubory[i].sada == a.soubory[j].sada; }

int main(int argc, char **argv) {
  if (argc < 2) { printf("pouziti: %s SLOZKA_Z_vyrob_rar5.py [--cizi SLOZKA ...]\n", argv[0]); return 2; }
  D = argv[1];
  std::vector<std::string> cizi;
  for (int i = 2; i < argc; i++) if (std::string(argv[i]) != "--cizi") cizi.push_back(argv[i]);
  printf("B350 - hra pro Atari v RAR 5 (novy WinRAR): rozbaleni a vyber v C++ (nap_archiv.h, nap_atari_archiv.h)\n");
  if (!existuje(D + "/rar5_kody.rar")) { printf("CHYBA: v %s nejsou archivy - nejdriv python3 vyrob_rar5.py %s\n", D.c_str(), D.c_str()); return 2; }

  printf("\nA) ROZBALENI RAR 5 (obsah se porovna s puvodnimi daty z vyrob_rar5.py)\n");
  Vysledek v = rozbalVse("rar5_ulozeny.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 3 && v.co("Hry/druha.xex") == "OK", "RAR5 ulozeny (metoda 0): 3 soubory, slozka Hry/ se preskoci");
  auto t0 = std::chrono::steady_clock::now();
  v = rozbalVse("rar5_kody.rar");
  printf("     %s  (%.0f ms)\n", popis(v).c_str(), ms(t0));
  over(v.otevreno && v.co("kody.bin") == "OK", "RAR5 se vsemi kody: literaly, 257, 258-261 + RD, 262+ (shody az 4097 B, vzdalenost nad 256 kB), 14 bloku s / bez tabulek, filtry DELTA E8 E8E9 ARM");
  v = rozbalVse("rar5_pevny.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 4, "pevny RAR5: 2. soubor zacina blokem bez tabulek, 3. prazdny bez dat (uprostred retezu), 4. s novymi tabulkami");
  v = rozbalVse("rar5_pevny.rar", {3, 1, 0, 2, 3});
  printf("     na preskacku 4,2,1,3,4: %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 5, "pevny RAR5 na preskacku (retez od 1. komprimovaneho souboru, prazdny soubor ho nerozdeli)");
  v = rozbalVse("rar5_bez_crc.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.co("hra.atr") == "OK" && v.co("odkaz.atr").find("odkaz") != std::string::npos,
       "RAR5 bez CRC32 (jako s otiskem BLAKE2): rozbali se (obsah = puvodni data), odkaz (symlink) neni soubor");

  printf("\nB) CO Z RAR 5 ATARI SPUSTI A CO NABIDNE K VYMENE\n");
  bool ok;
  nap::AtariArchiv a = vyber(D + "/vyber_pop5.rar", ok);
  over(ok && jmena(a) == std::vector<std::string>{"PoP_DD_final.atr", "PoP_SDA_final.atr", "PoP_SDB_final.atr"} && a.soubory[0].sada < 0 && stejnaSada(a, 1, 2) && a.typ == "RAR5",
       "Prince of Persia v RAR5: startuje DD, SDA + SDB sada k vymene, readme preskocen");
  a = vyber(D + "/vyber_sd_pevny5.rar", ok);
  over(ok && jmena(a) == std::vector<std::string>{"PoP_SDA_final.atr", "PoP_SDB_final.atr"} && stejnaSada(a, 0, 1), "pevny RAR5 (v archivu nejdriv SDB): startuje SDA, SDB k vymene");
  a = vyber(D + "/vyber_strany5.rar", ok);
  over(ok && jmena(a) == std::vector<std::string>{"Hra (Side A).atr", "Hra (Side B).atr"} && stejnaSada(a, 0, 1), "strany A / B v RAR5 (B prvni v archivu): startuje strana A");
  a = vyber(D + "/rar5_bez_crc.rar", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.atr"} && a.chybyRozbaleni.empty(), "RAR5 s odkazem: odkaz se ani nenabizi, ani nehlasi jako chyba");

  printf("\nC) ODOLNOST: poskozene RAR 5 nesmi appku shodit ani zaseknout\n");
  {
    const char *vzory[] = {"rar5_ulozeny.rar", "rar5_kody.rar", "rar5_pevny.rar", "rar5_bez_crc.rar", "vyber_pop5.rar", "vyber_sd_pevny5.rar", "vyber_strany5.rar"};
    std::mt19937 rng(350);
    int pokusu = 0, otevreno = 0;
    double nejdele = 0;
    t0 = std::chrono::steady_clock::now();
    for (const char *jm : vzory) {
      const std::vector<uint8_t> puv = nacti(D + "/" + jm);
      if (puv.empty()) continue;
      for (int k = 0; k < 200; k++) {
        std::vector<uint8_t> d = puv;
        const int druh = k % 5;
        if (druh == 0) { for (int j = 0, n = 1 + (int)(rng() % 8); j < n; j++) d[rng() % d.size()] ^= (uint8_t)(1 + rng() % 255); }
        else if (druh == 1) d.resize(rng() % d.size());
        else if (druh == 2) { for (int j = 0; j < 4; j++) d[d.size() / 4 + rng() % (d.size() / 2)] ^= (uint8_t)(1 << (rng() % 8)); }
        else if (druh == 3) { const size_t p = rng() % d.size(); d.insert(d.begin() + (long)p, (size_t)(1 + rng() % 16), (uint8_t)rng()); }
        else { for (size_t j = 8; j < d.size(); j++) if (rng() % 24 == 0) d[j] = (uint8_t)rng(); }
        auto t1 = std::chrono::steady_clock::now();
        nap::AtariArchiv x;
        if (x.otevri(d.data(), d.size(), jm)) otevreno++;
        nejdele = std::max(nejdele, ms(t1));
        pokusu++;
      }
    }
    for (int k = 0; k < 300; k++) {
      std::vector<uint8_t> d(64 + rng() % 4000);
      for (auto &b : d) b = (uint8_t)rng();
      const uint8_t s[] = {'R', 'a', 'r', '!', 0x1A, 0x07, 0x01, 0x00};
      std::memcpy(d.data(), s, 8);
      nap::AtariArchiv x;
      x.otevri(d.data(), d.size(), "nahodny.rar");
      pokusu++;
    }
    printf("     %d poskozenych / nahodnych RAR5 za %.0f ms (nejdelsi %.1f ms), otevreno %d\n", pokusu, ms(t0), nejdele, otevreno);
    over(pokusu == 7 * 200 + 300 && nejdele < 2000, "zadny pad ani zaseknuti (s -fsanitize=address,undefined i bez chyb pameti)");
  }

  printf("\nD) CIZI TESTOVACI RAR 5 (volitelne: libarchive, rarfile - jen lokalne)\n");
  if (cizi.empty()) printf("     (zadne - \"--cizi SLOZKA\")\n");
  else {
    int celkem = 0, cele = 0, odmitnute = 0, jine = 0;
    for (const std::string &s : cizi) {
      DIR *dir = opendir(s.c_str());
      if (!dir) continue;
      std::vector<std::string> v;
      while (dirent *e = readdir(dir)) { const std::string n = e->d_name; if (n.size() > 4 && n.rfind(".rar") == n.size() - 4) v.push_back(n); }
      closedir(dir);
      std::sort(v.begin(), v.end());
      for (const std::string &n : v) {
        const std::vector<uint8_t> d = nacti(s + "/" + n);
        if (d.size() < 8 || std::memcmp(d.data(), "Rar!\x1A\x07\x01\x00", 8) != 0) continue;    // jen RAR 5
        nap::archiv::Archiv x;
        std::string r;
        celkem++;
        if (!x.otevri(d.data(), d.size())) { r = "neotevreno: " + x.chyba; odmitnute++; }
        else {
          int k = 0, kOk = 0, linky = 0;
          std::string ch;
          for (size_t i = 0; i < x.polozky.size(); i++) {
            if (x.polozky[i].adresar) continue;
            if (x.polozky[i].odkaz) { linky++; continue; }
            std::vector<uint8_t> o;
            k++;
            if (x.rozbal(i, o)) kOk++; else if (ch.empty()) ch = x.chyba;
          }
          r = std::to_string(kOk) + "/" + std::to_string(k) + " OK" + (linky ? " (+ " + std::to_string(linky) + " odkaz)" : "") + (ch.empty() ? "" : " (" + ch + ")");
          if (kOk == k) cele++; else jine++;
        }
        for (char &c : r) if ((unsigned char)c < 0x20 || (unsigned char)c >= 0x7F) c = '?';
        printf("     %-58s %s\n", n.c_str(), r.c_str());
      }
    }
    printf("     celkem %d RAR5: %d cele v poradku (CRC sedi), %d odmitnutych (zamerne poskozene testy libarchive, vice dilu), %d jinych\n", celkem, cele, odmitnute, jine);
    over(celkem > 0 && jine == 0 && cele >= 15, "skutecne RAR5 (WinRAR): pevne, vic souboru, filtr ARM, BLAKE2, win32, odkazy ... - vsechny soubory rozbalene, CRC sedi");
  }
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
