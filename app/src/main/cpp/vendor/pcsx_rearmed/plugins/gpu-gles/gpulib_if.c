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

// BUILD2SK119: iResX/iResY (a z nich odvozeny rRatioRect) se nastavily
// JEDNOU pri startu (viz nap_gles_egl_init, natvrdo 320x240) a uz nikdy
// potom - ale PSXDisplay.DisplayMode se BEZNE meni behem hry (napr. FMV
// sekvence casto bezi ve vyssim rezimu jako 640x480).
// BUILD2SK120: SK119 NESTACILO - opravilo jen vstup do SetOGLDisplaySettings,
// ale ta funkce (precteno CELA, radek po radku, na Reneho vyslovnou zadost
// "prestan testovat porad dokola, poradne to oprav") vola VYHRADNE
// glScissor(). NIKDY se v ni ani nikde jinde v celem gpu-gles nevola
// glViewport() ani glOrtho() (promitaci matice) - obe se nastavuji JEDNOU,
// uvnitr GLinitialize, natvrdo podle tehdejsiho PSXDisplay.DisplayMode
// (320x240 pri nasem startu) a uz NIKDY POTOM. Presne tohle vysvetluje
// VSECHNY pozorovane artefakty najednou: kdyz hra zustala na 320x240,
// vypadalo to spravne (viewport/projekce sedely) - jakmile prepnula na
// jine rozliseni (bezne u FMV), viewport/projekce zustaly stare, scissor
// (diky SK119) uz spravny - vysledek: geometrie se mapuje pres SPATNOU
// projekci do SPRAVNE OREZANE oblasti = zoom, oriznuti, prekryvajici se
// artefakty ("cervena rozmazanina" pod NAUGHTY logem).
// Tahle verze replikuje PRESNE to, co GLinitialize dela pro viewport a
// projekci, ale znovupustitelne - spusti se JEN kdyz se rozliseni SKUTECNE
// zmenilo (ne kazdy jednotlivy snimek zbytecne).
void nap_gles_sync_display_settings(void)
{
  static int lastResX = -1, lastResY = -1;
  int curX = PSXDisplay.DisplayMode.x;
  int curY = PSXDisplay.DisplayMode.y;
  if (curX <= 0 || curY <= 0) return; // jeste nenastaveno smysluplne
  iResX = curX;
  iResY = curY;
  rRatioRect.left = 0; rRatioRect.top = 0;
  rRatioRect.right = iResX; rRatioRect.bottom = iResY;
  if (curX != lastResX || curY != lastResY) {
    // BUILD2SK120: presna kopie viewport+projekce casti z GLinitialize,
    // jen s AKTUALNIMI hodnotami misto tech z doby startu appky.
    glViewport(rRatioRect.left, iResY - (rRatioRect.top + rRatioRect.bottom), rRatioRect.right, rRatioRect.bottom);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, PSXDisplay.DisplayMode.x, PSXDisplay.DisplayMode.y, 0, -1, 1);
    nap_diag_log("BUILD2SK120 GLES_RESOLUTION_RESYNC oldX=%d oldY=%d newX=%d newY=%d", lastResX, lastResY, curX, curY);
    lastResX = curX;
    lastResY = curY;
  }
  SetOGLDisplaySettings(1);
}

static uint32_t *nap_gles_rb_buf = NULL; // BUILD2SK118: ted ARGB8888 primo (drive uint16_t RGB565)
static uint8_t *nap_gles_rb_rgba = NULL; // BUILD2SK105: docasny RGBA8 buffer pro bezpecny readback
static int nap_gles_rb_w = 0, nap_gles_rb_h = 0;
static int nap_gles_frame_count = 0; // BUILD2SK100: tep - kolik snimku uspesne prosel readback

