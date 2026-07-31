/*
 * naples2_gl.c - GLES2 renderer pro PS1 GPU
 * Viz naples2_gl.h. Vse se kresli v souradnicich VRAM 1:1 (0..1023 x 0..511),
 * zadne prepocty na displej - odtud pochazely chyby v puvodnim gpu-gles.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include "naples2_gl.h"

extern void nap_diag_log(const char *fmt, ...);

/* gpu.vram - autoritativni kopie VRAM v pameti (drzi ji gpulib) */
extern unsigned short *n2_host_vram(void);

/* ---------------------------------------------------------------- shadery */

static const char *VS =
"attribute vec2 aPos;\n"        /* souradnice VRAM 0..1023 / 0..511 */
"attribute vec3 aColor;\n"      /* 0..1 (z PS1 0..255) */
"attribute vec2 aUV;\n"         /* texel uvnitr stranky, 0..255 */
"attribute vec2 aPage;\n"       /* pocatek texturove stranky ve VRAM */
"attribute vec2 aClut;\n"       /* pozice palety ve VRAM */
"attribute float aMode;\n"      /* 0 = bez textury, 1 = 4bit, 2 = 8bit, 3 = 16bit */
"uniform vec2 uVram;\n"
"varying vec3 vColor;\n"
"varying vec2 vUV;\n"
"varying vec2 vPage;\n"
"varying vec2 vClut;\n"
"varying float vMode;\n"
"void main(){\n"
"  vec2 p = (aPos / uVram) * 2.0 - 1.0;\n"
"  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);\n"  /* VRAM y=0 nahore */
"  vColor = aColor; vUV = aUV; vPage = aPage; vClut = aClut; vMode = aMode;\n"
"}\n";

static const char *FS =
"precision highp float;\n"
"varying vec3 vColor;\n"
"varying vec2 vUV;\n"
"varying vec2 vPage;\n"
"varying vec2 vClut;\n"
"varying float vMode;\n"
"uniform sampler2D uVramTex;\n"
"uniform vec4 uTexWin;\n"   /* LUMINANCE_ALPHA = syrovych 16 bitu VRAM */
"uniform vec2 uVram;\n"
"uniform float uAlpha;\n"
"float raw16(vec2 texel){\n"
"  vec2 uv = (texel + 0.5) / uVram;\n"
"  vec4 t = texture2D(uVramTex, uv);\n"
"  return floor(t.r * 255.0 + 0.5) + floor(t.a * 255.0 + 0.5) * 256.0;\n"
"}\n"
"float okno(float u, float M, float O){\n"      /* PS1: (u AND NOT M) OR (O AND M) */
"  float res = 0.0, bit = 1.0;\n"
"  for (int i = 0; i < 8; i++) {\n"
"    float ub = mod(floor(u / bit), 2.0);\n"
"    float mb = mod(floor(M / bit), 2.0);\n"
"    float ob = mod(floor(O / bit), 2.0);\n"
"    res += ((mb > 0.5) ? ob : ub) * bit;\n"
"    bit *= 2.0;\n"
"  }\n"
"  return res;\n"
"}\n"
"vec4 from16(float v){\n"
"  float r = mod(v, 32.0);\n"
"  float g = mod(floor(v / 32.0), 32.0);\n"
"  float b = mod(floor(v / 1024.0), 32.0);\n"
"  return vec4(r / 31.0, g / 31.0, b / 31.0, 1.0);\n"
"}\n"
"void main(){\n"
"  vec3 c;\n"
"  if (vMode < 0.5) {\n"
"    c = vColor;\n"
"  } else {\n"
"    float u = floor(vUV.x);\n"
"    float v = floor(vUV.y);\n"
"    if (uTexWin.x > 0.5) u = okno(u, uTexWin.x, uTexWin.z);\n"   /* jen kdyz je maska */
"    if (uTexWin.y > 0.5) v = okno(v, uTexWin.y, uTexWin.w);\n"
"    float raw;\n"
"    if (vMode < 1.5) {\n"                    /* 4 bity na pixel */
"      raw = raw16(vec2(vPage.x + floor(u / 4.0), vPage.y + v));\n"
"      float idx = mod(floor(raw / exp2(mod(u, 4.0) * 4.0)), 16.0);\n"
"      raw = raw16(vec2(vClut.x + idx, vClut.y));\n"
"    } else if (vMode < 2.5) {\n"              /* 8 bitu na pixel */
"      raw = raw16(vec2(vPage.x + floor(u / 2.0), vPage.y + v));\n"
"      float idx = mod(floor(raw / exp2(mod(u, 2.0) * 8.0)), 256.0);\n"
"      raw = raw16(vec2(vClut.x + idx, vClut.y));\n"
"    } else {\n"                               /* 16 bitu primo */
"      raw = raw16(vec2(vPage.x + u, vPage.y + v));\n"
"    }\n"
"    if (raw < 0.5) discard;\n"                /* nulovy texel = pruhledny */
"    c = from16(raw).rgb * vColor * 2.0;\n"    /* PS1 modulace: 128 = neutral */
"  }\n"
"  gl_FragColor = vec4(clamp(c, 0.0, 1.0), uAlpha);\n"
"}\n";

