// nap_atari_cart.h
// ====================================================================
//  B316: CARTRIDGE PRO ATARI 130XE (soubory .car)
//
//  Rene: "slo by do atari emu pridat i priponu *.car - tedy aby tyto hry
//  sly v nasem emu spustit?" (poslal Wonderboy_VBXE.car = typ 42,
//  Atarimax 1 MB Flash (old), 128 bank po 8 KB, hra pro VBXE).
//
//  Soubor .car: 16 B hlavicka - "CART", typ (32 bit big endian), kontrolni
//  soucet (soucet vsech bajtu dat, 32 bit big endian), 4 B nic - a za ni
//  data ROM (velikost dana typem).
//
//  Cartridge ve slotu 130XE:
//   - okna $8000-$9FFF (linka RD4) a $A000-$BFFF (RD5) maji prednost pred
//     vestavenym BASICem i pred RAM (procesor i ANTIC),
//   - TRIG3 = RD5 - podle nej OS pozna, ze je cartridge zasunuta,
//   - banky se prepinaji ctenim nebo zapisem na $D500-$D5FF (linka CCTL),
//     u Bounty Bob pristupem na $8FF6-$8FF9 / $9FF6-$9FF9,
//   - tlacitko RESET cartridge neresetuje (slot 130XE nema resetovaci
//     linku) - stav bank se vraci jen pri zapnuti (POWER).
//  Chovani typu (ktera banka, kdy se okno vypne, stav po zapnuti) je podle
//  emulatoru Atari800 (src/cartridge.c, commit 98cc2dc) - test
//  test_overeni/b316 ho s timhle kodem porovnava pristup po pristupu pro
//  vsech podporovanych 67 typu. Kod je napsany znovu.
//  Bez Androidu - cele v C++, testuje se na PC.
// ====================================================================
#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

