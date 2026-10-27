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
//       prst podrzet 0,5 s  = leve tlacitko DRZI (tahem pak vyber jednotek
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
//  Vsechno cisty C++, bez Androidu - testuje se na PC (test_overeni/b317).
//
//  Vlakna: dotyk() / pcMys() / nastav() vola Java (vlakno mostu WebView,
//  server TV), snimek() a dotazy vlakno emulace - vse pod jednim zamkem.
//  snimek() se vola kazdy krok emulace PRED retro_run: spocita, co jadro
//  v tomhle snimku uvidi (posun, tlacitka); jadro se pak behem retro_run
//  pta (mysDotaz / padDotaz) a dostane presne tyhle hodnoty.
// =====================================================================
#pragma once
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

  // gesta - posun v podilu sirky obrazu, cas v ms
  static constexpr float TAP_POHYB = 0.03f;   // do 3 % sirky obrazu je to jeste tuknuti, ne tah
  static const int TAP_MS = 250;              // tuknuti = prst nahore do 0,25 s
  static const int DRZENI_MS = 500;           // prst na miste 0,5 s = leve tlacitko drzi (jako dlouhy stisk v Androidu)
  static const int DVA_MS = 350;              // dva prsty nahore do 0,35 s = prave tlacitko
  static const int KLIK_SNIMKU = 5;           // klik: tlacitko dole 5 snimku (~0,1 s jako prst na skutecne mysi)
  static const int MEZERA_SNIMKU = 4;         // a pak aspon 4 snimky nahore (dvojklik se nespoji)
  static constexpr float BODU_NA_SIRKU = 320.f;   // tah pres celou sirku obrazu = 320 bodu mysi (pri 100 %)
  static const int PC_HLIDAC_MS = 2500;       // mys z PC: prohlizec se odmlcel -> tlacitka pustit
  static const int MAX_ZASOBNIK = 1000;       // nevyzvednuty posun (body) - vic se zahodi (zadne dobihani)
  // smer (mys sama v portu 1): bodu za snimek - zacne pomalu (presne miritko), drzenim zrychli
  static constexpr float SMER_POMALU = 1.f, SMER_RYCHLE = 6.f;
  static const int SMER_ZRYCHLENI_MS = 700;   // za 0,7 s drzeni plna rychlost

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
  int rezim() const { std::lock_guard<std::mutex> l(m_); return rezim_; }
  int citlivost() const { std::lock_guard<std::mutex> l(m_); return citlivost_; }
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

  // ----------------------------------------------------------------
  //  porty v jadre (vlakno emulace, pod zamkem jadra, pred retro_run)
  // ----------------------------------------------------------------
  // jadro po retro_load_game zapoji do vsech portu standardni ovladac; souhrn (MYS ZDRAVI)
  // zacina pro novou hru od nuly
  void poNacteniHry() { std::lock_guard<std::mutex> l(m_); vJadru_ = VYP; resetGest_(); okno_ = Okno(); }
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
        n.id = id; n.x0 = n.x = x; n.y0 = n.y = y; n.t0 = t; n.tah = false; n.daleko = false;
        okno_.dotyku++;
        if (pocet_ == 1) { gesto_ = JEDEN; }
        else if (pocet_ == 2 && gesto_ == JEDEN && !prsty_[0].tah) { gesto_ = DVA; tDva_ = t; }
        break;
      }
      case POHYB: {
        if (!p) return 0;
        const float dx = x - p->x, dy = y - p->y;
        p->x = x; p->y = y;
        const float vzd = std::hypot(x - p->x0, y - p->y0) / w;
        if (vzd > TAP_POHYB * 2.f) p->daleko = true;
        if (gesto_ == JEDEN && p == &prsty_[0]) {
          if (!p->tah && vzd > TAP_POHYB) { p->tah = true; pridejDotyk_(x - p->x0, y - p->y0); }   // cely posun od dotyku
          else if (p->tah) pridejDotyk_(dx, dy);
        } else if (gesto_ == DRZENI && p == &prsty_[0]) {
          pridejDotyk_(dx, dy);                       // tah s drzenym levym tlacitkem
        } else if (gesto_ == DVA && p->daleko) {
          gesto_ = KONEC;                             // dvema prsty se tahlo - zadny klik
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
              if (!p->tah && t - p->t0 <= TAP_MS) { klik_(0); okno_.klikL++; udalost = KLIK_L; }
              gesto_ = KONEC;
            }
            break;
          case DRZENI:
            if (p == &prsty_[0]) { leveDrzi_ = false; okno_.drzeniKonec++; gesto_ = KONEC; }
            break;
          case DVA: {
            bool daleko = false;
            for (int i = 0; i < pocet_; i++) daleko |= prsty_[i].daleko;
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
  //  mys pocitace (prohlizec na TV / PC): posun v bodech obrazovky PC,
  //  tlacitka bit0 = leve, bit1 = prave
  // ----------------------------------------------------------------
  void pcMys(int dx, int dy, int tlacitka, long long t) {
    std::lock_guard<std::mutex> l(m_);
    if (rezim_ == VYP) return;
    const double k = citlivost_ / 100.0;
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
    // prst na miste dost dlouho -> leve tlacitko drzi
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
    // posun: cela cisla zaokrouhlena (zbytek do +-0,5 zustava na pristi snimek - nic se
    // neztrati ani nepribude), nejvys 127 za snimek
    int ix = (int)std::lround(zasX_), iy = (int)std::lround(zasY_);
    if (ix > 127) ix = 127;
    if (ix < -127) ix = -127;
    if (iy > 127) iy = 127;
    if (iy < -127) iy = -127;
    zasX_ -= ix; zasY_ -= iy;
    fX_ = ix; fY_ = iy;
    okno_.bodyX += ix < 0 ? -ix : ix;
    okno_.bodyY += iy < 0 ? -iy : iy;
    // kliky: 5 snimku dole, 4 nahore, dalsi z fronty
    bool pulz[2];
    for (int b = 0; b < 2; b++) {
      pulz[b] = false;
      if (dole_[b] > 0) { pulz[b] = true; if (--dole_[b] == 0) mezera_[b] = MEZERA_SNIMKU; }
      else if (mezera_[b] > 0) mezera_[b]--;
      else if (fronta_[b] > 0) { fronta_[b]--; pulz[b] = true; dole_[b] = KLIK_SNIMKU - 1; if (dole_[b] == 0) mezera_[b] = MEZERA_SNIMKU; }
    }
    fL_ = leveDrzi_ || pulz[0] || (pcTlac_ & 1) || (bity & BIT_KRIZEK);
    fP_ = pulz[1] || (pcTlac_ & 2) || (bity & BIT_KOLECKO);
    if (fL_) okno_.snimkuL++;
    if (fP_) okno_.snimkuP++;
  }

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

  // stav pro stranku: bit0 = mys zapnuta, bit1 = leve tlacitko drzi (prst), bit2 = prst na obraze
  int stavBity() const {
    std::lock_guard<std::mutex> l(m_);
    return (rezim_ != VYP ? 1 : 0) | (leveDrzi_ ? 2 : 0) | (pocet_ > 0 ? 4 : 0);
  }

  // ----------------------------------------------------------------
  //  zapamatovani pro kazdou hru: soubor s radky "klic <TAB> rezim <TAB> citlivost"
  // ----------------------------------------------------------------
  // nacte nastaveni hry (zadny zaznam = mys vypnuta, citlivost 100 %). Vraci true, kdyz zaznam byl.
  bool nactiProHru(const std::string &soubor, const std::string &klic) {
    int r = VYP, c = 100;
    bool nalez = false;
    for (const Zaznam &z : nactiSoubor_(soubor))
      if (z.klic == klic) { r = z.rezim; c = z.citlivost; nalez = true; }
    {
      std::lock_guard<std::mutex> l(m_);
      if (r < VYP || r > PORT2_OVL1) r = VYP;
      if (c < 25 || c > 400) c = 100;
      rezim_ = r; citlivost_ = c; klic_ = klic;
      resetGest_();
    }
    return nalez;
  }
  // ulozi nastaveni aktualni hry (klic z nactiProHru). Vraci true pri uspechu.
  bool ulozProHru(const std::string &soubor) {
    std::string klic;
    int r, c;
    { std::lock_guard<std::mutex> l(m_); klic = klic_; r = rezim_; c = citlivost_; }
    if (soubor.empty() || klic.empty()) return false;
    std::vector<Zaznam> v = nactiSoubor_(soubor);
    bool je = false;
    for (Zaznam &z : v) if (z.klic == klic) { z.rezim = r; z.citlivost = c; je = true; }
    if (!je) v.push_back(Zaznam{klic, r, c});
    if (v.size() > 500) v.erase(v.begin(), v.begin() + (long)(v.size() - 500));
    const std::string tmp = soubor + ".novy";
    FILE *f = std::fopen(tmp.c_str(), "w");
    if (!f) return false;
    std::fprintf(f, "# B317 mys PS1 pro kazdou hru: klic (ID disku), rezim (0 vyp, 1 port 1, 2 port 1 + ovladac 2, 3 port 2 + ovladac 1), citlivost %%\n");
    for (const Zaznam &z : v) std::fprintf(f, "%s\t%d\t%d\n", z.klic.c_str(), z.rezim, z.citlivost);
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
    if (rezim_ == VYP && okno_.prazdne()) return std::string();
    char b[400];
    std::snprintf(b, sizeof(b), "mys %s, citlivost %d %%: dotyku %d, posun %ld / %ld bodu, klik L %d P %d, drzeni L %d, "
                  "snimku s L %d P %d, smerem %d, z PC zprav %d (zmen tlacitek %d)",
                  rezim_ == VYP ? "vypnuta" : (rezim_ == PORT2_OVL1 ? "v portu 2" : "v portu 1"), citlivost_, okno_.dotyku,
                  okno_.bodyX, okno_.bodyY, okno_.klikL, okno_.klikP, okno_.drzeni, okno_.snimkuL, okno_.snimkuP,
                  okno_.snimkuSmer, okno_.pcZprav, okno_.pcTlacitek);
    okno_ = Okno();
    return b;
  }

 private:
  enum Gesto { NIC, JEDEN, DVA, DRZENI, KONEC };
  struct Prst { int id = 0; float x0 = 0, y0 = 0, x = 0, y = 0; long long t0 = 0; bool tah = false, daleko = false; };
  struct Zaznam { std::string klic; int rezim; int citlivost; };
  struct Okno {
    int dotyku = 0, klikL = 0, klikP = 0, drzeni = 0, drzeniKonec = 0, snimkuL = 0, snimkuP = 0, snimkuSmer = 0, pcZprav = 0, pcTlacitek = 0;
    long bodyX = 0, bodyY = 0;
    bool prazdne() const { return !dotyku && !klikL && !klikP && !drzeni && !pcZprav && !bodyX && !bodyY && !snimkuL && !snimkuP && !snimkuSmer; }
  };
  static const int MAXP = 4;

  mutable std::mutex m_;
  int rezim_ = VYP, citlivost_ = 100, vJadru_ = VYP;
  std::string klic_;
  // dotyk
  Prst prsty_[MAXP];
  int pocet_ = 0;
  Gesto gesto_ = NIC;
  long long tDva_ = 0;
  float sirka_ = 320.f;
  bool leveDrzi_ = false;
  // posun a tlacitka (zasobnik v double - stovky drobnych pohybu se sectou presne)
  double zasX_ = 0.0, zasY_ = 0.0;
  int fronta_[2] = {0, 0}, dole_[2] = {0, 0}, mezera_[2] = {0, 0};
  int pcTlac_ = 0;
  long long pcCas_ = 0;
  long long smerOd_ = -1;   // od kdy je drzen smer (zrychleni), -1 = nedrzi
  // co jadro uvidi v tomhle snimku
  int fX_ = 0, fY_ = 0;
  bool fL_ = false, fP_ = false;
  Okno okno_;
  std::vector<std::string> log_v_;

  Prst *najdi_(int id) { for (int i = 0; i < pocet_; i++) if (prsty_[i].id == id) return &prsty_[i]; return nullptr; }
  void pridej_(double dx, double dy) {
    zasX_ += dx; zasY_ += dy;
    if (zasX_ > MAX_ZASOBNIK) zasX_ = MAX_ZASOBNIK;
    if (zasX_ < -MAX_ZASOBNIK) zasX_ = -MAX_ZASOBNIK;
    if (zasY_ > MAX_ZASOBNIK) zasY_ = MAX_ZASOBNIK;
    if (zasY_ < -MAX_ZASOBNIK) zasY_ = -MAX_ZASOBNIK;
  }
  // posun prstu (body stranky) -> body mysi: cela sirka obrazu = 320 x citlivost
  void pridejDotyk_(float dx, float dy) {
    const double k = (double)BODU_NA_SIRKU / (double)sirka_ * (citlivost_ / 100.0);
    pridej_(dx * k, dy * k);
  }
  void klik_(int b) { if (fronta_[b] < 4) fronta_[b]++; }
  void resetGest_() {
    pocet_ = 0; gesto_ = NIC; leveDrzi_ = false;
    zasX_ = zasY_ = 0.0;
    for (int b = 0; b < 2; b++) fronta_[b] = dole_[b] = mezera_[b] = 0;
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
      if (!z.klic.empty()) v.push_back(z);
    }
    std::fclose(f);
    return v;
  }
};

}  // namespace nap
