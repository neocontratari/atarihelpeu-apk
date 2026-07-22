// core_demo.c - DOCASNE "jadro": zkusebni vzor za stejnou zasuvkou,
// jakou pouzije skutecne jadro (Sega / PS1). Kazdych ~7 vterin prepne
// rozliseni a format, aby se nazivo overila automaticka prestavba
// textury a letterboxu - presne to, co delaji skutecne hry:
//   256x224 RGBA (Sega NTSC)  ->  320x240 RGBA (Sega/PS1)
//   -> 512x240 RGB565 (PS1 hi-res) -> 640x480 XRGB (PS1 menu/BIOS)
// Po napojeni skutecneho jadra se tenhle soubor cely nahradi.

#include "core_api.h"
#include <string.h>

#define MAX_W 640
#define MAX_H 480

static unsigned char s_base[MAX_W * MAX_H * 4]; // staticky podklad vzoru
static unsigned char s_fb[MAX_W * MAX_H * 4];   // "framebuffer jadra"
static long       s_frame   = 0;
static int        s_stage   = -1;
static int        s_w = 0, s_h = 0;
static CoreFormat s_fmt     = CORE_FMT_RGBA8888;
static int        s_prev_lx = 0, s_prev_ly = 0;

static const struct { int w, h; CoreFormat fmt; } STAGES[4] = {
    {256, 224, CORE_FMT_RGBA8888},
    {320, 240, CORE_FMT_RGBA8888},
    {512, 240, CORE_FMT_RGB565},
    {640, 480, CORE_FMT_XRGB8888},
};

static int bpp(CoreFormat f) { return (f == CORE_FMT_RGB565) ? 2 : 4; }

static void put_px(unsigned char* buf, int x, int y,
                   unsigned char r, unsigned char g, unsigned char b) {
    if (s_fmt == CORE_FMT_RGB565) {
        unsigned short v = (unsigned short)(((r >> 3) << 11) |
                                            ((g >> 2) << 5)  |
                                             (b >> 3));
        ((unsigned short*)(void*)buf)[y * s_w + x] = v;
    } else if (s_fmt == CORE_FMT_XRGB8888) {
        unsigned char* p = buf + (size_t)(y * s_w + x) * 4;  // B,G,R,X
        p[0] = b; p[1] = g; p[2] = r; p[3] = 255;
    } else {
        unsigned char* p = buf + (size_t)(y * s_w + x) * 4;  // R,G,B,A
        p[0] = r; p[1] = g; p[2] = b; p[3] = 255;
    }
}

static void copy_px_from_base(int x, int y) {
    int n = bpp(s_fmt);
    memcpy(s_fb   + (size_t)(y * s_w + x) * (size_t)n,
           s_base + (size_t)(y * s_w + x) * (size_t)n, (size_t)n);
}

// Smaze stare linky (obnovi podklad) a nakresli nove bile.
// Za snimek se tak sahne jen na par tisic pixelu - u skutecneho jadra
// tuhle praci dela emulace sama.
static void draw_line_cross(int lx, int ly, int erase_lx, int erase_ly) {
    for (int x = 0; x < s_w; x++) copy_px_from_base(x, erase_ly);
    for (int y = 0; y < s_h; y++) copy_px_from_base(erase_lx, y);
    for (int x = 0; x < s_w; x++) put_px(s_fb, x, ly, 255, 255, 255);
    for (int y = 0; y < s_h; y++) put_px(s_fb, lx, y, 255, 255, 255);
}

static void stage_init(int stage) {
    s_stage = stage;
    s_w     = STAGES[stage].w;
    s_h     = STAGES[stage].h;
    s_fmt   = STAGES[stage].fmt;

    static const unsigned char bars[8][3] = {
        {235,235,235}, {235,235, 16}, { 16,235,235}, { 16,235, 16},
        {235, 16,235}, {235, 16, 16}, { 16, 16,235}, { 16, 16, 16},
    };
    for (int y = 0; y < s_h; y++) {
        for (int x = 0; x < s_w; x++) {
            const unsigned char* c = bars[(x * 8) / s_w];
            unsigned char r = c[0], g = c[1], b = c[2];
            if (x < 32 && y < 32 && (((x >> 2) + (y >> 2)) & 1)) {
                r = 0; g = 0; b = 0;  // sachovnice = kontrola ostrosti
            }
            put_px(s_base, x, y, r, g, b);
        }
    }
    memcpy(s_fb, s_base, (size_t)(s_w * s_h * bpp(s_fmt)));
    s_prev_lx = 0;
    s_prev_ly = 0;
}

void demo_step(void) {
    int stage = (int)((s_frame / 420) % 4);   // ~7 vterin na kazde rozliseni
    if (stage != s_stage) stage_init(stage);

    int lx = (int)( s_frame      % s_w);
    int ly = (int)((s_frame * 2) % s_h);
    draw_line_cross(lx, ly, s_prev_lx, s_prev_ly);
    s_prev_lx = lx;
    s_prev_ly = ly;
    s_frame++;
}

bool demo_get_frame(CoreFrame* out) {
    if (s_stage < 0) return false;
    out->pixels = s_fb;
    out->width  = s_w;
    out->height = s_h;
    out->pitch  = s_w * bpp(s_fmt);  // radky tesne za sebou
    out->format = s_fmt;
    return true;
}
