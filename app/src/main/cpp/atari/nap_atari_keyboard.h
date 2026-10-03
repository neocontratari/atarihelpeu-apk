// nap_atari_keyboard.h
// BUILD2SC1+SC2: klavesnice + konzolova tlacitka atari 130XE - CISTE V C++.
//
// Rene: "preved do apky sekce help atari c++ a dej si pozor at mame emu
// atari 130xe v HELP ciste v c++." Tenhle soubor NEKRESLI pres HTML/CSS -
// vyrabi primo pixely (RGB framebuffer), presne jako uz dela obrazovka
// samotneho Atari (AnticView). WebView ho jen zobrazi, nic si nedokresluje.
//
// BUILD2SC2 - DULEZITA OPRAVA: prvni verze (B289) vykreslovala popisky
// hranatym 8x8 ROM fontem a klavesy/tlacitka jako ploche obdelniky bez
// gradientu/stinu/zaobleni. Rene: "vubec neodpovida tomu co jsme tady
// celou tu dobu delali." Mel pravdu - zkontroloval jsem to prime vedle
// schvaleneho navrhu a byl v tom skutecny rozdil, ne jen kosmeticky:
//  - spatne pismo (blokovy retro font misto Chakra Petch z navrhu)
//  - zadny gradient/stin/zaobleni rohu na klavesach a tlacitkach
//  - popisky zkracene na vlastni pest (CTRL/RET/BRK...) misto skutecnych
//    textu z navrhu (Control/Return/Break...)
//  - chybel druhy (maly, posunuty) radek popisku - napr. '!' nad '1'
//  - pozadi ploche misto kovoveho gradientu
// Tahle verze to opravuje poctive, ne kosmeticky:
//  - skutecny font Chakra Petch (presne jako navrh pouziva z Google Fonts)
//    vlozeny primo jako bajtove pole (nap_atari_fonts.h) a vykreslovany
//    pres stb_truetype.h (vendor/stb/) - porad 100% C++, zadne Android/
//    Java fontovaci API.
//  - gradientove vyplne + zaoblene rohy + vnitrni/vnejsi stin - rucne
//    napocitane presne z CSS hodnot schvaleneho navrhu (viz komentare
//    u jednotlivych barev nize - kazda cituje svuj puvod).
//  - plne popisky (dva radky: maly l1 nahore, velky l2 dole) - 1:1
//    prevzate z rowLabels v navrhu (Main.dc.html), zadne vlastni zkratky.
//
// PUVOD DAT (zadne hadani - vse primo zmereno/odzkouseno):
//  - Pixelova geometrie (x,y,sirka,vyska) a popisky l1/l2 jsou 1:1
//    prevzate ze schvaleneho claude.ai navrhu (Main.dc.html, rowX/rowY/
//    rowW/rowH/rowLabels + consoleDefs + cstripeDefs) - presne ta cisla
//    a texty, ktere Rene uz videl a schvalil na navrhu 941x1672.
//  - Scankody jsou prevzate ze stareho, uz let overeneho JS jadra
//    (assets/emu_vbxe/index.html, tabulka ROWS) - rucne porovnano radek
//    po radku: oba zdroje maji STEJNY pocet klaves ve STEJNEM poradi v
//    kazde rade (15+14+14+13+1=57), takze se spoluji 1:1 bez domnenek.
//  - Barvy/gradienty/stiny jsou prepsane z CSS schvaleneho navrhu
//    (style.css - .key, .cbtn, .devicebox, .wordmark, .fuji, .cstripe) -
//    kazda barva v kodu nize cituje svoje puvodni CSS pravidlo.
//  - Font: Chakra Petch (SemiBold 600, Bold 700, BoldItalic 700) - presne
//    ty rezy, ktere navrh nacita pres Google Fonts CSS2 API. Licence SIL
//    OFL 1.1 (vendor/chakra_petch/OFL.txt) - smi se vkladat do aplikace.
//
// CO TO JESTE NENI (dalsi kolo, Rene uz byl informovan): kazetak, servisni
// tlacitka, zaoblene vnejsi rohy cele skrinky, skutecne rozostreni stinu
// (tady jen jednoducha aproximace - plna poloprusvitna ciara/pas misto
// gaussovského blur, ktery CSS umi a rucni rasterizer v rozumnem case ne).
#pragma once
#include <cstdint>
#include <cstring>
#include "../vendor/stb/stb_truetype.h"
#include "nap_atari_fonts.h"

