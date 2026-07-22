// =====================================================================
//  main.c - cista, "neprustrelna" inicializace EGL + OpenGL ES 2.0
//  pro Android, bez jedineho radku Javy (NativeActivity).
//
//  Co to dela:
//   1. Pri vytvoreni okna kompletne inicializuje EGL (display, config,
//      surface s explicitnim BACK bufferem = double buffering, kontext).
//   2. Zapne vsync (eglSwapInterval 1) -> zadne trhani ani blikani.
//   3. V hlavni smycce kazdy snimek vykresli a preklopi bufferem
//      pres eglSwapBuffers.
//   4. Kdyz cokoliv selze (otoceni, zamknuti, ztrata surface), vsechno
//      uklidi a znovu postavi. Zadny pad.
//
//  Kazdy krok se loguje do logcatu (tag EGLRender) i na
//  http://IP-TELEFONU:8765/log - stejny postup jako v emu10.
//
//  Polygony a texture (obraz z jadra emulatoru) prijdou POZDEJI do
//  funkce draw_frame() - je tam oznacene misto.
// =====================================================================

#define _POSIX_C_SOURCE 200809L  // kvuli clock_gettime pri prisnem rezimu prekladace

#include <android/log.h>
#include <android/looper.h>
#include <android/native_window.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include "logserver.h"

#define TAG "EGLRender"
#define LOGI(...) do { __android_log_print(ANDROID_LOG_INFO,  TAG, __VA_ARGS__); ls_log(__VA_ARGS__); } while (0)
#define LOGE(...) do { __android_log_print(ANDROID_LOG_ERROR, TAG, __VA_ARGS__); ls_log(__VA_ARGS__); } while (0)

// ------------------------------------------------------------------
// Stav rendereru
// ------------------------------------------------------------------
typedef struct {
    struct android_app* app;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig  config;
    bool       animating;   // smi se prave ted kreslit?
    long       frames;      // pocitadlo snimku (pro FPS log)
    double     fps_t0;
} Engine;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static const char* egl_err_str(EGLint e) {
    switch (e) {
        case EGL_SUCCESS:           return "EGL_SUCCESS";
        case EGL_NOT_INITIALIZED:   return "EGL_NOT_INITIALIZED";
        case EGL_BAD_ACCESS:        return "EGL_BAD_ACCESS";
        case EGL_BAD_ALLOC:         return "EGL_BAD_ALLOC";
        case EGL_BAD_ATTRIBUTE:     return "EGL_BAD_ATTRIBUTE";
        case EGL_BAD_CONFIG:        return "EGL_BAD_CONFIG";
        case EGL_BAD_CONTEXT:       return "EGL_BAD_CONTEXT";
        case EGL_BAD_DISPLAY:       return "EGL_BAD_DISPLAY";
        case EGL_BAD_MATCH:         return "EGL_BAD_MATCH";
        case EGL_BAD_NATIVE_WINDOW: return "EGL_BAD_NATIVE_WINDOW";
        case EGL_BAD_SURFACE:       return "EGL_BAD_SURFACE";
        case EGL_CONTEXT_LOST:      return "EGL_CONTEXT_LOST";
        default:                    return "EGL_(neznama chyba)";
    }
}

