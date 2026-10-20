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
// CAST OBRAZU V PAKETU (verze 3):
//   u16 x, u16 y, u16 w, u16 h   obdelnik, ktery se meni (w = 0: beze zmeny)
//   u8  kvalita
//   u32 delka JPEG, pak JPEG (baseline, ctverce zarovnane na 16)
#pragma once
#include <algorithm>
#include <cstdint>
#include <cstdio>
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
  // statistika (do logu)
  long long klicovych = 0, snimku = 0, beze = 0, zjemneni = 0, snizeni = 0, castecnych = 0;
  long long obdelniku = 0, zjemneniZaPohybu = 0, vicecastnych = 0;   // B313
  double kodMsSoucet = 0, kodMsMax = 0;
  long long kodN = 0;

  void zapomen() { ma = false; }
  static int maxKvalita(int w, int h) {
    const size_t n = (size_t)w * h;
    return n <= 110000 ? 88 : n <= 200000 ? 82 : 76;
  }
  std::string info() const {
    char b[360];
    snprintf(b, sizeof(b), "JPEG kvalita %d (max %d, snizeno %lldx), kodovani prumer %.1f ms max %.1f ms (pasu %d), castecnych %lld, zjemneni %lld, beze zmeny %lld z %lld",
             q, qMax, snizeni, kodN ? kodMsSoucet / kodN : 0.0, kodMsMax, casti, castecnych, zjemneni, beze, snimku);
    std::string s = b;
    if (viceObdelniku || jemneBloky) {
      snprintf(b, sizeof(b), ", B313: obdelniku %lld (snimku s vic castmi %lld), zjemneni za pohybu %lld", obdelniku, vicecastnych, zjemneniZaPohybu);
      s += b;
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
      qMax = qStrop > 0 ? qStrop : maxKvalita(W, H);
      if (q <= 0 || q > qMax) q = qMax;
      klicovych++;
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
    zmena.assign((size_t)bw * bh, 0);
    bool neco = false;
    for (int y = 0; y < H; y++) {
      const uint32_t *a = F + (size_t)y * W, *b = &pred[(size_t)y * W];
      if (std::memcmp(a, b, (size_t)W * 4) == 0) continue;
      uint8_t *zr = &zmena[(size_t)(y / 16) * bw];
      for (int bx = 0; bx < bw; bx++) {
        if (zr[bx]) continue;
        const int xa = bx * 16, nx = std::min(16, W - xa);
        if (std::memcmp(a + xa, b + xa, (size_t)nx * 4)) { zr[bx] = 1; neco = true; }
      }
    }
    if (!neco) {
      // cely obraz stoji: po chvilce jeste jednou ve vysoke kvalite (jako drive)
      if (++klid < KLID_SNIMKU) { beze++; prazdna(out); return 4u; }
      bool zjemnit = false;
      for (size_t i = 0; i < kval.size(); i++) if (kval[i] < Q_KLID) { zmena[i] = 1; zjemnit = true; }
      if (!zjemnit) { beze++; prazdna(out); return 4u; }
      zjemneni++;
      obdelnik(F, ohraniceni(zmena), Q_KLID, out);
      zapisStatistiku();
      return 0u;
    }
    klid = 0;
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
    // poradi: nejdriv zjemneni, pak zmeny (oboje z tehoz snimku - prekryv nevadi)
    size_t k = 0;
    for (const Obd &o : obdJem) obdelnik(F, o, Q_KLID, k++ == 0 ? out : dalsiCast(k - 2));
    for (const Obd &o : obdZm) obdelnik(F, o, kvZmen, k++ == 0 ? out : dalsiCast(k - 2));
    dalsiN = k > 0 ? k - 1 : 0;
    if (dalsiN > 0) vicecastnych++;
    zapisStatistiku();
    return 0u;
  }

 private:
  double kodSnimku_ = 0;

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
        while (!zasobnik.empty()) {
          const int i = zasobnik.back();
          zasobnik.pop_back();
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

  // jeden obdelnik (ve ctvercich) jako cast paketu v3: x, y, w, h, kvalita, delka, JPEG
  void obdelnik(const uint32_t *F, const Obd &o, int kv, std::vector<uint8_t> &cil) {
    const int x0 = o.x0 * 16, y0 = o.y0 * 16;
    const int x1 = std::min(W, o.x1 * 16), y1 = std::min(H, o.y1 * 16);
    const int rw = x1 - x0, rh = y1 - y0;
    if (rw <= 0 || rh <= 0) { prazdna(cil); return; }
    if (x0 > 0 || y0 > 0 || x1 < W || y1 < H) castecnych++;
    const size_t plocha = (size_t)rw * rh;
    // 512x480, 640x480: 2 jadra (ne vic - emulace PS1 jich potrebuje par sama);
    // B313: menu 1280x720 (bez emulace) az maxCasti
    casti = plocha <= 110000 ? 1 : (plocha <= 520000 ? 2 : std::max(2, maxCasti));
    jpg.clear();
    const double t0 = tedMsPresne();
    enc.koduj(F + (size_t)y0 * W + x0, (size_t)W, rw, rh, kv, jpg, casti);
    kodSnimku_ += tedMsPresne() - t0;
    p16(cil, (unsigned)x0); p16(cil, (unsigned)y0); p16(cil, (unsigned)rw); p16(cil, (unsigned)rh);
    p8(cil, (unsigned)kv);
    p32(cil, (uint32_t)jpg.size());
    cil.insert(cil.end(), jpg.begin(), jpg.end());
    for (int y = y0; y < y1; y++) std::memcpy(&pred[(size_t)y * W + x0], F + (size_t)y * W + x0, (size_t)rw * 4);
    for (int by = o.y0; by < o.y1; by++)
      for (int bx = o.x0; bx < o.x1; bx++) {
        kval[(size_t)by * bw + bx] = (uint8_t)kv;
        poslano[(size_t)by * bw + bx] = cislo;
      }
    obdelniku++;
  }
};

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
