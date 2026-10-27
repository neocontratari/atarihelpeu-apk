// B317: MYS PLAYSTATION - test logiky v C++ (app/src/main/cpp/nap_ps1_mys.h).
//
// Skutecna trida nap::Ps1Mys z appky (zadna kopie). Test hraje "jadro": pred kazdym
// snimkem zavola snimek() jako vlakno emulace, pak se pta mysDotaz() / padDotaz()
// jako PCSX behem retro_run. Dotyky posila jako stranka PS1 (body stranky, cas v ms).
//
//   g++ -std=c++17 -O2 -Wall -Wextra -Wshadow -pthread -o test_b317_mys test_b317_mys.cpp && ./test_b317_mys PRAC
//   ThreadSanitizer (dotyk / snimek / nastav / mys z PC z ruznych vlaken naraz):
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -DNAP_TSAN -o tsan_b317_mys test_b317_mys.cpp && ./tsan_b317_mys PRAC
#include "../../app/src/main/cpp/nap_ps1_mys.h"

#include <atomic>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using nap::Ps1Mys;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

// jeden snimek "jadra": co hra uvidi
struct Snimek { int x, y; bool l, p; };
static Snimek krok(Ps1Mys &m, long long t, uint32_t pad = 0) {
  m.snimek(t, pad);
  const unsigned port = (unsigned)(Ps1Mys::portMysi(m.rezim()) < 0 ? 0 : Ps1Mys::portMysi(m.rezim()));
  return Snimek{m.mysDotaz(port, Ps1Mys::ID_X), m.mysDotaz(port, Ps1Mys::ID_Y), m.mysDotaz(port, Ps1Mys::ID_LEVE) != 0,
                m.mysDotaz(port, Ps1Mys::ID_PRAVE) != 0};
}
// n snimku po 20 ms (PAL 50 Hz); soucet posunu, pocet snimku s L / P, kolik stisku (prechodu nahoru->dolu)
struct Souhrn { long sx = 0, sy = 0; int snL = 0, snP = 0, stiskL = 0, stiskP = 0, maxKrok = 0; std::string vzorL, vzorP; };
static Souhrn snimky(Ps1Mys &m, long long &t, int n, uint32_t pad = 0) {
  Souhrn s;
  bool predL = false, predP = false;
  for (int i = 0; i < n; i++) {
    t += 20;
    const Snimek k = krok(m, t, pad);
    s.sx += k.x; s.sy += k.y;
    if (std::abs(k.x) > s.maxKrok) s.maxKrok = std::abs(k.x);
    if (std::abs(k.y) > s.maxKrok) s.maxKrok = std::abs(k.y);
    if (k.l) s.snL++;
    if (k.p) s.snP++;
    if (k.l && !predL) s.stiskL++;
    if (k.p && !predP) s.stiskP++;
    s.vzorL += k.l ? 'L' : '.';
    s.vzorP += k.p ? 'P' : '.';
    predL = k.l; predP = k.p;
  }
  return s;
}
static std::string txt(long a) { return std::to_string(a); }

// obraz hry na strance (na sirku) 732 x 412 bodu
static const float W = 732.f, H = 412.f;

static void test_vychozi() {
  Ps1Mys m;
  over(m.rezim() == Ps1Mys::VYP && m.citlivost() == 100 && !m.zapnuta(), "vychozi: mys vypnuta, citlivost 100 %");
  unsigned d[2];
  Ps1Mys::zarizeni(Ps1Mys::VYP, d);
  over(d[0] == Ps1Mys::RD_JOYPAD && d[1] == Ps1Mys::RD_JOYPAD, "vypnuta: oba porty standardni ovladac (jako jadro po nacteni hry)");
  // s vypnutou mysi presne jako B316: tlacitka jen do portu 1, kazde id 0-15 projde
  bool vse = true;
  for (unsigned id = 0; id < 16; id++) {
    vse &= m.padDotaz(0, id, 1u << id) == 1;
    vse &= m.padDotaz(0, id, ~(1u << id) & 0xFFFFu) == 0;
    vse &= m.padDotaz(1, id, 0xFFFFu) == 0;
  }
  vse &= m.padDotaz(0, 16, 0xFFFFFFFFu) == 0;
  over(vse, "vypnuta: ovladac v portu 1 dostane vsech 16 tlacitek beze zmeny (KRIZEK i KOLECKO), port 2 nic");
  long long t = 1000;
  m.dotyk(Ps1Mys::DOLU, 1, 100, 100, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 1, 400, 100, W, H, t + 50);
  const int u = m.dotyk(Ps1Mys::NAHORU, 1, 400, 100, W, H, t + 80);
  const Souhrn s = snimky(m, t, 20, 1u << 0);
  over(u == 0 && s.sx == 0 && s.snL == 0 && m.stavBity() == 0, "vypnuta: dotyk na obraze ani KRIZEK mys nehybou");
  over(m.mysDotaz(0, Ps1Mys::ID_X) == 0 && m.mysDotaz(0, Ps1Mys::ID_LEVE) == 0, "vypnuta: jadro z mysi dostane same nuly");
  over(m.souhrn().empty(), "vypnuta a nic se nedelo: zadny radek MYS ZDRAVI");
}

