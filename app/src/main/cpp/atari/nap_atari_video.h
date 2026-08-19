// nap_atari_video.h
// BUILD2SA19: ANTIC - vykresleni PO JEDNOTLIVYCH SCANLINE.
//
// Puvodne jsem to delal jednim pruchodem na konci snimku. To byla zkratka
// a rozbila se na prvni obrazovce, ktera pouziva DLI (preruseni display
// listu) - self-test v ROM meni barvy uprostred obrazu a pri pruchodu na
// konci dostane cela obrazovka barvy posledniho DLI.
// Ted se kresli radek po radku, prolozene s procesorem, jak to dela zeleza.
//
// Co tu JE:  rezimy 2-7 (znakove), 8-15 (bodove), sirka pole podle DMACTL,
//            LMS, JMP, JVB, prazdne radky, inverzni video, hraci a strely.
// Co tu NENI: kolize, skrolovani, rezimy GTIA 9/10/11 pres PRIOR.
#pragma once
#include <cstdint>
#include <cstring>
#include "nap_atari_mem.h"

namespace nap {

// Paleta prevzata ze stavajiciho JavaScriptu. POCITANY ODHAD, ne mereni -
// az se zmeri na skutecnem 130XE, nahradi se tabulkou.
// ---------------------------------------------------------------------
//  PALETA GTIA
//
//  KAZDE CISLO TADY MA ZDROJ. Kdyz u nej zdroj neni, nema tu co delat.
//
//  [1] Datasheet GTIA C014805, list 21 "COLOR DELAY LINE OUTPUT":
//      odstin NENI cislo v tabulce, je to ZPOZDENI na zpozdovaci lince.
//        odstin 1 = 167 ns, krok 21 ns, odstin 15 = 461 ns  (pri V_DEL 7,0 V)
//      Faze = zpozdeni x frekvence barvonosne x 360.
//        21 ns / 279,37 ns (NTSC 3,579545 MHz) x 360 = 27,06 stupne na odstin
//
//  [2] Datasheet GTIA, list 4: jmena vsech 15 odstinu (Gold, Orange,
//      Red-Orange, Pink, Purple, Purple-Blue, Blue, Blue, Light-Blue,
//      Turquoise, Green-Blue, Green, Yellow-Green, Orange-Green,
//      Light-Orange). Prolozeni techto 15 bodu dalo krok 26,29 stupne
//      a pocatecni fazi 164 stupnu, prumerna odchylka 8,8 stupne.
//
//  [3] Zpetne inzenyrstvi (Trevin Beattie, "Notes on Video Output"):
//      "the formula appears to be 190 - 27 x hue for hues from 1-15"
//      -> u odstinu 1 vychazi 163 stupnu, krok 27,00.
//
//  Vsechny tri cesty se schazi na jednom stupni. Bereme [3], protoze je
//  to cele cislo a lezi mezi [1] a [2].
//
//  [4] Datasheet, list 4: bit 0 jasu je NEPOUZITY ("Not Used") - proto
//      se maskuje 0xFE a odstinu zbyva 8 rozlisitelnych jasu, ne 16.
//
//  [5] Rozbor kremiku (Avery Lee, virtualdub.org): "the luminance goes
//      out through a resistor bank" - jas je odporovy zebrik, tedy
//      linearni. Proto Y = lum/14 x Ymax a ne zadna krivka.
//
//  [6] Ymax = 0,93 dopocitano z jedineho SPOLEHLIVE zmereneho seda bodu:
//      $0C (odstin 0, jas 12) = RGB(203,203,203) -> 203/255 / (12/14).
//
//  CO TU NENI A JE TO POCTIVE RICT:
//   - Pin 17 "DEL" (Color delay line adjustment, 3,0 az 8,0 V) je
//     nastaveni na DESCE a kazdy kus Atari ho ma jinde. Jedna univerzalni
//     spravna paleta proto neexistuje. Tohle je stav pri 7,0 V.
//   - Sytost SYTOST=0,30 je jedina hodnota, kterou nemam z mereni.
//     Zpozdovaci linka meni jen FAZI, ne amplitudu, takze je konstantni -
//     ale jeji velikost je zatim odhad a je to tady napsane.
// ---------------------------------------------------------------------
#ifdef NAP_STARA_PALETA
// JEN PRO POROVNANI s puvodnim JS - stara paleta, at se lisi struktura,
// ne barva. V ostrem kodu se nepouziva.
inline uint32_t napAtariPalette(int v) {
  v &= 0xFE;
  const int hue = (v >> 4) & 15, lum = v & 15;
  const double y = 0.06 + 0.88 * (lum / 15.0);
  double r, g, b;
  if (hue == 0) { r = g = b = y; }
  else {
    const double PI = 3.14159265358979323846;
    const double ang = (180.0 - (hue - 1) * 25.7) * PI / 180.0;
    const double u = 0.30 * __builtin_cos(ang), w = 0.30 * __builtin_sin(ang);
    r = y + 1.14 * w; g = y - 0.395 * u - 0.581 * w; b = y + 2.032 * u;
  }
  auto cl = [](double x) { int t = (int)(x * 255.0 + 0.5); return t < 0 ? 0 : (t > 255 ? 255 : t); };
  return 0xFF000000u | ((uint32_t)cl(b) << 16) | ((uint32_t)cl(g) << 8) | (uint32_t)cl(r);
}
#else
inline uint32_t napAtariPalette(int v) {
  v &= 0xFE;                          // [4] bit 0 jasu je nepouzity
  const int hue = (v >> 4) & 15;
  const int lum = v & 15;

  const double Y = (lum / 14.0) * 0.93;          // [5][6] odporovy zebrik
  double r, g, b;
  if (hue == 0) {                                // [2] odstin 0 = Grey
    r = g = b = Y;
  } else {
    const double PI = 3.14159265358979323846;
    const double faze = (190.0 - 27.0 * hue) * PI / 180.0;   // [1][2][3]
    const double SYTOST = 0.30;                  // jedina neoverena hodnota
    const double U = SYTOST * __builtin_cos(faze);
    const double V = SYTOST * __builtin_sin(faze);
    r = Y + 1.140 * V;
    g = Y - 0.395 * U - 0.581 * V;
    b = Y + 2.032 * U;
  }
  auto cl = [](double x) { int t = (int)(x * 255.0 + 0.5); return t < 0 ? 0 : (t > 255 ? 255 : t); };
  return 0xFF000000u | ((uint32_t)cl(b) << 16) | ((uint32_t)cl(g) << 8) | (uint32_t)cl(r);
}
#endif

struct PmgStav {
  const int *hposp, *hposm, *sizep, *grafp;
  int sizem, grafm;
  const int *colpm;
  int prior, gractl, pmbase, dmactl;
};

struct AnticView {
  static const int W = 384;
  static const int H = 240;
  uint32_t fb[W * H];
  uint8_t  tridaPf[W * H];         // 0 = pozadi, 1..4 = PF0..PF3

