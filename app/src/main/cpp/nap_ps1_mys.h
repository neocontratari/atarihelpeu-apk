// nap_ps1_mys.h
// =====================================================================
//  B317: MYS PLAYSTATION (SCPH-1090) PRO PS1
//
//  Rene: "U PS1 - slo by napriklad u hry dune2000 pouzit i pres obrazovku
//  mys?" Dune 2000 (SLES-02250) mys PlayStation oficialne podporuje
//  (leve tlacitko = potvrdit, prave = zrusit).
//
//  Jadro PCSX-ReARMed mys umi (frontend/libretro.c update_input_mouse,
//  libpcsxcore/pad.c: ovladac s ID $12, tlacitka, posun X a Y v rozsahu
//  -128..127 za jedno cteni). Tady je vsechno, co mys "drzi v ruce":
//   - DOTYK NA OBRAZE HRY jako touchpad notebooku:
//       tah prstem          = pohyb kurzoru (cela sirka obrazu = 320 bodu
//                             mysi x citlivost - jako by obraz byl podlozka)
//       tuknuti             = klik levym tlacitkem
//       tuknuti dvema prsty = klik pravym tlacitkem
//       prst podrzet        = leve tlacitko DRZI (tahem pak vyber jednotek
//                             ramem), pusti se se zvednutim prstu
//   - KDYZ JE MYS SAMA (port 1, port 2 prazdny - ovladac neni zapojeny,
//     jeho tlacitka by jinak nic nedelala): KRIZEK = leve a KOLECKO = prave
//     tlacitko mysi (drzena, i spolu s tahem prstu), SMER (D-pad, packa,
//     sipky na klavesnici TV) = pomaly presny posun kurzoru, ktery se
//     drzenim zrychluje. Na obrazovce, na hernim ovladaci i na klavesnici
//     TV (sipky / WASD, K / mezernik, L). S ovladacem v druhem portu patri
//     vsechna tlacitka ovladaci, jako drive.
//   - MYS POCITACE (prohlizec na TV / PC): pohyb a obe tlacitka,
//   - KAM JE MYS ZAPOJENA (rezim): port 1 (port 2 prazdny - jako radi
//     emulator ePSXe pro Dune 2000), port 1 + ovladac v portu 2, nebo
//     port 2 + ovladac v portu 1 - hry se lisi, kde mys hledaji,
//   - zapamatovani rezimu a citlivosti PRO KAZDOU HRU (podle ID disku).
//
//  B318: Rene - "mys sice funguje ale je dost neomalena i pri nastaveni
//  citlivosti na 300 je to divny". Z jeho logu: Dune 2000 kresli presne
//  20 obrazu za sekundu (kazdy 3. snimek), tahy dlouhe, jedno nechtene
//  podrzeni. Zmeny:
//   - ZADNY SKOK NA ZACATKU TAHU: prvnich 1,5 % sirky obrazu kurzor stoji
//     (tuknuti je presne), pak se rozjede plynule od mista, kde prst je.
//     B317 stal az do 3 % a pak naraz poskocil o cele 3 % (pri 295 % o 28
//     bodu).
//   - ZRYCHLENI podle rychlosti prstu jako u mysi pocitace: pomaly tah =
//     jemny presny posun (az 0,4x), bezny tah (0,7 sirky obrazu za
//     sekundu) = presne rychlost z nastaveni, rychly tah = kurzor dojede
//     dal (az 2,5x). Jde vypnout v panelu MYS PLAYSTATION (pro kazdou hru).
//   - VYHLAZENI PODLE HRY: hra, ktera kresli jen kazdy 3. snimek, si mys
//     nejspis taky cte jen jednou za 3 snimky - posun z ostatnich dvou se
//     ztrati (i na skutecnem PlayStationu). Kolik z pohybu prstu hra
//     uvidela, pak zaviselo na tom, do kterych snimku dotyky zrovna padly:
//     kurzor se cukal, jednou skocil, jindy stal. Ted se posun rozlozi
//     rovnomerne do tolika snimku, kolik jich hra mezi dvema obrazy
//     vynecha (2 az 4) - hra vidi kazdy pohyb prstu stejne. Hra, ktera
//     cte kazdy snimek, dostane presne tolik jako drive (soucet sedi na
//     bod), jen o snimek pozdeji.
//   - klik po tuknuti az PO DOJETI KURZORU (klikne tam, kam prst ukazal)
//   - podrzeni = prst 0,65 s na miste (B317 0,5 s a do 3 %)
//   - mys z PC: posun v podilu sirky obrazu v prohlizeci (stejne miritko
//     jako prst), prohlizec ji zachyti kliknutim do obrazu
//   - denik: kazdy tah (doba, draha prstu, rychlost, zrychleni, kolik bodu
//     dostala hra), v MYS ZDRAVI tuknuti / tahy / vyhlazeni / oriznuti
//
//  Vsechno cisty C++, bez Androidu - testuje se na PC (test_overeni/b318).
//
//  Vlakna: dotyk() / pcMys() / nastav() vola Java (vlakno mostu WebView,
//  server TV), snimek() / poSnimku() a dotazy vlakno emulace - vse pod
//  jednim zamkem. snimek() se vola kazdy krok emulace PRED retro_run:
//  spocita, co jadro v tomhle snimku uvidi (posun, tlacitka); jadro se pak
//  behem retro_run pta (mysDotaz / padDotaz) a dostane presne tyhle
//  hodnoty. poSnimku() po retro_run: byl to novy obraz hry, nebo stejny?
// =====================================================================
#pragma once
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <string>
#include <vector>

namespace nap {

class Ps1Mys {
 public:
  // kam je mys zapojena
  enum Rezim { VYP = 0, PORT1 = 1, PORT1_OVL2 = 2, PORT2_OVL1 = 3 };
  // dotyk (stejna cisla posila stranka PS1)
  enum Akce { DOLU = 0, NAHORU = 1, POHYB = 2, ZRUSENO = 3 };

