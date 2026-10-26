// test_overeni/b314/test_jazyk_cpp.cpp
// ====================================================================
//  B314: JAZYK CZ / EN V C++ (bez GPU, bez Androidu)
//   1) karty menu: assets/menu/karty.txt ma u kazde karty 4 anglicke sloupce (11-14),
//      pouzijJazyk(en) prepne nazev / podtitul / popis / tlacitko a zpet presne na cesky,
//      prazdny anglicky sloupec = cesky text (tlacitko OPEN), stary soubor s 10 sloupci funguje,
//      KARTY_ZALOHA (kdyby soubor chybel) ma anglicke sloupce taky
//   2) pristroj Atari: tx(), stitek() (HRY -> GAMES ...), chybaKazety(), radkuText()
//      cesky beze zmeny, anglicky prelozene; prepinani za chodu (atomic)
//   g++ -std=c++17 -O2 -o test_jazyk_cpp test_jazyk_cpp.cpp && ./test_jazyk_cpp [ASSETS]
// ====================================================================
#include <cstdio>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "../../app/src/main/cpp/menu/nap_menu_karty.h"
#include "../../app/src/main/cpp/atari/nap_atari_jazyk.h"

using namespace napmenu;

static int kontrol = 0, chyb = 0;
static void over(bool ok, const std::string &co) {
  kontrol++;
  if (!ok) chyb++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
}

static bool maDiakritiku(const std::string &s) {
  // ceska pismena v UTF-8 zacinaji bajty 0xC3 (á é í ó ú ý) a 0xC4 / 0xC5 (č ď ě ň ř š ť ů ž)
  for (size_t i = 0; i + 1 < s.size(); i++) {
    const unsigned char c = (unsigned char)s[i];
    if (c == 0xC3 || c == 0xC4 || c == 0xC5) return true;
  }
  return false;
}

