// nap_tv_proud_jpeg.h
// B305: PS1 NA TV / PC CELA V C++ - KODEK "JPEG PO CASTECH" (verze paketu 3).
//
// PlayStation kresli 3D s tisici barev - bez ztraty by to bylo 10x vic dat,
// nez Wi-Fi do TV unese. Proto JPEG, ale:
//  - v puvodnim rozliseni hry (320x240, 512x240, 640x480 ...), ne zvetseny na
//    1280x720 jako driv v Jave (B301: 27-33 ms na snimek na hlavnim vlakne),
//    zvetsi az prohlizec,
//  - jen ta cast obrazu, ktera se zmenila (ctverce 16x16, obdelnik kolem nich),
//    stejny snimek (hra na 30 snimku/s, menu) = nic,
//  - kdyz obraz chvili stoji (menu, text), posle se jeste jednou ve vysoke
//    kvalite (94, barvy bez podvzorkovani) - text je pak ostry,
//  - kvalita se ridi siti: kdyz Java nestiha odesilat (zapis do site dlouho
//    ceka), kvalita klesne (mensi snimky), kdyz je sit volna, zase stoupne.
// Koduje se ve vlakne prohlizece (ne v emulaci), kazdy prohlizec zvlast;
// velky obraz (512x480, 640x480) na 2 pasy soubezne (nap_tv_proud_jpegenc.h).
//
// B313 (hlavni menu na TV v 1280x720, Rene: "grafika hlavniho menu je rozmazana"):
// volby, ktere zapina jen menu (PS1 a demo jedou jako drive):
//  - viceObdelniku: zmenene ctverce jako nekolik obdelniku (konzole na prostredni
//    karte, vedlejsi karty), ne jeden velky kolem vseho - kazdy obdelnik je vlastni
//    JPEG; snimek pak jde jako nekolik paketu za sebou (dalsi maji priznak 8 a zadny
//    zvuk - prohlizec kazdy jen nakresli na jeho misto, i stary),
//  - jemneBloky: ctverce, ktere uz chvili stoji (12 snimku neodesly), se zjemni na
//    kvalitu 94 i tehdy, kdyz se jinde neco hybe (drive jen kdyz stal cely obraz -
//    v menu se ale porad neco toci, takze se nezjemnilo nikdy: "zjemneni 0"),
//  - maxCasti 4 (menu nema emulaci) a qStrop (vlastni strop kvality),
//  - qVelkaZmena: pri listovani (meni se vic nez 40 % obrazu) nizsi kvalita - mene dat,
//    plynulejsi pohyb; qZpetMs: kvalita po zpomaleni site stoupa rychleji.
//
// B319 (Rene: "mam pocit, ze doslo ke zpomaleni pri prenosu LAN"; log B318: Dune 2000 na PC
// 48-57 snimku/s misto 59 - pri posunu mapy se menil cely obraz 736x480 20x za sekundu,
// ~35 kB na snimek, az 6 Mbit/s, Wi-Fi nestihala). Dve volby, ktere umi stranka WEB TV
// od B319 (dohodne je adresou proudu, starsi stranka je nedostane - nap_ps1_native.cpp):
//  - posun: kdyz se velka cast obrazu posunula (mapa), prohlizec ji zkopiruje ve svem
//    canvasu (cast "kopie", zadna data obrazu) a JPEG jde jen to, co je opravdu nove
//    (pruh na kraji, jednotky, kurzor),
//  - polovicni: cast, ktera je presne 2x zvetsena (2x Enhanced Resolution u 2D her - kazde
//    2x2 body stejne), jde v polovicnim rozliseni (barvy 4:4:4, kvalita +10 - stejna vernost,
//    ~50-60 % dat) a prohlizec ji nakresli 2x bez vyhlazeni = presne totez.
//
// B320 (log B319 z telefonu, Dune 2000 po F5): posun obrazu zkousen 322x, nalezen 0x; polovicni
// rozliseni jen 304 casti z 1137. Na mem umelem obraze (PC) to fungovalo, na skutecne hre ne.
// Pricina: PS1 kresli s DITHERINGEM (v logu "dithering ZAPNUTO") - mrizka 4x4 je pevne na obrazovce,
// ne na mape. Po posunu ma tentyz kus mapy o +-1 odstin (5bitova barva) jine body a ve 2x zvetsenem
// obrazu se lisi i 4 body jednoho bodu hry. Presne porovnani pak nenajde nic. Ted "stejny obsah"
// = kazdy kanal se lisi nejvys o toleranci (1 odstin = 8-9 v 8 bitech -> 10); opravdove zmeny
// (jednotky, kurzor, nova cast mapy) jsou mnohem vetsi. Prohlizec se nemeni (kopie a polovicni
// rozliseni jako v B319) - lisi se jen o sum ditheringu, ktery oko nevidi a JPEG ma vetsi.
//  - hledani posunu s toleranci: vzorek 8 bodu ve 2 radcich s hranou (jinak by ve 2x obrazu sedel
//    i o bod vedle), hlasuje pro vsechny posuny, kde sedi - spravny dostane hlas od kazdeho vzorku,
//  - pred = to, co prohlizec opravdu ma (i po kopii s toleranci). Ctverec z takove kopie (tol_) se
//    pak bere jako nezmeneny, dokud se od hry lisi nejvys o toleranci - chyba se tak nikdy nescita
//    (ani pri spatne nalezenem posunu na hladke plose); kdyz obraz stoji, zjemni se i on (presne),
//  - skupina zmen ve tvaru L (sikmy posun: novy pruh nahore i vlevo) se posila jako 2 obdelniky,
//    ne jeden pres skoro cely obraz (rozdelSkupinu).
// Overeno na SKUTECNEM vykreslovaci gpu_neon z appky (test_overeni/b320/integrace_b320_gpu.cpp):
// mapa z polygonu s texturou a ditheringem ve 2x - B319 pri liche fazi 0 kopii, B320 vsechny, dat
// 17 % drivejska. Polygony ve 2x ale nejsou presne 2x zvetsene (body textury posunute o bod, na
// hranach dlazdic jinak) - polovicni rozliseni u nich nejde ani s toleranci (jen u spritu).
//
// CAST OBRAZU V PAKETU (verze 3):
//   u16 x, u16 y, u16 w, u16 h   obdelnik, ktery se meni (w = 0: beze zmeny)
//   u8  kvalita                  B319: bit7 = JPEG v polovicnim rozliseni (w/2 x h/2), kreslit 2x;
//                                0xFF = KOPIE: obdelnik w x h se zkopiruje z canvasu z mista
//                                (zdroj x, zdroj y) na (x, y) - delka 4, data u16 zdroj x, u16 zdroj y
//   u32 delka JPEG, pak JPEG (baseline, ctverce zarovnane na 16)
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include "nap_tv_proud_spol.h"
#include "nap_tv_proud_jpegenc.h"

namespace nap {
namespace proud {

struct KodekJpeg {
  static constexpr int VERZE = 3;
  static const char *nazev() { return "JPEG po castech"; }
  static constexpr int Q_KLID = 94;          // zjemneni stojiciho obrazu
  static constexpr int Q_MIN = 45;
  static constexpr int KLID_SNIMKU = 12;     // ~0,2-0,4 s beze zmeny -> zjemnit
  static constexpr int MAX_OBDELNIKU = 6;    // B313: nejvic casti (JPEG) jednoho snimku
  static constexpr int SLOUCIT_CTVERCU = 4;  // B313: dva obdelniky spojit, kdyz to prida <= 4 ctverce 16x16

