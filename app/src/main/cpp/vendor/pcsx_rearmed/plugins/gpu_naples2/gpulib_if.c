/*
 * gpulib_if.c - spojka mezi gpulib a GLES2 rendererem NaPles2.
 *
 * Nahrazuje puvodni plugin gpu-gles (OpenGL ES 1). Rozdily:
 *   - vse v GLES2, takze kontext lze sdilet s eglrenderem (GLES1 to Mali
 *     odmitalo, odtud puvodni cesta pres glReadPixels a procesor),
 *   - kresli se primo v souradnicich VRAM 1:1 - zadne odecitani pozice
 *     displeje, ktere v gpu-gles posouvalo geometrii i orez mimo platno,
 *   - zapisy z procesoru do VRAM (logo BIOSu, snimky filmu) jdou rovnou
 *     do obrazu, takze intro i film nepotrebuji zadnou zvlastni cestu.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include "../gpulib/gpu.h"
#include "naples2_gl.h"

/* PCSX-ReARMed konvence: plugin do sebe vtahuje jadro gpulib. Odtud pochazi
   globalni 'gpu', tabulka 'cmd_lengths' i cele rozhrani GPUinit/GPUopen/...,
   ktere pouziva frontend/plugin.c. Stary gpu-gles delal totez - kdyz to tu
   chybelo, linker hlasil desitky nedefinovanych symbolu. */
#include "../gpulib/gpu.c"

extern void nap_diag_log(const char *fmt, ...);

/* renderer si sahá na autoritativni VRAM v pameti */
unsigned short *n2_host_vram(void) { return (unsigned short *)gpu.vram; }

/* ------------------------------------------------------------- rozhrani */

int renderer_init(void)
{
    /* Zadne GL volani tady! gpulib nas muze zavolat driv, nez vubec existuje
       EGL kontext. Skutecnou inicializaci (shadery, FBO, textury) provede
       n2_init() az z nap_gles_egl_init(), kdyz je kontext hotovy. Do te doby
       vsechny kreslici funkce bezpecne nic nedelaji (kontrola n2.ready). */
    return 0;
}

void renderer_finish(void) { n2_finish(); }

void renderer_sync_ecmds(uint32_t *ecmds) { n2_set_ecmds(ecmds); }

void renderer_update_caches(int x, int y, int w, int h, int state_changed)
{
    (void)state_changed;
    n2_vram_written(x, y, w, h);   /* CPU zapsal do VRAM -> na GPU i do obrazu */
}

void renderer_flush_queues(void) { n2_flush(); }

void renderer_set_interlace(int enable, int is_odd)
{
    (void)enable; (void)is_odd;    /* kreslime oba pulsnimky do teze VRAM */
}

void renderer_set_config(const struct rearmed_cbs *cbs) { (void)cbs; }

void renderer_notify_screen_change(const struct psx_gpu_screen *screen) { (void)screen; }

int renderer_do_cmd_list(uint32_t *list, int list_len, uint32_t *ex_regs,
                         int *cycles_sum, int *cycles_last, int *last_cmd)
{
    int used;
    (void)cycles_sum; (void)cycles_last;
    used = n2_do_cmd_list(list, list_len, ex_regs, last_cmd);
    return used;
}

/* --------------------------------------------------------------- vystup */

int  vout_init(void)   { return 0; }
int  vout_finish(void) { return 0; }
void vout_blank(struct psx_gpu *g) { (void)g; }
void vout_set_config(const struct rearmed_cbs *cbs) { (void)cbs; }

int vout_update(struct psx_gpu *g, int src_x, int src_y)
{
    (void)g; (void)src_x; (void)src_y;
    n2_flush();
    return 0;
}

/* gpulib chce cist VRAM z pameti (GP0 C0h, save state) - musime mu tam
   nejdriv dostat to, co nakreslila GPU. */
void vout_set_raw_vram(void *vram)
{
    (void)vram;
}

/* GPUopen/GPUclose definuje plugin, ne gpulib. V nasem libretro toku se
   nevolaji (kontext i renderer si zaridi nap_gles_egl_init -> n2_init), ale
   frontend/plugin.c na ne odkazuje v tabulce funkci - musi tedy existovat. */
static int n2_is_opened;

long GPUopen(unsigned long *disp, char *cap, char *cfg)
{
    (void)disp; (void)cap; (void)cfg;
    if (n2_is_opened) return -1;
    n2_is_opened = 1;
    return 0;
}

long GPUclose(void)
{
    if (!n2_is_opened) return 0;
    n2_is_opened = 0;
    return 0;
}

/* ------------------------------------- rozhrani, ktere ceka zbytek emu10 */

int nap_gles_vram_w(void) { return N2_VRAM_W; }
int nap_gles_vram_h(void) { return N2_VRAM_H; }

void nap_gles_sync_display_settings(void)
{
    n2_flush();   /* nic dalsiho neni treba - kreslime rovnou v souradnicich VRAM */
}

