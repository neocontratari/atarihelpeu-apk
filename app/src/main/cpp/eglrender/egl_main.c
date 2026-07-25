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

#include "egl_core_api.h"
#include "egl_logserver.h"

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
    GLint      loc_mode;    // prepinac formatu barev ve shaderu (0=RGBA, 1=XRGB)
    int        tex_w, tex_h;    // aktualni rozliseni textury (podle jadra)
    CoreFormat tex_fmt;
} Engine;

static double now_sec(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// ==================================================================
//  OVLADANI — dotyk na obrazovce v C (bez Java overlay).
//  Rozlozeni tlacitek podle rozmeru okna: vlevo dole D-pad,
//  vpravo dole ctyri akcni tlacitka, uprostred dole START/SELECT.
//  Kazdy prst se otestuje proti obdelnikum a slozi se bitmapa
//  RETRO_DEVICE_ID_JOYPAD_*, kterou core_ps1 vraci jadru.
// ==================================================================
#include <android/input.h>

extern void core_set_pad(unsigned state);

// RETRO_DEVICE_ID_JOYPAD_*
enum { JB_B=0, JJB_Y=1, JB_SELECT=2, JB_START=3, JB_UP=4, JB_DOWN=5,
       JB_LEFT=6, JB_RIGHT=7, JB_A=8, JB_X=9, JB_L=10, JB_R=11 };

static int pt_in(float px, float py, float x, float y, float w, float h) {
    return px >= x && px <= x + w && py >= y && py <= y + h;
}

static int32_t handle_input(struct android_app* app, AInputEvent* ev) {
    (void)app;
    if (AInputEvent_getType(ev) != AINPUT_EVENT_TYPE_MOTION) return 0;

    ANativeWindow* win = app->window;
    float W = win ? (float)ANativeWindow_getWidth(win)  : 1920.0f;
    float H = win ? (float)ANativeWindow_getHeight(win) : 1080.0f;

    // Geometrie ovladacu (odvozeno od vysky, at sedi na ruzne displeje)
    float u = H * 0.12f;              // zakladni velikost tlacitka
    float m = H * 0.06f;              // okraj
    // D-pad vlevo dole (kriz)
    float dcx = m + u*1.5f, dcy = H - m - u*1.5f;
    // Akcni tlacitka vpravo dole
    float acx = W - m - u*1.5f, acy = H - m - u*1.5f;
    // START/SELECT uprostred dole
    float scy = H - m - u*0.6f;

    unsigned st = 0;
    int pc = AMotionEvent_getPointerCount(ev);
    int act = AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_MASK;
    // Pri UP prstu ho nezapocitavat
    int upIdx = -1;
    if (act == AMOTION_EVENT_ACTION_POINTER_UP || act == AMOTION_EVENT_ACTION_UP)
        upIdx = (AMotionEvent_getAction(ev) & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

    for (int i = 0; i < pc; i++) {
        if (i == upIdx) continue;
        float x = AMotionEvent_getX(ev, i);
        float y = AMotionEvent_getY(ev, i);

        // D-pad: ctyri smery kolem stredu
        if (pt_in(x,y, dcx-u*0.5f, dcy-u*1.5f, u, u))       st |= (1u<<JB_UP);
        if (pt_in(x,y, dcx-u*0.5f, dcy+u*0.5f, u, u))       st |= (1u<<JB_DOWN);
        if (pt_in(x,y, dcx-u*1.5f, dcy-u*0.5f, u, u))       st |= (1u<<JB_LEFT);
        if (pt_in(x,y, dcx+u*0.5f, dcy-u*0.5f, u, u))       st |= (1u<<JB_RIGHT);

        // Akcni: trojuhelnik(nahore) X(dole) ctverec(vlevo) kolecko(vpravo)
        if (pt_in(x,y, acx-u*0.5f, acy-u*1.5f, u, u))       st |= (1u<<JB_X);   // trojuhelnik
        if (pt_in(x,y, acx-u*0.5f, acy+u*0.5f, u, u))       st |= (1u<<JB_B);   // X (potvrdit)
        if (pt_in(x,y, acx-u*1.5f, acy-u*0.5f, u, u))       st |= (1u<<JJB_Y);  // ctverec
        if (pt_in(x,y, acx+u*0.5f, acy-u*0.5f, u, u))       st |= (1u<<JB_A);   // kolecko

        // START / SELECT uprostred
        if (pt_in(x,y, W*0.5f - u,      scy, u*0.8f, u*0.7f)) st |= (1u<<JB_SELECT);
        if (pt_in(x,y, W*0.5f + u*0.2f, scy, u*0.8f, u*0.7f)) st |= (1u<<JB_START);

        // L / R nahore v rozich
        if (pt_in(x,y, m, m, u*1.4f, u*0.8f))               st |= (1u<<JB_L);
        if (pt_in(x,y, W-m-u*1.4f, m, u*1.4f, u*0.8f))      st |= (1u<<JB_R);
    }

    core_set_pad(st);
    // MUJ LOG: kdyz se stav tlacitek zmeni, zapsat ho. Vidim tak, jestli
    // dotyk vubec dopada na tlacitka - kdyz mackas a tady je porad 0,
    // vim, ze rozlozeni tlacitek nesedi na to, kam prsty dopadaji.
    static unsigned dbg_last_pad = 0;
    if (st != dbg_last_pad) {
        dbg_last_pad = st;
        __android_log_print(ANDROID_LOG_INFO, TAG, "MUJLOG ovladani: stav tlacitek = 0x%03x", st);
        ls_log("MUJLOG ovladani: stav tlacitek = 0x%03x", st);
    }
    return 1;
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

// Obraz ted dodava "jadro" pres zasuvku v core_api.h (nyni core_demo.c,
// pozdeji skutecne Sega/PS1 jadro). Renderer se o zdroj pixelu nestara -
// jen si je prevezme a zobrazí.

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
        "uniform float uMode;\n"   // 0 = RGBA, 1 = XRGB (prohodit kanaly na GPU)
        "void main() {\n"
        "  vec4 c = texture2D(uTex, vTex);\n"
        "  gl_FragColor = (uMode > 0.5) ? vec4(c.bgr, 1.0) : vec4(c.rgb, 1.0);\n"
        "}\n";

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

    e->loc_pos  = glGetAttribLocation(e->program, "aPos");
    e->loc_tex  = glGetAttribLocation(e->program, "aTex");
    e->loc_mode = glGetUniformLocation(e->program, "uMode");
    if (e->loc_pos < 0 || e->loc_tex < 0) {
        LOGE("GL: atributy shaderu se nenasly");
        return false;
    }
    glUseProgram(e->program);
    glUniform1i(glGetUniformLocation(e->program, "uTex"), 0);
    glUniform1f(e->loc_mode, 0.0f);

    glGenTextures(1, &e->texture);
    glBindTexture(GL_TEXTURE_2D, e->texture);
    // NEAREST = ostre "retro" pixely bez rozmazani
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    // Pamet textury se pridel az podle prvniho snimku jadra
    e->tex_w   = 0;
    e->tex_h   = 0;
    e->tex_fmt = CORE_FMT_RGBA8888;

    LOGI("GL: quad pripraven, textura se prizpusobi rozliseni jadra");
    return true;
}

// Prestavi texturu, kdykoli jadro zmeni rozliseni nebo format hry
static void ensure_texture(Engine* e, int w, int h, CoreFormat fmt) {
    if (e->tex_w == w && e->tex_h == h && e->tex_fmt == fmt) return;

    glBindTexture(GL_TEXTURE_2D, e->texture);
    if (fmt == CORE_FMT_RGB565) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0,
                     GL_RGB, GL_UNSIGNED_SHORT_5_6_5, NULL);
    } else {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    }
    LOGI("Jadro zmenilo rozliseni: %dx%d -> %dx%d (format %d) - textura prestavena",
         e->tex_w, e->tex_h, w, h, (int)fmt);
    e->tex_w   = w;
    e->tex_h   = h;
    e->tex_fmt = fmt;
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
    e->tex_w     = 0;
    e->tex_h     = 0;
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

    // 2) Krok jadra + prevzeti jeho framebufferu.
    //    CESTA A: pred krokem prepnout na gpu-gles kontext (hra kresli do
    //    canvasu), po kroku zpet na eglrender (kresli na okno). Prepinani
    //    dela RENDER vlakno tady - ne tick - takze retro_run se nezdrzuje
    //    a zvuk neskrece.
    core_bind_for_step();
    core_step();
    core_bind_for_display();

    double t2 = now_sec();  // po kroku jadra

    // ==============================================================
    //  CESTA A — kdyz jede gpu-gles, kreslime jeho HOTOVOU texturu.
    //  Zadny core_get_frame, zadne nahravani pixelu - jen si vezmeme
    //  id textury z jadra a nakreslime ji. Ostry obraz, nula procesoru.
    // ==============================================================
    if (core_use_texture()) {
        int sx=0, sy=0, sw=0, sh=0;
        unsigned tex = core_get_texture(&sx, &sy, &sw, &sh);
        static long dbgA_total=0, dbgA_empty=0;
        dbgA_total++;
        if (tex == 0 || sw <= 0 || sh <= 0) {
            dbgA_empty++;
            if (dbgA_total % 180 == 0)
                LOGI("MUJLOG cestaA: vydano=%ld prazdno=%ld (gpu-gles textura)", dbgA_total-dbgA_empty, dbgA_empty);
            // nic noveho - necháme na obrazovce predchozi snimek
            eglSwapBuffers(e->display, e->surface);
            return;
        }
        int VW = core_vram_w(), VH = core_vram_h();

        // letterbox podle vyrezu hry
        int vw = w, vh = (w * sh) / sw;
        if (vh > h) { vh = h; vw = (h * sw) / sh; }
        glViewport((w - vw)/2, (h - vh)/2, vw, vh);
        glClearColor(0.f,0.f,0.f,1.f);
        glClear(GL_COLOR_BUFFER_BIT);

        // UV vyrez ve VRAM texture (Y v GL zdola -> prevraceni)
        float u0 = (float)sx / VW,        u1 = (float)(sx+sw) / VW;
        float glY = (float)(VH - (sy+sh));
        float v0 = (glY + sh) / VH,       v1 = glY / VH;

        const GLfloat quad[] = {
            -1.f,-1.f, u0,v0,   1.f,-1.f, u1,v0,
            -1.f, 1.f, u0,v1,   1.f, 1.f, u1,v1,
        };
        glUseProgram(e->program);
        glUniform1f(e->loc_mode, 1.0f); // gpu-gles textura je XRGB-like
        glBindTexture(GL_TEXTURE_2D, (GLuint)tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glVertexAttribPointer((GLuint)e->loc_pos, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), quad);
        glVertexAttribPointer((GLuint)e->loc_tex, 2, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), quad+2);
        glEnableVertexAttribArray((GLuint)e->loc_pos);
        glEnableVertexAttribArray((GLuint)e->loc_tex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        if (dbgA_total % 180 == 0)
            LOGI("MUJLOG cestaA: vydano=%ld prazdno=%ld vyrez=%dx%d (ostry gpu-gles)", dbgA_total-dbgA_empty, dbgA_empty, sw, sh);

        if (!eglSwapBuffers(e->display, e->surface)) {
            EGLint err = eglGetError();
            LOGE("eglSwapBuffers (cestaA) selhal: %s", egl_err_str(err));
            ANativeWindow* win = e->app->window;
            egl_term(e);
            if (win && (err==EGL_BAD_SURFACE||err==EGL_CONTEXT_LOST||err==EGL_BAD_NATIVE_WINDOW||err==EGL_BAD_DISPLAY)) {
                if (egl_init(e, win)) e->animating = true;
            }
        }
        return; // CESTA A hotova - dal nepokracujeme na softwarovou cestu
    }

    CoreFrame fr;
    bool have_frame = core_get_frame(&fr);

    // ==============================================================
    //  MUJ LOG — abych videl to, co Rene vidi na obrazovce, v cislech.
    //  Cizi log meri jen rozpad casu. Tenhle meri to podstatne:
    //    vydalo  = kolikrat jadro vydalo snimek (core_get_frame == true)
    //    prazdno = kolikrat NEvydalo nic (have_frame == false) - to je
    //              presne to, co na obrazovce vidis jako zamrznuti/blik
    //    posledni rozliseni + format - at vidim, kdyz se hra prepne
    //              (film 320x240 vs 3D, RGB565 vs XRGB)
    //  Kdyz "prazdno" roste, obraz na obrazovce stoji - a ja to konecne
    //  vidim v logu, ne az z tveho screenshotu.
    // ==============================================================
    static long dbg_total = 0, dbg_empty = 0;
    static int  dbg_last_w = 0, dbg_last_h = 0, dbg_last_fmt = -1;
    dbg_total++;
    if (!have_frame) dbg_empty++;
    else if (fr.width != dbg_last_w || fr.height != dbg_last_h || (int)fr.format != dbg_last_fmt) {
        dbg_last_w = fr.width; dbg_last_h = fr.height; dbg_last_fmt = (int)fr.format;
        LOGI("MUJLOG zmena obrazu: %dx%d format=%s",
             fr.width, fr.height,
             fr.format == CORE_FMT_RGB565 ? "RGB565(film/2D)" :
             fr.format == CORE_FMT_XRGB8888 ? "XRGB8888(3D)" : "jiny");
    }
    if (dbg_total % 180 == 0) {
        LOGI("MUJLOG snimky: vydano=%ld prazdno=%ld (%.1f%% ztraceno) posledni=%dx%d",
             dbg_total - dbg_empty, dbg_empty,
             dbg_total ? (100.0 * dbg_empty / dbg_total) : 0.0,
             dbg_last_w, dbg_last_h);
    }

    if (have_frame) {
        ensure_texture(e, fr.width, fr.height, fr.format);
        glBindTexture(GL_TEXTURE_2D, e->texture);
        int    bppx  = (fr.format == CORE_FMT_RGB565) ? 2 : 4;
        int    tight = fr.width * bppx;
        int    pitch = (fr.pitch > 0) ? fr.pitch : tight;
        GLenum gfmt  = (fr.format == CORE_FMT_RGB565) ? GL_RGB : GL_RGBA;
        GLenum gtyp  = (fr.format == CORE_FMT_RGB565) ? GL_UNSIGNED_SHORT_5_6_5
                                                      : GL_UNSIGNED_BYTE;
        if (pitch == tight) {
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, fr.width, fr.height,
                            gfmt, gtyp, fr.pixels);
        } else {
            // Jadro ma radky dal od sebe (pitch) -> nahrat po radcich.
            // Porad zadny prepocet na CPU, jen vic malych nahrani.
            const unsigned char* src = (const unsigned char*)fr.pixels;
            for (int y = 0; y < fr.height; y++) {
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, y, fr.width, 1,
                                gfmt, gtyp, src);
                src += pitch;
            }
        }
    }

    double t3 = now_sec();  // po nahrani textury do grafiky

    if (have_frame) {
        // 3) Letterbox podle AKTUALNIHO rozliseni hry - roztazeni na
        //    fullscreen displeje dela cele GPU, pomer stran sedi vzdy.
        int vw = w;
        int vh = (w * fr.height) / fr.width;
        if (vh > h) { vh = h; vw = (h * fr.width) / fr.height; }
        glViewport((w - vw) / 2, (h - vh) / 2, vw, vh);

        // 4) Vykresleni quadu s texturou (prohozeni barev dela shader)
        static const GLfloat quad[] = {
            // x,     y,      u,    v   (v je obracene: radek 0 nahore)
            -1.0f, -1.0f,   0.0f, 1.0f,
             1.0f, -1.0f,   1.0f, 1.0f,
            -1.0f,  1.0f,   0.0f, 0.0f,
             1.0f,  1.0f,   1.0f, 0.0f,
        };
        glUseProgram(e->program);
        glUniform1f(e->loc_mode, (fr.format == CORE_FMT_XRGB8888) ? 1.0f : 0.0f);
        glVertexAttribPointer((GLuint)e->loc_pos, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(GLfloat), quad);
        glVertexAttribPointer((GLuint)e->loc_tex, 2, GL_FLOAT, GL_FALSE,
                              4 * sizeof(GLfloat), quad + 2);
        glEnableVertexAttribArray((GLuint)e->loc_pos);
        glEnableVertexAttribArray((GLuint)e->loc_tex);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

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
            LOGI("Bezi: %ld snimku, ~%.1f FPS, %dx%d | ms/snimek: pozadi %.2f + jadro %.2f + nahrani %.2f + kresleni %.2f + swap %.2f = %.2f",
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
    app->onInputEvent = handle_input; // OVLADANI: dotyk -> PS1 tlacitka (v C)

    logserver_set_upload_dir(app->activity->internalDataPath);
    logserver_start(8765);
    LOGI("=== AH EGL Render start (verze 1.4-PS1-NABEH) ===");
    core_init(app->activity->vm, app->activity->internalDataPath);

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
