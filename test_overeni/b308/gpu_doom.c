/* test_overeni/b308/gpu_doom.c
 * =====================================================================
 *  B308: DOOM NA PS1 - SKUTECNY GRAFICKY CIP APPKY (gpulib + gpu_neon)
 *  NA PC, S PRIKAZY PRESNE TAKOVYMI, JAKE POSILA PSX DOOM.
 *
 *  Rene: "problem je ciste v DOOMovi, kde se vubec neukazuje strop a celkove
 *  zlobi grafika". PSX Doom (podle PsyDoom - zpetne rozebrany puvodni kod,
 *  r_plane.cpp / r_segs.cpp / r_sky.cpp / i_main.cpp, puvodni vetve bez
 *  uprav PsyDoom) kresli:
 *    - nebe: SPRT 256 x vyska textury (8 bitu), pred nim DR_MODE se strankou
 *      textury nebe a oknem textury,
 *    - podlahy a STROPY po radcich: kazdy radek = trojuhelnik POLY_FT3
 *      vysoky 1 bod: (L,y) (R,y) (R,y+1), UV (UL,VL) (UR,VR) (UR,VR),
 *      pred plochou okno textury DR_TWIN 64x64 (opakovani textury),
 *    - zdi po sloupcich: kazdy sloupec = POLY_FT3 siroky 1 bod:
 *      (x,yt-1) (x+1,yb+1) (x,yb+1), UV (u,vt) (u,vb) (u,vb),
 *    - postavy POLY_FT4, stavovy radek dole SPRT,
 *    - dva obrazy vedle sebe: kresli se do (0,0) / (256,0), zobrazuje druhy;
 *      kresba zacina vymazanim (isbg), bez ditheringu (dtd = 0).
 *  Tady se to posle do SKUTECNEHO kodu appky (vendor/pcsx_rearmed: gpulib
 *  gpu.c + vout_pl.c + gpu_neon) a vyzvedne se obraz, ktery by sel na
 *  displej (pl_vout_flip) - jednou bez "Enhanced Resolution" (puvodni PS1),
 *  jednou s ni (appka ji ma zapnutou).
 *
 *  Sceny:
 *    doom     - mistnost v PSX Doom (strop pres celou sirku, podlaha, zdi,
 *               postavy, stavovy radek), 6 snimku za sebou (strankovani 0 / 256)
 *    doom_venku - totez venku: nejdriv nebe, strop jen cast, vpravo nahore nebe
 *    tenke    - nahodne trojuhelniky vysoke 1 bod nebo siroke 1 bod "jako Doom"
 *               (s texturou, ktera se napric nemeni; radky, sloupce, spicky,
 *               stinovane, pruhledne) + ctyruhelniky vysoke 1 bod - to, co
 *               oprava B308 meni
 *    nahodne  - primitivy jinych her: normalni trojuhelniky, ctyruhelniky,
 *               sprity, ale i tenke trojuhelniky z 3D siti (textura se napric
 *               meni, nebo bez textury) a pasy trojuhelniku s tenkymi kusy -
 *               tady se NESMI zmenit ani bod
 *
 *  Pouziti: gpu_doom VYSTUP.raw SCENA enh(0|1) texadj(0|1) [pocet_snimku]
 *  Vystup: hlavicka "NAPR" w h flipu (u32 LE) + body 15 bit (u16 LE).
 *  Vyhodnoceni (diry proti puvodnimu PS1, shoda stara/nova verze) dela
 *  kontrola_doom.py.
 * ===================================================================== */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include "frontend/plugin_lib.h"
#include "plugins/gpulib/gpu.h"

long GPUinit(void);
long GPUopen(unsigned long *disp, char *cap, char *cfg);
void GPUwriteStatus(uint32_t data);
void GPUwriteDataMem(uint32_t *mem, int count);
void GPUvBlank(int is_vblank, int lcf);
void GPUrearmedCallbacks(const struct rearmed_cbs *cbs);

struct rearmed_cbs pl_rearmed_cbs;
void SysPrintf(const char *fmt, ...) { va_list ap; va_start(ap, fmt); vfprintf(stderr, fmt, ap); va_end(ap); }

/* ---------------- obraz, ktery by sel na displej ---------------- */
static int out_w, out_h, flips;
static uint16_t out_px[1024 * 1024];
static int vo_open(void) { return 0; }
static void vo_mode(int w, int h, int rw, int rh, int bpp) { (void)rw; (void)rh; (void)bpp; out_w = w; out_h = h; }
static void vo_flip(const void *vram, int ofs, int bgr24, int x, int y, int w, int h, int dims) {
  (void)x; (void)y; (void)dims;
  if (!vram || bgr24) return;
  const uint16_t *s = (const uint16_t *)((const uint8_t *)vram + ofs);
  out_w = w; out_h = h;
  for (int r = 0; r < h; r++) memcpy(&out_px[r * w], &s[r * 1024], (size_t)w * 2);
  flips++;
}
static void vo_close(void) {}
static void *mm(unsigned int size) { void *p = NULL; if (posix_memalign(&p, 4096, size)) return NULL; memset(p, 0, size); return p; }
static void mu(void *p, unsigned int size) { (void)size; free(p); }
static unsigned int frame_cnt, hcnt;

/* ---------------- prikazy pro graficky cip ---------------- */
static uint32_t buf[64];
static void gp1(uint32_t v) { GPUwriteStatus(v); }
static void gp0n(int n) { GPUwriteDataMem(buf, n); }
static void gp0(uint32_t a) { buf[0] = a; gp0n(1); }