static void test_nastaveni() {
  Ps1Mys m;
  over(m.nastav(1, 100) && m.rezim() == 1, "nastav(1): mys v portu 1 (zmena hlasena)");
  over(!m.nastav(1, 100), "stejne nastaveni znovu: zadna zmena");
  m.nastav(7, 10);
  over(m.rezim() == Ps1Mys::VYP && m.citlivost() == 25, "spatny rezim -> vypnuta, citlivost 10 % -> 25 %");
  m.nastav(-3, 9999);
  over(m.rezim() == Ps1Mys::VYP && m.citlivost() == 400, "rezim -3 -> vypnuta, citlivost 9999 % -> 400 %");
  const char *ocek[4] = {"vypnuta, ovladac v portu 1", "mys v portu 1, port 2 prazdny", "mys v portu 1, ovladac v portu 2",
                         "mys v portu 2, ovladac v portu 1"};
  bool p = true;
  for (int r = 0; r < 4; r++) p &= std::string(Ps1Mys::popisRezimu(r)) == ocek[r];
  over(p, "popisy rezimu do deniku");
}

static void test_porty() {
  struct { int r; unsigned d0, d1; int pm, po; } T[] = {
      {Ps1Mys::VYP, Ps1Mys::RD_JOYPAD, Ps1Mys::RD_JOYPAD, -1, 0},
      {Ps1Mys::PORT1, Ps1Mys::RD_PSE_MOUSE, Ps1Mys::RD_NONE, 0, -1},
      {Ps1Mys::PORT1_OVL2, Ps1Mys::RD_PSE_MOUSE, Ps1Mys::RD_JOYPAD, 0, 1},
      {Ps1Mys::PORT2_OVL1, Ps1Mys::RD_JOYPAD, Ps1Mys::RD_PSE_MOUSE, 1, 0},
  };
  bool ok = true;
  for (auto &x : T) {
    unsigned d[2];
    Ps1Mys::zarizeni(x.r, d);
    ok &= d[0] == x.d0 && d[1] == x.d1 && Ps1Mys::portMysi(x.r) == x.pm && Ps1Mys::portOvladace(x.r) == x.po;
  }
  over(ok, "4 rezimy: co se zapoji do portu 1 / 2 (mys PlayStation = RETRO_DEVICE_SUBCLASS(MOUSE,0) = 0x102)");
  over(Ps1Mys::RD_PSE_MOUSE == 0x102u, "kod mysi PlayStation 0x102 (jako RETRO_DEVICE_PSE_MOUSE v libretro.c)");

  Ps1Mys m;
  unsigned d[2] = {99, 99};
  m.poNacteniHry();
  over(!m.zmenaPortu(d), "po nacteni hry s vypnutou mysi: porty se nemeni (zustava standardni ovladac)");
  m.nastav(Ps1Mys::PORT1, 100);
  over(m.zmenaPortu(d) && d[0] == Ps1Mys::RD_PSE_MOUSE && d[1] == Ps1Mys::RD_NONE, "zapnuti: port 1 = mys, port 2 = nic");
  over(!m.zmenaPortu(d), "dalsi snimek: uz nic (zapojeno jednou)");
  m.nastav(Ps1Mys::PORT2_OVL1, 100);
  over(m.zmenaPortu(d) && d[0] == Ps1Mys::RD_JOYPAD && d[1] == Ps1Mys::RD_PSE_MOUSE, "prepnuti: port 1 = ovladac, port 2 = mys");
  m.poNacteniHry();   // nova hra: jadro zapojilo do vsech portu standardni ovladac
  over(m.zmenaPortu(d) && d[0] == Ps1Mys::RD_JOYPAD && d[1] == Ps1Mys::RD_PSE_MOUSE, "po nacteni dalsi hry se mys zapoji znovu");
  m.nastav(Ps1Mys::VYP, 100);
  over(m.zmenaPortu(d) && d[0] == Ps1Mys::RD_JOYPAD && d[1] == Ps1Mys::RD_JOYPAD, "vypnuti: zpet oba standardni ovladac");
  std::string log;
  for (auto &r : m.vezmiLog()) log += r + "\n";
  over(log.find("MYS: jadro - port 1 = mys PlayStation, port 2 = nic") != std::string::npos &&
           log.find("MYS: jadro - port 1 = ovladac, port 2 = mys PlayStation") != std::string::npos,
       "denik: kazda zmena portu (\"MYS: jadro - port 1 = mys PlayStation, port 2 = nic\")");
}