  struct Popis { int bajtu, scanline, bpp, bodu; bool znakovy; };
  static Popis popisRezimu(int m) {
    switch (m) {
      case 2:  return {40,  8, 1,  8, true};
      case 3:  return {40, 10, 1,  8, true};
      case 4:  return {40,  8, 2,  8, true};
      case 5:  return {40, 16, 2,  8, true};
      case 6:  return {20,  8, 1, 16, true};
      case 7:  return {20, 16, 1, 16, true};
      case 8:  return {10,  8, 2, 32, false};
      case 9:  return {10,  4, 1, 32, false};
      case 10: return {20,  4, 2, 16, false};
      case 11: return {20,  2, 1, 16, false};
      case 12: return {20,  1, 1, 16, false};
      case 13: return {40,  2, 2,  8, false};
      case 14: return {40,  1, 2,  8, false};
      case 15: return {40,  1, 1,  8, false};
      default: return {0, 0, 0, 0, false};
    }
  }
  static int sirkaPole(int dmactl) {
    switch (dmactl & 3) { case 1: return 256; case 2: return 320; case 3: return 384; }
    return 0;
  }
  static int okrajPole(int dmactl) {
    switch (dmactl & 3) { case 1: return 64; case 2: return 32; case 3: return 0; }
    return 0;
  }
  /**
   * Kolik bajtu ANTIC natahne z pameti na jeden radek.
   * POZOR: pri zapnutem VODOROVNEM SKROLOVANI natahuje o JEDEN STUPEN
   * SIRSI radek (uzke->normalni, normalni->siroke). Kdyz se to nepocita,
   * ukazatel na pamet obrazovky se posouva malo a VSECHNO POD TIM se cte
   * ze spatne adresy - u Decathlonu tim zmizela cela tabule s WELCOME.
   */
  static int bajtuNaRadek(int mode, int dmactl, bool hskrol = false) {
    int sirka = dmactl & 3;
    if (!sirka) return 0;
    if (hskrol && sirka < 3) sirka++;          // o stupen sirsi nataceni
    const int b = (sirka == 1) ? 256 : (sirka == 2) ? 320 : 384;
    return popisRezimu(mode).bajtu * b / 320;
  }
  static int radekZnaku(int mode, int rad) {
    if (mode == 5 || mode == 7) return (rad >> 1) & 7;
    return rad & 7;
  }