/* Cesta pres procesor (kdyz kontext neni sdileny): precti oblast displeje. */
const void *nap_gles_grab_pixels(int *out_w, int *out_h)
{
    int sx = gpu.screen.src_x, sy = gpu.screen.src_y;
    int w  = gpu.screen.hres,  h  = gpu.screen.vres;
    const void *px;
    if (w <= 0) w = 320;
    if (h <= 0) h = 240;

    px = n2_read_display(sx, sy, w, h);
    if (!px) return NULL;
    if (out_w) *out_w = w;
    if (out_h) *out_h = h;
    {
        static long n = 0;
        if (++n % 180 == 1) {
            /* Rozhodujici merení: barva pixelu ze STREDU obrazu. Kdyz je tu
               neco jineho nez cerna, obraz se vykreslil a problem je az pri
               zobrazeni. Kdyz je cerna, nevykreslil se vubec. */
            const unsigned char *p = (const unsigned char *)px;
            size_t mid = ((size_t)(h / 2) * (size_t)w + (size_t)(w / 2)) * 4;
            nap_diag_log("NAPLES2 OBRAZ %dx%d z VRAM src=[%d,%d] stred=R%u G%u B%u",
                         w, h, sx, sy, p[mid], p[mid + 1], p[mid + 2]);
        }
    }
    return px;
}

/* Volano presne 1x za tick po retro_run (po VBlanku emulovaneho PS1).
   Krome dokresleni davky tady resime 24bitovy rezim (film): v nem lezi
   oblast displeje ve VRAM jako 3 bajty na pixel, takze se musi dekodovat
   zvlast - jinak z toho jsou rozsypane barvy pri spravne geometrii. */
void nap_gles_present_frame(void)
{
    // (Celou videopamet uz kazdy snimek nenahravame - stalo to vykon
    //  a v B31 to fungovalo bez toho. Textury chodi pres hlaseni o
    //  zapisech do videopameti.)
    n2_flush();
    // ===== ROZHODUJICI MERENI =====
    // Zjistime, jestli je vubec CO kreslit: kolik nenulovych slov ma
    // videopamet, jaka je oblast kresleni a jaky vyrez displeje.
    // - videopamet prazdna  -> obraz nevyrabi jadro (problem pred rendererem)
    // - videopamet plna, ale obraz cerny -> chyba je v mem kresleni
    {
        static long dg = 0;
        if (dg++ % 180 == 0) {
            const unsigned short *v = (const unsigned short *)gpu.vram;
            long nz = 0;
            if (v) for (int i = 0; i < 1024 * 512; i += 97) if (v[i]) nz++;
            nap_diag_log("NAPLES2 KONTROLA: nenulovych ve videopameti=%ld/5406, vyrez=%dx%d src=[%d,%d], oblast kresleni=%d,%d..%d,%d, rgb24=%d",
                         nz, gpu.screen.hres, gpu.screen.vres,
                         gpu.screen.src_x, gpu.screen.src_y,
                         n2_area_x0(), n2_area_y0(), n2_area_x1(), n2_area_y1(),
                         (gpu.status & PSX_GPU_STATUS_RGB24) ? 1 : 0);
        }
    }
    if (gpu.status & PSX_GPU_STATUS_RGB24) {
        int w = gpu.screen.hres > 0 ? gpu.screen.hres : 320;
        int h = gpu.screen.vres > 0 ? gpu.screen.vres : 240;
        n2_present_rgb24(gpu.screen.src_x, gpu.screen.src_y, w, h);
        {
            static long n = 0;
            if (n++ % 300 == 0)
                nap_diag_log("NAPLES2 FILM (24bit) %dx%d src=[%d,%d] - dekodovano zvlast",
                             w, h, gpu.screen.src_x, gpu.screen.src_y);
        }
    }
    /* Kolik prace pripada na snimek - odtud pozname, jestli neco trha obraz. */
    {
        static long fr = 0;
        if (++fr % 120 == 0) {
            long d, wr, bl;
            n2_take_counters(&d, &wr, &bl);
            {
                long vt = 0, tr = 0;
                n2_take_vert_counters(&vt, &tr);
                char tx[256];
                n2_dump_textures(tx, (int)sizeof(tx));
                nap_diag_log("NAPLES2 ZATEZ za 120 snimku: kresleni=%ld vrcholy=%ld ztoho_pruhlednych=%ld (na snimek: %.0f vrcholu)",
                             d, vt, tr, vt/120.0);
                nap_diag_log("NAPLES2 TEXTURY: %s", tx[0] ? tx : "(zadne texturovane utvary)");
            }
        }
    }
}

/* Primá cesta: id textury s celou VRAM + vyrez displeje. Pri sdilenem
   GLES2 kontextu si ji eglrender nakresli sam, bez kopirovani. */
unsigned nap_gles_grab_texture(int *out_x, int *out_y, int *out_w, int *out_h)
{
    n2_flush();
    n2_sync_for_other_context();  /* bez tohohle vidi eglrender nehotovy obraz */
    if (out_x) *out_x = gpu.screen.src_x;
    if (out_y) *out_y = gpu.screen.src_y;
    if (out_w) *out_w = gpu.screen.hres > 0 ? gpu.screen.hres : 320;
    if (out_h) *out_h = gpu.screen.vres > 0 ? gpu.screen.vres : 240;
    return n2_vram_texture();
}
