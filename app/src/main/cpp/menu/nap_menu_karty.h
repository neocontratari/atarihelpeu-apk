// nap_menu_karty.h
// ====================================================================
//  B311: KARTY HLAVNIHO MENU - seznam se cte ze souboru assets/menu/karty.txt
//
//  Rene: "ten kod pro to skrolovani obrazku napis modularne a dynamicky
//  a pocitej s tim, ze aplikaci budeme v budoucnu dale rozsirovat o dalsi
//  masiny a sekce. At tam pak staci jenom jednoduse prihodit dalsi velkou
//  ikonu a cele to skrolovani se s tim samo srovna."
//
//  Proto v kodu NENI zadny pevny seznam tlacitek: kazda karta je jeden
//  radek v karty.txt (poradi radku = poradi v menu). Novou masinu pridas
//  tak, ze das obrazek do assets/menu/ a pripises radek - karusel, tecky,
//  fyzika i dotyk se prizpusobi samy (nic se neprekládá).
//
//  Radek:  id | NAZEV | podtitul | popis | obrazek | cil | barva | zamek | efekt | tlacitko
//   id        kratke jmeno (log, zamky), napr. atari
//   NAZEV     velky napis na karte
//   podtitul  maly napis pod nazvem (na karte)
//   popis     veta pod kartami (co to je)
//   obrazek   obrazek karty v assets (png / jpg), napr. menu/karta_atari.jpg
//   cil       kam karta vede: stranka v assets (emu_sega/index.html),
//             web (https://...) nebo akce:jmeno
//   barva     barva karty #RRGGBB (zare, tecky, pozadi)
//   zamek     klic zamku (sega, ps1 - stav hlasi Java), prazdne = nikdy zamcena
//   efekt     zivy obrazek v C++ (3d:atari, 3d:nap16, 3d:nap32, fx:kazeta,
//             fx:dzubox, fx:web, fx:help); prazdne = jen obrazek
//   tlacitko  text tlacitka pod kartami (prazdne = OTEVRIT)
//  Prazdne radky a radky zacinajici # se preskoci. Chybejici pole na konci
//  radku = prazdna.
//
//  Bez Androidu a bez OpenGL - testuje se na PC (test_overeni/b311).
// ====================================================================
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace napmenu {

struct Karta {
  std::string id, nazev, podtitul, popis, obrazek, cil, zamekKlic, efekt, tlacitko;
  uint32_t barva = 0xF2B640;          // 0xRRGGBB
  // stav (nastavuje Java pres JNI)
  bool zamceno = false;
  std::string zamekText;              // proc je zamcena (ukaze se misto popisu)
};

// "#F2B640" / "f2b640" / "#fb4" -> 0xRRGGBB (spatny zapis = vychozi)
inline uint32_t barvaZHex(const std::string &s, uint32_t vychozi) {
  std::string h;
  for (char c : s)
    if (c != '#' && c != ' ' && c != '\t') h += c;
  auto hex = [](char c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
  };
  if (h.size() == 3) h = std::string() + h[0] + h[0] + h[1] + h[1] + h[2] + h[2];
  if (h.size() != 6) return vychozi;
  uint32_t v = 0;
  for (char c : h) {
    const int d = hex(c);
    if (d < 0) return vychozi;
    v = v * 16 + (uint32_t)d;
  }
  return v;
}

inline std::string orizni(const std::string &s) {
  size_t a = 0, b = s.size();
  while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) a++;
  while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) b--;
  return s.substr(a, b - a);
}

// Ceske znaky na velka pismena (jen to, co je potreba pro nazvy: a-z a hacky/carky)
inline std::string velka(const std::string &s) {
  static const char *MALA[] = {"á", "č", "ď", "é", "ě", "í", "ň", "ó", "ř", "š", "ť", "ú", "ů", "ý", "ž"};
  static const char *VEL[] = {"Á", "Č", "Ď", "É", "Ě", "Í", "Ň", "Ó", "Ř", "Š", "Ť", "Ú", "Ů", "Ý", "Ž"};
  std::string o;
  for (size_t i = 0; i < s.size();) {
    bool hotovo = false;
    for (int k = 0; k < 15; k++) {
      const size_t n = std::strlen(MALA[k]);
      if (s.compare(i, n, MALA[k]) == 0) { o += VEL[k]; i += n; hotovo = true; break; }
    }
    if (hotovo) continue;
    const char c = s[i];
    o += (c >= 'a' && c <= 'z') ? (char)(c - 32) : c;
    i++;
  }
  return o;
}