int main(int argc, char **argv) {
  const std::string assets = argc > 1 ? argv[1] : "../../app/src/main/assets";
  // ---------------- 1) karty menu ----------------
  std::ifstream f(assets + "/menu/karty.txt", std::ios::binary);
  std::stringstream ss;
  ss << f.rdbuf();
  const std::string text = ss.str();
  std::string chyby;
  std::vector<Karta> k = nactiKarty(text.data(), text.size(), &chyby);
  over(k.size() == 7 && chyby.empty(), "karty.txt: " + std::to_string(k.size()) + " karet, bez varovani" + (chyby.empty() ? "" : ": " + chyby));
  int plnych = 0;
  std::string seznam;
  for (const Karta &x : k) {
    bool vse = true;
    for (int j = 0; j < 4; j++) vse = vse && !x.en[j].empty() && !maDiakritiku(x.en[j]);
    if (vse) plnych++;
    seznam += " " + x.id + "=" + x.en[0] + "/" + x.en[3];
  }
  over(plnych == 7, "kazda karta ma 4 anglicke sloupce bez ceskych pismen:" + seznam);
  std::vector<Karta> en = k;
  pouzijJazyk(en, true);
  bool prepnuto = true;
  for (size_t i = 0; i < k.size(); i++)
    prepnuto = prepnuto && en[i].nazev == k[i].en[0] && en[i].podtitul == k[i].en[1] && en[i].popis == k[i].en[2] && en[i].tlacitko == k[i].en[3]
               && en[i].obrazek == k[i].obrazek && en[i].cil == k[i].cil && en[i].zamekKlic == k[i].zamekKlic;
  over(prepnuto, "pouzijJazyk(en): nazev, podtitul, popis a tlacitko anglicky; obrazek, cil, zamek a efekt beze zmeny");
  std::vector<Karta> zpet = en;
  pouzijJazyk(zpet, false);
  bool cesky = true;
  for (size_t i = 0; i < k.size(); i++)
    cesky = cesky && zpet[i].nazev == k[i].nazev && zpet[i].podtitul == k[i].podtitul && zpet[i].popis == k[i].popis && zpet[i].tlacitko == k[i].tlacitko;
  over(cesky, "pouzijJazyk(cs) po anglictine: presne puvodni ceske texty (" + zpet[3].nazev + ", " + zpet[3].tlacitko + ")");
  over(en[0].tlacitko == "START" && en[3].nazev == "MP3 PLAYER" && en[4].nazev == "JUKEBOX" && en[6].tlacitko == "OPEN GUIDE",
       "anglicke nazvy jako v navodu en.html: " + en[0].tlacitko + ", " + en[3].nazev + ", " + en[4].nazev + ", " + en[6].tlacitko);
  // stary soubor (10 sloupcu) a chybejici anglicky sloupec
  const char *stary = "a | ATARI | PODTITUL | Popis. | x.jpg | a.html | #F2B640 | | fx:x | SPUSTIT\n"
                      "b | BÉ | | Popis bé. | y.jpg | b.html | | | | \n"
                      "c | CÉ | POD | Popis. | z.jpg | c.html | | | | JEĎ | SEE | | Description. |\n";
  std::vector<Karta> s = nactiKarty(stary, std::strlen(stary));
  pouzijJazyk(s, true);
  over(s.size() == 3 && s[0].nazev == "ATARI" && s[0].tlacitko == "OPEN" && s[0].popis == "Popis." &&
       s[1].tlacitko == "OPEN" && s[1].nazev == u8"BÉ" && s[2].nazev == "SEE" && s[2].podtitul == "POD" &&
       s[2].popis == "Description." && s[2].tlacitko == "OPEN",
       "bez anglickych sloupcu: cesky text a tlacitko OPEN; jen cast sloupcu: zbytek cesky (" + s[2].nazev + " / " + s[2].podtitul + ")");
  pouzijJazyk(s, false);
  over(s[0].tlacitko == "SPUSTIT" && s[1].tlacitko == u8"OTEVŘÍT" && s[2].tlacitko == u8"JEĎ", "zpet cesky: SPUSTIT / OTEVŘÍT (prazdne) / JEĎ");
  std::vector<Karta> z = nactiKarty(KARTY_ZALOHA, std::strlen(KARTY_ZALOHA));
  int zEn = 0;
  for (const Karta &x : z) if (!x.en[0].empty() && !x.en[3].empty()) zEn++;
  over(z.size() == 7 && zEn == 7, "KARTY_ZALOHA (soubor v appce chybi): 7 karet, kazda s anglickym nazvem a tlacitkem");
  // stejne id a obrazky jako soubor
  bool stejne = z.size() == k.size();
  for (size_t i = 0; stejne && i < z.size(); i++) stejne = z[i].id == k[i].id && z[i].cil == k[i].cil && z[i].zamekKlic == k[i].zamekKlic;
  over(stejne, "KARTY_ZALOHA: stejne karty, cile a zamky jako karty.txt");

  // ---------------- 2) pristroj Atari ----------------
  using namespace nap;
  atariJazykEn().store(false);
  over(std::string(tx("ZAPNUTO", "ON")) == "ZAPNUTO" && std::string(stitek("HRY")) == "HRY" && std::string(stitek("MOBIL")) == "MOBIL" &&
       chybaKazety("WAV je prilis kratky") == "WAV je prilis kratky" && radkuText(1, true) == u8"1 ŘÁDEK" &&
       radkuText(3, true) == u8"3 ŘÁDKY" && radkuText(45, false) == u8"45 řádků",
       "Atari cesky beze zmeny: HRY, MOBIL, 1 ŘÁDEK, 3 ŘÁDKY, 45 řádků");
  atariJazykEn().store(true);
  over(std::string(tx("ZAPNUTO", "ON")) == "ON" && std::string(stitek("HRY")) == "GAMES" && std::string(stitek("MOBIL")) == "PHONE" &&
       std::string(stitek("CHYBA")) == "ERROR" && std::string(stitek("MEZERA")) == "SPACE" && std::string(stitek("NET")) == "NET" &&
       std::string(stitek("XEX")) == "XEX",
       "Atari anglicky: stitky NET / GAMES, XEX / PHONE, LOG / ERROR, SPACE (ostatni beze zmeny)");
  over(chybaKazety("WAV neni PCM (format 3)") == "WAV is not PCM (format 3)" &&
       chybaKazety("WAV je ticho - zadny signal kazety") == "WAV is silent - no tape signal" &&
       chybaKazety("neco jineho") == "neco jineho",
       "Atari anglicky: chyby kazety (i s pokracovanim textu)");
  over(radkuText(1, true) == "1 LINE" && radkuText(2, true) == "2 LINES" && radkuText(45, false) == "45 LINES",
       "Atari anglicky: 1 LINE, 2 LINES, 45 LINES");
  atariJazykEn().store(false);
  over(std::string(stitek("HRY")) == "HRY", "Atari zpet cesky za chodu (devCtl jazyk:cs)");
  std::printf("VYSLEDEK: %d kontrol, %d CHYB\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
