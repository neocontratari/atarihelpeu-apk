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

// =====================================================================
//  B355: MAPA TLACITEK - co ktere tlacitko ovladace dela, si hrac nastavi
//  u kazdeho emulatoru (Rene: "musi byt v nastaveni u kazdeho emu co ktere
//  tlacitko ma delat podle bt d-padu").
//
//  Radky mapy = tlacitka podle MISTA (A spodni, B prave, X leve, Y horni,
//  L1 R1 L2 R2 L3 R3, START, SELECT, MODE = HOME). Smer (krizek, leva packa)
//  zustava smerem; tlacitka C / Z (ovladace ve stylu Segy) maji pevne
//  chovani. Java (NapOvladacMapa) posila retezec "A=FIRE;B=FIRE;X=K33;..."
//  - co chybi nebo co jadro nezna, zustane vychozi. Katalog (radky, cile,
//  jmena CZ / EN, vychozi mapa) dava Jave taky tenhle soubor - jedno misto.
//
//  Cil MENU = v appce otevrit TLACITKA OVLADACE (resi Java, jadro nic).
//  B355 log Reneho: HOME (a tlacitko snimku Pro Controlleru = MODE) byl v
//  Atari OPTION - v Prince of Persia otevrel menu hry. Ted HOME = MENU.
// =====================================================================
enum : int { J_ATARI = 0, J_SEGA = 1, J_PS1 = 2, J_POCET = 3 };
enum : int { R_A = 0, R_B, R_X, R_Y, R_L1, R_R1, R_L2, R_R2, R_L3, R_R3, R_START, R_SELECT, R_MODE, R_POCET };
static const uint32_t R_BIT[R_POCET] = {T_A, T_B, T_X, T_Y, T_L1, T_R1, T_L2, T_R2, T_L3, T_R3, T_START, T_SELECT, T_MODE};
static const char *const R_TOKEN[R_POCET] = {"A", "B", "X", "Y", "L1", "R1", "L2", "R2", "L3", "R3", "START", "SELECT", "MODE"};
static const char *const R_CZ[R_POCET] = {
    u8"A · spodní (✕ PlayStation, B Nintendo)", u8"B · pravé (○ PlayStation, A Nintendo)",
    u8"X · levé (□ PlayStation, Y Nintendo)", u8"Y · horní (△ PlayStation, X Nintendo)",
    u8"L1 (L)", u8"R1 (R)", u8"L2 (ZL, levá spoušť)", u8"R2 (ZR, pravá spoušť)",
    u8"L3 (stisk levé páčky)", u8"R3 (stisk pravé páčky)", u8"START (+, OPTIONS)", u8"SELECT (−, SHARE)",
    u8"HOME (PS, Xbox)"};
static const char *const R_EN[R_POCET] = {
    u8"A · bottom (✕ PlayStation, B Nintendo)", u8"B · right (○ PlayStation, A Nintendo)",
    u8"X · left (□ PlayStation, Y Nintendo)", u8"Y · top (△ PlayStation, X Nintendo)",
    u8"L1 (L)", u8"R1 (R)", u8"L2 (ZL, left trigger)", u8"R2 (ZR, right trigger)",
    u8"L3 (left stick press)", u8"R3 (right stick press)", u8"START (+, OPTIONS)", u8"SELECT (−, SHARE)",
    u8"HOME (PS, Xbox)"};

static constexpr uint16_t CIL_NIC = 0;
static constexpr uint16_t CIL_MENU = 0x7FFF;
// Atari: 1-9 funkce, 0x100 + KBCODE (scankod 0-63, +$40 SHIFT, +$80 CONTROL) = klavesa drzena jako prstem
enum : uint16_t { A_FIRE = 1, A_NAHORU, A_DOLU, A_VLEVO, A_VPRAVO, A_START, A_SELECT, A_OPTION, A_BREAK, A_KLAVESA = 0x100 };
// Sega: 1 + bit vystupu segaMapuj (0 NAHORU 1 DOLU 2 VLEVO 3 VPRAVO 4 A 5 B 6 C 7 START 8 X 9 Y 10 Z 11 MODE)
enum : uint16_t { S_NAHORU = 1, S_DOLU, S_VLEVO, S_VPRAVO, S_A, S_B, S_C, S_START, S_X, S_Y, S_Z, S_MODE, S_POSLEDNI = S_MODE };
// PS1: 1 + id RetroPadu (viz ps1Mapuj)
enum : uint16_t { P_KRIZEK = 1, P_CTVEREC, P_SELECT, P_START, P_NAHORU, P_DOLU, P_VLEVO, P_VPRAVO, P_KOLECKO, P_TROJUHELNIK,
                  P_L1, P_R1, P_L2, P_R2, P_L3, P_R3, P_POSLEDNI = P_R3 };

