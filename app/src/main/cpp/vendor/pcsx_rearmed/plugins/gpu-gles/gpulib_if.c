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

// BUILD2SK134-fix2: MUSI byt PRED "gpuDraw.c" nize - gpuExternals.h (ktere
// gpuDraw.c/gpuTexture.c/gpuPrim.c samy includuji) uz sam includuje
// <GLES/glext.h> - kdyby GL_GLEXT_PROTOTYPES nebylo definovane driv nez
// TOTO prvni vlozeni, include-guard v glext.h by zablokoval jakekoli
// pozdejsi #include <GLES/glext.h> (i to me, dal v souboru) jako no-op a
// deklarace by se nikdy neobjevily - presne tohle shodilo build podruhe.
#define GL_GLEXT_PROTOTYPES
#include "gpuStdafx.h"
#include "gpuDraw.c"
#include "gpuTexture.c"
#include "gpuPrim.c"
#include "hud.c"
#include <time.h> // BUILD2SK131: presne mereni casu (clock_gettime) - viz nap_now_ms nize

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

// BUILD2SK134: Rene NECHCE tichy navrat ke starem pomalem zpusobu, kdyby
// FBO selhalo ("chci to bez berlicek a ciste... at aplikace klidne
// spadne, ne at tise jede v pomalem modu"). Souhlas s principem - tichy
// fallback by mohl vypadat jako "oprava funguje", kdyz ve skutecnosti
// vubec nenaskocila. Nejde ale o tvrdy pad cele appky (o zvuk, o
// pripadny druhy bezici emulator, o cely TV-cast) kvuli jedne veci, ktera
// realne nikdy neselze (GL_OES_framebuffer_object je na Android GLES1
// standardni pres 15 let) - misto toho: pokud FBO selze, VIDEO se proste
// PRESTANE POSILAT (cerna/zamrzla obrazovka, nic tise "nejak funguje") a
// v logu zustane hlasita, jednoznacna stopa proc. Zadne tise degradovane
// "funguje to, jen pomaleji".
// BUILD2SK133: SKUTECNA oprava mista SK132 berlicky (skip-kazdy-druhy) -
// viz velky komentar u nap_fbo_init_once nize. Misto kresleni primo do
// vychoziho pbufferu (kde kazdy nas glReadPixels MUSI pockat, az GPU
// dokonci VSECHNY cekajici prikazy - tak funguji vsechny mobilni GPU,
// vcetne S8, nezavisle na jejich vykonu) kreslime do dvou vlastnich
// textur/FBO strida se. Cteme vzdy tu DRUHOU (tu, do ktere se kreslilo
// MINULE) - do te doby uz ji GPU davno dokoncilo na pozadi, takze cteni
// je rychle. Plna kvalita, plne rozliseni, zadny preskoceny snimek -
// jen o jeden snimek (~16-33ms, na castovani nepostrehnutelne) zpozdene.
// (GL_GLEXT_PROTOTYPES uz je definovano uplne nahore v souboru, pred vsemi
// #include - viz komentar tam. glext.h uz je tim padem take davno
// vlozeny (transitivne pres gpuExternals.h) s deklaracemi viditelnymi.)
// BUILD2SK144: KORENOVA PRICINA nalezena PRIMO V DATECH, co uz Rene poslal
// (zadny novy test) - GLES_PIXEL_SAMPLE ukazal PRESNE stridajici se
// sumAvg=0 (100% cerna) / sumAvg~180 (normalni obsah), 71x za sebou - ne
// nahodile, ale SYSTEMATICKY. Pricina: SK133-140 nechavaly HRU kreslit
// STRIDAVE do DVOU RUZNYCH fyzickych textur (pro zrychleni cteni) - ale
// hra MA SVOJI VLASTNI dvojitou bufferaci (src_x strida 0/512 VNITR
// JEDNE 1024-sirokou VRAM plochy, viz gpu.c). Tyhle DVE NEZAVISLE
// stridajici se veci se mohly dostat do faze, kdy JEDNA fyzicka textura
// dostavala VZDY jen levou (0-511) polovinu hry a DRUHA vzdy jen pravou
// (512-1023) - a protoze cteme podle AKTUALNIHO src_x (ktere se stridá
// take), casto jsme se ptali na polovinu, do ktere ZROVNA TAHLE textura
// nikdy nedostala nic nakreslit -> cetli jsme cerny (nikdy neprepsany)
// okraj. OPRAVA (zakladni prepracovani): hra ted VZDY kresli do JEDINE,
// STALE textury (nap_canvas_*) - presne jako pred SK133, jeji vlastni
// src_x/512 strida funguje spravne, protoze cil kresleni uz se sam
// nemeni. Cteci vlakno pro zrychleni cteni ted dostava LEVNOU GPU->GPU
// kopii teto canvas do jedne ze 2 "snapshot" textur (nap_fbo_tex[]) -
// PO kazdem snimku, NIKDY tim ale nezasahujeme do toho, kam hra kresli.
static GLuint nap_canvas_tex = 0, nap_canvas_fbo = 0; // JEDINY cil kresleni pro hru - VYTVOREN JEDNOU, NIKDY znovu-nabindovan jinam
static GLuint nap_fbo_tex[2] = {0, 0}; // "snapshot" textury - jen cil GPU->GPU kopie a zdroj pro ctecí vlakno, hra do nich NIKDY nekresli primo
static int nap_fbo_ready = 0;      // BUILD2SK134: 1 = FBO cesta funguje; 0 = video se VUBEC neposila (zadny tichy navrat ke starem zpusobu - viz nap_fbo_init_once)
static int nap_snapshot_idx = 0;  // BUILD2SK144: ktery snapshot index byl NAPOSLED cilem kopie (uz NENI "kam hra kresli" - jen "kam jsme naposled kopirovali")
// BUILD2SK153: nap_fbo_meta_* ODSTRANENO - byla to KORENOVA PRICINA
// stroboskopickeho blikani. Request pro cteci vlakno se plnil z metadat
// ulozenych pri PREDCHOZI kopii do tehoz snapshotu (= 2 snimky stare),
// zatimco glCopyTexSubImage2D do snapshotu prave vlozila AKTUALNI snimek.
// U 60fps her s pravidelnym stridanim src_x 0/512 se to nahodou trefovalo
// (perioda 2 == perioda 2), ale u 30fps her (Crash!) jde src_x po ticich
// 0,0,512,512,... - hodnota stara 2 ticky je PRESNE OPACNA polovina nez
// ta aktualni -> KAZDY DRUHY snimek se cetl z poloviny, do ktere hra
// ZROVNA TED kresli (rozpracovany back buffer) -> stridani dobry/rozbity
// snimek = stroboskop. SK151 "useknuty snimek #6 (horni pulka obsah,
// spodni prazdna)" je presne zachyceny rozpracovany buffer. OPRAVA:
// request VZDY nese CERSTVA metadata (fresh_*) - presne to, co prave
// prislo do snapshotu kopii. Zadna pamet per-snapshot uz neni potreba.
static int nap_last_push_w = 0, nap_last_push_h = 0; // BUILD2SK153: rozmery NAPOSLED odeslaneho snimku - jediny spravny zaklad pro "zmenilo se rozliseni?" (viz do_clear nize)

// BUILD2SK153: zachyt GP1 registru JESTE PRED is_opened kontrolou v
// GPUwriteStatus_ext. GPUopen() se v nasem libretro toku NIKDY nevola
// (viz BUILD2SK102/106 v nap_ps1_native.cpp - inicializace se dela rucne),
// takze is_opened zustava 0 NAVZDY a CELY peops GP1 svet (RGB24 pro FMV
// videa, Disabled, Interlaced) je zamrzly na vychozich hodnotach - to je
// PRESNE duvod SK128 nalezu "Range=[0,0,0,0] po celou dobu". Tady si ty
// bity ulozime sami, bez ozivovani zbytku mrtvych handleru (zadne
// updateDisplay/ChangeDispOffsets kaskady - jen holy stav).
static int nap_disp_rgb24 = 0;      // GP1(08h) bit 4 - 24bit TrueColor (MDEC/FMV videa)
static int nap_disp_interlace = 0;  // GP1(08h) bit 5 - prokladany rezim (jen diagnostika; kreslime vzdy progresivne v plne vysce)
static int nap_disp_disabled = 0;   // GP1(03h) bit 0 - displej vypnuty (behem vypnuti NEPOSILAME snimky - drzime posledni, zadne cerne zablesky)
static int nap_gp1_ext_seen = 0;    // jednorazova diagnostika - potvrdi, ze gpu.c skutecne vola GPUwriteStatus_ext (viz handover: bez toho FMV/RGB24 zustane nefunkcni a je treba hledat jinou cestu)

// BUILD2SK153: obdelnikova obalka (bbox) vseho, co GPU od startu SKUTECNE
// vykreslilo do canvasu (polygony, sprity, fill). Ucel: canvas je
// autoritativni JEN tam, kde GPU neco nakreslilo - VSUDE JINDE (texturove
// stranky, CLUTy!) je autoritativni CPU pole psxVuw. Puvodni SK149
// nap_vram_read_sync cetla canvas BEZ tohoto omezeni - kdyby se nekdy
// spustila (dnes byla mrtva, iFrameReadType=0), prepsala by CPU-nahrana
// texturova data cernou (nikdy nekreslenou) plochou canvasu. Sync se ted
// VZDY orizne na tuhle obalku.
static int nap_render_bbox_x0 = 0, nap_render_bbox_y0 = 0, nap_render_bbox_x1 = 0, nap_render_bbox_y1 = 0; // x1/y1 exkluzivni; prazdne kdyz x0>=x1
static void nap_render_bbox_union(int x0, int y0, int x1, int y1)
{
  if (x0 < 0) x0 = 0;
  if (y0 < 0) y0 = 0;
  if (x1 > NAP_PSX_VRAM_W) x1 = NAP_PSX_VRAM_W;
  if (y1 > NAP_PSX_VRAM_H) y1 = NAP_PSX_VRAM_H;
  if (x0 >= x1 || y0 >= y1) return;
  if (nap_render_bbox_x0 >= nap_render_bbox_x1) { // prazdna -> prvni zapis
    nap_render_bbox_x0 = x0; nap_render_bbox_y0 = y0;
    nap_render_bbox_x1 = x1; nap_render_bbox_y1 = y1;
    return;
  }
  if (x0 < nap_render_bbox_x0) nap_render_bbox_x0 = x0;
  if (y0 < nap_render_bbox_y0) nap_render_bbox_y0 = y0;
  if (x1 > nap_render_bbox_x1) nap_render_bbox_x1 = x1;
  if (y1 > nap_render_bbox_y1) nap_render_bbox_y1 = y1;
}

