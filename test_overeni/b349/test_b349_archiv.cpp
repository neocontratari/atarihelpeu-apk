// test_b349_archiv.cpp - B349: hry pro Atari v ZIP / RAR (cisty C++: nap_archiv.h + nap_atari_archiv.h)
//
// Rene: "pokud to bude zabalene v rar v zipu tak at si s tim atari poradi - opravdu nehodlam davat
// 5 souboru ciste ven". Testy:
//  A) rozbaleni: ZIP (ulozeno, deflate: ulozene / pevne / dynamicke bloky, 17 MB, ZIP64, heslo,
//     poskozeny CRC, zkraceny), RAR4 (ulozeno, komprimovano se vsemi kody RAR 2.9, nizke bity po
//     nove tabulce, pevny archiv i na preskacku, RAR5, zkraceny) - obsah se porovna s puvodnimi daty
//  B) co z archivu Atari spusti a ktere diskety nabidne k vymene (Prince of Persia DD / SDA / SDB,
//     Disk 1-3, Side A / B, XEX, CAR, XFD, PAL / NTSC, vnoreny ZIP / RAR, __MACOSX, readme ...)
//  C) odolnost: poskozene archivy (prehozene bajty, zkraceni, nahodna data) - nesmi spadnout ani viset
//  D) volitelne: skutecne soubory (Prince of Persia RAR Reneho, Acid800 ZIP) - cesty v argumentech
//  E) volitelne: cizi testovaci archivy (libarchive, rarfile) - "--cizi SLOZKA"
//
// Spusteni (z test_overeni/b349):
//   python3 vyrob_archivy.py /tmp/b349_archivy
//   g++ -std=c++17 -O2 -Wall -Wextra -I../../app/src/main/cpp/atari -o test_b349_archiv test_b349_archiv.cpp
//   ./test_b349_archiv /tmp/b349_archivy [PoP_ATR.rar PoP_SD.rar acid800.zip] [--cizi SLOZKA ...]
#include <chrono>
#include <cstdio>
#include <cstring>
#include <dirent.h>
#include <random>
#include <string>
#include <vector>
#include "nap_atari_archiv.h"

static int chyb = 0, kontrol = 0;
static void over(bool ok, const std::string &co) {
  kontrol++;
  if (!ok) chyb++;
  printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str());
  fflush(stdout);
}
static std::vector<uint8_t> nacti(const std::string &cesta) {
  std::vector<uint8_t> d;
  FILE *f = fopen(cesta.c_str(), "rb");
  if (!f) return d;
  uint8_t b[65536];
  size_t n;
  while ((n = fread(b, 1, sizeof b, f)) > 0) d.insert(d.end(), b, b + n);
  fclose(f);
  return d;
}
static bool existuje(const std::string &cesta) { FILE *f = fopen(cesta.c_str(), "rb"); if (f) fclose(f); return f != nullptr; }
static std::string zakladniJmeno(const std::string &c) { const size_t l = c.find_last_of('/'); return l == std::string::npos ? c : c.substr(l + 1); }
static double ms(std::chrono::steady_clock::time_point t0) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count(); }

static std::string D;   // slozka s archivy z vyrob_archivy.py