static void test_tah() {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  long long t = 10000;
  // tah o 10 % sirky obrazu doprava a 5 % dolu: 32 / 16 bodu mysi (cela sirka = 320)
  m.dotyk(Ps1Mys::DOLU, 7, 300, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 7, 310, 200, W, H, t + 16);   // 1,4 % - jeste tuknuti, nic
  Souhrn s0 = snimky(m, t, 1);
  over(s0.sx == 0 && s0.sy == 0, "pohyb do 3 % sirky: kurzor stoji (muze to byt tuknuti)");
  m.dotyk(Ps1Mys::POHYB, 7, 340, 210, W, H, t + 16);
  m.dotyk(Ps1Mys::POHYB, 7, 373.2f, 236.6f, W, H, t + 32);
  const int u = m.dotyk(Ps1Mys::NAHORU, 7, 373.2f, 236.6f, W, H, t + 48);
  Souhrn s = snimky(m, t, 10);
  over(s.sx == 32 && s.sy == 16, "tah 73,2 x 36,6 bodu (10 % x 5 % sirky) = posun 32 / 16 bodu mysi (je " + txt(s.sx) + " / " + txt(s.sy) + ")");
  over(u == 0 && s.snL == 0 && s.snP == 0, "tah neni klik (zadne tlacitko)");

  // pres celou sirku: 320 bodu, za snimek nejvys 127 (jako skutecna mys), nic se neztrati
  m.dotyk(Ps1Mys::DOLU, 8, 0, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 8, W, 200, W, H, t + 30);
  m.dotyk(Ps1Mys::NAHORU, 8, W, 200, W, H, t + 60);
  s = snimky(m, t, 6);
  over(s.sx == 320 && s.maxKrok == 127, "tah pres celou sirku = 320 bodu, za snimek nejvys 127 (127 + 127 + 66)");

  // citlivost: 200 % dvojnasob, 25 % ctvrtina
  m.nastav(Ps1Mys::PORT1, 200);
  m.dotyk(Ps1Mys::DOLU, 9, 100, 100, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 9, 100 + 73.2f, 100, W, H, t + 30);
  m.dotyk(Ps1Mys::NAHORU, 9, 100 + 73.2f, 100, W, H, t + 60);
  s = snimky(m, t, 3);
  m.nastav(Ps1Mys::PORT1, 25);
  m.dotyk(Ps1Mys::DOLU, 9, 100, 100, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 9, 100 + 73.2f, 100, W, H, t + 30);
  m.dotyk(Ps1Mys::NAHORU, 9, 100 + 73.2f, 100, W, H, t + 60);
  Souhrn s2 = snimky(m, t, 3);
  over(s.sx == 64 && s2.sx == 8, "citlivost 200 % = 64 bodu, 25 % = 8 bodu (stejny tah 10 % sirky)");

  // drobne pohyby (0,5 bodu mysi kazdy) se scitaji - nic se neztrati zaokrouhlenim
  m.nastav(Ps1Mys::PORT1, 100);
  m.dotyk(Ps1Mys::DOLU, 3, 100, 100, W, H, t);
  float x = 100;
  for (int i = 0; i < 30; i++) { x += 1; m.dotyk(Ps1Mys::POHYB, 3, x, 100, W, H, t + i); }   // prekroci 3 % (21,96 b.)
  long sx = 0;
  for (int i = 0; i < 400; i++) {
    x -= 1.14375f;   // 0,5 bodu mysi
    m.dotyk(Ps1Mys::POHYB, 3, x, 100, W, H, t + 40 + i);
    if (i % 4 == 3) sx += krok(m, t + 40 + i).x;
  }
  m.dotyk(Ps1Mys::NAHORU, 3, x, 100, W, H, t + 500);
  t += 600;
  s = snimky(m, t, 3);
  sx += s.sx;
  const long ocek = (long)std::lround((x - 100) * 320.0 / W);
  over(std::labs(sx - ocek) <= 1, "400 drobnych pohybu po 0,5 bodu: celkem " + txt(sx) + " bodu, ocekavano " + txt(ocek) + " (nic se neztratilo)");

  // tah dolu a doleva = zaporne hodnoty
  m.dotyk(Ps1Mys::DOLU, 4, 500, 300, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 4, 500 - 146.4f, 300 - 73.2f, W, H, t + 30);
  m.dotyk(Ps1Mys::NAHORU, 4, 500 - 146.4f, 300 - 73.2f, W, H, t + 50);
  s = snimky(m, t, 3);
  over(s.sx == -64 && s.sy == -32, "tah doleva nahoru = zaporny posun (-64 / -32)");
}