namespace nap {

struct CartTyp { int kb; const char *jmeno; int umim; };

// typy .car 0-112: velikost v kB a nazev jako v Atari800 (cartridge_info.c);
// umim = 1 -> B316 ho umi spustit (jinak se slusne odmitne)
static const CartTyp CART_TYPY[113] = {
    {    0, "NONE",                                        0},   // 0
    {    8, "Standard 8 KB",                               1},   // 1
    {   16, "Standard 16 KB",                              1},   // 2
    {   16, "OSS two chip 16 KB (034M)",                   1},   // 3
    {   32, "Standard 32 KB 5200",                         0},   // 4
    {   32, "DB 32 KB",                                    1},   // 5
    {   16, "Two chip 16 KB 5200",                         0},   // 6
    {   40, "Bounty Bob 40 KB 5200",                       0},   // 7
    {   64, "64 KB Williams",                              1},   // 8
    {   64, "Express 64 KB",                               1},   // 9
    {   64, "Diamond 64 KB",                               1},   // 10
    {   64, "SpartaDOS X 64 KB",                           1},   // 11
    {   32, "XEGS 32 KB",                                  1},   // 12
    {   64, "XEGS 64 KB (banks 0-7)",                      1},   // 13
    {  128, "XEGS 128 KB",                                 1},   // 14
    {   16, "OSS one chip 16 KB",                          1},   // 15
    {   16, "One chip 16 KB 5200",                         0},   // 16
    {  128, "Decoded Atrax 128 KB",                        1},   // 17
    {   40, "Bounty Bob 40 KB",                            1},   // 18
    {    8, "Standard 8 KB 5200",                          0},   // 19
    {    4, "Standard 4 KB 5200",                          0},   // 20
    {    8, "Right slot 8 KB",                             0},   // 21
    {   32, "32 KB Williams",                              1},   // 22
    {  256, "XEGS 256 KB",                                 1},   // 23
    {  512, "XEGS 512 KB",                                 1},   // 24
    { 1024, "XEGS 1 MB",                                   1},   // 25
    {   16, "MegaCart 16 KB",                              1},   // 26
    {   32, "MegaCart 32 KB",                              1},   // 27
    {   64, "MegaCart 64 KB",                              1},   // 28
    {  128, "MegaCart 128 KB",                             1},   // 29
    {  256, "MegaCart 256 KB",                             1},   // 30
    {  512, "MegaCart 512 KB",                             1},   // 31
    { 1024, "MegaCart 1 MB",                               1},   // 32
    {   32, "Switchable XEGS 32 KB",                       1},   // 33
    {   64, "Switchable XEGS 64 KB",                       1},   // 34
    {  128, "Switchable XEGS 128 KB",                      1},   // 35
    {  256, "Switchable XEGS 256 KB",                      1},   // 36
    {  512, "Switchable XEGS 512 KB",                      1},   // 37
    { 1024, "Switchable XEGS 1 MB",                        1},   // 38
    {    8, "Phoenix 8 KB",                                1},   // 39
    {   16, "Blizzard 16 KB",                              1},   // 40
    {  128, "Atarimax 128 KB Flash",                       1},   // 41
    { 1024, "Atarimax 1 MB Flash (old)",                   1},   // 42
    {  128, "SpartaDOS X 128 KB",                          1},   // 43
    {    8, "OSS 8 KB",                                    1},   // 44
    {   16, "OSS two chip 16 KB (043M)",                   1},   // 45
    {    4, "Blizzard 4 KB",                               1},   // 46
    {   32, "AST 32 KB",                                   0},   // 47
    {   64, "Atrax SDX 64 KB",                             1},   // 48
    {  128, "Atrax SDX 128 KB",                            1},   // 49
    {   64, "Turbosoft 64 KB",                             1},   // 50
    {  128, "Turbosoft 128 KB",                            1},   // 51
    {   32, "Ultracart 32 KB",                             1},   // 52
    {    8, "Low bank 8 KB",                               1},   // 53
    {  128, "SIC! 128 KB",                                 1},   // 54
    {  256, "SIC! 256 KB",                                 1},   // 55
    {  512, "SIC! 512 KB",                                 1},   // 56
    {    2, "Standard 2 KB",                               1},   // 57
    {    4, "Standard 4 KB",                               1},   // 58
    {    4, "Right slot 4 KB",                             0},   // 59
    {   32, "Blizzard 32 KB",                              1},   // 60
    { 2048, "MegaMax 2 MB",                                1},   // 61
    {131072, "The!Cart 128 MB",                             0},   // 62
    { 4096, "Flash MegaCart 4 MB",                         1},   // 63
    { 2048, "MegaCart 2 MB",                               1},   // 64
    {32768, "The!Cart 32 MB",                              0},   // 65
    {65536, "The!Cart 64 MB",                              0},   // 66
    {   64, "XEGS 64 KB (banks 8-15)",                     0},   // 67
    {  128, "Atrax 128 KB",                                1},   // 68
    {   32, "aDawliah 32 KB",                              1},   // 69
    {   64, "aDawliah 64 KB",                              1},   // 70
    {   64, "Super Cart 64 KB 5200",                       0},   // 71
    {  128, "Super Cart 128 KB 5200",                      0},   // 72
    {  256, "Super Cart 256 KB 5200",                      0},   // 73
    {  512, "Super Cart 512 KB 5200",                      0},   // 74
    { 1024, "Atarimax 1 MB Flash (new)",                   1},   // 75
    {   16, "16 KB Williams",                              0},   // 76
    {    8, "MIO diagnostics 8KB",                         0},   // 77
    {    8, "Telelink II",                                 0},   // 78
    {   16, "Pronto",                                      0},   // 79
    {   64, "JRC64 (linear)",                              0},   // 80
    {   64, "MDDOS",                                       0},   // 81
    {   32, "COS32",                                       0},   // 82
    { 1024, "SIC+ 1024 KB",                                0},   // 83
    { 1032, "Corina 1M + 8K EEPROM",                       0},   // 84
    {  520, "Corina 512K + 512K SRAM + 8K EEPROM",         0},   // 85
    {    8, "XE Multicart (8KB)",                          0},   // 86
    {   16, "XE Multicart (16KB)",                         0},   // 87
    {   32, "XE Multicart (32KB)",                         0},   // 88
    {   64, "XE Multicart (64KB)",                         0},   // 89
    {  128, "XE Multicart (128KB)",                        0},   // 90
    {  256, "XE Multicart (256KB)",                        0},   // 91
    {  512, "XE Multicart (512KB)",                        0},   // 92
    { 1024, "XE Multicart (1024KB)",                       0},   // 93
    {   64, "Ram-Cart 64 KB",                              0},   // 94
    {  128, "Ram-Cart 128 KB",                             0},   // 95
    {  256, "Double Ram-Cart 256 KB",                      0},   // 96
    { 1024, "Ram-Cart 1 MB",                               0},   // 97
    { 2048, "Ram-Cart 2 MB",                               0},   // 98
    { 4096, "Ram-Cart 4 MB",                               0},   // 99
    { 8192, "Ram-Cart 8 MB",                               0},   // 100
    {16384, "Ram-Cart 16 MB",                              0},   // 101
    {32768, "Ram-Cart 32 MB",                              0},   // 102
    {   32, "SiDiCar 32 KB",                               0},   // 103
    {    8, "J(atari)Cart8(kB)",                           1},   // 104
    {   16, "J(atari)Cart16(kB)",                          1},   // 105
    {   32, "J(atari)Cart32(kB)",                          1},   // 106
    {   64, "J(atari)Cart64(kB)",                          1},   // 107
    {  128, "J(atari)Cart128(kB)",                         1},   // 108
    {  256, "J(atari)Cart256(kB)",                         1},   // 109
    {  512, "J(atari)Cart512(kB)",                         1},   // 110
    { 1024, "J(atari)Cart1024(kB)",                        1},   // 111
    {  512, "DCart",                                       1},   // 112
};

inline int cartKb(int typ) {
  if (typ >= 0 && typ <= 112) return CART_TYPY[typ].kb;
  return typ == 159 ? 40 : typ == 160 ? 64 : 0;
}
inline const char *cartJmeno(int typ) {
  if (typ >= 0 && typ <= 112) return CART_TYPY[typ].jmeno;
  if (typ == 159) return "Bounty Bob 40 KB 5200 (alt.)";
  if (typ == 160) return "JRC64 (interleaved)";
  return "?";
}
inline bool cartUmim(int typ) { return typ >= 1 && typ <= 112 && CART_TYPY[typ].umim; }
inline bool cartPro5200(int typ) {
  return typ == 4 || typ == 6 || typ == 7 || typ == 16 || typ == 19 || typ == 20 || (typ >= 71 && typ <= 74) || typ == 159;
}
inline bool cartPravySlot(int typ) { return typ == 21 || typ == 59; }   // pravy slot mel jen Atari 800

class Cartridge {
 public:
  enum Chyba { OK = 0, KRATKY, NENI_CAR, NEZNAMY_TYP, PRO_5200, PRAVY_SLOT, NEUMIM, MALO_DAT };
  static const int32_t NIC = -1;     // v okne cartridge neni (RAM / BASIC)
  static const int32_t FF = -2;      // okno cartridge, ctou se $FF (OSS: "prazdny" stav)

