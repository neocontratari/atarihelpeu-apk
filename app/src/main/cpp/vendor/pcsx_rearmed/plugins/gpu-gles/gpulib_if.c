/***************************************************************************
    begin                : Sun Mar 08 2009
    copyright            : (C) 1999-2009 by Pete Bernert
    email                : BlackDove@addcom.de

    PCSX rearmed rework (C) notaz, 2012
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version. See also the license.txt file for *
 *   additional informations.                                              *
 *                                                                         *
 ***************************************************************************/

#include "gpuStdafx.h"
#include "gpuDraw.c"
#include "gpuTexture.c"
#include "gpuPrim.c"
#include "hud.c"

// BUILD2SK98: most do libretro.c (viz tam - nap_gles_push_frame) - gpu-gles
// renderuje primo pres EGL/GL (eglSwapBuffers v updateDisplay nize), na
// rozdil od gpu_neon nikdy sam nepredaval hotovy snimek zpet do CPU pametu.
// Tenhle extern + volani glReadPixels o par radku niz je JEDINA zmena
// oproti puvodnimu chovani tohoto souboru - vsechno ostatni (samotne
// kresleni) zustava presne tak, jak to venovil puvodce pluginu.
extern void nap_gles_push_frame(void *pixels, int w, int h, int pitch);
extern void nap_diag_log(const char *fmt, ...); // BUILD2SK100: viz nap_ps1_native.cpp

// BUILD2SK119/120: (puvodni pokusy o synchronizaci s DisplayMode - viz
// historie v predchozich poznamkach, nahrazeno nasledujicim, uplnym
// resenim na zaklade SK121 dat).
// BUILD2SK122: SKUTECNA, KOMPLETNI OPRAVA - zalozena na konkretnich datech
// z Reneho SK121 logu (ne na dalsim odhadu). PSXDISPLAY_STATE ukazal
// DrawArea BEZNE presahujici 320x240 (napr. [0,0,639,479] = 640x480, nebo
// dokonce [512,12,1023,227] - posunute o 512 pixelu doprava, klasicky PS1
// trik dvojiteho bufferovani vyuzivajici CELOU 1024-sirokou VRAM). Nase
// projekce/viewport (SK120) porad predpokladaly "svet" o velikosti
// DisplayMode (320x240) - jakakoli geometrie umistena/skalovana vuci
// SKUTECNE (vetsi, casem posouvajici se) ploše se pres SPATNE meritko
// projekce zobrazila zvetsena/posunuta presne od prvniho snimku (Sony
// logo) - to vysvetluje VSECHNY nahlasene artefakty najednou.
// RESENI: prestat honit promenlivy DrawArea pro projekci - misto toho
// nastavit viewport+projekci JEDNOU na SKUTECNOU, pevnou velikost VRAM
// cipu PS1 (1024x512 - realna hardwarova hodnota, ne odhad), precist
// CELOU tuhle oblast, a az POTOM sami vyriznout presne tu viditelnou
// cast (DisplayMode velika, na pozici DisplayPosition) jednoduchym,
// snadno overitelnym indexovanim pole - stejny pruchod uz zaroven resi
// i drivejsi Y-flip (SK118), zadna dalsi rezie navic.
#define NAP_PSX_VRAM_W 1024
#define NAP_PSX_VRAM_H 512
// BUILD2SK128: KLICOVY NALEZ Z RENEHO LOGU (2026-07-20): PSXDISPLAY_STATE
// ukazal Range=[0,0,0,0], DispPos=[0,0] a DisplayMode=320x240 PO CELOU DOBU,
// i kdyz hra prokazatelne bezela v 512/640 rezimu (DrawArea az [0,0,639,479]
// a [512,12,1023,227]). Range nastavuje uplne KAZDA hra (GP1 0x06/0x07) -
// same nuly = do peops casti se NIKDY nedostane ZADNY GP1 prikaz
// (GPUwriteStatus_ext se efektivne nevola). Cely "display" stav peops je
// zamrzly na hodnotach z renderer_init(). DUSLEDKY, ktere presne odpovidaji
// pozorovanym symptomum:
//   1) rb_w/rb_h = DisplayMode = porad 320x240 -> cetli jsme 320x240 vyrez
//      z obrazu, ktery je ve skutecnosti 512 siroky -> "vyzoomovany" obraz.
//   2) DisplayPosition (0,0) nebyl fakt o hre, ale mrtva promenna - SK126
//      heuristika stavela na datech, ktera se nikdy neaktualizuji.
//   3) updateDisplay() na GP1(0x05) se nikdy nespusti -> cteme v nahodne
//      fazi vuci dvojitemu bufferingu hry -> blikani.
// OPRAVA: prestat se spolehat na peops GP1 stav a cist AUTORITATIVNI stav
// primo z gpulib jadra (gpu.screen.src_x/src_y/hres/vres - plni ho gpu.c,
// ktery je includovany primo do tohoto souboru, viz nap_gpulib_display_info
// definovana AZ ZA tim includem). ZAROVEN: zamrzle DisplayMode=320x240
// v SetOGLDisplaySettings() vytvarelo scale faktor XS=1024/320=3.2 pro
// scissor - DrawArea x0=512 se tim prepocital na x=1638, tj. MIMO povrch,
// takze kresleni do prave poloviny VRAM mohl scissor cely zahodit. Proto
// tady DisplayMode natvrdo drzime na VRAM velikosti (1024x512) -> XS=YS=1
// -> scissor = DrawArea 1:1 ve VRAM souradnicich, spravne pro obe poloviny.
// (DisplayMode uz od SK128 NIKDE nepouzivame jako velikost cteni - viz
// nap_gles_readback_and_push.)
static int nap_gpulib_display_info(int *sx, int *sy, int *w, int *h); // definice az za #include gpu.c nize
void nap_gles_sync_display_settings(void)
{
  // BUILD2SK128: nastavovat KAZDY tick (drive jen jednou pres flag) - je to
  // par levnych GL volani a je to pojistka proti cemukoli, co by projekci/
  // viewport mezitim prenastavilo (updateDisplayIfChanged/SetAspectRatio
  // maji vlastni glOrtho/glViewport - dnes jsou to mrtve cesty, ale nechceme
  // na tom stavet).
  iResX = NAP_PSX_VRAM_W;
  iResY = NAP_PSX_VRAM_H;
  rRatioRect.left = 0; rRatioRect.top = 0;
  rRatioRect.right = NAP_PSX_VRAM_W; rRatioRect.bottom = NAP_PSX_VRAM_H;
  PSXDisplay.DisplayMode.x = NAP_PSX_VRAM_W;  // BUILD2SK128: viz komentar vyse - kvuli XS=YS=1 ve scissor vypoctu
  PSXDisplay.DisplayMode.y = NAP_PSX_VRAM_H;
  glViewport(0, 0, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW); // BUILD2SK128: vratit rezim matice tak, jak ho necha GLinitialize (SK122 nechaval PROJECTION aktivni)
  static int nap_proj_logged = 0;
  if (!nap_proj_logged) {
    nap_proj_logged = 1;
    nap_diag_log("BUILD2SK128 GLES_VRAM_PROJECTION_EVERY_TICK w=%d h=%d", NAP_PSX_VRAM_W, NAP_PSX_VRAM_H);
  }
  SetOGLDisplaySettings(1); // scissor kazdy snimek - od SK128 pocita 1:1 ve VRAM souradnicich (viz vyse)
}


