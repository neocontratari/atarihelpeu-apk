// nap_menu_karusel.h
// ====================================================================
//  B311: KARUSEL HLAVNIHO MENU - fyzika, rozlozeni, dotyk a klavesy.
//
//  Bez OpenGL a bez Androidu (testuje se na PC, test_overeni/b311).
//  Pocet karet je libovolny - vsechno se pocita z "n" (pocet radku
//  v karty.txt), nikde neni pevny seznam.
//
//  Poloha karuselu "poz" je desetinne cislo: 0 = prvni karta uprostred,
//  1.5 = mezi druhou a treti. Prst tahne 1:1 (karta pod prstem jede presne
//  s nim), po pusteni:
//   - rychlost prstu (prumer pres poslednich ~70 ms, metoda nejmensich
//     ctvercu) se promitne dopredu (0,2 s) a karusel dojede na nejblizsi
//     kartu k tomu mistu; svizne svihnuti posune aspon o jednu kartu,
//   - dojezd je kriticky tlumena pruzina, ktera navazuje na rychlost prstu
//     (zadne cuknuti) a nezrychluje - tuhost se voli podle rychlosti,
//   - za prvni a posledni kartou "guma" (tahne se ztuha, po pusteni zpet).
//  Klepnuti: na prostredni kartu = spustit, na kartu vedle = dojet na ni,
//  na kolecko = OPTIONS, na tlacitko pod kartami = spustit, na tecku = dojet.
//  Prst polozeny na jedouci karusel ho zastavi (a klepnuti se nepocita).
// ====================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace napmenu {

// co ma Java udelat (navratova hodnota dotyku a klaves)
enum Akce {
  AKCE_NIC = 0,
  AKCE_KARTA = 1000,        // + index karty: spustit (po animaci)
  AKCE_NASTAVENI = 2000,    // kolecko OPTIONS
  AKCE_ZAMCENO = 3000,      // + index: klepnuti na zamcenou kartu (C++ ukaze proc)
};

enum Klavesa { KL_NIC = 0, KL_VLEVO = 1, KL_VPRAVO = 2, KL_POTVRD = 3, KL_ZACATEK = 4, KL_KONEC = 5 };

inline float hladce01(float x) {
  x = x < 0.f ? 0.f : (x > 1.f ? 1.f : x);
  return x * x * (3.f - 2.f * x);
}

// ------------------------------------------------------------------
//  Rozlozeni obrazovky (v bodech plochy, y dolu)
// ------------------------------------------------------------------
struct Rozlozeni {
  int W = 0, H = 0;
  bool naSirku = false, tv = false;
  float cw = 0, ch = 0;               // prostredni karta
  float cx = 0, cy = 0;               // jeji stred
  float r0 = 0, r1 = 0;               // rozestup stredu karet (|d| <= 1 a dal)
  float s1 = 0.84f, s2 = 0.70f;       // meritko karty o 1 a o 2 vedle
  float uhel1 = 0.30f;                // natoceni karty vedle (rad)
  float persp = 0;                    // perspektiva (vzdalenost oka v bodech)
  float dosah = 2.6f;                 // od jakeho |d| uz karta neni videt
  float odraz = 0.22f;                // vyska odrazu (dil vysky karty)
  // hlavicka
  float logoX = 0, logoY = 0, logoR = 0;
  float titulY = 0, titulPx = 0, sloganY = 0, sloganPx = 0;
  float ozubX = 0, ozubY = 0, ozubR = 0;
  // pod kartami
  float popisY = 0, popisW = 0, popisPx = 0;
  float tlacX = 0, tlacY = 0, tlacW = 0, tlacH = 0, tlacPx = 0;
  float teckyY = 0, teckaR = 0, teckaKrok = 0;
  float podpisY = 0, podpisPx = 0;
  float napovedaY = 0;                // TV: radek s klavesami
};