namespace nap {

// ---------------------------------------------------------------
//  TABULKA 57 KLAVES - zdroj viz komentar nahore.
//  scan: realny Atari scankod; -1=CTRL(zamek-prepinac), -2=SHIFT
//        (tuknuti=zamek, drzeni>320ms=docasne), -3=BREAK (jednorazove).
//  l1: maly popisek nahore (shift-znak/znacka) - '' kdyz zadny.
//  l2: velky popisek dole (hlavni znak/slovo).
//  POZOR (BUILD2SC2): l1/l2 jsou 1:1 opsane z rowLabels v navrhu, VCETNE
//  zdanlive "divne" dvojice na byvalych ','/'.'/'/ ' klavesach (radek3,
//  id51/52/53) - navrh tam ma l2='/' na DVOU ruznych klavesach (id51 i
//  id53, s jinym l1: '[' resp. '?'). Nejde o preklep pri prepisu - presne
//  tak to ma schvaleny navrh (Main.dc.html radek 414), overeno primo v
//  souboru. Pokud je to chyba PUVODNIHO navrhu, je to tema na samostatnou
//  otazku Renemu - tady se jen VERNE prevadi to, co uz schvalil.
// ---------------------------------------------------------------
struct KbdKeyDef { float x, y, w, h; int16_t scan; const char *l1; const char *l2; };

static const KbdKeyDef NAP_KBD_KEYS[57] = {
  // rada 0 (y=901,h=64)
  {41,901,58,64,28,   "","Esc"},
  {108,901,51,64,31,  "!","1"},
  {168,901,50,64,30,  "\"","2"},
  {227,901,48,64,26,  "#","3"},
  {284,901,48,64,24,  "$","4"},
  {340,901,47,64,29,  "%","5"},
  {395,901,48,64,27,  "&","6"},
  {452,901,45,64,51,  "'","7"},
  {505,901,46,64,53,  "@","8"},
  {560,901,45,64,48,  "(","9"},
  {614,901,46,64,50,  ")","0"},
  {667,901,48,64,54,  "Clear","<"},
  {723,901,48,64,55,  "Insert",">"},
  {779,901,50,64,52,  "Delete","BkSp"},
  {837,901,58,64,-3,  "","Break"},
  // rada 1 (y=966,h=67)
  {41,966,81,67,44,   "Clr Set","Tab"},
  {130,966,54,67,47,  "","Q"},
  {190,966,52,67,46,  "","W"},
  {249,966,53,67,42,  "","E"},
  {308,966,52,67,40,  "","R"},
  {367,966,50,67,45,  "","T"},
  {423,966,49,67,43,  "","Y"},
  {478,966,50,67,11,  "","U"},
  {533,966,47,67,13,  "","I"},
  {587,966,49,67,8,   "","O"},
  {641,966,49,67,10,  "","P"},
  {696,966,49,67,14,  "\xE2\x86\x91","-"},   // UTF-8 '↑'
  {751,966,51,67,15,  "\xE2\x86\x93","="},   // UTF-8 '↓'
  {808,966,87,67,12,  "","Return"},
  // rada 2 (y=1034,h=65)
  {41,1034,90,65,-1,  "","Control"},
  {141,1034,52,65,63, "","A"},
  {202,1034,50,65,62, "","S"},
  {261,1034,50,65,58, "","D"},
  {320,1034,50,65,56, "","F"},
  {378,1034,49,65,61, "","G"},
  {437,1034,46,65,57, "","H"},
  {492,1034,47,65,1,  "","J"},
  {548,1034,45,65,5,  "","K"},
  {601,1034,47,65,0,  "","L"},
  {656,1034,45,65,2,  ":",";"},
  {710,1034,49,65,6,  "\xE2\x86\x90","+"},   // UTF-8 '←'
  {766,1034,52,65,7,  "\xE2\x86\x92","*"},   // UTF-8 '→'
  {825,1034,70,65,60, "","Caps"},
  // rada 3 (y=1101,h=68)
  {41,1101,121,68,-2,  "","Shift"},
  {171,1101,51,68,23,  "","Z"},
  {231,1101,48,68,22,  "","X"},
  {289,1101,49,68,18,  "","C"},
  {345,1101,48,68,16,  "","V"},
  {403,1101,46,68,21,  "","B"},
  {459,1101,46,68,35,  "","N"},
  {514,1101,47,68,37,  "","M"},
  {570,1101,47,68,32,  "[","/"},
  {626,1101,46,68,34,  "]","."},
  {680,1101,45,68,38,  "?","/"},
  {734,1101,91,68,-2,  "","Shift"},
  {833,1101,62,68,39,  "Fuji","Inverse"},
  // mezernik
  {216,1175,484,52,33, "",""},
};

// ---------------------------------------------------------------
//  KONZOLOVY PAS (presne z consoleDefs v navrhu, y=740,h=73).
//  action: 0=HELP (klavesnicova matice, scan 17, jednorazove),
//          1/2/3=START/SELECT/OPTION (konzolovy spinac, drzi se),
//          4=RESET (jednorazove).
// ---------------------------------------------------------------
struct KbdConsoleDef { float x, y, w, h; const char *lab; int8_t action; };

static const KbdConsoleDef NAP_KBD_CONSOLE[5] = {
  {316.9f,740,179.2f,73,"HELP",   0},
  {436.4f,740,163.2f,73,"START",  1},
  {539.9f,740,165.2f,73,"SELECT", 2},
  {645.4f,740,165.7f,73,"OPTION", 3},
  {751.4f,740,163.2f,73,"RESET",  4},
};

// ---------------------------------------------------------------
//  Vysledek doteku - co se ma stat se SKUTECNYM strojem. Tahle
//  struktura ani hitTest/touchDown/touchUp SE STROJE NESAHAJI (zadne
//  g_stroj tady) - jen rozhodnou CO by se melo stat. Skutecne volani
//  klavesa()/consol/reset() dela az JNI (nap_atari_native.cpp), ktery
//  uz ma pristup ke g_stroj. Cisty oddeleny modul se tak da zkompilovat
//  a otestovat i SAMOSTATNE, bez cele Atari emulace.
//  BUILD2SC2: tahle cast se VUBEC NEMENI - je uz overena (test_b2sc1),
//  oprava vzhledu se ji netyka.
// ---------------------------------------------------------------
struct KbdEvent {
  enum Typ { NIC, KLAVESA, KONZOLE_MASK, RESET, BREAK } typ = NIC;
  int scan = 0;        // pro KLAVESA (uz vcetne SHIFT/CTRL bitu)
  int konzoleMask = 7; // pro KONZOLE_MASK - CELY vysledny 3bitovy stav
};

struct KRGB { uint8_t r, g, b; };
static inline KRGB krgb(int r, int g, int b) { return KRGB{ (uint8_t)r, (uint8_t)g, (uint8_t)b }; }

// ---------------------------------------------------------------
//  Deck - vlastni jen VIZUALNI/UI stav (co je prave zmacknute, jestli
//  je SHIFT zamceny) - realny stav stroje (klavesnice, konzole) zije
//  jen v Machine, nikdy se tu neduplikuje.
// ---------------------------------------------------------------
struct KbdDeck {
  // Zdrojovy prostor (stejny jako schvaleny navrh) 941x1672. BUILD2SC2:
  // vystupni rozliseni zvedeno z 0.5x na 0.75x (706x1254) - pri 0.5x bylo
  // pismo tak male (~6px), ze ani spravny font nevypadal dobre. Klavesnice
  // se prekresluje jen pri doteku (ne 50x/s jako obrazovka), takze vetsi
  // base64 payload (~3,6 MB misto ~1,6 MB) tady nevadi stejnym zpusobem
  // jako by vadil u neceho, co bezi kazdy snimek.
  static const int SRC_W = 941, SRC_H = 1672;
  static const int W = 706, H = 1254;   // round(SRC*0.75)

