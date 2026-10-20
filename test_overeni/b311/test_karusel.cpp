// B311: test logiky karuselu hlavniho menu (bez OpenGL, bez Androidu)
// SKUTECNE hlavicky z appky: app/src/main/cpp/menu/nap_menu_karty.h a nap_menu_karusel.h
//   g++ -std=c++17 -O2 -I../../app/src/main/cpp/menu -o test_karusel test_karusel.cpp && ./test_karusel
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "nap_menu_karty.h"
#include "nap_menu_karusel.h"

using namespace napmenu;

static int ok = 0, chyb = 0;
static void over(bool c, const char *co, const std::string &det = "") {
  if (c) { ok++; std::printf("OK     %s%s%s\n", co, det.empty() ? "" : "  ", det.c_str()); }
  else { chyb++; std::printf("CHYBA  %s%s%s\n", co, det.empty() ? "" : "  ", det.c_str()); }
}
static std::string f(const char *fmt, double a, double b = 0, double c = 0, double d = 0) {
  char s[256]; std::snprintf(s, sizeof s, fmt, a, b, c, d); return s;
}

// simulace prstu: tah z x0 do x1 za "trvani" s, vzorky po 8 ms (jako dotyk 120 Hz), pak zvednout
static int tahni(Karusel &k, double &t, float x0, float x1, double trvani, float y = 1000.f, double drzetPred = 0.0) {
  k.dolu(x0, y, t);
  const int N = std::max(1, (int)(trvani / 0.008));
  for (int i = 1; i <= N; i++) {
    t += trvani / N;
    k.tah(x0 + (x1 - x0) * (float)i / N, y, t);
    k.krok(t);
  }
  if (drzetPred > 0) { const int M = (int)(drzetPred / 0.008); for (int i = 0; i < M; i++) { t += 0.008; k.tah(x1, y, t); k.krok(t); } }
  t += 0.004;
  return k.nahoru(x1, y, t);
}

// kdyTemer = kdy uz je karusel do 2 % karty od cile (pro oko hotovo)
struct Beh { float maxV = 0, minPoz = 1e9f, maxPoz = -1e9f; double kdyKlid = -1, kdyTemer = -1; int obratu = 0; };
static Beh dobehni(Karusel &k, double &t, double max_s = 3.0) {
  Beh b;
  float predV = k.rychlost();
  const double t0 = t;
  while (t - t0 < max_s) {
    t += 1.0 / 60.0;
    k.krok(t);
    b.maxV = std::max(b.maxV, std::fabs(k.rychlost()));
    b.minPoz = std::min(b.minPoz, k.poz());
    b.maxPoz = std::max(b.maxPoz, k.poz());
    if (predV * k.rychlost() < 0 && std::fabs(k.rychlost()) > 0.01f) b.obratu++;
    predV = k.rychlost();
    if (b.kdyTemer < 0 && std::fabs(k.poz() - k.cil()) < 0.02f && k.stav() == Karusel::DOJEZD) b.kdyTemer = t - t0;
    if (k.vKlidu()) { b.kdyKlid = t - t0; if (b.kdyTemer < 0) b.kdyTemer = b.kdyKlid; break; }
  }
  return b;
}

