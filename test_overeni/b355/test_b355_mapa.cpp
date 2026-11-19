// test_overeni/b355/test_b355_mapa.cpp
// B355: MAPA TLACITEK HERNIHO OVLADACE (app/src/main/cpp/nap_ovladac.h) - ciste C++ bez Androidu.
// Rene: "musi byt v nastaveni u kazdeho emu co ktere tlacitko ma delat podle bt d-padu"; log 19. 11.: HOME
// (a tlacitko snimku Pro Controlleru = MODE) bylo v Atari OPTION - v Prince of Persia otevrelo menu hry.
//  A) vychozi mapy = chovani do B354 (Atari, Sega, PS1) krome zamernych zmen: HOME = MENU vsude,
//     Sega navic ovladac se 6 tlacitky (horni = Y, L2 = X, R2 = Z, Z = Z)
//  B) retezec mapy: zpet na stejny text, nezname radky / cile zustanou vychozi a spocitaji se
//  C) Atari: klavesa v bitech 8-16 (KBCODE + 1), BREAK bit 17, drzi-li se vic klaves plati posledni
//  D) katalog pro Javu: 13 radku, cile bez duplicit, kazdy token projde tam a zpet, D = vychozi mapa
//  E) scankody klaves = klavesnice pristroje (nap_atari_device.h KEYS) a standardni KBCODE Atari
// Zapise katalogy (katalog_<jadro>_<cs|en>.txt) pro JVM test jvm/TestB355Mapa (parser Javy na skutecnem vystupu).
// Preklad (z test_overeni/b355):
//   g++ -std=c++17 -O1 -Wall -Wextra -I../../app/src/main/cpp -o test_b355_mapa test_b355_mapa.cpp
//   ./test_b355_mapa [adresar pro katalogy]                -> VYSTUP_test_b355_mapa.txt
#include "nap_ovladac.h"
#include <cstdio>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace nap::ovladac;
static int ok = 0, chyb = 0;
static void over(bool b, const std::string &co) { (b ? ok : chyb)++; printf("%s  %s\n", b ? "OK   " : "CHYBA", co.c_str()); }
static std::string hx(uint32_t v) { char b[16]; snprintf(b, sizeof(b), "0x%05X", v); return b; }

// chovani do B354 (git ed9bf1f nap_ovladac.h) - opsane, at je videt, co se zmenilo
static uint32_t staraAtari(uint32_t t) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 0;
  if (t & T_DOLU) r |= 1u << 1;
  if (t & T_VLEVO) r |= 1u << 2;
  if (t & T_VPRAVO) r |= 1u << 3;
  if (t & (T_A | T_B | T_L1 | T_R1 | T_C)) r |= 1u << 4;
  if (t & T_START) r |= 1u << 5;
  if (t & T_SELECT) r |= 1u << 6;
  if (t & (T_MODE | T_L2 | T_R2)) r |= 1u << 7;
  if (t & T_X) r |= 1u << 8;                 // MEZERA (stare bity)
  if (t & T_Y) r |= 1u << 9;                 // RETURN
  return r;
}
static uint32_t staraSega(uint32_t t) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 0;
  if (t & T_DOLU) r |= 1u << 1;
  if (t & T_VLEVO) r |= 1u << 2;
  if (t & T_VPRAVO) r |= 1u << 3;
  if (t & (T_X | T_L1)) r |= 1u << 4;
  if (t & T_A) r |= 1u << 5;
  if (t & (T_B | T_C | T_R1)) r |= 1u << 6;
  if (t & T_START) r |= 1u << 7;
  return r;
}
static uint32_t staraPs1(uint32_t t) {
  uint32_t r = 0;
  if (t & (T_A | T_C)) r |= 1u << 0;
  if (t & T_X) r |= 1u << 1;
  if (t & (T_SELECT | T_MODE)) r |= 1u << 2;
  if (t & T_START) r |= 1u << 3;
  if (t & T_NAHORU) r |= 1u << 4;
  if (t & T_DOLU) r |= 1u << 5;
  if (t & T_VLEVO) r |= 1u << 6;
  if (t & T_VPRAVO) r |= 1u << 7;
  if (t & (T_B | T_Z)) r |= 1u << 8;
  if (t & T_Y) r |= 1u << 9;
  if (t & T_L1) r |= 1u << 10;
  if (t & T_R1) r |= 1u << 11;
  if (t & T_L2) r |= 1u << 12;
  if (t & T_R2) r |= 1u << 13;
  if (t & T_L3) r |= 1u << 14;
  if (t & T_R3) r |= 1u << 15;
  return r;
}
// stare bity Atari 8 / 9 (MEZERA / RETURN) -> novy kod klavesy (KBCODE + 1 v bitech 8-16)
static uint32_t staraAtariNovyZapis(uint32_t s) {
  uint32_t r = s & 0xFF;
  if (s & (1u << 8)) r |= (33u + 1) << 8;
  else if (s & (1u << 9)) r |= (12u + 1) << 8;
  return r;
}