  int typ = 0;                       // 0 = slot prazdny
  int stav = 0;                      // banka / priznaky (jako "state" v Atari800)
  std::string jmeno;                 // soubor
  std::vector<uint8_t> data;         // obsah ROM (u Atrax rozsifrovany, 2/4 KB doplnene na 8 KB)
  uint32_t soucetHlavicka = 0, soucetDat = 0;
  int32_t okno[4] = {NIC, NIC, NIC, NIC};   // $8000, $9000, $A000, $B000 po 4 KB: posun v data / NIC / FF
  long long zmen = 0;                // kolikrat se mapovani zmenilo (do logu)

  bool vlozena() const { return typ != 0; }
  bool rd4() const { return okno[0] != NIC || okno[1] != NIC; }
  bool rd5() const { return okno[2] != NIC || okno[3] != NIC; }
  bool bountyBob() const { return typ == 18; }

  void vyjmout() {
    typ = 0; stav = 0; jmeno.clear();
    std::vector<uint8_t>().swap(data);
    for (auto &o : okno) o = NIC;
    soucetHlavicka = soucetDat = 0; zmen = 0;
  }

  // bajt, ktery CPU / ANTIC uvidi na adrese a ($8000-$BFFF); jen kdyz okno != NIC
  inline uint8_t cti(uint16_t a) const {
    const int32_t o = okno[(a >> 12) - 8];
    return o >= 0 ? data[(size_t)o + (a & 0x0FFF)] : 0xFF;
  }