inline Rozlozeni spoctiRozlozeni(int W, int H, bool tv, int nKaret) {
  Rozlozeni r;
  r.W = W; r.H = H; r.tv = tv;
  const float w = (float)std::max(W, 1), h = (float)std::max(H, 1);
  const float S = std::min(w, h);
  r.naSirku = w > h * 0.82f;
  const float POMER = 0.75f;          // sirka : vyska karty
  if (!r.naSirku) {
    // na vysku: velka karta pres skoro celou sirku, sousedky vykukuji z kraje
    r.cw = w * 0.76f;
    r.ch = r.cw / POMER;
    const float maxCh = h * 0.52f;
    if (r.ch > maxCh) { r.ch = maxCh; r.cw = r.ch * POMER; }
    r.logoR = std::min(w * 0.072f, h * 0.034f);
    r.logoX = w * 0.5f;
    r.logoY = h * 0.012f + r.logoR * 1.05f;
    r.titulPx = std::min(w * 0.105f, h * 0.05f);
    r.titulY = r.logoY + r.logoR * 1.05f + r.titulPx * 0.62f;
    r.sloganPx = r.titulPx * 0.30f;
    r.sloganY = r.titulY + r.titulPx * 0.62f + r.sloganPx * 0.7f;
    r.ozubR = std::min(w * 0.058f, h * 0.03f);
    r.ozubX = w - r.ozubR * 1.9f;
    r.ozubY = r.logoY;
    const float hlavaKonec = r.sloganY + r.sloganPx * 1.2f;
    // pod kartami: popis (2 radky), tlacitko, tecky, podpis
    r.popisPx = std::min(w * 0.040f, h * 0.019f);
    r.tlacPx = std::min(w * 0.044f, h * 0.021f);
    r.tlacH = r.tlacPx * 2.35f;
    r.teckaR = std::min(w * 0.011f, h * 0.0055f);
    r.podpisPx = std::min(w * 0.040f, h * 0.019f);
    const float spodek = r.popisPx * 1.35f * 2.f + r.popisPx * 0.9f + r.tlacH + r.teckaR * 7.f + r.podpisPx * 1.6f;
    // karta uprostred mezi hlavickou a spodkem
    float volno = h - hlavaKonec - spodek;
    if (r.ch > volno * 0.86f) { r.ch = volno * 0.86f; r.cw = r.ch * POMER; }
    r.cx = w * 0.5f;
    r.cy = hlavaKonec + (volno - r.ch) * 0.30f + r.ch * 0.5f;
    float y = r.cy + r.ch * 0.5f + std::max(r.popisPx * 1.2f, (volno - r.ch) * 0.42f);
    r.popisY = y + r.popisPx * 0.7f;               // stred prvniho radku
    r.popisW = w * 0.86f;
    y = r.popisY + r.popisPx * 1.35f * 2.f + r.popisPx * 0.2f;
    r.tlacW = std::max(r.cw * 0.52f, r.tlacPx * 8.f);
    r.tlacX = w * 0.5f;
    r.tlacY = y + r.tlacH * 0.5f;
    r.teckyY = r.tlacY + r.tlacH * 0.5f + r.teckaR * 3.6f;
    r.teckaKrok = r.teckaR * 3.4f;
    r.podpisY = std::min(h - r.podpisPx * 0.9f, r.teckyY + r.teckaR * 2.f + r.podpisPx * 1.4f);
    r.odraz = 0.13f;
    r.s1 = 0.84f; r.s2 = 0.72f;
    r.uhel1 = 0.32f;
    r.dosah = 2.4f;
  } else {
    // na sirku (tablet, telefon na boku, TV 16:9): mensi karty, vic jich je videt
    r.logoR = h * (tv ? 0.060f : 0.052f);
    r.logoX = w * 0.5f - h * 0.40f;
    r.logoY = h * 0.018f + r.logoR * 1.02f;
    r.titulPx = h * 0.072f;
    r.titulY = r.logoY - r.titulPx * 0.12f;
    r.sloganPx = r.titulPx * 0.30f;
    r.sloganY = r.titulY + r.titulPx * 0.55f + r.sloganPx * 0.5f;
    r.ozubR = h * 0.045f;
    r.ozubX = w - r.ozubR * 2.0f;
    r.ozubY = r.logoY;
    const float hlavaKonec = r.logoY + r.logoR * 1.08f;
    r.popisPx = h * 0.034f;
    r.tlacPx = h * 0.037f;
    r.tlacH = r.tlacPx * 2.2f;
    r.teckaR = h * 0.0095f;
    r.podpisPx = h * 0.03f;
    const float spodek = r.popisPx * 1.35f * 1.f + r.popisPx * 0.8f + r.tlacH + r.teckaR * 6.f + (tv ? r.popisPx * 1.6f : 0.f);
    const float volno = h - hlavaKonec - spodek;
    r.ch = volno * 0.90f;
    r.cw = r.ch * POMER;
    r.cx = w * 0.5f;
    r.cy = hlavaKonec + volno * 0.47f;
    float y = r.cy + r.ch * 0.5f + volno * 0.05f;
    r.popisY = y + r.popisPx * 0.7f;
    r.popisW = std::min(w * 0.80f, r.cw * 3.2f);
    y = r.popisY + r.popisPx * 1.0f;
    r.tlacW = std::max(r.cw * 0.80f, r.tlacPx * 8.f);
    r.tlacX = w * 0.5f;
    r.tlacY = y + r.tlacH * 0.5f;
    r.teckyY = r.tlacY + r.tlacH * 0.5f + r.teckaR * 2.8f;
    r.teckaKrok = r.teckaR * 3.4f;
    r.napovedaY = h - r.popisPx * 0.9f;
    r.podpisY = 0.f;                                  // na sirku bez podpisu (neni misto)
    r.odraz = 0.f;
    r.s1 = 0.80f; r.s2 = 0.66f;
    r.uhel1 = 0.36f;
    r.dosah = 3.4f;
  }
  // rozestup: soused tesne vedle prostredni karty (s malou mezerou)
  const float mez = r.cw * (r.naSirku ? 0.06f : 0.045f);
  r.r0 = r.cw * 0.5f + r.cw * r.s1 * 0.5f * std::cos(r.uhel1) + mez;
  r.r1 = r.cw * r.s2 * (r.naSirku ? 1.02f : 0.82f) * std::cos(r.uhel1);
  r.persp = std::max(w, h) * 2.2f;
  (void)S;
  (void)nKaret;
  return r;
}