// ZIP s ulozenymi soubory (metoda 0) sestaveny primo tady
static std::vector<uint8_t> zipUlozeny(const std::vector<std::pair<std::string, std::vector<uint8_t>>> &soubory) {
  std::vector<uint8_t> z, cd;
  auto p16 = [](std::vector<uint8_t> &v, unsigned a) { v.push_back((uint8_t)a); v.push_back((uint8_t)(a >> 8)); };
  auto p32 = [&](std::vector<uint8_t> &v, uint32_t a) { p16(v, a & 0xFFFF); p16(v, a >> 16); };
  for (auto &f : soubory) {
    const uint32_t crc = nap::archiv::crc32(f.second.data(), f.second.size());
    const uint32_t ofs = (uint32_t)z.size(), n = (uint32_t)f.second.size();
    p32(z, 0x04034B50); p16(z, 20); p16(z, 0); p16(z, 0); p16(z, 0); p16(z, 0); p32(z, crc); p32(z, n); p32(z, n);
    p16(z, (unsigned)f.first.size()); p16(z, 0); z.insert(z.end(), f.first.begin(), f.first.end()); z.insert(z.end(), f.second.begin(), f.second.end());
    p32(cd, 0x02014B50); p16(cd, 20); p16(cd, 20); p16(cd, 0); p16(cd, 0); p16(cd, 0); p16(cd, 0); p32(cd, crc); p32(cd, n); p32(cd, n);
    p16(cd, (unsigned)f.first.size()); p16(cd, 0); p16(cd, 0); p16(cd, 0); p16(cd, 0); p32(cd, 0); p32(cd, ofs); cd.insert(cd.end(), f.first.begin(), f.first.end());
  }
  const uint32_t zc = (uint32_t)z.size();
  z.insert(z.end(), cd.begin(), cd.end());
  p32(z, 0x06054B50); p16(z, 0); p16(z, 0); p16(z, (unsigned)soubory.size()); p16(z, (unsigned)soubory.size()); p32(z, (uint32_t)cd.size()); p32(z, zc); p16(z, 0);
  return z;
}

// ---------------- A) rozbaleni ----------------
struct Vysledek {
  bool otevreno = false;
  std::string chyba;
  std::vector<std::pair<std::string, std::string>> polozky;   // jmeno -> "OK" / chyba
  std::string co(const std::string &jm) const { for (auto &p : polozky) if (p.first == jm) return p.second; return "(chybi)"; }
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
    if (r == "OK") {
      const std::string ref = D + "/obsah/" + jm + "/" + p.jmeno;
      if (existuje(ref) && nacti(ref) != o) r = "JINY OBSAH nez puvodni data";
    }
    v.polozky.push_back({p.jmeno, r});
  }
  return v;
}
static std::string popis(const Vysledek &v) {
  if (!v.otevreno) return "NEOTEVRENO: " + v.chyba;
  std::string s;
  for (auto &p : v.polozky) s += (s.empty() ? "" : ", ") + p.first + " " + p.second;
  return s;
}

