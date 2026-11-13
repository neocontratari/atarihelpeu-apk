// test_overeni/b320/mapa_dune.c
// B320: "DUNE 2000" PRO SKUTECNY VYKRESLOVAC PS1 (gpu_neon z appky) - maly program pro PlayStation
// (PS-EXE), ktery kresli 2D strategii jako Dune 2000: obraz 368x240, vlevo vyhled na mapu 296x240
// z dlazdic 16x16, vpravo panel 72 bodu, jednotky, kurzor. Hra kresli jen kazdy 3. snimek (20 obrazu
// za sekundu jako Dune 2000), dva obrazy ve videopameti (y 0 / 256) a prepina je GP1(05h).
//
// Mapa se kresli dvema zpusoby (test prepina za behu, STAV[S_REZIM]):
//   0 = dlazdice jako polygony s texturou a modulaci barvou (0x707070) + DITHERING (E1 bit 9) - tak
//       PS1 kresli textury se stinovanim; dithering je mrizka 4x4 pevne na obrazovce,
//   1 = dlazdice jako sprity (obdelniky s texturou) - PS1 je nikdy neditheruje.
// Jednotky, kurzor a ikony v panelu jsou vzdy sprity, panel je obdelnik jednou barvou.
// O kolik se mapa posune za jeden obraz hry, urcuje test (STAV[S_DX], STAV[S_DY]); hra hlasi,
// kolik obrazu nakreslila a kde je kamera.
//
// Preklad (udelej_mapu.py): clang --target=mipsel-unknown-elf -march=mips1 (R3000, clang hlida
// load delay sloty) -ffreestanding -nostdlib, start.S, ld.lld na 0x80010000, PS-EXE.
// Zadne nasobeni ani deleni (hazardy HI/LO na R3000) - jen posuny, scitani, xor.

typedef unsigned int u32;
typedef unsigned short u16;
typedef unsigned char u8;
typedef int s32;

#define GP0 (*(volatile u32 *)0x1F801810)
#define GP1 (*(volatile u32 *)0x1F801814)
#define I_STAT (*(volatile u32 *)0x1F801070)
#define I_MASK (*(volatile u32 *)0x1F801074)
#define STAV ((volatile u32 *)0x80100000)

// vymena s testem (pamet PS1 0x80100000, slova)
enum {
  S_ZNACKA = 0,   // 'DUNE' = hra bezi
  S_OBRAZU = 1,   // nakreslenych obrazu
  S_CAMX = 2, S_CAMY = 3,
  S_VBLANKU = 4,
  S_REZIM_HRA = 5, // rezim posledniho obrazu
  S_REZIM = 8,    // test: 0 = polygony s ditheringem, 1 = sprity
  S_DX = 9, S_DY = 10,   // test: posun kamery za obraz hry (se znamenkem)
  S_ZASTAV = 11   // test: 1 = jednotky stoji (obraz se nemeni)
};

#define VW 296
#define VH 240
#define SW 368
#define MAPA_T 64           // mapa 64x64 dlazdic = 1024x1024 bodu
#define TEX_X 512           // textury ve videopameti (15 bitu): 128x48 na (512, 0)
#define TEX_W 128
#define TEX_H 48

static u8 mapa[MAPA_T * MAPA_T];
static u16 tex[TEX_W * TEX_H];
static u32 rs = 0x1234567u;
static u32 rnd(void) { rs ^= rs << 13; rs ^= rs >> 17; rs ^= rs << 5; return rs; }
static u16 c15(int r, int g, int b) {
  if (r < 1) r = 1; if (g < 1) g = 1; if (b < 1) b = 1;
  if (r > 31) r = 31; if (g > 31) g = 31; if (b > 31) b = 31;
  return (u16)(r | (g << 5) | (b << 10));
}
static int absi(int v) { return v < 0 ? -v : v; }
static const s32 SMER[4] = {-1, 0, 1, 0};   // nahodny smer jednotky bez deleni (rnd() & 3)