  // obdelnik ve ctvercich 16x16 (x1, y1 za koncem)
  struct Obd { int x0 = 0, y0 = 0, x1 = 0, y1 = 0; int plocha() const { return (x1 - x0) * (y1 - y0); } };

  int W = 0, H = 0, bw = 0, bh = 0;
  bool ma = false;
  std::vector<uint32_t> pred;            // posledni poslany snimek (puvodni body)
  std::vector<uint8_t> kval;             // kvalita, ve ktere ma prohlizec ctverec 16x16
  std::vector<uint8_t> zmena;
  std::vector<uint32_t> poslano;         // B313: cislo snimku, kdy sel ctverec naposledy do prohlizece
  std::vector<uint8_t> jemnit, navstiveno;
  std::vector<int> zasobnik;
  std::vector<Obd> obdZm, obdJem;
  uint32_t cislo = 0;                    // B313: snimky tohoto prohlizece
  int klid = 0;
  int q = 0, qMax = 0;
  double zapis = 0;                      // prumerna doba zapisu do site (ms)
  long long qMs = 0;
  JpegEnc enc;
  std::vector<uint8_t> jpg;
  int casti = 1;                         // pasu soubezne (podle velikosti obdelniku)
  // B313: nastaveni pro hlavni menu na TV (1280x720). PS1 / demo zustavaji, jak byly.
  int maxCasti = 2;                      // pasu pro velky obraz (> 520 000 bodu); menu nema emulaci
  int qStrop = 0;                        // > 0: nejvyssi kvalita misto maxKvalita()
  bool viceObdelniku = false;            // zmenene casti jako nekolik obdelniku (ne jeden kolem vseho)
  bool jemneBloky = false;               // stojici casti zjemnit, i kdyz se jinde neco hybe
  int qVelkaZmena = 0;                   // > 0: strop kvality, kdyz se meni vic nez 40 % obrazu (listovani)
  int qZpetMs = 1500;                    // jak casto smi kvalita po zpomaleni site zase stoupnout
  // B313: dalsi casti tehoz snimku - Hub je posle jako dalsi pakety (priznak 8, bez zvuku)
  size_t dalsiN = 0;
  std::vector<std::vector<uint8_t>> dalsi;
  // B319: volby, ktere umi stranka WEB TV od B319 (nastavKodekPs1 - jen kdyz si je stranka rekla)
  static constexpr uint8_t KV_KOPIE = 0xFF;        // cast = kopie obdelniku v canvasu prohlizece
  static constexpr uint8_t KV_POLOVICNI = 0x80;    // priznak v bajtu kvality: JPEG v polovicnim rozliseni
  static constexpr int POSUN_R = 64;               // nejvetsi hledany posun (bodu) mezi dvema poslanymi snimky
  static constexpr int POSUN_VZORKU = 48;          // vzorku (8 bodu v radku) ze zmenenych ctvercu
  bool posun = false;                    // posun obrazu = kopie v prohlizeci + JPEG jen noveho
  bool polovicni = false;                // presne 2x zvetsene casti v polovicnim rozliseni
  // B320: tolerance (8 bitu na kanal) pro "stejny obsah" u posunu a polovicniho rozliseni - dithering
  // PS1 (0 = presne jako B319). Zmeny ctvercu (spoctiZmeny) zustavaji presne - s toleranci jen ve
  // ctvercich, ktere prohlizec ma z kopie s toleranci (tol_).
  static constexpr int TOLERANCE_PS1 = 10;
  int tolerance = 0;
  // statistika (do logu)
  long long klicovych = 0, snimku = 0, beze = 0, zjemneni = 0, snizeni = 0, castecnych = 0;
  long long obdelniku = 0, zjemneniZaPohybu = 0, vicecastnych = 0;   // B313
  long long posunu = 0, posunZkousek = 0, posunCtvercu = 0, polovicnich = 0;   // B319
  long long posunCtvercuTol = 0, polovicnichTol = 0;   // B320: z toho shodne jen s toleranci (dithering)
  long long posunTol = 0, tolZjemneno = 0;              // B320: posunu nalezenych jen s toleranci, ctvercu zjemnenych po zastaveni
  long long rozdelenychSkupin = 0;                      // B320: skupin ve tvaru L rozdelenych na obdelniky
  // B320: rozbor pro log (co z toho na skutecne hre plati): hledani s toleranci - kolikrat, kolikrat
  // malo hlasu (hladka mapa / jiny pohyb nez posun), kolikrat vitez neprosel overenim
  long long tolHledani = 0, tolMaloHlasu = 0, tolNeovereno = 0;
  double kodMsSoucet = 0, kodMsMax = 0;
  long long kodN = 0;

  void zapomen() { ma = false; }
  static int maxKvalita(int w, int h) {
    const size_t n = (size_t)w * h;
    return n <= 110000 ? 88 : n <= 200000 ? 82 : 76;
  }
  std::string info() const {
    char b[512];
    snprintf(b, sizeof(b), "JPEG kvalita %d (max %d, snizeno %lldx), kodovani prumer %.1f ms max %.1f ms (pasu %d), castecnych %lld, zjemneni %lld, beze zmeny %lld z %lld",
             q, qMax, snizeni, kodN ? kodMsSoucet / kodN : 0.0, kodMsMax, casti, castecnych, zjemneni, beze, snimku);
    std::string s = b;
    if (viceObdelniku || jemneBloky) {
      snprintf(b, sizeof(b), ", B313: obdelniku %lld (snimku s vic castmi %lld), zjemneni za pohybu %lld", obdelniku, vicecastnych, zjemneniZaPohybu);
      s += b;
    }
    if (posun || polovicni) {
      snprintf(b, sizeof(b), ", B319: posun obrazu %lld x (zkouseno %lld, ctvercu 16x16 zkopirovano %lld), casti v polovicnim rozliseni %lld z %lld",
               posunu, posunZkousek, posunCtvercu, polovicnich, obdelniku);
      s += b;
      if (tolerance > 0) {
        snprintf(b, sizeof(b), ", B320: tolerance ditheringu %d - posunu nalezenych jen s toleranci %lld (hledano %lld x, malo hlasu %lld, neovereno %lld), shodnych jen s toleranci: ctvercu %lld, casti v polovicnim %lld, po zastaveni zpresneno ctvercu %lld, skupin L rozdeleno %lld",
                 tolerance, posunTol, tolHledani, tolMaloHlasu, tolNeovereno, posunCtvercuTol, polovicnichTol, tolZjemneno, rozdelenychSkupin);
        s += b;
      }
    }
    return s;
  }