// BUILD2SK140: RENE VYSLOVNE PRIJAL RIZIKO (ma zalohu Build 96, chce
// hardwarove reseni misto dalsiho osekavani) - presouvame samotne cteni
// z GPU (glReadPixels) na SAMOSTATNE VLAKNO, aby na nej g_worker (vlakno,
// co pocita zvuk - viz retro_set_audio_sample_batch v nap_ps1_native.cpp)
// uz vubec nemuselo cekat. Pouziva se pthread (soubor je cisty C, ne
// C++, kompilator to potvrdil - proto pthread, ne std::thread) a DRUHY
// EGL kontext, ktery SDILI OBJEKTY (textury) s tim hlavnim - zadana,
// standardni technika (eglCreateContext se share_context parametrem).
// DULEZITE: samotne FBO KONTEJNERY (nap_fbo[] vyse) NEJSOU sdilene mezi
// kontexty, i kdyz kontexty sdili objekty - jen TEXTURY (nap_fbo_tex[])
// jsou sdilene. Ctecí vlakno si proto MUSI vytvorit VLASTNI FBO
// kontejnery (nap_reader_fbo[] nize), navazane na TYTEZ (sdilene)
// textury - odtud cte, kontejner hlavniho vlakna (nap_fbo[]) pouziva jen
// pro KRESLENI (write).
#include <pthread.h>
typedef struct {
  int idx;                 // ktery buffer (0/1) se ma cist
  int rb_w, rb_h, src_x, src_y; // co PLATILO, kdyz se do nej naposledy zacalo kreslit (SK133 metadata)
  int do_clear;             // SK139 logika - rozliseni se od minula zmenilo, vycistit po precteni
} nap_reader_req_t;
static pthread_t nap_reader_tid;
static pthread_mutex_t nap_reader_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t nap_reader_cv = PTHREAD_COND_INITIALIZER;
static int nap_reader_thread_launched = 0;
static volatile int nap_reader_has_req = 0;      // g_worker: "tady je novy pozadavek na precteni"
static nap_reader_req_t nap_reader_req;
static volatile int nap_reader_idx_free[2] = {1, 1}; // je tenhle buffer BEZPECNY pro g_worker znovu-nabindovat jako cil kresleni? (ctecí vlakno ho prave nepouziva)
static EGLDisplay nap_reader_disp = EGL_NO_DISPLAY;
static EGLContext nap_reader_ctx = EGL_NO_CONTEXT;
static EGLSurface nap_reader_surf = EGL_NO_SURFACE;
static GLuint nap_reader_fbo[2] = {0, 0}; // BUILD2SK140: VLASTNI FBO kontejnery ctecího vlakna (navazane na SDILENE textury nap_fbo_tex[])
static void *nap_reader_thread_main(void *arg); // definice az za nap_fbo_init_once - forward deklarace, aby ji sla spustit odsud

static void nap_fbo_init_once(void)
{
  static int tried = 0;
  if (tried) return;
  tried = 1;
  // BUILD2SK144: NEJDRIV canvas - JEDINY cil, kam bude hra VZDY kreslit.
  // Zadne stridani, zadne "kam se prave kresli" - presne jako pred SK133,
  // jen offscreen misto primo do pbufferu.
  glGenTextures(1, &nap_canvas_tex);
  glGenFramebuffersOES(1, &nap_canvas_fbo);
  int ok = (nap_canvas_tex && nap_canvas_fbo);
  if (ok) {
    glBindTexture(GL_TEXTURE_2D, nap_canvas_tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebufferOES(GL_FRAMEBUFFER_OES, nap_canvas_fbo);
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, nap_canvas_tex, 0);
    if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
      ok = 0;
      nap_diag_log("BUILD2SK144 GLES_CANVAS_INCOMPLETE_FATAL status=0x%x - VIDEO SE NEBUDE POSILAT", (unsigned)glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
    } else {
      glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
      glClear(GL_COLOR_BUFFER_BIT);
    }
  }
  // BUILD2SK144: 2 "snapshot" textury - JEN cil GPU->GPU kopie z canvasu a
  // zdroj pro ctecí vlakno. Hra do nich nikdy NEKRESLI - proto uz nepotrebuji
  // pro ne zadne FBO na strane g_workeru (jen glBindTexture pro kopii).
  if (ok) {
    glGenTextures(2, nap_fbo_tex);
    ok = (nap_fbo_tex[0] && nap_fbo_tex[1]);
    for (int i = 0; ok && i < 2; i++) {
      glBindTexture(GL_TEXTURE_2D, nap_fbo_tex[i]);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      // BUILD2SK144: rovnou naplnit cernou - musime docasne pripojit k FBO,
      // abychom na ni mohli zavolat glClear (textura sama o sobe neni cil
      // vykreslovacich prikazu) - pouzijeme K TOMU canvas_fbo kontejner
      // (docasne prepneme jeho attachment, pak vratime zpet na canvas_tex).
      glBindFramebufferOES(GL_FRAMEBUFFER_OES, nap_canvas_fbo);
      glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, nap_fbo_tex[i], 0);
      if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) == GL_FRAMEBUFFER_COMPLETE_OES) {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
      }
    }
    // BUILD2SK144: vratit canvas_fbo zpet na SVOJI VLASTNI (jedinou spravnou)
    // texturu - je KRITICKE, aby tenhle radek probehl, driv nez hra zacne
    // cokoliv kreslit.
    glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, nap_canvas_tex, 0);
  }
  if (ok) {
    nap_fbo_ready = 1;
    nap_snapshot_idx = 0;
    // canvas_fbo uz je bindnuty (predchozi blok) - a TAKHLE UZ ZUSTANE, po
    // celou dobu behu appky. Hra do nej muze zacit kreslit uplne normalne.
    nap_diag_log("BUILD2SK144 GLES_CANVAS_READY canvasTex=%u canvasFbo=%u snapshotTex=[%u,%u]", nap_canvas_tex, nap_canvas_fbo, nap_fbo_tex[0], nap_fbo_tex[1]);
    // BUILD2SK140: druhy (sdileny) EGL kontext + pthread pro cteci vlakno.
    // eglGetCurrentDisplay/Context vraci to, co uz g_worker (tohle vlakno)
    // nastavilo v nap_gles_egl_init (v nap_ps1_native.cpp) - zadne dalsi
    // propojeni mezi soubory netreba, jen se ptame EGL na to, co uz je
    // "current" prave TADY.
    EGLDisplay disp = eglGetCurrentDisplay();
    EGLContext mainCtx = eglGetCurrentContext();
    EGLint cfgAttribs[] = { EGL_SURFACE_TYPE, EGL_PBUFFER_BIT, EGL_RENDERABLE_TYPE, EGL_OPENGL_ES_BIT, EGL_RED_SIZE, 8, EGL_GREEN_SIZE, 8, EGL_BLUE_SIZE, 8, EGL_ALPHA_SIZE, 8, EGL_NONE };
    EGLConfig cfg; EGLint numCfg = 0;
    int readerSetupOk = eglChooseConfig(disp, cfgAttribs, &cfg, 1, &numCfg) && numCfg >= 1;
    if (!readerSetupOk) nap_diag_log("BUILD2SK140 GLES_READER_CHOOSECONFIG_FAIL err=0x%x", eglGetError());
    if (readerSetupOk) {
      EGLint pbufAttribs[] = { EGL_WIDTH, 1, EGL_HEIGHT, 1, EGL_NONE };
      nap_reader_surf = eglCreatePbufferSurface(disp, cfg, pbufAttribs);
      if (nap_reader_surf == EGL_NO_SURFACE) { readerSetupOk = 0; nap_diag_log("BUILD2SK140 GLES_READER_PBUFFER_FAIL err=0x%x", eglGetError()); }
    }
    if (readerSetupOk) {
      EGLint ctxAttribs[] = { EGL_NONE }; // GLES1 - zadna EGL_CONTEXT_CLIENT_VERSION potreba (to je jen pro GLES2/3)
      nap_reader_ctx = eglCreateContext(disp, cfg, mainCtx, ctxAttribs); // BUILD2SK140: mainCtx jako share_context - TOHLE sdili textury
      if (nap_reader_ctx == EGL_NO_CONTEXT) { readerSetupOk = 0; nap_diag_log("BUILD2SK140 GLES_READER_CREATECONTEXT_FAIL err=0x%x", eglGetError()); }
    }
    if (readerSetupOk) {
      nap_reader_disp = disp;
      if (pthread_create(&nap_reader_tid, NULL, nap_reader_thread_main, NULL) != 0) {
        readerSetupOk = 0;
        nap_diag_log("BUILD2SK140 GLES_READER_PTHREAD_CREATE_FAIL_FATAL");
      } else {
        nap_reader_thread_launched = 1;
      }
    }
    if (!readerSetupOk) {
      // BUILD2SK140: STEJNA hlasita filozofie jako SK134 - kdyz se
      // ctecí vlakno nepodari rozjet, VIDEO SE VUBEC NEPOSILA (misto
      // tichého navratu k puvodnimu primemu cteni na g_worker, ktere by
      // zase blokovalo zvuk). nap_fbo_ready se vraci na 0 - readback_and_push
      // uz o par radku vyse pak vzdy skonci hned na zacatku.
      nap_fbo_ready = 0;
      nap_diag_log("BUILD2SK140 GLES_READER_SETUP_FAILED_FATAL - video se NEBUDE posilat");
    }
  } else {
    // BUILD2SK134: ZADNY tichy navrat - nap_fbo_ready zustava 0 a
    // nap_gles_readback_and_push se ted na tenhle stav primo pta a
    // NEPOSLE zadny snimek (viz tam). Hlasite, jednoznacne, videt na
    // prvni pohled (obraz proste zustane cerny/zamrzly), ne skryte za
    // tise fungujicim pomalym rezimem.
    nap_diag_log("BUILD2SK134 GLES_FBO_INIT_FAILED_FATAL - video se NEBUDE posilat, dokud se tohle nevyresi");
  }
}