// dlazdice t (0..15) v texture: 8 v radku, 2 radky; radek 3 (v 32..47) = jednotky a ikony
static void dlazdice(int t) {
  const int u0 = (t & 7) << 4, v0 = (t >> 3) << 4;
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < 16; x++) {
      int r, g, b;
      const int z = (int)(rnd() & 7) - 3, z2 = (int)(rnd() & 3) - 1;
      if (t < 4) {                        // pisek se zrnem (4 varianty)
        r = 25 + z + (t & 1); g = 20 + z + z2 - (t >> 1); b = 12 + (z >> 1);
      } else if (t < 8) {                 // skaly
        const int okraj = (t == 4 && y < 2) || (t == 5 && x < 2) || (t == 6 && y > 13) || (t == 7 && x > 13);
        r = okraj ? 9 : 15 + z; g = okraj ? 7 : 12 + z; b = okraj ? 5 : 10 + z;
      } else if (t < 12) {                // koreni: pisek s oranzovymi teckami
        const int tecka = ((x + (t << 1)) & 3) == 0 && ((y + t) & 3) == 1;
        r = tecka ? 29 : 24 + z; g = tecka ? 14 + z2 : 19 + z; b = tecka ? 3 : 11;
      } else {                            // budova (vzor, okraj)
        const int okraj = x == 0 || y == 0 || x == 15 || y == 15;
        const int v = (((x >> 2) + (y >> 2)) & 1) ? 5 : 0;
        r = okraj ? 4 : 11 + v + (t & 3); g = okraj ? 4 : 11 + v; b = okraj ? 4 : 14 + v;
      }
      tex[(v0 + y) * TEX_W + u0 + x] = c15(r, g, b);
    }
}

static void textury(void) {
  for (int t = 0; t < 16; t++) dlazdice(t);
  // radek 3: jednotky 12x12 (2 barvy) na u 0 a 16, kurzor 10x10 na u 32, ikony 28x14 na u 48 a 80
  for (int y = 0; y < 16; y++)
    for (int x = 0; x < TEX_W; x++) tex[(32 + y) * TEX_W + x] = 0;   // pruhledne
  for (int k = 0; k < 2; k++)
    for (int y = 0; y < 12; y++)
      for (int x = 0; x < 12; x++) {
        const int okraj = x == 0 || y == 0 || x == 11 || y == 11;
        if ((x == 0 || x == 11) && (y == 0 || y == 11)) continue;
        tex[(32 + y) * TEX_W + (k << 4) + x] = okraj ? c15(2, 2, 2) : (((x + y) & 3) ? (k ? c15(6, 10, 28) : c15(28, 6, 6)) : c15(30, 30, 30));
      }
  for (int y = 0; y < 10; y++)
    for (int x = 0; x <= y; x++) tex[(32 + y) * TEX_W + 32 + x] = (x == 0 || x == y || y == 9) ? c15(1, 1, 1) : c15(31, 31, 31);
  for (int k = 0; k < 2; k++)
    for (int y = 0; y < 14; y++)
      for (int x = 0; x < 28; x++) tex[(32 + y) * TEX_W + 48 + (k << 5) + x] = c15(8 + (x >> 1) + k * 6, 14 + (y >> 1), 26 - k * 8);
  // nahrat do videopameti: GP0(A0h) kopie CPU -> VRAM
  GP0 = 0xA0000000u;
  GP0 = (0u << 16) | TEX_X;
  GP0 = ((u32)TEX_H << 16) | TEX_W;
  for (int i = 0; i < TEX_W * TEX_H; i += 2) GP0 = (u32)tex[i] | ((u32)tex[i + 1] << 16);
}

static void udelejMapu(void) {
  for (int i = 0; i < MAPA_T * MAPA_T; i++) mapa[i] = (u8)(rnd() & 3);
  for (int k = 0; k < 40; k++) {          // skaly a koreni (kosoctverce - bez nasobeni)
    const int cx = (int)(rnd() & 63), cy = (int)(rnd() & 63), r = 2 + (int)(rnd() & 3);
    const int druh = (rnd() & 1) ? 4 : 8;
    for (int y = cy - r; y <= cy + r; y++)
      for (int x = cx - r; x <= cx + r; x++) {
        if (x < 0 || y < 0 || x >= MAPA_T || y >= MAPA_T || absi(x - cx) + absi(y - cy) > r) continue;
        mapa[(y << 6) + x] = (u8)(druh + (rnd() & 3));
      }
  }
  for (int k = 0; k < 25; k++) {          // budovy 2x2 dlazdice
    const int x = (int)(rnd() & 63) & ~1, y = (int)(rnd() & 63) & ~1;
    if (x >= MAPA_T - 1 || y >= MAPA_T - 1) continue;
    mapa[(y << 6) + x] = 12; mapa[(y << 6) + x + 1] = 13; mapa[((y + 1) << 6) + x] = 14; mapa[((y + 1) << 6) + x + 1] = 15;
  }
}