  unsigned zakoduj(const uint32_t *F, int w, int h, int zapisMs, long long ted, std::vector<uint8_t> &out) {
    snimku++;
    cislo++;
    dalsiN = 0;
    kodSnimku_ = 0;
    const size_t n = (size_t)w * h;
    const bool klic = !ma || w != W || h != H || pred.size() != n;
    if (klic) {
      W = w; H = h; bw = (W + 15) / 16; bh = (H + 15) / 16;
      kval.assign((size_t)bw * bh, 0);
      poslano.assign((size_t)bw * bh, cislo);
      tol_.assign((size_t)bw * bh, 0);     // B320
      tolMa_ = false;
      qMax = qStrop > 0 ? qStrop : maxKvalita(W, H);
      if (q <= 0 || q > qMax) q = qMax;
      klicovych++;
      posunPosl_ = false;                  // B319
    }
    // kvalita podle site: Java hlasi, jak dlouho cekal zapis predchoziho paketu
    zapis = zapis * 0.8 + std::max(0, zapisMs) * 0.2;
    if (ted - qMs >= 600) {
      if (zapis > 12.0 && q > Q_MIN) { q = std::max(Q_MIN, q - 6); qMs = ted; snizeni++; }
      else if (zapis < 3.0 && q < qMax && ted - qMs >= qZpetMs) { q = std::min(qMax, q + 4); qMs = ted; }
    }
    if (klic) {
      pred.assign(F, F + n);
      Obd o; o.x1 = bw; o.y1 = bh;
      obdelnik(F, o, q, out);
      ma = true;
      zapisStatistiku();
      return 1u;
    }
    const bool neco = spoctiZmeny(F);
    if (!neco) {
      // cely obraz stoji: po chvilce jeste jednou ve vysoke kvalite (jako drive)
      if (++klid < KLID_SNIMKU) { beze++; prazdna(out); return 4u; }
      bool zjemnit = false;
      for (size_t i = 0; i < kval.size(); i++) if (kval[i] < Q_KLID) { zmena[i] = 1; zjemnit = true; }
      // B320: ctverce z kopie s toleranci (sum ditheringu z driv) - ted presne, jako zbytek
      if (tolMa_)
        for (size_t i = 0; i < tol_.size(); i++) if (tol_[i]) { tolZjemneno++; zmena[i] = 1; zjemnit = true; }
      if (!zjemnit) { beze++; prazdna(out); return 4u; }
      zjemneni++;
      obdelnik(F, ohraniceni(zmena), Q_KLID, out);
      zapisStatistiku();
      return 0u;
    }
    klid = 0;
    // B319: posun obrazu (mapa ve hre) - prohlizec ho zkopiruje ve svem canvasu, JPEG pak jen to,
    // co je opravdu nove (pruh na kraji, jednotky, kurzor)
    Obd ko;
    int vx = 0, vy = 0;
    const bool kop = posun && najdiPosun(F, ko, vx, vy);
    if (kop) spoctiZmeny(F);
    // B313: casti, ktere uz chvili stoji a prohlizec je ma v horsi kvalite, se zjemni hned
    // (menu: konzole na karte se toci porad, zbytek obrazovky stoji). Po kouscich (nejvys
    // ctvrtina obrazu za snimek) a jen kdyz sit stiha - at se neucpe.
    obdJem.clear();
    if (jemneBloky && zapis < 8.0) {
      jemnit.assign((size_t)bw * bh, 0);
      const int rozpocet = std::max(bw * 2, bw * bh / 4);
      int pocet = 0;
      for (int by = 0; by < bh && pocet < rozpocet; by++)
        for (int bx = 0; bx < bw; bx++) {
          const size_t i = (size_t)by * bw + bx;
          if (!zmena[i] && kval[i] < Q_KLID && cislo - poslano[i] >= (uint32_t)KLID_SNIMKU) { jemnit[i] = 1; pocet++; }
        }
      if (pocet > 0) { najdiObdelniky(jemnit, obdJem); zjemneniZaPohybu++; }
    }
    najdiObdelniky(zmena, obdZm);
    // B313: meni se skoro vse (listovani v menu) - pohyb oko neprecte, staci nizsi kvalita
    // (mene dat, plynulejsi); jakmile se zastavi, stojici casti se zjemni na 94
    // (kdyz sit nestiha a q kleslo, klesne o tolik i tahle kvalita)
    int kvZmen = q;
    if (qVelkaZmena > 0) {
      long long plocha = 0;
      for (const Obd &o : obdZm) plocha += o.plocha();
      if (plocha * 10 > (long long)bw * bh * 4) kvZmen = std::max(Q_MIN, std::min(q, qVelkaZmena - (qMax - q)));
    }
    // poradi: (B319) kopie, nejdriv zjemneni, pak zmeny (oboje z tehoz snimku - prekryv nevadi)
    size_t k = 0;
    auto cil = [&]() -> std::vector<uint8_t> & { return k++ == 0 ? out : dalsiCast(k - 2); };
    if (kop) kopie(cil(), ko, vx, vy);
    for (const Obd &o : obdJem) obdelnik(F, o, Q_KLID, cil());
    for (const Obd &o : obdZm) obdelnik(F, o, kvZmen, cil());
    if (k == 0) prazdna(out);                    // (nenastane - aspon jeden ctverec se zmenil)
    dalsiN = k > 0 ? k - 1 : 0;
    if (dalsiN > 0) vicecastnych++;
    zapisStatistiku();
    return 0u;
  }

 private:
  double kodSnimku_ = 0;
  // B319
  struct Hlas { int dx, dy, n; };
  std::vector<Hlas> hlasy_;
  bool posunPosl_ = false;                 // naposledy pouzity posun (zkusi se jako prvni)
  int posunPoslX_ = 0, posunPoslY_ = 0;
  int posunNeuspech_ = 0;                  // kolikrat po sobe se posun nenasel
  std::vector<uint8_t> kvalPred_, shoda_;    // B320: shoda_ = ktere ctverce sedi po posunu (1 presne, 2 s toleranci)
  std::vector<uint8_t> tol_;               // B320: ctverec ma prohlizec z kopie s toleranci (lisi se od hry sumem ditheringu)
  bool tolMa_ = false;                     // B320: nejaky takovy ctverec je
  struct Kand { int dx, dy; };
  std::vector<Hlas> poradiHlasu_;
  std::vector<int> clenove_;               // B320: ctverce jedne skupiny (najdiObdelniky)
  std::vector<uint8_t> skupMaska_;
  std::vector<Obd> rozdeleno_;
  std::vector<uint32_t> poslanoPred_, polBuf_;

  // ctverce 16x16, ktere se oproti minule poslanemu snimku (pred) zmenily
  // B320: ctverec, ktery ma prohlizec z kopie s toleranci (tol_), se zmenil, az kdyz se nejaky bod
  // lisi o vic nez toleranci (sum ditheringu ne) - pred je porad to, co prohlizec opravdu ma
  bool spoctiZmeny(const uint32_t *F) {
    zmena.assign((size_t)bw * bh, 0);
    bool neco = false;
    const bool sTol = tolerance > 0 && tolMa_;
    for (int y = 0; y < H; y++) {
      const uint32_t *a = F + (size_t)y * W, *b = &pred[(size_t)y * W];
      if (std::memcmp(a, b, (size_t)W * 4) == 0) continue;
      uint8_t *zr = &zmena[(size_t)(y / 16) * bw];
      const uint8_t *tr = sTol ? &tol_[(size_t)(y / 16) * bw] : nullptr;
      for (int bx = 0; bx < bw; bx++) {
        if (zr[bx]) continue;
        const int xa = bx * 16, nx = std::min(16, W - xa);
        if (std::memcmp(a + xa, b + xa, (size_t)nx * 4) == 0) continue;
        if (tr && tr[bx]) {
          bool blz = true;
          for (int i = 0; i < nx && blz; i++) blz = blizko(a[xa + i], b[xa + i], tolerance);
          if (blz) continue;
        }
        zr[bx] = 1; neco = true;
      }
    }
    return neco;
  }