static void castA() {
  printf("\nA) ROZBALENI ZIP / RAR (obsah se porovna s puvodnimi daty z vyrob_archivy.py)\n");
  Vysledek v = rozbalVse("zip_ulozeny.zip");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 3, "ZIP ulozeny (metoda 0): 3 soubory vcetne prazdneho");

  auto t0 = std::chrono::steady_clock::now();
  v = rozbalVse("zip_deflate.zip");
  const double t = ms(t0);
  printf("     %s  (%.0f ms)\n", popis(v).c_str(), t);
  over(v.otevreno && v.ok() == 8 && v.polozky.size() == 8, "ZIP deflate: dynamicke / pevne / ulozene bloky, urovne 0 1 6 9, 17 MB nul, prazdny soubor, slozka preskocena");

  v = rozbalVse("zip64.zip");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.co("hra.atr") == "OK" && v.co("Hry/druha.xex") == "OK", "ZIP64 (velikosti v rozsireni 0x0001) + jmeno se zpetnym lomitkem 'Hry\\druha.xex'");

  v = rozbalVse("zip_poskozeny_crc.zip");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.co("hra.xex").find("CRC") != std::string::npos && v.co("hra2.xex") == "OK", "ZIP s poskozenym bajtem: CRC nesedi -> chyba, druhy soubor v poradku");

  v = rozbalVse("zip_zkraceny.zip");
  printf("     %s\n", popis(v).c_str());
  over(!v.otevreno && v.chyba.find("chybi seznam") != std::string::npos, "zkraceny ZIP (nedostahovany): chybi seznam souboru");

  v = rozbalVse("zip_heslo.zip");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.co("tajna.atr").find("zaheslovany") != std::string::npos, "ZIP s heslem: soubor je zaheslovany (nerozbaluje se)");

  v = rozbalVse("rar_ulozeny.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 2, "RAR4 ulozeny (metoda 0x30)");

  t0 = std::chrono::steady_clock::now();
  v = rozbalVse("rar_kody.rar");
  printf("     %s  (%.0f ms)\n", popis(v).c_str(), ms(t0));
  over(v.otevreno && v.co("kody.bin") == "OK", "RAR 2.9 se vsemi kody: literaly, 258, 259-262, 263-270, 271+, LDD vcetne 16, 4 nove tabulky, vzdalenost > 256 kB (300 kB)");

  v = rozbalVse("rar_nizke_po_tabulce.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.co("nizke.bin") == "OK", "RAR 2.9: kod 16 hned po nove tabulce = nizke bity 0 (jako unrar / 7-Zip, ne 11 z minula)");

  v = rozbalVse("rar_pevny.rar");
  printf("     %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 4, "pevny (solid) RAR: 4 soubory jednim slovnikem (2. konci novou tabulkou, 3. prazdny)");
  v = rozbalVse("rar_pevny.rar", {3, 1, 0, 2, 3});
  printf("     na preskacku 4,2,1,3,4: %s\n", popis(v).c_str());
  over(v.otevreno && v.ok() == 5, "pevny RAR na preskacku (retez se rozbali znovu od zacatku)");

  v = rozbalVse("rar5.rar");
  printf("     %s\n", popis(v).c_str());
  over(!v.otevreno && v.chyba.find("RAR5") != std::string::npos, "RAR5: poctive \"zatim neumim\" (zadny pad)");

  v = rozbalVse("rar_zkraceny.rar");
  printf("     %s\n", popis(v).c_str());
  over(!v.otevreno || v.ok() < 2, "zkraceny RAR: chyba, zadny pad");
}

// ---------------- B) vyber pro Atari ----------------
static nap::AtariArchiv vyber(const std::string &cesta, bool &ok, double *trvani = nullptr) {
  const std::vector<uint8_t> d = nacti(cesta);
  nap::AtariArchiv a;
  auto t0 = std::chrono::steady_clock::now();
  ok = a.otevri(d.data(), d.size(), zakladniJmeno(cesta));
  if (trvani) *trvani = ms(t0);
  std::string s;
  for (size_t i = 0; i < a.soubory.size(); i++) {
    const nap::AtariSoubor &f = a.soubory[i];
    char b[300];
    snprintf(b, sizeof b, "%s%s[%c%s%s %zu B]", i ? ", " : "", f.jmeno.c_str(), f.druh, f.druh == 'A' ? (f.boot ? " boot" : " data") : "",
             f.sada >= 0 ? (" sada" + std::to_string(f.sada)).c_str() : "", f.data.size());
    s += b;
  }
  std::string p;
  for (auto &x : a.preskoceno) p += (p.empty() ? "" : ", ") + x;
  for (auto &x : a.chybyRozbaleni) p += (p.empty() ? "" : ", ") + x;
  printf("     %s (%s): %s%s%s%s\n", zakladniJmeno(cesta).c_str(), a.typ.c_str(), ok ? s.c_str() : ("NIC: " + a.chyba).c_str(),
         ok ? (" -> SPUSTI " + a.soubory[(size_t)a.vybrany].jmeno).c_str() : "", p.empty() ? "" : " | preskoceno: ", p.c_str());
  return a;
}
static std::vector<std::string> jmena(const nap::AtariArchiv &a) { std::vector<std::string> v; for (auto &f : a.soubory) v.push_back(f.jmeno); return v; }
static bool stejnaSada(const nap::AtariArchiv &a, size_t i, size_t j) { return i < a.soubory.size() && j < a.soubory.size() && a.soubory[i].sada >= 0 && a.soubory[i].sada == a.soubory[j].sada; }
static bool obsahuje(const std::vector<std::string> &v, const std::string &s) { for (auto &x : v) if (x.find(s) != std::string::npos) return true; return false; }

static void castB() {
  printf("\nB) CO Z ARCHIVU ATARI SPUSTI A CO NABIDNE K VYMENE (poradi = nabidka ATR / DISK)\n");
  bool ok;
  nap::AtariArchiv a = vyber(D + "/vyber_pop.rar", ok);
  over(ok && jmena(a) == std::vector<std::string>{"PoP_DD_final.atr", "PoP_SDA_final.atr", "PoP_SDB_final.atr"} && a.vybrany == 0 && a.soubory[0].sada < 0 &&
           stejnaSada(a, 1, 2) && obsahuje(a.preskoceno, "readme.txt") && a.typ == "RAR",
       "Prince of Persia (RAR): startuje DD (cela hra na 1 diskete), SDA + SDB = sada k vymene, readme preskocen");
  a = vyber(D + "/vyber_sd_pevny.rar", ok);
  over(ok && jmena(a) == std::vector<std::string>{"PoP_SDA_final.atr", "PoP_SDB_final.atr"} && stejnaSada(a, 0, 1),
       "pevny RAR, v archivu nejdriv SDB: startuje SDA (strana A), SDB pripravena k vymene");
  a = vyber(D + "/vyber_disky.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"Hra Disk 1.atr", "Hra Disk 2.atr", "Hra Disk 3.atr"} && stejnaSada(a, 0, 1) && stejnaSada(a, 1, 2) && a.soubory[0].boot,
       "Disk 1 / 2 / 3 (v ZIPu 3, 1, 2): startuje Disk 1, ostatni k vymene");
  a = vyber(D + "/vyber_strany.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"Hra (Side A).atr", "Hra (Side B).atr"} && stejnaSada(a, 0, 1), "Side A / Side B: startuje strana A");
  a = vyber(D + "/vyber_xex_readme.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.xex"} && a.preskoceno.size() == 2, "XEX + navod.txt + obrazek.png: spusti XEX, text a obrazek preskoci");
  a = vyber(D + "/vyber_data_a_xex.zip", ok);
  over(ok && a.soubory.size() == 2 && a.soubory[0].jmeno == "zavadec.xex" && !a.soubory[1].boot, "datova disketa (bez bootovaciho sektoru) + XEX: spusti XEX, disketa zustane v nabidce");
  a = vyber(D + "/vyber_pal_ntsc.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"Hra (PAL).xex", "Hra (NTSC).xex"}, "PAL a NTSC verze: startuje PAL");
  a = vyber(D + "/vyber_vnoreny.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"vnitrni.zip/hra.xex"} && obsahuje(a.preskoceno, "info.txt"), "ZIP v ZIPu: hra se najde uvnitr");
  a = vyber(D + "/vyber_vnoreny_rar.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.rar/hra.atr"} && a.soubory[0].boot, "RAR v ZIPu: disketa se najde uvnitr");
  a = vyber(D + "/vyber_xfd.zip", ok);
  over(ok && a.soubory.size() == 1 && a.soubory[0].druh == 'A' && a.soubory[0].data.size() == 92160 + 16 && a.soubory[0].data[0] == 0x96 &&
           a.soubory[0].data[2] == 0x80 && a.soubory[0].data[3] == 0x16 && a.soubory[0].boot,
       "XFD (disketa bez hlavicky): doplni se hlavicka ATR (720 sektoru = $1680 odstavcu)");
  a = vyber(D + "/vyber_macosx.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.atr"}, "__MACOSX/._hra.atr a ._hra.atr (smeti z Macu) se ignoruji");
  a = vyber(D + "/vyber_jen_readme.zip", ok);
  over(!ok && a.chyba.find("neni hra pro Atari") != std::string::npos, "v archivu jen readme a obrazek: \"v archivu neni hra pro Atari\"");
  a = vyber(D + "/vyber_cartridge.zip", ok);
  over(ok && a.soubory.size() == 1 && a.soubory[0].druh == 'C', "cartridge CAR v ZIPu");
  a = vyber(D + "/vyber_dve_hry.zip", ok);
  over(ok && a.soubory.size() == 2 && a.soubory[0].sada < 0 && a.soubory[1].sada < 0 && a.soubory[0].jmeno == "Jetboy.atr",
       "dve ruzne hry (Boulder / Jetboy): nejsou sada, startuje vetsi, druha je v nabidce");
  a = vyber(D + "/vyber_slozky.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.atr"}, "hra ve slozkach Hry/Atari/ (slozky se preskoci)");
  a = vyber(D + "/vyber_velky.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.xex"} && obsahuje(a.preskoceno, "obrovsky.bin"), "soubor nad 16 MB se ani nerozbali (preskocen), hra ano");
  a = vyber(D + "/vyber_neznamy.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra.xex"} && obsahuje(a.preskoceno, "hra.bin"), "neznamy obsah (hra.bin) se preskoci - rozhoduje obsah, ne pripona");
  a = vyber(D + "/vyber_jmena.zip", ok);
  over(ok && a.soubory.size() == 2 && obsahuje(jmena(a), "Hra \xE2\x80\x93 \xC5\x98" "e\xC5\xBE.xex") && obsahuje(jmena(a), "tab novy radek.xex"),
       "jmena: UTF-8 zustane, TAB / konec radku -> mezera (seznam pro Javu se nerozbije)");
  a = vyber(D + "/zip_heslo.zip", ok);
  over(!ok && a.chyba.find("zaheslovany") != std::string::npos, "jen zaheslovana hra: chyba \"zaheslovany\" (appka: ARCHIV MA HESLO)");
  a = vyber(D + "/zip_heslo_a_hra.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"volna.xex"} && a.chybyRozbaleni.size() == 1, "zaheslovany soubor + volna hra: spusti volnou");
  a = vyber(D + "/zip_poskozeny_crc.zip", ok);
  over(ok && jmena(a) == std::vector<std::string>{"hra2.xex"} && a.chybyRozbaleni.size() == 1, "poskozeny soubor (CRC) + dobry: spusti dobry");
  a = vyber(D + "/rar5.rar", ok);
  over(!ok && a.chyba.find("RAR5") != std::string::npos, "RAR5: chyba pro hlasku \"RAR5 ZATIM NEUMIM\"");
  // sbirka (limit): 260 her v jednom ZIPu -> do pameti jen 200
  {
    const std::vector<uint8_t> x = {0xFF, 0xFF, 0x00, 0x20, 0x01, 0x20, 0xA9, 0x00, 0xE0, 0x02, 0xE1, 0x02, 0x00, 0x20};
    const int N = 260;
    std::vector<std::pair<std::string, std::vector<uint8_t>>> hry;
    for (int i = 0; i < N; i++) { char jm[32]; snprintf(jm, sizeof jm, "hra%03d.xex", i); hry.push_back({jm, x}); }
    const std::vector<uint8_t> z = zipUlozeny(hry);
    nap::AtariArchiv s;
    const bool o = s.otevri(z.data(), z.size(), "sbirka.zip");
    printf("     sbirka.zip (ZIP): %zu her pro Atari, preskoceno %zu (posledni: %s)\n", s.soubory.size(), s.preskoceno.size(), s.preskoceno.empty() ? "-" : s.preskoceno.back().c_str());
    over(o && s.soubory.size() == nap::AtariArchiv::MAX_SOUBORU && s.preskoceno.size() == (size_t)N - nap::AtariArchiv::MAX_SOUBORU,
         "sbirka 260 her v jednom ZIPu: do pameti jen 200 (pojistka), zbytek preskocen s poznamkou (limit)");
  }
}