int main() {
  std::printf("B311 - karusel hlavniho menu: karty.txt, rozlozeni, fyzika, dotyk, klavesy\n\n");

  // ---------------- 1) karty.txt ----------------
  std::printf("1) karty.txt\n");
  {
    FILE *fp = std::fopen("../../app/src/main/assets/menu/karty.txt", "rb");
    std::string txt;
    if (fp) { char b[4096]; size_t n; while ((n = std::fread(b, 1, sizeof b, fp)) > 0) txt.append(b, n); std::fclose(fp); }
    over(!txt.empty(), "assets/menu/karty.txt existuje", f("%.0f B", (double)txt.size()));
    std::string ch;
    auto v = nactiKarty(txt.data(), txt.size(), &ch);
    over(v.size() == 7, "7 karet", f("%.0f", (double)v.size()));
    over(ch.empty(), "bez varovani", ch);
    const char *poradi[] = {"atari", "sega", "ps1", "mp3", "dzubox", "web", "help"};
    bool p = v.size() == 7;
    for (size_t i = 0; p && i < 7; i++) p = v[i].id == poradi[i];
    over(p, "poradi ATARI, SEGA, PS1, MP3, DZUBOX, WEB, HELP (HELP na konci)");
    if (v.size() == 7) {
      over(v[0].cil == "emu_atari_cpp/index.html", "ATARI -> emu_atari_cpp/index.html (Atari 130XE v C++)", v[0].cil);
      over(v[1].cil == "emu_sega/index.html" && v[1].zamekKlic == "sega", "SEGA -> emu_sega, zamek sega");
      over(v[2].cil == "emu_ps1/index.html" && v[2].zamekKlic == "ps1", "PS1 -> emu_ps1, zamek ps1");
      over(v[3].cil == "player/index.html", "MP3 PREHRAVAC -> player/index.html (Retro Cassette Player)", v[3].nazev);
      over(v[4].cil == "dj/index.html", "DZUBOX -> dj/index.html (PRO DJ MIXER)", v[4].nazev);
      over(v[5].cil == "https://atarihelp.eu/", "ATARIHELP.EU -> https://atarihelp.eu/");
      over(v[6].cil == "napoveda/index.html", "HELP -> napoveda/index.html (navod)");
      bool vsechnyEfekty = true, vsechnyObr = true;
      for (auto &k : v) { if (k.efekt.empty()) vsechnyEfekty = false; if (k.obrazek.empty()) vsechnyObr = false; }
      over(vsechnyEfekty && vsechnyObr, "kazda karta ma zivy efekt i obrazek (zaloha / HTML)");
      // obrazky existuji
      bool ex = true; std::string chybi;
      for (auto &k : v) {
        FILE *o = std::fopen(("../../app/src/main/assets/" + k.obrazek).c_str(), "rb");
        if (!o) { ex = false; chybi += k.obrazek + " "; } else std::fclose(o);
      }
      over(ex, "obrazky karet jsou v assets", chybi);
    }
    // zaloha v C++ = stejne karty
    auto z = nactiKarty(KARTY_ZALOHA, std::strlen(KARTY_ZALOHA));
    bool stejne = z.size() == v.size();
    for (size_t i = 0; stejne && i < z.size(); i++) stejne = z[i].id == v[i].id && z[i].cil == v[i].cil && z[i].efekt == v[i].efekt && z[i].zamekKlic == v[i].zamekKlic;
    over(stejne, "zaloha v C++ (kdyby karty.txt chybel) ma stejne karty, cile, efekty a zamky");
  }
  {
    // pridani karty = jeden radek; spatne radky
    const char *t =
        "\xEF\xBB\xBF# komentar\n"
        "\n"
        "  atari | ATARI 130XE | x | popis | a.jpg | emu_atari_cpp/index.html | #F2B640 |  | 3d:atari\r\n"
        "nova | | | | menu/nova.png | nova/index.html | #12ab\n"
        "| bez id | \n"
        "atari | DRUHE ATARI | | | | x.html | fb4\n"
        "c64 | COMMODORE 64 | 8-BIT | Nova masina | menu/c64.png | emu_c64/index.html | #8080FF | c64 | | SPUSTIT\n";
    std::string ch;
    auto v = nactiKarty(t, std::strlen(t), &ch);
    over(v.size() == 4, "parser: BOM, komentar, prazdny radek, CRLF, radek bez id preskocen", f("karet %.0f", (double)v.size()));
    if (v.size() == 4) {
      over(v[0].id == "atari" && v[0].efekt == "3d:atari" && v[0].barva == 0xF2B640 && v[0].tlacitko == "OTEVŘÍT",
           "orezane mezery, barva, chybejici tlacitko = OTEVŘÍT");
      over(v[1].nazev == "NOVA" && v[1].barva == 0xF2B640, "chybejici nazev = id velkymi, spatna barva = zlata", v[1].nazev);
      over(v[2].id != "atari" && v[2].barva == 0xFFBB44, "stejne id podruhe prejmenovano, barva fb4 = #FFBB44", v[2].id);
      over(v[3].nazev == "COMMODORE 64" && v[3].zamekKlic == "c64" && v[3].efekt.empty() && v[3].tlacitko == "SPUSTIT",
           "nova masina jednim radkem (jen obrazek, bez efektu)");
    }
    over(ch.find("chybi id") != std::string::npos && ch.find("uz je") != std::string::npos && ch.find("spatna barva") != std::string::npos,
         "varovani do logu (bez id, stejne id, spatna barva)");
    over(velka("džubox přehrávač") == "DŽUBOX PŘEHRÁVAČ", "velka pismena s hacky", velka("džubox přehrávač"));
  }

  // ---------------- 2) rozlozeni ----------------
  std::printf("\n2) rozlozeni (telefon na vysku, na sirku, tablet, TV)\n");
  struct Obr { int w, h; bool tv; const char *jm; };
  const Obr obr[] = {{1080, 2340, false, "telefon 1080x2340"}, {1080, 1920, false, "telefon 1080x1920"}, {720, 1280, false, "telefon 720x1280"},
                     {1600, 2560, false, "tablet na vysku"},   {2340, 1080, false, "telefon na sirku"},  {2560, 1600, false, "tablet na sirku"},
                     {854, 480, true, "TV / PC 854x480"}};
  for (const Obr &o : obr) {
    const Rozlozeni r = spoctiRozlozeni(o.w, o.h, o.tv, 7);
    const float top = r.cy - r.ch * 0.5f, bot = r.cy + r.ch * 0.5f;
    bool dobre = true; std::string proc;
    auto chyba = [&](const char *c) { dobre = false; proc += c; proc += " "; };
    if (r.cw < o.w * (r.naSirku ? 0.12f : 0.55f)) chyba("karta-mala");
    if (top < (r.naSirku ? r.logoY + r.logoR : r.sloganY + r.sloganPx * 0.5f)) chyba("karta-pres-hlavicku");
    if (bot + r.ch * r.odraz * 0.0f > r.popisY - r.popisPx * 0.7f) chyba("karta-pres-popis");
    if (r.tlacY - r.tlacH * 0.5f < r.popisY) chyba("tlacitko-pres-popis");
    if (r.teckyY + r.teckaR > o.h) chyba("tecky-mimo");
    if (r.podpisY > 0 && r.podpisY > o.h) chyba("podpis-mimo");
    if (r.ozubX + r.ozubR > o.w || r.ozubY - r.ozubR < 0) chyba("kolecko-mimo");
    // soused vykukuje (na vysku 6-16 % sirky)
    const Poza p1 = spoctiPozu(r, 1, 0.f);
    float x0 = 1e9f; for (int k = 0; k < 4; k++) x0 = std::min(x0, p1.rx[k]);
    const float vykuk = (o.w - x0) / o.w;
    if (!r.naSirku && (vykuk < 0.05f || vykuk > 0.18f)) chyba("soused-nevykukuje");
    const Poza p0 = spoctiPozu(r, 0, 0.f);
    float pr = -1e9f; for (int k = 0; k < 4; k++) pr = std::max(pr, p0.rx[k]);
    if (x0 < pr) chyba("soused-prekryva-stred");
    int videt = 0; for (int i = 0; i < 7; i++) if (spoctiPozu(r, i, 3.f).videt) videt++;
    over(dobre, o.jm, f("karta %.0fx%.0f, soused vykukuje %.1f %%, karet videt (stred 3) %.0f", r.cw, r.ch, vykuk * 100.f, (double)videt) + (proc.empty() ? "" : " PROBLEM: " + proc));
  }

  // ---------------- 3) fyzika ----------------
  std::printf("\n3) fyzika (telefon 1080x2340, 7 karet)\n");
  const Rozlozeni R = spoctiRozlozeni(1080, 2340, false, 7);
  {
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(R); k.nastavPrah(24.f);
    double t = 10.0; k.krok(t);
    // tah 1:1 - prst presne o rozestup doleva = presne o kartu dal (prah se odecte)
    k.dolu(900, 1000, t);
    for (int i = 1; i <= 60; i++) { t += 0.008; k.tah(900.f - (R.r0 + 24.f) * i / 60.f, 1000, t); k.krok(t); }
    over(std::fabs(k.poz() - 1.0f) < 1e-4f, "tah 1:1: prst o rozestup karet (+ prah) = presne o kartu", f("poz %.5f, rozestup %.0f bodu", k.poz(), R.r0));
    // karta pod prstem jede s prstem: stred prostredni karty se posunul presne o tah prstu
    {
      Karusel k2; k2.nastavPocet(7); k2.nastavRozlozeni(R); k2.nastavPrah(24.f);
      double t2 = 1; k2.krok(t2); k2.dolu(600, 1000, t2);
      t2 += 0.01; k2.tah(600 - 24 - 100, 1000, t2);
      const Poza p = spoctiPozu(R, 0, k2.poz());
      over(std::fabs((p.x - R.cx) - (-100.f)) < 0.05f, "karta pod prstem: prst -100 bodu = karta -100 bodu", f("posun karty %.2f", p.x - R.cx));
      k2.zrus();
    }
    // pomale pusteni (prst stal 120 ms) -> nejblizsi karta, bez prekmitu
    t += 0.12; k.tah(900.f - (R.r0 + 24.f), 1000, t);
    int a = k.nahoru(900.f - (R.r0 + 24.f), 1000, t + 0.005); t += 0.005;
    Beh b = dobehni(k, t);
    over(a == AKCE_NIC && k.poz() == 1.0f && b.maxPoz < 1.0005f && b.minPoz > 0.9995f, "pusteni bez rychlosti: zustane na karte 1", f("klid za %.0f ms", b.kdyKlid * 1000));
  }
  {
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(R); k.nastavPrah(24.f);
    double t = 10.0; k.krok(t);
    // tah o 0,3 karty a pomale pusteni -> zpet na 0
    tahni(k, t, 800, 800 - 24 - R.r0 * 0.30f, 0.4, 1000, 0.15);
    Beh b = dobehni(k, t);
    over(k.poz() == 0.f && b.kdyTemer < 0.36 && b.kdyKlid > 0 && b.kdyKlid < 0.75 && b.minPoz > -0.01f, "tah o 0,3 karty a pustit (stal) -> zpet na prvni kartu",
         f("pro oko hotovo za %.0f ms, klid za %.0f ms, nejmin %.4f", b.kdyTemer * 1000, b.kdyKlid * 1000, b.minPoz));
    // tah o 0,6 karty pomalu -> dal
    tahni(k, t, 800, 800 - 24 - R.r0 * 0.6f, 0.5, 1000, 0.15);
    b = dobehni(k, t);
    over(k.poz() == 1.f && b.maxPoz < 1.01f && b.kdyTemer < 0.40, "tah o 0,6 karty a pustit -> dalsi karta",
         f("pro oko hotovo za %.0f ms, klid za %.0f ms, nejvys %.4f", b.kdyTemer * 1000, b.kdyKlid * 1000, b.maxPoz));
    // kratke svihnuti (0,15 karty za 70 ms) -> dalsi karta
    tahni(k, t, 800, 800 - 24 - R.r0 * 0.15f, 0.07);
    b = dobehni(k, t);
    over(k.poz() == 2.f && b.obratu == 0 && b.kdyTemer < 0.45 && b.maxPoz < 2.01f, "kratke svihnuti (0,15 karty za 70 ms) -> dalsi karta, bez couvani a prekmitu",
         f("pro oko hotovo za %.0f ms, klid za %.0f ms, max %.2f karty/s", b.kdyTemer * 1000, b.kdyKlid * 1000, b.maxV));
    // svih zpet
    tahni(k, t, 300, 300 + 24 + R.r0 * 0.12f, 0.06);
    b = dobehni(k, t);
    over(k.poz() == 1.f, "svihnuti doprava -> predchozi karta", f("poz %.0f", k.poz()));
  }
  {
    // silny svih pres vic karet: plynule zpomaluje, nezrychli, nekmita, konci na karte
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(R); k.nastavPrah(24.f);
    double t = 10.0; k.krok(t);
    tahni(k, t, 1000, 1000 - 24 - R.r0 * 1.2f, 0.10);    // ~12 karet/s
    const float v0 = std::fabs(k.rychlost());
    const float p0 = k.poz();
    float maxV = 0, predV = v0; int zrychleni = 0; float predP = p0; bool monot = true;
    for (int i = 0; i < 240 && !k.vKlidu(); i++) {
      t += 1.0 / 60.0; k.krok(t);
      const float v = std::fabs(k.rychlost());
      if (v > predV * 1.02f + 0.05f) zrychleni++;
      predV = v; maxV = std::max(maxV, v);
      if (k.poz() < predP - 1e-4f) monot = false;
      predP = k.poz();
    }
    over(k.vKlidu() && std::fabs(k.poz() - std::round(k.poz())) < 1e-6f && k.poz() >= 3.f, "silny svih (~12 karet/s) dojede o nekolik karet a zastavi presne na karte",
         f("z %.2f na %.0f, rychlost pusteni %.1f karet/s", p0, k.poz(), v0));
    over(zrychleni == 0 && monot && maxV <= v0 * 1.02f + 0.05f, "silny svih: jen zpomaluje (zadne zrychleni ani couvani)", f("max %.2f karet/s", maxV));
    // svih az za konec: zastavi na posledni karte
    tahni(k, t, 1000, 1000 - 24 - R.r0 * 2.5f, 0.08);
    Beh b = dobehni(k, t);
    over(k.poz() == 6.f, "svih pres konec -> posledni karta (HELP)", f("poz %.0f, nejvys %.3f", k.poz(), b.maxPoz));
    // guma za koncem
    k.dolu(500, 1000, t);
    for (int i = 1; i <= 50; i++) { t += 0.008; k.tah(500.f - (24 + R.r0 * 1.5f) * i / 50.f, 1000, t); }
    const float za = k.poz() - 6.f;
    over(za > 0.15f && za < 0.6f, "guma za posledni kartou: tah o 1,5 karty = jen kousek", f("za koncem %.3f karty", za));
    t += 0.1; k.tah(500.f - (24 + R.r0 * 1.5f), 1000, t);
    k.nahoru(500.f - (24 + R.r0 * 1.5f), 1000, t + 0.01); t += 0.01;
    b = dobehni(k, t);
    over(k.poz() == 6.f && b.kdyKlid < 0.9, "pusteni za koncem -> zpet na posledni kartu", f("za %.0f ms", b.kdyKlid * 1000));
    // guma pred zacatkem
    k.skoc(0);
    k.dolu(300, 1000, t);
    for (int i = 1; i <= 50; i++) { t += 0.008; k.tah(300.f + (24 + R.r0) * i / 50.f, 1000, t); }
    over(k.poz() < -0.1f && k.poz() > -0.45f, "guma pred prvni kartou", f("poz %.3f", k.poz()));
    k.nahoru(300.f + 24 + R.r0, 1000, t + 0.01); t += 0.01;
    b = dobehni(k, t);
    over(k.poz() == 0.f, "pusteni pred zacatkem -> prvni karta");
  }
  {
    // 30 a 120 snimku/s: stejny vysledek (fyzika podle casu, ne podle snimku)
    float vysl[2]; double klid[2];
    for (int q = 0; q < 2; q++) {
      Karusel k; k.nastavPocet(7); k.nastavRozlozeni(R); k.nastavPrah(24.f);
      double t = 10.0; k.krok(t);
      tahni(k, t, 1000, 1000 - 24 - R.r0 * 0.9f, 0.12);
      const double krokS = q == 0 ? 1.0 / 30 : 1.0 / 120;
      const double t0 = t;
      while (!k.vKlidu() && t - t0 < 3) { t += krokS; k.krok(t); }
      vysl[q] = k.poz(); klid[q] = t - t0;
    }
    over(vysl[0] == vysl[1] && std::fabs(klid[0] - klid[1]) < 0.05, "30 i 120 snimku/s: stejna karta, stejny cas", f("karta %.0f / %.0f, %.0f / %.0f ms", vysl[0], vysl[1], klid[0] * 1000, klid[1] * 1000));
  }

  // ---------------- 4) klepnuti ----------------
  std::printf("\n4) klepnuti a klavesy\n");
  {
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(R); k.nastavPrah(24.f);
    double t = 10.0; k.krok(t);
    k.dolu(R.cx, R.cy, t); t += 0.08;
    int a = k.nahoru(R.cx + 3, R.cy + 2, t);
    over(a == AKCE_KARTA + 0, "klepnuti na prostredni kartu = spustit ATARI", f("akce %.0f", a));
    // soused vpravo (vykukuje u kraje)
    k.dolu(1070, R.cy, t); t += 0.08;
    a = k.nahoru(1070, R.cy, t);
    over(a == AKCE_NIC, "klepnuti na kartu vedle = nespusti");
    Beh b = dobehni(k, t);
    over(k.poz() == 1.f, "... ale dojede na ni", f("poz %.0f za %.0f ms", k.poz(), b.kdyKlid * 1000));
    // kolecko
    k.dolu(R.ozubX + R.ozubR * 0.8f, R.ozubY, t); t += 0.06;
    a = k.nahoru(R.ozubX + R.ozubR * 0.8f, R.ozubY, t);
    over(a == AKCE_NASTAVENI, "klepnuti na kolecko = OPTIONS");
    // tlacitko pod kartami
    k.dolu(R.tlacX, R.tlacY, t); t += 0.06;
    a = k.nahoru(R.tlacX, R.tlacY, t);
    over(a == AKCE_KARTA + 1, "tlacitko pod kartami = spustit prostredni (SEGA)", f("akce %.0f", a));
    // tecka posledni karty
    const float xt = R.cx + R.teckaKrok * 3.f;
    k.dolu(xt, R.teckyY, t); t += 0.06;
    a = k.nahoru(xt, R.teckyY, t);
    b = dobehni(k, t);
    over(a == AKCE_NIC && k.poz() == 6.f, "klepnuti na posledni tecku -> dojede na HELP", f("poz %.0f", k.poz()));
    // dlouhe drzeni = neni klepnuti
    k.dolu(R.cx, R.cy, t); t += 0.9;
    a = k.nahoru(R.cx, R.cy, t);
    over(a == AKCE_NIC, "drzeni prstu 0,9 s neni klepnuti");
    // prst na jedoucim karuselu ho zastavi a nespusti
    k.skoc(0);
    tahni(k, t, 1000, 1000 - 24 - R.r0 * 0.5f, 0.06);
    t += 0.05; k.krok(t);
    const float jede = std::fabs(k.rychlost());
    k.dolu(R.cx, R.cy, t); t += 0.06;
    a = k.nahoru(R.cx, R.cy, t);
    b = dobehni(k, t);
    over(jede > Karusel::ZACHYT && a == AKCE_NIC && std::fabs(k.poz() - std::round(k.poz())) < 1e-6f,
         "prst na jedoucim karuselu: zastavi, nic nespusti, dojede na kartu", f("jel %.1f karet/s", jede));
    // svisly pohyb prstu karuselem nehne
    k.skoc(2);
    k.dolu(500, 900, t);
    for (int i = 1; i <= 30; i++) { t += 0.008; k.tah(500 + i * 0.3f, 900 + i * 15.f, t); }
    over(k.poz() == 2.f, "svisly tah prstem karuselem nehne", f("poz %.3f", k.poz()));
    k.nahoru(509, 1350, t + 0.01); t += 0.01; dobehni(k, t);
  }
  {
    Karusel k; k.nastavPocet(7); k.nastavRozlozeni(R); k.nastavPrah(24.f);
    double t = 10.0; k.krok(t);
    k.klavesa(KL_VPRAVO); k.klavesa(KL_VPRAVO); k.klavesa(KL_VPRAVO);
    over(k.cil() == 3.f, "3x sipka doprava rychle za sebou = cil 3. karta dal", f("cil %.0f", k.cil()));
    int a = k.klavesa(KL_POTVRD);
    over(a == AKCE_NIC, "Enter, kdyz karusel jeste jede daleko = nic");
    Beh b = dobehni(k, t);
    a = k.klavesa(KL_POTVRD);
    over(k.poz() == 3.f && a == AKCE_KARTA + 3, "dojede na MP3 PREHRAVAC, Enter = spustit", f("za %.0f ms, akce %.0f", b.kdyKlid * 1000, a));
    k.klavesa(KL_KONEC); dobehni(k, t);
    over(k.poz() == 6.f, "End = posledni karta (HELP)");
    k.klavesa(KL_VPRAVO); dobehni(k, t);
    over(k.poz() == 6.f, "sipka doprava na konci = zustane");
    k.klavesa(KL_ZACATEK); dobehni(k, t);
    over(k.poz() == 0.f, "Home = prvni karta");
  }
  {
    // jedna karta a 15 karet (budouci rozsireni) - zadna zmena kodu
    Karusel k; k.nastavPocet(1); k.nastavRozlozeni(spoctiRozlozeni(1080, 2340, false, 1)); k.nastavPrah(24.f);
    double t = 10; k.krok(t);
    tahni(k, t, 900, 300, 0.1);
    dobehni(k, t);
    k.dolu(R.cx, R.cy, t); t += 0.05;
    int a = k.nahoru(R.cx, R.cy, t);
    over(k.poz() == 0.f && a == AKCE_KARTA, "1 karta: svih nic nerozbije, klepnuti spusti");
    Karusel m; m.nastavPocet(15); m.nastavRozlozeni(spoctiRozlozeni(1080, 2340, false, 15)); m.nastavPrah(24.f);
    m.krok(t);
    for (int i = 0; i < 4; i++) { tahni(m, t, 1000, 1000 - 24 - R.r0 * 1.0f, 0.06); }
    dobehni(m, t);
    over(m.poz() >= 6.f && m.poz() <= 14.f, "15 karet: svihy jedou dal (zadna pevna mez 7)", f("poz %.0f", m.poz()));
    m.klavesa(KL_KONEC); dobehni(m, t);
    over(m.poz() == 14.f, "15 karet: End = 15. karta");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", ok, chyb);
  return chyb ? 1 : 0;
}
