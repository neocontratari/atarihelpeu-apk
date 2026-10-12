// nap_demo_gl.h
// ====================================================================
//  B304: UVODNI DEMO N&P - pomocne veci pro OpenGL ES 2.0.
//
//  Bez Androidu: stejny kod kresli na telefonu (EGL na SurfaceView) i na
//  PC v testu (EGL bez okna, Mesa) - viz test_overeni/b304.
// ====================================================================
#pragma once
#include <GLES2/gl2.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#ifndef NAPDEMO_LOG
#define NAPDEMO_LOG(...) do { std::fprintf(stderr, __VA_ARGS__); std::fprintf(stderr, "\n"); } while (0)
#endif

namespace napdemo {

// ------------------------------------------------------------------
//  Male vektory (jen co je potreba)
// ------------------------------------------------------------------
struct V2 { float x = 0, y = 0; };
struct V3 {
  float x = 0, y = 0, z = 0;
  V3() {}
  V3(float a, float b, float c) : x(a), y(b), z(c) {}
  V3 operator+(const V3 &o) const { return V3(x + o.x, y + o.y, z + o.z); }
  V3 operator-(const V3 &o) const { return V3(x - o.x, y - o.y, z - o.z); }
  V3 operator*(float s) const { return V3(x * s, y * s, z * s); }
};
inline float dot(const V3 &a, const V3 &b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 cross(const V3 &a, const V3 &b) { return V3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline float delka(const V3 &a) { return std::sqrt(dot(a, a)); }
inline V3 norm(const V3 &a) { float l = delka(a); return l > 1e-9f ? a * (1.f / l) : V3(0, 1, 0); }
inline V3 lerp(const V3 &a, const V3 &b, float t) { return a + (b - a) * t; }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float clampf(float x, float a, float b) { return x < a ? a : (x > b ? b : x); }
inline float smooth(float a, float b, float x) {
  float t = clampf((x - a) / (b - a), 0.f, 1.f);
  return t * t * (3.f - 2.f * t);
}
// zacatek rychle, konec pomalu (najezd kamery "s uctou")
inline float easeOut(float t) { t = clampf(t, 0.f, 1.f); return 1.f - (1.f - t) * (1.f - t) * (1.f - t); }
inline float easeInOut(float t) { t = clampf(t, 0.f, 1.f); return t * t * (3.f - 2.f * t); }
// okno 0 -> 1 -> 0: nabeh od a do b, drzi, dobeh od c do d
inline float okno(float x, float a, float b, float c, float d) { return smooth(a, b, x) * (1.f - smooth(c, d, x)); }

// matice 4x4 po sloupcich (jak chce OpenGL)
struct M4 {
  float m[16];
  static M4 jednotka() { M4 r; for (int i = 0; i < 16; i++) r.m[i] = (i % 5 == 0) ? 1.f : 0.f; return r; }
  M4 operator*(const M4 &b) const {
    M4 r;
    for (int c = 0; c < 4; c++)
      for (int rr = 0; rr < 4; rr++) {
        float s = 0;
        for (int k = 0; k < 4; k++) s += m[k * 4 + rr] * b.m[c * 4 + k];
        r.m[c * 4 + rr] = s;
      }
    return r;
  }
  static M4 perspektiva(float fovY, float pomer, float n, float f) {
    M4 r; for (int i = 0; i < 16; i++) r.m[i] = 0;
    float t = 1.f / std::tan(fovY * 0.5f);
    r.m[0] = t / pomer; r.m[5] = t; r.m[10] = (f + n) / (n - f); r.m[11] = -1.f; r.m[14] = 2.f * f * n / (n - f);
    return r;
  }
  static M4 pohled(const V3 &oko, const V3 &cil, const V3 &nahoru) {
    V3 f = norm(cil - oko), s = norm(cross(f, nahoru)), u = cross(s, f);
    M4 r = jednotka();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -dot(s, oko); r.m[13] = -dot(u, oko); r.m[14] = dot(f, oko);
    return r;
  }
  static M4 posun(float x, float y, float z) { M4 r = jednotka(); r.m[12] = x; r.m[13] = y; r.m[14] = z; return r; }
  static M4 meritko(float x, float y, float z) { M4 r = jednotka(); r.m[0] = x; r.m[5] = y; r.m[10] = z; return r; }
  static M4 rotY(float a) { M4 r = jednotka(); r.m[0] = std::cos(a); r.m[8] = std::sin(a); r.m[2] = -std::sin(a); r.m[10] = std::cos(a); return r; }
  static M4 rotX(float a) { M4 r = jednotka(); r.m[5] = std::cos(a); r.m[9] = -std::sin(a); r.m[6] = std::sin(a); r.m[10] = std::cos(a); return r; }
  static M4 rotZ(float a) { M4 r = jednotka(); r.m[0] = std::cos(a); r.m[4] = -std::sin(a); r.m[1] = std::sin(a); r.m[5] = std::cos(a); return r; }
  // bod -> obrazovka (NDC); w <= 0 = za kamerou
  bool promitni(const V3 &p, float &x, float &y, float &z) const {
    float X = m[0] * p.x + m[4] * p.y + m[8] * p.z + m[12];
    float Y = m[1] * p.x + m[5] * p.y + m[9] * p.z + m[13];
    float Z = m[2] * p.x + m[6] * p.y + m[10] * p.z + m[14];
    float W = m[3] * p.x + m[7] * p.y + m[11] * p.z + m[15];
    if (W <= 1e-5f) return false;
    x = X / W; y = Y / W; z = Z / W;
    return true;
  }
};

// ------------------------------------------------------------------
//  Shadery
// ------------------------------------------------------------------
// Fragment shadery zacinaji timhle - highp, kdyz ho GPU umi (sum a mraky
// v mediump na nekterych Mali "zrnily").
static const char *FS_HLAVICKA =
    "#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n";

inline GLuint prelozShader(GLenum typ, const std::string &zdroj, const char *jmeno) {
  GLuint s = glCreateShader(typ);
  const char *z = zdroj.c_str();
  glShaderSource(s, 1, &z, nullptr);
  glCompileShader(s);
  GLint ok = 0; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char buf[2048]; buf[0] = 0; glGetShaderInfoLog(s, sizeof buf, nullptr, buf);
    NAPDEMO_LOG("DEMO shader %s (%s) se neprelozil: %s", jmeno, typ == GL_VERTEX_SHADER ? "VS" : "FS", buf);
  }
  return s;
}

struct Prog {
  GLuint id = 0;
  std::string jmeno;
  struct U { std::string n; GLint l; };
  std::vector<U> uni;
  bool ok = false;
  void sestav(const char *vs, const char *fs, const char *jm) {
    jmeno = jm;
    GLuint v = prelozShader(GL_VERTEX_SHADER, vs, jm);
    GLuint f = prelozShader(GL_FRAGMENT_SHADER, std::string(FS_HLAVICKA) + fs, jm);
    id = glCreateProgram();
    glAttachShader(id, v); glAttachShader(id, f);
    glBindAttribLocation(id, 0, "aPos");
    glBindAttribLocation(id, 1, "aN");
    glBindAttribLocation(id, 2, "aUV");
    glBindAttribLocation(id, 3, "aM");
    glLinkProgram(id);
    GLint l = 0; glGetProgramiv(id, GL_LINK_STATUS, &l);
    ok = l != 0;
    if (!ok) {
      char buf[2048]; buf[0] = 0; glGetProgramInfoLog(id, sizeof buf, nullptr, buf);
      NAPDEMO_LOG("DEMO program %s se nesestavil: %s", jm, buf);
    }
    glDeleteShader(v); glDeleteShader(f);
  }
  GLint u(const char *n) {
    for (auto &x : uni) if (x.n == n) return x.l;
    GLint l = glGetUniformLocation(id, n);
    uni.push_back({n, l});
    return l;
  }
  void pouzij() { glUseProgram(id); }
  void f1(const char *n, float a) { glUniform1f(u(n), a); }
  void f2(const char *n, float a, float b) { glUniform2f(u(n), a, b); }
  void f3(const char *n, float a, float b, float c) { glUniform3f(u(n), a, b, c); }
  void f3(const char *n, const V3 &v) { glUniform3f(u(n), v.x, v.y, v.z); }
  void f4(const char *n, float a, float b, float c, float d) { glUniform4f(u(n), a, b, c, d); }
  void i1(const char *n, int a) { glUniform1i(u(n), a); }
  void m4(const char *n, const M4 &m) { glUniformMatrix4fv(u(n), 1, GL_FALSE, m.m); }
  void tex(const char *n, int jednotka, GLuint t) {
    glActiveTexture(GL_TEXTURE0 + jednotka);
    glBindTexture(GL_TEXTURE_2D, t);
    glUniform1i(u(n), jednotka);
  }
};

// ------------------------------------------------------------------
//  Textura a vlastni framebuffer
// ------------------------------------------------------------------
inline GLuint novaTextura(int w, int h, GLenum fmt, const void *data, bool linear, bool opakovat = false) {
  GLuint t; glGenTextures(1, &t);
  glBindTexture(GL_TEXTURE_2D, t);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, fmt, w, h, 0, fmt, GL_UNSIGNED_BYTE, data);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, linear ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, linear ? GL_LINEAR : GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, opakovat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, opakovat ? GL_REPEAT : GL_CLAMP_TO_EDGE);
  return t;
}