static uint32_t *nap_gles_rb_buf = NULL; // BUILD2SK118: ted ARGB8888 primo (drive uint16_t RGB565)
static int nap_gles_rb_w = 0, nap_gles_rb_h = 0;
static int nap_gles_frame_count = 0; // BUILD2SK100: tep - kolik snimku uspesne prosel readback

static uint8_t *nap_gles_vram_rgba = NULL; // BUILD2SK122: FIXNI 1024x512x4 buffer - cela VRAM, alokovano jen jednou

static void nap_gles_readback_and_push(void)
{
 // BUILD2SK128: velikost i pozice cteni uz JEN z autoritativniho gpulib
 // stavu (gpu.screen - plneno primo z GP1 prikazu v gpu.c), ne ze zamrzleho
 // peops DisplayMode (viz velky komentar u nap_gles_sync_display_settings).
 int rb_w = 0, rb_h = 0, src_x = 0, src_y = 0;
 nap_gpulib_display_info(&src_x, &src_y, &rb_w, &rb_h);
 if (rb_w <= 0) rb_w = 320; // pojistka pro uplne prvni snimky pred prvnim GP1(0x08)
 if (rb_h <= 0) rb_h = 240;
 // BUILD2SK112: pridano glFinish() tady - hypoteza "GPU jeste nedokoncilo
 // kresleni" - NEPOMOHLO (video porad cerne, SK112 test) a Rene ohlasil
 // NOVY, postupne se zhorsujici problem se zvukem presne od tehdle zmeny.
 // BUILD2SK117: ODSTRANENO - glFinish() kazdy jednotlivy snimek nutí plne
 // zastaveni CPU/GPU, blokuje STEJNE vlakno, ktere pres retro_run() generuje
 // i zvuk - presne typ zmeny, co by zpusobil postupne se horsici audio
 // podle vytizeni. A neni to ani potreba: glReadPixels() je v OpenGL ES uz
 // sam o sobe definovany jako blokujici/synchronni - MUSI vratit spravna,
 // finalni data, takze uz sam pockat na dokonceni predchoziho kresleni,
 // ktere se tech pixelu tyka. glFinish() navic pred nim nepridaval zadnou
 // dalsi jistotu, jen zbytecnou rezii.
 // BUILD2SK100: tep KAZDYCH 30 snimku (stejny vzor jako jinde v projektu) -
 // pokud priste appka spadne, posledni zapsany tep rekne, kolik snimku se
 // stihlo VYKRESLIT (eglSwapBuffers UZ probehl - tenhle radek je AZ PO nem),
 // nez k padu doslo - a jestli se rozliseni mezitim nezmenilo na neco
 // podezreleho.
 nap_gles_frame_count++;
 if (nap_gles_frame_count % 30 == 1) {
  nap_diag_log("BUILD2SK100 GLES_FRAME_HEARTBEAT n=%d dispW=%d dispH=%d", nap_gles_frame_count, rb_w, rb_h);
 }
 if (rb_w <= 0 || rb_h <= 0 || rb_w > NAP_PSX_VRAM_W || rb_h > NAP_PSX_VRAM_H) return; // BUILD2SK122: mez presne podle skutecne VRAM velikosti
 // BUILD2SK124: KLICOVA OPRAVA - konkretni data ze SK123 testu ukazala
 // "scissor=[0,0,1024,512]" - scissor ZADNE UZITECNE OMEZENI NEDAVA (kryje
 // UPLNE CELOU VRAM)! SK123 pak cetlo skoro celou VRAM, z niz jen leva
 // cast (~320 sloupcu) obsahovala SKUTECNY obraz - zbytek byla nesouvisejici/
 // neinicializovana pamet, presne ten "sum" na prave polovine, co Rene
 // videl. SK122's PROJEKCE (1024x512 VRAM-scale) byla spravna a potrebna
 // (potvrzeno RAW_VS_FINAL shodou u sedych hodnot) - chyba byla, ze jsem
 // ZAROVEN zbytecne zkomplikoval i SAMOTNE CTENI (SK122 pres DisplayPosition,
 // SK123 pres scissor). Reseni: vratit cteni zpet na jednoduchy, primy
 // pristup - cist presne DisplayMode velikost OD ZACATKU (0,0), protoze
 // DisplayPosition byla ve VSECH dosavadnich datech VZDY (0,0) - jen TEDka
 // uz s opravenou (SK122) projekci pod tim, takze geometrie uz je spravne
 // namapovana driv, nez se vubec cte.
 if (nap_gles_rb_buf == NULL || nap_gles_rb_w != rb_w || nap_gles_rb_h != rb_h) {
  if (nap_gles_rb_buf != NULL) free(nap_gles_rb_buf);
  nap_gles_rb_buf = (uint32_t *)malloc((size_t)rb_w * (size_t)rb_h * 4);
  nap_gles_rb_w = rb_w;
  nap_gles_rb_h = rb_h;
 }
 if (nap_gles_vram_rgba == NULL) {
  nap_gles_vram_rgba = (uint8_t *)malloc((size_t)NAP_PSX_VRAM_W * (size_t)NAP_PSX_VRAM_H * 4); // pojistka na max mozny pripad
 }
 // BUILD2SK128: SK126 heuristika (preskakovat snimky podle DrawArea.x0)
 // ODSTRANENA - stavela na zamrzlem peops stavu (viz velky komentar u
 // nap_gles_sync_display_settings). "Kterou cast VRAM prave zobrazit" nam
 // rika primo gpulib: gpu.screen.src_x/src_y je SKUTECNA zobrazovaci pozice
 // z GP1(0x05) - presne ten "display-swap protokol", ktery se v SK98-SK127
 // hledal. Hra po dokonceni snimku prepne src_x mezi 0 a 512 (dvojity
 // buffering) a my proste cteme VZDY z prave zobrazovane pozice - stejne,
 // jako by to delal skutecny televizor pripojeny k PS1. Zadna heuristika,
 // zadne preskakovani snimku.
 if (nap_gles_rb_buf == NULL || nap_gles_vram_rgba == NULL) return; // alokace selhala - proste tenhle snimek preskoc, nic nespadne
 // BUILD2SK105: primo GL_RGBA+GL_UNSIGNED_BYTE - jedina kombinace, kterou
 // GLES specifikace zarucuje pro glReadPixels na jakemkoli zarizeni.
 {
  if (src_x < 0) src_x = 0;
  if (src_y < 0) src_y = 0;
  if (src_x + rb_w > NAP_PSX_VRAM_W) src_x = NAP_PSX_VRAM_W - rb_w;
  if (src_x < 0) src_x = 0;
  // BUILD2SK125 (princip zustava): Y-souradnice pro glReadPixels potrebuje
  // prevod - PS1 Y=0 je "nahore", ale glReadPixels bere Y=0 jako "dole"
  // (stejny flip-princip jako u glOrtho). PS1 radek Y odpovida GL radku
  // (VRAM_H-1-Y) - takze SPODEK cteneho useku (glReadPixels bere y jako
  // DOLNI okraj) je (VRAM_H - (src_y + rb_h)).
  int glY = NAP_PSX_VRAM_H - (src_y + rb_h);
  if (glY < 0) glY = 0;
  if (glY + rb_h > NAP_PSX_VRAM_H) glY = NAP_PSX_VRAM_H - rb_h;
  if (glY < 0) glY = 0;
  glReadPixels(src_x, glY, rb_w, rb_h, GL_RGBA, GL_UNSIGNED_BYTE, nap_gles_vram_rgba);
  if (nap_gles_frame_count % 30 == 1) {
   nap_diag_log("BUILD2SK128 GLES_READ_ANCHOR srcX=%d srcY=%d glY=%d rb_w=%d rb_h=%d", src_x, src_y, glY, rb_w, rb_h);
  }
 }
 {
  for (int y = 0; y < rb_h; y++) {
   const uint8_t *srcRow = nap_gles_vram_rgba + (size_t)y * rb_w * 4;
   uint32_t *dstRow = nap_gles_rb_buf + (size_t)(rb_h - 1 - y) * rb_w; // BUILD2SK118 styl flip - posledni GL radek na prvni Android radek
   for (int x = 0; x < rb_w; x++) {
    uint8_t r = srcRow[x * 4 + 0];
    uint8_t g = srcRow[x * 4 + 1];
    uint8_t b = srcRow[x * 4 + 2];
    dstRow[x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
   }
  }
 }
 // BUILD2SK104: obsah pixelu, ne jen rozmery - podezrele mala JPEG velikost
 // (5780B pro 320x240 hru) naznacuje, ze buffer muze byt skoro cerny/
 // prazdny i kdyz rozmery uz sedi. Levny soucet (staci vedet "je tam vubec
 // neco jineho nez 0") + par konkretnich vzorku pro presnejsi obraz. Stejne
 // omezeni jako heartbeat vyse (kazdy 30. snimek), aby to nezaplavilo log.
 if (nap_gles_frame_count % 30 == 1) {
  GLenum glerr = glGetError();
  unsigned long long sum = 0;
  int n = rb_w * rb_h; // BUILD2SK123: skutecna velikost bufferu (scissor), ne DisplayMode
  // BUILD2SK118: soucet jasu (ne uz syrovych packed hodnot - ARGB8888 ma
  // konstantni 0xFF v alfa bajtu, ktery by jinak zkresloval soucet).
  for (int i = 0; i < n; i++) {
    uint32_t px = nap_gles_rb_buf[i];
    sum += ((px >> 16) & 0xFF) + ((px >> 8) & 0xFF) + (px & 0xFF);
  }
  uint32_t pTL = nap_gles_rb_buf[0];
  uint32_t pCenter = nap_gles_rb_buf[n / 2];
  uint32_t pBR = nap_gles_rb_buf[n - 1];
  nap_diag_log("BUILD2SK124 GLES_PIXEL_SAMPLE glErr=0x%x sumAvg=%llu pTL=0x%08x pCenter=0x%08x pBR=0x%08x dispW=%d dispH=%d",
    (unsigned)glerr, (unsigned long long)(sum / (n > 0 ? n : 1)), (unsigned)pTL, (unsigned)pCenter, (unsigned)pBR,
    rb_w, rb_h);
  // BUILD2SK113: misto dalsich cisel - SKUTECNY (i kdyz hrubý, textovy)
  // obrazek toho, co se doopravdy zachytilo. Cisla (soucet, 3 vzorky) uz
  // nestaci rozlisit "jednolita barva" od "slozity obrazek s malym
  // kontrastem" - tohle to ukaze primo. Omezeno na malo opakovani CELKEM
  // (ne kazdych 30 snimku porad dokola), aby se log nezaplavil.
  static int nap_ascii_dump_count = 0;
  if (nap_ascii_dump_count < 8) {
    nap_ascii_dump_count++;
    const char *shades = " .:-=+*#%@";
    const int cols = 48, rows = 20;
    nap_diag_log("BUILD2SK113 GLES_ASCII_DUMP_START #%d src=%dx%d", nap_ascii_dump_count, rb_w, rb_h);
    for (int ry = 0; ry < rows; ry++) {
      char line[64];
      int lp = 0;
      for (int rx = 0; rx < cols; rx++) {
        int px_x = (rx * rb_w) / cols;
        int px_y = (ry * rb_h) / rows;
        if (px_x >= rb_w) px_x = rb_w - 1;
        if (px_y >= rb_h) px_y = rb_h - 1;
        uint32_t px = nap_gles_rb_buf[px_y * rb_w + px_x]; // BUILD2SK118: ted ARGB8888
        int r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
        int bright = (r + g + b) / 3;
        int idx = (bright * 9) / 255;
        if (idx < 0) idx = 0; if (idx > 9) idx = 9;
        line[lp++] = shades[idx];
      }
      line[lp] = '\0';
      nap_diag_log("BUILD2SK113 |%s|", line);
    }
  }
  // BUILD2SK109: gpuPrim.c ma makro DEFOPAQUEON - glAlphaFunc(GL_EQUAL,0.0f) -
  // "opaque" kresleni PROJDE jen kdyz je alfa PRESNE 0.0 (bezny PS1 zpusob
  // znaceni pruhlednosti masky). Pokud nahravani textur nenastavuje presne
  // tuhle hodnotu, KAZDY fragment by se tise zahodil - vysvetlilo by to
  // presne to, co vidime (cisteni funguje, geometrie ne, bez ohledu na
  // scissor/hloubku). Primy dotaz na aktualni stav, misto dalsiho hadani.
  {
    GLboolean alphaTestOn = glIsEnabled(GL_ALPHA_TEST);
    GLint alphaFunc = 0; GLfloat alphaRef = -1.0f;
    glGetIntegerv(GL_ALPHA_TEST_FUNC, &alphaFunc);
    glGetFloatv(GL_ALPHA_TEST_REF, &alphaRef);
    nap_diag_log("BUILD2SK109 GLES_ALPHA_TEST_CHECK enabled=%d func=0x%x ref=%f",
      (int)alphaTestOn, (unsigned)alphaFunc, (double)alphaRef);
  }
  // BUILD2SK121: DIAGNOSTIKA, ZADNA ZMENA CHOVANI (na Reneho vyslovny
  // pozadavek). SK120 ukazalo, ze VYSLEDNY scissor/viewport/rozliseni jsou
  // spravne a konzistentni - presto obraz porad vypada spatne (zoom,
  // problikavani). To znamena, ze pricina je NEKDE JINDE - primo v datech,
  // ktere do scissor-vypoctu VSTUPUJI (nikdy jsem je primo nezkontroloval,
  // jen VYSLEDEK), nebo v samotnem obsahu, co GPU vykresli, driv nez to
  // stihnu jakkoli zpracovat. Tenhle blok vypisuje OBOJI najednou.
  nap_diag_log("BUILD2SK121 PSXDISPLAY_STATE DrawArea=[%d,%d,%d,%d] DispPos=[%d,%d] Range=[%d,%d,%d,%d] DrawOffset=[%d,%d] Interlaced=%d Disabled=%d RGB24=%d",
    (int)PSXDisplay.DrawArea.x0, (int)PSXDisplay.DrawArea.y0, (int)PSXDisplay.DrawArea.x1, (int)PSXDisplay.DrawArea.y1,
    PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y,
    (int)PSXDisplay.Range.x0, (int)PSXDisplay.Range.y0, (int)PSXDisplay.Range.x1, (int)PSXDisplay.Range.y1,
    (int)PSXDisplay.DrawOffset.x, (int)PSXDisplay.DrawOffset.y,
    PSXDisplay.Interlaced, PSXDisplay.Disabled, PSXDisplay.RGB24);
  nap_diag_log("BUILD2SK121 PREVPSXDISPLAY_STATE DispPos=[%d,%d] Range=[%d,%d,%d,%d]",
    PreviousPSXDisplay.DisplayPosition.x, PreviousPSXDisplay.DisplayPosition.y,
    (int)PreviousPSXDisplay.Range.x0, (int)PreviousPSXDisplay.Range.y0, (int)PreviousPSXDisplay.Range.x1, (int)PreviousPSXDisplay.Range.y1);
  // BUILD2SK124: RAW (primo z GPU, jen precatana - jeste PRED flipem) vs
  // FINALNI (PO flipu) vzorek ze STEJNEHO mista - pokud RAW vypada dobre
  // ale FINALNI spatne, chyba je v nasem prevodu/flipu, ne v GPU kresleni.
  {
    int rawIdx = (rb_h/2) * rb_w * 4 + (rb_w/2) * 4;
    int finIdx = (rb_h/2) * rb_w;
    nap_diag_log("BUILD2SK123 RAW_VS_FINAL rawRGBA=[%d,%d,%d,%d] finalARGB=0x%08x",
      (int)nap_gles_vram_rgba[rawIdx+0], (int)nap_gles_vram_rgba[rawIdx+1], (int)nap_gles_vram_rgba[rawIdx+2], (int)nap_gles_vram_rgba[rawIdx+3],
      (unsigned)nap_gles_rb_buf[finIdx]);
  }
 }
 nap_gles_push_frame(nap_gles_rb_buf, rb_w, rb_h, rb_w * 4); // BUILD2SK124: DisplayMode rozmery (jednoduchy, spravny pristup)
}

static int is_opened;

static const short dispWidths[8] = {256,320,512,640,368,384,512,640};
short g_m1,g_m2,g_m3;
short DrawSemiTrans;

short          ly0,lx0,ly1,lx1,ly2,lx2,ly3,lx3;        // global psx vertex coords
int            GlobalTextAddrX,GlobalTextAddrY,GlobalTextTP;
int            GlobalTextREST,GlobalTextABR,GlobalTextPAGE;

unsigned int  dwGPUVersion;
int           iGPUHeight=512;
int           iGPUHeightMask=511;
int           GlobalTextIL;

unsigned char  *psxVub;
unsigned short *psxVuw;

GLfloat         gl_z=0.0f;
BOOL            bNeedInterlaceUpdate;
BOOL            bNeedRGB24Update;
BOOL            bChangeWinMode;
int             lGPUstatusRet;
unsigned int    ulGPUInfoVals[16];
VRAMLoad_t      VRAMWrite;
VRAMLoad_t      VRAMRead;
int             iDataWriteMode;
int             iDataReadMode;

int             lClearOnSwap;
int             lClearOnSwapColor;
BOOL            bSkipNextFrame;

PSXDisplay_t    PSXDisplay;
PSXDisplay_t    PreviousPSXDisplay;
TWin_t          TWin;
BOOL            bDisplayNotSet;
BOOL            bNeedWriteUpload;
int             iLastRGB24;

// don't do GL vram read
void CheckVRamRead(int x, int y, int dx, int dy, bool bFront)
{
}

void CheckVRamReadEx(int x, int y, int dx, int dy)
{
}

void SetFixes(void)
{
}

static void PaintBlackBorders(void)
{
 short s;
 glDisable(GL_SCISSOR_TEST); glError();
 if(bTexEnabled) {glDisable(GL_TEXTURE_2D);bTexEnabled=FALSE;} glError();
 if(bOldSmoothShaded) {glShadeModel(GL_FLAT);bOldSmoothShaded=FALSE;} glError();
 if(bBlendEnable)     {glDisable(GL_BLEND);bBlendEnable=FALSE;} glError();
 glDisable(GL_ALPHA_TEST); glError();

 glEnable(GL_ALPHA_TEST); glError();
 glEnable(GL_SCISSOR_TEST); glError();
}

static void fps_update(void);

void updateDisplay(void)
{
 bFakeFrontBuffer=FALSE;
 bRenderFrontBuffer=FALSE;

 if(PSXDisplay.RGB24)// && !bNeedUploadAfter)         // (mdec) upload wanted?
 {
  PrepareFullScreenUpload(-1);
  UploadScreen(PSXDisplay.Interlaced);                // -> upload whole screen from psx vram
  bNeedUploadTest=FALSE;
  bNeedInterlaceUpdate=FALSE;
  bNeedUploadAfter=FALSE;
  bNeedRGB24Update=FALSE;
 }
 else
 if(bNeedInterlaceUpdate)                             // smaller upload?
 {
  bNeedInterlaceUpdate=FALSE;
  xrUploadArea=xrUploadAreaIL;                        // -> upload this rect
  UploadScreen(TRUE);
 }

 if(dwActFixes&512) bCheckFF9G4(NULL);                // special game fix for FF9 

 if(PSXDisplay.Disabled)                              // display disabled?
 {
  // moved here
  glDisable(GL_SCISSOR_TEST); glError();                       
  glClearColor(0,0,0,128); glError();                 // -> clear whole backbuffer
  glClear(uiBufferBits); glError();
  glEnable(GL_SCISSOR_TEST); glError();                       
  gl_z=0.0f;
  bDisplayNotSet = TRUE;
 }

 if(iDrawnSomething)
 {
  fps_update();
  EGLBoolean swapOk = eglSwapBuffers(display, surface);
  // BUILD2SK111: DRIV nikdy nekontrolovano - eglGetError() je JINA vec nez
  // glGetError() (co uz kontrolujeme jinde). Kontroluje chyby na urovni
  // EGL/systemu/ovladace (napr. "tenhle povrch neni platny pro prezentaci"),
  // ne uvnitr samotneho GL vykreslovani. Rene se ptal, jestli neco na
  // urovni telefonu tohle nemuze blokovat - tohle presne overuje.
  if (nap_gles_frame_count % 30 == 1) {
    EGLint eglErr = eglGetError();
    nap_diag_log("BUILD2SK111 GLES_SWAP_CHECK path=updateDisplay ok=%d eglErr=0x%x", (int)swapOk, (unsigned)eglErr);
  }
  nap_gles_readback_and_push(); // BUILD2SK98
  iDrawnSomething=0;
 }

 if(lClearOnSwap)                                     // clear buffer after swap?
 {
  GLclampf g,b,r;

  if(bDisplayNotSet)                                  // -> set new vals
   SetOGLDisplaySettings(1);

  g=((GLclampf)GREEN(lClearOnSwapColor))/255.0f;      // -> get col
  b=((GLclampf)BLUE(lClearOnSwapColor))/255.0f;
  r=((GLclampf)RED(lClearOnSwapColor))/255.0f;
  glDisable(GL_SCISSOR_TEST); glError();                       
  glClearColor(r,g,b,128); glError();                 // -> clear 
  glClear(uiBufferBits); glError();
  glEnable(GL_SCISSOR_TEST); glError();                       
  lClearOnSwap=0;                                     // -> done
 }
 else 
 {
  if(iZBufferDepth)                                   // clear zbuffer as well (if activated)
   {
    glDisable(GL_SCISSOR_TEST); glError();
    glClear(GL_DEPTH_BUFFER_BIT); glError();
    glEnable(GL_SCISSOR_TEST); glError();
   }
 }
 gl_z=0.0f;

 // additional uploads immediatly after swapping
 if(bNeedUploadAfter)                                 // upload wanted?
 {
  bNeedUploadAfter=FALSE;                           
  bNeedUploadTest=FALSE;
  UploadScreen(-1);                                   // -> upload
 }

 if(bNeedUploadTest)
 {
  bNeedUploadTest=FALSE;
  if(PSXDisplay.InterlacedTest &&
     //iOffscreenDrawing>2 &&
     PreviousPSXDisplay.DisplayPosition.x==PSXDisplay.DisplayPosition.x &&
     PreviousPSXDisplay.DisplayEnd.x==PSXDisplay.DisplayEnd.x &&
     PreviousPSXDisplay.DisplayPosition.y==PSXDisplay.DisplayPosition.y &&
     PreviousPSXDisplay.DisplayEnd.y==PSXDisplay.DisplayEnd.y)
   {
    PrepareFullScreenUpload(TRUE);
    UploadScreen(TRUE);
   }
 }
}

void updateFrontDisplay(void)
{
 if(PreviousPSXDisplay.Range.x0||
    PreviousPSXDisplay.Range.y0)
  PaintBlackBorders();

 bFakeFrontBuffer=FALSE;
 bRenderFrontBuffer=FALSE;

 if(iDrawnSomething) {                                 // linux:
  EGLBoolean swapOk = eglSwapBuffers(display, surface);
  // BUILD2SK111: viz stejny komentar u updateDisplay() vyse - tohle je ale
  // ta DULEZITA cesta, protoze updateFrontDisplay() je funkce, kterou
  // SKUTECNE volame kazdy tick (SK103).
  if (nap_gles_frame_count % 30 == 1) {
    EGLint eglErr = eglGetError();
    nap_diag_log("BUILD2SK111 GLES_SWAP_CHECK path=updateFrontDisplay ok=%d eglErr=0x%x", (int)swapOk, (unsigned)eglErr);
  }
  nap_gles_readback_and_push(); // BUILD2SK98
 }
}

static void ChangeDispOffsetsX(void)                  // CENTER X
{
int lx,l;short sO;

if(!PSXDisplay.Range.x1) return;                      // some range given?

l=PSXDisplay.DisplayMode.x;

l*=(int)PSXDisplay.Range.x1;                         // some funky calculation
l/=2560;lx=l;l&=0xfffffff8;

if(l==PreviousPSXDisplay.Range.x1) return;            // some change?

sO=PreviousPSXDisplay.Range.x0;                       // store old

if(lx>=PSXDisplay.DisplayMode.x)                      // range bigger?
 {
  PreviousPSXDisplay.Range.x1=                        // -> take display width
   PSXDisplay.DisplayMode.x;
  PreviousPSXDisplay.Range.x0=0;                      // -> start pos is 0
 }
else                                                  // range smaller? center it
 {
  PreviousPSXDisplay.Range.x1=l;                      // -> store width (8 pixel aligned)
   PreviousPSXDisplay.Range.x0=                       // -> calc start pos
   (PSXDisplay.Range.x0-500)/8;
  if(PreviousPSXDisplay.Range.x0<0)                   // -> we don't support neg. values yet
   PreviousPSXDisplay.Range.x0=0;

  if((PreviousPSXDisplay.Range.x0+lx)>                // -> uhuu... that's too much
     PSXDisplay.DisplayMode.x)
   {
    PreviousPSXDisplay.Range.x0=                      // -> adjust start
     PSXDisplay.DisplayMode.x-lx;
    PreviousPSXDisplay.Range.x1+=lx-l;                // -> adjust width
   }                   
 }

if(sO!=PreviousPSXDisplay.Range.x0)                   // something changed?
 {
  bDisplayNotSet=TRUE;                                // -> recalc display stuff
 }
}

////////////////////////////////////////////////////////////////////////

static void ChangeDispOffsetsY(void)                  // CENTER Y
{
int iT;short sO;                                      // store previous y size

if(PSXDisplay.PAL) iT=48; else iT=28;                 // different offsets on PAL/NTSC

if(PSXDisplay.Range.y0>=iT)                           // crossed the security line? :)
 {
  PreviousPSXDisplay.Range.y1=                        // -> store width
   PSXDisplay.DisplayModeNew.y;
  
  sO=(PSXDisplay.Range.y0-iT-4)*PSXDisplay.Double;    // -> calc offset
  if(sO<0) sO=0;

  PSXDisplay.DisplayModeNew.y+=sO;                    // -> add offset to y size, too
 }
else sO=0;                                            // else no offset

if(sO!=PreviousPSXDisplay.Range.y0)                   // something changed?
 {
  PreviousPSXDisplay.Range.y0=sO;
  bDisplayNotSet=TRUE;                                // -> recalc display stuff
 }
}

static void updateDisplayIfChanged(void)
{
BOOL bUp;

if ((PSXDisplay.DisplayMode.y == PSXDisplay.DisplayModeNew.y) && 
    (PSXDisplay.DisplayMode.x == PSXDisplay.DisplayModeNew.x))
 {
  if((PSXDisplay.RGB24      == PSXDisplay.RGB24New) && 
     (PSXDisplay.Interlaced == PSXDisplay.InterlacedNew)) 
     return;                                          // nothing has changed? fine, no swap buffer needed
 }
else                                                  // some res change?
 {
  glLoadIdentity(); glError();
  glOrtho(0,PSXDisplay.DisplayModeNew.x,              // -> new psx resolution
            PSXDisplay.DisplayModeNew.y, 0, -1, 1); glError();
  if(bKeepRatio) SetAspectRatio();
 }

bDisplayNotSet = TRUE;                                // re-calc offsets/display area

bUp=FALSE;
if(PSXDisplay.RGB24!=PSXDisplay.RGB24New)             // clean up textures, if rgb mode change (usually mdec on/off)
 {
  PreviousPSXDisplay.RGB24=0;                         // no full 24 frame uploaded yet
  ResetTextureArea(FALSE);
  bUp=TRUE;
 }

PSXDisplay.RGB24         = PSXDisplay.RGB24New;       // get new infos
PSXDisplay.DisplayMode.y = PSXDisplay.DisplayModeNew.y;
PSXDisplay.DisplayMode.x = PSXDisplay.DisplayModeNew.x;
PSXDisplay.Interlaced    = PSXDisplay.InterlacedNew;

PSXDisplay.DisplayEnd.x=                              // calc new ends
 PSXDisplay.DisplayPosition.x+ PSXDisplay.DisplayMode.x;
PSXDisplay.DisplayEnd.y=
 PSXDisplay.DisplayPosition.y+ PSXDisplay.DisplayMode.y+PreviousPSXDisplay.DisplayModeNew.y;
PreviousPSXDisplay.DisplayEnd.x=
 PreviousPSXDisplay.DisplayPosition.x+ PSXDisplay.DisplayMode.x;
PreviousPSXDisplay.DisplayEnd.y=
 PreviousPSXDisplay.DisplayPosition.y+ PSXDisplay.DisplayMode.y+PreviousPSXDisplay.DisplayModeNew.y;

ChangeDispOffsetsX();
if(bUp) updateDisplay();                              // yeah, real update (swap buffer)
}

#define GPUwriteStatus_ext GPUwriteStatus_ext // for gpulib to see this
void GPUwriteStatus_ext(unsigned int gdata)
{
 if (!is_opened)
  return;

switch((gdata>>24)&0xff)
 {
  case 0x00:
   PSXDisplay.Disabled=1;
   PSXDisplay.DrawOffset.x=PSXDisplay.DrawOffset.y=0;
   drawX=drawY=0;drawW=drawH=0;
   sSetMask=0;lSetMask=0;bCheckMask=FALSE;iSetMask=0;
   usMirror=0;
   GlobalTextAddrX=0;GlobalTextAddrY=0;
   GlobalTextTP=0;GlobalTextABR=0;
   PSXDisplay.RGB24=FALSE;
   PSXDisplay.Interlaced=FALSE;
   bUsingTWin = FALSE;
   return;

  case 0x03:  
   PreviousPSXDisplay.Disabled = PSXDisplay.Disabled;
   PSXDisplay.Disabled = (gdata & 1);

   if (iOffscreenDrawing==4 &&
        PreviousPSXDisplay.Disabled && 
       !(PSXDisplay.Disabled))
    {

     if(!PSXDisplay.RGB24)
      {
       PrepareFullScreenUpload(TRUE);
       UploadScreen(TRUE); 
       updateDisplay();
      }
    }
   return;

  case 0x05: 
   {
    short sx=(short)(gdata & 0x3ff);
    short sy;

    sy = (short)((gdata>>10)&0x3ff);             // really: 0x1ff, but we adjust it later
    if (sy & 0x200) 
     {
      sy|=0xfc00;
      PreviousPSXDisplay.DisplayModeNew.y=sy/PSXDisplay.Double;
      sy=0;
     }
    else PreviousPSXDisplay.DisplayModeNew.y=0;

    if(sx>1000) sx=0;

    if(dwActFixes&8) 
     {
      if((!PSXDisplay.Interlaced) &&
         PreviousPSXDisplay.DisplayPosition.x == sx  &&
         PreviousPSXDisplay.DisplayPosition.y == sy)
       return;

      PSXDisplay.DisplayPosition.x = PreviousPSXDisplay.DisplayPosition.x;
      PSXDisplay.DisplayPosition.y = PreviousPSXDisplay.DisplayPosition.y;
      PreviousPSXDisplay.DisplayPosition.x = sx;
      PreviousPSXDisplay.DisplayPosition.y = sy;
     }
    else
     {
      if((!PSXDisplay.Interlaced) &&
         PSXDisplay.DisplayPosition.x == sx  &&
         PSXDisplay.DisplayPosition.y == sy)
       return;
      PreviousPSXDisplay.DisplayPosition.x = PSXDisplay.DisplayPosition.x;
      PreviousPSXDisplay.DisplayPosition.y = PSXDisplay.DisplayPosition.y;
      PSXDisplay.DisplayPosition.x = sx;
      PSXDisplay.DisplayPosition.y = sy;
     }

    PSXDisplay.DisplayEnd.x=
     PSXDisplay.DisplayPosition.x+ PSXDisplay.DisplayMode.x;
    PSXDisplay.DisplayEnd.y=
     PSXDisplay.DisplayPosition.y+ PSXDisplay.DisplayMode.y+PreviousPSXDisplay.DisplayModeNew.y;

    PreviousPSXDisplay.DisplayEnd.x=
     PreviousPSXDisplay.DisplayPosition.x+ PSXDisplay.DisplayMode.x;
    PreviousPSXDisplay.DisplayEnd.y=
     PreviousPSXDisplay.DisplayPosition.y+ PSXDisplay.DisplayMode.y+PreviousPSXDisplay.DisplayModeNew.y;

    bDisplayNotSet = TRUE;

    if (!(PSXDisplay.Interlaced))
     {
      updateDisplay();
     }
    else
    if(PSXDisplay.InterlacedTest && 
       ((PreviousPSXDisplay.DisplayPosition.x != PSXDisplay.DisplayPosition.x)||
        (PreviousPSXDisplay.DisplayPosition.y != PSXDisplay.DisplayPosition.y)))
     PSXDisplay.InterlacedTest--;
    return;
   }

  case 0x06:
   PSXDisplay.Range.x0=gdata & 0x7ff;      //0x3ff;
   PSXDisplay.Range.x1=(gdata>>12) & 0xfff;//0x7ff;

   PSXDisplay.Range.x1-=PSXDisplay.Range.x0;

   ChangeDispOffsetsX();
   return;

  case 0x07:
   PreviousPSXDisplay.Height = PSXDisplay.Height;

   PSXDisplay.Range.y0=gdata & 0x3ff;
   PSXDisplay.Range.y1=(gdata>>10) & 0x3ff;

   PSXDisplay.Height = PSXDisplay.Range.y1 - 
                       PSXDisplay.Range.y0 +
                       PreviousPSXDisplay.DisplayModeNew.y;

   if (PreviousPSXDisplay.Height != PSXDisplay.Height)
    {
     PSXDisplay.DisplayModeNew.y=PSXDisplay.Height*PSXDisplay.Double;
     ChangeDispOffsetsY();
     updateDisplayIfChanged();
    }
   return;

  case 0x08:
   PSXDisplay.DisplayModeNew.x = dispWidths[(gdata & 0x03) | ((gdata & 0x40) >> 4)];

   if (gdata&0x04) PSXDisplay.Double=2;
   else            PSXDisplay.Double=1;
   PSXDisplay.DisplayModeNew.y = PSXDisplay.Height*PSXDisplay.Double;

   ChangeDispOffsetsY();
 
   PSXDisplay.PAL           = (gdata & 0x08)?TRUE:FALSE; // if 1 - PAL mode, else NTSC
   PSXDisplay.RGB24New      = (gdata & 0x10)?TRUE:FALSE; // if 1 - TrueColor
   PSXDisplay.InterlacedNew = (gdata & 0x20)?TRUE:FALSE; // if 1 - Interlace

   PreviousPSXDisplay.InterlacedNew=FALSE;
   if (PSXDisplay.InterlacedNew)
    {
     if(!PSXDisplay.Interlaced)
      {
       PSXDisplay.InterlacedTest=2;
       PreviousPSXDisplay.DisplayPosition.x = PSXDisplay.DisplayPosition.x;
       PreviousPSXDisplay.DisplayPosition.y = PSXDisplay.DisplayPosition.y;
       PreviousPSXDisplay.InterlacedNew=TRUE;
      }
    }
   else 
    {
     PSXDisplay.InterlacedTest=0;
    }
   updateDisplayIfChanged();
   return;
 }
}

/////////////////////////////////////////////////////////////////////////////

#include <stdint.h>

#include "../gpulib/gpu.c"

// BUILD2SK128: pristup k autoritativnimu display stavu gpulib jadra.
// Definovano AZ TADY (za includem gpu.c), protoze globalni promenna `gpu`
// vznika teprve v gpu.c - vyse v souboru je jen forward deklarace teto
// funkce. gpu.screen.src_x/src_y plni primo GP1(0x05) handler v gpu.c,
// hres/vres plni GP1(0x06/0x07/0x08) - tohle je JEDINY spolehlivy zdroj
// "co se prave zobrazuje" v teto kodove zakladne (peops kopie stejnych
// hodnot je zamrzla, viz komentar u nap_gles_sync_display_settings).
static int nap_gpulib_display_info(int *sx, int *sy, int *w, int *h)
{
  *sx = (int)gpu.screen.src_x;
  *sy = (int)gpu.screen.src_y;
  *w  = (int)gpu.screen.hres;
  *h  = (int)gpu.screen.vres;
  return 0;
}

static void set_vram(void *vram)
{
 psxVub=vram;
 psxVuw=(unsigned short *)psxVub;
}

int renderer_init(void)
{
 set_vram(gpu.vram);

 PSXDisplay.RGB24        = FALSE;                      // init some stuff
 PSXDisplay.Interlaced   = FALSE;
 PSXDisplay.DrawOffset.x = 0;
 PSXDisplay.DrawOffset.y = 0;
 PSXDisplay.DisplayMode.x= 320;
 PSXDisplay.DisplayMode.y= 240;
 PSXDisplay.Disabled     = FALSE;
 PSXDisplay.Range.x0=0;
 PSXDisplay.Range.x1=0;
 PSXDisplay.Double = 1;

 lGPUstatusRet = 0x14802000;

 return 0;
}

static void clear_gl_state_for_menu(void)
{
 static const GLenum caps[] = {
  GL_ALPHA_TEST, GL_BLEND, GL_COLOR_LOGIC_OP, GL_COLOR_MATERIAL,
  GL_CULL_FACE, GL_DEPTH_TEST, GL_FOG, GL_LIGHTING, GL_NORMALIZE,
  GL_POLYGON_OFFSET_FILL, GL_RESCALE_NORMAL, GL_SAMPLE_ALPHA_TO_COVERAGE,
  GL_SAMPLE_ALPHA_TO_ONE, GL_SAMPLE_COVERAGE, GL_SCISSOR_TEST, GL_STENCIL_TEST
 };
 static const GLenum cstates[] = {
  GL_COLOR_ARRAY, GL_NORMAL_ARRAY, GL_POINT_SIZE_ARRAY_OES
 };
 size_t i;
 for (i = 0; i < sizeof(caps) / sizeof(caps[0]); i++)
  glDisable(caps[i]);
 for (i = 0; i < 6; i++)
  glDisable(GL_CLIP_PLANE0 + i);
 for (i = 0; i < 8; i++)
  glDisable(GL_LIGHT0 + i);
 for (i = 0; i < sizeof(cstates) / sizeof(cstates[0]); i++)
  glDisableClientState(cstates[i]);

 glColor4ub(255, 255, 255, 255);
 glLoadIdentity();
 glEnable(GL_TEXTURE_2D);
 glEnableClientState(GL_TEXTURE_COORD_ARRAY);
 glEnableClientState(GL_VERTEX_ARRAY);
}

void renderer_finish(void)
{
}

void renderer_notify_screen_change(const struct psx_gpu_screen *screen)
{
}

extern const unsigned char cmd_lengths[256];

// XXX: mostly dupe code from soft peops
int renderer_do_cmd_list(uint32_t *list, int list_len, uint32_t *ex_regs,
 int *cycles_sum_out, int *cycles_last, int *last_cmd)
{
  unsigned int cmd, len;
  unsigned int *list_start = list;
  unsigned int *list_end = list + list_len;

  for (; list < list_end; list += 1 + len)
  {
    cmd = *list >> 24;
    len = cmd_lengths[cmd];
    if (list + 1 + len > list_end) {
      cmd = -1;
      break;
    }

#ifndef TEST
    if (cmd == 0xa0 || cmd == 0xc0)
      break; // image i/o, forward to upper layer
    else if ((cmd & 0xf8) == 0xe0)
      ex_regs[cmd & 7] = list[0];
#endif

    primTableJ[cmd]((void *)list);

    switch(cmd)
    {
      case 0x48 ... 0x4F:
      {
        uint32_t num_vertexes = 2;
        uint32_t *list_position = &(list[3]);

        while(1)
        {
          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }

          if((*list_position & 0xf000f000) == 0x50005000)
            break;

          list_position++;
          num_vertexes++;
        }

        len += (num_vertexes - 2);
        break;
      }

      case 0x58 ... 0x5F:
      {
        uint32_t num_vertexes = 2;
        uint32_t *list_position = &(list[4]);

        while(1)
        {
          if(list_position >= list_end) {
            cmd = -1;
            goto breakloop;
          }

          if((*list_position & 0xf000f000) == 0x50005000)
            break;

          list_position += 2;
          num_vertexes++;
        }

        len += (num_vertexes - 2) * 2;
        break;
      }

#ifdef TEST
      case 0xA0:          //  sys -> vid
      {
        short *slist = (void *)list;
        uint32_t load_width = slist[4];
        uint32_t load_height = slist[5];
        uint32_t load_size = load_width * load_height;

        len += load_size / 2;
        break;
      }
#endif
    }
  }

breakloop:
  ex_regs[1] &= ~0x1ff;
  ex_regs[1] |= lGPUstatusRet & 0x1ff;

  *last_cmd = cmd;
  return list - list_start;
}