  void vymaz(int colbk) {
    const uint32_t c = napAtariPalette(colbk);
    for (int i = 0; i < W * H; i++) { fb[i] = c; tridaPf[i] = 0; }
  }

  // ==================================================================
  //  ANTIC -> proud kodu, GTIA -> barvy.
  //
  //  Datasheet GTIA, list 7: ANTIC posila do GTIA po kazdem barvovem
  //  taktu tri bity AN0-AN2. NEPOSILA BARVU. Vyznam:
  //     000 pozadi        100 playfield 0
  //     010 hblank, ZRUS 40znakovy rezim    101 playfield 1
  //     011 hblank, NASTAV 40znakovy rezim  110 playfield 2
  //                                        111 playfield 3
  //  Teprve GTIA na to nasadi barvy z registru, priority, hrace,
  //  strely a kolize.
  //
  //  Mel jsem to slite do jednoho - ANTIC u me rovnou psal RGB. Proto
  //  se sem nedaly poradne zavesit zmeny registru uprostred radku,
  //  priority ani kolize. Tohle je oprava te stavby, ne dalsi zaplata.
  // ==================================================================
  // Datasheet, list 14: "in the high resolution mode (ONE PIXEL PER
  // 1/2 COLOR CLOCK)". Rezimy 2, 3 a 15 maji bod na PUL taktu, takze
  // pole nesmi mit polozku na takt, ale na PUL taktu. S celymi takty
  // vychazelo u rezimu 2 delenim 4/8 = 0 a nenapsalo se NIC - proto
  // v self-testu zmizel spodni radek SELECT, START OR RESET.
  static const int TAKTU = 228;          // barvovych taktu na radek
  static const int PUL = TAKTU * 2;      // polozek v poli (pul taktu)
  static const int PUL0 = 64;            // polozka, ktera je v obraze na x=0
  uint8_t an[PUL];
  bool hires = false;                    // 40znakovy rezim (AN=011)

  void anPozadi() { for (int i = 0; i < PUL; i++) an[i] = 0; hires = false; }

  /** ANTIC: z display listu vyrobi proud kodu pro jeden radek. */
  void anRadek(AtariMem &m, int mode, int screen, int rad, int dmactl,
               int chbase, bool hskrol, int hscrol) {
    anPozadi();
    const Popis p = popisRezimu(mode);
    if (!p.bajtu || screen < 0 || !(dmactl & 3)) return;
    hires = (mode == 2 || mode == 3 || mode == 15);

    const int bajtu = bajtuNaRadek(mode, dmactl, hskrol);
    int sirka2 = dmactl & 3;
    if (hskrol && sirka2 < 3) sirka2++;
    // levy okraj v BARVOVYCH TAKTECH: normalni pole zacina na taktu 48
    const int taktOkraj = (sirka2 == 1) ? 64 : (sirka2 == 2) ? 48 : 32;
    const int taktZac = taktOkraj - (hskrol ? (hscrol & 15) : 0);
    const int pulZac = taktZac * 2;              // vse v PULTAKTECH
    const int pulNaBajt = p.bodu;                // 1 pultakt = 1 bod obrazu

    for (int i = 0; i < bajtu; i++) {
      int bity = 0, vyber = 0; bool inv = false;
      if (p.znakovy) {
        const int zn = m.cpuRead((screen + i) & 0xFFFF);
        int kod;
        if (mode == 6 || mode == 7) { kod = zn & 0x3F; vyber = (zn >> 6) & 3; }
        else { kod = zn & 0x7F; inv = (zn & 0x80) != 0; }
        const int cb = chbase & ((mode == 6 || mode == 7) ? 0xFE : 0xFC);
        if (mode == 3 && rad >= 8) bity = 0;
        else bity = m.cpuRead(((cb << 8) + kod * 8 + radekZnaku(mode, rad)) & 0xFFFF);
        if (inv && (mode == 2 || mode == 3)) bity ^= 0xFF;
      } else {
        bity = m.cpuRead((screen + i) & 0xFFFF);
      }
      const int zaklad = pulZac + i * pulNaBajt;

      if (p.bpp == 1) {
        const int krok = pulNaBajt / 8;
        for (int b = 0; b < 8; b++) {
          const bool on = (bity & (0x80 >> b)) != 0;
          int kod;
          if (mode == 2 || mode == 3 || mode == 15) kod = on ? 5 : 6;   // PF1 / PF2
          else if (p.znakovy)                        kod = on ? (4 + vyber) : 0;
          else                                       kod = on ? 4 : 0;   // PF0 / pozadi
          for (int k = 0; k < krok; k++) {
            const int t = zaklad + b * krok + k;
            if (t >= 0 && t < PUL) an[t] = (uint8_t)kod;
          }
        }
      } else {
        const int krok = pulNaBajt / 4;
        for (int b = 0; b < 4; b++) {
          const int v = (bity >> (6 - b * 2)) & 3;
          const int kod = (v == 0) ? 0 : ((v == 3 && inv) ? 7 : (3 + v));
          for (int k = 0; k < krok; k++) {
            const int t = zaklad + b * krok + k;
            if (t >= 0 && t < PUL) an[t] = (uint8_t)kod;
          }
        }
      }
    }
  }