// ---------------- C) odolnost ----------------
static void castC() {
  printf("\nC) ODOLNOST: poskozene archivy nesmi appku shodit ani zaseknout\n");
  const char *vzory[] = {"zip_ulozeny.zip", "zip64.zip", "zip_poskozeny_crc.zip", "rar_ulozeny.rar", "rar_kody.rar", "rar_pevny.rar", "rar_nizke_po_tabulce.rar",
                         "vyber_pop.rar", "vyber_sd_pevny.rar", "vyber_vnoreny.zip", "vyber_vnoreny_rar.zip", "vyber_xfd.zip", "vyber_disky.zip"};
  std::mt19937 rng(349);
  int pokusu = 0, otevreno = 0, hra = 0;
  auto t0 = std::chrono::steady_clock::now();
  double nejdele = 0;
  for (const char *jm : vzory) {
    const std::vector<uint8_t> puv = nacti(D + "/" + jm);
    if (puv.empty()) continue;
    for (int k = 0; k < 150; k++) {
      std::vector<uint8_t> d = puv;
      const int druh = k % 5;
      if (druh == 0) {                                        // par prehozenych bajtu kdekoli
        for (int j = 0, n = 1 + (int)(rng() % 8); j < n; j++) d[rng() % d.size()] ^= (uint8_t)(1 + rng() % 255);
      } else if (druh == 1) {                                 // zkraceni
        d.resize(rng() % d.size());
      } else if (druh == 2) {                                 // poskozena data za hlavickami (prvni 2/3 souboru)
        for (int j = 0; j < 4; j++) d[64 + rng() % (d.size() * 2 / 3 > 64 ? d.size() * 2 / 3 - 64 : 1)] ^= (uint8_t)(1 + rng() % 255);
      } else if (druh == 3) {                                 // nahodne bajty za signaturou
        for (size_t j = 8; j < d.size(); j++) if (rng() % 16 == 0) d[j] = (uint8_t)rng();
      } else {                                                // vlozene / vyrizle bajty
        const size_t p = rng() % d.size();
        if (rng() % 2) d.insert(d.begin() + (long)p, (size_t)(1 + rng() % 16), (uint8_t)rng());
        else d.erase(d.begin() + (long)p, d.begin() + (long)std::min(d.size(), p + 1 + rng() % 16));
      }
      auto t1 = std::chrono::steady_clock::now();
      nap::AtariArchiv a;
      if (a.otevri(d.data(), d.size(), jm)) { otevreno++; if (!a.soubory.empty()) hra++; }
      nejdele = std::max(nejdele, ms(t1));
      pokusu++;
    }
  }
  // ciste nahodna data se signaturou ZIP / RAR
  for (int k = 0; k < 300; k++) {
    std::vector<uint8_t> d(64 + rng() % 4000);
    for (auto &b : d) b = (uint8_t)rng();
    if (k % 2) { const uint8_t s[] = {'P', 'K', 3, 4}; std::memcpy(d.data(), s, 4); }
    else { const uint8_t s[] = {'R', 'a', 'r', '!', 0x1A, 0x07, 0x00}; std::memcpy(d.data(), s, 7); }
    nap::AtariArchiv a;
    a.otevri(d.data(), d.size(), "nahodny");
    pokusu++;
  }
  printf("     %d poskozenych / nahodnych archivu za %.0f ms (nejdelsi %.1f ms): otevreno %d, z toho s hrou %d\n", pokusu, ms(t0), nejdele, otevreno, hra);
  // hranicni soubory: ATR jen s hlavickou / bez celeho sektoru, kratky XEX / CAR, XFD 3 sektory
  {
    auto bajty = [](std::initializer_list<int> zac, size_t delka) { std::vector<uint8_t> v(delka, 0); size_t i = 0; for (int b : zac) if (i < delka) v[i++] = (uint8_t)b; return v; };
    const std::vector<uint8_t> z = zipUlozeny({{"hlavicka.atr", bajty({0x96, 0x02}, 16)}, {"skoro.atr", bajty({0x96, 0x02}, 143)}, {"sektor.atr", bajty({0x96, 0x02, 8, 0, 128, 0, 0, 0}, 144)},
                                               {"kratky.xex", bajty({0xFF, 0xFF}, 5)}, {"xex6.xex", bajty({0xFF, 0xFF, 0, 0x20, 0, 0x20}, 6)}, {"cart.car", bajty({'C', 'A', 'R', 'T'}, 15)},
                                               {"tri.xfd", bajty({0}, 384)}, {"prazdny.atr", {}}});
    nap::AtariArchiv a;
    const bool o = a.otevri(z.data(), z.size(), "hranice.zip");
    std::string s;
    for (auto &f : a.soubory) s += (s.empty() ? "" : ", ") + f.jmeno + "[" + f.druh + "]";
    printf("     hranice.zip: %s | preskoceno %zu\n", s.c_str(), a.preskoceno.size());
    over(o && a.soubory.size() == 3 && a.preskoceno.size() == 5 && obsahuje(jmena(a), "sektor.atr") && obsahuje(jmena(a), "xex6.xex") && obsahuje(jmena(a), "tri.xfd"),
         "hranicni soubory: ATR bez celeho sektoru, XEX < 6 B, CAR bez hlavicky, prazdny -> preskocit; ATR s 1 sektorem, XEX 6 B, XFD 3 sektory -> ano");
  }
  over(pokusu == 13 * 150 + 300 && nejdele < 2000, "zadny pad ani zaseknuti (prekladem s -fsanitize=address,undefined i bez chyb pameti - JAK_SPUSTIT.txt)");
}