struct CilInfo { uint16_t cil; const char *token; const char *cz; const char *en; };
static const CilInfo CILE_ATARI[] = {
    {CIL_NIC, "0", u8"nic", u8"nothing"}, {CIL_MENU, "MENU", u8"MENU – tlačítka ovladače", u8"MENU – controller buttons"},
    {A_FIRE, "FIRE", u8"FIRE", u8"FIRE"},
    {A_NAHORU, "NAHORU", u8"joystick nahoru", u8"joystick up"}, {A_DOLU, "DOLU", u8"joystick dolů", u8"joystick down"},
    {A_VLEVO, "VLEVO", u8"joystick vlevo", u8"joystick left"}, {A_VPRAVO, "VPRAVO", u8"joystick vpravo", u8"joystick right"},
    {A_START, "START", u8"START", u8"START"}, {A_SELECT, "SELECT", u8"SELECT", u8"SELECT"}, {A_OPTION, "OPTION", u8"OPTION", u8"OPTION"},
    {A_KLAVESA + 17, "K17", u8"HELP", u8"HELP"}, {A_BREAK, "BREAK", u8"BREAK", u8"BREAK"},
};
// klavesy Atari pro mapu (scankody jako nap_atari_device.h KEYS)
struct KlavesaInfo { uint8_t scan; const char *cz; const char *en; };
static const KlavesaInfo ATARI_KLAVESY[] = {
    {33, "MEZERA", "SPACE"}, {12, "RETURN", "RETURN"}, {28, "ESC", "ESC"}, {44, "TAB", "TAB"}, {52, "DELETE", "DELETE"},
    {60, "CAPS", "CAPS"}, {39, "INVERSE", "INVERSE"},
    {63, "A", "A"}, {21, "B", "B"}, {18, "C", "C"}, {58, "D", "D"}, {42, "E", "E"}, {56, "F", "F"}, {61, "G", "G"},
    {57, "H", "H"}, {13, "I", "I"}, {1, "J", "J"}, {5, "K", "K"}, {0, "L", "L"}, {37, "M", "M"}, {35, "N", "N"},
    {8, "O", "O"}, {10, "P", "P"}, {47, "Q", "Q"}, {40, "R", "R"}, {62, "S", "S"}, {45, "T", "T"}, {11, "U", "U"},
    {16, "V", "V"}, {46, "W", "W"}, {22, "X", "X"}, {43, "Y", "Y"}, {23, "Z", "Z"},
    {50, "0", "0"}, {31, "1", "1"}, {30, "2", "2"}, {26, "3", "3"}, {24, "4", "4"}, {29, "5", "5"}, {27, "6", "6"},
    {51, "7", "7"}, {53, "8", "8"}, {48, "9", "9"},
    {14, "-", "-"}, {15, "=", "="}, {6, "+", "+"}, {7, "*", "*"}, {2, ";", ";"}, {32, ",", ","}, {34, ".", "."},
    {38, "/", "/"}, {54, "<", "<"}, {55, ">", ">"},
};
static const CilInfo CILE_SEGA[] = {
    {CIL_NIC, "0", u8"nic", u8"nothing"}, {CIL_MENU, "MENU", u8"MENU – tlačítka ovladače", u8"MENU – controller buttons"},
    {S_A, "A", u8"A", u8"A"}, {S_B, "B", u8"B", u8"B"}, {S_C, "C", u8"C", u8"C"}, {S_START, "START", u8"START", u8"START"},
    {S_X, "X", u8"X (ovladač se 6 tlačítky)", u8"X (6-button pad)"}, {S_Y, "Y", u8"Y (ovladač se 6 tlačítky)", u8"Y (6-button pad)"},
    {S_Z, "Z", u8"Z (ovladač se 6 tlačítky)", u8"Z (6-button pad)"}, {S_MODE, "MODE", u8"MODE (ovladač se 6 tlačítky)", u8"MODE (6-button pad)"},
    {S_NAHORU, "NAHORU", u8"směr nahoru", u8"up"}, {S_DOLU, "DOLU", u8"směr dolů", u8"down"},
    {S_VLEVO, "VLEVO", u8"směr vlevo", u8"left"}, {S_VPRAVO, "VPRAVO", u8"směr vpravo", u8"right"},
};
static const CilInfo CILE_PS1[] = {
    {CIL_NIC, "0", u8"nic", u8"nothing"}, {CIL_MENU, "MENU", u8"MENU – tlačítka ovladače", u8"MENU – controller buttons"},
    {P_KRIZEK, "KRIZEK", u8"✕ křížek", u8"✕ cross"}, {P_KOLECKO, "KOLECKO", u8"○ kolečko", u8"○ circle"},
    {P_CTVEREC, "CTVEREC", u8"□ čtverec", u8"□ square"}, {P_TROJUHELNIK, "TROJUHELNIK", u8"△ trojúhelník", u8"△ triangle"},
    {P_L1, "L1", u8"L1", u8"L1"}, {P_R1, "R1", u8"R1", u8"R1"}, {P_L2, "L2", u8"L2", u8"L2"}, {P_R2, "R2", u8"R2", u8"R2"},
    {P_L3, "L3", u8"L3", u8"L3"}, {P_R3, "R3", u8"R3", u8"R3"}, {P_START, "START", u8"START", u8"START"},
    {P_SELECT, "SELECT", u8"SELECT", u8"SELECT"},
    {P_NAHORU, "NAHORU", u8"směr nahoru", u8"up"}, {P_DOLU, "DOLU", u8"směr dolů", u8"down"},
    {P_VLEVO, "VLEVO", u8"směr vlevo", u8"left"}, {P_VPRAVO, "VPRAVO", u8"směr vpravo", u8"right"},
};