  // B320: barvy se lisi v kazdem kanalu nejvys o T (dithering)
  static inline bool blizko(uint32_t a, uint32_t b, int T) {
    if (a == b) return true;
    const unsigned T2 = (unsigned)(2 * T);
    return (unsigned)((int)((a >> 8) & 255) - (int)((b >> 8) & 255) + T) <= T2 &&
           (unsigned)((int)((a >> 16) & 255) - (int)((b >> 16) & 255) + T) <= T2 &&
           (unsigned)((int)(a & 255) - (int)(b & 255) + T) <= T2;
  }
  // B319: ctverec (bx, by) noveho snimku = minuly snimek posunuty o (vx, vy)? (zdroj musi byt v obrazu)
  // B320: vraci 0 = ne, 1 = presne stejny, 2 = stejny jen s toleranci (dithering)
  int stejnyPoPosunu(const uint32_t *F, int bx, int by, int vx, int vy) const {
    const int px0 = bx * 16, py0 = by * 16, px1 = std::min(W, px0 + 16), py1 = std::min(H, py0 + 16);
    const int sx0 = px0 - vx, sy0 = py0 - vy;
    if (sx0 < 0 || sy0 < 0 || sx0 + (px1 - px0) > W || sy0 + (py1 - py0) > H) return 0;
    int vysl = 1;
    const int n = px1 - px0;
    for (int y = py0; y < py1; y++) {
      const uint32_t *a = F + (size_t)y * W + px0, *b = &pred[(size_t)(y - vy) * W + sx0];
      if (std::memcmp(a, b, (size_t)n * 4) == 0) continue;
      if (tolerance <= 0) return 0;
      for (int i = 0; i < n; i++) if (!blizko(a[i], b[i], tolerance)) return 0;
      vysl = 2;
    }
    return vysl;
  }

  // B319: POSUN OBRAZU. Zmenila se velka cast obrazu (posun mapy ve hre) - o kolik se obsah posunul?
  // Vzorky (8 bodu v radku, aspon 3 ruzne barvy) ze zmenenych ctvercu se hledaji v minulem snimku
  // (+-POSUN_R bodu), kazdy jednoznacny nalez hlasuje. Vitezny vektor se overi bod po bodu: ctverce,
  // ktere jsou po posunu PRESNE stejne, prohlizec zkopiruje (obdelnik kolem nich). Kopie se provede
  // i tady v pred (+ kvalita a stari ctvercu jdou s obsahem) - zmeny se pak spocitaji znovu.
  // Vraci obdelnik ve ctvercich (ko) a vektor: obsah z (x, y) je ted na (x + vx, y + vy).
  bool najdiPosun(const uint32_t *F, Obd &ko, int &vx, int &vy) {
    int zmen = 0;
    for (uint8_t z : zmena) zmen += z;
    if (zmen < std::max(24, (bw * bh) / 10)) return false;
    posunZkousek++;
    // nejdriv minuly posun (a jeho 2x, 3x - prohlizec mezitim snimek vynechal): mapa se posouva
    // porad stejne, overeni je levne a hledani se usetri
    if (posunPosl_) {
      for (int nas = 1; nas <= 3; nas++) {
        const int cx = posunPoslX_ * nas, cy = posunPoslY_ * nas;
        if (std::abs(cx) >= W || std::abs(cy) >= H) break;
        if (pouzijPosun(F, cx, cy, zmen, ko)) { vx = cx; vy = cy; posunNeuspech_ = 0; return true; }
      }
    }
    // hra, ktera se neposouva (3D - meni se vse jinak): po 5 neuspesich hledat jen kazdy 8. snimek
    if (posunNeuspech_ >= 5 && (cislo & 7) != 0) return false;
    // B320: nejdriv presne (jako B319 - hra bez ditheringu), kdyz nic, tak s toleranci ditheringu
    bool ok = hledejPosun(F, zmen, ko, vx, vy);
    if (!ok && tolerance > 0 && hledejPosunTol(F, zmen, ko, vx, vy)) { ok = true; posunTol++; }
    posunNeuspech_ = ok ? 0 : posunNeuspech_ + 1;
    return ok;
  }

  // B319: hlasovani vzorku (viz najdiPosun) - presne (B320: beze zmeny)
  bool hledejPosun(const uint32_t *F, int zmen, Obd &ko, int &vx, int &vy) {
    const int maxNalezu = 4;
    hlasy_.clear();
    const int krok = std::max(1, zmen / POSUN_VZORKU);
    int poradi = 0, vzorku = 0, sNalezem = 0;
    static const int RADKY[4] = {5, 11, 2, 14};
    for (int by = 0; by < bh && vzorku < POSUN_VZORKU; by++)
      for (int bx = 0; bx < bw && vzorku < POSUN_VZORKU; bx++) {
        if (!zmena[(size_t)by * bw + bx]) continue;
        if (poradi++ % krok) continue;
        const int x = bx * 16 + 4;
        if (x + 8 > W) continue;
        int y = -1;
        for (int r : RADKY) {
          const int yy = by * 16 + r;
          if (yy >= H) continue;
          const uint32_t *s = F + (size_t)yy * W + x;
          int ruznych = 1;
          for (int i = 1; i < 8 && ruznych < 3; i++) {
            bool nova = true;
            for (int j = 0; j < i; j++) if (s[i] == s[j]) { nova = false; break; }
            if (nova) ruznych++;
          }
          if (ruznych >= 3) { y = yy; break; }
        }
        if (y < 0) continue;                       // jednolita plocha - je "vsude", nehlasuje
        vzorku++;
        const uint32_t *s = F + (size_t)y * W + x;
        int nalezu = 0, ndx[4] = {0, 0, 0, 0}, ndy[4] = {0, 0, 0, 0};
        const int dxMin = std::max(-POSUN_R, x + 8 - W), dxMax = std::min(POSUN_R, x);
        for (int dy = -POSUN_R; dy <= POSUN_R && nalezu <= maxNalezu; dy++) {
          const int sy = y - dy;
          if (sy < 0 || sy >= H) continue;
          const uint32_t *r = &pred[(size_t)sy * W];
          for (int dx = dxMin; dx <= dxMax; dx++) {
            const uint32_t *q = r + (x - dx);
            if (q[0] != s[0] || q[7] != s[7] || std::memcmp(q + 1, s + 1, 6 * 4) != 0) continue;
            if (dx == 0 && dy == 0) continue;     // tenhle kousek se nehnul
            if (nalezu < 4) { ndx[nalezu] = dx; ndy[nalezu] = dy; }
            if (++nalezu > maxNalezu) break;
          }
        }
        if (nalezu == 0 || nalezu > maxNalezu) continue;   // nic, nebo nejednoznacne (opakujici se vzor)
        sNalezem++;
        for (int i = 0; i < nalezu; i++) {
          bool bylo = false;
          for (Hlas &h : hlasy_)
            if (h.dx == ndx[i] && h.dy == ndy[i]) { h.n++; bylo = true; break; }
          if (!bylo) hlasy_.push_back({ndx[i], ndy[i], 1});
        }
      }
    if (hlasy_.empty()) return false;
    const Hlas *nej = &hlasy_[0];
    for (const Hlas &h : hlasy_) if (h.n > nej->n) nej = &h;
    if (nej->n < 4 || nej->n * 10 < sNalezem * 3) return false;
    if (!pouzijPosun(F, nej->dx, nej->dy, zmen, ko)) return false;
    vx = nej->dx; vy = nej->dy;
    return true;
  }