  // libretro: zarizeni a dotazy mysi
  static const unsigned RD_NONE = 0, RD_JOYPAD = 1, RD_MOUSE = 2;
  static const unsigned RD_PSE_MOUSE = (1u << 8) | 2u;   // RETRO_DEVICE_SUBCLASS(RETRO_DEVICE_MOUSE, 0)
  static const unsigned ID_X = 0, ID_Y = 1, ID_LEVE = 2, ID_PRAVE = 3;
  // tlacitka RetroPad (bity, ktere jadro dostava za ovladac): KRIZEK = B (0), KOLECKO = A (8),
  // smer nahoru 4, dolu 5, vlevo 6, vpravo 7
  static const uint32_t BIT_KRIZEK = 1u << 0, BIT_KOLECKO = 1u << 8;
  static const uint32_t BIT_NAHORU = 1u << 4, BIT_DOLU = 1u << 5, BIT_VLEVO = 1u << 6, BIT_VPRAVO = 1u << 7;

  // gesta - vzdalenost v podilu sirky obrazu, cas v ms
  static constexpr float LANO = 0.015f;       // B318: do 1,5 % sirky kurzor stoji, pak jede plynule (bez skoku)
  static constexpr float TAP_DRAHA = 0.03f;   // tuknuti: prst se od dotyku nevzdalil vic nez 3 % sirky
  static const int TAP_MS = 250;              // a byl dole nejvys 0,25 s
  static const int DRZENI_MS = 650;           // B318: prst na miste (v lane) 0,65 s = leve drzi (B317 0,5 s)
  static const int DVA_MS = 350;              // dva prsty nahore do 0,35 s = prave tlacitko
  static const int KLIK_SNIMKU = 5;           // klik: tlacitko dole 5 snimku (~0,1 s jako prst na skutecne mysi)
  static const int MEZERA_SNIMKU = 4;         // a pak aspon 4 snimky nahore (dvojklik se nespoji)
  static const int KLIK_CEKA_MAX = 6;         // B318: klik po tuknuti ceka na dojeti kurzoru nejvys 6 snimku
  static constexpr float BODU_NA_SIRKU = 320.f;   // tah pres celou sirku obrazu = 320 bodu mysi (pri 100 %)
  static const int PC_HLIDAC_MS = 2500;       // mys z PC: prohlizec se odmlcel -> tlacitka pustit
  static const int MAX_ZASOBNIK = 1000;       // nevyzvednuty posun (body) - vic se zahodi (zadne dobihani)
  // smer (mys sama v portu 1): bodu za snimek - zacne pomalu (presne miritko), drzenim zrychli
  static constexpr float SMER_POMALU = 1.f, SMER_RYCHLE = 6.f;
  static const int SMER_ZRYCHLENI_MS = 700;   // za 0,7 s drzeni plna rychlost
  // B318: zrychleni podle rychlosti prstu v (sirky obrazu za sekundu): nasobek (v / V0)^EXP, od MIN do MAX
  static constexpr double ZR_V0 = 0.7, ZR_EXP = 0.5, ZR_MIN = 0.4, ZR_MAX = 2.5;
  static constexpr double ZR_TAU_MS = 40.0;   // rychlost prstu vyhlazena (mezi dotyky 8-16 ms)
  // B318: posun se rozlozi do tolika snimku, kolik jich hra mezi dvema obrazy vynecha
  static const int VYHL_MIN = 2, VYHL_MAX = 4;
  // B318: mys z PC - posun v 1/100000 sirky obrazu v prohlizeci (cela sirka = 320 bodu x citlivost)
  static constexpr double PC_JEDNOTEK_NA_SIRKU = 100000.0;
  // bity stavu pro stranku (stavBity)
  static const int STAV_ZAP = 1, STAV_LEVE_DRZI = 2, STAV_PRST = 4, STAV_ZRYCHLENI = 32;

  // ----------------------------------------------------------------
  //  nastaveni
  // ----------------------------------------------------------------
  // rezim 0-3, citlivost v % (25-400). Vraci true, kdyz se neco zmenilo.
  bool nastav(int rezim, int citlivost) {
    std::lock_guard<std::mutex> l(m_);
    if (rezim < VYP || rezim > PORT2_OVL1) rezim = VYP;
    if (citlivost < 25) citlivost = 25;
    if (citlivost > 400) citlivost = 400;
    const bool zmena = rezim != rezim_ || citlivost != citlivost_;
    if (rezim != rezim_) {
      resetGest_();
      log_(std::string("MYS: ") + popisRezimu(rezim) + " (citlivost " + std::to_string(citlivost) + " %)");
    } else if (citlivost != citlivost_) {
      log_("MYS: citlivost " + std::to_string(citlivost) + " %");
    }
    rezim_ = rezim;
    citlivost_ = citlivost;
    return zmena;
  }
  // B318: zrychleni podle rychlosti prstu zapnout / vypnout. Vraci true, kdyz se zmenilo.
  bool nastavZrychleni(bool zap) {
    std::lock_guard<std::mutex> l(m_);
    if (zap == zrychleni_) return false;
    zrychleni_ = zap;
    log_(zap ? "MYS: zrychleni ZAPNUTO (pomaly tah presne, rychly dal)" : "MYS: zrychleni VYPNUTO (kurzor jede presne jako prst)");
    return true;
  }
  int rezim() const { std::lock_guard<std::mutex> l(m_); return rezim_; }
  int citlivost() const { std::lock_guard<std::mutex> l(m_); return citlivost_; }
  bool zrychleni() const { std::lock_guard<std::mutex> l(m_); return zrychleni_; }
  bool zapnuta() const { std::lock_guard<std::mutex> l(m_); return rezim_ != VYP; }