struct Mapa {
  int jadro = J_ATARI;
  uint16_t cil[R_POCET] = {0};
};

inline Mapa vychoziMapa(int jadro) {
  // Atari: A B L1 R1 = FIRE, X = MEZERA, Y = RETURN, L2 R2 = OPTION, START, SELECT, HOME = MENU (drive OPTION)
  static const uint16_t ATARI[R_POCET] = {A_FIRE, A_FIRE, A_KLAVESA + 33, A_KLAVESA + 12, A_FIRE, A_FIRE, A_OPTION, A_OPTION,
                                          CIL_NIC, CIL_NIC, A_START, A_SELECT, CIL_MENU};
  // Sega: leve = A, spodni = B, prave = C (A B C v rade zleva), L1 = A, R1 = C, START jako drive;
  // nove ovladac se 6 tlacitky: horni = Y, L2 = X, R2 = Z (bojovky); HOME = MENU
  static const uint16_t SEGA[R_POCET] = {S_B, S_C, S_A, S_Y, S_A, S_C, S_X, S_Z, CIL_NIC, CIL_NIC, S_START, CIL_NIC, CIL_MENU};
  // PS1: stejne misto jako na ovladaci PlayStation; HOME = MENU (drive SELECT - SELECT ma svoje tlacitko)
  static const uint16_t PS1[R_POCET] = {P_KRIZEK, P_KOLECKO, P_CTVEREC, P_TROJUHELNIK, P_L1, P_R1, P_L2, P_R2, P_L3, P_R3,
                                        P_START, P_SELECT, CIL_MENU};
  Mapa m;
  m.jadro = (jadro == J_SEGA || jadro == J_PS1) ? jadro : J_ATARI;
  const uint16_t *v = m.jadro == J_SEGA ? SEGA : m.jadro == J_PS1 ? PS1 : ATARI;
  for (int i = 0; i < R_POCET; i++) m.cil[i] = v[i];
  return m;
}