  // B320: HLEDANI POSUNU S TOLERANCI (dithering PS1 - presne hledani nenaslo nic).
  // Vzorek = 8 bodu ve 2 radcich pod sebou (y liche: ve 2x obrazu dva ruzne radky hry) a musi mit
  // hranu - sousedni body (vedle sebe nebo pod sebou) se lisi o vic nez 2T+4. Sum ditheringu je
  // mensi (+-1 odstin), takze o bod vedle vzorek s hranou nesedi. Hlasuje pro VSECHNY posuny, kde
  // sedi (nejvys 6): bez svisle hrany sedi ve 2x obrazu i o radek vedle, bez vodorovne i o sloupec -
  // spravny posun ale dostane hlas od kazdeho vzorku, sousedni jen od nekterych. Overi se nejlepsi
  // (a kdyz neprojde, druhy).
  static constexpr int TOL_NALEZU = 6;
  static bool maHranu(const uint32_t *s, const uint32_t *s2, int h) {
    for (int i = 0; i < 8; i++) {
      if (rozdil(s[i], s2[i]) >= h) return true;
      if (i < 7 && (rozdil(s[i], s[i + 1]) >= h || rozdil(s2[i], s2[i + 1]) >= h)) return true;
    }
    return false;
  }
  static inline int rozdil(uint32_t a, uint32_t b) {
    const int dr = std::abs((int)((a >> 16) & 255) - (int)((b >> 16) & 255));
    const int dg = std::abs((int)((a >> 8) & 255) - (int)((b >> 8) & 255));
    const int db = std::abs((int)(a & 255) - (int)(b & 255));
    return std::max(dr, std::max(dg, db));
  }
  bool hledejPosunTol(const uint32_t *F, int zmen, Obd &ko, int &vx, int &vy) {
    const int T = tolerance;
    tolHledani++;
    hlasy_.clear();
    const int krok = std::max(1, zmen / POSUN_VZORKU);
    int poradi = 0, vzorku = 0, sNalezem = 0;
    static const int RADKY[6] = {5, 11, 1, 9, 13, 3};
    const int hrana = 2 * T + 4;
    const unsigned T2 = (unsigned)(2 * T);
    for (int by = 0; by < bh && vzorku < POSUN_VZORKU; by++)
      for (int bx = 0; bx < bw && vzorku < POSUN_VZORKU; bx++) {
        if (!zmena[(size_t)by * bw + bx]) continue;
        if (poradi++ % krok) continue;
        const int x = bx * 16 + 4;
        if (x + 8 > W) continue;
        int y = -1;
        for (int r : RADKY) {
          const int yy = by * 16 + r;
          if (yy + 1 >= H) continue;
          const uint32_t *s = F + (size_t)yy * W + x;
          if (maHranu(s, s + W, hrana)) { y = yy; break; }
        }
        if (y < 0) continue;                       // hladka plocha - sedela by skoro vsude, nehlasuje
        vzorku++;
        const uint32_t *s = F + (size_t)y * W + x, *s2 = s + W;
        const int s0r = (int)((s[0] >> 16) & 255), s0g = (int)((s[0] >> 8) & 255), s0b = (int)(s[0] & 255);
        Kand nal[TOL_NALEZU + 1];
        int nalezu = 0;
        const int dxMin = std::max(-POSUN_R, x + 8 - W), dxMax = std::min(POSUN_R, x);
        for (int dy = -POSUN_R; dy <= POSUN_R && nalezu <= TOL_NALEZU; dy++) {
          const int sy = y - dy;
          if (sy < 0 || sy + 1 >= H) continue;
          const uint32_t *r = &pred[(size_t)sy * W];
          for (int dx = dxMin; dx <= dxMax; dx++) {
            const uint32_t *q = r + (x - dx);
            // nejdriv levne prvni bod (zelena nejvic rozlisuje), pak zbytek obou radku
            const uint32_t v = q[0];
            if ((unsigned)((int)((v >> 8) & 255) - s0g + T) > T2 || (unsigned)((int)((v >> 16) & 255) - s0r + T) > T2 ||
                (unsigned)((int)(v & 255) - s0b + T) > T2) continue;
            bool ok = true;
            for (int i = 1; i < 8 && ok; i++) ok = blizko(q[i], s[i], T);
            const uint32_t *q2 = q + W;
            for (int i = 0; i < 8 && ok; i++) ok = blizko(q2[i], s2[i], T);
            if (!ok) continue;
            if (dx == 0 && dy == 0) continue;     // tenhle kousek se nehnul
            nal[nalezu].dx = dx; nal[nalezu].dy = dy;
            if (++nalezu > TOL_NALEZU) break;
          }
        }
        if (nalezu == 0 || nalezu > TOL_NALEZU) continue;   // nic, nebo hladke / opakujici se
        sNalezem++;
        for (int i = 0; i < nalezu; i++) {
          bool bylo = false;
          for (Hlas &h : hlasy_)
            if (h.dx == nal[i].dx && h.dy == nal[i].dy) { h.n++; bylo = true; break; }
          if (!bylo) hlasy_.push_back({nal[i].dx, nal[i].dy, 1});
        }
      }
    if (hlasy_.empty()) { tolMaloHlasu++; return false; }
    // poradi: nejvic hlasu; pri shode blizsi minulemu posunu
    poradiHlasu_ = hlasy_;
    const int lx = posunPosl_ ? posunPoslX_ : 0, ly = posunPosl_ ? posunPoslY_ : 0;
    std::stable_sort(poradiHlasu_.begin(), poradiHlasu_.end(), [&](const Hlas &a, const Hlas &b) {
      if (a.n != b.n) return a.n > b.n;
      return std::abs(a.dx - lx) + std::abs(a.dy - ly) < std::abs(b.dx - lx) + std::abs(b.dy - ly);
    });
    bool zkousel = false;
    for (size_t i = 0; i < poradiHlasu_.size() && i < 2; i++) {
      const Hlas &h = poradiHlasu_[i];
      if (h.n < 4 || h.n * 10 < sNalezem * 3) break;
      zkousel = true;
      if (pouzijPosun(F, h.dx, h.dy, zmen, ko)) { vx = h.dx; vy = h.dy; return true; }
    }
    if (zkousel) tolNeovereno++; else tolMaloHlasu++;
    return false;
  }