  // ---------------- nacteni .car ----------------
  // Vraci Chyba; pri OK je cartridge pripravena (stav po zapnuti). Spatny
  // kontrolni soucet = jen varovani (soucetHlavicka != soucetDat), jako Atari800.
  Chyba nactiCar(const uint8_t *p, size_t n, const std::string &jm) {
    vyjmout();
    if (!p || n < 16) return KRATKY;
    if (p[0] != 'C' || p[1] != 'A' || p[2] != 'R' || p[3] != 'T') return NENI_CAR;
    const uint32_t t = ((uint32_t)p[4] << 24) | ((uint32_t)p[5] << 16) | ((uint32_t)p[6] << 8) | p[7];
    const int kb = (t <= 1000u) ? cartKb((int)t) : 0;
    if (t == 0 || kb == 0) { typ = 0; posledniTyp = (int)(t > 100000u ? -1 : (int)t); return NEZNAMY_TYP; }
    posledniTyp = (int)t;
    if (cartPro5200((int)t)) return PRO_5200;
    if (cartPravySlot((int)t)) return PRAVY_SLOT;
    if (!cartUmim((int)t)) return NEUMIM;
    const size_t delka = (size_t)kb * 1024;
    if (n - 16 < delka) return MALO_DAT;
    soucetHlavicka = ((uint32_t)p[8] << 24) | ((uint32_t)p[9] << 16) | ((uint32_t)p[10] << 8) | p[11];
    uint32_t s = 0;
    for (size_t i = 0; i < delka; i++) s += p[16 + i];
    soucetDat = s;
    typ = (int)t;
    jmeno = jm;
    data.assign(p + 16, p + 16 + delka);
    upravData();
    studenyStart();
    return OK;
  }
  int posledniTyp = 0;               // typ z hlavicky posledniho pokusu (i neuspesneho) - do hlasek
  size_t prebytek(size_t n) const { const size_t d = (size_t)cartKb(typ) * 1024 + 16; return n > d ? n - d : 0; }

  // stav po zapnuti pocitace (POWER) - vychozi banka
  void studenyStart() {
    switch (typ) {
      case 3: stav = 1; break;          // OSS two chip 16 KB (034M)
      case 42: stav = 0x7f; break;      // Atarimax 1 MB (old): po zapnuti posledni banka
      case 63: stav = 254; break;       // Flash MegaCart 4 MB
      default: stav = 0; break;
    }
    mapuj();
  }

  // ---------------- $D500-$D5FF ----------------
  // cteni: u typu, ktere prepinaji uz ctenim (Atarimax, Williams, OSS ...), se
  // prepne; vraci hodnotu registru (SIC!, MegaCart 4 MB) nebo -1 = cartridge
  // na sbernici nic nedava (zustane, co tam bylo)
  int cteniD5(uint16_t a) {
    int novy = stav;
    if (pristupD5(a, novy)) zmenStav(novy);
    switch (typ) {
      case 54: case 55: case 56: case 63:
        if ((a & 0xe0) == 0x00) return stav & 0xff;
        break;
      case 112:                                              // DCart: $D500-$D5FF = kus ROM vybrane banky
        return data[(size_t)(stav & 0x3f) * 0x2000 + 0x1500 + (a & 0xff)];
      default: break;
    }
    return -1;
  }
  void zapisD5(uint16_t a, uint8_t v) {
    int novy = stav;
    switch (typ) {
      case 12: novy = v & 0x03; break;                       // XEGS 32 KB
      case 13: novy = v & 0x07; break;                       // XEGS 64 KB (banky 0-7)
      case 14: novy = v & 0x0f; break;                       // XEGS 128 KB
      case 23: novy = v & 0x1f; break;                       // XEGS 256 KB
      case 24: novy = v & 0x3f; break;                       // XEGS 512 KB
      case 25: novy = v & 0x7f; break;                       // XEGS 1 MB
      case 26: novy = v & 0x80; break;                       // MegaCart 16 KB
      case 27: novy = v & 0x81; break;                       // MegaCart 32 KB
      case 28: case 33: novy = v & 0x83; break;              // MegaCart 64 KB, Switchable XEGS 32 KB
      case 29: case 34: novy = v & 0x87; break;              // MegaCart 128 KB, Switchable XEGS 64 KB
      case 30: case 35: case 17: case 68: novy = v & 0x8f; break;   // MegaCart 256 KB, SwXEGS 128 KB, Atrax
      case 31: case 36: novy = v & 0x9f; break;              // MegaCart 512 KB, Switchable XEGS 256 KB
      case 32: case 37: novy = v & 0xbf; break;              // MegaCart 1 MB, Switchable XEGS 512 KB
      case 64: case 38: novy = v; break;                     // MegaCart 2 MB, Switchable XEGS 1 MB
      case 54: case 55: case 56: case 63:                    // SIC!, MegaCart 4 MB: registr na $D500-$D51F
        if ((a & 0xe0) == 0x00) novy = v;
        break;
      case 112: novy = a & 0xff; break;                      // DCart: banku vybira jen zapis (adresa)
      default:
        if (!pristupD5(a, novy)) return;
        break;
    }
    zmenStav(novy);
  }

