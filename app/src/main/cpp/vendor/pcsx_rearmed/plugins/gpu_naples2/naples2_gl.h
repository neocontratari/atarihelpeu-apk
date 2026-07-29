/*
 * naples2_gl.h - GLES2 renderer pro PS1 GPU (NaPles2)
 *
 * PROC VZNIKL:
 *   Puvodni plugin gpu-gles je OpenGL ES *1* (fixed function). Mali na S8
 *   odmita sdilet GLES1 kontext s GLES2 kontextem eglrenderu, takze se obraz
 *   musel tahat pres procesor (glReadPixels) a resit dva kontexty, vlastni
 *   platno a prepocty souradnic. Odtud cela trida chyb.
 *
 *   Tenhle renderer je cely v GLES2:
 *     - kresli PS1 primitiva primo shadery,
 *     - VRAM je GL textura, texturovani i CLUT se resi ve fragment shaderu,
 *     - kresli se v souradnicich VRAM 1:1 (zadne prepocty na displej),
 *     - kontext lze sdilet s eglrenderem (GLES2<->GLES2), takze obraz muze
 *       jit primo, bez kopirovani pres procesor.
 *
 * ROZHRANI: naplnuje renderer_* funkce z gpulib (viz plugins/gpulib/gpu.h).
 */

#ifndef NAPLES2_GL_H
#define NAPLES2_GL_H

#include <stdint.h>

#define N2_VRAM_W 1024
#define N2_VRAM_H 512

/* --- zivotni cyklus ---------------------------------------------------- */
int  n2_init(void);            /* vytvori shadery, FBO a textury VRAM */
void n2_finish(void);

/* --- kresleni ---------------------------------------------------------- */
/* Zpracuje seznam GP0 prikazu. Vraci pocet zpracovanych slov. */
int  n2_do_cmd_list(uint32_t *list, int list_len, uint32_t *ex_regs, int *last_cmd);
void n2_flush(void);           /* vykresli vse, co ceka v davce */

/* --- prenosy VRAM ------------------------------------------------------ */
/* CPU zapsal do gpu.vram (logo BIOSu, snimek filmu, textury) - dostat to na GPU */
void n2_vram_written(int x, int y, int w, int h);
/* GPU neco nakreslil a procesor si to chce precist - dostat to zpet do gpu.vram */
void n2_vram_sync_to_cpu(int x, int y, int w, int h);

/* --- stav -------------------------------------------------------------- */
void n2_set_ecmds(uint32_t *ecmds);   /* E1..E6 najednou (obnova stavu) */

/* --- vystup ------------------------------------------------------------ */
/* Textura, ve ktere lezi cela VRAM (1024x512 RGBA). Eglrender ji pri sdilenem
   kontextu muze kreslit primo - zadne kopirovani. */
unsigned n2_vram_texture(void);
/* Zaloha pro nesdileny kontext: precte oblast displeje do RGBA bufferu. */
const void* n2_read_display(int sx, int sy, int w, int h);

#endif /* NAPLES2_GL_H */