  // B319: overeni posunu (vx, vy) bod po bodu a kdyz se vyplati, kopie i tady v pred
  bool pouzijPosun(const uint32_t *F, int vx, int vy, int zmen, Obd &ko) {
    // ktere zmenene ctverce jsou po posunu stejne (B320: s toleranci ditheringu)
    shoda_.assign((size_t)bw * bh, 0);
    int x0 = bw, y0 = bh, x1 = 0, y1 = 0, uspora = 0, usporaTol = 0;
    for (int by = 0; by < bh; by++)
      for (int bx = 0; bx < bw; bx++) {
        if (!zmena[(size_t)by * bw + bx]) continue;
        const int st = stejnyPoPosunu(F, bx, by, vx, vy);
        if (!st) continue;
        shoda_[(size_t)by * bw + bx] = (uint8_t)st;
        uspora++;
        if (st == 2) usporaTol++;
        x0 = std::min(x0, bx); y0 = std::min(y0, by); x1 = std::max(x1, bx + 1); y1 = std::max(y1, by + 1);
      }
    if (uspora < std::max(8, zmen / 4)) return false;
    // nezmenene ctverce uvnitr kopie musi po ni zustat stejne (jinak by se museli poslat znovu)
    int skoda = 0;
    for (int by = y0; by < y1; by++)
      for (int bx = x0; bx < x1; bx++) {
        const size_t i = (size_t)by * bw + bx;
        if (zmena[i]) continue;
        const int st = stejnyPoPosunu(F, bx, by, vx, vy);
        if (!st) skoda++;
        else shoda_[i] = (uint8_t)st;
      }
    if (skoda * 4 > uspora) return false;
    ko.x0 = x0; ko.y0 = y0; ko.x1 = x1; ko.y1 = y1;
    // kopie i tady (stejne jako v canvasu prohlizece): radky v poradi, ktere neprepise zdroj
    const int X0 = x0 * 16, Y0 = y0 * 16, X1 = std::min(W, x1 * 16), Y1 = std::min(H, y1 * 16);
    const size_t sirka = (size_t)(X1 - X0) * 4;
    if (vy > 0) {
      for (int y = Y1 - 1; y >= Y0; y--) std::memmove(&pred[(size_t)y * W + X0], &pred[(size_t)(y - vy) * W + X0 - vx], sirka);
    } else {
      for (int y = Y0; y < Y1; y++) std::memmove(&pred[(size_t)y * W + X0], &pred[(size_t)(y - vy) * W + X0 - vx], sirka);
    }
    // B320: ctverce shodne jen s toleranci (dithering): pred = to, co ma prohlizec po kopii (lisi se od
    // hry sumem ditheringu). Priznak tol_: spoctiZmeny je pak bere jako nezmenene, dokud se od hry
    // nelisi o vic nez toleranci (jinak by se hned zase poslaly JPEGem a kopie by nic neusetrila).
    // Ctverec presne shodny (nebo neshodny - pujde JPEGem) priznak nema.
    if (tolerance > 0 && tol_.size() == (size_t)bw * bh) {
      for (int by = y0; by < y1; by++)
        for (int bx = x0; bx < x1; bx++) {
          const size_t i = (size_t)by * bw + bx;
          tol_[i] = shoda_[i] == 2 ? 1 : 0;
          if (tol_[i]) tolMa_ = true;
        }
    }
    posunCtvercuTol += usporaTol;
    // kvalita a stari ctverce jdou s obsahem: ctverec po posunu = nejhorsi z (az 4) zdrojovych
    kvalPred_ = kval;
    poslanoPred_ = poslano;
    for (int by = y0; by < y1; by++)
      for (int bx = x0; bx < x1; bx++) {
        const int sx0 = bx * 16 - vx, sy0 = by * 16 - vy;
        const int sx1 = std::min(W, bx * 16 + 16) - 1 - vx, sy1 = std::min(H, by * 16 + 16) - 1 - vy;
        int kv = 255;
        uint32_t ps = 0xFFFFFFFFu;
        for (int sby = sy0 / 16; sby <= sy1 / 16; sby++)
          for (int sbx = sx0 / 16; sbx <= sx1 / 16; sbx++) {
            const size_t j = (size_t)sby * bw + sbx;
            kv = std::min(kv, (int)kvalPred_[j]);
            ps = std::min(ps, poslanoPred_[j]);
          }
        kval[(size_t)by * bw + bx] = (uint8_t)kv;
        poslano[(size_t)by * bw + bx] = ps;
      }
    posunu++;
    posunCtvercu += uspora;
    posunPosl_ = true; posunPoslX_ = vx; posunPoslY_ = vy;
    return true;
  }

  // B319: cast "kopie" - prohlizec zkopiruje obdelnik ve svem canvasu (zdroj = cil - posun)
  void kopie(std::vector<uint8_t> &cil, const Obd &ko, int vx, int vy) {
    const int X0 = ko.x0 * 16, Y0 = ko.y0 * 16, X1 = std::min(W, ko.x1 * 16), Y1 = std::min(H, ko.y1 * 16);
    p16(cil, (unsigned)X0); p16(cil, (unsigned)Y0); p16(cil, (unsigned)(X1 - X0)); p16(cil, (unsigned)(Y1 - Y0));
    p8(cil, KV_KOPIE);
    p32(cil, 4u);
    p16(cil, (unsigned)(X0 - vx)); p16(cil, (unsigned)(Y0 - vy));
  }

  // B319: je obdelnik presne 2x zvetseny (kazde 2x2 body stejne)? - 2D hra v 2x Enhanced Resolution
  // B320: vraci 0 = ne, 1 = presne, 2 = jen s toleranci T (dithering - 4 body se lisi o +-1 odstin)
  static int zdvojeny(const uint32_t *F, size_t stride, int w, int h, int T = 0) {
    int vysl = 1;
    for (int y = 0; y < h; y += 2) {
      const uint32_t *a = F + (size_t)y * stride, *b = a + stride;
      bool presne = std::memcmp(a, b, (size_t)w * 4) == 0;
      for (int x = 0; x < w && presne; x += 2) presne = a[x] == a[x + 1];
      if (presne) continue;
      if (T <= 0) return 0;
      for (int x = 0; x < w; x += 2) {
        const uint32_t c = a[x];
        if (!blizko(a[x + 1], c, T) || !blizko(b[x], c, T) || !blizko(b[x + 1], c, T)) return 0;
      }
      vysl = 2;
    }
    return vysl;
  }
  // B320: prumer 4 bodu (kazdy kanal zvlast, zaokrouhleno)
  static inline uint32_t prumer4(uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    const uint32_t lo = ((a & 0x00FF00FFu) + (b & 0x00FF00FFu) + (c & 0x00FF00FFu) + (d & 0x00FF00FFu) + 0x00020002u) >> 2;
    const uint32_t hi = (((a >> 8) & 0x00FF00FFu) + ((b >> 8) & 0x00FF00FFu) + ((c >> 8) & 0x00FF00FFu) + ((d >> 8) & 0x00FF00FFu) + 0x00020002u) >> 2;
    return (lo & 0x00FF00FFu) | ((hi & 0x00FF00FFu) << 8);
  }

  std::vector<uint8_t> &dalsiCast(size_t i) {
    if (dalsi.size() <= i) dalsi.resize(i + 1);
    dalsi[i].clear();
    return dalsi[i];
  }
  void zapisStatistiku() {
    kodMsSoucet += kodSnimku_; kodN++;
    if (kodSnimku_ > kodMsMax) kodMsMax = kodSnimku_;
  }
  static void prazdna(std::vector<uint8_t> &out) { p16(out, 0); p16(out, 0); p16(out, 0); p16(out, 0); p8(out, 0); p32(out, 0); }