// ---------------- D) skutecne soubory ----------------
static void castD(const std::vector<std::string> &soubory) {
  printf("\nD) SKUTECNE SOUBORY (volitelne, v repozitari nejsou)\n");
  if (soubory.empty()) { printf("     (zadne - cesty k PoP_ATR / PoP_SD RAR a acid800 ZIP jako argumenty)\n"); return; }
  for (const std::string &c : soubory) {
    bool ok;
    double t = 0;
    nap::AtariArchiv a = vyber(c, ok, &t);
    printf("     rozbaleno a vybrano za %.1f ms\n", t);
    const std::string jm = zakladniJmeno(c);
    if (jm.find("PoP_ATR") != std::string::npos) {
      over(ok && a.soubory.size() == 3 && a.soubory[0].jmeno.find("PoP_DD") == 0 && a.soubory[1].jmeno.find("PoP_SDA") == 0 && stejnaSada(a, 1, 2) &&
               a.soubory[0].data.size() == 183952 && a.soubory[1].data.size() == 92176,
           "Prince of Persia " + jm + ": startuje DD (183952 B), SDA + SDB k vymene");
    } else if (jm.find("PoP_SD") != std::string::npos) {
      over(ok && a.soubory.size() == 2 && a.soubory[0].jmeno.find("PoP_SDA") == 0 && a.soubory[1].jmeno.find("PoP_SDB") == 0 && stejnaSada(a, 0, 1),
           "Prince of Persia " + jm + ": startuje SDA, SDB pripravena k vymene (ATR / DISK)");
    } else if (jm.find("acid800") != std::string::npos) {
      over(ok && a.soubory.size() == 1 && a.soubory[0].jmeno == "acid800.atr" && a.soubory[0].boot, "Acid800 " + jm + ": spusti acid800.atr, README.html preskoci");
    } else {
      over(ok, "archiv " + jm + " ma co spustit");
    }
  }
}