  static const char *popisRezimu(int r) {
    switch (r) {
      case PORT1: return "mys v portu 1, port 2 prazdny";
      case PORT1_OVL2: return "mys v portu 1, ovladac v portu 2";
      case PORT2_OVL1: return "mys v portu 2, ovladac v portu 1";
      default: return "vypnuta, ovladac v portu 1";
    }
  }
  // port (0 = port 1, 1 = port 2), kde je mys / ovladac; -1 = nikde
  static int portMysi(int r) { return (r == PORT1 || r == PORT1_OVL2) ? 0 : (r == PORT2_OVL1 ? 1 : -1); }
  static int portOvladace(int r) { return r == VYP ? 0 : (r == PORT1_OVL2 ? 1 : (r == PORT2_OVL1 ? 0 : -1)); }
  // co zapojit do portu 1 a 2 (retro_set_controller_port_device)
  static void zarizeni(int r, unsigned dev[2]) {
    switch (r) {
      case PORT1: dev[0] = RD_PSE_MOUSE; dev[1] = RD_NONE; break;
      case PORT1_OVL2: dev[0] = RD_PSE_MOUSE; dev[1] = RD_JOYPAD; break;
      case PORT2_OVL1: dev[0] = RD_JOYPAD; dev[1] = RD_PSE_MOUSE; break;
      default: dev[0] = RD_JOYPAD; dev[1] = RD_JOYPAD; break;   // jako jadro po nacteni hry
    }
  }
  // zrychleni pro rychlost prstu v (sirky obrazu za sekundu) - i pro testy a napovedu
  static double nasobekZrychleni(double v) {
    if (!(v > 1e-9)) return ZR_MIN;
    const double g = std::pow(v / ZR_V0, ZR_EXP);
    return g < ZR_MIN ? ZR_MIN : (g > ZR_MAX ? ZR_MAX : g);
  }

  // ----------------------------------------------------------------
  //  porty v jadre (vlakno emulace, pod zamkem jadra, pred retro_run)
  // ----------------------------------------------------------------
  // jadro po retro_load_game zapoji do vsech portu standardni ovladac; souhrn (MYS ZDRAVI)
  // a mereni, jak casto hra kresli, zacinaji pro novou hru od nuly
  void poNacteniHry() {
    std::lock_guard<std::mutex> l(m_);
    vJadru_ = VYP;
    resetGest_();
    okno_ = Okno();
    perioda_ = 1.0;
    odObrazu_ = 0;
    vyhlN_ = VYHL_MIN;
    vyhlHlasek_ = 0;
    tahuVDeniku_ = 0;
  }
  // je treba zmenit, co je v portech? -> dev[2] a true (a pak uz se to pocita za hotove)
  bool zmenaPortu(unsigned dev[2]) {
    std::lock_guard<std::mutex> l(m_);
    if (vJadru_ == rezim_) return false;
    zarizeni(rezim_, dev);
    vJadru_ = rezim_;
    log_(std::string("MYS: jadro - port 1 = ") + jmenoZar_(dev[0]) + ", port 2 = " + jmenoZar_(dev[1]));
    return true;
  }

  // ----------------------------------------------------------------
  //  dotyk na obraze hry (touchpad). x, y, w, h v bodech stranky,
  //  t = cas v ms (monotonni). Vraci, co dotyk udelal: KLIK_L / KLIK_P
  //  (stranka pak kratce zavibruje), jinak 0.
  // ----------------------------------------------------------------
  static const int KLIK_L = 1, KLIK_P = 2;
  int dotyk(int akce, int id, float x, float y, float w, float h, long long t) {
    (void)h;
    std::lock_guard<std::mutex> l(m_);
    if (rezim_ == VYP) return 0;
    if (!(w > 8.f)) w = 8.f;
    sirka_ = w;
    Prst *p = najdi_(id);
    int udalost = 0;
    switch (akce) {
      case DOLU: {
        if (p || pocet_ >= MAXP) return 0;
        Prst &n = prsty_[pocet_++];
        n = Prst();
        n.id = id; n.x0 = n.x = x; n.y0 = n.y = y; n.t0 = n.tPosl = t;
        okno_.dotyku++;
        if (pocet_ == 1) { gesto_ = JEDEN; }
        else if (pocet_ == 2 && gesto_ == JEDEN && !prsty_[0].tah) { gesto_ = DVA; tDva_ = t; }
        break;
      }
      case POHYB: {
        if (!p) return 0;
        const float dx = x - p->x, dy = y - p->y;
        const long long dt = t - p->tPosl;
        p->x = x; p->y = y; p->tPosl = t;
        const float vzd = std::hypot(x - p->x0, y - p->y0) / w;
        if (vzd > p->draha0) p->draha0 = vzd;
        if (gesto_ == DVA) {
          if (p->draha0 > TAP_DRAHA * 2.f) gesto_ = KONEC;   // dvema prsty se tahlo - zadny klik
          break;
        }
        if (!((gesto_ == JEDEN || gesto_ == DRZENI) && p == &prsty_[0])) break;   // dalsi prsty pri tahu jednim se nepocitaji
        if (!p->tah) {
          if (vzd <= LANO) break;                     // prst jeste v lane - kurzor stoji (presne tuknuti)
          // z lana ven: kurzor se rozjede od mista, kde prst je - jen o to, co lano presahlo
          p->tah = true;
          const float k = 1.f - LANO / vzd;
          const double s = (double)std::max(1LL, t - p->t0) / 1000.0;
          p->v = std::min(3.0, (double)vzd / s);      // pocatecni rychlost: prumer od dotyku
          pohybPrstu_(*p, (x - p->x0) * k, (y - p->y0) * k);
        } else {
          // rychlost prstu (sirky obrazu za sekundu), vyhlazena - dotyky chodi po 8-16 ms
          const double d = std::hypot((double)dx, (double)dy) / w;
          const double dtc = (double)std::max(1LL, std::min(100LL, dt));
          const double a = 1.0 - std::exp(-dtc / ZR_TAU_MS);
          p->v += a * (d / (dtc / 1000.0) - p->v);
          pohybPrstu_(*p, dx, dy);
        }
        break;
      }
      case NAHORU:
      case ZRUSENO: {
        if (!p) return 0;
        if (akce == ZRUSENO) {                        // system dotyk zrusil - nic nekliknout, vse pustit
          if (leveDrzi_) { leveDrzi_ = false; log_("MYS: dotyk zrusen - leve tlacitko pusteno"); }
          pocet_ = 0; gesto_ = NIC;
          return 0;
        }
        switch (gesto_) {
          case JEDEN:
            if (p == &prsty_[0]) {                    // dalsi prsty pri tahu jednim se nepocitaji
              if (p->draha0 <= TAP_DRAHA && t - p->t0 <= TAP_MS) { klik_(0); okno_.klikL++; okno_.tuknuti++; udalost = KLIK_L; }
              else if (p->tah) zapisTah_(*p, t);
              gesto_ = KONEC;
            }
            break;
          case DRZENI:
            if (p == &prsty_[0]) { leveDrzi_ = false; okno_.drzeniKonec++; if (p->tah) zapisTah_(*p, t); gesto_ = KONEC; }
            break;
          case DVA: {
            bool daleko = false;
            for (int i = 0; i < pocet_; i++) daleko |= prsty_[i].draha0 > TAP_DRAHA * 2.f;
            if (!daleko && t - tDva_ <= DVA_MS) { klik_(1); okno_.klikP++; udalost = KLIK_P; }
            gesto_ = KONEC;
            break;
          }
          default: break;
        }
        // prst ven ze seznamu
        const int i = (int)(p - prsty_);
        for (int k = i; k + 1 < pocet_; k++) prsty_[k] = prsty_[k + 1];
        pocet_--;
        if (pocet_ <= 0) { pocet_ = 0; gesto_ = NIC; }
        break;
      }
      default: break;
    }
    return udalost;
  }