  // Bounty Bob Strikes Back (typ 18): pristup na $8FF6-$8FF9 / $9FF6-$9FF9
  // vybere 4 KB banku pro $8000 / $9000 (cteni vrati uz bajt z nove banky)
  inline void bbPristup(uint16_t a) {
    const int lo = a & 0x0FFF;
    if (lo < 0xFF6 || lo > 0xFF9) return;
    const int b = lo - 0xFF6;
    zmenStav((a & 0xF000) == 0x8000 ? ((stav & 0x0c) | b) : ((stav & 0x03) | (b << 2)));
  }

 private:
  void zmenStav(int novy) {
    if (novy == stav) return;
    stav = novy;
    mapuj();
    zmen++;
  }

  // nove okno: 8 KB na slotu i (0 = $8000, 2 = $A000) od posunu off
  void okno8(int i, size_t off) { okno[i] = (int32_t)off; okno[i + 1] = (int32_t)(off + 0x1000); }

  // pristup na $D5xx (cteni i zapis) u typu, ktere reaguji na adresu
  bool pristupD5(uint16_t adr, int &novy) const {
    const int a = adr & 0xff;
    switch (typ) {
      case 3:   // OSS two chip 16 KB (034M)
      case 45:  // OSS two chip 16 KB (043M)
        if (a & 0x08) { novy = -1; return true; }
        switch (a & 0x07) {
          case 0x00: novy = 0; break;
          case 0x03: case 0x07: novy = (typ == 3) ? 1 : 2; break;
          case 0x04: novy = (typ == 3) ? 2 : 1; break;
          default: novy = 0xff; break;      // 1, 2, 5, 6: $A000-$AFFF = $FF
        }
        return true;
      case 5: novy = a & 0x03; return true;                 // DB 32 KB
      case 8: novy = a & 0x0f; return true;                 // Williams 64 KB
      case 22: novy = a & 0x0b; return true;                // Williams 32 KB
      case 9:                                               // Express 64 KB: $D57x
        if ((a & 0xf0) != 0x70) return false;
        novy = (a ^ 7) & 0x0f; return true;
      case 10:                                              // Diamond 64 KB: $D5Dx
        if ((a & 0xf0) != 0xd0) return false;
        novy = (a ^ 7) & 0x0f; return true;
      case 11: case 48:                                     // SpartaDOS X 64 KB, Atrax SDX 64 KB: $D5Ex
        if ((a & 0xf0) != 0xe0) return false;
        novy = (a & 0x08) ? (a & 0x0c) : ((a ^ 0x07) & 0x0f); return true;
      case 43: case 49:                                     // SpartaDOS X 128 KB, Atrax SDX 128 KB: $D5Ex/$D5Fx
        if ((a & 0xe0) != 0xe0) return false;
        novy = (a & 0x08) ? (a & 0x0c) : ((a ^ 0x17) & 0x1f); return true;
      case 15:                                              // OSS one chip 16 KB
        switch (a & 0x09) { case 0x00: novy = 1; break; case 0x01: novy = 3; break; case 0x08: novy = -1; break; default: novy = 2; break; }
        return true;
      case 44:                                              // OSS 8 KB
        switch (a & 0x09) { case 0x00: case 0x01: novy = 1; break; case 0x08: novy = -1; break; default: novy = 0; break; }
        return true;
      case 46: case 39: case 40: novy = 1; return true;     // Blizzard 4 / 16 KB, Phoenix 8 KB: vypnout
      case 41:                                              // Atarimax 128 KB: $D500-$D51F
        if ((a & 0xe0) != 0) return false;
        novy = a & 0x1f; return true;
      case 51: novy = a & 0x1f; return true;                // Turbosoft 128 KB
      case 50: novy = a & 0x17; return true;                // Turbosoft 64 KB
      case 42: case 61: case 75:                            // Atarimax 1 MB (old/new), MegaMax 2 MB,
      case 104: case 105: case 106: case 107:               // J(atari)Cart 8 KB - 1 MB
      case 108: case 109: case 110: case 111:
        novy = a; return true;
      case 52: novy = (stav + 1) % 5; return true;          // Ultracart 32 KB
      case 69: novy = (stav + 1) & 0x03; return true;       // aDawliah 32 KB
      case 70: novy = (stav + 1) & 0x07; return true;       // aDawliah 64 KB
      case 60:                                              // Blizzard 32 KB
        if (stav < 4) { novy = stav + 1; return true; }
        return false;
      default: return false;
    }
  }