/* prenos obdelniku z pameti do VRAM (logo BIOSu, snimek filmu, textury) */
static const char *VS_BLIT =
"attribute vec2 aPos;\n"
"attribute vec2 aUV;\n"
"uniform vec2 uVram;\n"
"varying vec2 vUV;\n"
"void main(){\n"
"  vec2 p = (aPos / uVram) * 2.0 - 1.0;\n"
"  gl_Position = vec4(p.x, -p.y, 0.0, 1.0);\n"
"  vUV = aUV;\n"
"}\n";

static const char *FS_BLIT =
"precision mediump float;\n"
"varying vec2 vUV;\n"
"uniform sampler2D uSrc;\n"
"void main(){ gl_FragColor = texture2D(uSrc, vUV); }\n";

/* ------------------------------------------------------------------ stav */

typedef struct {
    float x, y;
    float r, g, b;
    float u, v;
    float px, py;
    float cx, cy;
    float mode;
} N2Vert;

#define N2_MAX_VERTS 24576

static struct {
    int      ready;
    GLuint   prog, prog_blit;
    GLint    a_pos, a_col, a_uv, a_page, a_clut, a_mode;
    GLint    u_vram, u_tex, u_texwin, u_alpha;
    GLint    b_pos, b_uv, b_vram, b_src;
    GLuint   fbo, tex_out;      /* cil kresleni: cela VRAM jako RGBA */
    GLuint   tex_vram;          /* syrova VRAM (LUMINANCE_ALPHA) pro texturovani */
    GLuint   tex_blit;          /* docasna textura pro prenosy */
    N2Vert   verts[N2_MAX_VERTS];
    int      nverts;
    int      blend;             /* -1 = neprusvitne, 0..3 = rezim PS1 */
    /* stav z prikazu E1..E6 */
    int      page_x, page_y, page_mode, page_blend;
    int      clut_x, clut_y;
    int      win_mx, win_my, win_ox, win_oy;
    int      area_x0, area_y0, area_x1, area_y1;
    int      off_x, off_y;
    /* Cekajici zapisy do VRAM. Drive se kazdy zapis hned prekresloval do
       obrazu - her je takovych zapisu za snimek i stovky a na mobilnim GPU
       to znamena stovky prepnuti stavu a kreslicich volani, tedy trhani.
       Ted se slouci do jednoho obdelniku a prekresli se najednou, ale VZDY
       jeste PRED dalsim kreslenim primitiv, aby se poradi zachovalo. */
    /* Seznam cekajicich zapisu. Drive se slucovaly do JEDNOHO obdelniku -
       jenze kdyz hra zapise na dve vzdalena mista, takovy obdelnik pokryje
       i vsechno mezi nimi a prekresli se surovym obsahem pameti. Tim se
       smaze to, co GPU spravne nakreslila (u menu BIOSu z toho byla zelena
       zmet pres text). Ted se kazdy zapis prekresli zvlast. */
    int      dirty;                 /* kolik jich ceka */
    int      dr[64][4];             /* x0,y0,x1,y1 */
    long     n_writes, n_blits, n_draws, n_verts, n_trans;  /* pocitadla pro diagnostiku */
    int      seen[8][5];      /* stranka x,y, rezim, paleta x,y - co se opravdu pouziva */
    int      n_seen;
    unsigned char *scratch;     /* prevod 16bit -> RGBA pri prenosech */
    unsigned char *readbuf;     /* zaloha pro nesdileny kontext */
    int      readbuf_cap;
} n2;

/* ------------------------------------------------------------- pomocnici */

static void n2_flush_pending_vram(void);   /* definice nize - vola se uz z n2_flush */

static GLuint n2_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, NULL);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; GLsizei n = 0;
        glGetShaderInfoLog(s, sizeof(log) - 1, &n, log);
        log[n > 0 ? n : 0] = 0;
        nap_diag_log("NAPLES2 SHADER_FAIL typ=%d: %s", (int)type, log);
        glDeleteShader(s);
        return 0;
    }
    return s;
}

static GLuint n2_program(const char *vs_src, const char *fs_src)
{
    GLuint vs = n2_shader(GL_VERTEX_SHADER, vs_src);
    GLuint fs = n2_shader(GL_FRAGMENT_SHADER, fs_src);
    if (!vs || !fs) return 0;
    GLuint p = glCreateProgram();
    glAttachShader(p, vs);
    glAttachShader(p, fs);
    glLinkProgram(p);
    GLint ok = 0;
    glGetProgramiv(p, GL_LINK_STATUS, &ok);
    glDeleteShader(vs);
    glDeleteShader(fs);
    if (!ok) {
        char log[512]; GLsizei n = 0;
        glGetProgramInfoLog(p, sizeof(log) - 1, &n, log);
        log[n > 0 ? n : 0] = 0;
        nap_diag_log("NAPLES2 LINK_FAIL: %s", log);
        glDeleteProgram(p);
        return 0;
    }
    return p;
}

/* ------------------------------------------------------------------ init */