  // ----------------------------------------------------------------
  //  mys pocitace (prohlizec na TV / PC): posun v 1/100000 sirky obrazu
  //  v prohlizeci (stejne miritko jako prst: cela sirka = 320 bodu x
  //  citlivost; zrychleni uz dela pocitac sam), tlacitka bit0 = leve,
  //  bit1 = prave
  // ----------------------------------------------------------------
  void pcMys(int dx, int dy, int tlacitka, long long t) {
    std::lock_guard<std::mutex> l(m_);
    if (rezim_ == VYP) return;
    const double k = (double)BODU_NA_SIRKU / PC_JEDNOTEK_NA_SIRKU * (citlivost_ / 100.0);
    pridej_(dx * k, dy * k);
    if ((tlacitka & 3) != pcTlac_) okno_.pcTlacitek++;
    pcTlac_ = tlacitka & 3;
    pcCas_ = t;
    okno_.pcZprav++;
  }

  // ----------------------------------------------------------------
  //  kazdy krok emulace pred retro_run: co jadro uvidi v tomhle snimku.
  //  padBity = tlacitka ovladace (dotyk, herni ovladac, klavesnice TV)
  // ----------------------------------------------------------------
  void snimek(long long t, uint32_t padBity) {
    std::lock_guard<std::mutex> l(m_);
    if (rezim_ == VYP) { fX_ = fY_ = 0; fL_ = fP_ = false; smerOd_ = -1; return; }
    // tlacitka ovladace patri mysi jen tehdy, kdyz zadny ovladac zapojeny neni
    const uint32_t bity = rezim_ == PORT1 ? padBity : 0u;
    // prst na miste (v lane) dost dlouho -> leve tlacitko drzi
    if (gesto_ == JEDEN && pocet_ == 1 && !prsty_[0].tah && t - prsty_[0].t0 >= DRZENI_MS) {
      gesto_ = DRZENI;
      leveDrzi_ = true;
      okno_.drzeni++;
    }
    if (pcTlac_ && t - pcCas_ > PC_HLIDAC_MS) { pcTlac_ = 0; log_("MYS: mys z PC se 2,5 s neozvala - tlacitka pustena"); }
    // smer: pomaly presny posun, drzenim rychleji (nahoru + dolu naraz se vyrusi)
    const int sx = ((bity & BIT_VPRAVO) ? 1 : 0) - ((bity & BIT_VLEVO) ? 1 : 0);
    const int sy = ((bity & BIT_DOLU) ? 1 : 0) - ((bity & BIT_NAHORU) ? 1 : 0);
    if (sx || sy) {
      if (smerOd_ < 0) smerOd_ = t;
      float u = (float)(t - smerOd_) / (float)SMER_ZRYCHLENI_MS;
      if (u > 1.f) u = 1.f;
      const double v = (SMER_POMALU + (SMER_RYCHLE - SMER_POMALU) * u) * (citlivost_ / 100.0);
      pridej_(sx * v, sy * v);
      okno_.snimkuSmer++;
    } else {
      smerOd_ = -1;
    }
    // B318: co prislo od minuleho snimku, rozlozit rovnomerne do N snimku (N = jak casto hra kresli)
    const int n = vyhlazeniN_();
    if (n != nPosl_) {   // rozlozeni se zmenilo: zbytky fazi, ktere uz nejsou, do faze 0 (nic se neztrati)
      for (int c = n; c < VYHL_MAX; c++) { zasX_[0] += zasX_[c]; zasY_[0] += zasY_[c]; zasX_[c] = zasY_[c] = 0.0; }
      faze_ %= n;
      nPosl_ = n;
    }
    if (novX_ != 0.0 || novY_ != 0.0) {
      for (int i = 0; i < n; i++) { planX_[i] += novX_ / n; planY_[i] += novY_ / n; }
      novX_ = novY_ = 0.0;
    }
    // posun: cela cisla, nejvys 127 za snimek. Zbytek po zaokrouhleni jde do snimku o N dal
    // (stejna "faze"), ne do nasledujiciho: kazdy pohyb je rozlozeny do N snimku stejnym dilem,
    // takze kazda faze dostane presne 1/N - a hra, ktera cte jen kazdy N. snimek, vidi presne
    // 1/N kazdeho pohybu (kurzor neujizdi). Hra, ktera cte kazdy snimek, dostane soucet presne.
    const double vx = zasX_[faze_] + planX_[0], vy = zasY_[faze_] + planY_[0];
    for (int i = 0; i + 1 < VYHL_MAX; i++) { planX_[i] = planX_[i + 1]; planY_[i] = planY_[i + 1]; }
    planX_[VYHL_MAX - 1] = planY_[VYHL_MAX - 1] = 0.0;
    int ix = (int)std::lround(vx), iy = (int)std::lround(vy);
    bool orez = false;
    if (ix > 127) { ix = 127; orez = true; }
    if (ix < -127) { ix = -127; orez = true; }
    if (iy > 127) { iy = 127; orez = true; }
    if (iy < -127) { iy = -127; orez = true; }
    if (orez) okno_.oriznuto++;
    zasX_[faze_] = vx - ix; zasY_[faze_] = vy - iy;   // co se nevejde (vic nez 127), dojede za N snimku
    // pohyb skoncil (nic dalsiho v planu): drobne zbytky vsech fazi dohromady (kazdy do +-0,5)
    // dojedou hned - kurzor skonci presne tam, kam prst dojel
    bool nicVPlanu = novX_ == 0.0 && novY_ == 0.0;
    for (int i = 0; i < VYHL_MAX && nicVPlanu; i++) nicVPlanu = planX_[i] == 0.0 && planY_[i] == 0.0;
    if (nicVPlanu) {
      dorovnej_(zasX_, ix);
      dorovnej_(zasY_, iy);
    }
    faze_ = (faze_ + 1) % n;
    fX_ = ix; fY_ = iy;
    okno_.bodyX += ix < 0 ? -ix : ix;
    okno_.bodyY += iy < 0 ? -iy : iy;
    // kliky: 5 snimku dole, 4 nahore, dalsi z fronty. B318: klik po tuknuti az po dojeti
    // kurzoru - v tomhle snimku uz zadny posun a nic nezbyva (nejvys KLIK_CEKA_MAX snimku)
    const bool pohybCeka = ix != 0 || iy != 0 || zbyvaPosun_();
    bool pulz[2];
    for (int b = 0; b < 2; b++) {
      pulz[b] = false;
      if (dole_[b] > 0) { pulz[b] = true; if (--dole_[b] == 0) mezera_[b] = MEZERA_SNIMKU; }
      else if (mezera_[b] > 0) mezera_[b]--;
      else if (fronta_[b] > 0) {
        if (pohybCeka && cekani_[b] < KLIK_CEKA_MAX) cekani_[b]++;
        else {
          cekani_[b] = 0;
          fronta_[b]--; pulz[b] = true; dole_[b] = KLIK_SNIMKU - 1;
          if (dole_[b] == 0) mezera_[b] = MEZERA_SNIMKU;
        }
      }
    }
    fL_ = leveDrzi_ || pulz[0] || (pcTlac_ & 1) || (bity & BIT_KRIZEK);
    fP_ = pulz[1] || (pcTlac_ & 2) || (bity & BIT_KOLECKO);
    if (fL_) okno_.snimkuL++;
    if (fP_) okno_.snimkuP++;
  }

