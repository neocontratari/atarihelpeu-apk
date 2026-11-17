// B342: hlavni menu (SKUTECNY nap_menu_karusel.h z appky) - tlacitko OTOCENI OBRAZOVKY vlevo nahore
// (zrcadlo kolecka OPTIONS). Klepnuti = AKCE_OTOCIT (2001) -> Java NativeMenuBridge.AKCE_OTOCIT.
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/menu -o test_b342_karusel test_b342_karusel.cpp && ./test_b342_karusel
#include <cmath>
#include <cstdio>
#include <string>
#include "nap_menu_karusel.h"

using namespace napmenu;

static int ok = 0, chyb = 0;
static void over(bool c, const std::string &co) {
  if (c) ok++; else chyb++;
  std::printf("%s%s\n", c ? "OK     " : "CHYBA  ", co.c_str());
}
static std::string f(const char *fmt, double a = 0, double b = 0, double c = 0, double d = 0) {
  char s[300]; std::snprintf(s, sizeof s, fmt, a, b, c, d); return s;
}
static int klepni(Karusel &k, float x, float y, double &t) {
  k.dolu(x, y, t); t += 0.06;
  const int a = k.nahoru(x, y, t);
  for (int i = 0; i < 120; i++) { t += 1.0 / 60.0; k.krok(t); }
  return a;
}

int main() {
  std::printf("B342 - hlavni menu: tlacitko otoceni obrazovky (vlevo nahore, zrcadlo kolecka OPTIONS)\n\n");
  over(AKCE_OTOCIT == 2001 && AKCE_OTOCIT > AKCE_NASTAVENI && AKCE_OTOCIT < AKCE_ZAMCENO,
       "AKCE_OTOCIT = 2001 (mezi OPTIONS 2000 a zamcenou kartou 3000 - Java ji nezamenila za kartu ani zamek)");
  struct Obr { int w, h; bool tv; const char *jm; };
  const Obr obr[] = {{1080, 2340, false, "telefon na vysku 1080x2340"}, {720, 1280, false, "telefon na vysku 720x1280"},
                     {2340, 1080, false, "telefon na sirku 2340x1080"}, {1920, 1080, false, "TV 1920x1080 (cela obrazovka)"},
                     {608, 1080, false, "TV pruh 9:16 608x1080"},        {405, 720, false, "TV pruh 9:16 405x720"},
                     {2560, 1600, false, "tablet na sirku"},              {850, 1000, false, "skoro ctverec 850x1000 (rozdelena obrazovka)"},
                     {854, 480, true, "obraz pro TV / PC 854x480 (proud)"}};
  for (const Obr &o : obr) {
    const Rozlozeni r = spoctiRozlozeni(o.w, o.h, o.tv, 7);
    std::string proc;
    if (!(r.otocR > 0.f)) proc += "bez-tlacitka ";
    if (r.otocX - r.otocR < 0 || r.otocX + r.otocR > o.w || r.otocY - r.otocR < 0) proc += "mimo-displej ";
    // kolecko: kruhy (s rezervou pro prst 1,55 R) se neprekryvaji
    const float dk = std::hypot(r.otocX - r.ozubX, r.otocY - r.ozubY);
    if (dk < (r.otocR + r.ozubR) * 1.05f) proc += "pres-kolecko ";
    // mince N&P
    if (std::hypot(r.otocX - r.logoX, r.otocY - r.logoY) < r.otocR + r.logoR) proc += "pres-minci ";
    // napis N&P VISION: na vysku pod minci (tlacitko konci nad nim), na sirku vpravo od mince
    if (!r.naSirku && r.otocY + r.otocR > r.titulY - r.titulPx * 0.5f) proc += "pres-napis ";
    if (r.naSirku && r.otocX > r.logoX && r.otocX - r.otocR < r.logoX + r.logoR * 1.35f + r.titulPx * 6.2f && r.otocX < r.ozubX)
      proc += "pres-napis ";
    // karta (nejvyssi bod prostredni karty)
    if (r.otocY + r.otocR > r.cy - r.ch * 0.5f) proc += "pres-kartu ";
    over(proc.empty(), std::string(o.jm) + f(": otoceni (%.0f, %.0f) R %.0f, kolecko (%.0f)", r.otocX, r.otocY, r.otocR, r.ozubX)
                           + (proc.empty() ? "" : " PROBLEM: " + proc));
    if (!r.naSirku) over(std::fabs(r.otocX - (o.w - r.ozubX)) < 0.5f && r.otocY == r.ozubY && r.otocR == r.ozubR,
                         std::string(o.jm) + ": na vysku presne zrcadlo kolecka (stejna velikost i vyska)");
    // klepnuti
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(r); k.nastavPrah(24.f);
    double t = 1.0;
    const int a1 = klepni(k, r.otocX + r.otocR * 0.5f, r.otocY + r.otocR * 0.3f, t);
    const int a2 = klepni(k, r.ozubX, r.ozubY, t);
    const int a3 = klepni(k, r.cx, r.cy, t);
    // okraj rezervy pro prst (1,5 R) jeste plati, 1,7 R uz ne
    const int a4 = klepni(k, r.otocX, r.otocY + r.otocR * 1.5f, t);
    over(a1 == AKCE_OTOCIT && a2 == AKCE_NASTAVENI && a3 == AKCE_KARTA + 0 && a4 == AKCE_OTOCIT,
         std::string(o.jm) + f(": klepnuti otoceni %.0f, kolecko %.0f, karta %.0f, okraj otoceni %.0f", a1, a2, a3, a4));
  }
  // klepnuti na otoceni behem jizdy karuselu zastavi karusel na vybrane karte (jako kolecko)
  {
    const Rozlozeni r = spoctiRozlozeni(1080, 2340, false, 7);
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(r); k.nastavPrah(24.f);
    double t = 1.0;
    k.skoc(3);
    const int a = klepni(k, r.otocX, r.otocY, t);
    over(a == AKCE_OTOCIT && k.vybrana() == 3 && k.vKlidu(), f("otoceni nehne s karuselem (vybrana karta %.0f)", k.vybrana()));
  }
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", ok, chyb);
  return chyb ? 1 : 0;
}