// BUILD2SK154: pevna VRAM-space projekce/viewport/scissor na JEDNOM miste.
// Vola se (a) kazdy tick ze sync_display_settings, (b) HNED po kazdem
// nap_upload_vram_rect - vendor UploadScreen totiz uvnitr vola
// SetOGLDisplaySettings(0) a dalsi stav z gpuDraw.c (soubor nemame v
// balicku), takze po nem stav radeji VZDY vratime do znameho tvaru, aby
// vsechno kreslene POTOM ve stejnem ticku melo zarucene spravnou projekci.
static void nap_gles_apply_fixed_display(void)
{
  iResX = NAP_PSX_VRAM_W;
  iResY = NAP_PSX_VRAM_H;
  rRatioRect.left = 0; rRatioRect.top = 0;
  rRatioRect.right = NAP_PSX_VRAM_W; rRatioRect.bottom = NAP_PSX_VRAM_H;
  PSXDisplay.DisplayMode.x = NAP_PSX_VRAM_W;  // BUILD2SK128: kvuli XS=YS=1 ve scissor vypoctu
  PSXDisplay.DisplayMode.y = NAP_PSX_VRAM_H;
  glViewport(0, 0, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H);
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H, 0, -1, 1);
  glMatrixMode(GL_MODELVIEW); // BUILD2SK128: vratit rezim matice tak, jak ho necha GLinitialize
  glLoadIdentity(); // BUILD2SK153: MODELVIEW identita kazdy tick - pojistka
  static int nap_proj_logged = 0;
  if (!nap_proj_logged) {
    nap_proj_logged = 1;
    nap_diag_log("BUILD2SK128 GLES_VRAM_PROJECTION_EVERY_TICK w=%d h=%d", NAP_PSX_VRAM_W, NAP_PSX_VRAM_H);
  }
  SetOGLDisplaySettings(1); // scissor - od SK128 pocita 1:1 ve VRAM souradnicich
}

void nap_gles_sync_display_settings(void)
{
 nap_fbo_init_once(); // BUILD2SK133: jen jednou, levne po zbytek behu
  // BUILD2SK129: NALEZ Z RENEHO LOGU (SK128 test, Crash Bandicoot) -
  // BUILD2SK108 GLES_SCISSOR_CHECK hlasi napr. "y=284 w=512 h=216" (= PS1
  // radky ~12-227), zatimco BUILD2SK128 GLES_READ_ANCHOR ve STEJNEM okamziku
  // cte "rb_h=240" (= PS1 radky 0-239). Hra (jeji GP0 DrawArea) fyzicky
  // NIKDY nekresli do radku 0-11 a 228-239 - to je LEGITIMNI (asi
  // overscan okraj), scissor to spravne respektuje. My ale ten okraj
  // PRESTO cteme, protoze cteme cely gpu.screen.vres. Cokoliv tam ve VRAM
  // zbylo od BIOS/boot loga (GL surface neni na zacatku zarucene cerny -
  // driver muze vratit libovolna "smetiste" data) se tak dostane do
  // kazdeho snimku - a protoze oba pultove buffery (x=0 i x=512) maji
  // STEJNE uzky DrawArea (log to potvrzuje pro obe poloviny), zustava tam
  // navzdy. To presne odpovida hlaseni "dole prosvita PS1 intro" a prispiva
  // k blikani (pokud se ten smetiskovy obsah mezi snimky/pulkami lisi).
  // OPRAVA: JEDNORAZOVE (ne kazdy snimek - zbytecne drahe a nase VRAM-
  // -space plocha se mimo tenhle okraj stejne pak porad prepisuje hrou)
  // vycistit CELOU VRAM-space plochu na cernou HNED PRI PRVNIM volani teto
  // funkce - tedy driv, nez cokoliv vubec nakreslilo prvni snimek. Scissor
  // musime na chvili vypnout, jinak by glClear() vycistil jen aktualni
  // (uzsi) vyrez, ne celou plochu.
  // BUILD2SK134: SK130 "vycisti prvnich par volani do vychoziho pbufferu"
  // heuristika CELA ODSTRANENA (ne jen zakomentovana pro fallback pripad,
  // jak byla v SK133) - Rene nechce zadnou tichou zachrannou cestu pres
  // puvodni pbuffer. Kdyz FBO selze, nic se do pbufferu stejne nikdy
  // nebude cist (viz nap_gles_readback_and_push - hlasi FATAL a vraci se
  // hned na zacatku), takze cistit ho by bylo cisté plytvani.
  // BUILD2SK128: nastavovat KAZDY tick (drive jen jednou pres flag) - je to
  // par levnych GL volani a je to pojistka proti cemukoli, co by projekci/
  // viewport mezitim prenastavilo (updateDisplayIfChanged/SetAspectRatio
  // maji vlastni glOrtho/glViewport - dnes jsou to mrtve cesty, ale nechceme
  // na tom stavet).
  nap_gles_apply_fixed_display(); // BUILD2SK154: telo presunuto do pomocne funkce - stejnou obnovu potrebujeme i po kazdem nap_upload_vram_rect (viz nize)
}


static uint32_t *nap_gles_rb_buf = NULL; // BUILD2SK118: ted ARGB8888 primo (drive uint16_t RGB565)
static int nap_gles_rb_w = 0, nap_gles_rb_h = 0;
static int nap_gles_frame_count = 0; // BUILD2SK100: tep - kolik snimku uspesne prosel readback

static uint8_t *nap_gles_vram_rgba = NULL; // BUILD2SK122: FIXNI 1024x512x4 buffer - cela VRAM, alokovano jen jednou

// BUILD2SK131: presny cas v ms (monotonic - nezavisi na systemovych hodinach).
// Duvod pridani: SK112/SK117 historie v teto funkci (viz komentar nize u
// glReadPixels) uz JEDNOU empiricky potvrdila, ze blokujici GPU volani na
// TOMHLE vlakne postupne zhorsuje zvuk (g_worker vlakno v nap_ps1_native.cpp
// generuje zvuk PRIMO behem emulace - viz retro_set_audio_sample_batch - a
// je to STEJNE vlakno, co tady cte pixely). glReadPixels je uz ze sve
// podstaty blokujici, takze i bez glFinish() navic muze byt hlavnim
// zdrojem prave zmerenych audio underrunu (~34-59% smycek). Misto dalsiho
// hadani: presne zmerit, kolik ms tohle volani a nasledujici prevod
// SKUTECNE stoji, na TOMTO konkretnim telefonu.
static double nap_now_ms(void)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