  uint8_t fb[W * H * 3];

  bool pressedKey[57];
  bool consoleHeld[5];     // vizualni + drzeny stav START/SELECT/OPTION
  bool shiftLatched = false, shiftHeld = false;
  long long shiftDownAtMs = -1;
  bool ctrlLatched = false;

  KbdDeck() { std::memset(pressedKey, 0, sizeof(pressedKey)); std::memset(consoleHeld, 0, sizeof(consoleHeld)); }

  static inline int X(float srcX) { return (int)(srcX * (float)W / (float)SRC_W + 0.5f); }
  static inline int Y(float srcY) { return (int)(srcY * (float)H / (float)SRC_H + 0.5f); }
  // Delka (ne pozice) - pro font-size/polomery prevzate z navrhu v cqw.
  static inline float SX(float srcPx) { return srcPx * (float)W / (float)SRC_W; }

  inline void setPx(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    uint8_t *p = &fb[(y * W + x) * 3];
    p[0] = r; p[1] = g; p[2] = b;
  }
  inline void blendPx(int x, int y, int r, int g, int b, int a) {
    if (x < 0 || x >= W || y < 0 || y >= H || a <= 0) return;
    if (a > 255) a = 255;
    uint8_t *p = &fb[(y * W + x) * 3];
    if (a >= 255) { p[0] = (uint8_t)r; p[1] = (uint8_t)g; p[2] = (uint8_t)b; return; }
    p[0] = (uint8_t)((p[0] * (255 - a) + r * a) / 255);
    p[1] = (uint8_t)((p[1] * (255 - a) + g * a) / 255);
    p[2] = (uint8_t)((p[2] * (255 - a) + b * a) / 255);
  }
  void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) setPx(xx, yy, r, g, b);
  }
  void fillRectA(int x, int y, int w, int h, int r, int g, int b, int a) {
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) blendPx(xx, yy, r, g, b, a);
  }

  static inline KRGB lerp(KRGB a, KRGB b, float t) {
    if (t < 0) t = 0;
    if (t > 1) t = 1;
    return krgb((int)(a.r + (b.r - a.r) * t), (int)(a.g + (b.g - a.g) * t), (int)(a.b + (b.b - a.b) * t));
  }

  // Zaobleny obdelnik se svislym 3-bodovym gradientem - presny rucni
  // ekvivalent CSS "border-radius: rxPct%/ryPct%; background:linear-
  // gradient(180deg, c0 0%, c1 t1*100%, c2 100%)". Lehke antialiasovani
  // jen na rohove krivce (e<=1.4), stred/hrany jsou ostre (netreba -
  // jsou rovne, ne zaoblene).
  void fillRoundedGrad(int x, int y, int w, int h, float rxPct, float ryPct,
                        KRGB c0, KRGB c1, float t1, KRGB c2) {
    if (w <= 0 || h <= 0) return;
    float rx = w * rxPct, ry = h * ryPct;
    if (rx > w * 0.5f) rx = w * 0.5f;
    if (ry > h * 0.5f) ry = h * 0.5f;
    for (int yy = 0; yy < h; yy++) {
      float t = h > 1 ? (float)yy / (float)(h - 1) : 0.f;
      KRGB c = (t <= t1) ? lerp(c0, c1, t1 > 0.0001f ? t / t1 : 0.f)
                          : lerp(c1, c2, t1 < 0.9999f ? (t - t1) / (1.f - t1) : 1.f);
      for (int xx = 0; xx < w; xx++) {
        float dx = 0, dy = 0; bool corner = false;
        if (rx > 0.5f && ry > 0.5f) {
          if (xx < rx && yy < ry) { dx = rx - (xx + 0.5f); dy = ry - (yy + 0.5f); corner = true; }
          else if (xx >= w - rx && yy < ry) { dx = (xx + 0.5f) - (w - rx); dy = ry - (yy + 0.5f); corner = true; }
          else if (xx < rx && yy >= h - ry) { dx = rx - (xx + 0.5f); dy = (yy + 0.5f) - (h - ry); corner = true; }
          else if (xx >= w - rx && yy >= h - ry) { dx = (xx + 0.5f) - (w - rx); dy = (yy + 0.5f) - (h - ry); corner = true; }
        }
        if (!corner) { setPx(x + xx, y + yy, c.r, c.g, c.b); continue; }
        float e = (dx * dx) / (rx * rx) + (dy * dy) / (ry * ry);
        if (e <= 1.f) setPx(x + xx, y + yy, c.r, c.g, c.b);
        else if (e <= 1.4f) blendPx(x + xx, y + yy, c.r, c.g, c.b, (int)((1.f - (e - 1.f) / 0.4f) * 255));
      }
    }
  }

  // Vnitrni "bevel" (CSS inset box-shadow) - 1px svetly prouzek nahore,
  // 2-3px tmavy prechod dole. hiA/shA jsou 0..1 (presne CSS alpha).
  void bevelInset(int x, int y, int w, int h, KRGB hi, float hiA, KRGB sh, float shA) {
    if (w <= 2 || h <= 2) return;
    if (hiA > 0.001f) fillRectA(x + 1, y, w - 2, 1, hi.r, hi.g, hi.b, (int)(hiA * 255));
    int sb = h > 10 ? 3 : 2;
    for (int i = 0; i < sb && i < h - 1; i++) {
      float a = shA * (1.f - (float)i / (float)sb);
      fillRectA(x + 1, y + h - 1 - i, w - 2, 1, sh.r, sh.g, sh.b, (int)(a * 255));
    }
  }

  // Jemny vnejsi stin ("key floating nad podkladem") - CSS ma skutecny
  // gaussovsky blur (box-shadow 0 4px 5px), tady jen aproximace: plochy
  // poloprusvitny pas pod tvarem. Male meritko - staci na dojem hloubky.
  void dropShadow(int x, int y, int w, int h, float strength) {
    fillRectA(x + 1, y + h, w - 2, 2, 0, 0, 0, (int)(strength * 255));
  }

  void outlineRect(int x, int y, int w, int h, int thick, KRGB col) {
    fillRect(x, y, w, thick, col.r, col.g, col.b);
    fillRect(x, y + h - thick, w, thick, col.r, col.g, col.b);
    fillRect(x, y, thick, h, col.r, col.g, col.b);
    fillRect(x + w - thick, y, thick, h, col.r, col.g, col.b);
  }

  void drawLine(int x0, int y0, int x1, int y1, KRGB col) {
    int dx = x1 > x0 ? x1 - x0 : x0 - x1, sx = x0 < x1 ? 1 : -1;
    int dy = -(y1 > y0 ? y1 - y0 : y0 - y1), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
      setPx(x0, y0, col.r, col.g, col.b);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
    }
  }

  static inline bool pointInTri(float px, float py, float x0, float y0, float x1, float y1, float x2, float y2) {
    float d1 = (px - x1) * (y0 - y1) - (x0 - x1) * (py - y1);
    float d2 = (px - x2) * (y1 - y2) - (x1 - x2) * (py - y2);
    float d3 = (px - x0) * (y2 - y0) - (x2 - x0) * (py - y0);
    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(hasNeg && hasPos);
  }
  void fillTriangleGrad(int x0, int y0, int x1, int y1, int x2, int y2, KRGB top, KRGB bot) {
    int minY = y0 < y1 ? (y0 < y2 ? y0 : y2) : (y1 < y2 ? y1 : y2);
    int maxY = y0 > y1 ? (y0 > y2 ? y0 : y2) : (y1 > y2 ? y1 : y2);
    int minX = x0 < x1 ? (x0 < x2 ? x0 : x2) : (x1 < x2 ? x1 : x2);
    int maxX = x0 > x1 ? (x0 > x2 ? x0 : x2) : (x1 > x2 ? x1 : x2);
    for (int yy = minY; yy <= maxY; yy++) {
      float t = (maxY > minY) ? (float)(yy - minY) / (float)(maxY - minY) : 0.f;
      KRGB c = lerp(top, bot, t);
      for (int xx = minX; xx <= maxX; xx++)
        if (pointInTri(xx + 0.5f, yy + 0.5f, (float)x0, (float)y0, (float)x1, (float)y1, (float)x2, (float)y2))
          setPx(xx, yy, c.r, c.g, c.b);
    }
  }
  // ".fuji{...background:linear-gradient(160deg,#f0ece0,#cfc8b4);
  //  border:1px solid #aca692;clip-path:polygon(50% 2%,96% 96%,4% 96%)}"
  void drawFuji(int x, int y, int w, int h) {
    int x0 = x + w / 2, y0 = y + (int)(h * 0.02f);
    int x1 = x + (int)(w * 0.96f), y1 = y + (int)(h * 0.96f);
    int x2 = x + (int)(w * 0.04f), y2 = y + (int)(h * 0.96f);
    fillTriangleGrad(x0, y0, x1, y1, x2, y2, krgb(240, 236, 224), krgb(207, 200, 180));
    KRGB bord = krgb(172, 166, 146);
    drawLine(x0, y0, x1, y1, bord); drawLine(x1, y1, x2, y2, bord); drawLine(x2, y2, x0, y0, bord);
  }

  // --- stb_truetype pismo (Chakra Petch - presne jako schvaleny navrh,
  // viz nap_atari_fonts.h pro puvod/licenci). Lazy-init, sdileno mezi
  // vsemi instancemi KbdDeck (nema smysl parsovat font tabulky vicekrat).
  struct Font { stbtt_fontinfo info; bool ok = false; };
  static Font &fSemiBold() {
    static Font f; static bool init = false;
    if (!init) { f.ok = stbtt_InitFont(&f.info, NAP_FONT_CHAKRA_SEMIBOLD_TTF,
                 stbtt_GetFontOffsetForIndex(NAP_FONT_CHAKRA_SEMIBOLD_TTF, 0)) != 0; init = true; }
    return f;
  }
  static Font &fBold() {
    static Font f; static bool init = false;
    if (!init) { f.ok = stbtt_InitFont(&f.info, NAP_FONT_CHAKRA_BOLD_TTF,
                 stbtt_GetFontOffsetForIndex(NAP_FONT_CHAKRA_BOLD_TTF, 0)) != 0; init = true; }
    return f;
  }
  static Font &fBoldItalic() {
    static Font f; static bool init = false;
    if (!init) { f.ok = stbtt_InitFont(&f.info, NAP_FONT_CHAKRA_BOLDITALIC_TTF,
                 stbtt_GetFontOffsetForIndex(NAP_FONT_CHAKRA_BOLDITALIC_TTF, 0)) != 0; init = true; }
    return f;
  }

  // Minimalni UTF-8 dekoder - popisky obsahuji sipky (↑↓←→, 3-bajtove
  // UTF-8 posloupnosti), overene (fonttools), ze Chakra Petch tyhle
  // znaky skutecne obsahuje (nejde o nahodu/odhad).
  static inline int utf8Next(const char *&p) {
    unsigned char c = (unsigned char)*p;
    if (c < 0x80) { p++; return c; }
    if ((c & 0xE0) == 0xC0 && p[1]) { int cp = ((c & 0x1F) << 6) | ((unsigned char)p[1] & 0x3F); p += 2; return cp; }
    if ((c & 0xF0) == 0xE0 && p[1] && p[2]) {
      int cp = ((c & 0x0F) << 12) | (((unsigned char)p[1] & 0x3F) << 6) | ((unsigned char)p[2] & 0x3F);
      p += 3; return cp;
    }
    if ((c & 0xF8) == 0xF0 && p[1] && p[2] && p[3]) {
      int cp = ((c & 0x07) << 18) | (((unsigned char)p[1] & 0x3F) << 12) | (((unsigned char)p[2] & 0x3F) << 6) | ((unsigned char)p[3] & 0x3F);
      p += 4; return cp;
    }
    p++; return c;
  }

  static float textWidthTT(Font &f, const char *s, float pxH) {
    if (!f.ok || !s || !s[0]) return 0.f;
    float scale = stbtt_ScaleForPixelHeight(&f.info, pxH);
    float w = 0; const char *p = s; int prev = 0;
    while (*p) {
      int cp = utf8Next(p);
      int adv, lsb; stbtt_GetCodepointHMetrics(&f.info, cp, &adv, &lsb);
      w += adv * scale;
      if (prev) w += stbtt_GetCodepointKernAdvance(&f.info, prev, cp) * scale;
      prev = cp;
    }
    return w;
  }

  // Vykresli text, (px,baselineY) = leva strana na zakladni lince.
  void drawTextTT(Font &f, int px, int baselineY, const char *s, float pxH, int r, int g, int b, float alphaMul = 1.f) {
    if (!f.ok || !s || !s[0]) return;
    float scale = stbtt_ScaleForPixelHeight(&f.info, pxH);
    float xpos = (float)px; const char *p = s; int prev = 0;
    static unsigned char bmp[64 * 64];
    while (*p) {
      int cp = utf8Next(p);
      int adv, lsb; stbtt_GetCodepointHMetrics(&f.info, cp, &adv, &lsb);
      if (prev) xpos += stbtt_GetCodepointKernAdvance(&f.info, prev, cp) * scale;
      int x0, y0, x1, y1;
      stbtt_GetCodepointBitmapBox(&f.info, cp, scale, scale, &x0, &y0, &x1, &y1);
      int gw = x1 - x0, gh = y1 - y0;
      if (gw > 0 && gh > 0 && gw <= 64 && gh <= 64) {
        stbtt_MakeCodepointBitmap(&f.info, bmp, gw, gh, gw, scale, scale, cp);
        int ox = (int)(xpos + 0.5f) + x0, oy = baselineY + y0;
        for (int yy = 0; yy < gh; yy++) for (int xx = 0; xx < gw; xx++) {
          uint8_t cov = bmp[yy * gw + xx];
          if (!cov) continue;
          int a = (int)(cov * alphaMul);
          blendPx(ox + xx, oy + yy, r, g, b, a);
        }
      }
      xpos += adv * scale;
      prev = cp;
    }
  }

  // Centrovany jeden radek (vodorovne i svisle podle metrik fontu) -
  // ekvivalent CSS display:flex;align-items:center;justify-content:center.
  void drawTextCenteredTT(Font &f, int cx, int cy, const char *s, float pxH, int r, int g, int b, float alphaMul = 1.f) {
    if (!f.ok || !s || !s[0]) return;
    float scale = stbtt_ScaleForPixelHeight(&f.info, pxH);
    float w = textWidthTT(f, s, pxH);
    int ascent, descent, lineGap; stbtt_GetFontVMetrics(&f.info, &ascent, &descent, &lineGap);
    int baseline = (int)(cy + (ascent + descent) * scale * 0.5f + 0.5f);
    drawTextTT(f, (int)(cx - w / 2.f + 0.5f), baseline, s, pxH, r, g, b, alphaMul);
  }

  void render(long long nowMs) {
    // SHIFT: drzeni > 320ms (i bez pusteni) se ma projevit hned - stejne
    // jako puvodni JS setTimeout(...,320). render() se vola prubezne
    // (viz hlavni smycka pri doteku), takze polling tady staci.
    if (shiftDownAtMs >= 0 && !shiftHeld && (nowMs - shiftDownAtMs) >= 320) {
      shiftHeld = true; shiftLatched = true;
    }

    Font &semi = fSemiBold(); Font &bold = fBold(); Font &boldItalic = fBoldItalic();

    // pozadi - ".devicebox{background:linear-gradient(180deg,#e4e3e2 0%,
    // #c9c7c6 52%,#a7a5a4 100%)}" - presne ty 3 zastavky.
    for (int yy = 0; yy < H; yy++) {
      float t = H > 1 ? (float)yy / (float)(H - 1) : 0.f;
      KRGB c = (t <= 0.52f) ? lerp(krgb(228, 227, 226), krgb(201, 199, 198), t / 0.52f)
                             : lerp(krgb(201, 199, 198), krgb(167, 165, 164), (t - 0.52f) / 0.48f);
      for (int xx = 0; xx < W; xx++) setPx(xx, yy, c.r, c.g, c.b);
    }

    // napis ATARI 130XE + fuji trojuhelnik - ".wordmark" box (60,825,
    // 500,50), flex radek zleva, svisle centrovano. ".wm-atari{font-
    // style:italic;font-weight:700;font-size:2.9cqw;color:#d8511f;
    // margin-right:1.6cqw}" / ".wm-130xe{font-weight:700;font-size:
    // 2.9cqw;color:#2b2a28}" / ".fuji{width:3.2cqw;height:3.2cqw;
    // margin-right:1cqw}". 1cqw = 1% sirky devicebox = SX(9.41).
    {
      int by = Y(825), bh = Y(875) - by, cy = by + bh / 2;
      int fx = X(60);
      int fsz = (int)SX(30.1f);
      drawFuji(fx, cy - fsz / 2, fsz, fsz);
      float atariSize = SX(27.3f);
      float atariW = textWidthTT(boldItalic, "ATARI", atariSize);
      int ax = fx + fsz + (int)SX(9.4f);
      drawTextCenteredTT(boldItalic, ax + (int)(atariW / 2), cy, "ATARI", atariSize, 216, 81, 31);
      float xeSize = SX(27.3f);
      float xeW = textWidthTT(bold, "130XE", xeSize);
      int bx2 = ax + (int)atariW + (int)SX(15.1f);
      drawTextCenteredTT(bold, bx2 + (int)(xeW / 2), cy, "130XE", xeSize, 43, 42, 40);
    }

    // barevny 3-pasmovy prouzek u konzole - presne z cstripeDefs:
    // {277,740,40,73,'#bf4310'} + {277,762,40,18,'#0a7183'} +
    // {277,783,40,30,'#06308e'} (pozdejsi pasy prekryvaji drivejsi,
    // presne jako vyssi z-index v navrhu).
    fillRect(X(277), Y(740), X(317) - X(277), Y(813) - Y(740), 191, 67, 16);   // #bf4310
    fillRect(X(277), Y(762), X(317) - X(277), Y(780) - Y(762), 10, 113, 131);  // #0a7183
    fillRect(X(277), Y(783), X(317) - X(277), Y(813) - Y(783), 6, 48, 142);    // #06308e

    // POZOR: konzolova tlacitka se ZAMERNE prekryvaji (presne cisla ze
    // schvaleneho navrhu - Rene: "To prekryti tlacitek HELP Start Select
    // a Option se me libi to tak nech"). Popisek se centruje jen v
    // NEPREKRYTE casti vlastniho tlacitka (viz effW), tvar/gradient/stin
    // pouziva plnou zmerenou sirku - vizualne stejne "stridave" prekryti
    // jako ve schvalenem navrhu.
    // ".cbtn{border-radius:14%/22%;background:linear-gradient(180deg,
    //  #f1efea 0%,#dad7d0 55%,#c4c0b7 100%);box-shadow:inset 0 1.5px 0
    //  rgba(255,255,255,.7),inset 0 -2.5px 3px rgba(60,55,45,.22),0 2px 0
    //  rgba(0,0,0,.2),0 4px 5px rgba(0,0,0,.25)}" / ".cbtn.pressed{
    //  background:linear-gradient(180deg,#b9b4aa 0%,#c9c5ba 60%,#d9d5c9
    //  100%);box-shadow:inset 0 2px 5px rgba(40,36,26,.4),0 1px 0
    //  rgba(0,0,0,.15);transform:translateY(2px)}" / ".cbtn span{font-
    //  weight:700;font-size:1.55cqw;color:#3a3630}"
    for (int i = 0; i < 5; i++) {
      const KbdConsoleDef &d = NAP_KBD_CONSOLE[i];
      int x = X(d.x), y = Y(d.y), w = X(d.x + d.w) - x, h = Y(d.y + d.h) - y;
      // BUILD2SC2 OPRAVA SKUTECNE CHYBY (nalezeno vizualni kontrolou PNG,
      // ne jen cteni kodu): puvodni "bool held = consoleHeld[i]" (v B289
      // i tady pred touhle opravou) cetlo SPATNY index - HELP se vizualne
      // tvarilo zmacknute misto START a SELECT misto OPTION. Viz
      // consoleIsHeld() vyse a test_b2sc1 (scenar START+OPTION drzene).
      bool held = consoleIsHeld(i);
      int yy = held ? y + 2 : y;
      dropShadow(x, yy, w, h, held ? 0.15f : 0.22f);
      if (held) {
        fillRoundedGrad(x, yy, w, h, 0.14f, 0.22f, krgb(185, 180, 170), krgb(201, 197, 186), 0.60f, krgb(217, 213, 201));
        bevelInset(x, yy, w, h, krgb(255, 255, 255), 0.f, krgb(40, 36, 26), 0.40f);
      } else {
        fillRoundedGrad(x, yy, w, h, 0.14f, 0.22f, krgb(241, 239, 234), krgb(218, 215, 208), 0.55f, krgb(196, 192, 183));
        bevelInset(x, yy, w, h, krgb(255, 255, 255), 0.7f, krgb(60, 55, 45), 0.22f);
      }
      int cy = yy + h / 2;
      int effW = (i + 1 < 5) ? (X(NAP_KBD_CONSOLE[i + 1].x) - x) : w;
      if (effW < 1) effW = 1;
      drawTextCenteredTT(bold, x + effW / 2, cy, d.lab, SX(14.6f), 58, 54, 48);
    }

    // 57 klaves - ".key{border-radius:16%/20%;background:linear-gradient(
    //  180deg,#f4eee1 0%,#e3d8c4 48%,#cabea8 100%);box-shadow:inset 0
    //  1.4px 0 rgba(255,255,255,.75),inset 0 -2.6px 3.5px rgba(95,82,58,
    //  .28),0 2px 0 rgba(0,0,0,.22),0 4px 5px rgba(0,0,0,.3)}" / ".key
    //  .l1{font-size:1.25cqw;font-weight:600;opacity:.68;color:#453d2e}"
    //  / ".key .l2{font-size:1.6cqw;font-weight:700;color:#2b2a28}" /
    //  ".key.pressed{background:linear-gradient(180deg,#c3b89f 0%,
    //  #d2c7b1 55%,#e1d6c1 100%);...transform:translateY(2.4px)}" /
    //  ".key.shiftLatched{background:linear-gradient(180deg,#ffd87a 0%,
    //  #f3b63e 55%,#e0a02a 100%);box-shadow:...,0 0 0 2px #ffcf5c,...}"
    //  ".key.shiftLatched .l2{color:#4a3208}"
    for (int i = 0; i < 57; i++) {
      const KbdKeyDef &k = NAP_KBD_KEYS[i];
      int x = X(k.x), y = Y(k.y), w = X(k.x + k.w) - x, h = Y(k.y + k.h) - y;
      bool gold = (k.scan == -2) && shiftLatched;
      bool pressed = pressedKey[i] || (k.scan == -1 && ctrlLatched);
      int yy = pressed ? y + 2 : y;
      dropShadow(x, yy, w, h, pressed ? 0.15f : (gold ? 0.30f : 0.25f));
      if (gold) {
        fillRoundedGrad(x, yy, w, h, 0.16f, 0.20f, krgb(255, 216, 122), krgb(243, 182, 62), 0.55f, krgb(224, 160, 42));
        bevelInset(x, yy, w, h, krgb(255, 255, 255), 0.75f, krgb(120, 70, 10), 0.32f);
        outlineRect(x - 1, yy - 1, w + 2, h + 2, 1, krgb(255, 207, 92));
      } else if (pressed) {
        fillRoundedGrad(x, yy, w, h, 0.16f, 0.20f, krgb(195, 184, 159), krgb(210, 199, 177), 0.55f, krgb(225, 214, 193));
        bevelInset(x, yy, w, h, krgb(255, 255, 255), 0.f, krgb(60, 50, 30), 0.42f);
      } else {
        fillRoundedGrad(x, yy, w, h, 0.16f, 0.20f, krgb(244, 238, 225), krgb(227, 216, 196), 0.48f, krgb(202, 190, 168));
        bevelInset(x, yy, w, h, krgb(255, 255, 255), 0.75f, krgb(95, 82, 58), 0.28f);
      }

      bool hasL1 = k.l1 && k.l1[0];
      bool hasL2 = k.l2 && k.l2[0];
      if (hasL1 || hasL2) {
        float l1Size = SX(11.76f), l2Size = SX(15.06f);
        float h1 = l1Size * 1.15f, h2 = l2Size * 1.25f;
        int cyc = yy + h / 2;
        float top = hasL1 ? (cyc - (h1 + h2) / 2.f) : (cyc - h2 / 2.f);
        int l2r = 43, l2g = 42, l2b = 40; if (gold) { l2r = 74; l2g = 50; l2b = 8; }
        if (hasL1) drawTextCenteredTT(semi, x + w / 2, (int)(top + h1 / 2), k.l1, l1Size, 69, 61, 46, 0.68f);
        if (hasL2) drawTextCenteredTT(bold, x + w / 2, (int)(top + (hasL1 ? h1 : 0) + h2 / 2), k.l2, l2Size, l2r, l2g, l2b);
      }
    }
  }

  // (x,y) v OUTPUT prostoru (0..W-1,0..H-1). Vraci 0..56 = klaves,
  // 100..104 = konzolovy pas (100+action), -1 = nic.
  // BUILD2SC2: beze zmeny (geometrie se nehybla, jen vzhled).
  int hitTest(int x, int y) const {
    for (int i = 0; i < 5; i++) {
      const KbdConsoleDef &d = NAP_KBD_CONSOLE[i];
      int x0=X(d.x), y0=Y(d.y), x1=X(d.x+d.w), y1=Y(d.y+d.h);
      if (x>=x0 && x<x1 && y>=y0 && y<y1) return 100 + i;
    }
    for (int i = 0; i < 57; i++) {
      const KbdKeyDef &k = NAP_KBD_KEYS[i];
      int x0=X(k.x), y0=Y(k.y), x1=X(k.x+k.w), y1=Y(k.y+k.h);
      if (x>=x0 && x<x1 && y>=y0 && y<y1) return i;
    }
    return -1;
  }

  // Mapovani NAP_KBD_CONSOLE indexu (0..4=HELP/START/SELECT/OPTION/RESET)
  // na consoleHeld[] (0..2=START/SELECT/OPTION - viz touchDown nize).
  // BUILD2SC2: vytazeno do vlastni funkce prave proto, aby se tahle
  // mapovaci logika dala primo otestovat (viz test_b2sc1), ne jen
  // "doufat", ze render() a touchDown() si rozumi.
  bool consoleIsHeld(int i) const { return (i >= 1 && i <= 3) ? consoleHeld[i - 1] : false; }

  int consolHeldBits() const {
    int m = 0;
    if (consoleHeld[0]) m |= 1;   // START = bit0
    if (consoleHeld[1]) m |= 2;   // SELECT = bit1
    if (consoleHeld[2]) m |= 4;   // OPTION = bit2
    return m;
  }

  // BUILD2SC2: touchDown/touchUp se VUBEC NEMENI - cista logika, uz
  // overena (test_b2sc1), oprava vzhledu se ji netyka.
  KbdEvent touchDown(int id, long long nowMs) {
    KbdEvent ev;
    if (id >= 100 && id <= 104) {
      int action = id - 100;
      if (action == 0) { ev.typ = KbdEvent::KLAVESA; ev.scan = 17; return ev; }       // HELP
      if (action == 4) { ev.typ = KbdEvent::RESET; return ev; }                       // RESET
      int ci = action - 1;                                                             // START/SELECT/OPTION
      consoleHeld[ci] = true;
      ev.typ = KbdEvent::KONZOLE_MASK; ev.konzoleMask = 7 & ~consolHeldBits();
      return ev;
    }
    if (id < 0 || id > 56) return ev;
    const KbdKeyDef &k = NAP_KBD_KEYS[id];
    if (k.scan == -1) { ctrlLatched = !ctrlLatched; return ev; }           // CTRL: instant toggle
    if (k.scan == -2) { shiftDownAtMs = nowMs; shiftHeld = false; return ev; } // SHIFT: wait and see (tap/hold)
    if (k.scan == -3) { ev.typ = KbdEvent::BREAK; return ev; }             // BREAK: jednorazove
    pressedKey[id] = true;
    int scan = k.scan;
    if (shiftLatched) scan |= 0x40;
    if (ctrlLatched)  scan |= 0x80;
    ev.typ = KbdEvent::KLAVESA; ev.scan = scan & 0xFF;
    return ev;
  }

  KbdEvent touchUp(int id, long long /*nowMs*/) {
    KbdEvent ev;
    if (id >= 100 && id <= 104) {
      int action = id - 100;
      if (action == 1 || action == 2 || action == 3) {
        consoleHeld[action - 1] = false;
        ev.typ = KbdEvent::KONZOLE_MASK; ev.konzoleMask = 7 & ~consolHeldBits();
      }
      return ev;
    }
    if (id < 0 || id > 56) return ev;
    const KbdKeyDef &k = NAP_KBD_KEYS[id];
    if (k.scan == -2) {   // SHIFT pusteno
      if (shiftHeld) { shiftLatched = false; shiftHeld = false; }  // bylo drzeni - konci s pustenim
      else shiftLatched = !shiftLatched;                           // bylo jen tuknuti - prepni zamek
      shiftDownAtMs = -1;
      return ev;
    }
    pressedKey[id] = false;
    return ev;
  }
};

} // namespace nap
