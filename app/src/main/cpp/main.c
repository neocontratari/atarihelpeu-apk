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
    // rozpad casu snimku na jednotlive kroky (soucty pres 300 snimku)
    double     sum_bg, sum_fill, sum_up, sum_draw, sum_swap;
    GLuint     program;     // shader program pro quad s texturou
    GLuint     texture;     // textura 320x224 = budouci obraz jadra
    GLint      loc_pos;
    GLint      loc_tex;
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
// Testovaci obraz 320x224 (rozliseni Sega Mega Drive; PS1 ma podobne).
// KLICOVE: obsah se nahrava do grafiky znovu KAZDY snimek pres
// glTexSubImage2D - presne stejnou cestou pozdeji potece obraz
// z jadra emulatoru. Tenhle krok tedy zkousi a meri prave tu trasu.
// ------------------------------------------------------------------
#define TEX_W 320
#define TEX_H 224

static unsigned char s_pixels[TEX_W * TEX_H * 4];  // RGBA

static void fill_test_pattern(long frame) {
    // 8 svislych barevnych pruhu jako z televizniho testu
    static const unsigned char bars[8][3] = {
        {235,235,235}, {235,235, 16}, { 16,235,235}, { 16,235, 16},
        {235, 16,235}, {235, 16, 16}, { 16, 16,235}, { 16, 16, 16},
    };
    int line_x = (int)( frame      % TEX_W);  // svisla bila linka (jede doprava)
    int line_y = (int)((frame * 2) % TEX_H);  // vodorovna bila linka (jede dolu)

    unsigned char* p = s_pixels;
    for (int y = 0; y < TEX_H; y++) {
        for (int x = 0; x < TEX_W; x++) {
            const unsigned char* c = bars[(x * 8) / TEX_W];
            unsigned char r = c[0], g = c[1], b = c[2];
            if (x == line_x || y == line_y) { r = 255; g = 255; b = 255; }
            // sachovnice v levem hornim rohu = kontrola ostrosti pixelu
            if (x < 32 && y < 32 && (((x >> 2) + (y >> 2)) & 1)) { r = 0; g = 0; b = 0; }
            *p++ = r; *p++ = g; *p++ = b; *p++ = 255;
        }
    }
}

static GLuint compile_shader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, NULL);
    glCompileShader(sh);
    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        log[0] = 0;
        glGetShaderInfoLog(sh, sizeof log, NULL, log);
        LOGE("GL: shader se neprelozil: %s", log);
        glDeleteShader(sh);
        return 0;
    }
    return sh;
}

// Priprava quadu a textury (vola se na konci egl_init)
static bool gl_setup(Engine* e) {
    const char* vs_src =
        "attribute vec2 aPos;\n"
        "attribute vec2 aTex;\n"
        "varying vec2 vTex;\n"
        "void main() { vTex = aTex; gl_Position = vec4(aPos, 0.0, 1.0); }\n";
    const char* fs_src =
        "precision mediump float;\n"
        "varying vec2 vTex;\n"
        "uniform sampler2D uTex;\n"
        "void main() { gl_FragColor = texture2D(uTex, vTex); }\n";

    GLuint vs = compile_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) return false;

    e->program = glCreateProgram();
    glAttachShader(e->program, vs);
    glAttachShader(e->program, fs);
    glLinkProgram(e->program);
    glDeleteShader(vs);
    glDeleteShader(fs);

    GLint ok = 0;
    glGetProgramiv(e->program, GL_LINK_STATUS, &ok);
    if (!ok) {
        LOGE("GL: program se neslinkoval");
        return false;
    }

    e->loc_pos = glGetAttribLocation(e->program, "aPos");
    e->loc_tex = glGetAttribLocation(e->program, "aTex");
    if (e->loc_pos < 0 || e->loc_tex < 0) {
        LOGE("GL: atributy shaderu se nenasly");
        return false;
    }
    glUseProgram(e->program);
    glUniform1i(glGetUniformLocation(e->program, "uTex"), 0);

    glGenTextures(1, &e->texture);
    glBindTexture(GL_TEXTURE_2D, e->texture);
    // NEAREST = ostre "retro" pixely bez rozmazani
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    LOGI("GL: quad + textura %dx%d pripraveny (NEAREST, RGBA)", TEX_W, TEX_H);
    return true;
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
    e->program   = 0;   // GL objekty zanikly spolu s kontextem
    e->texture   = 0;
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

    // Priprava quadu a textury pro obraz
    if (!gl_setup(e)) {
        LOGE("GL: priprava quadu/textury selhala");
        return false;
    }

    e->frames = 0;
    e->fps_t0 = now_sec();
    e->sum_bg = e->sum_fill = e->sum_up = e->sum_draw = e->sum_swap = 0.0;
    return true;
}