struct Fbo {
  GLuint fbo = 0, tex = 0, hloubka = 0;
  int w = 0, h = 0;
  bool sHloubkou = false;
  // vraci true, kdyz se (pre)stavel
  bool priprav(int nw, int nh, bool hl = false) {
    if (nw < 8) nw = 8;
    if (nh < 8) nh = 8;
    if (fbo && w == nw && h == nh && sHloubkou == hl) return false;
    uvolni();
    w = nw; h = nh; sHloubkou = hl;
    tex = novaTextura(w, h, GL_RGBA, nullptr, true);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    if (hl) {
      glGenRenderbuffers(1, &hloubka);
      glBindRenderbuffer(GL_RENDERBUFFER, hloubka);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hloubka);
    }
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) NAPDEMO_LOG("DEMO FBO %dx%d neni hotovy: 0x%x", w, h, st);
    return true;
  }
  void uvolni() {
    if (tex) glDeleteTextures(1, &tex);
    if (hloubka) glDeleteRenderbuffers(1, &hloubka);
    if (fbo) glDeleteFramebuffers(1, &fbo);
    tex = fbo = hloubka = 0; w = h = 0;
  }
};

// Celoobrazovkovy obdelnik (aPos -1..1)
struct Ctverec {
  GLuint vbo = 0;
  void priprav() {
    const float v[] = {-1, -1, 1, -1, -1, 1, 1, 1};
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof v, v, GL_STATIC_DRAW);
  }
  void kresli() {
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
  }
};