static void nahraj(int x, int y, int w, int h, const uint16_t *px) {   /* CPU -> VRAM (LoadImage) */
  static uint32_t d[8 + 1024 * 64];
  int n = 0;
  d[n++] = 0xA0000000u; d[n++] = ((uint32_t)y << 16) | (uint32_t)x; d[n++] = ((uint32_t)h << 16) | (uint32_t)w;
  for (int i = 0; i < w * h; i += 2) d[n++] = (uint32_t)px[i] | ((uint32_t)(i + 1 < w * h ? px[i + 1] : 0) << 16);
  GPUwriteDataMem(d, n);
}

/* nahodna cisla - vlastni, aby x86 i arm64 dostaly presne totez */
static uint32_t nah_st = 12345u;
static uint32_t nah(void) { nah_st ^= nah_st << 13; nah_st ^= nah_st >> 17; nah_st ^= nah_st << 5; return nah_st; }
static int nahr(int a, int b) { return a + (int)(nah() % (uint32_t)(b - a + 1)); }

#define CLUT_X 0
#define CLUT_Y 480
#define CLUT_ID ((CLUT_Y << 6) | (CLUT_X >> 4))
#define CLUT_NEBE ((481 << 6) | 0)
#define CLUT_4 ((482 << 6) | 0)
#define TP_FLAT (8 | (1 << 7))                   /* stranka textur x=512, 8 bitu */
#define TP_ZED  (10 | (1 << 7))                  /* x=640, 8 bitu */
#define TP_NEBE (12 | (1 << 4) | (1 << 7))       /* x=768, y=256, 8 bitu */
#define TP_4    (8 | (1 << 4))                   /* x=512, y=256, 4 bity */
#define TP_16   (14 | (1 << 4) | (2 << 7))       /* x=896, y=256, 15 bitu */

static void tex_okno(int tx, int ty, int w, int h) {             /* DR_TWIN jako PsyQ SetTexWindow */
  gp0(0xE2000000u | ((uint32_t)((ty & 0xff) >> 3) << 15) | ((uint32_t)((tx & 0xff) >> 3) << 10)
      | ((uint32_t)(((~(h - 1)) & 0xff) >> 3) << 5) | (uint32_t)(((~(w - 1)) & 0xff) >> 3));
}
static void poly_ft3(int x0, int y0, int u0, int v0, int x1, int y1, int u1, int v1, int x2, int y2, int u2, int v2,
                     int tpage, uint32_t rgb) {
  buf[0] = 0x24000000u | (rgb & 0xffffff);
  buf[1] = ((uint32_t)(y0 & 0xffff) << 16) | (uint32_t)(x0 & 0xffff);
  buf[2] = ((uint32_t)CLUT_ID << 16) | ((uint32_t)(v0 & 0xff) << 8) | (uint32_t)(u0 & 0xff);
  buf[3] = ((uint32_t)(y1 & 0xffff) << 16) | (uint32_t)(x1 & 0xffff);
  buf[4] = ((uint32_t)tpage << 16) | ((uint32_t)(v1 & 0xff) << 8) | (uint32_t)(u1 & 0xff);
  buf[5] = ((uint32_t)(y2 & 0xffff) << 16) | (uint32_t)(x2 & 0xffff);
  buf[6] = ((uint32_t)(v2 & 0xff) << 8) | (uint32_t)(u2 & 0xff);
  gp0n(7);
}

/* PutDrawEnv(gDrawEnvs[i]): kreslici plocha 256x240 v (ox,0), posun, bez ditheringu, vymazat (isbg) */
static void draw_env(int ox) {
  gp0(0xE1000000u | (1u << 10));                          /* dtd = 0, dfe = 1 */
  gp0(0xE3000000u | (uint32_t)ox);                        /* vlevo nahore */
  gp0(0xE4000000u | (239u << 10) | (uint32_t)(ox + 255)); /* vpravo dole */
  gp0(0xE5000000u | (uint32_t)ox);                        /* posun */
  gp0(0xE2000000u);
  buf[0] = 0x02000000u; buf[1] = (uint32_t)ox; buf[2] = (240u << 16) | 256u; gp0n(3);   /* isbg - cerna */
}

/* ---------------- PSX Doom: nebe (r_sky.cpp R_DrawSky, puvodni vetev) ---------------- */
static void nebe(int u0) {
  gp0(0xE1000000u | TP_NEBE);                             /* DR_MODE: stranka nebe, dfe = 0, dtd = 0 */
  tex_okno(0, 0, 256, 0);                                 /* okno: sirka 256, vyska 0 = bez opakovani */
  buf[0] = 0x65808080u;                                   /* SPRT, textura bez stinovani (SetShadeTex) */
  buf[1] = 0;                                             /* x = 0, y = 0 */
  buf[2] = ((uint32_t)CLUT_NEBE << 16) | (uint32_t)(u0 & 0xff);
  buf[3] = (128u << 16) | 256u;                           /* 256 x 128 */
  gp0n(4);
}

