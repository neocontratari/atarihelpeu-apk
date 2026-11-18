// nap_atari_archiv.h
// B349: hra pro Atari zabalena v ZIP / RAR - co z archivu spustit a ktere
// diskety nabidnout k vymene (cisty C++, archivy cte nap_archiv.h).
//
// Rene: "pokud to bude zabalene v rar v zipu tak at si s tim atari poradi".
// Prince of Persia: RAR se 3 disketami (DD = cela hra na jedne, SDA + SDB =
// dve strany) + readme. Pravidla:
//  - z archivu se berou soubory pro Atari podle OBSAHU: ATR ($96 $02),
//    XEX / COM / EXE ($FF $FF), CAR ("CART"), XFD (disketa bez hlavicky ->
//    doplni se hlavicka ATR); readme, obrazky apod. se preskoci; ZIP / RAR
//    uvnitr archivu se otevre taky (jedna uroven),
//  - diskety, jejichz jmena se lisi jen jednim znakem / cislem (SDA / SDB,
//    Disk1 / Disk2, side A / side B), jsou SADA - startuje prvni (A / 1),
//    ostatni jsou pripravene k vymene bez restartu,
//  - samostatna bootovaci disketa (DD) ma prednost pred sadou (cela hra na
//    jedne diskete = bez vymeny), PAL pred NTSC, vetsi pred mensi,
//  - poradi druhu: bootovaci disketa ATR, pak XEX, pak cartridge CAR, nakonec
//    diskety bez bootovaciho sektoru (data).
#pragma once
#include <algorithm>
#include <cctype>
#include <string>
#include <vector>
#include "nap_archiv.h"

namespace nap {

struct AtariSoubor {
  std::string jmeno;               // jmeno v archivu (bez cesty; u vnoreneho archivu "vnitrni.zip/hra.atr")
  std::vector<uint8_t> data;       // pripraveny obsah (XFD uz s hlavickou ATR)
  char druh = '?';                 // 'A' disketa ATR, 'X' program XEX, 'C' cartridge CAR
  int sada = -1;                   // cislo sady disket (-1 = samostatna)
  bool boot = false;               // disketa ma bootovaci sektor
};

class AtariArchiv {
 public:
  std::string jmeno;               // jmeno archivu (napr. PoP_ATR_20211206_final.rar)
  std::string typ;                 // "ZIP" / "RAR"
  std::vector<AtariSoubor> soubory;   // poradi = nabidka (diskety, XEX, CAR)
  int vybrany = -1;                // co se ma spustit
  std::string chyba;               // proc nic (pro log a hlasku)
  std::vector<std::string> preskoceno;   // ostatni soubory archivu (readme ...)
  std::vector<std::string> chybyRozbaleni;
  // pojistka proti "sbirce" (tisice her v jednom ZIPu): vic se do pameti nerozbali
  static constexpr size_t MAX_SOUBORU = 200;
  static constexpr size_t MAX_DAT = 48u << 20;

  static bool jeArchiv(const uint8_t *d, size_t n) { return archiv::Archiv::poznej(d, n) != archiv::Archiv::ZADNY; }

  bool otevri(const uint8_t *d, size_t n, const std::string &jm) {
    jmeno = jm; soubory.clear(); vybrany = -1; chyba.clear(); preskoceno.clear(); chybyRozbaleni.clear(); typ.clear(); celkem = 0;
    if (!projdi(d, n, "", 0)) return false;
    if (soubory.empty()) {
      if (chyba.empty()) chyba = chybyRozbaleni.empty() ? "v archivu neni hra pro Atari (ATR, XEX, CAR)" : chybyRozbaleni[0];
      return false;
    }
    seradAVyber();
    return true;
  }

  // diskety v poradi pro vymenu (index do soubory)
  std::vector<int> diskety() const {
    std::vector<int> v;
    for (int i = 0; i < (int)soubory.size(); i++) if (soubory[i].druh == 'A') v.push_back(i);
    return v;
  }
  int pocet(char druh) const { int k = 0; for (auto &s : soubory) if (s.druh == druh) k++; return k; }

 private:
  size_t celkem = 0;               // rozbaleno bajtu (soubory pro Atari)