// ------------------------------------------------------------------
// Jeden snimek: kresleni do ZADNIHO bufferu + preklopeni na obrazovku
// ------------------------------------------------------------------
static void draw_frame(Engine* e) {
    if (e->display == EGL_NO_DISPLAY || e->surface == EGL_NO_SURFACE) return;

    double t0 = now_sec();

    // Velikost zjistujeme kazdy snimek - otoceni displeje tim padem
    // nikdy nerozbije obraz.
    EGLint w = 0, h = 0;
    eglQuerySurface(e->display, e->surface, EGL_WIDTH,  &w);
    eglQuerySurface(e->display, e->surface, EGL_HEIGHT, &h);

    // 1) Pozadi pres celou obrazovku (jemne dychajici tmavy ramecek)
    glViewport(0, 0, w, h);
    glClearColor(0.06f + 0.03f * (float)sin(t0 * 0.5),
                 0.07f + 0.03f * (float)sin(t0 * 0.4 + 2.0),
                 0.10f + 0.04f * (float)sin(t0 * 0.3 + 4.0),
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    double t1 = now_sec();  // po pozadi

    // 2) Novy obsah snimku -> textura. PRESNE TUDY pozdeji potece obraz
    //    z jadra emulatoru: misto fill_test_pattern() se sem stejnym
    //    volanim glTexSubImage2D nahraje jeho framebuffer.
    fill_test_pattern(e->frames);

    double t2 = now_sec();  // po vyrobe vzoru (cista prace procesoru)

    glBindTexture(GL_TEXTURE_2D, e->texture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TEX_W, TEX_H,
                    GL_RGBA, GL_UNSIGNED_BYTE, s_pixels);

    double t3 = now_sec();  // po nahrani textury do grafiky

    // 3) Letterbox: roztahnout co nejvic pri zachovani pomeru stran
    int vw = w;
    int vh = (w * TEX_H) / TEX_W;
    if (vh > h) { vh = h; vw = (h * TEX_W) / TEX_H; }
    glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);

    // 4) Vykresleni quadu s texturou
    static const GLfloat quad[] = {
        // x,     y,      u,    v   (v je obracene: radek 0 textury nahore)
        -1.0f, -1.0f,   0.0f, 1.0f,
         1.0f, -1.0f,   1.0f, 1.0f,
        -1.0f,  1.0f,   0.0f, 0.0f,
         1.0f,  1.0f,   1.0f, 0.0f,
    };
    glUseProgram(e->program);
    glVertexAttribPointer((GLuint)e->loc_pos, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), quad);
    glVertexAttribPointer((GLuint)e->loc_tex, 2, GL_FLOAT, GL_FALSE,
                          4 * sizeof(GLfloat), quad + 2);
    glEnableVertexAttribArray((GLuint)e->loc_pos);
    glEnableVertexAttribArray((GLuint)e->loc_tex);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    double t4 = now_sec();  // po zadani kresleni

    // 5) Preklopeni zadniho bufferu na obrazovku. Diky vsync presne
    //    v rytmu displeje -> zadne blikani.
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

    double t5 = now_sec();  // po preklopeni (obsahuje cekani na rytmus displeje)

    // Kazdych 300 snimku: FPS + presny rozpad casu snimku na kroky.
    // "swap" = cekani na vsync, to NENI ztraceny vykon, ale volna rezerva.
    e->frames++;
    e->sum_bg   += (t1 - t0) * 1000.0;
    e->sum_fill += (t2 - t1) * 1000.0;
    e->sum_up   += (t3 - t2) * 1000.0;
    e->sum_draw += (t4 - t3) * 1000.0;
    e->sum_swap += (t5 - t4) * 1000.0;
    if (e->frames % 300 == 0) {
        double dt = t0 - e->fps_t0;
        if (dt > 0.0) {
            double n = 300.0;
            LOGI("Bezi: %ld snimku, ~%.1f FPS, %dx%d | ms/snimek: pozadi %.2f + vzor %.2f + nahrani %.2f + kresleni %.2f + swap %.2f = %.2f",
                 e->frames, n / dt, w, h,
                 e->sum_bg / n, e->sum_fill / n, e->sum_up / n,
                 e->sum_draw / n, e->sum_swap / n,
                 (e->sum_bg + e->sum_fill + e->sum_up + e->sum_draw + e->sum_swap) / n);
        }
        e->fps_t0 = t0;
        e->sum_bg = e->sum_fill = e->sum_up = e->sum_draw = e->sum_swap = 0.0;
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
