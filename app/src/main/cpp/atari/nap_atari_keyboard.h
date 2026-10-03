// nap_atari_keyboard.h
// BUILD2SC1: klavesnice + konzolova tlacitka atari 130XE - CISTE V C++.
//
// Rene: "preved do apky sekce help atari c++ a dej si pozor at mame emu
// atari 130xe v HELP ciste v c++." Tenhle soubor NEKRESLI pres HTML/CSS -
// vyrabi primo pixely (RGB framebuffer), presne jako uz dela obrazovka
// samotneho Atari (AnticView). WebView ho jen zobrazi, nic si nedokresluje.
//
// PUVOD DAT (zadne hadani - obe tabulky primo zmereny/odzkousene):
//  - Pixelova geometrie (x,y,sirka,vyska, popisky) je 1:1 prevzata ze
//    schvaleneho claude.ai navrhu (Main.dc.html, rowX/rowY/rowW/rowH/
//    rowLabels + consoleDefs) - presne ta cisla, ktera Rene uz videl a
//    schvalil na fotce 941x1672.
//  - Scankody jsou prevzate ze stareho, uz let overeneho JS jadra
//    (assets/emu_vbxe/index.html, tabulka ROWS) - rucne porovnano radek
//    po radku: oba zdroje maji STEJNY pocet klaves ve STEJNEM poradi v
//    kazde rade (15+14+14+13+1=57), takze se spoluji 1:1 bez domnenek.
//
// CO TO NENI (zatim): kazetak, servisni tlacitka, logo obrazek, 3D stin
// na napisu ATARI 130XE. To je dalsi kolo (Rene uz byl o tom informovan).
//
// POPISKY NA KLAVESACH: v schvalenem navrhu je CSS flexbox, ktery text
// sam zmensi/zalomi. Tady se kresli pevnymi pixely ROM pisma (8x8), ktere
// se nezmensi - proto nekolik dlouhych slov (Control/Return/Break/BkSp/
// Inverse) ma tady kratsi zkratku (CTRL/RET/BRK/BS/INV), aby se vubec
// vesly do sirky klavesy. Mala "shift" znacka (CLEAR/INSERT/DELETE/Fuji)
// se v tomhle kole NEKRESLI - zpetna vazba shiftu je zlata klavesa SHIFT
// samotna.
#pragma once
#include <cstdint>
#include <cstring>
#include "nap_atari_roms.h"

namespace nap {

// ---------------------------------------------------------------
//  TABULKA 57 KLAVES - zdroj viz komentar nahore.
//  scan: realny Atari scankod; -1=CTRL(zamek-prepinac), -2=SHIFT
//        (tuknuti=zamek, drzeni>320ms=docasne), -3=BREAK (jednorazove).
// ---------------------------------------------------------------
struct KbdKeyDef { float x, y, w, h; int16_t scan; const char *lab; };

static const KbdKeyDef NAP_KBD_KEYS[57] = {
  // rada 0 (y=901,h=64) - presne z rowX[0]/rowW[0]/ROWS[0]
  {41,901,58,64,28,"ESC"}, {108,901,51,64,31,"1"}, {168,901,50,64,30,"2"},
  {227,901,48,64,26,"3"}, {284,901,48,64,24,"4"}, {340,901,47,64,29,"5"},
  {395,901,48,64,27,"6"}, {452,901,45,64,51,"7"}, {505,901,46,64,53,"8"},
  {560,901,45,64,48,"9"}, {614,901,46,64,50,"0"}, {667,901,48,64,54,"<"},
  {723,901,48,64,55,">"}, {779,901,50,64,52,"BS"}, {837,901,58,64,-3,"BRK"},
  // rada 1 (y=966,h=67)
  {41,966,81,67,44,"TAB"}, {130,966,54,67,47,"Q"}, {190,966,52,67,46,"W"},
  {249,966,53,67,42,"E"}, {308,966,52,67,40,"R"}, {367,966,50,67,45,"T"},
  {423,966,49,67,43,"Y"}, {478,966,50,67,11,"U"}, {533,966,47,67,13,"I"},
  {587,966,49,67,8,"O"}, {641,966,49,67,10,"P"}, {696,966,49,67,14,"-"},
  {751,966,51,67,15,"="}, {808,966,87,67,12,"RET"},
  // rada 2 (y=1034,h=65)
  {41,1034,90,65,-1,"CTRL"}, {141,1034,52,65,63,"A"}, {202,1034,50,65,62,"S"},
  {261,1034,50,65,58,"D"}, {320,1034,50,65,56,"F"}, {378,1034,49,65,61,"G"},
  {437,1034,46,65,57,"H"}, {492,1034,47,65,1,"J"}, {548,1034,45,65,5,"K"},
  {601,1034,47,65,0,"L"}, {656,1034,45,65,2,";"}, {710,1034,49,65,6,"+"},
  {766,1034,52,65,7,"*"}, {825,1034,70,65,60,"CAPS"},
  // rada 3 (y=1101,h=68)
  {41,1101,121,68,-2,"SHIFT"}, {171,1101,51,68,23,"Z"}, {231,1101,48,68,22,"X"},
  {289,1101,49,68,18,"C"}, {345,1101,48,68,16,"V"}, {403,1101,46,68,21,"B"},
  {459,1101,46,68,35,"N"}, {514,1101,47,68,37,"M"}, {570,1101,47,68,32,","},
  {626,1101,46,68,34,"."}, {680,1101,45,68,38,"/"}, {734,1101,91,68,-2,"SHIFT"},
  {833,1101,62,68,39,"INV"},
  // mezernik
  {216,1175,484,52,33,""},
};

// ---------------------------------------------------------------
//  KONZOLOVY PAS (presne z consoleDefs v mockupu, y=740,h=73).
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
// ---------------------------------------------------------------
struct KbdEvent {
  enum Typ { NIC, KLAVESA, KONZOLE_MASK, RESET, BREAK } typ = NIC;
  int scan = 0;        // pro KLAVESA (uz vcetne SHIFT/CTRL bitu)
  int konzoleMask = 7; // pro KONZOLE_MASK - CELY vysledny 3bitovy stav
};

// ---------------------------------------------------------------
//  Deck - vlastni jen VIZUALNI/UI stav (co je prave zmacknute, jestli
//  je SHIFT zamceny) - realny stav stroje (klavesnice, konzole) zije
//  jen v Machine, nikdy se tu neduplikuje.
// ---------------------------------------------------------------
struct KbdDeck {
  // Zdrojovy prostor (stejny jako schvalena fotka) 941x1672, vystup
  // zmenseny na polovinu - base64 RGB v plne velikosti (941*1672*3 =
  // 4,7 MB) by bylo pro WebView bridge zbytecne velke na kazdy dotek;
  // polovicni rozliseni (~1,2 MB syrovych dat) je rozumny kompromis.
  // Viditelne to jsou "retro" hranate pismo (ROM font bez vyhlazeni) -
  // k vzhledu Atari to sedi, neni to chyba.
  static const int SRC_W = 941, SRC_H = 1672;
  static const int W = 471, H = 836;   // round(SRC*0.5)