/* ---------------- PSX Doom: plochy (r_plane.cpp R_DrawFlatSpans, puvodni vetev) ---------------- */
static const int32_t YSLOPE[200] = {
  0x147AE,0x14AFD,0x14E5E,0x151D0,0x15555,0x158ED,0x15C98,0x16058,0x1642C,0x16816,0x16C16,0x1702E,0x1745D,0x178A4,0x17D05,0x18181,
  0x18618,0x18ACB,0x18F9C,0x1948B,0x19999,0x19EC8,0x1A41A,0x1A98E,0x1AF28,0x1B4E8,0x1BACF,0x1C0E0,0x1C71C,0x1CD85,0x1D41D,0x1DAE6,
  0x1E1E1,0x1E913,0x1F07C,0x1F81F,0x20000,0x20820,0x21084,0x2192E,0x22222,0x22B63,0x234F7,0x23EE0,0x24924,0x253C8,0x25ED0,0x26A43,
  0x27627,0x28282,0x28F5C,0x29CBC,0x2AAAA,0x2B931,0x2C859,0x2D82D,0x2E8BA,0x2FA0B,0x30C30,0x31F38,0x33333,0x34834,0x35E50,0x3759F,
  0x38E38,0x3A83A,0x3C3C3,0x3E0F8,0x40000,0x42108,0x44444,0x469EE,0x49249,0x4BDA1,0x4EC4E,0x51EB8,0x55555,0x590B2,0x5D174,0x61861,
  0x66666,0x6BCA1,0x71C71,0x78787,0x80000,0x88888,0x92492,0x9D89D,0xAAAAA,0xBA2E8,0xCCCCC,0xE38E3,0x100000,0x124924,0x155555,0x199999,
  0x200000,0x2AAAAA,0x400000,0x800000,0x0,-0x800000,-0x400000,-0x2AAAAA,-0x200000,-0x199999,-0x155555,-0x124924,-0x100000,-0xE38E3,-0xCCCCC,-0xBA2E8,
  -0xAAAAA,-0x9D89D,-0x92492,-0x88888,-0x80000,-0x78787,-0x71C71,-0x6BCA1,-0x66666,-0x61861,-0x5D174,-0x590B2,-0x55555,-0x51EB8,-0x4EC4E,-0x4BDA1,
  -0x49249,-0x469EE,-0x44444,-0x42108,-0x40000,-0x3E0F8,-0x3C3C3,-0x3A83A,-0x38E38,-0x3759F,-0x35E50,-0x34834,-0x33333,-0x31F38,-0x30C30,-0x2FA0B,
  -0x2E8BA,-0x2D82D,-0x2C859,-0x2B931,-0x2AAAA,-0x29CBC,-0x28F5C,-0x28282,-0x27627,-0x26A43,-0x25ED0,-0x253C8,-0x24924,-0x23EE0,-0x234F7,-0x22B63,
  -0x22222,-0x2192E,-0x21084,-0x20820,-0x20000,-0x1F81F,-0x1F07C,-0x1E913,-0x1E1E1,-0x1DAE6,-0x1D41D,-0x1CD85,-0x1C71C,-0x1C0E0,-0x1BACF,-0x1B4E8,
  -0x1AF28,-0x1A98E,-0x1A41A,-0x19EC8,-0x19999,-0x1948B,-0x18F9C,-0x18ACB,-0x18618,-0x18181,-0x17D05,-0x178A4,-0x1745D,-0x1702E,-0x16C16,-0x16816,
  -0x1642C,-0x16058,-0x15C98,-0x158ED,-0x15555,-0x151D0,-0x14E5E,-0x14AFD };

static double view_ang = 0.7;      /* uhel pohledu */
static int32_t view_x = 0x00512345, view_y = 0x00233456;   /* poloha (fixed 16.16) */

/* jeden radek plochy presne jako puvodni R_DrawFlatSpans (vcetne deleni na kusy) */
static void radek_plochy(int y, int L, int R, int planeViewZ, uint32_t rgb) {
  if (L == R) return;
  if (L > R) { int t = L; L = R; R = t; }
  L -= 2; R += 2;
  const int32_t vc = (int32_t)(cos(view_ang) * 65536.0), vs = (int32_t)(sin(view_ang) * 65536.0);
  const int32_t dist = (int32_t)(((int64_t)YSLOPE[y] * planeViewZ) >> 16);
  const int32_t uOff = dist * vc + view_x, vOff = dist * vs + view_y;
  int32_t uStep = dist * vs, vStep = dist * -vc;
  if (uStep < 0) uStep += 128 - 1;
  if (vStep < 0) vStep += 128 - 1;
  uStep /= 128; vStep /= 128;
  int32_t UL = ((L - 128) * uStep + uOff) >> 16, UR = ((R - 128) * uStep + uOff) >> 16;
  int32_t VL = ((L - 128) * vStep + vOff) >> 16, VR = ((R - 128) * vStep + vOff) >> 16;
  if (UL < UR) { int32_t a = UL & ~63; UL -= a; UR -= a; } else { int32_t a = UR & ~63; UR -= a; UL -= a; }
  if (VL < VR) { int32_t a = VL & ~63; VL -= a; VR -= a; } else { int32_t a = VR & ~63; VR -= a; VL -= a; }
  int pieces = 0;
  if (UL > 255 || UR > 255 || VL > 255 || VR > 255) {
    int us = abs(UR - UL) >> 7, vsz = abs(VR - VL) >> 7;
    pieces = us > vsz ? us : vsz;
  }
  if (pieces == 0) {
    poly_ft3(L, y, UL, VL, R, y, UR, VR, R, y + 1, UR, VR, TP_FLAT, rgb);
    return;
  }
  pieces++;
  const int xs = (R - L) / pieces, us = (UR - UL) / pieces, vsp = (VR - VL) / pieces;
  int xl = L, ul = UL, vl = VL;
  for (int p = 0; p < pieces; p++) {
    int xr = xl + xs, ur = ul + us, vr = vl + vsp;
    int ua = 0, va = 0;
    if (ul < ur && ur > 255) { ua = ul & ~127; ul -= ua; ur -= ua; }
    else if (ul > ur && ul > 255) { ua = ur & ~127; ur -= ua; ul -= ua; }
    if (vl < vr && vr > 255) { va = vl & ~127; vl -= va; vr -= va; }
    else if (vl > vr && vl > 255) { va = vr & ~127; vr -= va; vl -= va; }
    poly_ft3(xl, y, ul, vl, xr, y, ur, vr, xr, y + 1, ur, vr, TP_FLAT, rgb);
    xl = xr; ul = ur + ua; vl = vr + va;
  }
}