int n2_init(void)
{
    if (n2.ready) return 0;
    memset(&n2, 0, sizeof(n2));

    n2.prog = n2_program(VS, FS);
    if (!n2.prog) return -1;
    n2.prog_blit = n2_program(VS_BLIT, FS_BLIT);
    if (!n2.prog_blit) return -1;

    n2.a_pos  = glGetAttribLocation(n2.prog, "aPos");
    n2.a_col  = glGetAttribLocation(n2.prog, "aColor");
    n2.a_uv   = glGetAttribLocation(n2.prog, "aUV");
    n2.a_page = glGetAttribLocation(n2.prog, "aPage");
    n2.a_clut = glGetAttribLocation(n2.prog, "aClut");
    n2.a_mode = glGetAttribLocation(n2.prog, "aMode");
    n2.u_vram   = glGetUniformLocation(n2.prog, "uVram");
    n2.u_tex    = glGetUniformLocation(n2.prog, "uVramTex");
    n2.u_texwin = glGetUniformLocation(n2.prog, "uTexWin");
    n2.u_alpha  = glGetUniformLocation(n2.prog, "uAlpha");

    n2.b_pos  = glGetAttribLocation(n2.prog_blit, "aPos");
    n2.b_uv   = glGetAttribLocation(n2.prog_blit, "aUV");
    n2.b_vram = glGetUniformLocation(n2.prog_blit, "uVram");
    n2.b_src  = glGetUniformLocation(n2.prog_blit, "uSrc");

    /* cil kresleni = cela VRAM jako RGBA textura */
    glGenTextures(1, &n2.tex_out);
    glBindTexture(GL_TEXTURE_2D, n2.tex_out);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, N2_VRAM_W, N2_VRAM_H, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenFramebuffers(1, &n2.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, n2.fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D, n2.tex_out, 0);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        nap_diag_log("NAPLES2 FBO_FAIL stav=0x%x", glCheckFramebufferStatus(GL_FRAMEBUFFER));
        return -1;
    }
    glViewport(0, 0, N2_VRAM_W, N2_VRAM_H);
    glClearColor(0.f, 0.f, 0.f, 1.f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* syrova VRAM pro texturovani: 2 bajty na texel */
    glGenTextures(1, &n2.tex_vram);
    glBindTexture(GL_TEXTURE_2D, n2.tex_vram);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE_ALPHA, N2_VRAM_W, N2_VRAM_H, 0,
                 GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenTextures(1, &n2.tex_blit);


    n2.scratch = (unsigned char *)malloc((size_t)N2_VRAM_W * N2_VRAM_H * 4);
    if (!n2.scratch) return -1;

    n2.blend    = -1;
    n2.win_mx   = 0; n2.win_my = 0; n2.win_ox = 0; n2.win_oy = 0;   /* 0 = bez maskovani */
    n2.area_x1  = N2_VRAM_W; n2.area_y1 = N2_VRAM_H;
    n2.ready    = 1;
    nap_diag_log("NAPLES2 PRIPRAVEN (GLES2, VRAM %dx%d, textura=%u)", N2_VRAM_W, N2_VRAM_H, n2.tex_out);
    return 0;
}

void n2_finish(void)
{
    if (!n2.ready) return;
    glDeleteFramebuffers(1, &n2.fbo);
    glDeleteTextures(1, &n2.tex_out);
    glDeleteTextures(1, &n2.tex_vram);
    glDeleteTextures(1, &n2.tex_blit);
    glDeleteProgram(n2.prog);
    glDeleteProgram(n2.prog_blit);
    free(n2.scratch); n2.scratch = NULL;
    free(n2.readbuf); n2.readbuf = NULL;
    n2.ready = 0;
}

/* -------------------------------------------------------------- kresleni */

static void n2_apply_blend(void)
{
    switch (n2.blend) {
    case 0: /* pul pozadi + pul popredi */
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    case 1: /* pozadi + popredi */
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    case 2: /* pozadi - popredi */
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        glBlendFunc(GL_ONE, GL_ONE);
        break;
    case 3: /* pozadi + ctvrtina popredi */
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_ADD);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    default:
        glDisable(GL_BLEND);
        break;
    }
}