  uint8_t fb[W * H * 3];

  bool pressedKey[57];
  bool consoleHeld[5];     // vizualni + drzeny stav START/SELECT/OPTION
  bool shiftLatched = false, shiftHeld = false;
  long long shiftDownAtMs = -1;
  bool ctrlLatched = false;

  KbdDeck() { std::memset(pressedKey, 0, sizeof(pressedKey)); std::memset(consoleHeld, 0, sizeof(consoleHeld)); }

  static inline int X(float srcX) { return (int)(srcX * (float)W / (float)SRC_W + 0.5f); }
  static inline int Y(float srcY) { return (int)(srcY * (float)H / (float)SRC_H + 0.5f); }

  inline void setPx(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x < 0 || x >= W || y < 0 || y >= H) return;
    uint8_t *p = &fb[(y * W + x) * 3];
    p[0] = r; p[1] = g; p[2] = b;
  }
  void fillRect(int x, int y, int w, int h, uint8_t r, uint8_t g, uint8_t b) {
    for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) setPx(xx, yy, r, g, b);
  }

  // ROM font - presne stejny zdroj a format jako uz overene v
  // nap_intro_gl.cpp (TextObrazovka::nahraj): 8x8 bodu, font na $E000
  // (NAP_OS_ROM+0x2000), 1 bajt = 1 radek znaku, bit 0x80>>x = bod x.
  static inline int vnitrniKod(char z) {
    uint8_t c = (uint8_t)z;
    if (c >= 0x20 && c <= 0x5F) return c - 0x20;
    if (c >= 0x60 && c <= 0x7F) return c;
    return 0;
  }
  void drawChar(int px, int py, char z, int scale, uint8_t r, uint8_t g, uint8_t b) {
    const uint8_t *font = NAP_OS_ROM + 0x2000;
    int k = vnitrniKod(z);
    for (int y = 0; y < 8; y++) {
      uint8_t bity = font[k * 8 + y];
      if (!bity) continue;
      for (int x = 0; x < 8; x++) {
        if (!(bity & (0x80 >> x))) continue;
        fillRect(px + x * scale, py + y * scale, scale, scale, r, g, b);
      }
    }
  }
  // Leva horni souradnice textu. 1 znak = 8*scale bodu + scale mezera.
  void drawText(int px, int py, const char *s, int scale, uint8_t r, uint8_t g, uint8_t b) {
    int x = px;
    for (const char *c = s; *c; c++) { drawChar(x, py, *c, scale, r, g, b); x += 8 * scale + scale; }
  }
  static int textWidth(const char *s, int scale) {
    int n = (int)strlen(s);
    if (n == 0) return 0;
    return n * (8 * scale + scale) - scale;
  }
  void drawTextCentered(int cx, int cy, const char *s, int scale, uint8_t r, uint8_t g, uint8_t b) {
    int w = textWidth(s, scale), h = 8 * scale;
    drawText(cx - w / 2, cy - h / 2, s, scale, r, g, b);
  }

  // Jednoducha "raised keycap" iluze bez gradientu - 1-2px svetla
  // horni/leva hrana, tmava spodni/prava hrana. Presne stejny princip
  // jako box-shadow v schvalenem CSS navrhu, jen bez rozostreni.
  void drawCap(int x, int y, int w, int h, bool pressed, bool gold) {
    uint8_t baseR, baseG, baseB, topR, topG, topB, botR, botG, botB;
    if (gold) {       // SHIFT zamceny - presne odstiny .key.shiftLatched
      baseR=243;baseG=182;baseB=62; topR=255;topG=216;topB=122; botR=224;botG=160;botB=42;
    } else if (pressed) {
      baseR=201;baseG=191;baseB=169; topR=168;topG=155;topB=127; botR=225;botG=214;botB=193;
    } else {          // .key - normal
      baseR=227;baseG=216;baseB=196; topR=244;topG=238;topB=225; botR=202;botG=190;botB=168;
    }
    int yy = pressed ? y + 1 : y;    // .pressed{transform:translateY(2.4px)}
    fillRect(x, yy, w, h, baseR, baseG, baseB);
    fillRect(x, yy, w, 2, topR, topG, topB);
    fillRect(x, yy, 2, h, topR, topG, topB);
    fillRect(x, yy + h - 2, w, 2, botR, botG, botB);
    fillRect(x + w - 2, yy, 2, h, botR, botG, botB);
  }

  void render(long long nowMs) {
    // SHIFT: drzeni > 320ms (i bez pusteni) se ma projevit hned - stejne
    // jako puvodni JS setTimeout(...,320). render() se vola prubezne
    // (viz hlavni smycka), takze polling tady staci, zadny casovac netreba.
    if (shiftDownAtMs >= 0 && !shiftHeld && (nowMs - shiftDownAtMs) >= 320) {
      shiftHeld = true; shiftLatched = true;
    }

    // pozadi - .devicebox prumerny odstin
    fillRect(0, 0, W, H, 184, 182, 180);

    // napis ATARI 130XE (bez 3D stinu zatim - viz komentar nahore)
    drawTextCentered(X(230), Y(850), "ATARI", 2, 216, 81, 31);
    drawTextCentered(X(620), Y(850), "130XE", 2, 43, 42, 40);

    // barevny prouzek u konzole (presne 3 odstiny z cstripeDefs)
    fillRect(X(277), Y(740), X(317)-X(277), Y(813)-Y(740), 191, 67, 16);

    // POZOR: konzolova tlacitka se ZAMERNE prekryvaji (presne cisla ze
    // schvaleneho navrhu - Rene: "To prekryti tlacitek HELP Start Select
    // a Option se me libi to tak nech"). V CSS to funguje, protoze pozdejsi
    // (vyssi z-index) tlacitko prekryje jen OKRAJ predchoziho, ne jeho
    // stred s popiskem. Tady, bez CSS vrstev, by prosty fillRect() dalsiho
    // tlacitka popisek predchoziho preresil - proto se popisek centruje
    // jen v NEPREKRYTE (viditelne) casti vlastniho tlacitka, ne v celem
    // zmerenem obdelniku. Samotny tvar/stin (drawCap) pouziva plnou sirku -
    // vizualne stejne "stridave" prekryti jako ve schvalenem navrhu.
    for (int i = 0; i < 5; i++) {
      const KbdConsoleDef &d = NAP_KBD_CONSOLE[i];
      int x=X(d.x), y=Y(d.y), w=X(d.x+d.w)-x, h=Y(d.y+d.h)-y;
      bool held = consoleHeld[i];
      drawCap(x, y, w, h, held, false);
      int cy = held ? y+h/2+1 : y+h/2;
      int effW = (i+1 < 5) ? (X(NAP_KBD_CONSOLE[i+1].x) - x) : w;
      if (effW < 1) effW = 1;
      drawTextCentered(x+effW/2, cy, d.lab, 1, 58, 54, 48);
    }

    for (int i = 0; i < 57; i++) {
      const KbdKeyDef &k = NAP_KBD_KEYS[i];
      int x=X(k.x), y=Y(k.y), w=X(k.x+k.w)-x, h=Y(k.y+k.h)-y;
      bool gold = (k.scan == -2) && shiftLatched;
      bool pressed = pressedKey[i] || (k.scan == -1 && ctrlLatched);
      drawCap(x, y, w, h, pressed, gold);
      if (k.lab[0]) {
        int cy = pressed ? y+h/2+1 : y+h/2;
        uint8_t tr=43,tg=42,tb=40;
        if (gold) { tr=74; tg=50; tb=8; }
        drawTextCentered(x+w/2, cy, k.lab, 1, tr, tg, tb);
      }
    }
  }

  // (x,y) v OUTPUT prostoru (0..W-1,0..H-1). Vraci 0..56 = klaves,
  // 100..104 = konzolovy pas (100+action), -1 = nic.
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

  int consolHeldBits() const {
    int m = 0;
    if (consoleHeld[0]) m |= 1;   // START = bit0
    if (consoleHeld[1]) m |= 2;   // SELECT = bit1
    if (consoleHeld[2]) m |= 4;   // OPTION = bit2
    return m;
  }

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