// Vertex: pozice, normala, uv, material (4 floaty: id + 3 parametry)
struct Vrchol { float p[3]; float n[3]; float uv[2]; float m[4]; };

struct Sit {
  GLuint vbo = 0;
  int pocet = 0;
  void nahraj(const std::vector<Vrchol> &v) {
    if (!vbo) glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(v.size() * sizeof(Vrchol)), v.data(), GL_STATIC_DRAW);
    pocet = (int)v.size();
  }
  void kresli(GLenum rezim = GL_TRIANGLES) const {
    if (!vbo || !pocet) return;
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    const GLsizei s = sizeof(Vrchol);
    for (int i = 0; i < 4; i++) glEnableVertexAttribArray(i);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, s, (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, s, (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, s, (void *)(6 * sizeof(float)));
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, s, (void *)(8 * sizeof(float)));
    glDrawArrays(rezim, 0, pocet);
    for (int i = 0; i < 4; i++) glDisableVertexAttribArray(i);
  }
};

// deterministicky "nahodny" generator (stejne demo pokazde)
struct Los {
  uint32_t s;
  explicit Los(uint32_t seed = 12345) : s(seed ? seed : 1) {}
  uint32_t dalsi() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return s; }
  float f() { return (dalsi() >> 8) * (1.f / 16777216.f); }          // 0..1
  float r(float a, float b) { return a + (b - a) * f(); }
};

}  // namespace napdemo