inline const CilInfo *cileJadra(int jadro, int &n) {
  if (jadro == J_SEGA) { n = (int)(sizeof(CILE_SEGA) / sizeof(CILE_SEGA[0])); return CILE_SEGA; }
  if (jadro == J_PS1) { n = (int)(sizeof(CILE_PS1) / sizeof(CILE_PS1[0])); return CILE_PS1; }
  n = (int)(sizeof(CILE_ATARI) / sizeof(CILE_ATARI[0]));
  return CILE_ATARI;
}
inline const KlavesaInfo *atariKlavesa(int scan) {
  for (const KlavesaInfo &k : ATARI_KLAVESY) if (k.scan == scan) return &k;
  return nullptr;
}

// token cile ("FIRE", "K33", "MENU" ...) -> cil; -1 = jadro token nezna
inline int cilZTokenu(int jadro, const std::string &tok) {
  if (tok.empty()) return -1;
  int n = 0;
  const CilInfo *c = cileJadra(jadro, n);
  for (int i = 0; i < n; i++) if (tok == c[i].token) return c[i].cil;
  if (jadro == J_ATARI && tok.size() >= 2 && tok.size() <= 4 && tok[0] == 'K') {
    int v = 0;
    for (size_t i = 1; i < tok.size(); i++) {
      if (tok[i] < '0' || tok[i] > '9') return -1;
      v = v * 10 + (tok[i] - '0');
    }
    return v <= 255 ? (int)(A_KLAVESA + v) : -1;
  }
  return -1;
}
inline std::string tokenCile(int jadro, uint16_t cil) {
  int n = 0;
  const CilInfo *c = cileJadra(jadro, n);
  for (int i = 0; i < n; i++) if (c[i].cil == cil) return c[i].token;
  if (jadro == J_ATARI && cil >= A_KLAVESA && cil <= A_KLAVESA + 255) return "K" + std::to_string(cil - A_KLAVESA);
  return "0";
}
// jmeno cile pro hrace (UTF-8) nebo do logu (en=false, log=true: bez diakritiky)
inline std::string jmenoCile(int jadro, uint16_t cil, bool en, bool log = false) {
  if (log) {
    if (cil == CIL_NIC) return "nic";
    if (cil == CIL_MENU) return "MENU (tlacitka ovladace)";
    if (jadro == J_ATARI) {
      static const char *L[] = {"nic", "FIRE", "joystick nahoru", "joystick dolu", "joystick vlevo", "joystick vpravo",
                                "START", "SELECT", "OPTION", "BREAK"};
      if (cil <= A_BREAK) return L[cil];
      if (cil == A_KLAVESA + 17) return "HELP";
      if (cil >= A_KLAVESA && cil <= A_KLAVESA + 255) {
        const KlavesaInfo *k = atariKlavesa(cil - A_KLAVESA);
        return std::string("klavesa ") + (k ? k->cz : ("kod " + std::to_string(cil - A_KLAVESA)).c_str());
      }
      return "?";
    }
    return tokenCile(jadro, cil);
  }
  int n = 0;
  const CilInfo *c = cileJadra(jadro, n);
  for (int i = 0; i < n; i++) if (c[i].cil == cil) return en ? c[i].en : c[i].cz;
  if (jadro == J_ATARI && cil >= A_KLAVESA && cil <= A_KLAVESA + 255) {
    const KlavesaInfo *k = atariKlavesa(cil - A_KLAVESA);
    const std::string jm = k ? (en ? k->en : k->cz) : std::to_string(cil - A_KLAVESA);
    return (en ? u8"key " : u8"klávesa ") + jm;
  }
  return "?";
}