/* plocha jednoho subsektoru: okno textury 64x64 a radky y0..y1-1 mezi L(y) a R(y) */
static void plocha(int y0, int y1, int l0, int l1, int r0, int r1, int planeViewZ, int tx, int ty) {
  tex_okno(tx, ty, 64, 64);
  for (int y = y0; y < y1; y++) {
    const double f = (y1 - 1 > y0) ? (double)(y - y0) / (y1 - 1 - y0) : 0.0;
    const int L = (int)(l0 + (l1 - l0) * f), R = (int)(r0 + (r1 - r0) * f);
    int sv = 160 - (abs(YSLOPE[y]) >> 14);                 /* svetlo podle vzdalenosti */
    if (sv < 40) sv = 40;
    radek_plochy(y, L, R, planeViewZ, (uint32_t)(sv | (sv << 8) | (sv << 16)));
  }
}

/* ---------------- PSX Doom: zdi (r_segs.cpp R_DrawWallColumns, puvodni vetev) ---------------- */
static void zed(int x0, int x1, int yt0, int yb0, int yt1, int yb1, int u0) {
  tex_okno(0, 0, 64, 128);
  for (int x = x0; x < x1; x++) {
    const double f = (double)(x - x0) / (x1 - x0);
    const int yt = (int)(yt0 + (yt1 - yt0) * f), yb = (int)(yb0 + (yb1 - yb0) * f);
    const int u = (u0 + x * 3) & 63;
    poly_ft3(x, yt - 1, u, 0, x + 1, yb + 1, u, 127, x, yb + 1, u, 127, TP_ZED, 0x707070);
  }
}

/* postava (vec) jako POLY_FT4 + stavovy radek jako SPRT */
static void poly_ft4(int x0, int y0, int x1, int y1, int u0, int v0, int u1, int v1, int tpage, int clut, uint32_t rgb, int pruhl) {
  buf[0] = (pruhl ? 0x2E000000u : 0x2C000000u) | (rgb & 0xffffff);
  buf[1] = ((uint32_t)(y0 & 0xffff) << 16) | (uint32_t)(x0 & 0xffff);
  buf[2] = ((uint32_t)clut << 16) | ((uint32_t)(v0 & 0xff) << 8) | (uint32_t)(u0 & 0xff);
  buf[3] = ((uint32_t)(y0 & 0xffff) << 16) | (uint32_t)(x1 & 0xffff);
  buf[4] = ((uint32_t)tpage << 16) | ((uint32_t)(v0 & 0xff) << 8) | (uint32_t)(u1 & 0xff);
  buf[5] = ((uint32_t)(y1 & 0xffff) << 16) | (uint32_t)(x0 & 0xffff);
  buf[6] = ((uint32_t)(v1 & 0xff) << 8) | (uint32_t)(u0 & 0xff);
  buf[7] = ((uint32_t)(y1 & 0xffff) << 16) | (uint32_t)(x1 & 0xffff);
  buf[8] = ((uint32_t)(v1 & 0xff) << 8) | (uint32_t)(u1 & 0xff);
  gp0n(9);
}
static void sprt(int x, int y, int w, int h, int u, int v, int clut, uint32_t cmd_rgb) {
  buf[0] = cmd_rgb;
  buf[1] = ((uint32_t)(y & 0xffff) << 16) | (uint32_t)(x & 0xffff);
  buf[2] = ((uint32_t)clut << 16) | ((uint32_t)(v & 0xff) << 8) | (uint32_t)(u & 0xff);
  buf[3] = ((uint32_t)h << 16) | (uint32_t)w;
  gp0n(4);
}

static void scena_doom(int fr, int venku) {
  if (venku) {
    /* venku: nebe (R_DrawSky se kresli prvni), strop jen vlevo, vpravo nahore nebe */
    nebe((int)(-view_ang * 40.0));
    plocha(0, 60, 0, 30, 150, 170, 64, 64, 64);
    plocha(60, 100, 30, 0, 170, 256, 80, 0, 64);
  } else {
    /* mistnost: strop pres celou sirku ze tri subsektoru, za nim jen cerne pozadi */
    plocha(0, 60, 0, 0, 130, 150, 64, 64, 64);
    plocha(0, 60, 130, 150, 256, 256, 72, 0, 64);
    plocha(60, 100, 0, 0, 256, 256, 80, 0, 64);
  }
  /* podlaha: tri subsektory */
  plocha(100, 140, 0, 0, 120, 140, -41, 128, 0);
  plocha(100, 140, 120, 140, 256, 256, -41, 192, 64);
  plocha(140, 200, 0, 0, 256, 256, -50, 0, 128);
  /* zdi */
  zed(96, 160, 70, 130, 60, 140, fr * 5);
  zed(10, 60, 40, 150, 55, 135, 17);
  zed(200, 250, 50, 145, 30, 160, 33);
  /* postavy (vec, pruhledny prizrak) */
  gp0(0xE2000000u);
  poly_ft4(110, 95, 150, 160, 0, 0, 63, 127, TP_ZED, CLUT_ID, 0x808080, 0);
  poly_ft4(30, 80, 70, 150, 0, 0, 63, 63, TP_FLAT, CLUT_ID, 0x606060, 1);
  /* stavovy radek: SPRT 256x40 (4 bity) a cislice */
  gp0(0xE1000000u | (1u << 10) | TP_4);
  sprt(0, 200, 256, 40, 0, 0, CLUT_4, 0x65808080u);
  for (int i = 0; i < 3; i++) sprt(20 + i * 12, 210, 11, 16, 16 * i, 64, CLUT_4, 0x64808080u);
}