  static std::string maleBezCesty(const std::string &cesta) {
    const size_t l = cesta.find_last_of('/');
    std::string s = l == std::string::npos ? cesta : cesta.substr(l + 1);
    for (char &c : s) c = (char)std::tolower((unsigned char)c);
    return s;
  }
  static std::string pripona(const std::string &maleJmeno) {
    const size_t t = maleJmeno.find_last_of('.');
    return t == std::string::npos ? std::string() : maleJmeno.substr(t);
  }
  static std::string bezPripony(const std::string &maleJmeno) {
    const size_t t = maleJmeno.find_last_of('.');
    return t == std::string::npos ? maleJmeno : maleJmeno.substr(0, t);
  }

  bool projdi(const uint8_t *d, size_t n, const std::string &predpona, int hloubka) {
    archiv::Archiv a;
    if (hloubka == 0) typ = archiv::Archiv::nazevTypu(archiv::Archiv::poznej(d, n));   // i kdyz nejde otevrit (RAR5)
    if (!a.otevri(d, n)) {
      if (hloubka == 0) chyba = a.chyba;
      else chybyRozbaleni.push_back(predpona + ": " + a.chyba);
      return hloubka > 0;
    }
    if (hloubka == 0) typ = archiv::Archiv::nazevTypu(a.typ);
    for (size_t i = 0; i < a.polozky.size(); i++) {
      const archiv::Polozka &p = a.polozky[i];
      if (p.adresar || p.odkaz) continue;
      const std::string male = maleBezCesty(p.jmeno), ext = pripona(male);
      const size_t lom = p.jmeno.find_last_of('/');
      std::string kratke = predpona + (lom == std::string::npos ? p.jmeno : p.jmeno.substr(lom + 1));
      for (char &c : kratke) if ((unsigned char)c < 0x20 || c == 0x7F) c = ' ';   // TAB / konec radku by rozbil seznam
      if (p.jmeno.rfind("__MACOSX/", 0) == 0 || male.rfind("._", 0) == 0) continue;
      static const char *text[] = {".txt", ".nfo", ".diz", ".doc", ".pdf", ".htm", ".html", ".md", ".jpg", ".jpeg", ".png", ".gif", ".bmp", ".url"};
      bool jeText = false;
      for (const char *t : text) if (ext == t) jeText = true;
      if (jeText || p.velikost > (16u << 20)) { preskoceno.push_back(kratke); continue; }
      if (soubory.size() >= MAX_SOUBORU || celkem + p.velikost > MAX_DAT) { preskoceno.push_back(kratke + " (limit)"); continue; }
      std::vector<uint8_t> o;
      if (!a.rozbal(i, o)) { chybyRozbaleni.push_back(kratke + ": " + a.chyba); continue; }
      if ((ext == ".zip" || ext == ".rar" || archiv::Archiv::poznej(o.data(), o.size()) != archiv::Archiv::ZADNY) && hloubka == 0) {
        projdi(o.data(), o.size(), kratke + "/", 1);
        continue;
      }
      AtariSoubor s;
      s.jmeno = kratke;
      if (o.size() >= 16 + 128 && o[0] == 0x96 && o[1] == 0x02) s.druh = 'A';      // hlavicka + aspon 1 sektor
      else if (o.size() >= 16 && o[0] == 'C' && o[1] == 'A' && o[2] == 'R' && o[3] == 'T') s.druh = 'C';
      else if (o.size() >= 6 && o[0] == 0xFF && o[1] == 0xFF) s.druh = 'X';
      else if (ext == ".xfd" && o.size() >= 128 * 3 && o.size() % 128 == 0) {
        // XFD = disketa bez hlavicky (sektory po 128 B) -> doplnit hlavicku ATR
        const size_t par = o.size() / 16;
        uint8_t h[16] = {0x96, 0x02, (uint8_t)par, (uint8_t)(par >> 8), 128, 0, (uint8_t)(par >> 16), 0, 0, 0, 0, 0, 0, 0, 0, 0};
        o.insert(o.begin(), h, h + 16);
        s.druh = 'A';
      }
      if (s.druh == '?') { preskoceno.push_back(kratke); continue; }
      if (s.druh == 'A' && o.size() >= 16 + 128) {
        // bootovaci sektor: pocet sektoru >= 1, adresa nahrani $0400..$BFFF
        const uint8_t *b = o.data() + 16;
        const unsigned adresa = b[2] | (b[3] << 8);
        s.boot = b[1] >= 1 && adresa >= 0x0400 && adresa < 0xC000;
      }
      celkem += o.size();
      s.data.swap(o);
      soubory.push_back(std::move(s));
    }
    return true;
  }