// ------------------------------------------------------------------
// Kompletni uklid EGL (bezpecne i kdyz je inicializovano jen napul)
// ------------------------------------------------------------------
static void egl_term(Engine* e) {
    if (e->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(e->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (e->context != EGL_NO_CONTEXT) eglDestroyContext(e->display, e->context);
        if (e->surface != EGL_NO_SURFACE) eglDestroySurface(e->display, e->surface);
        eglTerminate(e->display);
        LOGI("EGL: uklizeno");
    }
    e->display   = EGL_NO_DISPLAY;
    e->surface   = EGL_NO_SURFACE;
    e->context   = EGL_NO_CONTEXT;
    e->animating = false;
}

// ------------------------------------------------------------------
// Neprustrelna inicializace EGL - krok po kroku, vse zalogovane
// ------------------------------------------------------------------
static bool egl_init(Engine* e, ANativeWindow* window) {
    LOGI("EGL: start inicializace");

    // 1) Display
    e->display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (e->display == EGL_NO_DISPLAY) {
        LOGE("EGL: eglGetDisplay selhal");
        return false;
    }
    EGLint major = 0, minor = 0;
    if (!eglInitialize(e->display, &major, &minor)) {
        LOGE("EGL: eglInitialize selhal (%s)", egl_err_str(eglGetError()));
        return false;
    }
    LOGI("EGL: verze %d.%d", major, minor);

    // 2) Konfigurace - nejdrive plne barvy RGBA8888, kdyz neni, tak RGB565
    const EGLint attribs_rgba8[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,   8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE,  8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 0,   // az budou 3D polygony s hloubkou, zmenit na 16
        EGL_NONE
    };
    const EGLint attribs_rgb565[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_SURFACE_TYPE,    EGL_WINDOW_BIT,
        EGL_RED_SIZE,   5,
        EGL_GREEN_SIZE, 6,
        EGL_BLUE_SIZE,  5,
        EGL_DEPTH_SIZE, 0,
        EGL_NONE
    };
    EGLint num = 0;
    if (!eglChooseConfig(e->display, attribs_rgba8, &e->config, 1, &num) || num < 1) {
        LOGI("EGL: RGBA8888 neni k dispozici, zkousim RGB565");
        if (!eglChooseConfig(e->display, attribs_rgb565, &e->config, 1, &num) || num < 1) {
            LOGE("EGL: zadna pouzitelna konfigurace (%s)", egl_err_str(eglGetError()));
            return false;
        }
    }
    EGLint r = 0, g = 0, b = 0, a = 0, vid = 0;
    eglGetConfigAttrib(e->display, e->config, EGL_RED_SIZE,   &r);
    eglGetConfigAttrib(e->display, e->config, EGL_GREEN_SIZE, &g);
    eglGetConfigAttrib(e->display, e->config, EGL_BLUE_SIZE,  &b);
    eglGetConfigAttrib(e->display, e->config, EGL_ALPHA_SIZE, &a);
    eglGetConfigAttrib(e->display, e->config, EGL_NATIVE_VISUAL_ID, &vid);
    LOGI("EGL: config R%dG%dB%dA%d", r, g, b, a);

    // 3) Format bufferu okna musi sedet s vybranou konfiguraci
    ANativeWindow_setBuffersGeometry(window, 0, 0, vid);

    // 4) Kreslici plocha - EXPLICITNE zadni buffer = double buffering
    const EGLint surf_attribs[] = { EGL_RENDER_BUFFER, EGL_BACK_BUFFER, EGL_NONE };
    e->surface = eglCreateWindowSurface(e->display, e->config, window, surf_attribs);
    if (e->surface == EGL_NO_SURFACE) {
        LOGE("EGL: eglCreateWindowSurface selhal (%s)", egl_err_str(eglGetError()));
        return false;
    }

    // 5) Kontext OpenGL ES 2.0
    const EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    e->context = eglCreateContext(e->display, e->config, EGL_NO_CONTEXT, ctx_attribs);
    if (e->context == EGL_NO_CONTEXT) {
        LOGE("EGL: eglCreateContext selhal (%s)", egl_err_str(eglGetError()));
        return false;
    }

    // 6) Aktivace
    if (!eglMakeCurrent(e->display, e->surface, e->surface, e->context)) {
        LOGE("EGL: eglMakeCurrent selhal (%s)", egl_err_str(eglGetError()));
        return false;
    }

    // 7) VSYNC: swap se ceka na obnovovaci frekvenci displeje.
    //    Tohle je ta pojistka proti blikani a trhani obrazu.
    eglSwapInterval(e->display, 1);

    // 8) Kontrola a vypis vysledku
    EGLint rb = 0, w = 0, h = 0;
    eglQuerySurface(e->display, e->surface, EGL_RENDER_BUFFER, &rb);
    eglQuerySurface(e->display, e->surface, EGL_WIDTH,  &w);
    eglQuerySurface(e->display, e->surface, EGL_HEIGHT, &h);
    LOGI("EGL: surface %dx%d, %s, vsync ON", w, h,
         (rb == EGL_BACK_BUFFER) ? "DOUBLE BUFFERING aktivni (BACK buffer)"
                                 : "pozor: single buffer");
    LOGI("GL: %s | %s | %s",
         (const char*)glGetString(GL_VENDOR),
         (const char*)glGetString(GL_RENDERER),
         (const char*)glGetString(GL_VERSION));

    e->frames = 0;
    e->fps_t0 = now_sec();
    return true;
}

// ------------------------------------------------------------------
// Jeden snimek: kresleni do ZADNIHO bufferu + preklopeni na obrazovku
// ------------------------------------------------------------------
static void draw_frame(Engine* e) {
    if (e->display == EGL_NO_DISPLAY || e->surface == EGL_NO_SURFACE) return;

    // Velikost zjistujeme kazdy snimek - otoceni displeje tim padem
    // nikdy nerozbije viewport.
    EGLint w = 0, h = 0;
    eglQuerySurface(e->display, e->surface, EGL_WIDTH,  &w);
    eglQuerySurface(e->display, e->surface, EGL_HEIGHT, &h);
    glViewport(0, 0, w, h);

    // Testovaci obraz: pomalu se prelevajici tmava barva.
    // Kdyz je swap spatne, je to videt okamzite (blikani/skoky).
    // Kdyz je vse dobre, barva tece naprosto plynule - i v zrcadleni na TV.
    double t = now_sec();
    float rr = 0.10f + 0.08f * (float)sin(t * 0.9);
    float gg = 0.16f + 0.10f * (float)sin(t * 0.6 + 2.0);
    float bb = 0.28f + 0.15f * (float)sin(t * 0.4 + 4.0);
    glClearColor(rr, gg, bb, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // ==============================================================
    //  >>> SEM POZDEJI PRIJDOU POLYGONY / TEXTURA S OBRAZEM JADRA <<<
    //  (vsechno kolem - EGL, smycka, swap, vsync - uz je hotove)
    // ==============================================================

    // Preklopeni zadniho bufferu na obrazovku. Diky vsync presne
    // v rytmu displeje -> zadne blikani.
    if (!eglSwapBuffers(e->display, e->surface)) {
        EGLint err = eglGetError();
        LOGE("eglSwapBuffers selhal: %s", egl_err_str(err));

        // Neprustrelnost: pri ztrate surface/kontextu vse zborime
        // a postavime znovu, misto abychom spadli.
        ANativeWindow* win = e->app->window;
        egl_term(e);
        if (win && (err == EGL_BAD_SURFACE || err == EGL_CONTEXT_LOST ||
                    err == EGL_BAD_NATIVE_WINDOW || err == EGL_BAD_DISPLAY)) {
            LOGI("EGL: zkousim kompletni re-inicializaci");
            if (egl_init(e, win)) e->animating = true;
        }
        return;
    }

    // Kazdych 300 snimku vypis FPS - dukaz zivota v 8765/log
    e->frames++;
    if (e->frames % 300 == 0) {
        double dt = t - e->fps_t0;
        if (dt > 0.0) LOGI("Bezi: %ld snimku, ~%.1f FPS, %dx%d", e->frames, 300.0 / dt, w, h);
        e->fps_t0 = t;
    }
}

// ------------------------------------------------------------------
// Zpravy od systemu (okno vzniklo/zaniklo, focus...)
// ------------------------------------------------------------------
static void handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* e = (Engine*)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            LOGI("System: INIT_WINDOW (okno je pripravene)");
            if (app->window) {
                if (e->display != EGL_NO_DISPLAY) egl_term(e);  // pojistka proti dvojite inicializaci
                if (egl_init(e, app->window)) {
                    e->animating = true;
                } else {
                    LOGE("Inicializace EGL selhala - obraz nepobezi, viz logy vyse");
                }
            }
            break;

        case APP_CMD_TERM_WINDOW:
            LOGI("System: TERM_WINDOW (okno mizi - uklizim EGL)");
            egl_term(e);
            break;

        case APP_CMD_GAINED_FOCUS:
            LOGI("System: GAINED_FOCUS");
            if (e->display != EGL_NO_DISPLAY) e->animating = true;
            break;

        case APP_CMD_LOST_FOCUS:
            LOGI("System: LOST_FOCUS (pauza kresleni)");
            e->animating = false;
            break;

        default:
            break;
    }
}