// ------------------------------------------------------------------
//  Poloha jedne karty na obrazovce
// ------------------------------------------------------------------
struct Poza {
  int i = 0;
  float d = 0;                // vzdalenost od stredu (v kartach, + vpravo)
  float x = 0, y = 0;         // stred (body)
  float s = 1;                // meritko
  float uhel = 0;             // natoceni kolem svisle osy (rad), + = pravy okraj dozadu
  float jas = 1, alfa = 1;
  bool videt = false;
  // rohy po promitnuti: 0 vlevo nahore, 1 vpravo nahore, 2 vpravo dole, 3 vlevo dole
  float rx[4] = {0, 0, 0, 0}, ry[4] = {0, 0, 0, 0}, rw[4] = {1, 1, 1, 1};
};

// bod roviny karty (u, v v -1..1, v dolu) -> obrazovka; w pro perspektivne spravnou texturu
inline void promitni(const Rozlozeni &r, const Poza &p, float u, float v, float &X, float &Y, float &W) {
  const float hw = r.cw * p.s * 0.5f, hh = r.ch * p.s * 0.5f;
  const float lx = u * hw;
  const float x = lx * std::cos(p.uhel);
  const float z = -lx * std::sin(p.uhel);
  const float k = r.persp / (r.persp - z);
  X = r.cx + (p.x - r.cx + x) * k;
  Y = r.cy + (p.y - r.cy + v * hh) * k;
  W = 1.f / k;
}