/* ---------------- trojuhelnik / ctyruhelnik s libovolnymi atributy ---------------- */
typedef struct { int x, y, u, v; uint32_t c; } vrchol;

#define XY(i) (((uint32_t)(p[i].y & 0xffff) << 16) | (uint32_t)(p[i].x & 0xffff))
#define UV(i) (((uint32_t)(p[i].v & 0xff) << 8) | (uint32_t)(p[i].u & 0xff))
static void troj3(const vrchol *p, int stin, int tex, int pruhl, int tp) {
  const uint32_t clut = (uint32_t)(tp == TP_4 ? CLUT_4 : CLUT_ID) << 16, tpg = (uint32_t)tp << 16;
  const uint32_t semi = pruhl ? 0x02000000u : 0;
  if (!stin && !tex) {
    buf[0] = 0x20000000u | semi | p[0].c; buf[1] = XY(0); buf[2] = XY(1); buf[3] = XY(2); gp0n(4);
  } else if (stin && !tex) {
    buf[0] = 0x30000000u | semi | p[0].c; buf[1] = XY(0); buf[2] = p[1].c; buf[3] = XY(1); buf[4] = p[2].c; buf[5] = XY(2);
    gp0n(6);
  } else if (!stin) {
    buf[0] = 0x24000000u | semi | p[0].c; buf[1] = XY(0); buf[2] = clut | UV(0); buf[3] = XY(1); buf[4] = tpg | UV(1);
    buf[5] = XY(2); buf[6] = UV(2); gp0n(7);
  } else {
    buf[0] = 0x34000000u | semi | p[0].c; buf[1] = XY(0); buf[2] = clut | UV(0); buf[3] = p[1].c; buf[4] = XY(1);
    buf[5] = tpg | UV(1); buf[6] = p[2].c; buf[7] = XY(2); buf[8] = UV(2); gp0n(9);
  }
}
static void ctyr4(const vrchol *p, int stin, int pruhl, int tp) {          /* s texturou */
  const uint32_t clut = (uint32_t)(tp == TP_4 ? CLUT_4 : CLUT_ID) << 16, tpg = (uint32_t)tp << 16;
  const uint32_t semi = pruhl ? 0x02000000u : 0;
  if (!stin) {
    buf[0] = 0x2C000000u | semi | p[0].c; buf[1] = XY(0); buf[2] = clut | UV(0); buf[3] = XY(1); buf[4] = tpg | UV(1);
    buf[5] = XY(2); buf[6] = UV(2); buf[7] = XY(3); buf[8] = UV(3); gp0n(9);
  } else {
    buf[0] = 0x3C000000u | semi | p[0].c; buf[1] = XY(0); buf[2] = clut | UV(0); buf[3] = p[1].c; buf[4] = XY(1);
    buf[5] = tpg | UV(1); buf[6] = p[2].c; buf[7] = XY(2); buf[8] = UV(2); buf[9] = p[3].c; buf[10] = XY(3);
    buf[11] = UV(3); gp0n(12);
  }
}
#undef XY
#undef UV

static uint32_t barva(void) { return 0x202020u | (nah() & 0x7f7f7fu); }
static int nahodna_stranka(void) { const uint32_t a = nah(); return (a & 1) ? TP_FLAT : (a & 2) ? TP_4 : TP_16; }

/* Stejna podminka jako oprava B308 v psx_gpu_parse.c (do_thin_triangle_enhanced):
 * trojuhelnik s texturou vysoky 1 bod (nebo siroky 1 bod), ve kterem se textura
 * (a u stinovaneho i barva) napric tenkym smerem nemeni = "Doom". */
static int stejne_napric(int f0, int f1, int f2, int d1, int d2) { return (f1 - f0) * d2 == (f2 - f0) * d1; }
static int jako_doom(const vrchol *p, int stin, int tex) {
  if (!tex) return 0;
  int ya = p[0].y, yb = p[0].y, xa = p[0].x, xb = p[0].x;
  for (int k = 1; k < 3; k++) {
    if (p[k].y < ya) ya = p[k].y;
    if (p[k].y > yb) yb = p[k].y;
    if (p[k].x < xa) xa = p[k].x;
    if (p[k].x > xb) xb = p[k].x;
  }
  int radky;
  if (yb - ya == 1) radky = 1;
  else if (xb - xa == 1) radky = 0;
  else return 0;
  const int d1 = radky ? p[1].x - p[0].x : p[1].y - p[0].y, d2 = radky ? p[2].x - p[0].x : p[2].y - p[0].y;
  if (!stejne_napric(p[0].u, p[1].u, p[2].u, d1, d2) || !stejne_napric(p[0].v, p[1].v, p[2].v, d1, d2)) return 0;
  if (stin) {
    for (int s = 0; s < 24; s += 8)
      if (!stejne_napric((int)(p[0].c >> s) & 255, (int)(p[1].c >> s) & 255, (int)(p[2].c >> s) & 255, d1, d2)) return 0;
  }
  return 1;
}
static void kopie_atr(vrchol *kam, const vrchol *odkud) { kam->u = odkud->u; kam->v = odkud->v; kam->c = odkud->c; }