  /**
   * GTIA: z proudu kodu udela barvy, pridá hrace a strely, vyresi priority.
   * Registr se cte VZDY pro dany barvovy takt - tim je zmena uprostred
   * radky uplne prirozena a nemusi se nikam dolepovat.
   */
  template <class Stroj>
  void gtiaRadek(AtariMem &m, int y, int sl, const Stroj &S) {
    if (y < 0 || y >= H) return;
    uint32_t *dst = fb + y * W;

    const int dmactl = S.dmactl;
    const bool dvouradkove = !(dmactl & 0x10);
    const int base = (S.pmbase & (dvouradkove ? 0xFC : 0xF8)) << 8;
    const bool dmaHraci  = (dmactl & 0x08) && (S.gractl & 0x02);
    const bool dmaStrely = (dmactl & 0x04) && (S.gractl & 0x01);
    const int li = dvouradkove ? (sl >> 1) : sl;

    int gp[4], gm;
    for (int p = 0; p < 4; p++)
      gp[p] = dmaHraci ? m.cpuRead((base + (dvouradkove ? 512 : 1024)
                 + p * (dvouradkove ? 128 : 256) + li) & 0xFFFF)
                       : (S.regNaX(0x0D + p, 0) & 0xFF);
    gm = dmaStrely ? m.cpuRead((base + (dvouradkove ? 384 : 768) + li) & 0xFFFF)
                   : (S.regNaX(0x11, 0) & 0xFF);

    for (int t = PUL0; t < PUL; t++) {
      const int x = t - PUL0;
      if (x < 0 || x >= W) continue;
      const int takt = t / 2;                        // hraci a strely jdou po CELYCH taktech
      const int trida = an[t];                       // 0 pozadi, 4..7 PF0..PF3
      const int prior = S.regNaX(0x1B, x);

      // barva hraciho pole
      uint32_t c;
      if (trida == 0) c = napAtariPalette(S.regNaX(0x1A, x));
      else if (hires && trida == 5) {
        // 40znakovy rezim: PF1 dava JAS, odstin z PF2 (datasheet, list 7)
        const int pf1 = S.regNaX(0x17, x), pf2 = S.regNaX(0x18, x);
        c = napAtariPalette((pf2 & 0xF0) | (pf1 & 0x0E));
      } else c = napAtariPalette(S.regNaX(0x16 + (trida - 4), x));

      // hraci a strely pres to, podle priority
      for (int p = 3; p >= 0; p--) {
        const int hpos = S.regNaX(0x00 + p, x);
        const int sz = S.regNaX(0x08 + p, x) & 3;
        const int nas = (sz == 1) ? 2 : (sz == 3) ? 4 : 1;
        const int d = takt - hpos;
        if (d < 0 || d >= 8 * nas) continue;
        if (!(gp[p] & (0x80 >> (d / nas)))) continue;
        if (prebiji(trida, p, prior)) continue;
        c = napAtariPalette(S.regNaX(0x12 + p, x));
      }
      for (int p = 3; p >= 0; p--) {
        const int bity = (gm >> (p * 2)) & 3;
        if (!bity) continue;
        const int hpos = S.regNaX(0x04 + p, x);
        const int sz = (S.regNaX(0x0C, x) >> (p * 2)) & 3;
        const int nas = (sz == 1) ? 2 : (sz == 3) ? 4 : 1;
        const int d = takt - hpos;
        if (d < 0 || d >= 2 * nas) continue;
        if (!(bity & (0x02 >> (d / nas)))) continue;
        if (prebiji(trida, p, prior)) continue;
        c = napAtariPalette((prior & 0x10) ? S.regNaX(0x19, x)   // paty hrac = PF3
                                           : S.regNaX(0x12 + p, x));
      }
      dst[x] = c;
    }
  }

  static bool prebiji(int trida, int hrac, int prior) {
    if (trida == 0) return false;              // pozadi nikdy neprebiji
    if (prior & 0x01) return false;            // hraci nad vsim
    if (prior & 0x02) return hrac >= 2;        // P0,P1 nad polem
    if (prior & 0x04) return true;             // pole nad hraci
    if (prior & 0x08) return (trida <= 2);     // PF0,PF1 nad hraci
    return false;
  }
};

} // namespace nap