static void test_tuknuti() {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  long long t = 20000;
  m.dotyk(Ps1Mys::DOLU, 1, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 1, 205, 203, W, H, t + 40);   // chveni prstu 0,8 % sirky
  const int u = m.dotyk(Ps1Mys::NAHORU, 1, 205, 203, W, H, t + 110);
  Souhrn s = snimky(m, t, 15);
  over(u == Ps1Mys::KLIK_L, "tuknuti (0,11 s, chveni prstu): dotyk hlasi klik levym (stranka zavibruje)");
  over(s.stiskL == 1 && s.snL == Ps1Mys::KLIK_SNIMKU && s.sx == 0 && s.sy == 0 && s.snP == 0,
       "tuknuti = leve tlacitko 1x dole presne 5 snimku, kurzor stoji: " + s.vzorL);

  m.dotyk(Ps1Mys::DOLU, 2, 200, 200, W, H, t);
  const int u2 = m.dotyk(Ps1Mys::NAHORU, 2, 200, 200, W, H, t + 300);
  s = snimky(m, t, 10);
  over(u2 == 0 && s.snL == 0, "prst nahore az po 0,3 s: zadny klik (neni to tuknuti)");

  // dvojklik: dva kliky oddelene (5 dole, 4 nahore, 5 dole) - hra je nespoji
  m.dotyk(Ps1Mys::DOLU, 3, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::NAHORU, 3, 200, 200, W, H, t + 60);
  m.dotyk(Ps1Mys::DOLU, 4, 200, 200, W, H, t + 120);
  m.dotyk(Ps1Mys::NAHORU, 4, 200, 200, W, H, t + 170);
  s = snimky(m, t, 20);
  over(s.stiskL == 2 && s.vzorL.substr(0, 14) == "LLLLL....LLLLL", "dvojite tuknuti = 2 kliky: " + s.vzorL);

  // 10 rychlych tuknuti: fronta drzi nejvys 4 dalsi (zadne nekonecne dobihani)
  for (int i = 0; i < 10; i++) {
    m.dotyk(Ps1Mys::DOLU, 10 + i, 200, 200, W, H, t + i * 5);
    m.dotyk(Ps1Mys::NAHORU, 10 + i, 200, 200, W, H, t + i * 5 + 2);
  }
  s = snimky(m, t, 60);
  over(s.stiskL == 4, "10 tuknuti za 50 ms: nejvys 4 kliky ve fronte (" + txt(s.stiskL) + ")");
}

static void test_dva_prsty() {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  long long t = 30000;
  m.dotyk(Ps1Mys::DOLU, 1, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::DOLU, 2, 320, 210, W, H, t + 30);
  const int u1 = m.dotyk(Ps1Mys::NAHORU, 2, 320, 210, W, H, t + 120);
  const int u2 = m.dotyk(Ps1Mys::NAHORU, 1, 200, 200, W, H, t + 140);
  Souhrn s = snimky(m, t, 15);
  over(u1 == Ps1Mys::KLIK_P && u2 == 0, "tuknuti dvema prsty: klik pravym (hlaseno jednou)");
  over(s.stiskP == 1 && s.snP == Ps1Mys::KLIK_SNIMKU && s.snL == 0 && s.sx == 0,
       "dva prsty = prave tlacitko 5 snimku, leve ani posun ne: " + s.vzorP);

  // dva prsty, ale jeden tahne -> nic
  m.dotyk(Ps1Mys::DOLU, 3, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::DOLU, 4, 320, 210, W, H, t + 20);
  m.dotyk(Ps1Mys::POHYB, 4, 420, 260, W, H, t + 60);
  m.dotyk(Ps1Mys::NAHORU, 4, 420, 260, W, H, t + 100);
  m.dotyk(Ps1Mys::NAHORU, 3, 200, 200, W, H, t + 110);
  s = snimky(m, t, 10);
  over(s.snP == 0 && s.snL == 0 && s.sx == 0, "dva prsty a jeden tahne (posun 2 prsty): zadny klik, kurzor stoji");

  // tah jednim, druhy prst prilozen behem tahu -> neni to pravy klik, tah pokracuje
  m.dotyk(Ps1Mys::DOLU, 5, 100, 100, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 5, 200, 100, W, H, t + 20);
  m.dotyk(Ps1Mys::DOLU, 6, 500, 300, W, H, t + 40);
  m.dotyk(Ps1Mys::NAHORU, 6, 500, 300, W, H, t + 80);
  m.dotyk(Ps1Mys::POHYB, 5, 300, 100, W, H, t + 100);
  m.dotyk(Ps1Mys::NAHORU, 5, 300, 100, W, H, t + 120);
  s = snimky(m, t, 5);
  over(s.snP == 0 && s.snL == 0 && s.sx == (long)std::lround(200 * 320.0 / W),
       "tah + kratky dotyk druhym prstem: zadny klik, tah pokracuje (" + txt(s.sx) + " bodu)");
}