/* ---------------- tenke primitivy jako v Doomu (to, co B308 opravuje) ---------------- */
static void scena_tenke(void) {
  gp0(0xE2000000u);
  for (int i = 0; i < 600; i++) {
    const int stin = (int)(nah() & 1), pruhl = (nah() % 5) == 0, druh = (int)(nah() % 6), tp = nahodna_stranka();
    const int dl = nahr(1, 120);
    const int x = nahr(-20, 250), y = nahr(-5, 236);
    vrchol p[4];
    for (int k = 0; k < 4; k++) { p[k].u = nahr(0, 255); p[k].v = nahr(0, 255); p[k].c = barva(); }
    switch (druh) {
      case 0:   /* Doom plocha: (L,y) (R,y) (R,y+1), spodni bod ma UV praveho */
        p[0].x = x; p[0].y = y; p[1].x = x + dl; p[1].y = y; p[2].x = x + dl; p[2].y = y + 1; kopie_atr(&p[2], &p[1]); break;
      case 1:   /* radek, spodni bod pod levym */
        p[0].x = x; p[0].y = y; p[1].x = x + dl; p[1].y = y; p[2].x = x; p[2].y = y + 1; kopie_atr(&p[2], &p[0]); break;
      case 2:   /* radek spickou nahoru (PS1 z nej nekresli nic) */
        p[0].x = x; p[0].y = y + 1; p[1].x = x + dl; p[1].y = y + 1; p[2].x = x; p[2].y = y; kopie_atr(&p[2], &p[0]); break;
      case 3:   /* Doom zed: (x,yt-1) (x+1,yb+1) (x,yb+1), dolni hrana stejne UV */
        p[0].x = x; p[0].y = y - 1; p[1].x = x + 1; p[1].y = y + dl; p[2].x = x; p[2].y = y + dl; kopie_atr(&p[2], &p[1]);
        p[0].u = p[1].u; break;
      case 4:   /* sloupec, horni hrana siroka 1 bod (vysoky aspon 2 - 1x1 je radek) */
        p[0].x = x; p[0].y = y; p[1].x = x + 1; p[1].y = y; p[2].x = x; p[2].y = y + (dl < 2 ? 2 : dl);
        kopie_atr(&p[1], &p[0]); break;
      default:  /* ctyruhelnik vysoky 1 bod, napric stejne UV */
        p[0].x = x; p[0].y = y; p[1].x = x + dl; p[1].y = y; p[2].x = x; p[2].y = y + 1; p[3].x = x + dl; p[3].y = y + 1;
        kopie_atr(&p[2], &p[0]); kopie_atr(&p[3], &p[1]);
        ctyr4(p, stin, pruhl, tp);
        continue;
    }
    if (!jako_doom(p, stin, 1)) { fprintf(stderr, "chyba sceny: tenky trojuhelnik druh %d neni jako Doom\n", druh); exit(3); }
    troj3(p, stin, 1, pruhl, tp);
  }
}

/* ---------------- nahodne primitivy jinych her (tady se NESMI nic zmenit) ---------------- */
static int rozmer_ok(int x0, int y0, int x1, int y1, int x2, int y2) {
  int xa = x0 < x1 ? x0 : x1; xa = xa < x2 ? xa : x2;
  int xb = x0 > x1 ? x0 : x1; xb = xb > x2 ? xb : x2;
  int ya = y0 < y1 ? y0 : y1; ya = ya < y2 ? ya : y2;
  int yb = y0 > y1 ? y0 : y1; yb = yb > y2 ? yb : y2;
  return xb - xa >= 2 && yb - ya >= 2;
}

/* Pozor: kazde nah() zvlast do promenne - poradi vyhodnoceni argumentu funkce
 * neni v C dane (x86 a arm64 prekladac ho maji jinak -> jina scena). */
