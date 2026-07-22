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
static int nap_fbo_meta_sx[2], nap_fbo_meta_sy[2], nap_fbo_meta_w[2] = {320,320}, nap_fbo_meta_h[2] = {240,240}; // BUILD2SK133: co PLATILO v okamziku, kdy se do tenhle SNAPSHOT naposledy zkopirovalo - NE aktualni gpu.screen (to uz muze byt o snimek dal)

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
   if (req.do_clear) {
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    nap_diag_log("BUILD2SK140 GLES_SCENE_CHANGE_CLEAR idx=%d", req.idx);
   }
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
  if (nap_gles_frame_count < 200 || nap_gles_frame_count % 31 == 1) {
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
   if (nap_ascii_dump_count < 30) { // BUILD2SK151: 8 -> 30, at zachytime vic prilezitosti behem husteho useku
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

static void nap_gles_readback_and_push(void)
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
 if (fresh_w <= 0 || fresh_h <= 0 || fresh_w > NAP_PSX_VRAM_W || fresh_h > NAP_PSX_VRAM_H) return;
 if (!nap_fbo_ready) return;
 // BUILD2SK144: g_worker uz TADY NEDELA ZADNE cteni z GPU (stejne jako
 // SK140) - ale uz TAKY NEPREHAZUJE, kam hra kresli (to byla ta chyba,
 // viz velky komentar u globalnich promennych). Hra porad kresli do
 // JEDINEHO canvasu (uz bindnuty, nikdy se nemeni). Tady jen: (1) levna
 // GPU->GPU kopie aktualniho obsahu canvasu do jednoho ze 2 "snapshot"
 // policek, (2) domluva s ctecim vlaknem, at tenhle snapshot zpracuje.
 int other_idx = 1 - nap_snapshot_idx;
 pthread_mutex_lock(&nap_reader_mtx);
 // BUILD2SK140 (princip beze zmeny): pockat, az ctecí vlakno DOKONCI svoji
 // GPU cast (cteni + pripadne cisteni) predchoziho pozadavku na TENTO SAMY
 // snapshot - teprve pak je bezpecne do nej znovu kopirovat. V beznem
 // pripade uz je to davno hotove.
 while (nap_reader_has_req) pthread_cond_wait(&nap_reader_cv, &nap_reader_mtx);
 while (!nap_reader_idx_free[other_idx]) pthread_cond_wait(&nap_reader_cv, &nap_reader_mtx);
 nap_reader_idx_free[other_idx] = 0; // zabirame ho pro NOVY pozadavek
 nap_reader_req.idx = other_idx;
 nap_reader_req.rb_w = nap_fbo_meta_w[other_idx];
 nap_reader_req.rb_h = nap_fbo_meta_h[other_idx];
 nap_reader_req.src_x = nap_fbo_meta_sx[other_idx];
 nap_reader_req.src_y = nap_fbo_meta_sy[other_idx];
 nap_reader_req.do_clear = (fresh_w != nap_fbo_meta_w[other_idx] || fresh_h != nap_fbo_meta_h[other_idx]);
 pthread_mutex_unlock(&nap_reader_mtx); // BUILD2SK144: odemknout PRED GL praci nize - mutex je jen pro sdilena data, ne pro GL volani
 // BUILD2SK144: LEVNA GPU->GPU kopie - canvas_fbo je porad bindnuty (jako
 // vzdy), takze glCopyTexSubImage2D cte primo z NEJ do snapshot textury.
 // Zadny CPU prenos, zadne cekani na "je GPU hotove" - jen GPU->GPU
 // presun, typicky < 1ms, a hlavne: NEDOTYKA se toho, kam hra kresli.
 glBindTexture(GL_TEXTURE_2D, nap_fbo_tex[other_idx]);
 glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, NAP_PSX_VRAM_W, NAP_PSX_VRAM_H);
 // BUILD2SK144: SK139 princip, aplikovany na CANVAS misto na jednotlive
 // snapshoty (canvas je ted ten JEDINY, trvaly povrch, kam hra kresli -
 // takze presne tady, ne na snapshotu, muze stara scena "viset" do nove).
 // Canvas_fbo je porad bindnuty (nikdy jsme ho neopustili), takze glClear
 // tady cisti PRAVE jeho - AZ PO kopii vyse, takze posledni platny snimek
 // stare sceny se jeste stihl zachytit do snapshotu, driv nez zmizi.
 if (nap_reader_req.do_clear) {
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);
 }
 // BUILD2SK133/144: tenhle snapshot dostal cerstva data - ulozit aktualni
 // (fresh) rozmery/pozici, at az na nej priste dojde rada cteni, vime, co
 // do nej PRAVE prislo.
 nap_fbo_meta_sx[other_idx] = fresh_sx;
 nap_fbo_meta_sy[other_idx] = fresh_sy;
 nap_fbo_meta_w[other_idx] = fresh_w;
 nap_fbo_meta_h[other_idx] = fresh_h;
 nap_snapshot_idx = other_idx;
 pthread_mutex_lock(&nap_reader_mtx);
 nap_reader_has_req = 1;
 pthread_cond_broadcast(&nap_reader_cv);
 pthread_mutex_unlock(&nap_reader_mtx);
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
  if (nap_gles_frame_count % 31 == 1) {
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
  if (nap_gles_frame_count % 31 == 1) {
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