static void test_drzeni() {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  long long t = 40000;
  m.dotyk(Ps1Mys::DOLU, 1, 200, 200, W, H, t);
  Souhrn s = snimky(m, t, 20);    // 0,4 s
  over(s.snL == 0 && (m.stavBity() & 2) == 0, "prst na miste 0,4 s: tlacitko jeste ne");
  s = snimky(m, t, 6);            // 0,52 s
  over(s.snL > 0 && (m.stavBity() & 2) != 0, "prst na miste 0,5 s: leve tlacitko DRZI (stav bit1 pro stranku)");
  m.dotyk(Ps1Mys::POHYB, 1, 273.2f, 236.6f, W, H, t + 10);
  s = snimky(m, t, 5);
  over(s.snL == 5 && s.sx == 32 && s.sy == 16, "tah s drzenym levym = vyber ramem (posun 32 / 16, tlacitko porad dole)");
  const int u = m.dotyk(Ps1Mys::NAHORU, 1, 273.2f, 236.6f, W, H, t + 10);
  s = snimky(m, t, 5);
  over(u == 0 && s.snL == 0 && (m.stavBity() & 2) == 0, "prst nahoru: leve tlacitko pusteno, zadny klik navic");

  // podrzeni a ZRUSENO (system dotyk zrusil)
  m.dotyk(Ps1Mys::DOLU, 2, 200, 200, W, H, t);
  snimky(m, t, 30);
  m.dotyk(Ps1Mys::ZRUSENO, 2, 200, 200, W, H, t);
  s = snimky(m, t, 5);
  over(s.snL == 0 && m.stavBity() == 1, "dotyk zrusen behem drzeni: tlacitko pusteno, zadny prst");
  bool vLogu = false;
  for (auto &r : m.vezmiLog()) vLogu |= r.find("dotyk zrusen - leve tlacitko pusteno") != std::string::npos;
  over(vLogu, "denik: \"MYS: dotyk zrusen - leve tlacitko pusteno\"");

  // drzeni se zmenou rezimu: vse pusteno
  m.dotyk(Ps1Mys::DOLU, 3, 200, 200, W, H, t);
  snimky(m, t, 30);
  m.nastav(Ps1Mys::PORT1_OVL2, 100);
  s = snimky(m, t, 3);
  over(s.snL == 0 && m.stavBity() == 1, "zmena rezimu behem drzeni: tlacitko pusteno");
}