inline Poza spoctiPozu(const Rozlozeni &r, int i, float poz) {
  Poza p;
  p.i = i;
  p.d = (float)i - poz;
  const float a = std::fabs(p.d), sg = p.d < 0.f ? -1.f : 1.f;
  float x = a <= 1.f ? a * r.r0 : r.r0 + (a - 1.f) * r.r1;
  p.x = r.cx + sg * x;
  p.y = r.cy;
  if (a <= 1.f) p.s = 1.f - (1.f - r.s1) * hladce01(a);
  else if (a <= 2.f) p.s = r.s1 + (r.s2 - r.s1) * (a - 1.f);
  else p.s = std::max(0.3f, r.s2 - (a - 2.f) * 0.08f);
  // natoceni roste do |d| = 1, pak uz jen malo (vzdalene karty jako na valci)
  p.uhel = sg * r.uhel1 * (a <= 1.f ? hladce01(a) * 0.35f + a * 0.65f : 1.f + (std::min(a, 3.f) - 1.f) * 0.18f);
  p.jas = 1.f - 0.40f * hladce01(std::min(a, 1.f)) - 0.18f * std::min(std::max(a - 1.f, 0.f), 1.f);
  p.alfa = 1.f - hladce01((a - (r.dosah - 0.5f)) / 0.5f);
  p.videt = p.alfa > 0.002f;
  for (int k = 0; k < 4; k++) {
    const float u = (k == 0 || k == 3) ? -1.f : 1.f, v = (k < 2) ? -1.f : 1.f;
    promitni(r, p, u, v, p.rx[k], p.ry[k], p.rw[k]);
  }
  if (p.videt) {
    float x0 = p.rx[0], x1 = p.rx[0];
    for (int k = 1; k < 4; k++) { x0 = std::min(x0, p.rx[k]); x1 = std::max(x1, p.rx[k]); }
    if (x1 < 0.f || x0 > (float)r.W) p.videt = false;
  }
  return p;
}

// bod uvnitr ctyruhelniku (konvexni, rohy po smeru nebo proti)
inline bool vCtyruhelniku(const float *X, const float *Y, float px, float py) {
  int kladne = 0, zaporne = 0;
  for (int k = 0; k < 4; k++) {
    const int j = (k + 1) & 3;
    const float c = (X[j] - X[k]) * (py - Y[k]) - (Y[j] - Y[k]) * (px - X[k]);
    if (c > 0) kladne++;
    else if (c < 0) zaporne++;
  }
  return kladne == 0 || zaporne == 0;
}

// ------------------------------------------------------------------
//  Fyzika a vstup
// ------------------------------------------------------------------
class Karusel {
 public:
  enum Stav { KLID = 0, TAH = 1, DOJEZD = 2 };

  // ladeni (v kartach a sekundach)
  static constexpr float PROJEKCE = 0.20f;       // jak daleko dopredu se rychlost promitne
  static constexpr float SVIH = 0.55f;           // od teto rychlosti (karet/s) svih posune aspon o kartu
  static constexpr float RYCH_MAX = 28.f;        // karet/s
  static constexpr float OMEGA_MIN = 7.f, OMEGA_KRATKY = 15.f, OMEGA_MAX = 22.f, OMEGA_KLAVESA = 15.f, OMEGA_ZPET = 13.f;
  static constexpr float ZACHYT = 0.25f;         // prst na karuselu jedoucim rychleji = jen zastavit
  static constexpr double KLEPNUTI_MAX_S = 0.6;  // delsi drzeni neni klepnuti

  void nastavPocet(int n) {
    n_ = std::max(1, n);
    poz_ = std::min(std::max(poz_, 0.f), (float)(n_ - 1));
    cil_ = (float)std::lround(std::min(std::max(cil_, 0.f), (float)(n_ - 1)));
  }
  int pocet() const { return n_; }
  void nastavRozlozeni(const Rozlozeni &r) { roz_ = r; }
  const Rozlozeni &rozlozeni() const { return roz_; }
  void nastavPrah(float px) { prah_ = std::max(2.f, px); }   // touch slop (Java: ViewConfiguration)

