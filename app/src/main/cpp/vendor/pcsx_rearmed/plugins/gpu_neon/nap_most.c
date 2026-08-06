/* ============================================================================
 *  MUSTEK MEZI APLIKACI A PROVERENYM VYKRESLOVACEM gpu_neon
 *
 *  PROC TENHLE SOUBOR EXISTUJE
 *  ---------------------------
 *  Do B82 se do telefonu prekladal rucne psany vykreslovac gpu_naples2
 *  (1291 radku). Vedle nej lezelo v projektu nepouzitych 19 377 radku
 *  hotoveho a provereneho vykreslovace gpu_neon, ktery je soucasti jadra
 *  PCSX-ReARMed. Rucne psany vykreslovac neumel vsechno, co PlayStation
 *  dela - odtud kostickovana grafika ve Formuli, artefakty ve Star Wars
 *  a blikajici dema.
 *
 *  Od B83 se preklada gpu_neon. Aplikace ale volala osm funkci, ktere
 *  patrily jen k tomu rucne psanemu vykreslovaci. Tenhle soubor je
 *  dodava, aby se nemuselo prepisovat pul aplikace.
 *
 *  JAK TED TECE OBRAZ
 *  ------------------
 *    jadro -> gpu_neon kresli do videopameti (gpu.vram)
 *          -> gpulib preda snimek pres retro_video_refresh
 *          -> nap_video() v nap_ps1_native.cpp ho ulozi
 *          -> nativni plocha (SurfaceView) ho nakresli pres OpenGL ES
 *
 *  Kresleni na obrazovku tedy PORAD BEZI PRES GPU a OpenGL ES. Zmenilo se
 *  jen to, ze prikazy PlayStation prevadi na obraz provereny vykreslovac
 *  misto rucne psaneho.
 * ==========================================================================*/

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "../gpulib/gpu.h"

extern void nap_diag_log(const char *fmt, ...);

/* Priprava grafiky. Provereny vykreslovac zadny vlastni GL kontext
   nepotrebuje - obraz kresli do videopameti a na obrazovku ho dostane
   nativni plocha. Vracime uspech, aby se start jadra nezastavil. */
int nap_gles_egl_init(void)
{
    nap_diag_log("VYKRESLOVAC: gpu_neon (provereny, soucast jadra PCSX-ReARMed)");
    return 1;
}

/* Rozmery videopameti PlayStation. */
int nap_gles_vram_w(void) { return 1024; }
int nap_gles_vram_h(void) { return  512; }

/* Snimek si aplikace bere z nap_video(), sem uz nesaha. */
const void *nap_gles_grab_pixels(int *out_w, int *out_h)
{
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    return NULL;
}

/* Prima cesta pres sdilenou texturu neexistuje - a nikdy nefungovala,
   viz poznamka v eglrender/egl_main.c. Nula = neni. */
unsigned nap_gles_grab_texture(int *out_x, int *out_y, int *out_w, int *out_h)
{
    if (out_x) *out_x = 0;
    if (out_y) *out_y = 0;
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    return 0;
}

/* Tyhle tri delal rucne psany vykreslovac. Provereny je nepotrebuje. */
void nap_gles_present_frame(void) { }
void nap_gles_readback_and_push(void) { }
void nap_gles_sync_display_settings(void) { }