// ---------------- E) cizi testovaci archivy ----------------
static void castE(const std::vector<std::string> &slozky) {
  printf("\nE) CIZI TESTOVACI ARCHIVY (volitelne: libarchive, rarfile - jen lokalne)\n");
  if (slozky.empty()) { printf("     (zadne - \"--cizi SLOZKA\")\n"); return; }
  int celkem = 0, ok = 0, neumim = 0, jine = 0;
  std::vector<std::string> zname;
  for (const std::string &s : slozky) {
    DIR *dir = opendir(s.c_str());
    if (!dir) continue;
    std::vector<std::string> v;
    while (dirent *e = readdir(dir)) { const std::string n = e->d_name; if (n.size() > 4 && (n.rfind(".rar") == n.size() - 4 || n.rfind(".zip") == n.size() - 4)) v.push_back(n); }
    closedir(dir);
    std::sort(v.begin(), v.end());
    for (const std::string &n : v) {
      const std::vector<uint8_t> d = nacti(s + "/" + n);
      nap::archiv::Archiv a;
      std::string r;
      if (!a.otevri(d.data(), d.size())) r = "neotevreno: " + a.chyba;
      else {
        int k = 0, kOk = 0;
        std::string ch;
        for (size_t i = 0; i < a.polozky.size(); i++) {
          if (a.polozky[i].adresar || a.polozky[i].odkaz) continue;   // B350: odkaz (symlink) v RAR5 neni soubor
          std::vector<uint8_t> o;
          k++;
          if (a.rozbal(i, o)) kOk++; else if (ch.empty()) ch = a.chyba;
        }
        r = std::to_string(kOk) + "/" + std::to_string(k) + " OK" + (ch.empty() ? "" : " (" + ch + ")");
        if (kOk == k) ok++;
      }
      celkem++;
      const bool vysvetleno = r.find("RAR5") != std::string::npos || r.find("PPMd") != std::string::npos || r.find("zaheslovan") != std::string::npos ||
                              r.find("vice dilu") != std::string::npos || r.find("stary format") != std::string::npos || r.find("moc velky") != std::string::npos ||
                              r.find("zkraceny") != std::string::npos || r.find("spatna hlavicka") != std::string::npos;
      if (r.find(" OK") != std::string::npos && r.find("(") == std::string::npos) {}
      else if (vysvetleno) neumim++;
      else jine++;
      std::string rt = r;                                     // jmena v poskozenych archivech: jen tisknutelne
      for (char &ch : rt) if ((unsigned char)ch < 0x20 || (unsigned char)ch >= 0x7F) ch = '?';
      printf("     %-58s %s\n", n.c_str(), rt.c_str());
      if (n == "rar3-solid.rar" || n == "test_read_format_rar_filter.rar" || n == "test_read_format_rar_multi_lzss_blocks.rar" ||
          n == "test_read_format_rar_binary_data.rar" || n == "test_read_format_rar_compress_normal.rar" || n == "seektest.rar")
        zname.push_back(n + (r.find("(") == std::string::npos && r.find(" OK") != std::string::npos ? "" : " CHYBA"));
    }
  }
  printf("     celkem %d archivu: %d cele v poradku, %d znamych \"neumim\" (RAR5, PPMd, heslo, vice dilu, RAR 1.5, poskozene), %d jinych\n", celkem, ok, neumim, jine);
  bool zn = !zname.empty();
  for (auto &x : zname) if (x.find("CHYBA") != std::string::npos) zn = false;
  over(zn && jine == 0, "skutecne RAR 2.9: pevny (rar3-solid), filtr E8 (bsdcat.exe), 20 MB vic bloku, 1 MB nahodnych dat - CRC sedi; zbytek jen zname \"neumim\"");
}

int main(int argc, char **argv) {
  if (argc < 2) { printf("pouziti: %s SLOZKA_Z_vyrob_archivy.py [soubory ...] [--cizi SLOZKA ...]\n", argv[0]); return 2; }
  D = argv[1];
  std::vector<std::string> soubory, cizi;
  bool c = false;
  for (int i = 2; i < argc; i++) {
    if (std::string(argv[i]) == "--cizi") { c = true; continue; }
    (c ? cizi : soubory).push_back(argv[i]);
  }
  printf("B349 - hry pro Atari v ZIP / RAR: rozbaleni a vyber v C++ (nap_archiv.h, nap_atari_archiv.h)\n");
  if (!existuje(D + "/rar_kody.rar")) { printf("CHYBA: v %s nejsou archivy - nejdriv python3 vyrob_archivy.py %s\n", D.c_str(), D.c_str()); return 2; }
  castA();
  castB();
  castC();
  castD(soubory);
  castE(cizi);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