  float poz() const { return poz_; }
  float rychlost() const { return rych_; }
  Stav stav() const { return stav_; }
  int vybrana() const { return std::min(std::max((int)std::lround(poz_), 0), n_ - 1); }
  bool vKlidu() const { return stav_ == KLID; }
  bool prstDole() const { return dole_; }

  // okamzite na kartu (zacatek, navrat z emulatoru)
  void skoc(int i) {
    poz_ = cil_ = (float)std::min(std::max(i, 0), n_ - 1);
    rych_ = 0.f;
    stav_ = KLID;
    dole_ = false;
  }

  // ---- fyzika: posun do casu t (sekundy) ----
  void krok(double t) {
    if (casPosl_ < 0.0) { casPosl_ = t; return; }
    double dt = t - casPosl_;
    casPosl_ = t;
    if (dt <= 0.0) return;
    if (dt > 0.1) dt = 0.1;                // appka se zasekla - neskakat
    if (stav_ == DOJEZD) {
      // kriticky tlumena pruzina, po malych krocich (presne i pri 30 snimcich/s)
      int kroku = (int)std::ceil(dt / 0.002);
      const float h = (float)(dt / kroku);
      for (int k = 0; k < kroku; k++) {
        const float e = poz_ - cil_;
        const float a = -omega_ * omega_ * e - 2.f * omega_ * rych_;
        rych_ += a * h;
        poz_ += rych_ * h;
      }
      // klid: pod 1/1000 karty (pod bod) a skoro stoji - pak presne na kartu
      if (std::fabs(poz_ - cil_) < 0.001f && std::fabs(rych_) < 0.012f) {
        poz_ = cil_;
        rych_ = 0.f;
        stav_ = KLID;
      }
    }
  }

  // ---- dotyk (x, y v bodech plochy, t sekundy) ----
  void dolu(float x, float y, double t) {
    dole_ = true;
    tazeno_ = false;
    zachyceno_ = std::fabs(rych_) > ZACHYT && stav_ == DOJEZD;
    x0_ = x; y0_ = y; t0_ = t;
    pozStart_ = poz_;
    stav_ = TAH;                         // karusel stoji pod prstem
    rych_ = 0.f;
    vzorku_ = 0;
    pridejVzorek(x, t);
  }

  void tah(float x, float y, double t) {
    if (!dole_) return;
    (void)y;
    pridejVzorek(x, t);
    const float dx = x - x0_;
    if (!tazeno_) {
      if (std::fabs(dx) < prah_) return;
      tazeno_ = true;
      // bez cuknuti: prah se odecte (karta se rozjede od nuly)
      x0_ += dx > 0 ? prah_ : -prah_;
      pozStart_ = poz_;
    }
    syrova_ = pozStart_ - (x - x0_) / std::max(roz_.r0, 1.f);
    poz_ = guma(syrova_);
  }

  // vraci Akce (+ index karty)
  int nahoru(float x, float y, double t) {
    if (!dole_) return AKCE_NIC;
    dole_ = false;
    pridejVzorek(x, t);
    if (tazeno_) {
      pust(-odhadRychlosti() / std::max(roz_.r0, 1.f));
      return AKCE_NIC;
    }
    // klepnuti
    const bool kratke = (t - t0_) <= KLEPNUTI_MAX_S;
    if (zachyceno_ || !kratke) { dojedNa((float)vybranaZ(poz_), 0.f, OMEGA_ZPET); return AKCE_NIC; }
    return klepnuti(x, y);
  }

  void zrus() {
    if (!dole_) return;
    dole_ = false;
    pust(0.f);
  }