  Obd ohraniceni(const std::vector<uint8_t> &m) const {
    Obd o; o.x0 = bw; o.y0 = bh; o.x1 = 0; o.y1 = 0;
    for (int by = 0; by < bh; by++)
      for (int bx = 0; bx < bw; bx++)
        if (m[(size_t)by * bw + bx]) {
          o.x0 = std::min(o.x0, bx); o.x1 = std::max(o.x1, bx + 1);
          o.y0 = std::min(o.y0, by); o.y1 = std::max(o.y1, by + 1);
        }
    return o;
  }
  static Obd spoj(const Obd &a, const Obd &b) {
    Obd u; u.x0 = std::min(a.x0, b.x0); u.y0 = std::min(a.y0, b.y0); u.x1 = std::max(a.x1, b.x1); u.y1 = std::max(a.y1, b.y1);
    return u;
  }

  // Ctverce v mape -> obdelniky: souvisle skupiny (i pres roh), blizke se spoji,
  // nejvys MAX_OBDELNIKU. Bez viceObdelniku (nebo kdyz je skupin moc): jeden kolem vseho.
  // B320: skupina ve tvaru L (po posunu mapy nahoru / dolu novy pruh dole + sloupec na hranici
  // vyhledu a panelu, ktery se zkopirovat neda) by mela obdelnik pres skoro cely obraz - kdyz je v nem
  // vic nez polovina ctvercu navic, rozdeli se na obdelniky z radku (rozdelSkupinu).
  void najdiObdelniky(const std::vector<uint8_t> &m, std::vector<Obd> &ven) {
    ven.clear();
    const Obd vse = ohraniceni(m);
    if (vse.x1 <= vse.x0) return;
    if (!viceObdelniku) { ven.push_back(vse); return; }
    navstiveno.assign(m.size(), 0);
    for (int sy = vse.y0; sy < vse.y1; sy++)
      for (int sx = vse.x0; sx < vse.x1; sx++) {
        const size_t s0 = (size_t)sy * bw + sx;
        if (!m[s0] || navstiveno[s0]) continue;
        if (ven.size() >= 32) { ven.clear(); ven.push_back(vse); return; }   // roztristene - jeden obdelnik
        Obd o; o.x0 = sx; o.y0 = sy; o.x1 = sx + 1; o.y1 = sy + 1;
        zasobnik.clear();
        zasobnik.push_back((int)s0);
        navstiveno[s0] = 1;
        clenove_.clear();
        while (!zasobnik.empty()) {
          const int i = zasobnik.back();
          zasobnik.pop_back();
          clenove_.push_back(i);
          const int x = i % bw, y = i / bw;
          o.x0 = std::min(o.x0, x); o.x1 = std::max(o.x1, x + 1);
          o.y0 = std::min(o.y0, y); o.y1 = std::max(o.y1, y + 1);
          for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
              const int nx = x + dx, ny = y + dy;
              if (nx < 0 || ny < 0 || nx >= bw || ny >= bh) continue;
              const size_t j = (size_t)ny * bw + nx;
              if (m[j] && !navstiveno[j]) { navstiveno[j] = 1; zasobnik.push_back((int)j); }
            }
        }
        if (tolerance > 0 && (int)clenove_.size() >= 6 && (int)clenove_.size() * 2 < o.plocha() && rozdelSkupinu(o, ven)) continue;
        ven.push_back(o);
      }
    // spojit blizke (spolecny obdelnik neni o moc vetsi nez oba zvlast) - JPEG ma hlavicku ~0,6 kB
    for (bool spojeno = true; spojeno;) {
      spojeno = false;
      for (size_t a = 0; a < ven.size() && !spojeno; a++)
        for (size_t b = a + 1; b < ven.size(); b++) {
          const Obd u = spoj(ven[a], ven[b]);
          if (u.plocha() <= ven[a].plocha() + ven[b].plocha() + SLOUCIT_CTVERCU) {
            ven[a] = u;
            ven.erase(ven.begin() + (long)b);
            spojeno = true;
            break;
          }
        }
    }
    // moc obdelniku: spojovat ty, ktere spolu pridaji nejmene ctvercu navic
    while (ven.size() > (size_t)MAX_OBDELNIKU) {
      size_t na = 0, nb = 1;
      int nej = 1 << 30;
      for (size_t a = 0; a < ven.size(); a++)
        for (size_t b = a + 1; b < ven.size(); b++) {
          const int navic = spoj(ven[a], ven[b]).plocha() - ven[a].plocha() - ven[b].plocha();
          if (navic < nej) { nej = navic; na = a; nb = b; }
        }
      ven[na] = spoj(ven[na], ven[nb]);
      ven.erase(ven.begin() + (long)nb);
    }
  }

  // B320: skupina (clenove_, obdelnik o) -> obdelniky z radku: v kazdem radku useky clenu, stejny
  // usek v radku pod nim prodlouzi obdelnik. Vic nez 8 obdelniku = nevyplati se (hlavicky JPEG).
  bool rozdelSkupinu(const Obd &o, std::vector<Obd> &ven) {
    const int w = o.x1 - o.x0, h = o.y1 - o.y0;
    skupMaska_.assign((size_t)w * h, 0);
    for (int i : clenove_) skupMaska_[(size_t)(i / bw - o.y0) * w + (i % bw - o.x0)] = 1;
    rozdeleno_.clear();
    size_t otevreneOd = 0;                   // obdelniky z minuleho radku = rozdeleno_[otevreneOd..]
    for (int y = 0; y < h; y++) {
      const size_t konecMinulych = rozdeleno_.size();
      for (int x = 0; x < w;) {
        if (!skupMaska_[(size_t)y * w + x]) { x++; continue; }
        int x2 = x;
        while (x2 < w && skupMaska_[(size_t)y * w + x2]) x2++;
        bool prodlouzen = false;
        for (size_t k = otevreneOd; k < konecMinulych; k++) {
          Obd &r = rozdeleno_[k];
          if (r.x0 == o.x0 + x && r.x1 == o.x0 + x2 && r.y1 == o.y0 + y) {
            // presune se mezi otevrene tohoto radku (na konec)
            Obd d = r; d.y1 = o.y0 + y + 1;
            r.y1 = -1;                         // znacka: prodlouzen (smaze se nize)
            rozdeleno_.push_back(d);
            prodlouzen = true;
            break;
          }
        }
        if (!prodlouzen) { Obd d; d.x0 = o.x0 + x; d.x1 = o.x0 + x2; d.y0 = o.y0 + y; d.y1 = o.y0 + y + 1; rozdeleno_.push_back(d); }
        x = x2;
      }
      // prodlouzene z minuleho radku pryc (jejich nova podoba je mezi otevrenymi)
      size_t zapis = otevreneOd;
      for (size_t k = otevreneOd; k < konecMinulych; k++)
        if (rozdeleno_[k].y1 != -1) rozdeleno_[zapis++] = rozdeleno_[k];
      const size_t uzavrenych = zapis;
      for (size_t k = konecMinulych; k < rozdeleno_.size(); k++) rozdeleno_[zapis++] = rozdeleno_[k];
      rozdeleno_.resize(zapis);
      otevreneOd = uzavrenych;
      if (rozdeleno_.size() > 8) return false;
    }
    for (const Obd &r : rozdeleno_) ven.push_back(r);
    rozdelenychSkupin++;
    return true;
  }