int main(int argc, char **argv) {
  const std::string kam = argc > 1 ? argv[1] : ".";
  printf("B355 - mapa tlacitek herniho ovladace (nap_ovladac.h): vychozi mapy, retezec, Atari klavesy, katalog pro Javu\n");
  static const uint32_t VSE[] = {T_A, T_B, T_X, T_Y, T_L1, T_R1, T_L2, T_R2, T_L3, T_R3, T_START, T_SELECT, T_MODE,
                                 T_NAHORU, T_DOLU, T_VLEVO, T_VPRAVO, T_C, T_Z};
  // ---------- A) vychozi mapy ----------
  {
    std::string zmeny;
    int stejne = 0;
    for (uint32_t t : VSE) {
      const uint32_t n = atariBity(t), s = staraAtariNovyZapis(staraAtari(t));
      if (n == s) stejne++; else zmeny += " " + jmena(t) + ": " + hx(s) + " -> " + hx(n);
    }
    printf("Atari vychozi: %d z 19 stejne, zmeny:%s\n", stejne, zmeny.c_str());
    over(stejne == 18 && atariBity(T_MODE) == 0 && atariBity(T_X) == (34u << 8) && atariBity(T_Y) == (13u << 8) &&
             atariBity(T_A | T_NAHORU) == 0x11 && atariBity(T_L2) == 0x80 && atariBity(T_START | T_SELECT) == 0x60,
         "A1) Atari vychozi = do B354 (A B L1 R1 C = FIRE, START, SELECT, L2 R2 = OPTION, X = MEZERA, Y = RETURN); jen HOME uz neni OPTION (MENU)");
  }
  {
    std::string zmeny;
    int stejne = 0;
    for (uint32_t t : VSE) {
      const uint32_t n = segaBity(t), s = staraSega(t);
      if (n == s) stejne++; else zmeny += " " + jmena(t) + ": " + hx(s) + " -> " + hx(n);
    }
    printf("Sega vychozi: %d z 19 stejne, zmeny:%s\n", stejne, zmeny.c_str());
    over(stejne == 15 && segaBity(T_Y) == (1u << 9) && segaBity(T_L2) == (1u << 8) && segaBity(T_R2) == (1u << 10) &&
             segaBity(T_Z) == (1u << 10) && segaBity(T_SELECT) == 0 && segaBity(T_MODE) == 0,
         "A2) Sega vychozi = do B354 (leve A, spodni B, prave C, L1 A, R1 C, START); navic 6 tlacitek: horni Y, L2 X, R2 Z, Z = Z");
  }
  {
    std::string zmeny;
    int stejne = 0;
    for (uint32_t t : VSE) {
      const uint32_t n = ps1Bity(t), s = staraPs1(t);
      if (n == s) stejne++; else zmeny += " " + jmena(t) + ": " + hx(s) + " -> " + hx(n);
    }
    printf("PS1 vychozi: %d z 19 stejne, zmeny:%s\n", stejne, zmeny.c_str());
    over(stejne == 18 && ps1Bity(T_MODE) == 0 && ps1Bity(T_SELECT) == (1u << 2),
         "A3) PS1 vychozi = do B354 (stejne misto jako PlayStation); HOME uz neni SELECT (MENU) - SELECT ma svoje tlacitko");
  }
  {
    bool menu = true;
    for (int j = 0; j < J_POCET; j++) menu = menu && vychoziMapa(j).cil[R_MODE] == CIL_MENU;
    over(menu, "A4) HOME (MODE) = MENU ve vsech trech - v appce otevre TLACITKA OVLADACE, do jadra nic");
  }
  // ---------- B) retezec ----------
  {
    bool tamZpet = true;
    std::string chyba;
    for (int j = 0; j < J_POCET; j++) {
      const std::string s = mapaNaRetezec(vychoziMapa(j));
      int nez = -1;
      const Mapa m = mapaZRetezce(j, s, &nez);
      if (!stejnaMapa(m, vychoziMapa(j)) || nez != 0 || mapaNaRetezec(m) != s) { tamZpet = false; chyba += " jadro " + std::to_string(j); }
      printf("vychozi %d: %s\n", j, s.c_str());
    }
    int nez = -1;
    const Mapa prazdna = mapaZRetezce(J_SEGA, "", &nez);
    over(tamZpet && stejnaMapa(prazdna, vychoziMapa(J_SEGA)) && nez == 0, "B1) vychozi mapa -> text -> mapa beze zmeny (vsechna jadra); \"\" = vychozi" + chyba);
  }
  {
    int nez = -1;
    const Mapa m = mapaZRetezce(J_ATARI, "A=K10; B=BREAK,X=NAHORU;MODE=OPTION;L1=K17;Q=FIRE;Y=KX;R1=K300;START=;SELECT=SELECT", &nez);
    const std::string s = mapaNaRetezec(m);
    printf("vlastni Atari: %s (spatnych %d)\n", s.c_str(), nez);
    over(s == "A=K10;B=BREAK;X=NAHORU;Y=K12;L1=K17;R1=FIRE;L2=OPTION;R2=OPTION;L3=0;R3=0;START=START;SELECT=SELECT;MODE=OPTION" && nez == 4,
         "B2) vlastni mapa (oddelovac ; , mezera): A = klavesa P, B = BREAK, X = joystick nahoru, HOME = OPTION, L1 = HELP; "
         "neznamy radek Q, cil KX, K300 a prazdny START zustanou vychozi (4 spatne)");
    int nez2 = -1;
    const Mapa p = mapaZRetezce(J_PS1, "A=KOLECKO;B=KRIZEK;Y=FIRE;MODE=SELECT", &nez2);
    const Mapa sg = mapaZRetezce(J_SEGA, "A=A;B=B;X=C;START=MODE;Y=K33", &nez2);
    over(ps1Mapuj(T_A, p) == (1u << 8) && ps1Mapuj(T_B, p) == 1u && ps1Mapuj(T_Y, p) == ps1Bity(T_Y) && ps1Mapuj(T_MODE, p) == (1u << 2) &&
             segaMapuj(T_A, sg) == (1u << 4) && segaMapuj(T_X, sg) == (1u << 6) && segaMapuj(T_START, sg) == (1u << 11) && segaMapuj(T_Y, sg) == (1u << 9),
         "B3) PS1: prohozeny krizek a kolecko, HOME = SELECT; cil jineho jadra (Y=FIRE v PS1, Y=K33 v Seze) zustane vychozi");
  }
  // ---------- C) Atari klavesy, BREAK, posledni ----------
  {
    const Mapa m = mapaZRetezce(J_ATARI, "A=K10;L1=K17;X=K28;B=BREAK;Y=0");
    const uint32_t a = atariMapuj(T_A, m), al1 = atariMapuj(T_A | T_L1, m, R_L1), al1a = atariMapuj(T_A | T_L1, m, R_A),
                   bez = atariMapuj(T_A | T_L1, m), b = atariMapuj(T_B, m), y = atariMapuj(T_Y, m), x = atariMapuj(T_X | T_NAHORU, m);
    printf("A=%s A+L1 (posledni L1)=%s (posledni A)=%s (bez posledniho)=%s B=%s Y=%s X+nahoru=%s\n",
           hx(a).c_str(), hx(al1).c_str(), hx(al1a).c_str(), hx(bez).c_str(), hx(b).c_str(), hx(y).c_str(), hx(x).c_str());
    over(a == (11u << 8) && al1 == (18u << 8) && al1a == (11u << 8) && bez == (11u << 8) && b == AP_BREAK && y == 0 && x == ((29u << 8) | 1u),
         "C1) klavesa = KBCODE + 1 v bitech 8-16 (P = 10, HELP = 17, ESC = 28); drzi-li se dve, plati naposled stisknuta; BREAK bit 17; Y = nic");
    const Mapa k = mapaZRetezce(J_ATARI, "A=K255");
    over(atariMapuj(T_A, k) == (256u << 8) && (atariMapuj(T_A, k) & AP_KLAVESA_MASKA) == atariMapuj(T_A, k) && !(AP_KLAVESA_MASKA & AP_BREAK) &&
             !(AP_KLAVESA_MASKA & 0xFFu),
         "C2) nejvyssi KBCODE 255 (SHIFT + CONTROL) se vejde do bitu 8-16 a nesaha na BREAK ani na joystick");
    const Mapa menu = vychoziMapa(J_ATARI);
    over(atariMapuj(T_MODE, menu) == 0 && segaMapuj(T_MODE, vychoziMapa(J_SEGA)) == 0 && ps1Mapuj(T_MODE, vychoziMapa(J_PS1)) == 0,
         "C3) cil MENU jadro nic neudela (dialog otevira Java)");
  }
  // ---------- D) katalog ----------
  {
    bool vse = true;
    std::string popis;
    for (int j = 0; j < J_POCET; j++)
      for (int en = 0; en < 2; en++) {
        const std::string k = katalog(j, en != 0);
        std::istringstream in(k);
        std::string r;
        int radku = 0, cilu = 0, d = 0;
        std::set<std::string> tokeny;
        bool tamZpet = true, jmena = true;
        std::string vychozi;
        while (std::getline(in, r)) {
          if (r.rfind("R\t", 0) == 0) radku++;
          else if (r.rfind("C\t", 0) == 0) {
            cilu++;
            const size_t t2 = r.find('\t', 2);
            const std::string tok = r.substr(2, t2 - 2), jm = t2 == std::string::npos ? "" : r.substr(t2 + 1);
            if (!tokeny.insert(tok).second) tamZpet = false;
            const int c = cilZTokenu(j, tok);
            if (c < 0 || tokenCile(j, (uint16_t)c) != tok) tamZpet = false;
            if (jm.empty() || jm == "?" || jmenoCile(j, (uint16_t)(c < 0 ? 0 : c), en != 0) != jm) jmena = false;
          } else if (r.rfind("D\t", 0) == 0) { d++; vychozi = r.substr(2); }
        }
        const bool v = radku == 13 && d == 1 && vychozi == mapaNaRetezec(vychoziMapa(j)) && tamZpet && jmena && tokeny.count("MENU") && tokeny.count("0");
        vse = vse && v;
        popis += " " + std::to_string(j) + (en ? "en" : "cs") + ":" + std::to_string(cilu) + (v ? "" : "!");
        std::ofstream(kam + "/katalog_" + std::to_string(j) + "_" + (en ? "en" : "cs") + ".txt") << k;
      }
    printf("katalog (jadro jazyk : cilu):%s\n", popis.c_str());
    over(vse, "D1) katalog vsech jader CZ / EN: 13 radku, cile bez duplicit (vc. MENU a nic), token -> cil -> token, jmena, D = vychozi mapa");
    const std::string kc = katalog(J_ATARI, false), ke = katalog(J_ATARI, true);
    over(kc.find("C\tK33\tklávesa MEZERA") != std::string::npos && ke.find("C\tK33\tkey SPACE") != std::string::npos &&
             kc.find("C\tK10\tklávesa P") != std::string::npos && kc.find("C\tK17\tHELP") != std::string::npos &&
             katalog(J_PS1, false).find(u8"C\tKRIZEK\t✕ křížek") != std::string::npos && katalog(J_SEGA, true).find("C\tZ\tZ (6-button pad)") != std::string::npos,
         "D2) jmena pro hrace: klavesa MEZERA / key SPACE, klavesa P, HELP, ✕ krizek, Z (6-button pad)");
  }
  // ---------- E) scankody ----------
  {
    // klavesnice pristroje (nap_atari_device.h KEYS: {x,y,w,h,scan,"l1","l2"})
    std::ifstream f("../../app/src/main/cpp/atari/nap_atari_device.h");
    std::stringstream ss; ss << f.rdbuf();
    const std::string h = ss.str();
    std::map<std::string, int> pristroj;
    const size_t z = h.find("static const KeyDef KEYS[57] = {"), kon = h.find("};", z);
    size_t i = z;
    while (z != std::string::npos && (i = h.find('{', i + 1)) != std::string::npos && i < kon) {
      const size_t e = h.find('}', i);
      const std::string pol = h.substr(i + 1, e - i - 1);
      std::vector<std::string> c;
      std::string cur;
      bool uv = false;
      for (size_t q = 0; q < pol.size(); q++) {
        const char ch = pol[q];
        if (ch == '\\' && q + 1 < pol.size()) { cur += ch; cur += pol[++q]; continue; }   // \" uvnitr stitku
        if (ch == '"') uv = !uv;
        if (ch == ',' && !uv) { c.push_back(cur); cur.clear(); } else cur += ch;
      }
      c.push_back(cur);
      if (c.size() == 7) {
        std::string l2 = c[6];
        if (l2.size() >= 2) l2 = l2.substr(1, l2.size() - 2);
        const int sc = std::stoi(c[4]);
        if (sc == 33) l2 = "SPACE";
        if (!l2.empty() && !pristroj.count(l2)) pristroj[l2] = sc;
      }
      i = e;
    }
    static const std::map<std::string, std::string> JM = {{"MEZERA", "SPACE"}, {"ESC", "Esc"}, {"TAB", "Tab"}, {"RETURN", "Return"},
                                                          {"DELETE", "BkSp"}, {"CAPS", "Caps"}, {"INVERSE", "Inverse"}};
    int shod = 0, n = 0;
    std::string chyby;
    for (const KlavesaInfo &k : ATARI_KLAVESY) {
      std::string l2 = JM.count(k.cz) ? JM.at(k.cz) : std::string(k.cz);
      if (l2 == "," || l2 == "/") continue;                     // na pristroji maji stitky "[ /" a "? /" - viz standard nize
      n++;
      if (pristroj.count(l2) && pristroj[l2] == k.scan) shod++;
      else chyby += " " + std::string(k.cz) + "=" + std::to_string(k.scan) + "(pristroj " + (pristroj.count(l2) ? std::to_string(pristroj[l2]) : "?") + ")";
    }
    printf("klavesy na pristroji: %zu, z mapy porovnano %d, shodnych %d%s\n", pristroj.size(), n, shod, chyby.c_str());
    // standardni KBCODE Atari 400/800/XL (Altirra Hardware Reference): , = $20, / = $26, HELP = $11
    const KlavesaInfo *carka = atariKlavesa(32), *lomitko = atariKlavesa(38);
    over(n >= 50 && shod == n && carka && std::string(carka->cz) == "," && lomitko && std::string(lomitko->cz) == "/" &&
             cilZTokenu(J_ATARI, "K17") == A_KLAVESA + 17,
         "E1) scankody klaves mapy = klavesnice pristroje (" + std::to_string(shod) + " z " + std::to_string(n) + "), carka $20, lomitko $26, HELP $11");
  }
  printf("\nVYSLEDEK: %d OK, %d CHYBA\n", ok, chyb);
  return chyb ? 1 : 0;
}