static void test_tlacitka_a_smer() {
  const uint32_t KRIZEK = Ps1Mys::BIT_KRIZEK, KOLECKO = Ps1Mys::BIT_KOLECKO, VPRAVO = Ps1Mys::BIT_VPRAVO,
                 VLEVO = Ps1Mys::BIT_VLEVO, NAHORU = Ps1Mys::BIT_NAHORU, DOLU = Ps1Mys::BIT_DOLU;
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  long long t = 50000;
  Souhrn s = snimky(m, t, 10, KRIZEK);
  Souhrn s2 = snimky(m, t, 7, KOLECKO);
  over(s.snL == 10 && s.snP == 0 && s2.snP == 7 && s2.snL == 0, "mys sama v portu 1: KRIZEK drzi leve, KOLECKO prave tlacitko");
  bool zadny = true;
  for (unsigned port = 0; port < 2; port++)
    for (unsigned id = 0; id < 16; id++) zadny &= m.padDotaz(port, id, 0xFFFFu) == 0;
  over(zadny, "mys sama v portu 1: ovladac neni zapojeny nikde (hra tlacitka nevidi)");

  // smer: 1 bod za snimek, drzenim az 6 (za 0,7 s)
  s = snimky(m, t, 1, VPRAVO);
  over(s.sx == 1 && s.sy == 0, "D-pad vpravo, prvni snimek: 1 bod (presne miritko)");
  s = snimky(m, t, 34, VPRAVO);   // do 0,7 s
  Souhrn s3 = snimky(m, t, 10, VPRAVO);
  over(s3.sx == 60 && s3.maxKrok == 6, "D-pad drzeny 0,7 s+: plna rychlost 6 bodu za snimek (" + txt(s3.sx) + " za 10 snimku)");
  // souhrn za prvnich 35 snimku: 1 + (1..6 linearne)
  double ocek = 0;
  for (int i = 0; i < 35; i++) { double u = std::min(1.0, i * 20 / 700.0); ocek += 1 + 5 * u; }
  over(std::fabs(1 + s.sx - ocek) <= 1.0, "zrychleni linearne za 0,7 s: " + txt(1 + s.sx) + " bodu za 35 snimku (vypocet " + txt((long)ocek) + ")");
  s = snimky(m, t, 3);
  s = snimky(m, t, 2, VLEVO | NAHORU);
  // 2 snimky po 1-1,06 bodu (+ zbytek do 0,5 z minula) - ne plna rychlost (12)
  over(s.sx <= -1 && s.sx >= -3 && s.sy <= -1 && s.sy >= -3,
       "po pusteni zase pomalu; sikmo vlevo nahoru 2 snimky = " + txt(s.sx) + " / " + txt(s.sy) + " (plnou rychlosti by bylo -12)");
  s = snimky(m, t, 10, NAHORU | DOLU);
  over(s.sy == 0 && s.sx == 0, "nahoru + dolu naraz: vyrusi se");
  m.nastav(Ps1Mys::PORT1, 50);
  s = snimky(m, t, 4, DOLU);
  over(s.sy == 2, "citlivost 50 %: smer 0,5 bodu za snimek (4 snimky = 2 body)");
  s = snimky(m, t, 5, KRIZEK | VPRAVO);
  over(s.snL == 5 && s.sx > 0, "KRIZEK + smer naraz: tazeni s drzenym levym (vyber ramem jen tlacitky / klavesnici TV)");

  // s ovladacem v druhem portu patri vsechna tlacitka ovladaci - mys je nebere
  m.nastav(Ps1Mys::PORT1_OVL2, 100);
  s = snimky(m, t, 10, KRIZEK | KOLECKO | VPRAVO);
  bool padOk = m.padDotaz(1, 0, KRIZEK) == 1 && m.padDotaz(1, 8, KOLECKO) == 1 && m.padDotaz(1, 7, VPRAVO) == 1 &&
               m.padDotaz(0, 0, KRIZEK) == 0;
  over(s.snL == 0 && s.snP == 0 && s.sx == 0 && padOk,
       "mys port 1 + ovladac port 2: KRIZEK / KOLECKO / smer jdou ovladaci v portu 2, mys stoji");
  m.nastav(Ps1Mys::PORT2_OVL1, 100);
  s = snimky(m, t, 5, KRIZEK);
  padOk = m.padDotaz(0, 0, KRIZEK) == 1 && m.padDotaz(1, 0, KRIZEK) == 0;
  m.dotyk(Ps1Mys::DOLU, 1, 10, 10, W, H, t);
  m.dotyk(Ps1Mys::NAHORU, 1, 10, 10, W, H, t + 50);
  Souhrn s4 = snimky(m, t, 8);
  over(s.snL == 0 && padOk && s4.stiskL == 1 && m.mysDotaz(0, Ps1Mys::ID_LEVE) == 0,
       "ovladac port 1 + mys port 2: tlacitka ovladaci v portu 1, tuknuti = klik mysi v portu 2 (port 1 z mysi nic)");
}

static void test_pc() {
  Ps1Mys m;
  long long t = 60000;
  m.pcMys(50, 50, 1, t);
  Souhrn s = snimky(m, t, 2);
  over(s.sx == 0 && s.snL == 0, "mys z PC s vypnutou mysi: nic");
  m.nastav(Ps1Mys::PORT1, 150);
  m.pcMys(40, -20, 1, t);
  s = snimky(m, t, 3);
  over(s.sx == 60 && s.sy == -30 && s.snL == 3, "mys z PC: posun x citlivost (40 x 150 % = 60), leve tlacitko drzi");
  m.pcMys(0, 0, 2, t);
  s = snimky(m, t, 3);
  over(s.snL == 0 && s.snP == 3, "mys z PC: prave tlacitko");
  s = snimky(m, t, 130);   // 2,6 s ticho
  Souhrn s2 = snimky(m, t, 3);
  bool vLogu = false;
  for (auto &r : m.vezmiLog()) vLogu |= r.find("mys z PC se 2,5 s neozvala - tlacitka pustena") != std::string::npos;
  over(s2.snP == 0 && vLogu, "prohlizec se odmlcel 2,5 s: tlacitka pustena + radek v deniku");
}