  // ---- klavesy (sipky, Enter) ----
  int klavesa(int kl) {
    if (dole_) return AKCE_NIC;
    const int zaklad = stav_ == DOJEZD ? (int)std::lround(cil_) : vybrana();
    switch (kl) {
      case KL_VLEVO: dojedNa((float)std::max(0, zaklad - 1), rych_, OMEGA_KLAVESA); return AKCE_NIC;
      case KL_VPRAVO: dojedNa((float)std::min(n_ - 1, zaklad + 1), rych_, OMEGA_KLAVESA); return AKCE_NIC;
      case KL_ZACATEK: dojedNa(0.f, rych_, OMEGA_KLAVESA); return AKCE_NIC;
      case KL_KONEC: dojedNa((float)(n_ - 1), rych_, OMEGA_KLAVESA); return AKCE_NIC;
      case KL_POTVRD: {
        const int i = stav_ == DOJEZD ? (int)std::lround(cil_) : vybrana();
        if (std::fabs(poz_ - (float)i) > 0.35f) return AKCE_NIC;     // jeste jede
        if (stav_ == DOJEZD) { poz_ = cil_ = (float)i; rych_ = 0; stav_ = KLID; }
        return AKCE_KARTA + i;
      }
      default: return AKCE_NIC;
    }
  }

  // dojet na kartu i (klepnuti na kartu vedle, na tecku)
  void dojedNa(float i, float v0, float omega) {
    cil_ = std::min(std::max(std::round(i), 0.f), (float)(n_ - 1));
    rych_ = std::max(-RYCH_MAX, std::min(RYCH_MAX, v0));
    omega_ = omega;
    stav_ = DOJEZD;
  }

  // ---- co je pod prstem (pro klepnuti) ----
  // -1 = nic, jinak index karty (prostredni karta ma prednost - lezi navrchu)
  int kartaNa(float x, float y) const {
    int nej = -1;
    float nejA = 1e9f;
    for (int i = 0; i < n_; i++) {
      const Poza p = spoctiPozu(roz_, i, poz_);
      if (!p.videt || p.alfa < 0.3f) continue;
      if (!vCtyruhelniku(p.rx, p.ry, x, y)) continue;
      const float a = std::fabs(p.d);
      if (a < nejA) { nejA = a; nej = i; }
    }
    return nej;
  }

  // ---- pro test: stav pruziny ----
  float cil() const { return cil_; }
  float omega() const { return omega_; }

 private:
  int n_ = 1;
  Rozlozeni roz_;
  float prah_ = 20.f;
  float poz_ = 0.f, rych_ = 0.f, cil_ = 0.f, omega_ = 14.f;
  Stav stav_ = KLID;
  double casPosl_ = -1.0;
  // prst
  bool dole_ = false, tazeno_ = false, zachyceno_ = false;
  float x0_ = 0, y0_ = 0, pozStart_ = 0, syrova_ = 0;
  double t0_ = 0;
  static constexpr int VZORKU = 24;
  float vx_[VZORKU] = {};
  double vt_[VZORKU] = {};
  int vzorku_ = 0, vzHlava_ = 0;

  int vybranaZ(float p) const { return std::min(std::max((int)std::lround(p), 0), n_ - 1); }

  void pridejVzorek(float x, double t) {
    vx_[vzHlava_] = x;
    vt_[vzHlava_] = t;
    vzHlava_ = (vzHlava_ + 1) % VZORKU;
    if (vzorku_ < VZORKU) vzorku_++;
  }

  // rychlost prstu v bodech/s: primka nejmensich ctvercu pres vzorky z poslednich 70 ms
  // (prst, ktery se pred zvednutim zastavil, ma rychlost ~0)
  float odhadRychlosti() const {
    if (vzorku_ < 2) return 0.f;
    const int posl = (vzHlava_ + VZORKU - 1) % VZORKU;
    const double tk = vt_[posl];
    double st = 0, sx = 0, stt = 0, stx = 0;
    int m = 0;
    for (int k = 0; k < vzorku_; k++) {
      const int j = (posl - k + VZORKU) % VZORKU;
      const double dt = vt_[j] - tk;           // <= 0
      if (dt < -0.070) break;
      st += dt; sx += vx_[j]; stt += dt * dt; stx += dt * vx_[j];
      m++;
    }
    if (m < 2) return 0.f;
    const double jm = m * stt - st * st;
    if (jm < 1e-9) return 0.f;
    return (float)((m * stx - st * sx) / jm);
  }

