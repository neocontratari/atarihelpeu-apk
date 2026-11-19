// test_overeni/b320/test_b320_ovladac.cpp
// ====================================================================
//  B320: HERNI OVLADAC (Retroid Pocket 6, gamepad) - logika v C++ (nap_ovladac.h) a analogova
//  packa jako mys PlayStation (nap_ps1_mys.h). Vse presne ze zdrojaku appky.
//
//  Kontroly:
//   A) klavesy Androidu -> logicka tlacitka (podle mista na ovladaci), drzeni / pusteni, uhlopricky
//   B) osy: krizek HAT, leva packa jako smer s hysterezi (zadne cukani), mrtva zona (zadne
//      driftovani), spouste L2 / R2 analogove s hysterezi, maska (ovladac bez osy = nic), NaN
//   C) PS1: tlacitka = STEJNA cisla RetroPad jako dotyk na strance (MainActivity.ps1ButtonId -
//      precte se ze zdrojaku): spodni KRIZEK, prave KOLECKO, leve CTVEREC, horni TROJUHELNIK.
//      Srovnani s mapovanim do B319 (git 0f80608 napTlacitkoPodleKlavesy) - tam bylo B = CTVEREC,
//      X = SELECT, Y = START, START = TROJUHELNIK, SELECT = KOLECKO
//   D) Sega: leve A, spodni B, prave C (Genesis Plus GX), START; bity jako g_input jadra Segy
//   E) Atari: stejne jako Java do B319 (napAtariBitPodleKlavesy z gitu 0f80608) + nic navic spatne
//   F) mys PlayStation analogovou packou: rychlost podle vychylky (krivka na druhou), citlivost
//      odmocninou, prava packa vzdy, leva jen s mysi samotnou v portu 1 (vic vychylena vyhrava),
//      s ovladacem v portu 2 leva packa ridi ovladac; tlacitka ovladace = tlacitka mysi (PORT1)
//
//   g++ -std=c++17 -O2 -Wall -Wextra -pthread -I../../app/src/main/cpp -o test_b320_ovladac test_b320_ovladac.cpp
//   ./test_b320_ovladac REPO
// ====================================================================
#include <cmath>
#include <cstdio>
#include <fstream>
#include <map>
#include <regex>
#include <sstream>
#include <string>

#include "nap_ovladac.h"
#include "nap_ps1_mys.h"

using namespace nap::ovladac;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string t(long long v) { return std::to_string(v); }
static std::string f2(double v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); return b; }
static std::string hx(unsigned v) { char b[16]; snprintf(b, sizeof(b), "0x%X", v); return b; }

static std::string cti(const std::string &p) {
  std::ifstream f(p);
  std::stringstream s;
  s << f.rdbuf();
  return s.str();
}

// osy: o[8] a maska
static void osa(Stav &s, int maska, float lx, float ly, float rx = 0, float ry = 0, float hx_ = 0, float hy = 0, float l2 = 0, float r2 = 0) {
  const float o[O_POCET] = {lx, ly, rx, ry, hx_, hy, l2, r2};
  osy(s, o, maska);
}

// PS1 RetroPad -> jmeno (jako ps1_tlacitka_text v nap_ps1_native.cpp)
static const char *PS1J[16] = {"KRIZEK", "CTVEREC", "SELECT", "START", "NAHORU", "DOLU", "VLEVO", "VPRAVO",
                               "KOLECKO", "TROJUHELNIK", "L1", "R1", "L2", "R2", "L3", "R3"};
static std::string ps1Text(uint32_t b) {
  std::string s;
  for (int i = 0; i < 16; i++) if (b & (1u << i)) { if (!s.empty()) s += '+'; s += PS1J[i]; }
  return s.empty() ? "nic" : s;
}