  // ----------------------------------------------------------------
  //  B318: po retro_run - byl v tomhle snimku novy obraz hry? (jadro hlasi
  //  stejny obraz, kdyz hra nic noveho nenakreslila.) Z toho, kolik snimku
  //  je mezi obrazy, se pozna, jak casto hra kresli (Dune 2000: kazdy 3.).
  // ----------------------------------------------------------------
  void poSnimku(bool novyObraz) {
    std::lock_guard<std::mutex> l(m_);
    odObrazu_++;
    okno_.snimku++;
    if (!novyObraz) return;
    const int s = odObrazu_ < 6 ? odObrazu_ : 6;
    perioda_ += (s - perioda_) * 0.15;               // ~1 s pameti pri 20 obrazech za sekundu
    odObrazu_ = 0;
    okno_.obrazu++;
    // rozlozeni podle periody - s hysterezi (hra, ktera kresli stridave po 2 a 3 snimcich,
    // neprepina sem a tam)
    int n = (int)std::lround(perioda_);
    n = n < VYHL_MIN ? VYHL_MIN : (n > VYHL_MAX ? VYHL_MAX : n);
    if (n != vyhlN_ && std::fabs(perioda_ - vyhlN_) > 0.7) {
      vyhlN_ = n;
      if (rezim_ != VYP && vynuceno_ == 0 && vyhlHlasek_ < 20) {
        vyhlHlasek_++;
        char b[160];
        std::snprintf(b, sizeof(b), "MYS: hra kresli kazdy %.1f. snimek - posun mysi se rozklada do %d snimku", perioda_, n);
        log_(b);
      }
    }
  }
  // kolik snimku pro rozlozeni posunu (2-4); pro testy jde vynutit (0 = podle hry)
  int vyhlazeni() const { std::lock_guard<std::mutex> l(m_); return vyhlazeniN_(); }
  void vynutVyhlazeni(int n) { std::lock_guard<std::mutex> l(m_); vynuceno_ = n < 0 ? 0 : (n > VYHL_MAX ? VYHL_MAX : n); }