struct Jed { s32 x, y, vx, vy; };
static struct Jed jed[24];

static void nastavKresleni(int x0, int y0, int x1, int y1, int bufY, int dither) {
  // E1: textury na (512, 0) = stranka X 8, 15 bitu (2 << 7), dithering (bit 9), kreslit i do zobrazeneho (bit 10)
  GP0 = 0xE1000000u | 8u | (2u << 7) | (dither ? (1u << 9) : 0u) | (1u << 10);
  GP0 = 0xE2000000u;
  GP0 = 0xE3000000u | (u32)x0 | ((u32)(y0 + bufY) << 10);
  GP0 = 0xE4000000u | (u32)x1 | ((u32)(y1 + bufY) << 10);
  GP0 = 0xE5000000u | ((u32)bufY << 11);
  GP0 = 0xE6000000u;
}
static u32 vrchol(int x, int y) { return ((u32)(y & 0xFFFF) << 16) | (u32)(x & 0xFFFF); }

static void sprite(int x, int y, int u, int v, int w, int h) {
  GP0 = 0x65808080u;                    // obdelnik s texturou, nepruhledny, bez modulace
  GP0 = vrchol(x, y);
  GP0 = ((u32)v << 8) | (u32)u;
  GP0 = ((u32)h << 16) | (u32)w;
}

static void kresli(int bufY, int camX, int camY, int rezim, int kurX, int kurY, int tik) {
  // --- mapa (oriznuta na vyhled 296x240) ---
  nastavKresleni(0, 0, VW - 1, VH - 1, bufY, rezim == 0);
  const int tx0 = camX >> 4, ty0 = camY >> 4, ox = camX & 15, oy = camY & 15;
  const u32 tpage = 8u | (2u << 7);
  for (int ty = 0; ty <= (VH >> 4); ty++)
    for (int tx = 0; tx <= (VW >> 4) + 1; tx++) {
      const int mx = tx0 + tx, my = ty0 + ty;
      if (mx >= MAPA_T || my >= MAPA_T) continue;
      const int t = mapa[(my << 6) + mx];
      const int u0 = (t & 7) << 4, v0 = (t >> 3) << 4;
      const int x = (tx << 4) - ox, y = (ty << 4) - oy;
      if (x >= VW || y >= VH) continue;
      if (rezim == 0) {
        GP0 = 0x2C707070u;              // ctyruhelnik s texturou, modulace barvou 0x70 (7/8)
        GP0 = vrchol(x, y);
        GP0 = ((u32)v0 << 8) | (u32)u0;
        GP0 = vrchol(x + 16, y);
        GP0 = (tpage << 16) | ((u32)v0 << 8) | (u32)(u0 + 16);
        GP0 = vrchol(x, y + 16);
        GP0 = ((u32)(v0 + 16) << 8) | (u32)u0;
        GP0 = vrchol(x + 16, y + 16);
        GP0 = ((u32)(v0 + 16) << 8) | (u32)(u0 + 16);
      } else {
        GP0 = 0x7D808080u;              // sprite 16x16 s texturou, bez modulace
        GP0 = vrchol(x, y);
        GP0 = ((u32)v0 << 8) | (u32)u0;
      }
    }
  // --- jednotky (sprity) ---
  for (int i = 0; i < 24; i++) {
    const int sx = jed[i].x - camX, sy = jed[i].y - camY;
    if (sx < -12 || sy < -12 || sx >= VW || sy >= VH) continue;
    sprite(sx, sy, (i & 1) << 4, 32, 12, 12);
  }
  // --- panel vpravo (cely obraz) ---
  nastavKresleni(0, 0, SW - 1, VH - 1, bufY, 0);
  GP0 = 0x60302018u;                    // obdelnik jednou barvou (BGR)
  GP0 = vrchol(VW, 0);
  GP0 = ((u32)VH << 16) | (u32)(SW - VW);
  for (int i = 0; i < 6; i++) sprite(VW + 6 + ((i & 1) << 5), 70 + i * 27, 48 + ((i & 1) << 5), 32, 28, 14);
  // ramecek kamery na male mape (posouva se s kamerou) a kredity (meni se)
  GP0 = 0x60F0F0F0u;
  GP0 = vrchol(VW + 4 + (camX >> 4), 6 + (camY >> 4));
  GP0 = (2u << 16) | 18u;
  GP0 = 0x6040C0F0u;
  GP0 = vrchol(VW + 8, 228);
  GP0 = (6u << 16) | (u32)(4 + ((tik >> 3) & 31));
  // --- kurzor ---
  sprite(kurX, kurY, 32, 32, 10, 10);
}