  // podle typu a stavu nastavi okna (co CPU vidi na $8000-$BFFF)
  void mapuj() {
    for (auto &o : okno) o = NIC;
    const int s = stav;
    switch (typ) {
      case 1: case 57: case 58: case 46: case 39:           // Standard 8 / 2 / 4 KB, Blizzard 4 KB, Phoenix 8 KB
        if ((typ == 46 || typ == 39) && s) break;           // Blizzard / Phoenix: po pristupu na $D5 vypnuto
        okno8(2, 0);
        break;
      case 53: okno8(0, 0); break;                          // Low bank 8 KB: jen $8000-$9FFF
      case 2: case 40:                                      // Standard 16 KB, Blizzard 16 KB
        if (typ == 40 && s) break;
        okno8(0, 0); okno8(2, 0x2000);
        break;
      case 3: case 45:                                      // OSS two chip 16 KB: $B000 = 4. kus
      case 15: case 44: {                                   // OSS one chip 16 KB, OSS 8 KB: $B000 = 1. kus
        if (s < 0) break;
        okno[3] = (typ == 3 || typ == 45) ? 0x3000 : 0;
        okno[2] = (s == 0xff) ? FF : (int32_t)(s * 0x1000);
        break;
      }
      case 8: case 9: case 10: case 11: case 22: case 48: case 70:   // Williams, Express, Diamond, SDX 64, Atrax SDX 64, aDawliah 64
        if (s & 0x08) break;
        okno8(2, (size_t)(s & 0x07) * 0x2000);
        break;
      case 5: case 12: case 33: zapniXegs(0x6000); break;   // DB 32 KB, XEGS / Switchable XEGS 32 KB
      case 13: case 34: zapniXegs(0xe000); break;           // 64 KB
      case 14: case 35: zapniXegs(0x1e000); break;          // 128 KB
      case 23: case 36: zapniXegs(0x3e000); break;          // 256 KB
      case 24: case 37: zapniXegs(0x7e000); break;          // 512 KB
      case 25: case 38: zapniXegs(0xfe000); break;          // 1 MB
      case 17: case 42: case 68: case 75:                   // Atrax 128 KB, Atarimax 1 MB (old/new)
        if (s & 0x80) break;
        okno8(2, (size_t)(s & 0x7f) * 0x2000);
        break;
      case 41: case 50: case 51:                            // Atarimax 128 KB, Turbosoft 64 / 128 KB
        if (s & 0x10) break;
        okno8(2, (size_t)(s & 0x0f) * 0x2000);
        break;
      case 26: case 27: case 28: case 29: case 30: case 31: case 32: case 61: case 64:   // MegaCart, MegaMax 2 MB
        if (s & 0x80) break;
        okno8(0, (size_t)(s & 0x7f) * 0x4000); okno8(2, (size_t)(s & 0x7f) * 0x4000 + 0x2000);
        break;
      case 63:                                              // Flash MegaCart 4 MB
        if (s == 0xff) break;
        okno8(0, (size_t)s * 0x4000); okno8(2, (size_t)s * 0x4000 + 0x2000);
        break;
      case 54: case 55: case 56: {                          // SIC! 128 / 256 / 512 KB
        const int m = typ == 54 ? 0x07 : typ == 55 ? 0x0f : 0x1f;
        if (s & 0x20) okno8(0, (size_t)(s & m) * 0x4000);
        if (!(s & 0x40)) okno8(2, (size_t)(s & m) * 0x4000 + 0x2000);
        break;
      }
      case 43: case 49:                                     // SpartaDOS X 128 KB, Atrax SDX 128 KB
        if (s & 0x08) break;
        okno8(2, (size_t)((s & 0x07) + ((s & 0x10) >> 1)) * 0x2000);
        break;
      case 52: case 60: case 69:                            // Ultracart 32 KB, Blizzard 32 KB, aDawliah 32 KB
        if (s & 0x04) break;
        okno8(2, (size_t)(s & 0x03) * 0x2000);
        break;
      case 18:                                              // Bounty Bob Strikes Back 40 KB
        okno[0] = (int32_t)((s & 0x03) * 0x1000);
        okno[1] = (int32_t)(0x4000 + ((s & 0x0c) >> 2) * 0x1000);
        okno8(2, 0x8000);
        break;
      case 104: case 105: case 106: case 107: case 108: case 109: case 110: case 111: case 112: {   // J(atari)Cart, DCart
        static const int MASKA[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0x3f};
        if (s & 0x80) break;
        okno8(2, (size_t)(s & MASKA[typ - 104]) * 0x2000);
        break;
      }
      default: break;
    }
  }
  // XEGS: $8000-$9FFF = vybrana banka, $A000-$BFFF = posledni banka; Switchable (bit 7) = vypnuto cele
  void zapniXegs(size_t posledni) {
    if (stav & 0x80) return;
    okno8(0, (size_t)stav * 0x2000);
    okno8(2, posledni);
  }

