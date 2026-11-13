// nap_ovladac.h
// =====================================================================
//  B320: HERNI OVLADAC - Retroid Pocket 6 (vestaveny), Bluetooth / USB
//  gamepad (Xbox, PlayStation, 8BitDo ...). Spolecne pro PS1, Segu
//  a Atari 130XE - kazde jadro si tenhle soubor prelozi samo.
//
//  Rene: "proste to riskni ... je to pro nas vyzva Retroid pocket naslepo".
//  Uzivatel z fora: "jestli to pujde na Retro Pocket 6 s tim, ze by se na
//  obrazovce po pripojeni stahnul nas d-pad a najelo to na origo tlacitka".
//
//  Java (NapOvladac) jen zjisti, ze udalost prisla z ovladace, a preda
//  sem kod klavesy Androidu (KeyEvent.KEYCODE_*) a osy (packy, krizek,
//  spouste). CO tlacitko udela, rozhoduje tenhle soubor:
//
//    1) klavesa Androidu -> LOGICKE tlacitko (podle MISTA na ovladaci,
//       ne podle napisu): A = spodni, B = prave, X = leve, Y = horni
//       (Android tak hlasi Xbox rozlozeni - Retroid v rezimu Xbox taky),
//    2) osy -> smer (krizek HAT, leva packa s hysterezi), spouste L2 / R2
//       (analogove - Retroid 6 ma spouste s Hallovym cidlem) s hysterezi,
//       packy pro analogove pouziti (mys PlayStation v Dune 2000),
//    3) logicka tlacitka -> konkretni konzole:
//       PS1    RetroPad (libretro): spodni = KRIZEK, prave = KOLECKO,
//              leve = CTVEREC, horni = TROJUHELNIK - stejne misto jako na
//              ovladaci PlayStation; L1 R1 L2 R2 L3 R3, START, SELECT
//       Sega   leve = A, spodni = B, prave = C (jako Genesis Plus GX
//              v RetroArchu - A B C v rade zleva), START
//       Atari  spodni / prave / L1 / R1 = FIRE, leve = MEZERA, horni =
//              RETURN, START / SELECT, L2 / R2 / MODE = OPTION (B296)
//
//  Vsechno cisty C++ bez Androidu - testuje se na PC (test_overeni/b320).
//  Cisla klaves a os jsou pevna cisla API Androidu (android/keycodes.h,
//  android/input.h) - opsana sem, at jde soubor prelozit i na PC.
// =====================================================================
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