static void nap_gles_readback_and_push(void)
{
 int rb_w = PSXDisplay.DisplayMode.x;
 int rb_h = PSXDisplay.DisplayMode.y;
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
 if (rb_w <= 0 || rb_h <= 0 || rb_w > 2048 || rb_h > 2048) return; // rozumne meze, zadny divoky alloc
 if (nap_gles_rb_buf == NULL || nap_gles_rb_w != rb_w || nap_gles_rb_h != rb_h) {
  if (nap_gles_rb_buf != NULL) free(nap_gles_rb_buf);
  if (nap_gles_rb_rgba != NULL) free(nap_gles_rb_rgba);
  nap_gles_rb_buf = (uint32_t *)malloc((size_t)rb_w * (size_t)rb_h * 4); // BUILD2SK118: ted ARGB8888 primo, ne RGB565
  nap_gles_rb_rgba = (uint8_t *)malloc((size_t)rb_w * (size_t)rb_h * 4); // BUILD2SK105
  nap_gles_rb_w = rb_w;
  nap_gles_rb_h = rb_h;
 }
 if (nap_gles_rb_buf == NULL || nap_gles_rb_rgba == NULL) return; // alokace selhala - proste tenhle snimek preskoc, nic nespadne
 // BUILD2SK105: primo GL_RGBA+GL_UNSIGNED_BYTE - jedina kombinace, kterou
 // GLES specifikace zarucuje pro glReadPixels na jakemkoli zarizeni.
 glReadPixels(0, 0, rb_w, rb_h, GL_RGBA, GL_UNSIGNED_BYTE, nap_gles_rb_rgba);
 // BUILD2SK118: DVE VECI NAJEDNOU v jednom pruchodu:
 // 1) Uz NEPREVADIME dolu na RGB565 a pak zpet nahoru na ARGB v nap_video()
 //    (dva plne pruchody delajici a zase odedelavajici stejnou praci, navic
 //    ztraci barevnou presnost pri 565 zaokrouhlovani) - rovnou balime jako
 //    ARGB8888, presne format, ktery Android/Java uz stejne pozaduje.
 // 2) OTACIME RADKY (Y-flip) - glReadPixels vraci radek 0 jako SPODEK
 //    obrazu (OpenGL ma osu Y nahoru), zatimco Android Bitmap/Canvas ceka
 //    radek 0 jako VRCH (Y dolu) - presne tohle zpusobilo obraz vzhuru
 //    nohama, co Rene videl na screenshotu.
 {
  for (int y = 0; y < rb_h; y++) {
   const uint8_t *srcRow = nap_gles_rb_rgba + (size_t)y * rb_w * 4;
   uint32_t *dstRow = nap_gles_rb_buf + (size_t)(rb_h - 1 - y) * rb_w; // BUILD2SK118: flip - posledni GL radek jde na prvni Android radek
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
  int n = rb_w * rb_h;
  // BUILD2SK118: soucet jasu (ne uz syrovych packed hodnot - ARGB8888 ma
  // konstantni 0xFF v alfa bajtu, ktery by jinak zkresloval soucet).
  for (int i = 0; i < n; i++) {
    uint32_t px = nap_gles_rb_buf[i];
    sum += ((px >> 16) & 0xFF) + ((px >> 8) & 0xFF) + (px & 0xFF);
  }
  uint32_t pTL = nap_gles_rb_buf[0];
  uint32_t pCenter = nap_gles_rb_buf[n / 2];
  uint32_t pBR = nap_gles_rb_buf[n - 1];
  nap_diag_log("BUILD2SK118 GLES_PIXEL_SAMPLE glErr=0x%x sumAvg=%llu pTL=0x%08x pCenter=0x%08x pBR=0x%08x",
    (unsigned)glerr, (unsigned long long)(sum / (n > 0 ? n : 1)), (unsigned)pTL, (unsigned)pCenter, (unsigned)pBR);
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
        int sx = (rx * rb_w) / cols;
        int sy = (ry * rb_h) / rows;
        if (sx >= rb_w) sx = rb_w - 1;
        if (sy >= rb_h) sy = rb_h - 1;
        uint32_t px = nap_gles_rb_buf[sy * rb_w + sx]; // BUILD2SK118: ted ARGB8888
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
  // BUILD2SK110: posledni neoverena cast zakladniho GL stavu - viewport.
  // Scissor uz jsme overili a opravili (SK106/108), alpha test overen a
  // vyloucen jako pricina (SK109, je permisivni). Viewport je JEDINE, co
  // jeste primo neurcuje, KAM se geometrie na obrazovce vlastne premapuje -
  // pokud je i tohle degenerovane (podobne jako byl scissor pred opravou),
  // je to dalsi mozne vysvetleni.
  {
    GLint viewport[4] = {0,0,0,0};
    glGetIntegerv(GL_VIEWPORT, viewport);
    nap_diag_log("BUILD2SK110 GLES_VIEWPORT_CHECK x=%d y=%d w=%d h=%d",
      viewport[0], viewport[1], viewport[2], viewport[3]);
  }
 }
 nap_gles_push_frame(nap_gles_rb_buf, rb_w, rb_h, rb_w * 4); // BUILD2SK118: *4 pro ARGB8888 (drive *2 pro RGB565)
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