  // ----------------------------------------------------------------
  //  co jadro dostane (retro_input_state behem retro_run)
  // ----------------------------------------------------------------
  int16_t mysDotaz(unsigned port, unsigned id) const {
    std::lock_guard<std::mutex> l(m_);
    if ((int)port != portMysi(rezim_)) return 0;
    switch (id) {
      case ID_X: return (int16_t)fX_;
      case ID_Y: return (int16_t)fY_;
      case ID_LEVE: return fL_ ? 1 : 0;
      case ID_PRAVE: return fP_ ? 1 : 0;
      default: return 0;
    }
  }
  // tlacitko ovladace (RetroPad id 0-15) pro port; bity = co je prave stisknuto.
  // Ovladac dostane vsechna tlacitka - jen v portu, kde je zapojeny (mys sama
  // v portu 1 = ovladac nikde; jeho tlacitka pak ridi mys, viz snimek()).
  int16_t padDotaz(unsigned port, unsigned id, uint32_t bity) const {
    if (id >= 16) return 0;
    int r;
    { std::lock_guard<std::mutex> l(m_); r = rezim_; }
    if ((int)port != portOvladace(r)) return 0;
    return (bity & (1u << id)) ? 1 : 0;
  }

  // stav pro stranku: bit0 = mys zapnuta, bit1 = leve tlacitko drzi (prst), bit2 = prst na obraze,
  // bit5 = zrychleni zapnuto (bity 3 a 4 jsou v odpovedi na dotyk klik L / P)
  int stavBity() const {
    std::lock_guard<std::mutex> l(m_);
    return (rezim_ != VYP ? STAV_ZAP : 0) | (leveDrzi_ ? STAV_LEVE_DRZI : 0) | (pocet_ > 0 ? STAV_PRST : 0) |
           (zrychleni_ ? STAV_ZRYCHLENI : 0);
  }

  // ----------------------------------------------------------------
  //  zapamatovani pro kazdou hru: soubor s radky
  //  "klic <TAB> rezim <TAB> citlivost <TAB> zrychleni" (B317 radky bez zrychleni = zapnuto)
  // ----------------------------------------------------------------
  // nacte nastaveni hry (zadny zaznam = mys vypnuta, citlivost 100 %, zrychleni zapnuto).
  // Vraci true, kdyz zaznam byl.
  bool nactiProHru(const std::string &soubor, const std::string &klic) {
    int r = VYP, c = 100, z = 1;
    bool nalez = false;
    for (const Zaznam &zz : nactiSoubor_(soubor))
      if (zz.klic == klic) { r = zz.rezim; c = zz.citlivost; z = zz.zrychleni; nalez = true; }
    {
      std::lock_guard<std::mutex> l(m_);
      if (r < VYP || r > PORT2_OVL1) r = VYP;
      if (c < 25 || c > 400) c = 100;
      rezim_ = r; citlivost_ = c; zrychleni_ = z != 0; klic_ = klic;
      resetGest_();
    }
    return nalez;
  }
  // ulozi nastaveni aktualni hry (klic z nactiProHru). Vraci true pri uspechu.
  bool ulozProHru(const std::string &soubor) {
    std::string klic;
    int r, c, z;
    { std::lock_guard<std::mutex> l(m_); klic = klic_; r = rezim_; c = citlivost_; z = zrychleni_ ? 1 : 0; }
    if (soubor.empty() || klic.empty()) return false;
    std::vector<Zaznam> v = nactiSoubor_(soubor);
    bool je = false;
    for (Zaznam &zz : v) if (zz.klic == klic) { zz.rezim = r; zz.citlivost = c; zz.zrychleni = z; je = true; }
    if (!je) v.push_back(Zaznam{klic, r, c, z});
    if (v.size() > 500) v.erase(v.begin(), v.begin() + (long)(v.size() - 500));
    const std::string tmp = soubor + ".novy";
    FILE *f = std::fopen(tmp.c_str(), "w");
    if (!f) return false;
    std::fprintf(f, "# B317 mys PS1 pro kazdou hru: klic (ID disku), rezim (0 vyp, 1 port 1, 2 port 1 + ovladac 2, 3 port 2 + ovladac 1), citlivost %%, B318: zrychleni (1 zap, 0 vyp)\n");
    for (const Zaznam &zz : v) std::fprintf(f, "%s\t%d\t%d\t%d\n", zz.klic.c_str(), zz.rezim, zz.citlivost, zz.zrychleni);
    const bool ok = std::fclose(f) == 0;
    return ok && std::rename(tmp.c_str(), soubor.c_str()) == 0;
  }
  std::string klic() const { std::lock_guard<std::mutex> l(m_); return klic_; }
  // klic hry: ID disku (SLES02250), jinak jmeno souboru; jen bezpecne znaky.
  // SLUS99999 je zastupne ID jadra (disk bez SYSTEM.CNF, PS-EXE) - ma ho vic her,
  // proto se pak bere jmeno souboru.
  static std::string klicHry(const char *idDisku, const std::string &cesta) {
    std::string k;
    if (idDisku) for (const char *p = idDisku; *p && k.size() < 16; p++) if (std::isalnum((unsigned char)*p) || *p == '_' || *p == '-' || *p == '.') k += *p;
    if (k == "SLUS99999" && !cesta.empty()) k.clear();
    if (k.empty()) {
      const size_t l = cesta.find_last_of("/\\");
      const std::string jm = l == std::string::npos ? cesta : cesta.substr(l + 1);
      for (char ch : jm) if (k.size() < 80 && (unsigned char)ch >= 32 && ch != '\t') k += ch;
    }
    return k.empty() ? std::string("BIOS") : k;
  }