namespace nap {
namespace ovladac {

// ---------------------------------------------------------------------
//  kody klaves Androidu (AKEYCODE_* = KeyEvent.KEYCODE_*)
// ---------------------------------------------------------------------
enum : int {
  K_DPAD_UP = 19, K_DPAD_DOWN = 20, K_DPAD_LEFT = 21, K_DPAD_RIGHT = 22, K_DPAD_CENTER = 23,
  K_BUTTON_A = 96, K_BUTTON_B = 97, K_BUTTON_C = 98, K_BUTTON_X = 99, K_BUTTON_Y = 100, K_BUTTON_Z = 101,
  K_BUTTON_L1 = 102, K_BUTTON_R1 = 103, K_BUTTON_L2 = 104, K_BUTTON_R2 = 105,
  K_BUTTON_THUMBL = 106, K_BUTTON_THUMBR = 107,
  K_BUTTON_START = 108, K_BUTTON_SELECT = 109, K_BUTTON_MODE = 110,
  K_BUTTON_1 = 188, K_BUTTON_16 = 203,
  K_DPAD_UP_LEFT = 268, K_DPAD_DOWN_LEFT = 269, K_DPAD_UP_RIGHT = 270, K_DPAD_DOWN_RIGHT = 271,
};

// ---------------------------------------------------------------------
//  logicka tlacitka (bity) - podle mista na ovladaci
// ---------------------------------------------------------------------
enum : uint32_t {
  T_A = 1u << 0,        // spodni
  T_B = 1u << 1,        // prave
  T_X = 1u << 2,        // leve
  T_Y = 1u << 3,        // horni
  T_L1 = 1u << 4, T_R1 = 1u << 5, T_L2 = 1u << 6, T_R2 = 1u << 7,
  T_L3 = 1u << 8, T_R3 = 1u << 9,
  T_START = 1u << 10, T_SELECT = 1u << 11, T_MODE = 1u << 12,
  T_NAHORU = 1u << 13, T_DOLU = 1u << 14, T_VLEVO = 1u << 15, T_VPRAVO = 1u << 16,
  T_C = 1u << 17, T_Z = 1u << 18,       // tlacitka C / Z (nektere ovladace ve stylu Segy)
  T_SMER = T_NAHORU | T_DOLU | T_VLEVO | T_VPRAVO,
};

// klavesa Androidu -> logicka tlacitka (0 = neni klavesa ovladace)
inline uint32_t tlacitkoZKlavesy(int k) {
  switch (k) {
    case K_BUTTON_A: return T_A;
    case K_DPAD_CENTER: return T_A;         // nektere ovladace (rezim "Android TV") hlasi A takhle
    case K_BUTTON_B: return T_B;
    case K_BUTTON_X: return T_X;
    case K_BUTTON_Y: return T_Y;
    case K_BUTTON_C: return T_C;
    case K_BUTTON_Z: return T_Z;
    case K_BUTTON_L1: return T_L1;
    case K_BUTTON_R1: return T_R1;
    case K_BUTTON_L2: return T_L2;
    case K_BUTTON_R2: return T_R2;
    case K_BUTTON_THUMBL: return T_L3;
    case K_BUTTON_THUMBR: return T_R3;
    case K_BUTTON_START: return T_START;
    case K_BUTTON_SELECT: return T_SELECT;
    case K_BUTTON_MODE: return T_MODE;
    case K_DPAD_UP: return T_NAHORU;
    case K_DPAD_DOWN: return T_DOLU;
    case K_DPAD_LEFT: return T_VLEVO;
    case K_DPAD_RIGHT: return T_VPRAVO;
    case K_DPAD_UP_LEFT: return T_NAHORU | T_VLEVO;      // nektere ovladace posilaji uhlopricky zvlast
    case K_DPAD_DOWN_LEFT: return T_DOLU | T_VLEVO;
    case K_DPAD_UP_RIGHT: return T_NAHORU | T_VPRAVO;
    case K_DPAD_DOWN_RIGHT: return T_DOLU | T_VPRAVO;
    default: return 0;
  }
}

// ---------------------------------------------------------------------
//  osy: Java posle 8 cisel v tomhle poradi (uz srovnanych podle ovladace -
//  ktera osa je prava packa a ktera spoust, pozna Java z rozsahu os)
// ---------------------------------------------------------------------
enum { O_LX = 0, O_LY, O_RX, O_RY, O_HX, O_HY, O_L2, O_R2, O_POCET };
// maska: co ovladac ma (bez toho se osa nepouzije - nektere ovladace posilaji nuly)
enum { M_HAT = 1, M_PRAVA = 2, M_SPOUSTE = 4, M_LEVA = 8 };

static constexpr float MRTVA_ZONA = 0.20f;   // packa: kolem stredu nic (zadne driftovani)
static constexpr float SMER_ZAP = 0.50f;     // packa jako krizek: od poloviny vychylky smer zapnout
static constexpr float SMER_VYP = 0.35f;     // ... a pod 35 % vypnout (hystereze - zadne cukani)
static constexpr float SPOUST_ZAP = 0.50f;   // analogova spoust jako tlacitko
static constexpr float SPOUST_VYP = 0.35f;
static constexpr float HAT_PRAH = 0.50f;     // krizek jako osa HAT (-1 / 0 / 1)

// packa po mrtve zone: delka 0 az 1 (od okraje zony plynule), smer zachovany
inline void mrtvaZona(float &x, float &y, float zona = MRTVA_ZONA) {
  if (!(x == x)) x = 0.f;                    // NaN
  if (!(y == y)) y = 0.f;
  const float d = std::sqrt(x * x + y * y);
  if (d <= zona) { x = y = 0.f; return; }
  const float m = std::fmin(1.f, (d - zona) / (1.f - zona));
  x = x / d * m;
  y = y / d * m;
}

struct Stav {
  uint32_t klavesy = 0;        // drzena tlacitka z udalosti klaves
  uint32_t hat = 0;            // smer z krizku (osy HAT)
  uint32_t packa = 0;          // smer z leve packy (s hysterezi)
  uint32_t spouste = 0;        // L2 / R2 z analogovych spousti (s hysterezi)
  float lx = 0, ly = 0, rx = 0, ry = 0;      // packy po mrtve zone (-1 .. 1)
  float l2 = 0, r2 = 0;                      // spouste 0 .. 1
  long long klaves = 0, osUdalosti = 0;      // pocitadla (log)
  uint32_t vse(bool sPackou = true) const { return klavesy | hat | spouste | (sPackou ? packa : 0u); }
};

// udalost klavesy. Vraci logicka tlacitka (0 = neni klavesa ovladace - Java ji pusti dal)
inline uint32_t klavesa(Stav &s, int keyCode, bool dolu) {
  const uint32_t b = tlacitkoZKlavesy(keyCode);
  if (!b) return 0;
  if (dolu) s.klavesy |= b; else s.klavesy &= ~b;
  s.klaves++;
  return b;
}

// smer z jedne osy s hysterezi (minus / plus = bity)
inline uint32_t smerOsy(float v, uint32_t drive, uint32_t minus, uint32_t plus) {
  uint32_t r = 0;
  if (drive & minus) { if (v < -SMER_VYP) r |= minus; } else if (v < -SMER_ZAP) r |= minus;
  if (drive & plus) { if (v > SMER_VYP) r |= plus; } else if (v > SMER_ZAP) r |= plus;
  if ((r & minus) && (r & plus)) r &= ~(minus | plus);    // nemuze nastat, pojistka
  return r;
}

// udalost os (Java: MotionEvent ze zdroje JOYSTICK). o[8] viz O_*, maska M_*
inline void osy(Stav &s, const float o[O_POCET], int maska) {
  s.osUdalosti++;
  // krizek jako HAT: -1 / 0 / 1
  uint32_t h = 0;
  if (maska & M_HAT) {
    if (o[O_HX] < -HAT_PRAH) h |= T_VLEVO; else if (o[O_HX] > HAT_PRAH) h |= T_VPRAVO;
    if (o[O_HY] < -HAT_PRAH) h |= T_NAHORU; else if (o[O_HY] > HAT_PRAH) h |= T_DOLU;
  }
  s.hat = h;
  // leva packa: smer s hysterezi (kazda osa zvlast - uhlopricky jdou samy)
  float lx = (maska & M_LEVA) ? o[O_LX] : 0.f, ly = (maska & M_LEVA) ? o[O_LY] : 0.f;
  if (!(lx == lx)) lx = 0.f;
  if (!(ly == ly)) ly = 0.f;
  s.packa = smerOsy(lx, s.packa, T_VLEVO, T_VPRAVO) | smerOsy(ly, s.packa, T_NAHORU, T_DOLU);
  mrtvaZona(lx, ly);
  s.lx = lx; s.ly = ly;
  float rx = (maska & M_PRAVA) ? o[O_RX] : 0.f, ry = (maska & M_PRAVA) ? o[O_RY] : 0.f;
  mrtvaZona(rx, ry);
  s.rx = rx; s.ry = ry;
  // spouste
  uint32_t sp = 0;
  float l2 = (maska & M_SPOUSTE) ? o[O_L2] : 0.f, r2 = (maska & M_SPOUSTE) ? o[O_R2] : 0.f;
  if (!(l2 == l2)) l2 = 0.f;
  if (!(r2 == r2)) r2 = 0.f;
  s.l2 = std::fmax(0.f, std::fmin(1.f, l2));
  s.r2 = std::fmax(0.f, std::fmin(1.f, r2));
  if ((s.spouste & T_L2) ? s.l2 > SPOUST_VYP : s.l2 >= SPOUST_ZAP) sp |= T_L2;
  if ((s.spouste & T_R2) ? s.r2 > SPOUST_VYP : s.r2 >= SPOUST_ZAP) sp |= T_R2;
  s.spouste = sp;
}

// vse pustit (ovladac odpojen, odchod ze hry, appka na pozadi)
inline void pustVse(Stav &s) {
  s.klavesy = s.hat = s.packa = s.spouste = 0;
  s.lx = s.ly = s.rx = s.ry = s.l2 = s.r2 = 0.f;
}

// ---------------------------------------------------------------------
//  PS1: RetroPad (libretro) - stejna cisla jako dotyk (MainActivity.ps1ButtonId)
//  0 B=KRIZEK 1 Y=CTVEREC 2 SELECT 3 START 4-7 smer 8 A=KOLECKO 9 X=TROJUHELNIK
//  10 L1 11 R1 12 L2 13 R2 14 L3 15 R3
// ---------------------------------------------------------------------
inline uint32_t ps1Bity(uint32_t t) {
  uint32_t r = 0;
  if (t & (T_A | T_C)) r |= 1u << 0;        // spodni = KRIZEK
  if (t & T_X) r |= 1u << 1;                // leve = CTVEREC
  if (t & (T_SELECT | T_MODE)) r |= 1u << 2;
  if (t & T_START) r |= 1u << 3;
  if (t & T_NAHORU) r |= 1u << 4;
  if (t & T_DOLU) r |= 1u << 5;
  if (t & T_VLEVO) r |= 1u << 6;
  if (t & T_VPRAVO) r |= 1u << 7;
  if (t & (T_B | T_Z)) r |= 1u << 8;        // prave = KOLECKO
  if (t & T_Y) r |= 1u << 9;                // horni = TROJUHELNIK
  if (t & T_L1) r |= 1u << 10;
  if (t & T_R1) r |= 1u << 11;
  if (t & T_L2) r |= 1u << 12;
  if (t & T_R2) r |= 1u << 13;
  if (t & T_L3) r |= 1u << 14;
  if (t & T_R3) r |= 1u << 15;
  return r;
}

// ---------------------------------------------------------------------
//  Sega Mega Drive (nap_sega_native_proof.cpp g_input[0..7]):
//  0 NAHORU 1 DOLU 2 VLEVO 3 VPRAVO 4 A 5 B 6 C 7 START
// ---------------------------------------------------------------------
inline uint32_t segaBity(uint32_t t) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 0;
  if (t & T_DOLU) r |= 1u << 1;
  if (t & T_VLEVO) r |= 1u << 2;
  if (t & T_VPRAVO) r |= 1u << 3;
  if (t & (T_X | T_L1)) r |= 1u << 4;      // leve (a L1) = A
  if (t & T_A) r |= 1u << 5;               // spodni = B
  if (t & (T_B | T_C | T_R1)) r |= 1u << 6;   // prave (a R1, tlacitko C) = C
  if (t & T_START) r |= 1u << 7;
  return r;
}

// ---------------------------------------------------------------------
//  Atari 130XE (NativeAtariCoreBridge.devPad, B296): bit0 nahoru 1 dolu 2 vlevo
//  3 vpravo 4 FIRE 5 START 6 SELECT 7 OPTION 8 klavesa MEZERA 9 klavesa RETURN
// ---------------------------------------------------------------------
inline uint32_t atariBity(uint32_t t) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 0;
  if (t & T_DOLU) r |= 1u << 1;
  if (t & T_VLEVO) r |= 1u << 2;
  if (t & T_VPRAVO) r |= 1u << 3;
  if (t & (T_A | T_B | T_L1 | T_R1 | T_C)) r |= 1u << 4;     // FIRE
  if (t & T_START) r |= 1u << 5;
  if (t & T_SELECT) r |= 1u << 6;
  if (t & (T_MODE | T_L2 | T_R2)) r |= 1u << 7;              // OPTION
  if (t & T_X) r |= 1u << 8;                                 // MEZERA (Wolfenstein: dvere)
  if (t & T_Y) r |= 1u << 9;                                 // RETURN
  return r;
}