  // guma za okrajem: ztuha, nejvys asi o kartu
  float guma(float p) const {
    const float maxP = (float)(n_ - 1);
    auto g = [](float o) { return (1.f - 1.f / (o * 0.55f + 1.f)); };
    if (p < 0.f) return -g(-p);
    if (p > maxP) return maxP + g(p - maxP);
    return p;
  }

  void pust(float v) {
    v = std::max(-RYCH_MAX, std::min(RYCH_MAX, v));
    const float maxP = (float)(n_ - 1);
    if (poz_ < 0.f || poz_ > maxP) {
      // za okrajem: zpet na krajni kartu (rychlost od prstu ven se nepouzije)
      dojedNa(poz_ < 0.f ? 0.f : maxP, (poz_ < 0.f) == (v < 0.f) ? 0.f : v, OMEGA_ZPET);
      return;
    }
    const float prom = poz_ + v * PROJEKCE;
    float c = std::round(prom);
    if (std::fabs(v) > SVIH) {
      // svih: aspon na dalsi kartu ve smeru prstu
      if (v > 0.f) c = std::max(c, std::ceil(poz_ + 0.02f));
      else c = std::min(c, std::floor(poz_ - 0.02f));
    }
    c = std::min(std::max(c, 0.f), maxP);
    // tuhost podle rychlosti: dlouhy svih nesmi zrychlovat (2*v/D), kratky musi
    // "cvaknout" (nejvys par set ms) - prst, ktery stal, = pevna tuhost
    const float D = c - poz_;
    float om = OMEGA_ZPET;
    if (std::fabs(v) >= 0.05f && std::fabs(D) > 1e-3f && v * D > 0.f)
      om = std::min(OMEGA_MAX, std::max(std::fabs(D) <= 1.2f ? OMEGA_KRATKY : OMEGA_MIN, 2.f * v / D));
    dojedNa(c, v, om);
  }

  int klepnuti(float x, float y) {
    const Rozlozeni &r = roz_;
    // kolecko OPTIONS (s rezervou pro prst)
    if (r.ozubR > 0.f) {
      const float dx = x - r.ozubX, dy = y - r.ozubY, rr = r.ozubR * 1.55f;
      if (dx * dx + dy * dy <= rr * rr) { dojedNa((float)vybrana(), 0.f, OMEGA_ZPET); return AKCE_NASTAVENI; }
    }
    const int i = kartaNa(x, y);
    const int stred = vybrana();
    if (i >= 0) {
      if (i == stred && std::fabs(poz_ - (float)i) < 0.2f) {
        poz_ = cil_ = (float)i; rych_ = 0; stav_ = KLID;
        return AKCE_KARTA + i;
      }
      dojedNa((float)i, 0.f, OMEGA_ZPET);
      return AKCE_NIC;
    }
    // tlacitko pod kartami = prostredni karta
    if (r.tlacW > 0.f && std::fabs(x - r.tlacX) <= r.tlacW * 0.6f && std::fabs(y - r.tlacY) <= r.tlacH * 0.85f) {
      poz_ = cil_ = (float)stred; rych_ = 0; stav_ = KLID;
      return AKCE_KARTA + stred;
    }
    // tecky
    if (r.teckaKrok > 0.f && std::fabs(y - r.teckyY) <= std::max(r.teckaR * 4.f, r.teckaKrok * 0.8f)) {
      const float x0 = r.cx - r.teckaKrok * (float)(n_ - 1) * 0.5f;
      const int k = (int)std::lround((x - x0) / r.teckaKrok);
      if (k >= 0 && k < n_ && std::fabs(x - (x0 + k * r.teckaKrok)) <= r.teckaKrok * 0.6f) {
        dojedNa((float)k, 0.f, OMEGA_ZPET);
        return AKCE_NIC;
      }
    }
    dojedNa((float)stred, 0.f, OMEGA_ZPET);
    return AKCE_NIC;
  }
};

}  // namespace napmenu