// "A=FIRE;B=FIRE;X=K33;..." -> mapa (co chybi nebo je neznamo, zustane vychozi; nezname = pocet spatnych dvojic)
inline Mapa mapaZRetezce(int jadro, const std::string &s, int *nezname = nullptr) {
  Mapa m = vychoziMapa(jadro);
  int spatne = 0;
  size_t i = 0;
  while (i < s.size()) {
    size_t j = i;
    while (j < s.size() && s[j] != ';' && s[j] != ',' && s[j] != ' ' && s[j] != '\n' && s[j] != '\t') j++;
    if (j > i) {
      const std::string dvojice = s.substr(i, j - i);
      const size_t r = dvojice.find('=');
      bool ok = false;
      if (r != std::string::npos && r > 0) {
        const std::string radek = dvojice.substr(0, r), tok = dvojice.substr(r + 1);
        for (int k = 0; k < R_POCET; k++) {
          if (radek != R_TOKEN[k]) continue;
          const int c = cilZTokenu(m.jadro, tok);
          if (c >= 0) { m.cil[k] = (uint16_t)c; ok = true; }
          break;
        }
      }
      if (!ok) spatne++;
    }
    i = j + 1;
  }
  if (nezname) *nezname = spatne;
  return m;
}
inline std::string mapaNaRetezec(const Mapa &m) {
  std::string s;
  for (int i = 0; i < R_POCET; i++) {
    if (i) s += ';';
    s += R_TOKEN[i];
    s += '=';
    s += tokenCile(m.jadro, m.cil[i]);
  }
  return s;
}
// do logu: "A(spodni)=FIRE, X(leve)=klavesa MEZERA, ..."
inline std::string mapaPopis(const Mapa &m) {
  static const char *J[R_POCET] = {"A(spodni)", "B(prave)", "X(leve)", "Y(horni)", "L1", "R1", "L2", "R2", "L3", "R3",
                                   "START", "SELECT", "HOME"};
  std::string s;
  for (int i = 0; i < R_POCET; i++) {
    if (i) s += ", ";
    s += J[i];
    s += '=';
    s += jmenoCile(m.jadro, m.cil[i], false, true);
  }
  return s;
}
inline bool stejnaMapa(const Mapa &a, const Mapa &b) {
  if (a.jadro != b.jadro) return false;
  for (int i = 0; i < R_POCET; i++) if (a.cil[i] != b.cil[i]) return false;
  return true;
}
// katalog pro Javu (dialog TLACITKA OVLADACE): radky "R\ttoken\tjmeno", cile "C\ttoken\tjmeno", "D\tvychozi mapa"
inline std::string katalog(int jadro, bool en) {
  std::string s;
  for (int i = 0; i < R_POCET; i++) { s += "R\t"; s += R_TOKEN[i]; s += '\t'; s += en ? R_EN[i] : R_CZ[i]; s += '\n'; }
  int n = 0;
  const CilInfo *c = cileJadra(jadro, n);
  for (int i = 0; i < n; i++) { s += "C\t"; s += c[i].token; s += '\t'; s += en ? c[i].en : c[i].cz; s += '\n'; }
  if (jadro == J_ATARI) {
    for (const KlavesaInfo &k : ATARI_KLAVESY) {
      s += "C\tK" + std::to_string(k.scan) + '\t' + (en ? u8"key " : u8"klávesa ") + (en ? k.en : k.cz) + '\n';
    }
  }
  s += "D\t" + mapaNaRetezec(vychoziMapa(jadro)) + '\n';
  return s;
}

// ---------------------------------------------------------------------
//  PS1: RetroPad (libretro) - stejna cisla jako dotyk (MainActivity.ps1ButtonId)
//  0 B=KRIZEK 1 Y=CTVEREC 2 SELECT 3 START 4-7 smer 8 A=KOLECKO 9 X=TROJUHELNIK
//  10 L1 11 R1 12 L2 13 R2 14 L3 15 R3. B355: tlacitka podle mapy (C = KRIZEK, Z = KOLECKO pevne)
// ---------------------------------------------------------------------
inline uint32_t ps1Mapuj(uint32_t t, const Mapa &m) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 4;
  if (t & T_DOLU) r |= 1u << 5;
  if (t & T_VLEVO) r |= 1u << 6;
  if (t & T_VPRAVO) r |= 1u << 7;
  if (t & T_C) r |= 1u << 0;
  if (t & T_Z) r |= 1u << 8;
  for (int i = 0; i < R_POCET; i++)
    if ((t & R_BIT[i]) && m.cil[i] >= P_KRIZEK && m.cil[i] <= P_POSLEDNI) r |= 1u << (m.cil[i] - 1);
  return r;
}
inline uint32_t ps1Bity(uint32_t t) {
  static const Mapa V = vychoziMapa(J_PS1);
  return ps1Mapuj(t, V);
}