// Prectu karty z textu karty.txt. Do "chyby" (kdyz neni nullptr) pise varovani.
inline std::vector<Karta> nactiKarty(const char *text, size_t n, std::string *chyby = nullptr) {
  std::vector<Karta> v;
  if (!text) return v;
  size_t i = 0;
  int cisloRadku = 0;
  // UTF-8 BOM na zacatku souboru
  if (n >= 3 && (unsigned char)text[0] == 0xEF && (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF) i = 3;
  while (i < n) {
    size_t j = i;
    while (j < n && text[j] != '\n') j++;
    std::string radek(text + i, j - i);
    i = j + 1;
    cisloRadku++;
    radek = orizni(radek);
    if (radek.empty() || radek[0] == '#') continue;
    std::vector<std::string> pole;
    size_t a = 0;
    while (true) {
      const size_t b = radek.find('|', a);
      pole.push_back(orizni(radek.substr(a, b == std::string::npos ? std::string::npos : b - a)));
      if (b == std::string::npos) break;
      a = b + 1;
    }
    while (pole.size() < 10) pole.push_back("");
    Karta k;
    k.id = pole[0];
    k.nazev = pole[1];
    k.podtitul = pole[2];
    k.popis = pole[3];
    k.obrazek = pole[4];
    k.cil = pole[5];
    k.barva = barvaZHex(pole[6], 0xF2B640);
    k.zamekKlic = pole[7];
    k.efekt = pole[8];
    k.tlacitko = pole[9];
    if (k.id.empty()) {
      if (chyby) *chyby += "radek " + std::to_string(cisloRadku) + ": chybi id - preskakuji\n";
      continue;
    }
    if (k.cil.empty() && chyby) *chyby += "radek " + std::to_string(cisloRadku) + " (" + k.id + "): chybi cil - karta nikam nevede\n";
    if (!pole[6].empty() && barvaZHex(pole[6], 0x1000000) == 0x1000000 && chyby)
      *chyby += "radek " + std::to_string(cisloRadku) + " (" + k.id + "): spatna barva '" + pole[6] + "'\n";
    if (k.nazev.empty()) k.nazev = velka(k.id);
    if (k.tlacitko.empty()) k.tlacitko = "OTEVŘÍT";
    // stejne id dvakrat - druhe dostane priponu (zamky a log se nesmi plest)
    for (const Karta &o : v)
      if (o.id == k.id) {
        if (chyby) *chyby += "radek " + std::to_string(cisloRadku) + ": id '" + k.id + "' uz je - prejmenovano\n";
        k.id += "_" + std::to_string(cisloRadku);
        break;
      }
    v.push_back(k);
  }
  return v;
}

// Kdyby soubor v appce chybel nebo byl prazdny: menu musi fungovat vzdycky.
// (Stejny obsah jako assets/menu/karty.txt v B311.)
static const char *KARTY_ZALOHA =
    "atari | ATARI 130XE | EMULÁTOR · C++ | Atari 130XE s grafikou VBXE, celé v C++. | menu/karta_atari.jpg | "
    "emu_atari_cpp/index.html | #F2B640 | | 3d:atari | SPUSTIT\n"
    "sega | SEGA MEGA DRIVE | 16-BIT KONZOLE | 16bitové hry. | menu/karta_sega.jpg | emu_sega/index.html | #3FA9FF | sega | 3d:nap16 | SPUSTIT\n"
    "ps1 | PS1 | PLAYSTATION · 32-BIT | 32bitové hry. | menu/karta_ps1.jpg | emu_ps1/index.html | #B26BFF | ps1 | 3d:nap32 | SPUSTIT\n"
    "mp3 | MP3 PŘEHRÁVAČ | RETRO CASSETTE PLAYER | Hudba z telefonu. | menu/karta_mp3.jpg | player/index.html | #FFB347 | | fx:kazeta | PŘEHRÁT\n"
    "dzubox | DŽUBOX | PRO DJ MIXER | DJ mixer. | menu/karta_dzubox.jpg | dj/index.html | #FF4FD8 | | fx:dzubox | ROZJET\n"
    "web | ATARIHELP.EU | OFICIÁLNÍ WEB | Web atarihelp.eu. | menu/karta_web.jpg | https://atarihelp.eu/ | #4FE0C0 | | fx:web | OTEVŘÍT\n"
    "help | HELP | NÁVOD A OTÁZKY | Úplný návod. | menu/karta_help.jpg | napoveda/index.html | #F2D27A | | fx:help | OTEVŘÍT\n";

}  // namespace napmenu