void main(void) {
  __asm__ volatile("mtc0 $zero, $12");   // preruseni vypnuta - VBlank se cte z I_STAT
  I_MASK = 0;
  for (int i = 0; i < 64; i++) STAV[i] = 0;
  GP1 = 0x00000000u;                    // reset GPU
  GP1 = 0x08000040u;                    // 368x240, NTSC, 15 bitu, bez prokladani
  GP1 = 0x06000000u | 0x260u | ((0x260u + 368u * 7u) << 12);
  GP1 = 0x07000000u | 16u | (256u << 10);
  GP1 = 0x05000000u;
  GP1 = 0x03000000u;                    // obraz zapnout
  textury();
  udelejMapu();
  for (int i = 0; i < 24; i++) {
    jed[i].x = 200 + (int)(rnd() & 511); jed[i].y = 200 + (int)(rnd() & 511);
    jed[i].vx = SMER[rnd() & 3]; jed[i].vy = (rnd() & 1) ? 1 : -1;
  }
  int camX = 320, camY = 320, buf = 0, odpocet = 0, kurX = 100, kurY = 100, kdx = 2, kdy = 1, tik = 0;
  u32 vbl = 0, obrazu = 0;
  STAV[S_ZNACKA] = 0x454E5544u;         // "DUNE"
  for (;;) {
    while (!(I_STAT & 1u)) {}
    I_STAT = ~1u;
    STAV[S_VBLANKU] = ++vbl;
    if (odpocet > 0) { odpocet--; continue; }
    odpocet = 2;                          // kresli kazdy 3. snimek (20 obrazu / s)
    const int rezim = (int)STAV[S_REZIM];
    camX += (s32)STAV[S_DX];
    camY += (s32)STAV[S_DY];
    if (camX < 0) camX = 0;
    if (camY < 0) camY = 0;
    if (camX > (MAPA_T << 4) - VW - 16) camX = (MAPA_T << 4) - VW - 16;
    if (camY > (MAPA_T << 4) - VH - 16) camY = (MAPA_T << 4) - VH - 16;
    if (!STAV[S_ZASTAV]) {
      for (int i = 0; i < 24; i++) {
        jed[i].x += jed[i].vx; jed[i].y += jed[i].vy;
        if ((rnd() & 63) == 0) { jed[i].vx = SMER[rnd() & 3]; jed[i].vy = SMER[rnd() & 3]; }
      }
      kurX += kdx; kurY += kdy;
      if (kurX < 10 || kurX > 270) kdx = -kdx;
      if (kurY < 10 || kurY > 220) kdy = -kdy;
      tik++;
    }
    const int bufY = buf ? 256 : 0;
    kresli(bufY, camX, camY, rezim, kurX, kurY, tik);
    GP1 = 0x05000000u | ((u32)bufY << 10);   // zobrazit novy obraz
    buf ^= 1;
    STAV[S_CAMX] = (u32)camX; STAV[S_CAMY] = (u32)camY; STAV[S_REZIM_HRA] = (u32)rezim;
    STAV[S_OBRAZU] = ++obrazu;
  }
}