// ------------------------------------------------------------------
// HLAVNI SMYCKA
// ------------------------------------------------------------------
void android_main(struct android_app* app) {
    static Engine engine;
    memset(&engine, 0, sizeof engine);
    engine.display = EGL_NO_DISPLAY;
    engine.surface = EGL_NO_SURFACE;
    engine.context = EGL_NO_CONTEXT;
    engine.app     = app;

    app->userData = &engine;
    app->onAppCmd = handle_cmd;

    logserver_start(8765);
    LOGI("=== AH EGL Render start ===");

    for (;;) {
        // Zpracovani vsech cekajicich udalosti od systemu.
        // Kdyz se kresli: timeout 0 (nezastavovat snimky).
        // Kdyz se nekresli: -1 (spat a setrit baterii, dokud neco neprijde).
        int events;
        struct android_poll_source* source = NULL;
        while (ALooper_pollOnce(engine.animating ? 0 : -1, NULL, &events,
                                (void**)&source) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                LOGI("=== Konec aplikace ===");
                egl_term(&engine);
                return;
            }
            source = NULL;
        }
        if (app->destroyRequested) {
            LOGI("=== Konec aplikace ===");
            egl_term(&engine);
            return;
        }

        // Jeden snimek + eglSwapBuffers (vsync sam ohlida tempo)
        if (engine.animating) draw_frame(&engine);
    }
}
