// B318: MYS PLAYSTATION - jemnejsi touchpad. Test logiky v C++ (app/src/main/cpp/nap_ps1_mys.h).
//
// Rene (B317): "mys sice funguje ale je dost neomalena i pri nastaveni citlivosti na 300 je to
// divny". Skutecna trida nap::Ps1Mys z appky (zadna kopie). Test hraje "jadro": pred kazdym
// snimkem zavola snimek() jako vlakno emulace, po nem poSnimku() (novy obraz hry / stejny),
// a pta se mysDotaz() / padDotaz() jako PCSX behem retro_run. Dotyky posila jako stranka PS1
// (body stranky, cas v ms). "Hra" muze cist mys kazdy snimek, nebo jen kazdy 3. (jako Dune 2000,
// ktera kresli 20 obrazu za sekundu).
//
//   g++ -std=c++17 -O2 -Wall -Wextra -Wshadow -pthread -o test_b318_mys test_b318_mys.cpp && ./test_b318_mys PRAC
//   ThreadSanitizer (dotyk / snimek / poSnimku / nastaveni / mys z PC z ruznych vlaken naraz):
//   g++ -std=c++17 -O1 -g -fsanitize=thread -pthread -DNAP_TSAN -o tsan_b318_mys test_b318_mys.cpp && ./tsan_b318_mys PRAC
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
static std::string txt(long a) { return std::to_string(a); }
static std::string txtf(double a, int des = 2) { char b[32]; std::snprintf(b, sizeof(b), "%.*f", des, a); return b; }