  // ----------------------------------------------------------------
  //  denik
  // ----------------------------------------------------------------
  std::vector<std::string> vezmiLog() { std::lock_guard<std::mutex> l(m_); std::vector<std::string> v; v.swap(log_v_); return v; }
  // souhrn za okno (ZDRAVI): prazdny, kdyz je mys vypnuta a nic se nedelo
  std::string souhrn() {
    std::lock_guard<std::mutex> l(m_);
    if (rezim_ == VYP && okno_.prazdne()) { okno_.snimku = okno_.obrazu = 0; return std::string(); }
    char zr[48], tahy[120], hra[64];
    if (!zrychleni_) std::snprintf(zr, sizeof(zr), "VYP");
    else if (okno_.tahu > 0) std::snprintf(zr, sizeof(zr), "ZAP (%.1f-%.1fx)", okno_.gMin, okno_.gMax);
    else std::snprintf(zr, sizeof(zr), "ZAP");
    if (okno_.tahu > 0) std::snprintf(tahy, sizeof(tahy), "tahu %d (prumer %.2f s)", okno_.tahu, okno_.tahMs / 1000.0 / okno_.tahu);
    else std::snprintf(tahy, sizeof(tahy), "tahu 0");
    if (okno_.obrazu > 0) std::snprintf(hra, sizeof(hra), "hra kresli kazdy %.1f. snimek", (double)okno_.snimku / okno_.obrazu);
    else std::snprintf(hra, sizeof(hra), "hra nekresli nic noveho");
    char b[640];
    std::snprintf(b, sizeof(b), "mys %s, citlivost %d %%, zrychleni %s, vyhlazeni %d snimky (%s): dotyku %d (tuknuti %d, %s, drzeni L %d), "
                  "posun %ld / %ld bodu (oriznuto snimku %d), klik L %d P %d, snimku s L %d P %d, smerem %d, z PC zprav %d (zmen tlacitek %d)",
                  rezim_ == VYP ? "vypnuta" : (rezim_ == PORT2_OVL1 ? "v portu 2" : "v portu 1"), citlivost_, zr, vyhlazeniN_(), hra,
                  okno_.dotyku, okno_.tuknuti, tahy, okno_.drzeni, okno_.bodyX, okno_.bodyY, okno_.oriznuto, okno_.klikL, okno_.klikP,
                  okno_.snimkuL, okno_.snimkuP, okno_.snimkuSmer, okno_.pcZprav, okno_.pcTlacitek);
    okno_ = Okno();
    return b;
  }

 private:
  enum Gesto { NIC, JEDEN, DVA, DRZENI, KONEC };
  struct Prst {
    int id = 0;
    float x0 = 0, y0 = 0, x = 0, y = 0;
    long long t0 = 0, tPosl = 0;
    bool tah = false;        // prst opustil lano - kurzor jede
    float draha0 = 0;        // nejvetsi vzdalenost od mista dotyku (podil sirky)
    double v = 0;            // rychlost prstu (sirky obrazu za sekundu), vyhlazena
    // denik tahu
    double draha = 0, body = 0, vMax = 0, gMin = 1e9, gMax = 0;
  };
  struct Zaznam { std::string klic; int rezim; int citlivost; int zrychleni; };
  struct Okno {
    int dotyku = 0, tuknuti = 0, tahu = 0, klikL = 0, klikP = 0, drzeni = 0, drzeniKonec = 0, snimkuL = 0, snimkuP = 0, snimkuSmer = 0;
    int pcZprav = 0, pcTlacitek = 0, oriznuto = 0, snimku = 0, obrazu = 0;
    long bodyX = 0, bodyY = 0;
    long long tahMs = 0;
    double gMin = 1e9, gMax = 0;
    bool prazdne() const { return !dotyku && !klikL && !klikP && !drzeni && !pcZprav && !bodyX && !bodyY && !snimkuL && !snimkuP && !snimkuSmer; }
  };
  static const int MAXP = 4;

  mutable std::mutex m_;
  int rezim_ = VYP, citlivost_ = 100, vJadru_ = VYP;
  bool zrychleni_ = true;
  std::string klic_;
  // dotyk
  Prst prsty_[MAXP];
  int pocet_ = 0;
  Gesto gesto_ = NIC;
  long long tDva_ = 0;
  float sirka_ = 320.f;
  bool leveDrzi_ = false;
  // posun: nov = prislo od minuleho snimku, plan = rozlozeno do dalsich snimku,
  // zas = zbytek do celych bodu pro kazdou fazi snimku (double - stovky drobnych pohybu
  // se sectou presne), faze_ = snimek mod N
  double novX_ = 0.0, novY_ = 0.0;
  double planX_[VYHL_MAX] = {0, 0, 0, 0}, planY_[VYHL_MAX] = {0, 0, 0, 0};
  double zasX_[VYHL_MAX] = {0, 0, 0, 0}, zasY_[VYHL_MAX] = {0, 0, 0, 0};
  int faze_ = 0, nPosl_ = VYHL_MIN;
  int fronta_[2] = {0, 0}, dole_[2] = {0, 0}, mezera_[2] = {0, 0}, cekani_[2] = {0, 0};
  int pcTlac_ = 0;
  long long pcCas_ = 0;
  long long smerOd_ = -1;   // od kdy je drzen smer (zrychleni), -1 = nedrzi
  // jak casto hra kresli: perioda_ = snimku mezi obrazy (vyhlazena), odObrazu_ = snimku od posledniho,
  // vyhlN_ = do kolika snimku se posun rozklada (vynuceno_ > 0 jen v testech)
  double perioda_ = 1.0;
  int odObrazu_ = 0, vyhlN_ = VYHL_MIN, vynuceno_ = 0, vyhlHlasek_ = 0, tahuVDeniku_ = 0;
  // co jadro uvidi v tomhle snimku
  int fX_ = 0, fY_ = 0;
  bool fL_ = false, fP_ = false;
  Okno okno_;
  std::vector<std::string> log_v_;