void n2_flush(void)
{
    if (!n2.ready) return;
    n2_flush_pending_vram();   /* zapisy do VRAM musi byt v obraze DRIV nez primitiva */
    if (n2.nverts <= 0) return;
    n2.n_draws++;
    n2.n_verts += n2.nverts;
    if (n2.blend >= 0) n2.n_trans += n2.nverts;

    glBindFramebuffer(GL_FRAMEBUFFER, n2.fbo);
    glViewport(0, 0, N2_VRAM_W, N2_VRAM_H);

    /* orez = oblast kresleni, primo v souradnicich VRAM */
    glEnable(GL_SCISSOR_TEST);
    {
        int x0 = n2.area_x0, y0 = n2.area_y0;
        int x1 = n2.area_x1, y1 = n2.area_y1;
        if (x1 > N2_VRAM_W) x1 = N2_VRAM_W;
        if (y1 > N2_VRAM_H) y1 = N2_VRAM_H;
        if (x0 < 0) x0 = 0;
        if (y0 < 0) y0 = 0;
        if (x1 < x0) x1 = x0;
        if (y1 < y0) y1 = y0;
        /* glScissor pocita zdola, nase VRAM shora */
        glScissor(x0, N2_VRAM_H - y1, x1 - x0, y1 - y0);
    }

    glUseProgram(n2.prog);
    glUniform2f(n2.u_vram, (float)N2_VRAM_W, (float)N2_VRAM_H);
    glUniform4f(n2.u_texwin, (float)n2.win_mx, (float)n2.win_my,
                             (float)n2.win_ox, (float)n2.win_oy);
    glUniform1f(n2.u_alpha, (n2.blend == 0) ? 0.5f : ((n2.blend == 3) ? 0.25f : 1.0f));
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, n2.tex_vram);
    glUniform1i(n2.u_tex, 0);
    n2_apply_blend();

    {
        const char *base = (const char *)n2.verts;
        GLsizei st = (GLsizei)sizeof(N2Vert);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glVertexAttribPointer((GLuint)n2.a_pos,  2, GL_FLOAT, GL_FALSE, st, base + 0);
        glVertexAttribPointer((GLuint)n2.a_col,  3, GL_FLOAT, GL_FALSE, st, base + 8);
        glVertexAttribPointer((GLuint)n2.a_uv,   2, GL_FLOAT, GL_FALSE, st, base + 20);
        glVertexAttribPointer((GLuint)n2.a_page, 2, GL_FLOAT, GL_FALSE, st, base + 28);
        glVertexAttribPointer((GLuint)n2.a_clut, 2, GL_FLOAT, GL_FALSE, st, base + 36);
        glVertexAttribPointer((GLuint)n2.a_mode, 1, GL_FLOAT, GL_FALSE, st, base + 44);
        glEnableVertexAttribArray((GLuint)n2.a_pos);
        glEnableVertexAttribArray((GLuint)n2.a_col);
        glEnableVertexAttribArray((GLuint)n2.a_uv);
        glEnableVertexAttribArray((GLuint)n2.a_page);
        glEnableVertexAttribArray((GLuint)n2.a_clut);
        glEnableVertexAttribArray((GLuint)n2.a_mode);
        glDrawArrays(GL_TRIANGLES, 0, n2.nverts);
    }
    glDisable(GL_SCISSOR_TEST);
    n2.nverts = 0;
}

static void n2_vert(int x, int y, unsigned rgb, int u, int v, int mode)
{
    N2Vert *o;
    if (n2.nverts >= N2_MAX_VERTS) n2_flush();
    o = &n2.verts[n2.nverts++];
    o->x = (float)(x + n2.off_x);
    o->y = (float)(y + n2.off_y);
    o->r = (float)( rgb        & 0xff) / 255.0f;
    o->g = (float)((rgb >>  8) & 0xff) / 255.0f;
    o->b = (float)((rgb >> 16) & 0xff) / 255.0f;
    o->u = (float)u;  o->v = (float)v;
    o->px = (float)n2.page_x; o->py = (float)n2.page_y;
    o->cx = (float)n2.clut_x; o->cy = (float)n2.clut_y;
    o->mode = (float)mode;
    if (mode != 0) {          /* zaznamenat pouzitou texturu (diagnostika) */
        int q, nasel = 0;
        for (q = 0; q < n2.n_seen; q++)
            if (n2.seen[q][0] == n2.page_x && n2.seen[q][1] == n2.page_y &&
                n2.seen[q][2] == mode && n2.seen[q][3] == n2.clut_x &&
                n2.seen[q][4] == n2.clut_y) { nasel = 1; break; }
        if (!nasel && n2.n_seen < 8) {
            n2.seen[n2.n_seen][0] = n2.page_x; n2.seen[n2.n_seen][1] = n2.page_y;
            n2.seen[n2.n_seen][2] = mode;
            n2.seen[n2.n_seen][3] = n2.clut_x; n2.seen[n2.n_seen][4] = n2.clut_y;
            n2.n_seen++;
        }
    }
}

static void n2_set_blend(int b)
{
    if (b != n2.blend) { n2_flush(); n2.blend = b; }
}

/* ------------------------------------------------------- prenosy do VRAM */

/* Nakresli RGBA obdelnik ze 'scratch' do obrazu (FBO) na pozici x,y. */
static void n2_blit_scratch(int x, int y, int w, int h)
{
    glBindTexture(GL_TEXTURE_2D, n2.tex_blit);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, n2.scratch);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, n2.fbo);
    glViewport(0, 0, N2_VRAM_W, N2_VRAM_H);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glUseProgram(n2.prog_blit);
    glUniform2f(n2.b_vram, (float)N2_VRAM_W, (float)N2_VRAM_H);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, n2.tex_blit);
    glUniform1i(n2.b_src, 0);
    {
        float x0 = (float)x, y0 = (float)y, x1 = (float)(x + w), y1 = (float)(y + h);
        const float pos[12] = { x0,y0,  x1,y0,  x0,y1,   x1,y0,  x1,y1,  x0,y1 };
        const float uv [12] = { 0,0,    1,0,    0,1,     1,0,    1,1,    0,1  };
        glVertexAttribPointer((GLuint)n2.b_pos, 2, GL_FLOAT, GL_FALSE, 0, pos);
        glVertexAttribPointer((GLuint)n2.b_uv,  2, GL_FLOAT, GL_FALSE, 0, uv);
        glEnableVertexAttribArray((GLuint)n2.b_pos);
        glEnableVertexAttribArray((GLuint)n2.b_uv);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
}