  // jmena se lisi jen kratkym usekem (1-2 znaky, pismena / cisla stejne delky): SDA/SDB, 1/2, 01/02
  static bool sourozenci(const std::string &a, const std::string &b, std::string &ua, std::string &ub) {
    if (a == b) return false;
    size_t p = 0;
    while (p < a.size() && p < b.size() && a[p] == b[p]) p++;
    size_t s = 0;
    while (s < a.size() - p && s < b.size() - p && a[a.size() - 1 - s] == b[b.size() - 1 - s]) s++;
    ua = a.substr(p, a.size() - p - s); ub = b.substr(p, b.size() - p - s);
    if (ua.size() != ub.size() || ua.empty() || ua.size() > 2) return false;
    for (char c : ua + ub) if (!std::isalnum((unsigned char)c)) return false;
    return true;
  }

  void seradAVyber() {
    // sady disket (sjednoceni sourozencu)
    const int n = (int)soubory.size();
    std::vector<int> rodic(n);
    for (int i = 0; i < n; i++) rodic[i] = i;
    auto koren = [&](int x) { while (rodic[x] != x) x = rodic[x] = rodic[rodic[x]]; return x; };
    std::vector<std::string> klic(n);
    for (int i = 0; i < n; i++) klic[i] = bezPripony(maleBezCesty(soubory[i].jmeno));
    for (int i = 0; i < n; i++) for (int j = i + 1; j < n; j++) {
      if (soubory[i].druh != 'A' || soubory[j].druh != 'A') continue;
      std::string ua, ub;
      if (sourozenci(klic[i], klic[j], ua, ub)) rodic[koren(i)] = koren(j);
    }
    std::vector<int> velikostSady(n, 0);
    for (int i = 0; i < n; i++) velikostSady[koren(i)]++;
    for (int i = 0; i < n; i++) soubory[i].sada = (soubory[i].druh == 'A' && velikostSady[koren(i)] > 1) ? koren(i) : -1;
    // poradi: diskety (samostatne bootovaci -> sady -> ostatni), XEX, CAR; v ramci podle jmena
    // bootovaci disketa samostatna 3600 > bootovaci v sade 3400 > XEX 2500 > CAR 2000 >
    // disketa bez bootovaciho sektoru (data, druha strana) 1000 / 1200
    auto skore = [&](const AtariSoubor &s) {
      int k = 0;
      if (s.druh == 'A') k = (s.boot ? 3400 : 1000) + (s.sada < 0 ? 200 : 0);
      else if (s.druh == 'X') k = 2500;
      else k = 2000;
      const std::string m = maleBezCesty(s.jmeno);
      if (m.find("pal") != std::string::npos) k += 20;
      if (m.find("ntsc") != std::string::npos) k -= 20;
      return k;
    };
    // stabilni razeni: skore, pak sada (aby clenove sady byli u sebe), pak jmeno
    std::vector<int> idx(n);
    for (int i = 0; i < n; i++) idx[i] = i;
    std::vector<int> skoreSady(n, -1000000);
    for (int i = 0; i < n; i++) if (soubory[i].sada >= 0) skoreSady[soubory[i].sada] = std::max(skoreSady[soubory[i].sada], skore(soubory[i]));
    std::stable_sort(idx.begin(), idx.end(), [&](int a, int b) {
      const AtariSoubor &x = soubory[a], &y = soubory[b];
      const int sx = x.sada >= 0 ? skoreSady[x.sada] : skore(x), sy = y.sada >= 0 ? skoreSady[y.sada] : skore(y);
      if (sx != sy) return sx > sy;
      if (x.sada != y.sada) return x.sada < y.sada;
      if (x.data.size() != y.data.size() && x.sada < 0 && y.sada < 0) return x.data.size() > y.data.size();
      return maleBezCesty(x.jmeno) < maleBezCesty(y.jmeno);
    });
    std::vector<AtariSoubor> nove;
    for (int i : idx) nove.push_back(std::move(soubory[i]));
    soubory.swap(nove);
    vybrany = soubory.empty() ? -1 : 0;
  }
};

}  // namespace nap