static void *nap_reader_thread_main(void *arg)
{
 (void)arg;
 // BUILD2SK140: PRVNI VEC na tomhle vlakne - navazat NAS VLASTNI kontext
 // (sdili textury s hlavnim, ale je to jiny kontext - proto ho musime
 // sami udelat "current" tady, presne jak to hlavni vlakno delalo se
 // svym kontextem v nap_gles_egl_init).
 if (!eglMakeCurrent(nap_reader_disp, nap_reader_surf, nap_reader_surf, nap_reader_ctx)) {
  nap_diag_log("BUILD2SK140 GLES_READER_MAKECURRENT_FAIL_FATAL err=0x%x - ctecí vlakno se nespustilo, video se nebude posilat", eglGetError());
  return NULL;
 }
 // BUILD2SK140: FBO KONTEJNERY nejsou mezi kontexty sdilene (i kdyz
 // kontext sdili objekty) - jen SAMOTNE TEXTURY jsou. Proto si tady
 // vytvarime VLASTNI 2 FBO, navazane na TYTEZ (sdilene) textury
 // nap_fbo_tex[] - hlavni vlakno kresli pres SVE fbo kontejnery
 // (nap_fbo[]), tenhle kontejner tady je JEN pro cteni ze stejne
 // podkladove textury.
 glGenFramebuffersOES(2, nap_reader_fbo);
 int reader_ok = (nap_reader_fbo[0] && nap_reader_fbo[1]);
 for (int i = 0; reader_ok && i < 2; i++) {
  glBindFramebufferOES(GL_FRAMEBUFFER_OES, nap_reader_fbo[i]);
  glFramebufferTexture2DOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_TEXTURE_2D, nap_fbo_tex[i], 0);
  if (glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES) != GL_FRAMEBUFFER_COMPLETE_OES) {
   reader_ok = 0;
   nap_diag_log("BUILD2SK140 GLES_READER_FBO_INCOMPLETE_FATAL i=%d status=0x%x", i, (unsigned)glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES));
  }
 }
 if (!reader_ok) {
  nap_diag_log("BUILD2SK140 GLES_READER_INIT_FAILED_FATAL - video se nebude posilat");
  return NULL;
 }
 nap_diag_log("BUILD2SK140 GLES_READER_THREAD_READY fbo=[%u,%u]", nap_reader_fbo[0], nap_reader_fbo[1]);
 for (;;) {
  nap_reader_req_t req;
  pthread_mutex_lock(&nap_reader_mtx);
  while (!nap_reader_has_req) pthread_cond_wait(&nap_reader_cv, &nap_reader_mtx);
  req = nap_reader_req;
  nap_reader_has_req = 0;
  pthread_mutex_unlock(&nap_reader_mtx);

  int rb_w = req.rb_w, rb_h = req.rb_h, src_x = req.src_x, src_y = req.src_y;
  // BUILD2SK118: FIXNI 1024x512x4 buffer - cela VRAM, alokovano jen jednou.
  // BUILD2SK140: alokace (rb_buf/vram_rgba) ted zije JEN na tomhle vlakne -
  // uz do ni nikdy nesahne g_worker, takze zadne zamykani netreba.
  if (nap_gles_rb_buf == NULL || nap_gles_rb_w != rb_w || nap_gles_rb_h != rb_h) {
   if (nap_gles_rb_buf != NULL) free(nap_gles_rb_buf);
   nap_gles_rb_buf = (uint32_t *)malloc((size_t)rb_w * (size_t)rb_h * 4);
   nap_gles_rb_w = rb_w;
   nap_gles_rb_h = rb_h;
  }
  if (nap_gles_vram_rgba == NULL) {
   nap_gles_vram_rgba = (uint8_t *)malloc((size_t)NAP_PSX_VRAM_W * (size_t)NAP_PSX_VRAM_H * 4);
  }
  if (nap_gles_rb_buf == NULL || nap_gles_vram_rgba == NULL) {
   // alokace selhala - tenhle pozadavek preskoc, uvolni buffer pro g_worker at neuvizne v cekani
   pthread_mutex_lock(&nap_reader_mtx);
   nap_reader_idx_free[req.idx] = 1;
   pthread_cond_broadcast(&nap_reader_cv);
   pthread_mutex_unlock(&nap_reader_mtx);
   continue;
  }
  double nap_t0 = 0.0, nap_t1 = 0.0, nap_t2 = 0.0;
  {
   if (src_x < 0) src_x = 0;
   if (src_y < 0) src_y = 0;
   if (src_x + rb_w > NAP_PSX_VRAM_W) src_x = NAP_PSX_VRAM_W - rb_w;
   if (src_x < 0) src_x = 0;
   int glY = NAP_PSX_VRAM_H - (src_y + rb_h);
   if (glY < 0) glY = 0;
   if (glY + rb_h > NAP_PSX_VRAM_H) glY = NAP_PSX_VRAM_H - rb_h;
   if (glY < 0) glY = 0;
   glBindFramebufferOES(GL_FRAMEBUFFER_OES, nap_reader_fbo[req.idx]); // BUILD2SK140: NAS VLASTNI kontejner, stejna sdilena textura jako g_worker prave zapisuje do TE DRUHE
   nap_t0 = nap_now_ms();
   glReadPixels(src_x, glY, rb_w, rb_h, GL_RGBA, GL_UNSIGNED_BYTE, nap_gles_vram_rgba);
   nap_t1 = nap_now_ms();
   // BUILD2SK153: cisteni SNAPSHOTU tady ODSTRANENO - snapshot se pri
   // KAZDEM snimku kompletne prepisuje glCopyTexSubImage2D pres CELOU
   // plochu 1024x512 (viz nap_gles_readback_and_push), takze v nem nikdy
   // nemuze "viset" stary obsah. Cisteni CANVASU pri zmene rozliseni
   // (SK129 border-bleed ochrana) zustava - ale dela se na hlavnim vlakne,
   // JEDNOU na zmenu (viz tamtez), ne 2x jako drive (per-snapshot
   // porovnani vystrelilo pro oba snapshoty po sobe a druhy glClear uz
   // smazal cerstve nakresleny snimek nove sceny -> cerny zablesk pri
   // kazdem prepnuti menu<->hra).
   if (nap_gles_frame_count % 31 == 1) {
    nap_diag_log("BUILD2SK140 GLES_READ_ANCHOR srcX=%d srcY=%d glY=%d rb_w=%d rb_h=%d readIdx=%d", src_x, src_y, glY, rb_w, rb_h, req.idx);
   }
  }
  // BUILD2SK140: precteno - buffer uz je BEZPECNY pro g_worker znovu
  // pouzit jako cil kresleni (VSECHNY GPU operace nad nim - cteni i
  // pripadne cisteni - uz jsou hotove, glReadPixels/glClear jsou
  // synchronni). Signalizujeme HNED, driv nez CPU-only zpracovani nize
  // (flip/diagnostika/push) - g_worker tim nemusi cekat na CPU cast,
  // jen na tu GPU cast, ktera uz je hotova.
  pthread_mutex_lock(&nap_reader_mtx);
  nap_reader_idx_free[req.idx] = 1;
  pthread_cond_broadcast(&nap_reader_cv);
  pthread_mutex_unlock(&nap_reader_mtx);

  {
   for (int y = 0; y < rb_h; y++) {
    const uint8_t *srcRow = nap_gles_vram_rgba + (size_t)y * rb_w * 4;
    uint32_t *dstRow = nap_gles_rb_buf + (size_t)(rb_h - 1 - y) * rb_w;
    for (int x = 0; x < rb_w; x++) {
     uint8_t r = srcRow[x * 4 + 0];
     uint8_t g = srcRow[x * 4 + 1];
     uint8_t b = srcRow[x * 4 + 2];
     dstRow[x] = 0xFF000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
    }
   }
  }
  nap_t2 = nap_now_ms();
  {
   static double nap_read_ms_sum = 0.0, nap_flip_ms_sum = 0.0;
   static int nap_timing_n = 0;
   nap_read_ms_sum += (nap_t1 - nap_t0);
   nap_flip_ms_sum += (nap_t2 - nap_t1);
   nap_timing_n++;
   if (nap_timing_n >= 30) {
    nap_diag_log("BUILD2SK140 GLES_READBACK_TIMING avgReadMs=%.2f avgFlipMs=%.2f avgTotalMs=%.2f n=%d rb_w=%d rb_h=%d (uz MIMO g_worker/audio vlakno)",
      nap_read_ms_sum / nap_timing_n, nap_flip_ms_sum / nap_timing_n,
      (nap_read_ms_sum + nap_flip_ms_sum) / nap_timing_n, nap_timing_n, rb_w, rb_h);
    nap_read_ms_sum = 0.0; nap_flip_ms_sum = 0.0; nap_timing_n = 0;
   }
  }
  // BUILD2SK151: RENE ZACHYTIL KONKRETNI USEKNUTY SNIMEK (#6 z 8 - horni
  // polovina obsah, spodni prazdna) - ale vzorkovani kazdych 31 snimku
  // nam z toho da jen 8 vzorku CELKEM za celou session, prilis rídké na
  // to, aby bylo videt, jak casto/pravidelne se to deje. Behem PRVNICH
  // 200 snimku (pokryje cely intro) vzorkujeme KAZDY tick - cistě
  // mereni, zadna zmena chovani.
  if (nap_gles_frame_count < 400 || nap_gles_frame_count % 31 == 1) { // BUILD2SK152: 200->400, aby to sahalo za tu cernou/rampovaci fazi az do casti, kde uz je videt (nebo neni) skutecny tvar loga
   GLenum glerr = glGetError();
   unsigned long long sum = 0;
   int n = rb_w * rb_h;
   int nearBlack = 0;
   for (int i = 0; i < n; i++) {
     uint32_t px = nap_gles_rb_buf[i];
     int r = (px >> 16) & 0xFF, g = (px >> 8) & 0xFF, b = px & 0xFF;
     sum += (unsigned)(r + g + b);
     if (r + g + b < 6) nearBlack++;
   }
   uint32_t pTL = nap_gles_rb_buf[0];
   uint32_t pCenter = nap_gles_rb_buf[n / 2];
   uint32_t pBR = nap_gles_rb_buf[n - 1];
   nap_diag_log("BUILD2SK151 GLES_PIXEL_SAMPLE glErr=0x%x sumAvg=%llu nearBlackPct=%d pTL=0x%08x pCenter=0x%08x pBR=0x%08x dispW=%d dispH=%d n=%d",
     (unsigned)glerr, (unsigned long long)(sum / (n > 0 ? n : 1)), (n > 0 ? (nearBlack * 100 / n) : -1), (unsigned)pTL, (unsigned)pCenter, (unsigned)pBR,
     rb_w, rb_h, nap_gles_frame_count);
   static int nap_ascii_dump_count = 0;
   if (nap_ascii_dump_count < 150) { // BUILD2SK152: 30 -> 150 - minule se vycerpalo jeste v cerne/rampovaci fazi (snimky 1-30), potrebujeme sahat az za snimek ~110, kde uz jas znatelne roste
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
         uint32_t px = nap_gles_rb_buf[px_y * rb_w + px_x];
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
   // BUILD2SK140: GLES_ALPHA_TEST_CHECK ZDE VYNECHANO - je to dotaz na GL
   // STAV (glIsEnabled/glGetIntegerv), ktery je PER-KONTEXT, ne sdileny.
   // Na tomhle (ctecim) kontextu by vracelo jen jeho VLASTNI (nepouzivany,
   // vychozi) stav, ne stav hlavniho kresliciho kontextu - zavadejici
   // cislo, radsi zadne nez spatne.
   nap_diag_log("BUILD2SK121 PSXDISPLAY_STATE DrawArea=[%d,%d,%d,%d] DispPos=[%d,%d] Range=[%d,%d,%d,%d] DrawOffset=[%d,%d] Interlaced=%d Disabled=%d RGB24=%d",
     (int)PSXDisplay.DrawArea.x0, (int)PSXDisplay.DrawArea.y0, (int)PSXDisplay.DrawArea.x1, (int)PSXDisplay.DrawArea.y1,
     PSXDisplay.DisplayPosition.x, PSXDisplay.DisplayPosition.y,
     (int)PSXDisplay.Range.x0, (int)PSXDisplay.Range.y0, (int)PSXDisplay.Range.x1, (int)PSXDisplay.Range.y1,
     (int)PSXDisplay.DrawOffset.x, (int)PSXDisplay.DrawOffset.y,
     PSXDisplay.Interlaced, PSXDisplay.Disabled, PSXDisplay.RGB24);
   nap_diag_log("BUILD2SK121 PREVPSXDISPLAY_STATE DispPos=[%d,%d] Range=[%d,%d,%d,%d]",
     PreviousPSXDisplay.DisplayPosition.x, PreviousPSXDisplay.DisplayPosition.y,
     (int)PreviousPSXDisplay.Range.x0, (int)PreviousPSXDisplay.Range.y0, (int)PreviousPSXDisplay.Range.x1, (int)PreviousPSXDisplay.Range.y1);
   {
     int ry = rb_h / 2, cx = rb_w / 2;
     int rawIdx = (ry * rb_w + cx) * 4;
     int finIdx = (rb_h - 1 - ry) * rb_w + cx;
     nap_diag_log("BUILD2SK130 RAW_VS_FINAL rawRGBA=[%d,%d,%d,%d] finalARGB=0x%08x",
       (int)nap_gles_vram_rgba[rawIdx+0], (int)nap_gles_vram_rgba[rawIdx+1], (int)nap_gles_vram_rgba[rawIdx+2], (int)nap_gles_vram_rgba[rawIdx+3],
       (unsigned)nap_gles_rb_buf[finIdx]);
   }
  }
  nap_gles_push_frame(nap_gles_rb_buf, rb_w, rb_h, rb_w * 4);
 }
 return NULL;
}