void renderer_sync_ecmds(uint32_t *ecmds)
{
  cmdTexturePage((unsigned char *)&ecmds[1]);
  cmdTextureWindow((unsigned char *)&ecmds[2]);
  cmdDrawAreaStart((unsigned char *)&ecmds[3]);
  cmdDrawAreaEnd((unsigned char *)&ecmds[4]);
  cmdDrawOffset((unsigned char *)&ecmds[5]);
  cmdSTP((unsigned char *)&ecmds[6]);
}

void renderer_update_caches(int x, int y, int w, int h, int state_changed)
{
 VRAMWrite.x = x;
 VRAMWrite.y = y;
 VRAMWrite.Width = w;
 VRAMWrite.Height = h;
 if(is_opened)
  CheckWriteUpdate();
}

void renderer_flush_queues(void)
{
}

void renderer_set_interlace(int enable, int is_odd)
{
}

int vout_init(void)
{
  return 0;
}

int vout_finish(void)
{
  return 0;
}

int vout_update(struct psx_gpu *gpu, int src_x, int src_y)
{
 // BUILD2SK128: DIAGNOSTIKA - poprve v cele SK98-SK128 serii zaznamename
 // SKUTECNOU zobrazovaci pozici z gpulib jadra (src_x/src_y prichazi primo
 // z GP1(0x05) handleru v gpu.c). Ocekavani pro Crash Bandicoot: src_x se
 // bude behem hrani stridat mezi 0 a 512 (dvojity buffering). Log kazdou
 // ZMENU (omezeno poctem, at nezaplavi vystup) + obcasny periodicky tep.
 {
  static int nap_vu_count = 0;
  static int nap_vu_last_sx = -12345, nap_vu_last_sy = -12345;
  static int nap_vu_change_logs = 0;
  nap_vu_count++;
  if (src_x != nap_vu_last_sx || src_y != nap_vu_last_sy) {
   if (nap_vu_change_logs < 80) {
    nap_vu_change_logs++;
    nap_diag_log("BUILD2SK128 VOUT_SRC_CHANGE n=%d src=[%d,%d] prev=[%d,%d] hres=%d vres=%d",
      nap_vu_count, src_x, src_y, nap_vu_last_sx, nap_vu_last_sy,
      (int)gpu->screen.hres, (int)gpu->screen.vres);
   }
   nap_vu_last_sx = src_x;
   nap_vu_last_sy = src_y;
  } else if (nap_vu_count % 600 == 1) {
   nap_diag_log("BUILD2SK128 VOUT_SRC_PERIODIC n=%d src=[%d,%d] hres=%d vres=%d",
     nap_vu_count, src_x, src_y, (int)gpu->screen.hres, (int)gpu->screen.vres);
  }
 }
 if(PSXDisplay.Interlaced)                            // interlaced mode?
 {
  if(PSXDisplay.DisplayMode.x>0 && PSXDisplay.DisplayMode.y>0)
   {
    updateDisplay();                                  // -> swap buffers (new frame)
    return 1;
   }
 }
 else if(bRenderFrontBuffer)                          // no interlace mode? and some stuff in front has changed?
 {
  updateFrontDisplay();                               // -> update front buffer
  return 1;
 }
 return 0;
}