// jmena logickych tlacitek (log)
inline std::string jmena(uint32_t t) {
  static const char *J[19] = {"A(spodni)", "B(prave)", "X(leve)", "Y(horni)", "L1", "R1", "L2", "R2", "L3", "R3",
                              "START", "SELECT", "MODE", "nahoru", "dolu", "vlevo", "vpravo", "C", "Z"};
  std::string s;
  for (int i = 0; i < 19; i++)
    if (t & (1u << i)) { if (!s.empty()) s += '+'; s += J[i]; }
  return s.empty() ? std::string("nic") : s;
}

// ---------------------------------------------------------------------
//  ANALOGOVA PACKA JAKO MYS (PS1 - mys PlayStation, Dune 2000): rychlost
//  kurzoru v bodech mysi za snimek. Plna vychylka = PACKA_MAX bodu
//  (60 snimku/s: ~1 sirka obrazu za sekundu pri citlivosti 100 %),
//  krivka na druhou - kousek od stredu jemne a presne. Citlivost z panelu
//  MYS PLAYSTATION (pro dotyk) se tu bere jen odmocninou (295 % -> 1,7x),
//  aby packa nelitala.
// ---------------------------------------------------------------------
static constexpr float PACKA_MAX = 6.f;
inline float packaRychlost(float m, int citlivost) {
  if (!(m > 0.f)) return 0.f;
  if (m > 1.f) m = 1.f;
  float c = citlivost / 100.f;
  if (c < 0.25f) c = 0.25f;
  if (c > 4.f) c = 4.f;
  return PACKA_MAX * m * m * std::sqrt(c);
}

}  // namespace ovladac
}  // namespace nap