// jeden snimek "jadra": co hra uvidi
struct Snimek { int x, y; bool l, p; };
static Snimek krok(Ps1Mys &m, long long t, uint32_t pad = 0, bool novyObraz = true) {
  m.snimek(t, pad);
  const unsigned port = (unsigned)(Ps1Mys::portMysi(m.rezim()) < 0 ? 0 : Ps1Mys::portMysi(m.rezim()));
  const Snimek s{m.mysDotaz(port, Ps1Mys::ID_X), m.mysDotaz(port, Ps1Mys::ID_Y), m.mysDotaz(port, Ps1Mys::ID_LEVE) != 0,
                 m.mysDotaz(port, Ps1Mys::ID_PRAVE) != 0};
  m.poSnimku(novyObraz);
  return s;
}
// n snimku po 20 ms (PAL 50 Hz); soucet posunu, pocet snimku s L / P, kolik stisku (prechodu nahoru->dolu)
struct Souhrn {
  long sx = 0, sy = 0;
  int snL = 0, snP = 0, stiskL = 0, stiskP = 0, maxKrok = 0, prvniL = -1, posledniPosun = -1;
  std::string vzorL, vzorP;
  std::vector<int> dx;
};
static Souhrn snimky(Ps1Mys &m, long long &t, int n, uint32_t pad = 0) {
  Souhrn s;
  bool predL = false, predP = false;
  for (int i = 0; i < n; i++) {
    t += 20;
    const Snimek k = krok(m, t, pad);
    s.sx += k.x; s.sy += k.y;
    s.dx.push_back(k.x);
    if (std::abs(k.x) > s.maxKrok) s.maxKrok = std::abs(k.x);
    if (std::abs(k.y) > s.maxKrok) s.maxKrok = std::abs(k.y);
    if (k.x || k.y) s.posledniPosun = i;
    if (k.l && s.prvniL < 0) s.prvniL = i;
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
static std::string log(Ps1Mys &m) { std::string r; for (auto &x : m.vezmiLog()) r += x + "\n"; return r; }

// obraz hry na strance (na sirku) 732 x 412 bodu; lano 1,5 % = 10,98 bodu
static const float W = 732.f, H = 412.f;
static const double LANO_B = 0.015 * 732.0;
static double body(double px, int citlivost = 100) { return px * 320.0 / W * citlivost / 100.0; }

// mys bez zrychleni (presne pocty bodu) v portu 1
static void zapni(Ps1Mys &m, int citlivost = 100, bool zrychleni = false) {
  m.nastav(Ps1Mys::PORT1, citlivost);
  m.nastavZrychleni(zrychleni);
  m.vezmiLog();
}

static void test_vychozi() {
  Ps1Mys m;
  over(m.rezim() == Ps1Mys::VYP && m.citlivost() == 100 && !m.zapnuta() && m.zrychleni(),
       "vychozi: mys vypnuta, citlivost 100 %, zrychleni zapnuto");
  unsigned d[2];
  Ps1Mys::zarizeni(Ps1Mys::VYP, d);
  over(d[0] == Ps1Mys::RD_JOYPAD && d[1] == Ps1Mys::RD_JOYPAD, "vypnuta: oba porty standardni ovladac (jako jadro po nacteni hry)");
  bool vse = true;
  for (unsigned id = 0; id < 16; id++) {
    vse &= m.padDotaz(0, id, 1u << id) == 1;
    vse &= m.padDotaz(0, id, ~(1u << id) & 0xFFFFu) == 0;
    vse &= m.padDotaz(1, id, 0xFFFFu) == 0;
  }
  vse &= m.padDotaz(0, 16, 0xFFFFFFFFu) == 0;
  over(vse, "vypnuta: ovladac v portu 1 dostane vsech 16 tlacitek beze zmeny (KRIZEK i KOLECKO), port 2 nic (jako B316 / B317)");
  long long t = 1000;
  m.dotyk(Ps1Mys::DOLU, 1, 100, 100, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 1, 400, 100, W, H, t + 50);
  const int u = m.dotyk(Ps1Mys::NAHORU, 1, 400, 100, W, H, t + 80);
  m.pcMys(5000, 5000, 1, t);
  const Souhrn s = snimky(m, t, 20, 1u << 0);
  over(u == 0 && s.sx == 0 && s.snL == 0 && m.stavBity() == Ps1Mys::STAV_ZRYCHLENI,
       "vypnuta: dotyk, mys z PC ani KRIZEK mys nehybou; stav pro stranku jen bit5 = zrychleni zapnuto (32)");
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
  m.vezmiLog();
  over(m.nastavZrychleni(false) && !m.zrychleni() && !m.nastavZrychleni(false) && m.nastavZrychleni(true) && m.zrychleni(),
       "zrychleni vypnout / zapnout (zmena hlasena jen pri zmene)");
  const std::string lg = log(m);
  over(lg.find("MYS: zrychleni VYPNUTO (kurzor jede presne jako prst)") != std::string::npos &&
           lg.find("MYS: zrychleni ZAPNUTO (pomaly tah presne, rychly dal)") != std::string::npos,
       "denik: \"MYS: zrychleni VYPNUTO ...\" a \"MYS: zrychleni ZAPNUTO ...\"");
  m.nastav(1, 100);
  m.nastavZrychleni(false);
  over((m.stavBity() & Ps1Mys::STAV_ZRYCHLENI) == 0 && m.stavBity() == 1, "stav pro stranku: zrychleni vypnuto = bit5 dole (1)");
}

// B318: zadny skok na zacatku tahu - B317 stal az do 3 % sirky a pak naraz poskocil o 3 %
static void test_lano() {
  Ps1Mys m;
  zapni(m);
  long long t = 10000;
  m.dotyk(Ps1Mys::DOLU, 7, 300, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 7, 310, 200, W, H, t + 16);   // 1,37 % - v lane
  Souhrn s0 = snimky(m, t, 3);
  over(s0.sx == 0 && s0.sy == 0, "prst 1,4 % sirky od dotyku (v lane): kurzor stoji (tuknuti zustane presne)");
  // prst po 1 bodu stranky za snimek dal: kurzor se rozjede plynule - zadny skok
  Souhrn s;
  float x = 310;
  for (int i = 0; i < 40; i++) {
    x += 1;
    m.dotyk(Ps1Mys::POHYB, 7, x, 200, W, H, t + 5);
    const Souhrn k = snimky(m, t, 1);
    s.sx += k.sx;
    if (k.maxKrok > s.maxKrok) s.maxKrok = k.maxKrok;
  }
  m.dotyk(Ps1Mys::NAHORU, 7, x, 200, W, H, t + 5);
  const Souhrn konec = snimky(m, t, 5);
  s.sx += konec.sx;
  const long ocek = std::lround(body(x - 300 - LANO_B));
  over(s.maxKrok <= 1, "prst po 1 bodu za snimek pres hranici lana: kurzor nejvys 1 bod za snimek - zadny skok (B317: skok o 10 bodu)");
  over(std::labs(s.sx - ocek) <= 1, "tah " + txtf(x - 300, 0) + " bodu stranky: kurzor " + txt(s.sx) + " bodu = vse za lanem (" + txt(ocek) + ")");

  // klasicky tah 10 % sirky ZA lanem = 32 bodu, 5 % dolu = 16 (rozlozene do 2 snimku)
  const float dx = 73.2f + (float)LANO_B * 0.894427f, dy = 36.6f + (float)LANO_B * 0.447214f;   // smer (2, 1)
  m.dotyk(Ps1Mys::DOLU, 8, 300, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 8, 300 + dx * 0.5f, 200 + dy * 0.5f, W, H, t + 16);
  m.dotyk(Ps1Mys::POHYB, 8, 300 + dx, 200 + dy, W, H, t + 32);
  const int u = m.dotyk(Ps1Mys::NAHORU, 8, 300 + dx, 200 + dy, W, H, t + 48);
  s = snimky(m, t, 10);
  over(s.sx == 32 && s.sy == 16 && u == 0 && s.snL == 0,
       "tah 10 % x 5 % sirky za lanem = posun 32 / 16 bodu (je " + txt(s.sx) + " / " + txt(s.sy) + "), zadny klik");
}

// B318: zrychleni podle rychlosti prstu
static Souhrn tahRychlosti(Ps1Mys &m, long long &t, double v, double draha) {
  // prst se pohne o drahu (podil sirky) rychlosti v (sirky za sekundu): dotyk kazdych 16 ms
  // a po nem snimek (jako telefon s displejem 60 Hz)
  const int kroku = std::max(2, (int)std::lround(draha / v / 0.016));
  const float x0 = 100, krokPx = (float)(draha * W / kroku);
  m.dotyk(Ps1Mys::DOLU, 1, x0, 200, W, H, t);
  Souhrn s;
  for (int k = 1; k <= kroku; k++) {
    t += 16;
    m.dotyk(Ps1Mys::POHYB, 1, x0 + krokPx * k, 200, W, H, t);
    s.sx += krok(m, t + 1).x;
  }
  m.dotyk(Ps1Mys::NAHORU, 1, x0 + krokPx * kroku, 200, W, H, t + 4);
  const Souhrn a = snimky(m, t, 8);
  s.sx += a.sx;
  return s;
}
static void test_zrychleni() {
  over(std::fabs(Ps1Mys::nasobekZrychleni(0.7) - 1.0) < 1e-9 && std::fabs(Ps1Mys::nasobekZrychleni(0.175) - 0.5) < 1e-9 &&
           std::fabs(Ps1Mys::nasobekZrychleni(2.8) - 2.0) < 1e-9,
       "krivka zrychleni: 0,7 sirky/s = 1x (rychlost z nastaveni plati), 0,175 = 0,5x, 2,8 = 2x");
  over(Ps1Mys::nasobekZrychleni(0.01) == Ps1Mys::ZR_MIN && Ps1Mys::nasobekZrychleni(50) == Ps1Mys::ZR_MAX &&
           Ps1Mys::nasobekZrychleni(0) == Ps1Mys::ZR_MIN,
       "krivka zrychleni: nejmene 0,4x (pomalounku), nejvys 2,5x");
  const double draha = 0.2, ocek = body(draha * W - LANO_B);   // 20 % sirky, z toho 1,5 % lano
  Ps1Mys m;
  long long t = 20000;
  zapni(m, 100, false);
  const Souhrn vp = tahRychlosti(m, t, 0.1, draha), vs = tahRychlosti(m, t, 0.7, draha), vr = tahRychlosti(m, t, 2.8, draha);
  over(std::labs(vp.sx - std::lround(ocek)) <= 1 && std::labs(vs.sx - std::lround(ocek)) <= 1 && std::labs(vr.sx - std::lround(ocek)) <= 1,
       "zrychleni VYPNUTO: tah 20 % sirky pomalu / normalne / rychle = vzdy stejne " + txt(vp.sx) + " / " + txt(vs.sx) + " / " +
           txt(vr.sx) + " bodu (vypocet " + txtf(ocek, 1) + ")");
  const std::string lgVyp = log(m);
  m.nastavZrychleni(true);
  const Souhrn zp = tahRychlosti(m, t, 0.1, draha), zs = tahRychlosti(m, t, 0.7, draha), zr = tahRychlosti(m, t, 2.8, draha);
  over(zs.sx >= ocek * 0.9 && zs.sx <= ocek * 1.1,
       "zrychleni ZAPNUTO, bezny tah (0,7 sirky/s): " + txt(zs.sx) + " bodu = jako bez zrychleni (" + txtf(ocek, 1) +
           ") - Reneho 295 % plati dal");
  over(zp.sx >= ocek * 0.35 && zp.sx <= ocek * 0.5,
       "pomaly presny tah (0,1 sirky/s): " + txt(zp.sx) + " bodu = 0,4x - kurzor jde jemne (vybrat jednu jednotku)");
  over(zr.sx >= ocek * 1.8 && zr.sx <= ocek * 2.1, "rychly tah (2,8 sirky/s): " + txt(zr.sx) + " bodu = 2x - pres celou mapu jednim tahem");
  // denik tahu: prvnich 12 tahu podrobne
  const std::string lg = lgVyp + log(m);
  const size_t t1 = lg.find("MYS TAH 1: "), t4 = lg.find("MYS TAH 4: ");
  over(t1 != std::string::npos && t4 != std::string::npos && lg.find("zrychleni vypnuto", t1) < t4 &&
           lg.find("zrychleni 0.4-0.4x", t4) != std::string::npos && lg.find("sirky obrazu (rychlost prumer ") != std::string::npos,
       "denik: kazdy tah - \"" + lg.substr(t1, lg.find('\n', t1) - t1) + "\", \"" + lg.substr(t4, lg.find('\n', t4) - t4) + "\"");
  for (int i = 0; i < 12; i++) tahRychlosti(m, t, 0.7, 0.1);
  const std::string lg2 = log(m);
  over(lg2.find("MYS: dalsi tahy uz jen v souctu MYS ZDRAVI") != std::string::npos && lg2.find("MYS TAH 13") == std::string::npos,
       "denik: po 12 tazich uz jen souhrn (zadne zahlceni)");
}

// B318: rozlozeni posunu podle toho, jak casto hra kresli
static void test_vyhlazeni() {
  Ps1Mys m;
  zapni(m);
  over(m.vyhlazeni() == 2, "na zacatku (hra nic nekreslila): posun rozlozen do 2 snimku");
  long long t = 30000;
  // jeden pohyb 30 bodu (kurzor) -> 15 + 15
  m.pcMys(9375, 0, 0, t);   // 9375 / 100000 sirky x 320 = 30 bodu
  Souhrn s = snimky(m, t, 4);
  over(s.dx[0] == 15 && s.dx[1] == 15 && s.dx[2] == 0 && s.sx == 30, "posun 30 bodu -> 15 + 15 (2 snimky), soucet presne 30");
  // hra kresli jen kazdy 3. snimek (Dune 2000: 20 obrazu za sekundu)
  for (int i = 0; i < 90; i++) { m.snimek(t += 17, 0); m.poSnimku(i % 3 == 2); }
  const std::string lg = log(m);
  over(m.vyhlazeni() == 3 && lg.find("posun mysi se rozklada do 3 snimku") != std::string::npos,
       "hra kresli kazdy 3. snimek -> posun do 3 snimku; denik: \"" + lg.substr(0, lg.find('\n')) + "\"");
  m.pcMys(9375, 0, 0, t);
  s = snimky(m, t, 4);
  over(s.dx[0] == 10 && s.dx[1] == 10 && s.dx[2] == 10 && s.dx[3] == 0, "30 bodu -> 10 + 10 + 10");
  // hra stridave po 2 a 3 snimcich (perioda 2,5): zadne prepinani sem a tam (hystereze)
  int zmen = 0, pred = m.vyhlazeni();
  for (int i = 0; i < 300; i++) {
    const int k = i % 5;   // novy obraz v 2. a 5. snimku z peti
    m.snimek(t += 17, 0);
    m.poSnimku(k == 1 || k == 4);
    if (m.vyhlazeni() != pred) { zmen++; pred = m.vyhlazeni(); }
  }
  over(zmen <= 1, "hra stridave po 2 a 3 snimcich: rozlozeni se neprepina sem a tam (zmen " + txt(zmen) + ")");
  // hra kresli kazdy snimek (60 obrazu za sekundu) -> zase 2
  for (int i = 0; i < 90; i++) { m.snimek(t += 17, 0); m.poSnimku(true); }
  over(m.vyhlazeni() == 2, "hra kresli kazdy snimek -> zase 2 snimky (nejmene - vyhladi i nerovnomerne dotyky)");
  // hra stoji (pauza, zadny novy obraz) -> nejvys 4
  for (int i = 0; i < 400; i++) { m.snimek(t += 17, 0); m.poSnimku(i % 12 == 11); }
  over(m.vyhlazeni() == 4, "hra skoro nekresli -> nejvys 4 snimky (" + txt(m.vyhlazeni()) + ")");
  m.poNacteniHry();
  over(m.vyhlazeni() == 2, "nova hra: zase od 2 snimku");
  // velky posun: nic se neztrati, 127 za snimek
  zapni(m);
  m.pcMys(100000, -50000, 0, t);   // 320 / -160 bodu
  s = snimky(m, t, 8);
  over(s.sx == 320 && s.sy == -160 && s.maxKrok <= 127, "velky posun 320 / -160: soucet presny, za snimek nejvys 127 (" + txt(s.maxKrok) + ")");
  // zasobnik: nevyzvednuty posun nejvys 1000 bodu (hra mys necte - zadne dobihani)
  for (int i = 0; i < 20; i++) m.pcMys(100000, 0, 0, t);
  s = snimky(m, t, 20);
  over(s.sx == 1000, "20x 320 bodu najednou: hra dostane nejvys 1000 (zbytek zahozen, zadne dlouhe dobihani) - " + txt(s.sx));
}

// hra cte mys jen kazdy 3. snimek (jako Dune 2000 - kresli 20 obrazu za sekundu): co uvidi?
// Prst jede rovnomerne; dotyky chodi 60x za sekundu, ale s chvenim +-6 ms (most WebView ->
// Java -> C++), snimky jadra po 16,683 ms - tak jako v telefonu. Merime, kam hra posune
// kurzor, proti tomu, kam by mel byt (tretina pohybu prstu - hra kazdy 2. a 3. snimek nevidi).
struct Cteni { double prumer, kolisani, odchylkaMax, celkem, prst; };
static Cteni hraCteKazdy3(int vynutit, unsigned seme) {
  Ps1Mys m;
  zapni(m);
  m.vynutVyhlazeni(vynutit);
  unsigned rnd = seme;
  auto nahoda = [&rnd]() { rnd = rnd * 1103515245u + 12345u; return (double)((rnd >> 8) & 0xFFFF) / 65535.0; };
  const double v = 0.5;   // bodu stranky za ms (0,68 sirky obrazu za sekundu - bezny tah)
  m.dotyk(Ps1Mys::DOLU, 1, 0, 200, W, H, 0);
  double tDotyk = 0, tSnimek = 16.683, hra = 0, maxOdch = 0;
  int k = 0, snimek = 0;
  long dodano = 0;   // co by videla hra, ktera cte kazdy snimek
  std::vector<int> videno;
  while (tDotyk < 1300) {
    k++;
    tDotyk = k * 16.667 + (nahoda() - 0.5) * 12.0;
    while (tSnimek < tDotyk) {
      m.snimek((long long)tSnimek, 0);
      const int dx = m.mysDotaz(0, Ps1Mys::ID_X);
      dodano += dx;
      m.poSnimku(snimek % 3 == 0);
      if (snimek % 3 == 0) {   // hra cte jen tehdy, kdyz kresli
        hra += dx;
        if (tSnimek > 200 && tSnimek < 1200) {
          videno.push_back(dx);
          maxOdch = std::max(maxOdch, std::fabs(hra - dodano / 3.0));
        }
      }
      snimek++;
      tSnimek += 16.683;
    }
    m.dotyk(Ps1Mys::POHYB, 1, (float)(tDotyk * v), 200, W, H, (long long)tDotyk);
  }
  double sum = 0, sum2 = 0;
  for (int x : videno) { sum += x; sum2 += (double)x * x; }
  const double n = (double)videno.size(), pr = sum / n;
  return Cteni{pr, std::sqrt(std::max(0.0, sum2 / n - pr * pr)), maxOdch, hra, (double)dodano};
}
static void test_hra_cte_kazdy_3() {
  double k317 = 0, k318 = 0, o317 = 0, o318 = 0, p317 = 0, p318 = 0;
  for (unsigned s = 1; s <= 8; s++) {
    const Cteni a = hraCteKazdy3(1, s), b = hraCteKazdy3(3, s);
    k317 += a.kolisani / 8; k318 += b.kolisani / 8;
    o317 = std::max(o317, a.odchylkaMax); o318 = std::max(o318, b.odchylkaMax);
    p317 += a.prumer / 8; p318 += b.prumer / 8;
  }
  over(o318 <= 3.0,
       "hra cte jen kazdy 3. snimek: B318 (rozlozeni do 3 snimku) - kurzor ve hre presne na tretine pohybu prstu, odchylka nejvys " +
           txtf(o318, 1) + " bodu (8 behu s chvenim dotyku)");
  over(o317 >= 2.0 * o318 && o317 >= 4.0,
       "stejne dotyky jako B317 (bez rozlozeni): hra videla nahodne vic / min - kurzor ujizdel od prstu az o " + txtf(o317, 1) + " bodu");
  over(k318 < 0.75 * k317,
       "posun za jedno cteni hry: B318 kolisa +-" + txtf(k318) + " bodu, B317 +-" + txtf(k317) + " (kurzor se cukal)");
  over(std::fabs(p318 - p317) < 0.5,
       "rychlost kurzoru ve hre zustava (B317 " + txtf(p317) + " / B318 " + txtf(p318) + " bodu za cteni) - Reneho citlivost plati dal");
}

static void test_tuknuti() {
  Ps1Mys m;
  zapni(m);
  long long t = 40000;
  m.dotyk(Ps1Mys::DOLU, 1, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 1, 205, 203, W, H, t + 40);   // chveni prstu 0,8 % sirky (v lane)
  const int u = m.dotyk(Ps1Mys::NAHORU, 1, 205, 203, W, H, t + 110);
  Souhrn s = snimky(m, t, 15);
  over(u == Ps1Mys::KLIK_L, "tuknuti (0,11 s, chveni prstu): dotyk hlasi klik levym (stranka zavibruje)");
  over(s.stiskL == 1 && s.snL == Ps1Mys::KLIK_SNIMKU && s.sx == 0 && s.sy == 0 && s.snP == 0 && s.prvniL == 0,
       "tuknuti = leve tlacitko hned v dalsim snimku, 1x dole presne 5 snimku, kurzor stoji: " + s.vzorL);

  m.dotyk(Ps1Mys::DOLU, 2, 200, 200, W, H, t);
  const int u2 = m.dotyk(Ps1Mys::NAHORU, 2, 200, 200, W, H, t + 300);
  s = snimky(m, t, 10);
  over(u2 == 0 && s.snL == 0, "prst nahore az po 0,3 s: zadny klik (neni to tuknuti)");

  // B318: tuknuti, pri kterem prst ujel 2,3 % (pres lano, ale porad tuknuti): kurzor dojede, AZ PAK klik
  m.dotyk(Ps1Mys::DOLU, 5, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 5, 216.8f, 200, W, H, t + 60);
  const int u5 = m.dotyk(Ps1Mys::NAHORU, 5, 216.8f, 200, W, H, t + 120);
  s = snimky(m, t, 15);
  over(u5 == Ps1Mys::KLIK_L && s.sx == std::lround(body(16.8 - LANO_B)) && s.prvniL > s.posledniPosun && s.stiskL == 1,
       "tuknuti s posunem prstu 2,3 %: kurzor +" + txt(s.sx) + " bod(y) a klik AZ PO dojeti (posun do snimku " + txt(s.posledniPosun) +
           ", klik od " + txt(s.prvniL) + ") - klikne tam, kam prst ukazal");
  m.dotyk(Ps1Mys::DOLU, 6, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 6, 225, 200, W, H, t + 60);
  const int u6 = m.dotyk(Ps1Mys::NAHORU, 6, 225, 200, W, H, t + 120);
  s = snimky(m, t, 10);
  over(u6 == 0 && s.snL == 0 && s.sx > 0, "prst ujel 3,4 % (vic nez tuknuti): jen posun, zadny klik");

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

  // klik nesmi cekat donekonecna, kdyz se kurzor porad hybe (mys z PC jede): nejvys 6 snimku
  m.dotyk(Ps1Mys::DOLU, 30, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::NAHORU, 30, 200, 200, W, H, t + 50);
  Souhrn s2;
  for (int i = 0; i < 12; i++) {
    m.pcMys(2000, 0, 0, t);
    const Souhrn a = snimky(m, t, 1);
    s2.snL += a.snL;
    if (a.snL && s2.prvniL < 0) s2.prvniL = i;
  }
  over(s2.prvniL >= 0 && s2.prvniL <= Ps1Mys::KLIK_CEKA_MAX, "kurzor se porad hybe (mys z PC): klik po tuknuti cekal jen " + txt(s2.prvniL) + " snimku");
}

static void test_dva_prsty() {
  Ps1Mys m;
  zapni(m);
  long long t = 50000;
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
  s = snimky(m, t, 6);
  over(s.snP == 0 && s.snL == 0 && s.sx == std::lround(body(200 - LANO_B)),
       "tah + kratky dotyk druhym prstem: zadny klik, tah pokracuje (" + txt(s.sx) + " bodu)");
}

static void test_drzeni() {
  Ps1Mys m;
  zapni(m);
  long long t = 60000;
  m.dotyk(Ps1Mys::DOLU, 1, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 1, 206, 203, W, H, t + 100);   // chveni v lane
  Souhrn s = snimky(m, t, 30);    // 0,6 s
  over(s.snL == 0 && (m.stavBity() & 2) == 0, "prst na miste 0,6 s: tlacitko jeste ne (B317 uz po 0,5 s - Rene obcas omylem)");
  s = snimky(m, t, 3);            // 0,66 s
  over(s.snL > 0 && (m.stavBity() & 2) != 0, "prst na miste 0,65 s: leve tlacitko DRZI (stav bit1 pro stranku)");
  const float dx = 73.2f + (float)LANO_B;
  m.dotyk(Ps1Mys::POHYB, 1, 200 + dx, 200, W, H, t + 10);
  s = snimky(m, t, 5);
  over(s.snL == 5 && s.sx == 32, "tah s drzenym levym = vyber ramem (posun 32 za lanem, tlacitko porad dole)");
  const int u = m.dotyk(Ps1Mys::NAHORU, 1, 200 + dx, 200, W, H, t + 10);
  s = snimky(m, t, 5);
  over(u == 0 && s.snL == 0 && (m.stavBity() & 2) == 0, "prst nahoru: leve tlacitko pusteno, zadny klik navic");

  // prst se pred 0,65 s pohnul za lano (zacal tahnout) -> zadne drzeni, jen posun
  m.dotyk(Ps1Mys::DOLU, 2, 200, 200, W, H, t);
  m.dotyk(Ps1Mys::POHYB, 2, 215, 200, W, H, t + 200);
  s = snimky(m, t, 40);
  m.dotyk(Ps1Mys::NAHORU, 2, 215, 200, W, H, t);
  over(s.snL == 0, "prst se do 0,65 s pohnul o 2 % (tah): leve se nestiskne");

  // podrzeni a ZRUSENO (system dotyk zrusil)
  m.dotyk(Ps1Mys::DOLU, 3, 200, 200, W, H, t);
  snimky(m, t, 40);
  m.dotyk(Ps1Mys::ZRUSENO, 3, 200, 200, W, H, t);
  s = snimky(m, t, 5);
  over(s.snL == 0 && m.stavBity() == 1, "dotyk zrusen behem drzeni: tlacitko pusteno, zadny prst");
  over(log(m).find("dotyk zrusen - leve tlacitko pusteno") != std::string::npos, "denik: \"MYS: dotyk zrusen - leve tlacitko pusteno\"");

  // drzeni se zmenou rezimu: vse pusteno
  m.dotyk(Ps1Mys::DOLU, 4, 200, 200, W, H, t);
  snimky(m, t, 40);
  m.nastav(Ps1Mys::PORT1_OVL2, 100);
  s = snimky(m, t, 3);
  over(s.snL == 0 && m.stavBity() == 1, "zmena rezimu behem drzeni: tlacitko pusteno");
}

static void test_tlacitka_a_smer() {
  const uint32_t KRIZEK = Ps1Mys::BIT_KRIZEK, KOLECKO = Ps1Mys::BIT_KOLECKO, VPRAVO = Ps1Mys::BIT_VPRAVO,
                 VLEVO = Ps1Mys::BIT_VLEVO, NAHORU = Ps1Mys::BIT_NAHORU, DOLU = Ps1Mys::BIT_DOLU;
  Ps1Mys m;
  zapni(m);
  long long t = 70000;
  Souhrn s = snimky(m, t, 10, KRIZEK);
  Souhrn s2 = snimky(m, t, 7, KOLECKO);
  over(s.snL == 10 && s.snP == 0 && s2.snP == 7 && s2.snL == 0, "mys sama v portu 1: KRIZEK drzi leve, KOLECKO prave tlacitko (beze zmeny)");
  bool zadny = true;
  for (unsigned port = 0; port < 2; port++)
    for (unsigned id = 0; id < 16; id++) zadny &= m.padDotaz(port, id, 0xFFFFu) == 0;
  over(zadny, "mys sama v portu 1: ovladac neni zapojeny nikde (hra tlacitka nevidi)");
  // smer: 1 bod za snimek, drzenim az 6 (za 0,7 s) - rozlozene do 2 snimku
  s = snimky(m, t, 36, VPRAVO);
  s2 = snimky(m, t, 10, VPRAVO);
  double ocek = 0;
  for (int i = 0; i < 36; i++) { double u = std::min(1.0, i * 20 / 700.0); ocek += 1 + 5 * u; }
  over(std::fabs(s.sx - ocek) <= 4.0 && s2.sx == 60 && s2.maxKrok == 6,
       "D-pad drzeny: zrychli z 1 na 6 bodu za snimek (" + txt(s.sx) + " za 36 snimku, vypocet " + txtf(ocek, 0) + "; pak " + txt(s2.sx) + " za 10)");
  s = snimky(m, t, 4);
  s = snimky(m, t, 10, NAHORU | DOLU);
  over(s.sy == 0 && s.sx == 0, "nahoru + dolu naraz: vyrusi se");
  s = snimky(m, t, 6, VLEVO | NAHORU);
  over(s.sx < 0 && s.sy < 0 && s.sx >= -8, "sikmo vlevo nahoru zase pomalu (" + txt(s.sx) + " / " + txt(s.sy) + ")");
  // s ovladacem v druhem portu patri vsechna tlacitka ovladaci - mys je nebere
  m.nastav(Ps1Mys::PORT1_OVL2, 100);
  s = snimky(m, t, 10, KRIZEK | KOLECKO | VPRAVO);
  const bool padOk = m.padDotaz(1, 0, KRIZEK) == 1 && m.padDotaz(1, 8, KOLECKO) == 1 && m.padDotaz(1, 7, VPRAVO) == 1 &&
                     m.padDotaz(0, 0, KRIZEK) == 0;
  over(s.snL == 0 && s.snP == 0 && s.sx == 0 && padOk, "mys port 1 + ovladac port 2: KRIZEK / KOLECKO / smer jdou ovladaci v portu 2, mys stoji");
}

static void test_pc() {
  Ps1Mys m;
  long long t = 80000;
  m.pcMys(10000, 10000, 1, t);
  Souhrn s = snimky(m, t, 2);
  over(s.sx == 0 && s.snL == 0, "mys z PC s vypnutou mysi: nic");
  zapni(m, 150, true);
  m.pcMys(10000, -5000, 1, t);   // 10 % a -5 % sirky obrazu v prohlizeci
  s = snimky(m, t, 4);
  over(s.sx == 48 && s.sy == -24 && s.snL == 4,
       "mys z PC: 10 % sirky obrazu v prohlizeci = 32 bodu x 150 % = 48 (stejne miritko jako prst, bez zrychleni - to dela PC), leve drzi");
  m.pcMys(0, 0, 2, t);
  s = snimky(m, t, 3);
  over(s.snL == 0 && s.snP == 3, "mys z PC: prave tlacitko");
  snimky(m, t, 130);   // 2,6 s ticho
  Souhrn s2 = snimky(m, t, 3);
  over(s2.snP == 0 && log(m).find("mys z PC se 2,5 s neozvala - tlacitka pustena") != std::string::npos,
       "prohlizec se odmlcel 2,5 s: tlacitka pustena + radek v deniku");
}

static void test_pamet(const std::string &prac) {
  const std::string soubor = prac + "/PS1_mys_pro_hry_b318.txt";
  std::remove(soubor.c_str());
  Ps1Mys m;
  over(!m.nactiProHru(soubor, "SLUS00973") && m.rezim() == Ps1Mys::VYP && m.citlivost() == 100 && m.zrychleni(),
       "hra bez zaznamu: mys vypnuta, citlivost 100 %, zrychleni zapnuto");
  m.nastav(Ps1Mys::PORT1_OVL2, 295);
  m.nastavZrychleni(false);
  over(m.ulozProHru(soubor), "ulozeni pro Dune 2000 USA (SLUS00973): port 1 + ovladac 2, 295 %, zrychleni vypnuto");
  Ps1Mys m2;
  over(m2.nactiProHru(soubor, "SLUS00973") && m2.rezim() == 2 && m2.citlivost() == 295 && !m2.zrychleni(),
       "novy start appky: SLUS00973 = port 1 + ovladac 2, 295 %, zrychleni vypnuto");
  std::ifstream f(soubor);
  std::stringstream ss;
  ss << f.rdbuf();
  over(ss.str().rfind("# B317", 0) == 0 && ss.str().find("zrychleni (1 zap, 0 vyp)") != std::string::npos &&
           ss.str().find("SLUS00973\t2\t295\t0\n") != std::string::npos,
       "soubor: klic TAB rezim TAB citlivost TAB zrychleni");
  // soubor z B317 (3 sloupce - Reneho Dune 2000): zrychleni zapnuto, nic se neztrati
  {
    std::ofstream o(soubor);
    o << "# B317 mys PS1 pro kazdou hru\nSLUS00973\t2\t295\nSLES02250\t1\t140\n";
  }
  Ps1Mys m3;
  over(m3.nactiProHru(soubor, "SLUS00973") && m3.rezim() == 2 && m3.citlivost() == 295 && m3.zrychleni(),
       "soubor z B317 (bez sloupce zrychleni): Reneho Dune 2000 zustava port 1 + ovladac 2, 295 %, zrychleni zapnuto");
  m3.nastavZrychleni(false);
  m3.ulozProHru(soubor);
  Ps1Mys m4;
  over(m4.nactiProHru(soubor, "SLES02250") && m4.rezim() == 1 && m4.citlivost() == 140 && m4.zrychleni() &&
           m4.nactiProHru(soubor, "SLUS00973") && !m4.zrychleni(),
       "po ulozeni: ostatni hry z B317 souboru beze zmeny, zrychleni jen u Dune vypnuto");
  // B317 cte soubor B318 (navrat na starsi verzi): citlivost z "295\t0" = 295
  over(std::atoi("295\t0\n") == 295, "starsi appka (B317) z radku B318 precte citlivost spravne (atoi skonci na TAB)");
  // rozbity soubor
  {
    std::ofstream o(soubor);
    o << "# x\nnesmysl\nA\t9\t9999\nB\t2\nC\t1\t5\t7\n\t1\t100\nD\t2\t300\tx\n";
  }
  Ps1Mys m5;
  bool ok = m5.nactiProHru(soubor, "A") && m5.rezim() == Ps1Mys::VYP && m5.citlivost() == 100;
  ok &= !m5.nactiProHru(soubor, "B");
  ok &= m5.nactiProHru(soubor, "C") && m5.rezim() == 1 && m5.citlivost() == 100 && m5.zrychleni();
  ok &= m5.nactiProHru(soubor, "D") && m5.rezim() == 2 && m5.citlivost() == 300 && !m5.zrychleni();
  over(ok, "poskozeny soubor: radek bez citlivosti preskocen, nesmysly opraveny, zrychleni 7 = zap, x = vyp");
  over(!m5.ulozProHru(prac + "/neexistuje/x.txt"), "slozka neexistuje: ulozeni hlasi chybu (do deniku)");
}

static void test_souhrn() {
  Ps1Mys m;
  zapni(m, 100, true);
  long long t = 90000;
  for (int i = 0; i < 60; i++) { m.snimek(t += 17, 0); m.poSnimku(i % 3 == 2); }
  m.souhrn();
  m.dotyk(Ps1Mys::DOLU, 1, 0, 0, W, H, t);
  for (int k = 1; k <= 10; k++) m.dotyk(Ps1Mys::POHYB, 1, 20.f * k, 0, W, H, t + 16 * k);
  m.dotyk(Ps1Mys::NAHORU, 1, 200, 0, W, H, t + 170);
  m.dotyk(Ps1Mys::DOLU, 2, 0, 0, W, H, t + 300);
  m.dotyk(Ps1Mys::NAHORU, 2, 0, 0, W, H, t + 350);
  for (int i = 0; i < 30; i++) { krok(m, t += 17, i < 20 ? Ps1Mys::BIT_VPRAVO : 0, i % 3 == 2); }
  m.pcMys(100000, 0, 1, t);
  for (int i = 0; i < 6; i++) krok(m, t += 17, 0, i % 3 == 2);
  const std::string s = m.souhrn();
  over(s.find("mys v portu 1, citlivost 100 %, zrychleni ZAP (") == 0 && s.find("vyhlazeni 3 snimky (hra kresli kazdy 3.0. snimek)") != std::string::npos &&
           s.find("dotyku 2 (tuknuti 1, tahu 1 (prumer 0.17 s), drzeni L 0)") != std::string::npos &&
           s.find("klik L 1 P 0") != std::string::npos && s.find("smerem 20") != std::string::npos &&
           s.find("(oriznuto snimku ") != std::string::npos && s.find("z PC zprav 1 (zmen tlacitek 1)") != std::string::npos,
       "souhrn do MYS ZDRAVI: " + s);
  const std::string s2 = m.souhrn();
  over(s2.find("dotyku 0 (tuknuti 0, tahu 0, drzeni L 0), posun 0 / 0 bodu") != std::string::npos, "dalsi okno zacina od nuly: " + s2);
  m.nastavZrychleni(false);
  over(m.souhrn().find("zrychleni VYP, vyhlazeni 3 snimky (hra nekresli nic noveho)") != std::string::npos, "zrychleni vypnuto a hra stoji - v souhrnu");
  m.poNacteniHry();
  over(m.souhrn().find("vyhlazeni 2 snimky") != std::string::npos, "nova hra (poNacteniHry): rozlozeni od zacatku (2)");
  m.nastav(Ps1Mys::VYP, 100);
  m.souhrn();
  over(m.souhrn().empty(), "vypnuta a nic: prazdny souhrn");
}

// soubezne jako v appce: stranka (dotyky), vlakno emulace (snimek + poSnimku + dotazy jadra), nastaveni, TV
static void test_vlakna(const std::string &prac) {
  Ps1Mys m;
  m.nastav(Ps1Mys::PORT1, 100);
  std::atomic<bool> konec{false};
  std::atomic<long> dotazu{0};
  const std::string soubor = prac + "/PS1_mys_vlakna_b318.txt";
  std::thread emu([&] {
    long long t = 0;
    int i = 0;
    while (!konec) {
      t += 20;
      m.snimek(t, (t / 200) % 3 == 0 ? Ps1Mys::BIT_KRIZEK | Ps1Mys::BIT_VPRAVO : 0);
      unsigned d[2];
      m.zmenaPortu(d);
      for (unsigned p = 0; p < 2; p++)
        for (unsigned id = 0; id < 4; id++) { m.mysDotaz(p, id); dotazu++; }
      for (unsigned id = 0; id < 16; id++) m.padDotaz(0, id, 0xFFFF);
      m.poSnimku(++i % 3 == 0);
      m.vezmiLog();
      m.souhrn();
      m.vyhlazeni();
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
      m.nastavZrychleni(i % 2 == 0);
      m.nactiProHru(soubor, "HRA" + std::to_string(i % 5));
      m.ulozProHru(soubor);
      m.klic();
      if (i % 50 == 0) m.poNacteniHry();
    }
  });
  std::thread tv([&] { for (int i = 0; i < 3000; i++) m.pcMys(i % 9 - 4, i % 5 - 2, i % 4, i); });
  stranka.join(); nastaveni.join(); tv.join();
  konec = true;
  emu.join();
  over(dotazu > 0, "soubezne: stranka, vlakno emulace (+ poSnimku), nastaveni + zrychleni + soubor, mys z PC - dobehlo (" +
                       txt(dotazu.load()) + " dotazu jadra)");
}

int main(int argc, char **argv) {
  const std::string prac = argc > 1 ? argv[1] : ".";
  std::printf("B318 - mys PlayStation: jemnejsi touchpad (nap_ps1_mys.h)\n");
#ifndef NAP_TSAN
  test_vychozi();
  test_nastaveni();
  test_lano();
  test_zrychleni();
  test_vyhlazeni();
  test_hra_cte_kazdy_3();
  test_tuknuti();
  test_dva_prsty();
  test_drzeni();
  test_tlacitka_a_smer();
  test_pc();
  test_pamet(prac);
  test_souhrn();
#endif
  test_vlakna(prac);
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