/* Nahraje CELOU videopamet do texturovaci textury.
   PROC: renderer se dosud spolehal, ze mu jadro kazdy zapis do videopameti
   ohlasi (renderer_update_caches). Log ale ukazal "zapisyVRAM=0" - u her,
   ktere nahravaji textury pres DMA, hlaseni nechodi vubec. Textury pak byly
   PRAZDNE, kazdy texturovany pixel se zahodil a na obrazovce zbyly jen
   jednobarevne plochy. Tohle na hlaseni nezavisi. */
void n2_upload_all_vram(void)
{
    const unsigned short *src = n2_host_vram();
    if (!n2.ready || !src) return;
    glBindTexture(GL_TEXTURE_2D, n2.tex_vram);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    /* 16bit slova videopameti = presne 2 bajty na texel (LUMINANCE_ALPHA),
       takze zadny prevod - primy prenos pameti. */
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, N2_VRAM_W, N2_VRAM_H,
                    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, src);
}

/* Prekresli najednou vsechny cekajici zapisy do VRAM. */
static void n2_flush_pending_vram(void)
{
    const unsigned short *src = n2_host_vram();
    int k, n;
    if (!n2.ready || !n2.dirty || !src) return;
    n = n2.dirty;
    n2.dirty = 0;
    for (k = 0; k < n; k++) {
        int x = n2.dr[k][0], y = n2.dr[k][1];
        int w = n2.dr[k][2] - x, h = n2.dr[k][3] - y;
        int i, j;
        if (w <= 0 || h <= 0) continue;
        for (j = 0; j < h; j++) {
            const unsigned short *s2 = src + (size_t)(y + j) * N2_VRAM_W + x;
            unsigned char *d = n2.scratch + (size_t)j * w * 4;
            for (i = 0; i < w; i++) {
                unsigned p = s2[i];
                d[i * 4 + 0] = (unsigned char)((p        & 31) * 255 / 31);
                d[i * 4 + 1] = (unsigned char)(((p >> 5) & 31) * 255 / 31);
                d[i * 4 + 2] = (unsigned char)(((p >> 10) & 31) * 255 / 31);
                d[i * 4 + 3] = 255;
            }
        }
        n2_blit_scratch(x, y, w, h);
        n2.n_blits++;
    }
}

/* 24bitovy rezim (film): ve VRAM lezi 3 bajty na pixel (R,G,B), radek VRAM
   ma N2_VRAM_W*2 bajtu. Cist to jako 15bit dava spravnou geometrii, ale
   rozsypane barvy - proto zvlastni dekod. */
void n2_present_rgb24(int sx, int sy, int w, int h)
{
    const unsigned short *src = n2_host_vram();
    int i, j, max_px;
    if (!n2.ready || !src || w <= 0 || h <= 0) return;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx > N2_VRAM_W - 1) sx = N2_VRAM_W - 1;
    if (sy > N2_VRAM_H - 1) sy = N2_VRAM_H - 1;
    if (sy + h > N2_VRAM_H) h = N2_VRAM_H - sy;
    max_px = (N2_VRAM_W * 2 - sx * 2) / 3;      /* neprecist za pravy okraj radku */
    if (w > max_px) w = max_px;
    if (w <= 0 || h <= 0) return;

    n2_flush();
    n2.dirty = 0;   /* film prepisuje celou plochu displeje - starsi zapisy netreba */
    for (j = 0; j < h; j++) {
        const unsigned char *s = (const unsigned char *)(src + (size_t)(sy + j) * N2_VRAM_W + sx);
        unsigned char *d = n2.scratch + (size_t)j * w * 4;
        for (i = 0; i < w; i++) {
            d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
            d += 4; s += 3;
        }
    }
    n2_blit_scratch(sx, sy, w, h);
}

void n2_vram_written(int x, int y, int w, int h)
{
    const unsigned short *src = n2_host_vram();
    int i, j;
    if (!n2.ready || !src || w <= 0 || h <= 0) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > N2_VRAM_W) w = N2_VRAM_W - x;
    if (y + h > N2_VRAM_H) h = N2_VRAM_H - y;
    if (w <= 0 || h <= 0) return;

    /* 1) syrova data pro texturovani */
    for (j = 0; j < h; j++) {
        const unsigned short *s = src + (size_t)(y + j) * N2_VRAM_W + x;
        unsigned char *d = n2.scratch + (size_t)j * w * 2;
        for (i = 0; i < w; i++) {
            d[i * 2 + 0] = (unsigned char)( s[i]       & 0xff); /* nizsi bajt -> r */
            d[i * 2 + 1] = (unsigned char)((s[i] >> 8) & 0xff); /* vyssi bajt -> a */
        }
    }
    glBindTexture(GL_TEXTURE_2D, n2.tex_vram);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h,
                    GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, n2.scratch);

    /* 2) do obrazu se to prekresli az naraz (viz n2_flush_pending_vram) -
          jen si oznacime oblast. Drive se kreslilo hned pri kazdem zapisu
          a to trhalo obraz. */
    if (n2.dirty < 64) {
        n2.dr[n2.dirty][0] = x; n2.dr[n2.dirty][1] = y;
        n2.dr[n2.dirty][2] = x + w; n2.dr[n2.dirty][3] = y + h;
        n2.dirty++;
    } else {
        /* Kdyby jich bylo opravdu hodne, posledni rozsirime - poradi se tim
           nezmeni a nehrozi, ze bychom zapis ztratili. */
        int *r = n2.dr[63];
        if (x < r[0]) r[0] = x;
        if (y < r[1]) r[1] = y;
        if (x + w > r[2]) r[2] = x + w;
        if (y + h > r[3]) r[3] = y + h;
    }
    n2.n_writes++;
}