static int tenkych_jinych = 0;
static void scena_nahodne(void) {
  for (int i = 0; i < 700; i++) {
    const int co = (int)(nah() % 14);
    if ((nah() % 7) == 0) {                          /* obcas jine okno textury / pruhlednost */
      if (nah() & 1) {
        const int tx = nahr(0, 7) * 8, ty = nahr(0, 7) * 8;
        const int tw = 32 << (nah() % 3), th = 32 << (nah() % 3);
        tex_okno(tx, ty, tw, th);
      } else gp0(0xE2000000u);
      const uint32_t abr = nah() & 3, dith = nah() & 1;
      gp0(0xE1000000u | (1u << 10) | (abr << 5) | TP_FLAT | (dith << 9));
    }
    if (co < 6) {                                    /* trojuhelnik aspon 2 x 2 */
      vrchol p[3];
      const int r = nahr(2, 120);
      do {
        p[0].x = nahr(-30, 260); p[0].y = nahr(-30, 250);
        p[1].x = p[0].x + nahr(-r, r); p[1].y = p[0].y + nahr(-r, r);
        p[2].x = p[0].x + nahr(-r, r); p[2].y = p[0].y + nahr(-r, r);
      } while (!rozmer_ok(p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y));
      for (int k = 0; k < 3; k++) { p[k].u = nahr(0, 255); p[k].v = nahr(0, 255); p[k].c = barva(); }
      const int stin = (int)(nah() & 1), tex = (int)(nah() & 1), pruhl = (nah() % 4) == 0, tp = nahodna_stranka();
      troj3(p, stin, tex, pruhl, tp);
    } else if (co < 9) {                             /* tenky trojuhelnik "z 3D site" (textura se meni napric) */
      vrchol p[3];
      const int stin = (int)(nah() & 1), tex = (int)(nah() & 1), pruhl = (nah() % 4) == 0, tp = nahodna_stranka();
      const int x = nahr(-20, 250), y = nahr(-5, 236), dl = nahr(1, 100), tvar = (int)(nah() % 4);
      for (int k = 0; k < 3; k++) { p[k].u = nahr(0, 255); p[k].v = nahr(0, 255); p[k].c = barva(); }
      if (tvar == 0) { p[0].x = x; p[0].y = y; p[1].x = x + dl; p[1].y = y; p[2].x = x + nahr(0, dl); p[2].y = y + 1; }
      else if (tvar == 1) { p[0].x = x; p[0].y = y + 1; p[1].x = x + dl; p[1].y = y + 1; p[2].x = x + nahr(-5, dl + 5); p[2].y = y; }
      else if (tvar == 2) { p[0].x = x; p[0].y = y; p[1].x = x + 1; p[1].y = y + nahr(0, dl); p[2].x = x; p[2].y = y + dl; }
      else { p[0].x = x + 1; p[0].y = y; p[1].x = x; p[1].y = y + nahr(-3, 3); p[2].x = x + 1; p[2].y = y + dl; }
      if (jako_doom(p, stin, tex)) continue;         /* nahodou "jako Doom" - tahle scena je o ostatnich */
      tenkych_jinych++;
      troj3(p, stin, tex, pruhl, tp);
    } else if (co < 11) {                            /* ctyruhelnik (2 trojuhelniky) */
      const int x0 = nahr(-30, 250), y0 = nahr(-30, 230), w = nahr(2, 100), h = nahr(2, 100);
      const int sk = nahr(-4, 4), dy = nahr(-3, 3);
      const int X[4] = {x0, x0 + w, x0 + sk, x0 + w + sk}, Y[4] = {y0, y0 + dy, y0 + h, y0 + h};
      if (!rozmer_ok(X[0], Y[0], X[1], Y[1], X[2], Y[2]) || !rozmer_ok(X[1], Y[1], X[2], Y[2], X[3], Y[3])) continue;
      const int tp = (nah() & 1) ? TP_FLAT : TP_16;
      const uint32_t semi = (nah() % 4) == 0 ? 0x02000000u : 0;
      const uint32_t b0 = 0x404040u | (nah() & 0x7f7f7fu);
      buf[0] = 0x2C000000u | semi | b0;
      for (int k = 0; k < 4; k++) {
        buf[1 + k * 2] = ((uint32_t)(Y[k] & 0xffff) << 16) | (uint32_t)(X[k] & 0xffff);
        const int u = (k & 1) ? 63 + nahr(0, 60) : nahr(0, 60);
        const int v = (k & 2) ? 63 + nahr(0, 60) : nahr(0, 60);
        uint32_t extra = k == 0 ? ((uint32_t)CLUT_ID << 16) : k == 1 ? ((uint32_t)tp << 16) : 0;
        buf[2 + k * 2] = extra | ((uint32_t)v << 8) | (uint32_t)u;
      }
      gp0n(9);
    } else if (co < 12) {                            /* pas trojuhelniku (sit) s tenkymi kusy */
      const int x0 = nahr(-20, 200), y0 = nahr(0, 230), n = nahr(4, 30), tp = nahodna_stranka();
      const int stin = (int)(nah() & 1), tex = (int)(nah() & 1);
      vrchol s[64];
      for (int k = 0; k < n + 2; k++) {
        s[k].x = x0 + k * nahr(1, 6); s[k].y = y0 + ((k & 1) ? nahr(0, 2) : 0);
        s[k].u = (s[k].x * 3 + s[k].y * 5) & 255; s[k].v = (s[k].y * 7 + s[k].x) & 255; s[k].c = barva();
      }
      for (int k = 0; k < n; k++) {
        vrchol p[3] = {s[k], s[k + 1], s[k + 2]};
        if (jako_doom(p, stin, tex)) continue;
        if (!rozmer_ok(p[0].x, p[0].y, p[1].x, p[1].y, p[2].x, p[2].y)) tenkych_jinych++;
        troj3(p, stin, tex, 0, tp);
      }
    } else {                                         /* sprite */
      const int w = nahr(2, 90), h = nahr(2, 90);
      const int tp = (nah() & 1) ? TP_FLAT : TP_4;
      gp0(0xE1000000u | (1u << 10) | (uint32_t)tp);
      const int x = nahr(-20, 250), y = nahr(-20, 230), u = nahr(0, 255), v = nahr(0, 255);
      const int clut = (nah() & 1) ? CLUT_ID : CLUT_4;
      const uint32_t cmd = ((nah() & 1) ? 0x64000000u : 0x66000000u) | 0x808080u;
      sprt(x, y, w, h, u, v, clut, cmd);
    }
  }
}