static void test_pamet(const std::string &prac) {
  const std::string soubor = prac + "/PS1_mys_pro_hry.txt";
  std::remove(soubor.c_str());
  Ps1Mys m;
  over(!m.nactiProHru(soubor, "SLES02250") && m.rezim() == Ps1Mys::VYP && m.citlivost() == 100 && m.klic() == "SLES02250",
       "hra bez zaznamu (zadny soubor): mys vypnuta, citlivost 100 %");
  m.nastav(Ps1Mys::PORT1, 140);
  over(m.ulozProHru(soubor), "ulozeni pro hru SLES02250");
  m.nactiProHru(soubor, "SCUS94163");
  over(m.rezim() == Ps1Mys::VYP && m.citlivost() == 100, "jina hra (SCUS94163): mys vypnuta - kazda hra zvlast");
  m.nastav(Ps1Mys::PORT2_OVL1, 80);
  m.ulozProHru(soubor);
  Ps1Mys m2;
  over(m2.nactiProHru(soubor, "SLES02250") && m2.rezim() == Ps1Mys::PORT1 && m2.citlivost() == 140,
       "znovu SLES02250 (nova instance = novy start appky): port 1, 140 %");
  over(m2.nactiProHru(soubor, "SCUS94163") && m2.rezim() == Ps1Mys::PORT2_OVL1 && m2.citlivost() == 80, "SCUS94163: port 2, 80 %");
  m2.nactiProHru(soubor, "SLES02250");
  m2.nastav(Ps1Mys::VYP, 140);
  m2.ulozProHru(soubor);
  Ps1Mys m3;
  over(m3.nactiProHru(soubor, "SLES02250") && m3.rezim() == Ps1Mys::VYP, "vypnuti se taky pamatuje (zaznam prepsan, ne pridan)");
  std::ifstream f(soubor);
  std::stringstream ss;
  ss << f.rdbuf();
  const std::string obsah = ss.str();
  int radku = 0;
  for (char c : obsah) radku += c == '\n';
  over(obsah.rfind("# B317", 0) == 0 && obsah.find("SLES02250\t0\t140\n") != std::string::npos &&
           obsah.find("SCUS94163\t3\t80\n") != std::string::npos && radku == 3,
       "soubor: hlavicka + 1 radek na hru (klic TAB rezim TAB citlivost)");
  // rozbity soubor: spatne radky se preskoci, nesmysly se opravi
  {
    std::ofstream o(soubor);
    o << "# x\nnesmysl\nA\t9\t9999\nB\t2\nC\t1\t5\n\t1\t100\nD\t2\t300\n";
  }
  Ps1Mys m4;
  bool ok = m4.nactiProHru(soubor, "A") && m4.rezim() == Ps1Mys::VYP && m4.citlivost() == 100;
  ok &= !m4.nactiProHru(soubor, "B");
  ok &= m4.nactiProHru(soubor, "C") && m4.rezim() == 1 && m4.citlivost() == 100;
  ok &= m4.nactiProHru(soubor, "D") && m4.rezim() == 2 && m4.citlivost() == 300;
  over(ok, "poskozeny soubor: radek bez citlivosti preskocen, rezim 9 -> vypnuta, citlivost 5 / 9999 -> 100 %");
  // nejvys 500 her (nejstarsi pryc)
  for (int i = 0; i < 520; i++) {
    m4.nactiProHru(soubor, "HRA" + std::to_string(i));
    m4.nastav(Ps1Mys::PORT1, 100 + i % 7);
    m4.ulozProHru(soubor);
  }
  std::ifstream f2(soubor);
  std::string r;
  int n = 0;
  bool je519 = false, je0 = false;
  while (std::getline(f2, r)) {
    if (r.empty() || r[0] == '#') continue;
    n++;
    je519 |= r.rfind("HRA519\t", 0) == 0;
    je0 |= r.rfind("HRA0\t", 0) == 0;
  }
  over(n == 500 && je519 && !je0, "nejvys 500 her v souboru (nejstarsi se zahodi): " + txt(n));
  over(!Ps1Mys().ulozProHru(soubor), "bez nacteni hry (zadny klic) se nic neuklada");
  over(!m4.ulozProHru(prac + "/neexistuje/x.txt"), "slozka neexistuje: ulozeni hlasi chybu (do deniku)");
  std::ifstream tmp(soubor + ".novy");
  over(!tmp.good(), "zapis pres docasny soubor a prejmenovani (zadny .novy nezustal)");
}

static void test_klic() {
  over(Ps1Mys::klicHry("SLES02250", "/x/Dune 2000 (E).cue") == "SLES02250", "klic hry = ID disku (SLES02250)");
  over(Ps1Mys::klicHry("", "/storage/PS1/Dune 2000 (E).bin") == "Dune 2000 (E).bin", "bez ID disku: jmeno souboru");
  over(Ps1Mys::klicHry(nullptr, "") == "BIOS", "bez disku: BIOS");
  over(Ps1Mys::klicHry("SLUS99999", "/hry/homebrew.exe") == "homebrew.exe" && Ps1Mys::klicHry("SLUS99999", "") == "SLUS99999",
       "zastupne ID jadra SLUS99999 (vic her ho ma): jmeno souboru");
  over(Ps1Mys::klicHry("AB\tC|D/E", "") == "ABCDE", "jen bezpecne znaky (TAB a | ne)");
  over(Ps1Mys::klicHry("", "C:\\hry\\a\tb.iso") == "ab.iso", "jmeno souboru i s obracenym lomitkem, bez TAB");
}