  Prst *najdi_(int id) { for (int i = 0; i < pocet_; i++) if (prsty_[i].id == id) return &prsty_[i]; return nullptr; }
  int vyhlazeniN_() const { return vynuceno_ > 0 ? vynuceno_ : vyhlN_; }
  // konec pohybu: kdyz v tomhle snimku nic nejede, soucet zbytku vsech fazi (|soucet| >= 0,5)
  // se posle ted - do te faze zustane jen zbytek pod pul bodu
  void dorovnej_(double zas[VYHL_MAX], int &i) {
    if (i != 0) return;
    double s = 0.0;
    for (int c = 0; c < VYHL_MAX; c++) s += zas[c];
    if (std::fabs(s) < 0.5 || std::fabs(s) > 127.0) return;   // velky zbytek (po oriznuti) dojede po fazich
    i = (int)std::lround(s);
    for (int c = 0; c < VYHL_MAX; c++) zas[c] = 0.0;
    zas[faze_] = s - i;
  }
  // jeste neco dojede? (posun v planu, nebo zbytek vetsi nez zaokrouhleni - po oriznuti na 127)
  bool zbyvaPosun_() const {
    double x = novX_, y = novY_;
    for (int i = 0; i < VYHL_MAX; i++) {
      x += planX_[i]; y += planY_[i];
      if (std::fabs(zasX_[i]) > 0.5 || std::fabs(zasY_[i]) > 0.5) return true;
    }
    return std::fabs(x) >= 0.5 || std::fabs(y) >= 0.5;
  }
  void pridej_(double dx, double dy) {
    novX_ += dx; novY_ += dy;
    // nevyzvednuty posun (i rozlozeny do dalsich snimku) nejvys MAX_ZASOBNIK bodu - zbytek se zahodi
    double cx = novX_, cy = novY_;
    for (int i = 0; i < VYHL_MAX; i++) { cx += planX_[i] + zasX_[i]; cy += planY_[i] + zasY_[i]; }
    if (cx > MAX_ZASOBNIK) novX_ -= cx - MAX_ZASOBNIK;
    else if (cx < -MAX_ZASOBNIK) novX_ -= cx + MAX_ZASOBNIK;
    if (cy > MAX_ZASOBNIK) novY_ -= cy - MAX_ZASOBNIK;
    else if (cy < -MAX_ZASOBNIK) novY_ -= cy + MAX_ZASOBNIK;
  }
  // posun prstu (body stranky) -> body mysi: cela sirka obrazu = 320 x citlivost, x zrychleni
  void pohybPrstu_(Prst &p, float dx, float dy) {
    const double g = zrychleni_ ? nasobekZrychleni(p.v) : 1.0;
    const double k = (double)BODU_NA_SIRKU / (double)sirka_ * (citlivost_ / 100.0) * g;
    pridej_(dx * k, dy * k);
    p.draha += std::hypot((double)dx, (double)dy) / sirka_;
    p.body += std::hypot(dx * k, dy * k);
    if (g < p.gMin) p.gMin = g;
    if (g > p.gMax) p.gMax = g;
    if (p.v > p.vMax) p.vMax = p.v;
  }
  // tah skoncil: do souhrnu, prvnich 12 tahu hry i do deniku
  void zapisTah_(const Prst &p, long long t) {
    const long long ms = t - p.t0 > 0 ? t - p.t0 : 0;
    okno_.tahu++;
    okno_.tahMs += ms;
    if (zrychleni_) {
      if (p.gMin < okno_.gMin) okno_.gMin = p.gMin;
      if (p.gMax > okno_.gMax) okno_.gMax = p.gMax;
    }
    if (tahuVDeniku_ >= 12) return;
    tahuVDeniku_++;
    char zr[48];
    if (zrychleni_) std::snprintf(zr, sizeof(zr), "%.1f-%.1fx", p.gMin < 1e8 ? p.gMin : 1.0, p.gMax);
    else std::snprintf(zr, sizeof(zr), "vypnuto");
    char b[260];
    std::snprintf(b, sizeof(b), "MYS TAH %d: %.2f s, prst %.2f sirky obrazu (rychlost prumer %.2f, nejvys %.2f sirky/s), zrychleni %s, "
                  "do hry %.0f bodu (citlivost %d %%)",
                  tahuVDeniku_, ms / 1000.0, p.draha, ms > 0 ? p.draha / (ms / 1000.0) : 0.0, p.vMax, zr, p.body, citlivost_);
    log_(b);
    if (tahuVDeniku_ == 12) log_("MYS: dalsi tahy uz jen v souctu MYS ZDRAVI");
  }
  void klik_(int b) { if (fronta_[b] < 4) fronta_[b]++; }
  void resetGest_() {
    pocet_ = 0; gesto_ = NIC; leveDrzi_ = false;
    novX_ = novY_ = 0.0;
    for (int i = 0; i < VYHL_MAX; i++) planX_[i] = planY_[i] = zasX_[i] = zasY_[i] = 0.0;
    for (int b = 0; b < 2; b++) fronta_[b] = dole_[b] = mezera_[b] = cekani_[b] = 0;
    pcTlac_ = 0;
    smerOd_ = -1;
    fX_ = fY_ = 0; fL_ = fP_ = false;
  }
  void log_(const std::string &s) { if (log_v_.size() < 50) log_v_.push_back(s); }
  static const char *jmenoZar_(unsigned d) {
    return d == RD_PSE_MOUSE ? "mys PlayStation" : d == RD_JOYPAD ? "ovladac" : d == RD_NONE ? "nic" : "?";
  }
  static std::vector<Zaznam> nactiSoubor_(const std::string &soubor) {
    std::vector<Zaznam> v;
    if (soubor.empty()) return v;
    FILE *f = std::fopen(soubor.c_str(), "r");
    if (!f) return v;
    char r[512];
    while (std::fgets(r, sizeof(r), f)) {
      if (r[0] == '#') continue;
      char *t1 = std::strchr(r, '\t');
      if (!t1) continue;
      char *t2 = std::strchr(t1 + 1, '\t');
      if (!t2) continue;
      Zaznam z;
      z.klic.assign(r, t1);
      z.rezim = std::atoi(t1 + 1);
      z.citlivost = std::atoi(t2 + 1);
      const char *t3 = std::strchr(t2 + 1, '\t');
      z.zrychleni = t3 ? (std::atoi(t3 + 1) != 0 ? 1 : 0) : 1;   // B317 radek (bez sloupce) = zapnuto
      if (!z.klic.empty()) v.push_back(z);
    }
    std::fclose(f);
    return v;
  }
};

}  // namespace nap