  // jeden obdelnik (ve ctvercich) jako cast paketu v3: x, y, w, h, kvalita, delka, JPEG
  void obdelnik(const uint32_t *F, const Obd &o, int kv, std::vector<uint8_t> &cil) {
    const int x0 = o.x0 * 16, y0 = o.y0 * 16;
    const int x1 = std::min(W, o.x1 * 16), y1 = std::min(H, o.y1 * 16);
    const int rw = x1 - x0, rh = y1 - y0;
    if (rw <= 0 || rh <= 0) { prazdna(cil); return; }
    if (x0 > 0 || y0 > 0 || x1 < W || y1 < H) castecnych++;
    // B319: presne 2x zvetsena cast (2D hra v 2x Enhanced Resolution) jde v polovicnim rozliseni -
    // barvy v plnem rozliseni (4:4:4) a kvalita +10: stejna vernost jako drive, ~50-60 % dat.
    // Prohlizec ji nakresli 2x bez vyhlazeni = presne zdvojene body jako drive.
    // B320: s toleranci ditheringu (2x2 body se lisi jen o +-1 odstin) - jako bod se vezme jejich prumer
    const int zdv = (polovicni && !(rw & 1) && !(rh & 1)) ? zdvojeny(F + (size_t)y0 * W + x0, (size_t)W, rw, rh, tolerance) : 0;
    const bool pol = zdv != 0;
    int kvJ = kv;
    jpg.clear();
    const double t0 = tedMsPresne();
    if (pol) {
      const int hw = rw / 2, hh = rh / 2;
      polBuf_.resize((size_t)hw * hh);
      for (int y = 0; y < hh; y++) {
        const uint32_t *s = F + (size_t)(y0 + 2 * y) * W + x0;
        uint32_t *d = &polBuf_[(size_t)y * hw];
        if (zdv == 1) { for (int x = 0; x < hw; x++) d[x] = s[2 * x]; }
        else { const uint32_t *s2 = s + W; for (int x = 0; x < hw; x++) d[x] = prumer4(s[2 * x], s[2 * x + 1], s2[2 * x], s2[2 * x + 1]); }
      }
      if (zdv == 2) polovicnichTol++;
      // stejna vernost jako cely obraz (mereno na zrnite mape, test_overeni/b319): 76 -> 86, zjemneni 94 -> 97
      kvJ = kv >= Q_KLID ? std::min(100, kv + 3) : std::min(90, kv + 10);
      casti = (size_t)hw * hh <= 110000 ? 1 : 2;
      enc.koduj(polBuf_.data(), (size_t)hw, hw, hh, kvJ, jpg, casti, true);
      polovicnich++;
    } else {
      const size_t plocha = (size_t)rw * rh;
      // 512x480, 640x480: 2 jadra (ne vic - emulace PS1 jich potrebuje par sama);
      // B313: menu 1280x720 (bez emulace) az maxCasti
      casti = plocha <= 110000 ? 1 : (plocha <= 520000 ? 2 : std::max(2, maxCasti));
      enc.koduj(F + (size_t)y0 * W + x0, (size_t)W, rw, rh, kv, jpg, casti);
    }
    kodSnimku_ += tedMsPresne() - t0;
    p16(cil, (unsigned)x0); p16(cil, (unsigned)y0); p16(cil, (unsigned)rw); p16(cil, (unsigned)rh);
    p8(cil, (unsigned)(pol ? (kvJ | KV_POLOVICNI) : kv));   // kval ctvercu (nize) zustava kv - stejna vernost
    p32(cil, (uint32_t)jpg.size());
    cil.insert(cil.end(), jpg.begin(), jpg.end());
    for (int y = y0; y < y1; y++) std::memcpy(&pred[(size_t)y * W + x0], F + (size_t)y * W + x0, (size_t)rw * 4);
    for (int by = o.y0; by < o.y1; by++)
      for (int bx = o.x0; bx < o.x1; bx++) {
        kval[(size_t)by * bw + bx] = (uint8_t)kv;
        poslano[(size_t)by * bw + bx] = cislo;
      }
    if (tolMa_)                              // B320: prohlizec ted ma presne snimek hry
      for (int by = o.y0; by < o.y1; by++) std::memset(&tol_[(size_t)by * bw + o.x0], 0, (size_t)(o.x1 - o.x0));
    obdelniku++;
  }
};

// B319: volby PS1 pro stranku WEB TV B319 (adresa proudu ?v=319, nap_ps1_native.cpp proudOtevri2):
// posun obrazu, polovicni rozliseni zdvojenych casti a zmeny jako vic obdelniku (po posunu mapy zbyva
// pruh na kraji a jednotky - jeden obdelnik kolem vseho by byl skoro cely obraz).
static constexpr int VOLBA_POSUN = 1, VOLBA_POLOVICNI = 2;
inline void nastavKodekPs1(KodekJpeg &k, int volby) {
  k.posun = (volby & VOLBA_POSUN) != 0;
  k.polovicni = (volby & VOLBA_POLOVICNI) != 0;
  if (k.posun || k.polovicni) {
    k.viceObdelniku = true;
    k.tolerance = KodekJpeg::TOLERANCE_PS1;   // B320: dithering PS1 (stranka WEB TV se nemeni)
  }
}

// B313: kodek pro hlavni menu na TV (nap_menu_gl.cpp i testy - jedno misto): 4 pasy (menu
// nema emulaci), strop 85 (konzole na karte), pri listovani 75, vic obdelniku, zjemneni
// stojicich casti za pohybu, kvalita po zpomaleni site stoupa kazdych 0,6 s.
inline void nastavKodekMenu(KodekJpeg &k) {
  k.maxCasti = 4;
  k.qStrop = 85;
  k.qVelkaZmena = 75;
  k.qZpetMs = 600;
  k.viceObdelniku = true;
  k.jemneBloky = true;
}

// Prectene casti obrazu verze 3 (pro testy na PC; JPEG si rozbali test sam).
// B319: kvalita 0xFF = kopie (jpeg ukazuje na u16 zdroj x, u16 zdroj y), bit7 = JPEG v polovicnim rozliseni.
struct CastJpeg {
  unsigned x = 0, y = 0, w = 0, h = 0, kvalita = 0;
  const uint8_t *jpeg = nullptr;
  size_t delka = 0;
  // vrati pocet prectenych bajtu, 0 = chyba
  size_t cti(const uint8_t *p, size_t n) {
    if (n < 13) return 0;
    x = p[0] | (p[1] << 8); y = p[2] | (p[3] << 8); w = p[4] | (p[5] << 8); h = p[6] | (p[7] << 8);
    kvalita = p[8];
    delka = (size_t)p[9] | ((size_t)p[10] << 8) | ((size_t)p[11] << 16) | ((size_t)p[12] << 24);
    if (13 + delka > n) return 0;
    jpeg = p + 13;
    return 13 + delka;
  }
};

}  // namespace proud
}  // namespace nap