int main(int argc, char **argv) {
  const std::string repo = argc > 1 ? argv[1] : "../..";
  std::printf("B320 - herni ovladac (Retroid Pocket 6, gamepad): nap_ovladac.h + packa jako mys PS1 (nap_ps1_mys.h)\n\n");

  // ================= A) klavesy =================
  {
    struct K { int kod; uint32_t t; const char *j; };
    const K KL[] = {{K_BUTTON_A, T_A, "BUTTON_A"}, {K_BUTTON_B, T_B, "BUTTON_B"}, {K_BUTTON_X, T_X, "BUTTON_X"}, {K_BUTTON_Y, T_Y, "BUTTON_Y"},
                    {K_BUTTON_C, T_C, "BUTTON_C"}, {K_BUTTON_Z, T_Z, "BUTTON_Z"}, {K_BUTTON_L1, T_L1, "L1"}, {K_BUTTON_R1, T_R1, "R1"},
                    {K_BUTTON_L2, T_L2, "L2"}, {K_BUTTON_R2, T_R2, "R2"}, {K_BUTTON_THUMBL, T_L3, "THUMBL"}, {K_BUTTON_THUMBR, T_R3, "THUMBR"},
                    {K_BUTTON_START, T_START, "START"}, {K_BUTTON_SELECT, T_SELECT, "SELECT"}, {K_BUTTON_MODE, T_MODE, "MODE"},
                    {K_DPAD_UP, T_NAHORU, "DPAD_UP"}, {K_DPAD_DOWN, T_DOLU, "DPAD_DOWN"}, {K_DPAD_LEFT, T_VLEVO, "DPAD_LEFT"},
                    {K_DPAD_RIGHT, T_VPRAVO, "DPAD_RIGHT"}, {K_DPAD_CENTER, T_A, "DPAD_CENTER"},
                    {K_DPAD_UP_LEFT, T_NAHORU | T_VLEVO, "UP_LEFT"}, {K_DPAD_DOWN_LEFT, T_DOLU | T_VLEVO, "DOWN_LEFT"},
                    {K_DPAD_UP_RIGHT, T_NAHORU | T_VPRAVO, "UP_RIGHT"}, {K_DPAD_DOWN_RIGHT, T_DOLU | T_VPRAVO, "DOWN_RIGHT"}};
    int spatne = 0;
    std::string chyby;
    for (const K &k : KL)
      if (tlacitkoZKlavesy(k.kod) != k.t) { spatne++; chyby += std::string(" ") + k.j; }
    // klavesy, ktere ovladac NENI (Java je pusti Androidu): ZPET, Enter, mezernik, hlasitost, BUTTON_1..16
    const int NE[] = {4, 66, 62, 24, 25, 3, 111, K_BUTTON_1, 195, K_BUTTON_16, 0, -1, 1000};
    int ne = 0;
    for (int k : NE) if (tlacitkoZKlavesy(k) != 0) { ne++; chyby += " ne:" + t(k); }
    over(spatne == 0 && ne == 0, "A1) " + t((long long)(sizeof(KL) / sizeof(KL[0]))) + " klaves ovladace -> logicka tlacitka podle mista (A spodni, B prave, X leve, Y horni), uhlopricky krizku; ZPET, Enter, hlasitost, BUTTON_1..16 nejsou tlacitka hry" + chyby);
    Stav s;
    const uint32_t r1 = klavesa(s, K_BUTTON_A, true);
    klavesa(s, K_BUTTON_B, true);
    const uint32_t obe = s.klavesy;
    klavesa(s, K_BUTTON_A, false);
    const uint32_t jenB = s.klavesy;
    const uint32_t r0 = klavesa(s, 4 /* KEYCODE_BACK */, true);
    klavesa(s, K_DPAD_UP_RIGHT, true);
    klavesa(s, K_DPAD_UP_RIGHT, false);
    over(r1 == T_A && obe == (T_A | T_B) && jenB == T_B && r0 == 0 && s.klavesy == T_B && s.klaves == 5,
         "A2) drzeni a pusteni: A+B drzeno " + hx(obe) + ", A pusteno -> " + hx(jenB) + ", ZPET vraci 0 (Java ji pusti dal), uhlopricka dolu i nahoru zpet");
    pustVse(s);
    over(s.vse() == 0 && s.klavesy == 0, "A3) pustVse (ovladac odpojen / odchod ze hry): nic nedrzi");
  }

  // ================= B) osy =================
  {
    Stav s;
    const int VSE = M_HAT | M_LEVA | M_PRAVA | M_SPOUSTE;
    osa(s, VSE, 0, 0, 0, 0, -1, 0);
    const uint32_t h1 = s.hat;
    osa(s, VSE, 0, 0, 0, 0, 1, 1);
    const uint32_t h2 = s.hat;
    osa(s, VSE, 0, 0, 0, 0, 0, -1);
    const uint32_t h3 = s.hat;
    osa(s, VSE, 0, 0, 0, 0, 0, 0);
    const uint32_t h4 = s.hat;
    osa(s, M_LEVA, 0, 0, 0, 0, -1, 0);       // ovladac bez HAT: osa se nepouzije
    over(h1 == T_VLEVO && h2 == (T_VPRAVO | T_DOLU) && h3 == T_NAHORU && h4 == 0 && s.hat == 0,
         "B1) krizek jako osy HAT: -1 vlevo, (1,1) vpravo+dolu, Y -1 nahoru (Android: zaporne = nahoru), 0 nic; bez HAT v masce nic");
    // leva packa: hystereze 0,50 / 0,35
    Stav p;
    osa(p, M_LEVA, 0.45f, 0);
    const uint32_t a = p.packa;
    osa(p, M_LEVA, 0.55f, 0);
    const uint32_t b = p.packa;
    osa(p, M_LEVA, 0.40f, 0);
    const uint32_t c = p.packa;
    osa(p, M_LEVA, 0.30f, 0);
    const uint32_t d = p.packa;
    osa(p, M_LEVA, -0.6f, -0.6f);
    const uint32_t e = p.packa;
    osa(p, M_LEVA, -0.36f, -0.2f);
    const uint32_t f = p.packa;
    over(a == 0 && b == T_VPRAVO && c == T_VPRAVO && d == 0 && e == (T_VLEVO | T_NAHORU) && f == T_VLEVO,
         "B2) leva packa jako smer s hysterezi: 0,45 nic, 0,55 vpravo, zpet na 0,40 porad vpravo (zadne cukani), 0,30 nic; sikmo (-0,6,-0,6) vlevo+nahoru, (-0,36,-0,2) jen vlevo");
    // mrtva zona: 0,2 - kolem stredu nic, od okraje zony plynule, plna vychylka 1
    Stav m;
    osa(m, M_LEVA | M_PRAVA, 0.15f, 0.1f, 1.0f, 0.0f);
    const float lx0 = m.lx, ly0 = m.ly, rx1 = m.rx;
    osa(m, M_LEVA | M_PRAVA, 0.6f, 0.0f, 0.0f, -0.21f);
    over(lx0 == 0 && ly0 == 0 && std::fabs(rx1 - 1.f) < 1e-6f && std::fabs(m.lx - 0.5f) < 1e-5f && std::fabs(m.ry + 0.0125f) < 1e-4f,
         "B3) mrtva zona 0,20: (0,15, 0,1) = 0 (packa v klidu nedriftuje), plna vychylka 1, 0,6 -> " + f2(m.lx) + ", -0,21 -> " + f2(m.ry) + " (plynule od okraje zony)");
    // spouste: analogove 0..1, tlacitko s hysterezi 0,50 / 0,35
    Stav sp;
    osa(sp, M_SPOUSTE, 0, 0, 0, 0, 0, 0, 0.49f, 0.9f);
    const uint32_t s1 = sp.spouste;
    osa(sp, M_SPOUSTE, 0, 0, 0, 0, 0, 0, 0.5f, 0.4f);
    const uint32_t s2 = sp.spouste;
    osa(sp, M_SPOUSTE, 0, 0, 0, 0, 0, 0, 0.36f, 0.34f);
    const uint32_t s3 = sp.spouste;
    osa(sp, 0, 0, 0, 0, 0, 0, 0, 1.0f, 1.0f);   // ovladac bez analogovych spousti (jen tlacitka L2 / R2)
    const uint32_t s4 = sp.spouste;
    over(s1 == T_R2 && s2 == (T_L2 | T_R2) && s3 == T_L2 && s4 == 0 && sp.l2 == 0,
         "B4) analogove spouste L2 / R2: 0,49 nic, 0,5 drzi, pusteni pod 0,35 (R2 0,34 pusteno, L2 0,36 drzi); bez spousti v masce nic");
    Stav n;
    const float nan = std::nanf("");
    osa(n, VSE, nan, nan, nan, nan, nan, nan, nan, nan);
    over(n.vse() == 0 && n.lx == 0 && n.ry == 0 && n.l2 == 0, "B5) NaN z ovladace (rozbita osa) = nic");
    // vse(): leva packa se smerem / bez (s mysi samotnou ridi leva packa kurzor, ne smer)
    Stav v;
    klavesa(v, K_BUTTON_X, true);
    osa(v, M_LEVA | M_SPOUSTE, 0.9f, 0, 0, 0, 0, 0, 0, 0.8f);
    over(v.vse(true) == (T_X | T_VPRAVO | T_R2) && v.vse(false) == (T_X | T_R2),
         "B6) vse(): klavesy + spouste + smer leve packy " + hx(v.vse(true)) + ", bez leve packy " + hx(v.vse(false)));
  }

  // ================= C) PS1 =================
  {
    // dotyk na strance PS1: MainActivity.ps1ButtonId - precist ze zdrojaku
    const std::string src = cti(repo + "/app/src/main/java/eu/atarihelp/emu10/MainActivity.java");
    std::map<std::string, int> dotyk;
    {
      const size_t i = src.find("private int ps1ButtonId(String button)");
      const std::string tel = i == std::string::npos ? "" : src.substr(i, 1600);
      std::regex r("\"([A-Z0-9]+)\"\\.equals\\(b\\)[^;]*return (\\d+);");
      for (std::sregex_iterator it(tel.begin(), tel.end(), r), e; it != e; ++it) {
        const std::string blok = it->str();
        std::regex r2("\"([A-Z0-9]+)\"\\.equals\\(b\\)");
        for (std::sregex_iterator j(blok.begin(), blok.end(), r2), e2; j != e2; ++j) dotyk[(*j)[1]] = std::stoi((*it)[2]);
      }
    }
    struct P { uint32_t t; const char *dotykJmeno; const char *popis; };
    const P PS[] = {{T_A, "CROSS", "spodni (A) = KRIZEK"},        {T_B, "CIRCLE", "prave (B) = KOLECKO"},
                    {T_X, "SQUARE", "leve (X) = CTVEREC"},        {T_Y, "TRIANGLE", "horni (Y) = TROJUHELNIK"},
                    {T_L1, "L1", "L1"},  {T_R1, "R1", "R1"},  {T_L2, "L2", "L2"},  {T_R2, "R2", "R2"},
                    {T_START, "START", "START"}, {T_SELECT, "SELECT", "SELECT"},
                    {T_NAHORU, "UP", "nahoru"}, {T_DOLU, "DOWN", "dolu"}, {T_VLEVO, "LEFT", "vlevo"}, {T_VPRAVO, "RIGHT", "vpravo"}};
    int shod = 0, n = 0;
    std::string chyby;
    for (const P &p : PS) {
      n++;
      auto it = dotyk.find(p.dotykJmeno);
      const uint32_t ocek = it == dotyk.end() ? 0xFFFFFFFFu : (1u << it->second);
      if (ps1Bity(p.t) == ocek) shod++;
      else chyby += std::string(" ") + p.popis + "=" + ps1Text(ps1Bity(p.t));
    }
    over(dotyk.size() >= 14 && shod == n,
         "C1) PS1: fyzicka tlacitka = stejna cisla RetroPad jako dotyk na strance (ps1ButtonId ze zdrojaku, " + t((long long)dotyk.size()) + " jmen): " + t(shod) + " z " + t(n) + chyby);
    // B355: HOME (MODE) = MENU (v appce TLACITKA OVLADACE, jadro nic) - do B354 SELECT; SELECT ma svoje tlacitko
    over(ps1Bity(T_L3) == (1u << 14) && ps1Bity(T_R3) == (1u << 15) && ps1Bity(T_MODE) == 0 && ps1Bity(T_C) == 1u && ps1Bity(T_Z) == (1u << 8),
         "C2) PS1: L3 / R3 (stisk packy) = 14 / 15, MODE = nic (B355: MENU), C = KRIZEK, Z = KOLECKO (ovladace ve stylu Segy)");
    // mapovani do B319 (git 0f80608): stejne klavesy -> co hra dostala tehdy a ted
    struct S { int kod; int b319; const char *j; };
    const S STARE[] = {{K_BUTTON_A, 0, "A"}, {K_BUTTON_B, 1, "B"}, {K_BUTTON_X, 2, "X"}, {K_BUTTON_Y, 3, "Y"}, {K_BUTTON_SELECT, 8, "SELECT"},
                       {K_BUTTON_START, 9, "START"}, {K_BUTTON_L1, 10, "L1"}, {K_BUTTON_R2, 13, "R2"}};
    std::string popis;
    int opraveno = 0, stejne = 0;
    for (const S &x : STARE) {
      const uint32_t ted = ps1Bity(tlacitkoZKlavesy(x.kod));
      const uint32_t tehdy = 1u << x.b319;
      if (ted != tehdy) { opraveno++; popis += std::string(" ") + x.j + ": " + PS1J[x.b319] + " -> " + ps1Text(ted) + ";"; }
      else stejne++;
    }
    over(opraveno == 5 && stejne == 3,
         "C3) oprava proti B319 (mapovani v Jave, git 0f80608):" + popis + " A, L1, R2 jako drive");
  }

  // ================= D) Sega =================
  {
    // poradi g_input v jadre Segy (nap_sega_native_proof.cpp): 0 U 1 D 2 L 3 R 4 A 5 B 6 C 7 S
    const std::string src = cti(repo + "/app/src/main/cpp/nap_sega_native_proof.cpp");
    const bool poradi = src.find("case CLOWNMDEMU_BUTTON_A: return nap_vstup(4)") != std::string::npos &&
                        src.find("case CLOWNMDEMU_BUTTON_B: return nap_vstup(5)") != std::string::npos &&
                        src.find("case CLOWNMDEMU_BUTTON_C: return nap_vstup(6)") != std::string::npos &&
                        src.find("case CLOWNMDEMU_BUTTON_START: return nap_vstup(7)") != std::string::npos &&
                        src.find("case CLOWNMDEMU_BUTTON_UP: return nap_vstup(0)") != std::string::npos;
    over(poradi && segaBity(T_X) == (1u << 4) && segaBity(T_A) == (1u << 5) && segaBity(T_B) == (1u << 6) && segaBity(T_START) == (1u << 7) &&
             segaBity(T_L1) == (1u << 4) && segaBity(T_R1) == (1u << 6) && segaBity(T_C) == (1u << 6) &&
             segaBity(T_NAHORU | T_VPRAVO) == ((1u << 0) | (1u << 3)) && segaBity(T_Y) == (1u << 9) && segaBity(T_SELECT) == 0,
         "D1) Sega: leve = A, spodni = B, prave = C (A B C v rade zleva jako Genesis Plus GX), L1 = A, R1 = C, START; bity = g_input jadra Segy (cteno ze zdrojaku); "
         "B355: horni = Y ovladace se 6 tlacitky (bit 9)");
  }

  // ================= E) Atari =================
  {
    // Java do B319 (git 0f80608 napAtariBitPodleKlavesy) - ovladac tam uz byl, musi zustat stejny
    struct A { int kod; uint32_t bit; const char *j; };
    const A STARE[] = {{K_DPAD_UP, 1, "nahoru"}, {K_DPAD_DOWN, 2, "dolu"}, {K_DPAD_LEFT, 4, "vlevo"}, {K_DPAD_RIGHT, 8, "vpravo"},
                       {K_DPAD_CENTER, 16, "stred"}, {K_BUTTON_A, 16, "A"}, {K_BUTTON_B, 16, "B"}, {K_BUTTON_L1, 16, "L1"}, {K_BUTTON_R1, 16, "R1"},
                       {K_BUTTON_START, 32, "START"}, {K_BUTTON_SELECT, 64, "SELECT"}, {K_BUTTON_L2, 128, "L2"}, {K_BUTTON_R2, 128, "R2"},
                       // B355: HOME (MODE) = MENU (do B354 OPTION - v Prince of Persia otevrel menu hry); klavesa ovladace
                       // je ted kod (KBCODE + 1 v bitech 8-16): X = MEZERA 33, Y = RETURN 12
                       {K_BUTTON_MODE, 0, "MODE"}, {K_BUTTON_X, (33u + 1) << 8, "X"}, {K_BUTTON_Y, (12u + 1) << 8, "Y"}};
    int shod = 0, n = 0;
    std::string chyby;
    for (const A &a : STARE) {
      n++;
      if (atariBity(tlacitkoZKlavesy(a.kod)) == a.bit) shod++;
      else chyby += std::string(" ") + a.j;
    }
    over(shod == n && atariBity(T_C) == 16 && atariBity(T_L3 | T_R3) == 0,
         "E1) Atari 130XE: " + t(shod) + " z " + t(n) + " tlacitek stejne jako Java do B319 (FIRE A/B/L1/R1, START, SELECT, OPTION L2/R2, X = MEZERA, Y = RETURN; "
         "B355: HOME = MENU, klavesa jako kod)" + chyby);
  }

  // ================= F) mys PlayStation packou =================
  {
    using nap::Ps1Mys;
    auto soucet = [](Ps1Mys &m, int snimku, uint32_t bity, int port, int &leve) {
      long long sx = 0, sy = 0;
      leve = 0;
      for (int i = 0; i < snimku; i++) {
        m.snimek(1000 + i * 16, bity);
        sx += m.mysDotaz((unsigned)port, Ps1Mys::ID_X);
        sy += m.mysDotaz((unsigned)port, Ps1Mys::ID_Y);
        if (m.mysDotaz((unsigned)port, Ps1Mys::ID_LEVE)) leve++;
        m.poSnimku(true);
      }
      return std::make_pair(sx, sy);
    };
    int lv = 0;
    Ps1Mys m;
    m.nastav(Ps1Mys::PORT1, 100);
    m.packy(0, 0, 1.0f, 0);                    // prava packa naplno vpravo
    auto p1 = soucet(m, 60, 0, 0, lv);
    m.packy(0, 0, 0, 0);
    auto dobeh = soucet(m, 8, 0, 0, lv);       // dojede, co bylo rozlozene do dalsich snimku
    const long long celkem = p1.first + dobeh.first;
    over(std::llabs(celkem - 360) <= 1 && p1.second == 0 && dobeh.first <= 18,
         "F1) prava packa naplno: 6 bodu mysi za snimek -> " + t(celkem) + " bodu za 60 snimku (1 s, ~1 sirka obrazu), po pusteni dojede jen " + t(dobeh.first));
    m.packy(0, 0, 0.5f, -0.5f);
    auto p2 = soucet(m, 60, 0, 0, lv);
    m.packy(0, 0, 0, 0);
    auto d2 = soucet(m, 8, 0, 0, lv);
    const double ocek2 = 60 * packaRychlost(std::hypot(0.5f, 0.5f), 100) / std::sqrt(2.0);
    over(std::fabs(p2.first + d2.first - ocek2) <= 2 && std::fabs(-(p2.second + d2.second) - ocek2) <= 2,
         "F2) sikmo napul (0,5, -0,5): krivka na druhou -> " + t(p2.first + d2.first) + " / " + t(p2.second + d2.second) + " bodu (vypocet " + f2(ocek2) + ")");
    Ps1Mys m4;
    m4.nastav(Ps1Mys::PORT1, 400);
    m4.packy(0, 0, 1.0f, 0);
    auto p4 = soucet(m4, 30, 0, 0, lv);
    over(p4.first >= 2 * 6 * 30 - 30 && p4.first <= 2 * 6 * 30 + 1,
         "F3) citlivost 400 %: packa jen 2x rychleji (odmocnina - packa nelita): " + t(p4.first) + " bodu za 30 snimku");
    // leva packa: s mysi samotnou v portu 1 ano (vic vychylena vyhrava), s ovladacem v portu 2 ne
    Ps1Mys m5;
    m5.nastav(Ps1Mys::PORT1, 100);
    m5.packy(0.8f, 0, 0.3f, 0);
    auto p5 = soucet(m5, 60, 0, 0, lv);
    Ps1Mys m6;
    m6.nastav(Ps1Mys::PORT1_OVL2, 100);
    m6.packy(0.8f, 0, 0, 0);
    auto p6 = soucet(m6, 60, 0, 0, lv);
    m6.packy(0.8f, 0, 0.3f, 0);
    auto p6b = soucet(m6, 60, 0, 0, lv);
    const double ocek5 = 60 * packaRychlost(0.8f, 100);
    over(std::fabs(p5.first - ocek5) <= 25 && p6.first == 0 && p6b.first > 0 && p6b.first < p5.first / 4,
         "F4) leva packa: mys samotna v portu 1 (Dune 2000) jede leva (vic vychylena) " + t(p5.first) + " bodu; s ovladacem v portu 2 leva mys nehybe (" +
             t(p6.first) + "), jen prava " + t(p6b.first));
    Ps1Mys m7;
    m7.nastav(Ps1Mys::VYP, 100);
    m7.packy(0, 0, 1, 1);
    auto p7 = soucet(m7, 30, 0, 0, lv);
    over(p7.first == 0 && p7.second == 0, "F5) mys vypnuta: packa mys nehybe (jde jako ovladac)");
    // tlacitka ovladace v rezimu mysi: KRIZEK = leve, D-pad = smer kurzoru (PORT1); v PORT1_OVL2 ne
    Ps1Mys m8;
    m8.nastav(Ps1Mys::PORT1, 100);
    int leve8 = 0;
    soucet(m8, 20, ps1Bity(T_A), 0, leve8);
    auto sm = soucet(m8, 20, ps1Bity(T_VPRAVO), 0, lv);
    Ps1Mys m9;
    m9.nastav(Ps1Mys::PORT1_OVL2, 100);
    int leve9 = 0;
    soucet(m9, 20, ps1Bity(T_A), 0, leve9);
    const int16_t pad9 = m9.padDotaz(1, 0, ps1Bity(T_A));
    over(leve8 >= 18 && sm.first > 20 && leve9 == 0 && pad9 == 1,
         "F6) mys v portu 1: spodni (KRIZEK) = leve tlacitko mysi (" + t(leve8) + " snimku), krizek vpravo = kurzor " + t(sm.first) +
             " bodu; mys + ovladac v portu 2: KRIZEK jde ovladaci v portu 2, ne mysi");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