void n2_vram_sync_to_cpu(int x, int y, int w, int h)
{
    unsigned short *dst = n2_host_vram();
    int i, j;
    if (!n2.ready || !dst || w <= 0 || h <= 0) return;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (x + w > N2_VRAM_W) w = N2_VRAM_W - x;
    if (y + h > N2_VRAM_H) h = N2_VRAM_H - y;
    if (w <= 0 || h <= 0) return;

    n2_flush();
    glBindFramebuffer(GL_FRAMEBUFFER, n2.fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(x, N2_VRAM_H - (y + h), w, h, GL_RGBA, GL_UNSIGNED_BYTE, n2.scratch);
    for (j = 0; j < h; j++) {
        const unsigned char *s = n2.scratch + (size_t)(h - 1 - j) * w * 4; /* GL cte zdola */
        unsigned short *d = dst + (size_t)(y + j) * N2_VRAM_W + x;
        for (i = 0; i < w; i++) {
            unsigned r = s[i * 4 + 0] * 31 / 255;
            unsigned g = s[i * 4 + 1] * 31 / 255;
            unsigned b = s[i * 4 + 2] * 31 / 255;
            d[i] = (unsigned short)(r | (g << 5) | (b << 10));
        }
    }
}

/* ---------------------------------------------------------------- vystup */

unsigned n2_vram_texture(void) { return n2.ready ? n2.tex_out : 0; }

const void* n2_last_display(void) { return (n2.ready && n2.readbuf_cap > 0) ? n2.readbuf : NULL; }

void n2_sync_for_other_context(void)
{
    if (!n2.ready) return;
    /* Podminka GL/EGL pro sdilene objekty: producent musi praci odeslat,
       jinak si ji konzument nemusi precist hotovou. */
    glFlush();
}

unsigned n2_peek_pixel(int x, int y)
{
    unsigned char px[4] = {0,0,0,0};
    if (!n2.ready) return 0;
    if (x < 0) x = 0;
    if (x > N2_VRAM_W - 1) x = N2_VRAM_W - 1;
    if (y < 0) y = 0;
    if (y > N2_VRAM_H - 1) y = N2_VRAM_H - 1;
    glBindFramebuffer(GL_FRAMEBUFFER, n2.fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    /* radek 0 videopameti lezi v obrazu nahore, proto prevraceni */
    glReadPixels(x, N2_VRAM_H - 1 - y, 1, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
    return ((unsigned)px[0] << 16) | ((unsigned)px[1] << 8) | (unsigned)px[2];
}

int n2_area_x0(void) { return n2.area_x0; }
int n2_area_y0(void) { return n2.area_y0; }
int n2_area_x1(void) { return n2.area_x1; }
int n2_area_y1(void) { return n2.area_y1; }

void n2_take_counters(long *draws, long *writes, long *blits)
{
    if (draws)  *draws  = n2.n_draws;
    if (writes) *writes = n2.n_writes;
    if (blits)  *blits  = n2.n_blits;
    n2.n_draws = n2.n_writes = n2.n_blits = 0;
}

/* Vypise, jake texturove stranky a palety se pouzivaji, a vynuluje seznam. */
void n2_dump_textures(char *out, int cap)
{
    int q, n = 0;
    if (!out || cap < 8) return;
    out[0] = 0;
    for (q = 0; q < n2.n_seen && n < cap - 40; q++) {
        const char *rez = (n2.seen[q][2] == 1) ? "4bit" : (n2.seen[q][2] == 2) ? "8bit" : "16bit";
        n += snprintf(out + n, (size_t)(cap - n), "[str %d,%d %s pal %d,%d] ",
                      n2.seen[q][0], n2.seen[q][1], rez, n2.seen[q][3], n2.seen[q][4]);
    }
    n2.n_seen = 0;
}

void n2_take_vert_counters(long *verts, long *trans)
{
    if (verts) *verts = n2.n_verts;
    if (trans) *trans = n2.n_trans;
    n2.n_verts = n2.n_trans = 0;
}

const void* n2_read_display(int sx, int sy, int w, int h)
{
    int need;
    if (!n2.ready || w <= 0 || h <= 0) return NULL;
    if (sx < 0) sx = 0;
    if (sy < 0) sy = 0;
    if (sx + w > N2_VRAM_W) w = N2_VRAM_W - sx;
    if (sy + h > N2_VRAM_H) h = N2_VRAM_H - sy;
    if (w <= 0 || h <= 0) return NULL;

    need = w * h * 4;
    if (need > n2.readbuf_cap) {
        unsigned char *nb = (unsigned char *)realloc(n2.readbuf, (size_t)need);
        if (!nb) return NULL;
        n2.readbuf = nb; n2.readbuf_cap = need;
    }
    n2_flush();
    glBindFramebuffer(GL_FRAMEBUFFER, n2.fbo);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    /* VRAM y=0 je nahore, glReadPixels cte zdola - eglrender ceka zdola nahoru */
    glReadPixels(sx, N2_VRAM_H - (sy + h), w, h, GL_RGBA, GL_UNSIGNED_BYTE, n2.readbuf);
    return n2.readbuf;
}

/* ------------------------------------------------------ prikazy GP0 E1-E6 */

static void n2_texpage(unsigned v)
{
    /* ZADNY n2_flush(): stranka i paleta jsou ulozene u KAZDEHO vrcholu,
       takze se davka delit nemusi. Hry meni stranku mnohokrat za snimek -
       drivejsi dokreslovani z toho delalo stovky kreslicich volani za snimek
       a to kouslo obraz i zvuk. */
    n2.page_x = (int)((v & 0x0f) * 64);
    n2.page_y = (int)(((v >> 4) & 1) * 256);
    n2.page_blend = (int)((v >> 5) & 3);
    n2.page_mode  = (int)((v >> 7) & 3);   /* 0=4bit 1=8bit 2=16bit */
}

static void n2_texwin(unsigned v)
{
    int mx = (int)(v & 0x1f), my = (int)((v >> 5) & 0x1f);
    int ox = (int)((v >> 10) & 0x1f), oy = (int)((v >> 15) & 0x1f);
    n2_flush();
    /* Posilame primo MASKU a OFFSET v pixelech - shader si udela presne
       to, co dela PS1: (u AND NOT maska) OR (offset AND maska).
       Drive se tu pocitala "velikost okna" a v shaderu se delal zbytek po
       deleni; to sedi jen pro nektere masky a u ostatnich se textura cetla
       posunuta a opakovala se v malych dlazdicich. */
    n2.win_mx = mx * 8;
    n2.win_my = my * 8;
    n2.win_ox = ox * 8;
    n2.win_oy = oy * 8;
}

void n2_set_ecmds(uint32_t *e)
{
    if (!e) return;
    n2_texpage(e[1] & 0xffffff);
    n2_texwin (e[2] & 0xffffff);
    n2_flush();
    n2.area_x0 = (int)( e[3]        & 0x3ff);
    n2.area_y0 = (int)((e[3] >> 10) & 0x3ff);
    n2.area_x1 = (int)( e[4]        & 0x3ff) + 1;
    n2.area_y1 = (int)((e[4] >> 10) & 0x3ff) + 1;
    n2.off_x   = (int)(((int)( e[5]        & 0x7ff) << 21) >> 21);
    n2.off_y   = (int)(((int)((e[5] >> 11) & 0x7ff) << 21) >> 21);
}

/* -------------------------------------------------- zpracovani GP0 davky */

static int n2_s11(unsigned v) { return (int)(((int)(v & 0x7ff)) << 21) >> 21; }

int n2_do_cmd_list(uint32_t *list, int list_len, uint32_t *ex_regs, int *last_cmd)
{
    uint32_t *start = list, *end = list + list_len;
    unsigned cmd = (unsigned)-1;
    extern const unsigned char cmd_lengths[256];

    if (!n2.ready) { if (last_cmd) *last_cmd = -1; return list_len; }

    while (list < end) {
        unsigned len;
        cmd = *list >> 24;
        len = cmd_lengths[cmd];
        if (list + 1 + len > end) { cmd = (unsigned)-1; break; }

        /* gpulib chce mit registry E1..E6 aktualni (obnova stavu, save state) */
        if (ex_regs && cmd >= 0xe1 && cmd <= 0xe6)
            ex_regs[cmd & 7] = list[0] & 0xffffff;

        if (cmd >= 0x20 && cmd <= 0x3f) {          /* mnohouhelniky */
            int quad   = (cmd & 8) ? 1 : 0;
            int shaded = (cmd & 0x10) ? 1 : 0;
            int textur = (cmd & 4) ? 1 : 0;
            int trans  = (cmd & 2) ? 1 : 0;

            int nv     = quad ? 4 : 3;
            int i, k = 1;
            int px[4], py[4], pu[4], pv[4];
            unsigned pc[4];
            unsigned base = list[0] & 0xffffff;

            for (i = 0; i < nv; i++) {
                pc[i] = (i == 0 || !shaded) ? base : (list[k++] & 0xffffff);
                px[i] = n2_s11((unsigned)(list[k] & 0xffff));
                py[i] = n2_s11((unsigned)((list[k] >> 16) & 0xffff));
                k++;
                if (textur) {
                    pu[i] = (int)( list[k]        & 0xff);
                    pv[i] = (int)((list[k] >>  8) & 0xff);
                    if (i == 0) { unsigned c = (list[k] >> 16) & 0xffff;
                                  n2.clut_x = (int)((c & 0x3f) * 16);
                                  n2.clut_y = (int)((c >> 6) & 0x1ff); }
                    if (i == 1) { n2_texpage((list[k] >> 16) & 0xffff); }
                    k++;
                } else { pu[i] = pv[i] = 0; }
            }
            n2_set_blend(trans ? n2.page_blend : -1);
            {
                int mode = textur ? (n2.page_mode == 0 ? 1 : (n2.page_mode == 1 ? 2 : 3)) : 0;
                int idx[6] = {0,1,2, 1,2,3};
                int cnt = quad ? 6 : 3;
                for (i = 0; i < cnt; i++) {
                    int t = idx[i];
                    n2_vert(px[t], py[t], pc[t], pu[t], pv[t], mode);
                }
            }
            list += 1 + len;
            continue;
        }

        if (cmd >= 0x60 && cmd <= 0x7f) {          /* obdelniky a sprity */
            unsigned base = list[0] & 0xffffff;
            int textur = (cmd & 4) ? 1 : 0;
            int trans  = (cmd & 2) ? 1 : 0;
            int k = 1, w, h, u0 = 0, v0 = 0;
            int x = n2_s11((unsigned)(list[k] & 0xffff));
            int y = n2_s11((unsigned)((list[k] >> 16) & 0xffff));
            k++;
            if (textur) {
                u0 = (int)( list[k]        & 0xff);
                v0 = (int)((list[k] >>  8) & 0xff);
                { unsigned c = (list[k] >> 16) & 0xffff;
                  n2.clut_x = (int)((c & 0x3f) * 16);
                  n2.clut_y = (int)((c >> 6) & 0x1ff); }
                k++;
            }
            switch ((cmd >> 3) & 3) {
            case 0:  w = (int)( list[k] & 0x3ff);
                     h = (int)((list[k] >> 16) & 0x1ff); break;
            case 1:  w = h = 1;  break;
            case 2:  w = h = 8;  break;
            default: w = h = 16; break;
            }
            n2_set_blend(trans ? n2.page_blend : -1);
            {
                int mode = textur ? (n2.page_mode == 0 ? 1 : (n2.page_mode == 1 ? 2 : 3)) : 0;
                n2_vert(x,     y,     base, u0,     v0,     mode);
                n2_vert(x + w, y,     base, u0 + w, v0,     mode);
                n2_vert(x,     y + h, base, u0,     v0 + h, mode);
                n2_vert(x + w, y,     base, u0 + w, v0,     mode);
                n2_vert(x + w, y + h, base, u0 + w, v0 + h, mode);
                n2_vert(x,     y + h, base, u0,     v0 + h, mode);
            }
            list += 1 + len;
            continue;
        }

        if (cmd >= 0x40 && cmd <= 0x5f) {          /* cary - kreslime jako tenke obdelniky */
            unsigned base = list[0] & 0xffffff;
            int shaded = (cmd & 0x10) ? 1 : 0;
            int k = 1;
            int x0 = n2_s11((unsigned)(list[k] & 0xffff));
            int y0 = n2_s11((unsigned)((list[k] >> 16) & 0xffff));
            unsigned c1 = base;
            int x1, y1;
            k++;
            if (shaded) { c1 = list[k] & 0xffffff; k++; }
            x1 = n2_s11((unsigned)(list[k] & 0xffff));
            y1 = n2_s11((unsigned)((list[k] >> 16) & 0xffff));
            n2_set_blend((cmd & 2) ? n2.page_blend : -1);
            n2_vert(x0, y0, base, 0, 0, 0);
            n2_vert(x1, y1, c1,   0, 0, 0);
            n2_vert(x0, y0 + 1, base, 0, 0, 0);
            n2_vert(x1, y1, c1,   0, 0, 0);
            n2_vert(x1, y1 + 1, c1, 0, 0, 0);
            n2_vert(x0, y0 + 1, base, 0, 0, 0);
            list += 1 + len;
            continue;
        }

        switch (cmd) {
        case 0x02: {                               /* vyplneni obdelniku */
            unsigned base = list[0] & 0xffffff;
            int x = (int)( list[1]        & 0x3f0);
            int y = (int)((list[1] >> 16) & 0x1ff);
            int w = (int)(((list[2] & 0x3ff) + 15) & ~15);
            int h = (int)((list[2] >> 16) & 0x1ff);
            int sx = n2.off_x, sy = n2.off_y;
            n2_set_blend(-1);
            n2.off_x = n2.off_y = 0;               /* vypln posun neresi */
            n2_vert(x,     y,     base, 0, 0, 0);
            n2_vert(x + w, y,     base, 0, 0, 0);
            n2_vert(x,     y + h, base, 0, 0, 0);
            n2_vert(x + w, y,     base, 0, 0, 0);
            n2_vert(x + w, y + h, base, 0, 0, 0);
            n2_vert(x,     y + h, base, 0, 0, 0);
            n2.off_x = sx; n2.off_y = sy;
            break;
        }
        case 0xe1: n2_texpage(list[0] & 0xffffff); break;
        case 0xe2: n2_texwin (list[0] & 0xffffff); break;        case 0xe3: n2_flush();
                   n2.area_x0 = (int)( list[0]        & 0x3ff);
                   n2.area_y0 = (int)((list[0] >> 10) & 0x3ff); break;
        case 0xe4: n2_flush();
                   n2.area_x1 = (int)( list[0]        & 0x3ff) + 1;
                   n2.area_y1 = (int)((list[0] >> 10) & 0x3ff) + 1; break;
        case 0xe5: n2.off_x = n2_s11((unsigned)( list[0]        & 0x7ff));
                   n2.off_y = n2_s11((unsigned)((list[0] >> 11) & 0x7ff)); break;
        default: break;                            /* ostatni resi gpulib */
        }
        list += 1 + len;
    }

    if (last_cmd) *last_cmd = (int)cmd;
    return (int)(list - start);
}