static int nap_gles_readback_and_push(void) // BUILD2SK154: vraci 1=snimek predan ctecce, 0=preskoceno/nepripraveno - volajici pak NEnuluje iDrawnSomething a zkusi to znovu pristi tick
{
 // BUILD2SK128: velikost i pozice cteni uz JEN z autoritativniho gpulib
 // stavu (gpu.screen - plneno primo z GP1 prikazu v gpu.c), ne ze zamrzleho
 // peops DisplayMode.
 int fresh_w = 0, fresh_h = 0, fresh_sx = 0, fresh_sy = 0;
 nap_gpulib_display_info(&fresh_sx, &fresh_sy, &fresh_w, &fresh_h);
 if (fresh_w <= 0) fresh_w = 320;
 if (fresh_h <= 0) fresh_h = 240;
 nap_gles_frame_count++;
 if (nap_gles_frame_count % 31 == 1) {
  nap_diag_log("BUILD2SK100 GLES_FRAME_HEARTBEAT n=%d dispW=%d dispH=%d", nap_gles_frame_count, fresh_w, fresh_h);
 }
 if (fresh_w <= 0 || fresh_h <= 0 || fresh_w > NAP_PSX_VRAM_W || fresh_h > NAP_PSX_VRAM_H) return 0;
 if (!nap_fbo_ready) return 0;
 // BUILD2SK144: g_worker uz TADY NEDELA ZADNE cteni z GPU (stejne jako
 // SK140) - ale uz TAKY NEPREHAZUJE, kam hra kresli (to byla ta chyba,
 // viz velky komentar u globalnich promennych). Hra porad kresli do
 // JEDINEHO canvasu (uz bindnuty, nikdy se nemeni). Tady jen: (1) levna
 // GPU->GPU kopie aktualniho obsahu canvasu do jednoho ze 2 "snapshot"
 // policek, (2) domluva s ctecim vlaknem, at tenhle snapshot zpracuje.
 int other_idx = 1 - nap_snapshot_idx;
 int dims_changed = (nap_last_push_w != 0 && (fresh_w != nap_last_push_w || fresh_h != nap_last_push_h)); // BUILD2SK153: proti NAPOSLED ODESLANEMU snimku (globalne), ne proti 2 snimky staremu stavu jednoho snapshotu
 pthread_mutex_lock(&nap_reader_mtx);
 // BUILD2SK154: NIKDY NEBLOKOVAT g_worker (vlakno se zvukem!). SK153 a vse
 // pred nim tady CEKALO, az ctecí vlakno dokonci predchozi pozadavek.
 // Reneho log 2026-07-22 ale ukazal presny retez selhani: u 640x480 trva
 // glReadPixels na ctecim vlakne ~6.3ms (GLES_READBACK_TIMING avgReadMs=6.28,
 // soubezne s TV PixelCopy spickou) -> cekani tady nafouklo avgDispMs az na
 // 7.7ms -> tick prekrocil 16.6ms rozpocet -> jadro zapnulo frameskip ->
 // u obsahu kresleneho ve 2 pruchodech (BIOS: smaz/vykresli) vypadaval kazdy
 // druhy pruchod -> odesilaly se snimky zachycene PO smazani (cerne,
 // alpha=0) s obcasnym kompletnim pri preskoku parity - presne "blikajici"
 // SCE obrazovka + 2829 audio underrunu. OPRAVA: kdyz ctecka jeste pracuje,
 // tenhle snimek proste NEODESLEME (return 0) - obraz na TV/telefonu podrzi
 // predchozi snimek (nepostrehnutelne), emulace a zvuk bezi dal bez cekani.
 if (nap_reader_has_req || !nap_reader_idx_free[other_idx]) {
  pthread_mutex_unlock(&nap_reader_mtx);
  {
   static int nap_skip_count = 0;
   nap_skip_count++;
   if (nap_skip_count <= 10 || (nap_skip_count % 120 == 0)) {
    nap_diag_log("BUILD2SK154 GLES_PRESENT_SKIP_BUSY n=%d (ctecka jeste cte predchozi snimek - tenhle vynechavame, zadne cekani)", nap_skip_count);
   }
  }
  return 0;
 }
 nap_reader_idx_free[other_idx] = 0; // zabirame ho pro NOVY pozadavek
 // BUILD2SK153: KLICOVA OPRAVA STROBOSKOPU - request nese VZDY CERSTVA
 // metadata (co prave TED plati v gpu.screen a co za okamzik prijde do
 // snapshotu kopii nize). Drive se tady kopirovala metadata z minule kopie
 // do tehoz snapshotu (2 snimky stara) - u 30fps her (src_x po ticich
 // 0,0,512,512,...) to znamenalo cist KAZDY DRUHY snimek z opacne,
 // prave rozkreslene poloviny VRAM. Viz velky komentar u byvalych
 // nap_fbo_meta_* poli.
 nap_reader_req.idx = other_idx;
 nap_reader_req.rb_w = fresh_w;
 nap_reader_req.rb_h = fresh_h;
 nap_reader_req.src_x = fresh_sx;
 nap_reader_req.src_y = fresh_sy;
 nap_reader_req.do_clear = 0; // BUILD2SK153: cisteni snapshotu na ctecim vlakne zruseno (plna kopie ho prepisuje cely) - pole zustava jen kvuli stabilite struktury
 pthread_mutex_unlock(&nap_reader_mtx); // BUILD2SK144: odemknout PRED GL praci nize - mutex je jen pro sdilena data, ne pro GL volani
 // BUILD2SK144: LEVNA GPU->GPU kopie - canvas_fbo je porad bindnuty (jako
 // vzdy), takze glCopyTexSubImage2D cte primo z NEJ do snapshot textury.
 // Zadny CPU prenos, zadne cekani na "je GPU hotove" - jen GPU->GPU
 // presun, typicky < 1ms, a hlavne: NEDOTYKA se toho, kam hra kresli.
 glBindTexture(GL_TEXTURE_2D, nap_fbo_tex[other_idx]);
 // BUILD2SK154: kopirovat jen VYREZ, ktery ctecka skutecne precte (stejna
 // matematika orezu jako v nap_reader_thread_main, vcetne prevraceni Y) -
 // u 640x480 je to 60 % plochy, u 512x240 jen 23 % -> mene prace pro GPU
 // presne ve chvilich, kdy je ho potreba setrit. Snapshot mimo tento vyrez
 // muze byt stary - ctecka ho ale NIKDY necte (cte presne tyhle souradnice).
 {
  int cp_x = fresh_sx, cp_y = fresh_sy, cp_w = fresh_w, cp_h = fresh_h;
  if (cp_x < 0) cp_x = 0;
  if (cp_y < 0) cp_y = 0;
  if (cp_x + cp_w > NAP_PSX_VRAM_W) cp_x = NAP_PSX_VRAM_W - cp_w;
  if (cp_x < 0) cp_x = 0;
  int cp_glY = NAP_PSX_VRAM_H - (cp_y + cp_h);
  if (cp_glY < 0) cp_glY = 0;
  if (cp_glY + cp_h > NAP_PSX_VRAM_H) cp_glY = NAP_PSX_VRAM_H - cp_h;
  if (cp_glY < 0) cp_glY = 0;
  glCopyTexSubImage2D(GL_TEXTURE_2D, 0, cp_x, cp_glY, cp_x, cp_glY, cp_w, cp_h);
 }
 // BUILD2SK153: glFlush zarucuje, ze kopie vyse bude viditelna i z DRUHEHO
 // (cteciho) EGL kontextu, ktery sdili textury s timhle. Drive tuhle roli
 // "nahodou" plnil eglSwapBuffers v updateFrontDisplay - ktery je ale na
 // pbufferu podle EGL specifikace no-op, takze se spolehalo na stesti
 // ovladace. glFlush NENI glFinish - NECEKA na dokonceni (zadny navrat
 // SK112/117 problemu s blokovanim zvukoveho vlakna), jen odesle frontu.
 glFlush();
 // BUILD2SK144/153: SK129/139 border-bleed ochrana - pri zmene rozliseni
 // vycistit CANVAS (jediny trvaly povrch, kde stara scena muze "viset" do
 // nove). AZ PO kopii vyse, takze posledni platny snimek stare sceny se
 // jeste stihl zachytit. NOVE (SK153): porovnava se proti naposled
 // ODESLANEMU snimku -> vystreli PRESNE JEDNOU na kazdou zmenu rozliseni.
 // Drive per-snapshot porovnani vystrelilo 2x po sobe a druhy glClear uz
 // mazal cerstve nakresleny snimek NOVE sceny -> cerny zablesk pri kazdem
 // prepnuti menu<->hra (Reneho hlaseni k bodu "zmena rozliseni").
 if (dims_changed) {
  glDisable(GL_SCISSOR_TEST); // BUILD2SK154: glClear respektuje scissor - v tenhle okamzik je na hernim vyrezu, cistit chceme CELY canvas
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
  glEnable(GL_SCISSOR_TEST);
  // canvas je po vycisteni cely cerny -> nic z drivejsiho GPU kresleni uz
  // neexistuje -> obalka vykreslene plochy se musi vynulovat, jinak by
  // nap_vram_read_sync mohla cist cernou plochu jako "platny" obsah.
  nap_render_bbox_x0 = nap_render_bbox_y0 = nap_render_bbox_x1 = nap_render_bbox_y1 = 0;
  nap_diag_log("BUILD2SK153 GLES_RES_CHANGE_CANVAS_CLEAR old=%dx%d new=%dx%d", nap_last_push_w, nap_last_push_h, fresh_w, fresh_h);
 }
 nap_last_push_w = fresh_w;
 nap_last_push_h = fresh_h;
 nap_snapshot_idx = other_idx;
 pthread_mutex_lock(&nap_reader_mtx);
 nap_reader_has_req = 1;
 pthread_cond_broadcast(&nap_reader_cv);
 pthread_mutex_unlock(&nap_reader_mtx);
 return 1; // BUILD2SK154: snimek skutecne predan ctecce
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

// BUILD2SK149: PUVODNE prazdne no-op funkce ("don't do GL vram read").
// Nalezeno primo v kodu (ne dohadem): kdyz hra dela VRAM->VRAM kopii
// (napr. MoveImageWrapped v gpuPrim.c - PS1 efekty, presuny obrazu),
// kopiruje se z CPU strany pole psxVuw (16-bit "stinova" kopie VRAM,
// ze ktere se taky stavi vsechny GL textury - viz gpuTexture.c). Tyhle
// dve prazdne funkce mely PRED takovou kopii zajistit, ze psxVuw
// SKUTECNE obsahuje to, co uz je vykresleno pres OpenGL - bez toho
// kopie pracuje se starymi/nulovymi daty mist se skutecnym obsahem,
// coz muze vypadat presne jako blikani/chybejici casti obrazu.
// OPRAVA: precist danou oblast z (porad bindnuteho) canvasu a prevest
// na PS1 nativni 16-bit format primo do psxVuw. Volá se jen pri
// konkretnich, cilenych operacich (ne kazdy snimek jako hlavni cteni),
// takze synchronni glReadPixels tady NEMA stejny dopad na zvuk, jaky
// mela puvodni kazde-snimkova verze.
static uint8_t *nap_vramread_tmp = NULL;
static size_t nap_vramread_tmp_cap = 0;
static void nap_vram_read_sync(int x, int y, int dx, int dy)
{
  if (!nap_fbo_ready) return;
  // BUILD2SK153: KRITICKE OMEZENI - synchronizovat z canvasu do psxVuw se
  // smi JEN uvnitr obalky skutecne GPU-vykreslene plochy (nap_render_bbox).
  // Canvas je autoritativni pouze tam, kde GPU neco nakreslilo; vsude jinde
  // (texturove stranky, CLUTy - typicky prava/spodni cast VRAM) je
  // autoritativni CPU pole psxVuw a canvas tam ma jen cernou (nikdy
  // neprepsanou) vypln. Puvodni SK149 verze tohle nerozlisovala - kdyby se
  // spustila na texturovou oblast, prepsala by hre textury cernou.
  if (nap_render_bbox_x0 >= nap_render_bbox_x1) return; // GPU zatim nic nenakreslilo - psxVuw je autoritativni cely
  if (x  < nap_render_bbox_x0) x  = nap_render_bbox_x0;
  if (y  < nap_render_bbox_y0) y  = nap_render_bbox_y0;
  if (dx > nap_render_bbox_x1) dx = nap_render_bbox_x1;
  if (dy > nap_render_bbox_y1) dy = nap_render_bbox_y1;
  int w = dx - x, h = dy - y;
  if (x < 0) { w += x; x = 0; }
  if (y < 0) { h += y; y = 0; }
  if (x + w > NAP_PSX_VRAM_W) w = NAP_PSX_VRAM_W - x;
  if (y + h > NAP_PSX_VRAM_H) h = NAP_PSX_VRAM_H - y;
  if (w <= 0 || h <= 0) return;
  size_t need = (size_t)w * (size_t)h * 4;
  if (need > nap_vramread_tmp_cap) {
    uint8_t *nb = (uint8_t *)realloc(nap_vramread_tmp, need);
    if (!nb) return; // alokace selhala - preskocit, nespadnout
    nap_vramread_tmp = nb;
    nap_vramread_tmp_cap = need;
  }
  int glY = NAP_PSX_VRAM_H - (y + h); // BUILD2SK125 styl - stejny prevod jako hlavni cteni
  if (glY < 0) glY = 0;
  glReadPixels(x, glY, w, h, GL_RGBA, GL_UNSIGNED_BYTE, nap_vramread_tmp);
  for (int row = 0; row < h; row++) {
    const uint8_t *srcRow = nap_vramread_tmp + (size_t)(h - 1 - row) * w * 4; // stejny flip princip jako hlavni cteni
    unsigned short *dstRow = psxVuw + (size_t)1024 * (y + row) + x;
    for (int col = 0; col < w; col++) {
      uint8_t r = srcRow[col*4+0], g = srcRow[col*4+1], b = srcRow[col*4+2], a = srcRow[col*4+3];
      // BUILD2SK149: PS1 VRAM 16-bit format - 1 bit STP + 5+5+5 bitu BGR (viz
      // existujici prevodni makra v gpuPrim.c pouzivajici stejne posuny).
      dstRow[col] = (unsigned short)(((r >> 3) & 0x1F) | (((g >> 3) & 0x1F) << 5) | (((b >> 3) & 0x1F) << 10) | ((a >= 128) ? 0x8000 : 0));
    }
  }
}

// don't do GL vram read
void CheckVRamRead(int x, int y, int dx, int dy, bool bFront)
{
  (void)bFront; // BUILD2SK149: nasa architektura ma jen JEDEN trvaly canvas (viz SK144) - zadny samostatny "front" k rozlisovani
  nap_vram_read_sync(x, y, dx, dy);
}

void CheckVRamReadEx(int x, int y, int dx, int dy)
{
  nap_vram_read_sync(x, y, dx, dy);
}

// BUILD2SK153: OPACNY smer nez nap_vram_read_sync - dostat CPU->VRAM zapisy
// (GP0 A0 image load: FMV snimky, loading obrazovky, stmivaci prechody,
// 2D pozadi; a cile GP0 80 VRAM->VRAM kopii) do CANVASU, aby je readback
// videl. TOHLE JE HLAVNI DUVOD "neuplne grafiky": puvodni cesta
// (CheckWriteUpdate -> CheckAgainstScreen -> UploadScreen) je cela zavisla
// na peops PSXDisplay.DisplayPosition/DisplayEnd - a ty jsou kvuli
// is_opened=0 zamrzle na NULACH -> CheckAgainstScreen vzdy FALSE ->
// UploadScreen se NIKDY nezavolal -> zadny CPU zapis se do canvasu nikdy
// nedostal. Tady delame rozhodnuti sami, proti AUTORITATIVNIMU stavu
// (gpu.screen pres nap_gpulib_display_info) a pouzivame UploadScreen(-1):
// varianta -1 ("upload after") klade vrcholy v HOLYCH VRAM souradnicich
// (zadne odecitani DisplayPosition jako varianty 0/1) - presne spravne pod
// SK128 VRAM-space projekci (glOrtho 0..1024 x 0..512).
static void nap_upload_vram_rect(int x, int y, int w, int h)
{
  if (!nap_fbo_ready) return;
  if (w <= 0 || h <= 0) return;
  int dsx = 0, dsy = 0, dw = 0, dh = 0;
  nap_gpulib_display_info(&dsx, &dsy, &dw, &dh);
  if (dw <= 0) dw = 320;
  if (dh <= 0) dh = 240;
  // 24bit rezim: VRAMWrite/kopie chodí v 16bit jednotkach (halfword), ale
  // LoadDirectMovieFast + zobrazovaci sirka pracuji v PIXELECH (3 bajty).
  // Stejny prepocet *2/3 jako vendor PrepareRGB24Upload.
  int px = x, pw = w, pdsx = dsx;
  if (nap_disp_rgb24) {
    px = (x * 2) / 3;
    pw = (w * 2) / 3;
    pdsx = (dsx * 2) / 3;
    (void)pdsx; // sirkovy prekryv v RGB24 neresime po sloupcich - viz radkove pasmo nize
  }
  // Radkove pasmo displeje: nahravame, pokud se zapis RADKOVE prekryva s
  // [src_y, src_y+vres) - tim pokryjeme JAK prave zobrazovanou, TAK druhou
  // (back-buffer, src_x 0/512) polovinu VRAM ve stejnych radcich (hry
  // bezne zapisuji HUD/pozadi do back-bufferu tesne pred flipem - podle
  // sloupcu by se to nedalo spolehlive poznat). Texturove stranky lezici v
  // JINYCH radcich (bezne y>=256 u 240p her) se tim automaticky vynechaji
  // (setri GPU quady); kdyz radky sdili, nahraje se to taky - nevadi,
  // obsah canvasu == psxVuw, jen o par quadu vic. RGB24 (FMV) nahravame
  // VZDY - snimek filmu se proste musi objevit.
  int yy0 = y, yy1 = y + h;
  if (yy1 > NAP_PSX_VRAM_H) yy1 = NAP_PSX_VRAM_H;
  if (!nap_disp_rgb24) {
    int band0 = dsy, band1 = dsy + dh;
    if (band1 > NAP_PSX_VRAM_H) band1 = NAP_PSX_VRAM_H;
    if (yy1 <= band0 || yy0 >= band1) return; // cely zapis mimo radky displeje (typicky texturova data) - canvas ho nepotrebuje
    // BUILD2SK154: CLUT/paletove pasky (16x1, 256x1...) do canvasu nepatri -
    // v Reneho logu se nahravaly porad dokola ([0,480 16x1] apod.), na
    // spodnich radcich 480-rezimu by byly i VIDET jako barevne smeti, a
    // kazdy takovy upload zbytecne tahal vendor UploadScreen uprostred
    // snimku. Skutecny obrazovy zapis ma vzdy vic nez par radku.
    if (h < 4) return;
    // BUILD2SK154: SLOUPCOVA brana - radkove pasmo nestaci (Reneho log:
    // texturove bloky na x>=640 pri 640x480 displeji prochazely). Zapis musi
    // sloupcove protinat zobrazovanou oblast; u sirek <=512 bereme OBE
    // poloviny VRAM (0/512 double-buffering - zapisy do back-bufferu pred
    // flipem musi projit), u sirsich rezimu jen skutecny vyrez.
    {
      int ok = 0;
      if (dw <= 512) {
        int hb = dsx & 511;
        if (!(x + w <= hb       || x >= hb + dw))       ok = 1; /* leva polovina  */
        if (!(x + w <= hb + 512 || x >= hb + 512 + dw)) ok = 1; /* prava polovina */
      } else {
        if (!(x + w <= dsx || x >= dsx + dw)) ok = 1;
      }
      if (!ok) return; /* typicky texturove stranky vpravo od displeje */
    }
  }
  if (px < 0) px = 0;
  if (yy0 < 0) yy0 = 0;
  if (px + pw > NAP_PSX_VRAM_W) pw = NAP_PSX_VRAM_W - px;
  if (pw <= 0 || yy1 <= yy0) return;
  xrUploadArea.x0 = (short)px;
  xrUploadArea.x1 = (short)(px + pw);
  xrUploadArea.y0 = (short)yy0;
  xrUploadArea.y1 = (short)yy1;
  UploadScreen(-1); // vendor cesta: 256x256 dlazdice, LoadDirectMovieFast (umi 15bit i 24bit podle PSXDisplay.RGB24), spravny state-dance, iDrawnSomething=2
  // BUILD2SK154: UploadScreen uvnitr vola SetOGLDisplaySettings(0) a dalsi
  // stav z gpuDraw.c (soubor NEMAME v balicku - nemuzeme doverovat, co
  // presne s projekci/viewportem/scissorem udela). Proto HNED po navratu
  // vratime cely zobrazovaci stav do naseho pevneho tvaru - vsechno, co se
  // kresli PO tomhle uploadu ve stejnem ticku, ma zarucene spravnou
  // projekci. Levne (par GL volani) a odstranuje to jedinou zbylou
  // neznamou zavislost na gpuDraw.c.
  nap_gles_apply_fixed_display();
  {
    static int nap_upload_logs = 0;
    if (nap_upload_logs < 40) {
      nap_upload_logs++;
      nap_diag_log("BUILD2SK153 GLES_VRAM_WRITE_UPLOAD rect=[%d,%d %dx%d] rgb24=%d disp=[%d,%d %dx%d]", px, yy0, pw, yy1 - yy0, nap_disp_rgb24, dsx, dsy, dw, dh);
    }
  }
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

 // BUILD2SK153: eglSwapBuffers + readback ODSUD ODSTRANENY. Prezentace
 // snimku se ted deje na JEDINEM miste - nap_gles_present_frame(), presne
 // JEDNOU za tick, po dokonceni cele emulace snimku (= po VBlanku
 // emulovaneho PS1). updateDisplay() se muze (kdyby nekdy ozil GP1 tok)
 // zavolat i UPROSTRED zpracovani prikazu snimku - prezentovat v tu chvili
 // by znamenalo cist rozkresleny obraz. (eglSwapBuffers je na pbufferu
 // podle EGL spec no-op, takze jeho odstranenim se nic neztraci.)
 // iDrawnSomething se tady uz NEnuluje - vlastni ho present().

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

 // BUILD2SK153: swap+readback ODSUD ODSTRANENY (viz updateDisplay vyse) -
 // jedine misto prezentace je nap_gles_present_frame() nize. Tahle funkce
 // se tim stava jen "uklidem priznaku" pro pripadne vendor cesty.
}

// BUILD2SK153: JEDINE misto, kde se snimek predava dal (kopie do snapshotu
// + probuzeni cteciho vlakna). Vola se z nap_worker (nap_ps1_native.cpp)
// PRESNE JEDNOU za tick, AZ PO navratu retro_run() - tj. po VBlanku
// emulovaneho PS1, kdy je cely prikazovy seznam snimku zpracovany a
// zobrazovana polovina VRAM je zarucene dokreslena. Cte se s CERSTVYM
// gpu.screen stavem - pokud hra behem snimku flipla buffer zapisem
// GP1(05h), gpu.c uz ma nove src_x/src_y a my ctneme presne novou,
// prave dokoncenou polovinu. Tim je splnena zasada "prezentovat jednou
// za snimek, ve VBlanku, s respektem ke GP1(05h) flipu" - bez ozivovani
// mrtvych peops handleru a bez prezentaci uprostred kresleni.
void nap_gles_present_frame(void)
{
 static int nap_present_logged = 0;
 if (!nap_present_logged) {
  nap_present_logged = 1;
  nap_diag_log("BUILD2SK154 GLES_VERSION_CONFIRM file=gpulib_if.c single-present-per-tick + neblokujici predani aktivni");
 }
 if (!nap_fbo_ready) return;
 if (nap_disp_disabled) {
  // GP1(03h): displej vypnuty - NEposilat nic (prijimac drzi posledni
  // snimek). Hry to bezne pulznou pri prepinani rezimu; posilat tady
  // cernou by byl presne ten "zablesk pri zmene rozliseni" navic.
  static int nap_disabled_logs = 0;
  if (nap_disabled_logs < 10) {
   nap_disabled_logs++;
   nap_diag_log("BUILD2SK153 GLES_PRESENT_SKIP_DISABLED");
  }
  return;
 }
 if (iDrawnSomething) {
  if (nap_gles_readback_and_push()) // BUILD2SK98 mechanika, SK153 cadence, SK154 bez cekani
   iDrawnSomething = 0; // vynulovat JEN kdyz snimek skutecne odesel - pri preskoku (ctecka pracuje) to zkusime znovu pristi tick se stejnym (ci novejsim) obsahem
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
  // BUILD2SK153: puvodni blok tady delal glLoadIdentity+glOrtho na PRAVE
  // AKTIVNI matici (po nasem sync je aktivni MODELVIEW! - glOrtho v
  // modelview = trvale rozbita transformace vsech nasledujicich vrcholu)
  // a SetAspectRatio() prepinal viewport na rozmery obrazovky - presne to
  // "divoke meneni viewportu pri zmene rozliseni". Pod SK128 architekturou
  // je projekce/viewport VYHRADNE ve spravě nap_gles_sync_display_settings
  // (pevne 1024x512 VRAM-space, kazdy tick) - zmena PS1 rezimu na tom NIC
  // nemeni, meni se jen vyrez pri cteni (gpu.screen hres/vres). Blok
  // ODSTRANEN; bookkeeping stavu nize zustava.
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
 // BUILD2SK153: zachyt dulezitych GP1 bitu JESTE PRED is_opened kontrolou.
 // GPUopen() se v nasem toku nikdy nevola -> is_opened je 0 navzdy -> vsechno
 // POD touto kontrolou je mrtvy kod (presne pricina SK128 nalezu "peops GP1
 // stav zamrzly"). Vedome NEOZIVUJEME cele handlery (updateDisplay kaskady
 // s eglSwapBuffers uprostred snimku, ChangeDispOffsets matematika - nikdy
 // tady netestovane) - bereme si jen 3 holé bity stavu, ktere realne
 // potrebujeme, a aplikujeme je HNED (bezime na g_worker vlakne uvnitr
 // retro_run, takze LoadDirectMovieFast/UploadScreen je uvidi jeste v tomtez
 // ticku, ve kterem hra registr zapsala):
 //   GP1(08h) bit4 = 24bit TrueColor (FMV/MDEC) - bez nej se FMV data ve
 //     VRAM interpretuji jako 15bit -> barevny sum ("cerveny sum" na
 //     Naughty Dog logu) nebo nic.
 //   GP1(08h) bit5 = interlace (jen evidence/diagnostika - kreslime vzdy
 //     progresivne plnou vysku, cadence snimku na tom uz NEZAVISI, viz
 //     nap_gles_present_frame).
 //   GP1(03h) bit0 = displej vypnuty - behem vypnuti nedavame snimky
 //     (drzime posledni misto cernych zablesku pri prepinani rezimu).
 {
  unsigned int nap_cmd = (gdata >> 24) & 0xff;
  if (nap_cmd == 0x08) {
   int new_rgb24 = (gdata & 0x10) ? 1 : 0;
   nap_disp_interlace = (gdata & 0x20) ? 1 : 0;
   if (new_rgb24 != nap_disp_rgb24) {
    nap_disp_rgb24 = new_rgb24;
    // stejny minimalni prechod, jaky delal updateDisplayIfChanged pri zmene
    // RGB24 (uklid texturove cache - 15bit a 24bit interpretace tehoz mista
    // ve VRAM nesmi zustat pomichane v cache):
    PSXDisplay.RGB24New = new_rgb24 ? TRUE : FALSE;
    PSXDisplay.RGB24    = new_rgb24 ? TRUE : FALSE; // primo - updateDisplayIfChanged tudy nikdy nepobezi (is_opened=0)
    PreviousPSXDisplay.RGB24 = 0;
    ResetTextureArea(FALSE);
    nap_diag_log("BUILD2SK153 GLES_GP1_RGB24_CHANGE rgb24=%d interlace=%d", nap_disp_rgb24, nap_disp_interlace);
   }
  } else if (nap_cmd == 0x03) {
   nap_disp_disabled = (gdata & 1) ? 1 : 0;
  }
  if (!nap_gp1_ext_seen) {
   nap_gp1_ext_seen = 1;
   nap_diag_log("BUILD2SK153 GLES_GP1_EXT_ALIVE firstCmd=0x%02x - gpu.c skutecne vola GPUwriteStatus_ext (RGB24/Disabled zachyt funguje)", nap_cmd);
  }
 }
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
    {
      // BUILD2SK153: GP0(C0h) = VRAM -> CPU cteni. Zpracovava ho gpulib
      // jadro (gpu.c) primo z gpu.vram (== psxVuw) - NAS plugin uz se k tomu
      // nedostane. psxVuw ale NEOBSAHUJE nic z toho, co GPU vykreslilo pres
      // OpenGL -> hra by si precetla stara/prazdna data (screenshoty, save
      // nahledy, efekty s pouzitim vlastniho snimku - presne "nestahne
      // obraz z GPU zpet" z Reneho bodu 2). Tady, TESNE PRED predanim
      // jadru, si prislusny obdelnik stahneme z canvasu do psxVuw
      // (nap_vram_read_sync - orizne se sam na skutecne vykreslenou
      // plochu, texturovych oblasti se nedotkne). Hlavicka C0 (3 slova) je
      // diky bounds-checku vyse zarucene cela v bufferu.
      if (cmd == 0xc0) {
        int rx = (int)(list[1] & 0x3ff);
        int ry = (int)((list[1] >> 16) & 0x1ff);
        int rw = (int)(((list[2] & 0xffff) - 1) & 0x3ff) + 1;
        int rh = (int)((((list[2] >> 16) & 0xffff) - 1) & 0x1ff) + 1;
        nap_vram_read_sync(rx, ry, rx + rw, ry + rh);
      }
      break; // image i/o, forward to upper layer
    }
    else if ((cmd & 0xf8) == 0xe0)
      ex_regs[cmd & 7] = list[0];
#endif

    // BUILD2SK153: GP0(80h) = VRAM -> VRAM kopie ("framebuffer-to-
    // framebuffer" z Reneho bodu 2 - stmivani, pruhledy, kopie snimku).
    // primMoveImage (o par radku niz) kopiruje v CPU poli psxVuw - ale
    // pokud je ZDROJEM neco, co vykreslilo GPU, musi se to NEJDRIV stahnout
    // z canvasu, jinak se kopiruje stary/prazdny obsah. (CheckVRamRead
    // uvnitr primMoveImage je podmineny iFrameReadType&2, coz je u nas 0 -
    // SK149 oprava tam byla fakticky mrtvy kod. Delame to tady, na jednom
    // miste, bez ohledu na konfiguracni prepinace.)
    if (cmd == 0x80) {
      int sx0 = (int)(list[1] & 0x3ff);
      int sy0 = (int)((list[1] >> 16) & 0x1ff);
      int mw  = (int)(((list[3] & 0xffff) - 1) & 0x3ff) + 1;
      int mh  = (int)((((list[3] >> 16) & 0xffff) - 1) & 0x1ff) + 1;
      nap_vram_read_sync(sx0, sy0, sx0 + mw, sy0 + mh);
    }

    primTableJ[cmd]((void *)list);

    // BUILD2SK153: evidence skutecne GPU-vykreslene plochy (viz
    // nap_render_bbox_union) + dokonceni VRAM->VRAM kopie smerem do canvasu.
    if (cmd == 0x02) {
      // FillRect - ignoruje DrawArea, kresli presne dany obdelnik (sirka
      // zaokrouhlena na 16 jako v primBlkFill).
      short *nap_s = (short *)list;
      int fx = (int)(nap_s[2] & 0x3ff);
      int fy = (int)(nap_s[3] & 0x1ff);
      int fw = (int)(nap_s[4] & 0x3ff);
      int fh = (int)(nap_s[5] & 0x1ff);
      fw = (fw + 15) & ~15;
      if (fh == 0x1ff) fh = 512;
      nap_render_bbox_union(fx, fy, fx + fw, fy + fh);
    }
    else if (cmd >= 0x20 && cmd <= 0x7f) {
      // vsechny polygony/cary/sprity - orezavaji se na aktualni DrawArea,
      // takze DrawArea je bezpecna (mirne nadhodnocena) obalka.
      nap_render_bbox_union((int)PSXDisplay.DrawArea.x0, (int)PSXDisplay.DrawArea.y0,
                            (int)PSXDisplay.DrawArea.x1 + 1, (int)PSXDisplay.DrawArea.y1 + 1);
    }
    else if (cmd == 0x80) {
      // cil kopie: psxVuw uz je zkopirovane (primMoveImage vyse) - kdyz cil
      // lezi v radcich displeje, nahrat ho do canvasu, aby ho videl readback
      // (vendor cesta pres CheckAgainstScreen je s nulovym DisplayEnd mrtva).
      int dx0 = (int)(list[2] & 0x3ff);
      int dy0 = (int)((list[2] >> 16) & 0x1ff);
      int mw  = (int)(((list[3] & 0xffff) - 1) & 0x3ff) + 1;
      int mh  = (int)((((list[3] >> 16) & 0xffff) - 1) & 0x1ff) + 1;
      nap_upload_vram_rect(dx0, dy0, mw, mh);
    }

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
 // BUILD2SK153: puvodni "if(is_opened)" gate ODSTRANEN - is_opened je v
 // nasem toku 0 NAVZDY (GPUopen se nevola), takze se CheckWriteUpdate
 // NIKDY nespustil -> InvalidateTextureArea se na CPU zapisy NIKDY
 // nevolala -> hry, ktere si textury za behu prepisuji (animovane textury,
 // streamovane pozadi - presne "hry ukladaji textury za behu" z Reneho
 // popisu), kreslili ze STARE cache. CheckWriteUpdate je ted volany vzdy;
 // jeho VLASTNI upload vetve zustavaji necinne (CheckAgainstScreen je s
 // nulovym DisplayEnd porad FALSE - vedome, viz nap_upload_vram_rect,
 // ktery to rozhodnuti dela sam proti autoritativnimu gpu.screen stavu).
 CheckWriteUpdate();
 nap_upload_vram_rect(x, y, w, h);
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

// ==================================================================
//  CESTA A — dvirka pro eglrender (ostry gpu-gles obraz bez Javy).
//
//  Doted gpu-gles obraz kreslil, ale predaval ho pres ctecí vlakno
//  (glReadPixels -> procesor -> Java -> obrazovka) = ta berlicka.
//  Ostra textura ale existuje UZ PRED tim ctenim: nap_fbo_tex, hned
//  po GPU->GPU kopii z canvasu. Tahle funkce udela presne tu kopii
//  (jako present_frame, ale BEZ ctecky) a vrati eglrenderu id textury
//  + vyrez. eglrender si ji nakresli primo. Zadny procesor, zadna Java.
//
//  Vola se z eglrender vlakna po kazdem retro_run, pod stejnym GL
//  kontextem sdilenym s gpu-gles.
// ==================================================================
unsigned nap_gles_grab_texture(int* out_x, int* out_y, int* out_w, int* out_h)
{
    if (!nap_fbo_ready) return 0;

    int fresh_w = 0, fresh_h = 0, fresh_sx = 0, fresh_sy = 0;
    nap_gpulib_display_info(&fresh_sx, &fresh_sy, &fresh_w, &fresh_h);
    if (fresh_w <= 0) fresh_w = 320;
    if (fresh_h <= 0) fresh_h = 240;
    if (fresh_w > NAP_PSX_VRAM_W || fresh_h > NAP_PSX_VRAM_H) return 0;

    int other_idx = 1 - nap_snapshot_idx;

    // Levna GPU->GPU kopie aktualniho canvasu do snapshot textury -
    // presne jako v readback_and_push, ale bez predani ctecce.
    glBindTexture(GL_TEXTURE_2D, nap_fbo_tex[other_idx]);
    {
        int cp_x = fresh_sx, cp_y = fresh_sy, cp_w = fresh_w, cp_h = fresh_h;
        if (cp_x < 0) cp_x = 0;
        if (cp_y < 0) cp_y = 0;
        if (cp_x + cp_w > NAP_PSX_VRAM_W) cp_x = NAP_PSX_VRAM_W - cp_w;
        if (cp_x < 0) cp_x = 0;
        int cp_glY = NAP_PSX_VRAM_H - (cp_y + cp_h);
        if (cp_glY < 0) cp_glY = 0;
        if (cp_glY + cp_h > NAP_PSX_VRAM_H) cp_glY = NAP_PSX_VRAM_H - cp_h;
        if (cp_glY < 0) cp_glY = 0;
        glCopyTexSubImage2D(GL_TEXTURE_2D, 0, cp_x, cp_glY, cp_x, cp_glY, cp_w, cp_h);
    }
    glFlush(); // kopie viditelna i z eglrender kontextu (sdili textury)

    nap_snapshot_idx = other_idx;

    if (out_x) *out_x = fresh_sx;
    if (out_y) *out_y = fresh_sy;
    if (out_w) *out_w = fresh_w;
    if (out_h) *out_h = fresh_h;
    return (unsigned)nap_fbo_tex[other_idx];
}

// Rozmery VRAM textury (eglrender potrebuje pro prepocet UV souradnic).
int nap_gles_vram_w(void) { return NAP_PSX_VRAM_W; }
int nap_gles_vram_h(void) { return NAP_PSX_VRAM_H; }