static void test_souhrn() {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  long long t = 70000;
  m.dotyk(Ps1Mys::DOLU, 1, 0, 0, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 1, 183, 0, W, H, t + 20);
  m.dotyk(Ps1Mys::NAHORU, 1, 183, 0, W, H, t + 40);
  m.dotyk(Ps1Mys::DOLU, 2, 0, 0, W, H, t + 100);
  m.dotyk(Ps1Mys::NAHORU, 2, 0, 0, W, H, t + 150);
  snimky(m, t, 20, Ps1Mys::BIT_VPRAVO);
  const std::string s = m.souhrn();
  over(s.find("mys v portu 1, citlivost 100 %: dotyku 2, posun ") == 0 && s.find("klik L 1 P 0") != std::string::npos &&
           s.find("smerem 20") != std::string::npos,
       "souhrn do MYS ZDRAVI: " + s);
  const std::string s2 = m.souhrn();
  over(s2.find("dotyku 0, posun 0 / 0 bodu") != std::string::npos, "dalsi okno zacina od nuly (zapnuta mys = radek vzdy)");
  m.dotyk(Ps1Mys::DOLU, 3, 0, 0, W, H, t + 200);
  m.dotyk(Ps1Mys::NAHORU, 3, 0, 0, W, H, t + 230);
  snimky(m, t, 3);
  m.poNacteniHry();
  over(m.souhrn().find("dotyku 0, posun 0 / 0 bodu, klik L 0") != std::string::npos,
       "nova hra (poNacteniHry): souhrn zacina od nuly - nic z minule hry");
  m.nastav(Ps1Mys::VYP, 100);
  m.souhrn();
  over(m.souhrn().empty(), "vypnuta a nic: prazdny souhrn");
  Ps1Mys m2;
  for (int i = 0; i < 80; i++) m2.nastav(1 + i % 3, 100);
  over(m2.vezmiLog().size() == 50 && m2.vezmiLog().empty(), "denik mysi: nejvys 50 radku ve fronte, vyzvednuti ji vyprazdni");
}

// soubezne jako v appce: stranka (dotyky), vlakno emulace (snimek + dotazy jadra), nastaveni, TV
static void test_vlakna(const std::string &prac) {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  std::atomic<bool> konec{false};
  std::atomic<long> dotazu{0};
  const std::string soubor = prac + "/PS1_mys_vlakna.txt";
  std::thread emu([&] {
    long long t = 0;
    while (!konec) {
      t += 20;
      m.snimek(t, (t / 200) % 3 == 0 ? Ps1Mys::BIT_KRIZEK | Ps1Mys::BIT_VPRAVO : 0);
      unsigned d[2];
      m.zmenaPortu(d);
      for (unsigned p = 0; p < 2; p++)
        for (unsigned id = 0; id < 4; id++) { m.mysDotaz(p, id); dotazu++; }
      for (unsigned id = 0; id < 16; id++) m.padDotaz(0, id, 0xFFFF);
      m.vezmiLog();
      m.souhrn();
    }
  });
  std::thread stranka([&] {
    for (int i = 0; i < 4000; i++) {
      const long long t = i * 3;
      m.dotyk(Ps1Mys::DOLU, i % 3, 10.f * (i % 50), 100, W, H, t);
      m.dotyk(Ps1Mys::POHYB, i % 3, 10.f * (i % 50) + 40, 120, W, H, t + 1);
      m.dotyk(i % 7 ? Ps1Mys::NAHORU : Ps1Mys::ZRUSENO, i % 3, 0, 0, W, H, t + 2);
      m.stavBity();
    }
  });
  std::thread nastaveni([&] {
    for (int i = 0; i < 300; i++) {
      m.nastav(i % 4, 50 + i % 100);
      m.nactiProHru(soubor, "HRA" + std::to_string(i % 5));
      m.ulozProHru(soubor);
      m.klic();
    }
  });
  std::thread tv([&] { for (int i = 0; i < 3000; i++) m.pcMys(i % 9 - 4, i % 5 - 2, i % 4, i); });
  stranka.join(); nastaveni.join(); tv.join();
  konec = true;
  emu.join();
  over(dotazu > 0, "soubezne: stranka, vlakno emulace, nastaveni + soubor, mys z PC - dobehlo (" + txt(dotazu.load()) + " dotazu jadra)");
}

int main(int argc, char **argv) {
  const std::string prac = argc > 1 ? argv[1] : ".";
  std::printf("B317 - mys PlayStation: logika v C++ (nap_ps1_mys.h)\n");
#ifndef NAP_TSAN
  test_vychozi();
  test_nastaveni();
  test_porty();
  test_tah();
  test_tuknuti();
  test_dva_prsty();
  test_drzeni();
  test_tlacitka_a_smer();
  test_pc();
  test_pamet(prac);
  test_klic();
  test_souhrn();
#endif
  test_vlakna(prac);
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