// ---------------------------------------------------------------------
//  Sega Mega Drive (nap_sega_native_proof.cpp g_input[0..7]):
//  0 NAHORU 1 DOLU 2 VLEVO 3 VPRAVO 4 A 5 B 6 C 7 START; B355: 8 X 9 Y 10 Z 11 MODE
//  (clownmdemu se pta i na ovladac se 6 tlacitky). C = C, Z = Z pevne.
// ---------------------------------------------------------------------
inline uint32_t segaMapuj(uint32_t t, const Mapa &m) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 0;
  if (t & T_DOLU) r |= 1u << 1;
  if (t & T_VLEVO) r |= 1u << 2;
  if (t & T_VPRAVO) r |= 1u << 3;
  if (t & T_C) r |= 1u << 6;
  if (t & T_Z) r |= 1u << 10;
  for (int i = 0; i < R_POCET; i++)
    if ((t & R_BIT[i]) && m.cil[i] >= S_NAHORU && m.cil[i] <= S_POSLEDNI) r |= 1u << (m.cil[i] - 1);
  return r;
}
inline uint32_t segaBity(uint32_t t) {
  static const Mapa V = vychoziMapa(J_SEGA);
  return segaMapuj(t, V);
}

// ---------------------------------------------------------------------
//  Atari 130XE: udalost PAD (nap_atari_native.cpp). bit0 nahoru 1 dolu 2 vlevo 3 vpravo 4 FIRE
//  5 START 6 SELECT 7 OPTION; B355: bity 8-16 klavesa (KBCODE + 1, 0 = zadna - drzi se jako prstem),
//  bit 17 BREAK. Drzi-li se vic tlacitek s klavesou, plati naposled stisknute (posledni = radek R_*).
// ---------------------------------------------------------------------
enum : uint32_t { AP_KLAVESA_POSUN = 8, AP_KLAVESA_MASKA = 0x1FFu << 8, AP_BREAK = 1u << 17 };
inline uint32_t atariMapuj(uint32_t t, const Mapa &m, int posledni = -1) {
  uint32_t r = 0;
  if (t & T_NAHORU) r |= 1u << 0;
  if (t & T_DOLU) r |= 1u << 1;
  if (t & T_VLEVO) r |= 1u << 2;
  if (t & T_VPRAVO) r |= 1u << 3;
  if (t & T_C) r |= 1u << 4;                                  // C = FIRE (pevne)
  int klavesa = -1;
  for (int i = 0; i < R_POCET; i++) {
    if (!(t & R_BIT[i])) continue;
    const uint16_t c = m.cil[i];
    if (c >= A_KLAVESA && c <= A_KLAVESA + 255) {
      if (klavesa < 0 || i == posledni) klavesa = c - A_KLAVESA;
      continue;
    }
    switch (c) {
      case A_FIRE: r |= 1u << 4; break;
      case A_NAHORU: r |= 1u << 0; break;
      case A_DOLU: r |= 1u << 1; break;
      case A_VLEVO: r |= 1u << 2; break;
      case A_VPRAVO: r |= 1u << 3; break;
      case A_START: r |= 1u << 5; break;
      case A_SELECT: r |= 1u << 6; break;
      case A_OPTION: r |= 1u << 7; break;
      case A_BREAK: r |= AP_BREAK; break;
      default: break;                                         // nic, MENU
    }
  }
  if (klavesa >= 0) r |= (uint32_t)(klavesa + 1) << AP_KLAVESA_POSUN;
  return r;
}
inline uint32_t atariBity(uint32_t t) {
  static const Mapa V = vychoziMapa(J_ATARI);
  return atariMapuj(t, V);
}
// radek mapy (R_*) pro jedno logicke tlacitko; -1 = smer / C / Z
inline int radekTlacitka(uint32_t t) {
  for (int i = 0; i < R_POCET; i++) if (t == R_BIT[i]) return i;
  return -1;
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