int main(int argc, char **argv) {
  const char *vystup = argc > 1 ? argv[1] : "vystup.raw";
  const char *scena = argc > 2 ? argv[2] : "doom";
  const int enh = argc > 3 ? atoi(argv[3]) : 1;
  const int texadj = argc > 4 ? atoi(argv[4]) : 1;

  memset(&pl_rearmed_cbs, 0, sizeof(pl_rearmed_cbs));
  pl_rearmed_cbs.pl_vout_open = vo_open;
  pl_rearmed_cbs.pl_vout_set_mode = vo_mode;
  pl_rearmed_cbs.pl_vout_flip = vo_flip;
  pl_rearmed_cbs.pl_vout_close = vo_close;
  pl_rearmed_cbs.mmap = mm;
  pl_rearmed_cbs.munmap = mu;
  pl_rearmed_cbs.gpu_frame_count = &frame_cnt;
  pl_rearmed_cbs.gpu_hcnt = &hcnt;
  pl_rearmed_cbs.dithering = 1;
  pl_rearmed_cbs.thread_rendering = 0;
  pl_rearmed_cbs.gpu_neon.allow_interlace = 2;
  pl_rearmed_cbs.gpu_neon.enhancement_enable = enh;
  pl_rearmed_cbs.gpu_neon.enhancement_no_main = 0;
  pl_rearmed_cbs.gpu_neon.enhancement_tex_adj = texadj;

  GPUinit();
  GPUrearmedCallbacks(&pl_rearmed_cbs);
  GPUopen(NULL, NULL, NULL);
  gp1(0x00000000);                                       /* reset */
  gp1(0x06000000u | (0xC60u << 12) | 0x260u);            /* vodorovny rozsah */
  gp1(0x07000000u | (256u << 10) | 16u);                 /* svisly rozsah 240 radku */
  gp1(0x08000000u);                                      /* 256x240 NTSC 15 bit */
  gp1(0x03000000u);                                      /* displej zapnout */

  /* palety: 256 barev, zadna 0 (pruhledna) */
  uint16_t clut[256];
  for (int i = 0; i < 256; i++) clut[i] = (uint16_t)(0x8000 | (((i * 7) & 31)) | (((i * 3 + 9) & 31) << 5) | (((31 - i) & 31) << 10));
  nahraj(CLUT_X, CLUT_Y, 256, 1, clut);
  for (int i = 0; i < 256; i++) clut[i] = (uint16_t)(((i >> 3) + 4) << 10 | ((i >> 4) + 8) << 5 | ((i >> 5) + 6) | (i ? 0 : 0x8000));
  nahraj(0, 481, 256, 1, clut);
  for (int i = 0; i < 16; i++) clut[i] = (uint16_t)(0x8000 | ((i * 2) & 31) | ((31 - i) << 5) | ((i * 5) & 31) << 10);
  nahraj(0, 482, 16, 1, clut);
  /* 16 ploch 64x64 (8 bitu) na strance x=512 a zed 64x128 na strance x=640 */
  static uint16_t tex[128 * 256];
  for (int v = 0; v < 256; v++)
    for (int u = 0; u < 256; u += 2) {
      int a = 1 + (((u >> 3) ^ (v >> 3)) & 1) * 100 + ((u >> 6) + (v >> 6) * 4) * 8;
      int b = 1 + ((((u + 1) >> 3) ^ (v >> 3)) & 1) * 100 + (((u + 1) >> 6) + (v >> 6) * 4) * 8;
      tex[v * 128 + u / 2] = (uint16_t)((a & 255) | ((b & 255) << 8));
    }
  nahraj(512, 0, 128, 256, tex);
  static uint16_t zt[32 * 128];
  for (int v = 0; v < 128; v++)
    for (int u = 0; u < 64; u += 2) zt[v * 32 + u / 2] = (uint16_t)((2 + ((v >> 2) & 63)) | ((2 + ((v >> 2) & 63) + 64) << 8));
  nahraj(640, 0, 32, 128, zt);
  /* nebe 256x128 (8 bitu) na x=768, y=256 */
  static uint16_t nb[128 * 128];
  for (int v = 0; v < 128; v++)
    for (int u = 0; u < 256; u += 2) {
      int a = 1 + ((u * 3 + v * 2 + ((u >> 4) * 37 % 23)) & 255);
      int b = 1 + (((u + 1) * 3 + v * 2 + (((u + 1) >> 4) * 37 % 23)) & 255);
      nb[v * 128 + u / 2] = (uint16_t)((a > 255 ? 255 : a) | ((b > 255 ? 255 : b) << 8));
    }
  nahraj(768, 256, 128, 128, nb);
  /* 4 bity: stavovy radek a cislice na x=512, y=256 (64 halfwordu = 256 texelu) */
  static uint16_t t4[64 * 128];
  for (int v = 0; v < 128; v++)
    for (int u = 0; u < 64; u++) {
      uint16_t w = 0;
      for (int k = 0; k < 4; k++) w |= (uint16_t)((1 + ((u * 4 + k) / 5 + v / 3) % 15) << (k * 4));
      t4[v * 64 + u] = w;
    }
  nahraj(512, 256, 64, 128, t4);
  /* 15 bitu na x=896, y=256 */
  static uint16_t t16[64 * 256];
  for (int v = 0; v < 256; v++)
    for (int u = 0; u < 64; u++) t16[v * 64 + u] = (uint16_t)(0x8000 | (u >> 1) | ((v >> 3) << 5) | (((u ^ v) & 31) << 10));
  nahraj(896, 256, 64, 256, t16);

  /* nekolik snimku (rezim "Enhanced" se zapina az od druheho) */
  int cur = 0;
  const int doom = strncmp(scena, "doom", 4) == 0;
  const int snimku = argc > 5 ? atoi(argv[5]) : doom ? 6 : 3;   /* 5. argument: pocet snimku (mereni rychlosti) */
  for (int fr = 0; fr < snimku; fr++) {
    const int ox = cur ? 256 : 0;
    nah_st = 12345u + (uint32_t)fr * 7919u;
    draw_env(ox);
    if (doom) scena_doom(fr, strcmp(scena, "doom_venku") == 0);
    else if (strcmp(scena, "tenke") == 0) scena_tenke();
    else scena_nahodne();
    gp1(0x05000000u | (uint32_t)ox);      /* zobrazit, co se nakreslilo */
    frame_cnt++;
    GPUvBlank(1, 0);
    GPUvBlank(0, 0);
    cur ^= 1;
    view_ang += 0.05;
  }

  FILE *f = fopen(vystup, "wb");
  if (!f) { fprintf(stderr, "nejde zapsat %s\n", vystup); return 2; }
  const uint32_t hl[4] = {0x5250414eu /* "NAPR" */, (uint32_t)out_w, (uint32_t)out_h, (uint32_t)flips};
  fwrite(hl, 4, 4, f);
  fwrite(out_px, 2, (size_t)out_w * out_h, f);
  fclose(f);
  printf("scena=%s enh=%d texadj=%d vystup %dx%d (flipu %d) tenkych_jinych=%d -> %s\n", scena, enh, texadj, out_w, out_h,
         flips, tenkych_jinych, vystup);
  return 0;
}