void vout_blank(struct psx_gpu *gpu)
{
}

void vout_set_config(const struct rearmed_cbs *cbs)
{
}

static struct rearmed_cbs *cbs;

long GPUopen(unsigned long *disp, char *cap, char *cfg)
{
 int ret;

 if (is_opened) {
  fprintf(stderr, "double GPUopen\n");
  return -1;
 }
 iResX = cbs->screen_w;
 iResY = cbs->screen_h;
 rRatioRect.left   = rRatioRect.top=0;
 rRatioRect.right  = iResX;
 rRatioRect.bottom = iResY;

 bDisplayNotSet = TRUE; 
 bSetClip = TRUE;
 CSTEXTURE = CSVERTEX = CSCOLOR = 0;

 InitializeTextureStore();                             // init texture mem

 ret = GLinitialize(cbs->gles_display, cbs->gles_surface);
 MakeDisplayLists();

 is_opened = 1;
 return ret;
}

long GPUclose(void)
{
 if (!is_opened)
  return 0;
 is_opened = 0;

 KillDisplayLists();
 clear_gl_state_for_menu();
 GLcleanup();                                          // close OGL
 return 0;
}

/* acting as both renderer and vout handler here .. */
void renderer_set_config(const struct rearmed_cbs *cbs_)
{
 cbs = (void *)cbs_; // ugh..

 iOffscreenDrawing = 0;
 iZBufferDepth = 0;
 iFrameReadType = 0;
 bKeepRatio = TRUE;

 dwActFixes = cbs->gpu_peopsgl.dwActFixes;
 bDrawDither = cbs->gpu_peopsgl.bDrawDither;
 iFilterType = cbs->gpu_peopsgl.iFilterType;
 iFrameTexType = cbs->gpu_peopsgl.iFrameTexType;
 iUseMask = cbs->gpu_peopsgl.iUseMask;
 bOpaquePass = cbs->gpu_peopsgl.bOpaquePass;
 bAdvancedBlend = cbs->gpu_peopsgl.bAdvancedBlend;
 bUseFastMdec = cbs->gpu_peopsgl.bUseFastMdec;
 iTexGarbageCollection = cbs->gpu_peopsgl.iTexGarbageCollection;
 iVRamSize = cbs->gpu_peopsgl.iVRamSize;

 if (cbs->pl_set_gpu_caps)
  cbs->pl_set_gpu_caps(GPU_CAP_OWNS_DISPLAY);

 if (is_opened && cbs->gles_display != NULL && cbs->gles_surface != NULL) {
  if (cbs->gles_display != display || cbs->gles_surface != surface) {
   // HACK...
   fprintf(stderr, "gles reinit hack\n");
   GPUclose();
   GPUopen(NULL, NULL, NULL);
  }
 }

 set_vram(gpu.vram);
}

void SetAspectRatio(void)
{
 if (cbs->pl_get_layer_pos)
  cbs->pl_get_layer_pos(&rRatioRect.left, &rRatioRect.top, &rRatioRect.right, &rRatioRect.bottom);

 glScissor(rRatioRect.left,
           iResY-(rRatioRect.top+rRatioRect.bottom),
	   rRatioRect.right,rRatioRect.bottom);
 glViewport(rRatioRect.left,
           iResY-(rRatioRect.top+rRatioRect.bottom),
           rRatioRect.right,rRatioRect.bottom);
 glError();
}

static void fps_update(void)
{
 char buf[16];

 cbs->flip_cnt++;
 if(cbs->flips_per_sec != 0)
 {
  snprintf(buf,sizeof(buf),"%2d %4.1f",cbs->flips_per_sec,cbs->vsps_cur);
  DisplayText(buf, 0);
 }
 if(cbs->cpu_usage != 0)
 {
  snprintf(buf,sizeof(buf),"%3d",cbs->cpu_usage);
  DisplayText(buf, 1);
 }
}