  // Standard 2 / 4 KB a Blizzard 4 KB doplnit na 8 KB (jak je CPU vidi na $A000-$BFFF);
  // Atrax 128 KB a Atrax SDX maji prehazene adresni a datove vodice (ochrana proti kopirovani)
  void upravData() {
    if (typ == 57 || typ == 58 || typ == 46) {
      const size_t k = data.size();
      std::vector<uint8_t> d(0x2000, 0xff);
      if (typ == 46) { std::memcpy(&d[0], data.data(), k); std::memcpy(&d[0x1000], data.data(), k); }   // zrcadlo
      else std::memcpy(&d[0x2000 - k], data.data(), k);       // na konec okna ($B800 / $B000)
      data.swap(d);
    }
    if (typ == 68) rozsifruj(ATRAX_A, ATRAX_D);
    if (typ == 48 || typ == 49) rozsifruj(ATRAXSDX_A, ATRAXSDX_D);
  }
  static constexpr unsigned ATRAX_A[17] = {0x0020, 0x0040, 0x0080, 0x1000, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010,
                                           0x0100, 0x0400, 0x0800, 0x0200, 0x2000, 0x4000, 0x8000, 0x10000};
  static constexpr uint8_t ATRAX_D[8] = {0x10, 0x20, 0x04, 0x80, 0x08, 0x01, 0x02, 0x40};
  static constexpr unsigned ATRAXSDX_A[17] = {0x0040, 0x0080, 0x1000, 0x8000, 0x4000, 0x2000, 0x0100, 0x0020, 0x0010,
                                              0x0008, 0x0001, 0x0002, 0x0004, 0x0200, 0x0800, 0x0400, 0x10000};
  static constexpr uint8_t ATRAXSDX_D[8] = {0x02, 0x08, 0x80, 0x40, 0x01, 0x04, 0x20, 0x10};
  void rozsifruj(const unsigned *mA, const uint8_t *mD) {
    const size_t n = data.size();
    std::vector<uint8_t> d(n);
    for (size_t i = 0; i < n; i++) {
      unsigned ra = 0;
      for (int b = 0; b < 17; b++) if (i & (1u << b)) ra |= mA[b];
      const uint8_t v = ra < n ? data[ra] : 0xff;
      uint8_t o = 0;
      for (int b = 0; b < 8; b++) if (v & (1u << b)) o |= mD[b];
      d[i] = o;
    }
    data.swap(d);
  }
};

}  // namespace nap
