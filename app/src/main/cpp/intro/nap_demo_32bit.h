// nap_demo_32bit.h
// ====================================================================
//  B307: 32BITOVA UKAZKA PRO UVODNI DEMO (konzole N&P 32)
//
//  Vlastni nocni zavod ulicemi mesta ve stylu 32bitovych 3D konzoli:
//  320 x 240 bodu, malo polygonu, textury 64 x 64 bez vyhlazovani a se
//  vsemi "chybami" te doby, ktere k tomu patri:
//   - afinni mapovani textur (textura se na velkych plochach "krouti"),
//   - vrcholy prichycene k mrizce obrazovky (polygony jemne "plavou"),
//   - osvetleni spoctene do vrcholu (Gouraud), mlha do tmy,
//   - barvy 15 bit (5 bitu na slozku) s rozkladem 4 x 4.
//  Textury, auta, budovy, lampy i neony se postavi v C++ pri startu -
//  zadna cizi grafika. Obraz je cista funkce casu.
// ====================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>
#include "nap_demo_gl.h"

namespace napdemo {

static const char *VS_32 = R"(
attribute vec3 aPos;
attribute vec3 aN;      // barva vrcholu (osvetleni spoctene predem)
attribute vec2 aUV;
attribute vec4 aM;      // x dlazdice atlasu (8 x 4), y svitici (bez osvetleni)
uniform mat4 uMVP;
uniform mat4 uMV;
uniform vec3 uTon;      // nasobek osvetleni (auta pod lampami)
uniform vec2 uMriz;     // prichyceni vrcholu (pul rozliseni)
varying vec3 vUVq;
varying vec4 vBarva;
varying vec2 vTile;
void main(){
  vec4 c = uMVP*vec4(aPos,1.0);
  if (c.w > 0.05) {
    vec2 nd = c.xy/c.w;
    nd = floor(nd*uMriz + 0.5)/uMriz;
    c.xy = nd*c.w;
  }
  gl_Position = c;
  vUVq = vec3(aUV*c.w, c.w);
  vTile = vec2(mod(aM.x, 8.0)*0.125, floor(aM.x/8.0)*0.25);
  float hl = -(uMV*vec4(aPos,1.0)).z;
  float mlha = clamp((hl - 25.0)/150.0, 0.0, 1.0);
  vec3 sv = aN*uTon;
  if (aM.y > 0.5) { sv = vec3(0.5); mlha *= 0.35; }
  vBarva = vec4(sv, mlha);
}
)";
static const char *FS_32 = R"(
varying vec3 vUVq;
varying vec4 vBarva;
varying vec2 vTile;
uniform sampler2D uAtlas;
uniform sampler2D uDith;
uniform vec3 uMlha;
uniform float uMlhaSila;     // 1 = nepruhledne, 0 = pricitane zare (mlha cerna)
void main(){
  vec2 uv = vUVq.xy/vUVq.z;                       // afinne (bez perspektivy)
  vec2 t = vTile + (fract(uv)*63.0 + 0.5)/vec2(512.0, 256.0);
  vec4 tx = texture2D(uAtlas, t);
  if (tx.a < 0.5) discard;
  vec3 c = tx.rgb*vBarva.rgb*2.0;                 // modulace: 0.5 = beze zmeny
  c = mix(c, uMlha*uMlhaSila, vBarva.a);
  float d = texture2D(uDith, gl_FragCoord.xy/4.0).r*255.0 - 4.0;
  c = floor(clamp(c*255.0 + d, 0.0, 255.0)/8.0)/31.0;
  gl_FragColor = vec4(c, 1.0);
}
)";
// obloha: prechod, hvezdy, mesic (v nizkem rozliseni, s rozkladem)
static const char *FS_32NEBE = R"(
varying vec2 vUV;
uniform sampler2D uDith;
uniform float uPosun;
float h(vec2 p){ return fract(sin(dot(p, vec2(12.9898,78.233)))*43758.5453); }
void main(){
  vec2 px = floor(vUV*vec2(320.0,240.0));
  float y = vUV.y;
  vec3 c = mix(vec3(0.22,0.10,0.28), vec3(0.015,0.02,0.07), smoothstep(0.42, 1.0, y));
  c = mix(vec3(0.38,0.16,0.32), c, smoothstep(0.30, 0.50, y));
  vec2 m = vec2(fract(vUV.x*0.55 + 0.30 - uPosun), y) - vec2(0.5, 0.82);
  m.x *= 1.333*1.82;
  float r = length(m);
  c = mix(c, vec3(0.95,0.93,0.82), smoothstep(0.070, 0.064, r));
  c += vec3(0.30,0.32,0.45)*exp(-r*14.0)*0.6;
  float st = step(0.996, h(px + vec2(floor(uPosun*700.0), 0.0)))*smoothstep(0.55, 0.8, y);
  c += vec3(0.7)*st;
  float d = texture2D(uDith, gl_FragCoord.xy/4.0).r*255.0 - 4.0;
  c = floor(clamp(c*255.0 + d, 0.0, 255.0)/8.0)/31.0;
  gl_FragColor = vec4(c, 1.0);
}
)";
// HUD (prekryv spocitany v C++, premultiplied alfa)
static const char *FS_32HUD = R"(
varying vec2 vUV;
uniform sampler2D uTex;
void main(){ gl_FragColor = texture2D(uTex, vec2(vUV.x, 1.0 - vUV.y)); }
)";

class Hra32 {
 public:
  static constexpr int W = 320, H = 240;
  Fbo fbo;
  bool ok = false;

  bool init(const unsigned char *pismo8x8 /* 96 znaku x 8 radku */) {
    std::memcpy(pismo, pismo8x8, sizeof pismo);
    p3d.sestav(VS_32, FS_32, "32bit");
    pNebe.sestav(VS_PLNY32(), FS_32NEBE, "32bit-nebe");
    pHud.sestav(VS_PLNY32(), FS_32HUD, "32bit-hud");
    ctverec.priprav();
    postavAtlas();
    {   // matice rozkladu (4x4, hodnoty 0..7 = -4..+3)
      static const unsigned char M[16] = {0, 4, 1, 5, 6, 2, 7, 3, 1, 5, 0, 4, 7, 3, 6, 2};
      texDith = novaTextura(4, 4, GL_LUMINANCE, M, false, true);
    }
    postavMesto();
    postavAuto(autoHrac, 0.86f, 0.10f, 0.12f, true);
    postavAuto(autoSoupr, 0.12f, 0.36f, 0.86f, false);
    hudPx.assign((size_t)W * H, 0);
    texHud = novaTextura(W, H, GL_RGBA, hudPx.data(), false);
    ok = p3d.ok && pNebe.ok && pHud.ok;
    return ok;
  }

  // t = sekundy od zacatku ukazky
  void snimek(float t) {
    if (!ok) return;
    if (fbo.priprav(W, H, true)) {
      glBindTexture(GL_TEXTURE_2D, fbo.tex);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, fbo.fbo);
    glViewport(0, 0, W, H);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    // pohyb: hrac 61 m/s, souper 50 m/s, predjeti kolem 0,6 s
    const float zH = -t * 61.f - 20.f;
    const float zS = -t * 50.f - 28.f;
    const float pruhH = lerpf(2.1f, -2.1f, smooth(0.10f, 0.55f, t));
    const float xH = stredX(zH) + pruhH, xS = stredX(zS) + 2.1f;
    const float smer = std::atan(dStred(zH));
    pNebe.pouzij();
    pNebe.tex("uDith", 1, texDith);
    pNebe.f1("uPosun", smer * 0.25f + t * 0.004f);
    ctverec.kresli();
    // kamera za autem, mirne zpozdena v zatacce
    const float zK = zH + 6.2f;
    const V3 oko(stredX(zK) + pruhH * 0.85f + 0.3f * std::sin(t * 2.1f), 2.15f + 0.05f * std::sin(t * 9.f), zK);
    const V3 cil(xH, 0.95f, zH - 7.f);
    M4 proj = M4::perspektiva(1.02f, (float)W / H, 0.3f, 400.f);
    M4 view = M4::pohled(oko, cil, V3(0, 1, 0));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    p3d.pouzij();
    p3d.tex("uAtlas", 0, texAtlas);
    p3d.tex("uDith", 1, texDith);
    p3d.f3("uMlha", V3(0.13f, 0.08f, 0.21f));
    p3d.f1("uMlhaSila", 1.f);
    p3d.f2("uMriz", 160.f, 120.f);
    p3d.m4("uMVP", proj * view);
    p3d.m4("uMV", view);
    p3d.f3("uTon", V3(1, 1, 1));
    mesto.kresli();
    // auta: osvetleni podle blizkych lamp (jedou pod nimi)
    auto kresliAuto = [&](Sit &s, float x, float z, float naklon) {
      const float lampa = svetloLamp(z);
      M4 m = M4::posun(x, 0.f, z) * M4::rotY(std::atan(dStred(z))) * M4::rotZ(naklon);
      p3d.m4("uMVP", proj * view * m);
      p3d.m4("uMV", view * m);
      p3d.f3("uTon", V3(0.62f + 0.85f * lampa, 0.60f + 0.70f * lampa, 0.70f + 0.45f * lampa));
      s.kresli();
    };
    kresliAuto(autoSoupr, xS, zS, 0.f);
    const float zmenaPruhu = (smooth(0.10f, 0.32f, t) - smooth(0.32f, 0.55f, t));
    kresliAuto(autoHrac, xH, zH, zmenaPruhu * 0.06f);
    // zare: kaluze svetla pod lampami (pricitaji se, nezapisuji hloubku)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    glDepthMask(GL_FALSE);
    p3d.m4("uMVP", proj * view);
    p3d.m4("uMV", view);
    p3d.f3("uTon", V3(1, 1, 1));
    p3d.f1("uMlhaSila", 0.f);
    zare.kresli();
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    // HUD
    kresliHud(t, zH, zS);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    pHud.pouzij();
    pHud.tex("uTex", 0, texHud);
    ctverec.kresli();
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
  }

 private:
  static const char *VS_PLNY32() {
    return "attribute vec2 aPos;\nvarying vec2 vUV;\nvoid main(){ vUV = aPos*0.5+0.5; gl_Position = vec4(aPos,0.0,1.0); }\n";
  }
  Prog p3d, pNebe, pHud;
  Ctverec ctverec;
  GLuint texAtlas = 0, texDith = 0, texHud = 0;
  Sit mesto, zare, autoHrac, autoSoupr;
  std::vector<uint32_t> hudPx;
  uint8_t pismo[96][8];
  std::vector<float> lampyZ;      // z lamp (obe strany)
  std::vector<V3> lampyPoz;

  // ---------------- silnice ----------------
  static float stredX(float z) { return 7.5f * std::sin(z * 0.011f) + 3.f * std::sin(z * 0.027f + 1.f); }
  static float dStred(float z) { return 7.5f * 0.011f * std::cos(z * 0.011f) + 3.f * 0.027f * std::cos(z * 0.027f + 1.f); }
  static constexpr float PUL = 8.4f;      // pul sirky silnice (4 pruhy)
  static constexpr float CHOD = 3.4f;     // chodnik

  float svetloLamp(float z) const {
    float s = 0.f;
    for (float lz : lampyZ) { const float d = std::fabs(lz - z); if (d < 9.f) s = std::max(s, 1.f - d / 9.f); }
    return s;
  }

  // ---------------- atlas 512 x 256: 32 dlazdic 64 x 64 (8 x 4) ----------------
  enum {
    T_ASF = 0, T_ASF_L = 1, T_ASF_P = 2, T_CHOD = 3, T_FAS_A = 4, T_FAS_B = 5, T_FAS_C = 6, T_TMA = 7,
    T_NEON_NP = 8, T_NEON_32 = 9, T_NEON_ARC = 10, T_LAK = 11, T_SKLO = 12, T_ZADNI = 13, T_SPZ = 14, T_CERNA = 15,
    T_LAMPA = 16, T_ZARE = 17, T_NEON_SVIS = 18, T_NEON_HOT = 19, T_FAS_D = 20
  };
  std::vector<uint32_t> atl;
  static uint32_t rgba(float r, float g, float b, float a = 1.f) {
    auto c = [](float x) { int v = (int)(x * 255.f + 0.5f); return (uint32_t)(v < 0 ? 0 : (v > 255 ? 255 : v)); };
    return c(r) | (c(g) << 8) | (c(b) << 16) | (c(a) << 24);
  }
  // vsechny dlazdice se v shaderu nasobi 2x (0.5 = plna barva)
  static uint32_t pul(float r, float g, float b) { return rgba(r * 0.5f, g * 0.5f, b * 0.5f); }
  static float hsh(int x, int y, int s) {
    uint32_t h = (uint32_t)x * 374761393u + (uint32_t)y * 668265263u + (uint32_t)s * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u; h ^= h >> 16;
    return (float)(h & 0xFFFF) / 65535.f;
  }
  void bodA(int tile, int x, int y, uint32_t c) {
    if (x < 0 || y < 0 || x >= 64 || y >= 64) return;
    const int ox = (tile % 8) * 64, oy = (tile / 8) * 64;
    atl[(size_t)(oy + y) * 512 + ox + x] = c;
  }
  void textA(int tile, int x, int y, const char *s, int mer, uint32_t c) {
    for (; *s; s++, x += 8 * mer) {
      const int ch = (unsigned char)*s - 0x20;
      if (ch < 0 || ch >= 96) continue;
      for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < 8; xx++)
          if (pismo[ch][yy] & (0x80 >> xx))
            for (int a = 0; a < mer; a++)
              for (int b = 0; b < mer; b++) bodA(tile, x + xx * mer + b, y + yy * mer + a, c);
    }
  }
  // neon: svitici text se zari; svisle = pismena pod sebou
  void neonA(int tile, const char *s, int mer, float r, float g, float b, bool svisle) {
    std::vector<float> m(64 * 64, 0.f);
    const int n = (int)std::strlen(s);
    for (int i = 0; i < n; i++) {
      const int ch = (unsigned char)s[i] - 0x20;
      if (ch < 0 || ch >= 96) continue;
      const int x0 = svisle ? (64 - 8 * mer) / 2 : (64 - n * 8 * mer) / 2 + i * 8 * mer;
      const int y0 = svisle ? (64 - n * 8 * mer) / 2 + i * 8 * mer : (64 - 8 * mer) / 2;
      for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < 8; xx++)
          if (pismo[ch][yy] & (0x80 >> xx))
            for (int a = 0; a < mer; a++)
              for (int bb = 0; bb < mer; bb++) {
                const int X = x0 + xx * mer + bb, Y = y0 + yy * mer + a;
                if (X >= 0 && Y >= 0 && X < 64 && Y < 64) m[(size_t)Y * 64 + X] = 1.f;
              }
    }
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) {
        float z = 0.f;
        for (int dy = -4; dy <= 4; dy++)
          for (int dx = -4; dx <= 4; dx++) {
            const int X = x + dx, Y = y + dy;
            if (X >= 0 && Y >= 0 && X < 64 && Y < 64) z += m[(size_t)Y * 64 + X] * std::exp(-(dx * dx + dy * dy) * 0.18f);
          }
        z = std::min(1.f, z * 0.16f);
        const float j = m[(size_t)y * 64 + x];
        const bool ram = (x < 2 || y < 2 || x > 61 || y > 61);       // ramecek ceduly
        float rr = 0.04f + r * z * 0.9f + j * (0.55f + 0.45f * r);
        float gg = 0.03f + g * z * 0.9f + j * (0.55f + 0.45f * g);
        float bl = 0.07f + b * z * 0.9f + j * (0.55f + 0.45f * b);
        if (ram) { rr = r * 0.7f; gg = g * 0.7f; bl = b * 0.7f; }
        bodA(tile, x, y, pul(std::min(1.f, rr), std::min(1.f, gg), std::min(1.f, bl)));
      }
  }
  void fasada(int tile, float r0, float g0, float b0, int typ) {
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) {
        float r = r0, g = g0, b = b0;
        r *= 0.92f + 0.16f * hsh(x / 2, y / 2, tile);
        const int ox = x % 16, oy = y % 16;
        bool okno;
        if (typ == 0) okno = ox > 4 && ox < 12 && oy > 4 && oy < 12;            // ctvercova okna
        else if (typ == 1) okno = ox > 1 && ox < 15 && oy > 3 && oy < 13;       // sklo
        else if (typ == 2) okno = ox > 5 && ox < 11 && oy > 3 && oy < 13;       // uzka (cihla)
        else okno = oy > 5 && oy < 11;                                          // pasova okna
        if (okno) {
          const float s = hsh(typ == 3 ? 0 : x / 16, y / 16, 10 + tile);
          if (s < 0.38f) { r = 1.0f; g = 0.80f; b = 0.46f; }
          else if (s < 0.50f) { r = 0.55f; g = 0.80f; b = 1.0f; }
          else { r = 0.05f; g = 0.06f; b = 0.10f; }
          if (typ == 3 && ((x / 4) % 5 == 0)) { r *= 0.5f; g *= 0.5f; b *= 0.5f; }
        }
        bodA(tile, x, y, pul(r, g, b));
      }
  }
  void postavAtlas() {
    atl.assign(512 * 256, pul(0.25f, 0.25f, 0.25f));
    for (int t = 0; t < 3; t++)          // asfalt (3 varianty car)
      for (int y = 0; y < 64; y++)
        for (int x = 0; x < 64; x++) {
          float v = 0.25f + 0.07f * hsh(x, y, 1) + 0.03f * hsh(x / 4, y / 4, 2);
          float r = v, g = v, b = v * 1.06f;
          const bool levaPrerus = (t == 0 || t == 2) && x < 3 && (y % 32) < 18;
          const bool levaPlna = t == 1 && x < 3;
          const bool pravaPlna = t == 2 && x > 60;
          if (levaPrerus || levaPlna || pravaPlna) { r = 0.85f; g = 0.82f; b = 0.70f; }
          bodA(t, x, y, pul(r, g, b));
        }
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) {
        float v = 0.36f + 0.06f * hsh(x, y, 3);
        if ((x % 16) == 0 || (y % 16) == 0) v = 0.22f;
        bodA(T_CHOD, x, y, pul(v, v * 0.96f, v));
      }
    fasada(T_FAS_A, 0.30f, 0.27f, 0.31f, 0);
    fasada(T_FAS_B, 0.12f, 0.18f, 0.32f, 1);
    fasada(T_FAS_C, 0.38f, 0.19f, 0.14f, 2);
    fasada(T_FAS_D, 0.22f, 0.24f, 0.26f, 3);
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) { float v = 0.10f + 0.04f * hsh(x, y, 7); bodA(T_TMA, x, y, pul(v, v, v * 1.1f)); }
    neonA(T_NEON_NP, "N&P", 2, 1.0f, 0.25f, 0.75f, false);
    neonA(T_NEON_32, "32", 3, 0.2f, 0.95f, 1.0f, false);
    neonA(T_NEON_ARC, "ARCADE", 1, 1.0f, 0.85f, 0.2f, false);
    neonA(T_NEON_SVIS, "N&P", 2, 0.3f, 1.0f, 0.5f, true);
    neonA(T_NEON_HOT, "HOTEL", 1, 1.0f, 0.3f, 0.25f, false);
    for (int y = 0; y < 64; y++)         // lak auta: bila se sparami (barvu dava vrchol)
      for (int x = 0; x < 64; x++) {
        float v = 0.92f + 0.08f * (1.f - y / 64.f);
        if (y == 30 || x == 40) v = 0.62f;
        bodA(T_LAK, x, y, pul(v, v, v));
      }
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) {
        float r = 0.06f, g = 0.10f, b = 0.20f;
        if (std::fabs((x - y * 0.7f) - 18.f) < 5.f) { r += 0.35f; g += 0.4f; b += 0.45f; }
        bodA(T_SKLO, x, y, pul(r, g, b));
      }
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) {
        float r = 1.0f, g = 0.12f, b = 0.14f;
        if ((y % 16) < 3) { r = 0.5f; g = 0.03f; }
        if (x > 52) { r = 1.f; g = 0.92f; b = 0.85f; }
        bodA(T_ZADNI, x, y, pul(r, g, b));
      }
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) {
        const float v = (x < 2 || y < 2 || x > 61 || y > 61) ? 0.1f : 0.92f;
        bodA(T_SPZ, x, y, pul(v, v, v * 0.96f));
      }
    textA(T_SPZ, 8, 18, "N&P", 2, pul(0.04f, 0.04f, 0.08f));
    textA(T_SPZ, 16, 38, "32", 2, pul(0.04f, 0.04f, 0.08f));
    for (int y = 0; y < 64; y++)
      for (int x = 0; x < 64; x++) { float v = 0.05f + 0.03f * hsh(x, y, 9); bodA(T_CERNA, x, y, pul(v, v, v)); }
    for (int y = 0; y < 64; y++)         // hlava lampy: bila az zluta, svitici
      for (int x = 0; x < 64; x++) bodA(T_LAMPA, x, y, pul(1.0f, 0.93f - 0.1f * (y / 64.f), 0.70f));
    for (int y = 0; y < 64; y++)         // zare (kaluz svetla): kruhovy prechod do cerna
      for (int x = 0; x < 64; x++) {
        const float dx = (x - 31.5f) / 31.5f, dy = (y - 31.5f) / 31.5f, d = std::sqrt(dx * dx + dy * dy);
        const float k = std::max(0.f, 1.f - d);
        const float g = k * k * (0.55f + 0.45f * k);
        bodA(T_ZARE, x, y, pul(0.95f * g, 0.66f * g, 0.32f * g));
      }
    texAtlas = novaTextura(512, 256, GL_RGBA, atl.data(), false, false);
    atl.clear(); atl.shrink_to_fit();
  }

  // ---------------- stavba geometrie ----------------
  struct Stav {
    std::vector<Vrchol> v;
    void vrchol(const V3 &p, const V3 &c, float u, float vv, float tile, float emis) {
      Vrchol x;
      x.p[0] = p.x; x.p[1] = p.y; x.p[2] = p.z;
      x.n[0] = c.x; x.n[1] = c.y; x.n[2] = c.z;
      x.uv[0] = u; x.uv[1] = vv;
      x.m[0] = tile; x.m[1] = emis; x.m[2] = 0; x.m[3] = 0;
      v.push_back(x);
    }
  };
  // osvetleni vrcholu: mesic + okoli + lampy (do 33 m)
  V3 svetlo(const V3 &p, const V3 &n) const {
    V3 c(0.20f, 0.17f, 0.30f);
    const V3 mesic = norm(V3(-0.3f, 0.8f, -0.5f));
    c = c + V3(0.10f, 0.12f, 0.22f) * std::max(0.f, dot(n, mesic));
    for (size_t i = 0; i < lampyPoz.size(); i++) {
      const V3 L = lampyPoz[i] - p;
      const float d2 = dot(L, L);
      if (d2 > 1100.f) continue;
      const float d = std::sqrt(d2);
      const float k = std::max(0.f, dot(n, L * (1.f / d))) * (1.f / (1.f + d2 * 0.010f));
      c = c + V3(1.45f, 1.02f, 0.58f) * k;
    }
    return V3(std::min(1.3f, c.x) * 0.5f, std::min(1.3f, c.y) * 0.5f, std::min(1.3f, c.z) * 0.5f);
  }

  void quad(Stav &s, V3 a, V3 b, V3 c, V3 d, float tile, float u0, float v0, float u1, float v1, bool emis = false) {
    const V3 n = norm(cross(b - a, d - a));
    auto sv = [&](const V3 &p) { return emis ? V3(0.5f, 0.5f, 0.5f) : svetlo(p, n); };
    const float e = emis ? 1.f : 0.f;
    s.vrchol(a, sv(a), u0, v0, tile, e); s.vrchol(b, sv(b), u1, v0, tile, e); s.vrchol(c, sv(c), u1, v1, tile, e);
    s.vrchol(a, sv(a), u0, v0, tile, e); s.vrchol(c, sv(c), u1, v1, tile, e); s.vrchol(d, sv(d), u0, v1, tile, e);
  }
  // kvadr (a < b ve vsech osach) - steny s dlazdici, opakovani u/v podle rozmeru
  void kvadr(Stav &s, V3 a, V3 b, float tileBok, float tileVrch, float opakU, float opakV, bool emis = false) {
    const V3 p000(a.x, a.y, a.z), p100(b.x, a.y, a.z), p110(b.x, b.y, a.z), p010(a.x, b.y, a.z);
    const V3 p001(a.x, a.y, b.z), p101(b.x, a.y, b.z), p111(b.x, b.y, b.z), p011(a.x, b.y, b.z);
    const float sx = (b.x - a.x) / opakU, sz = (b.z - a.z) / opakU, sy = (b.y - a.y) / opakV;
    quad(s, p001, p101, p111, p011, tileBok, 0, sy, sx, 0, emis);      // +z
    quad(s, p100, p000, p010, p110, tileBok, 0, sy, sx, 0, emis);      // -z
    quad(s, p000, p001, p011, p010, tileBok, 0, sy, sz, 0, emis);      // -x
    quad(s, p101, p100, p110, p111, tileBok, 0, sy, sz, 0, emis);      // +x
    quad(s, p011, p111, p110, p010, tileVrch, 0, 0, 1, 1, emis);       // vrch
  }

  void postavMesto() {
    Stav s, z;
    lampyZ.clear(); lampyPoz.clear();
    for (float zz = -6.f; zz > -470.f; zz -= 18.f) {
      lampyZ.push_back(zz);
      lampyPoz.push_back(V3(stredX(zz) - PUL + 1.2f, 7.2f, zz));
      lampyPoz.push_back(V3(stredX(zz - 9.f) + PUL - 1.2f, 7.2f, zz - 9.f));
    }
    // silnice: 4 pruhy, useky po 2 m (svetlo lamp se rozlije po vrcholech)
    const float L = 2.f;
    for (float zz = 20.f; zz > -480.f; zz -= L) {
      const float z1 = zz - L;
      const float x0 = stredX(zz), x1 = stredX(z1);
      for (int p = 0; p < 4; p++) {
        const float a = -PUL + p * PUL * 0.5f, b = a + PUL * 0.5f;
        const float tile = p == 0 ? T_ASF_L : (p == 3 ? T_ASF_P : T_ASF);
        quad(s, V3(x0 + a, 0, zz), V3(x0 + b, 0, zz), V3(x1 + b, 0, z1), V3(x1 + a, 0, z1), tile, 0, -zz / 8.f, 1, -z1 / 8.f);
      }
      for (int st = -1; st <= 1; st += 2) {
        const float in = st * PUL, out = st * (PUL + CHOD);
        const float y = 0.15f;
        if (st < 0) {
          quad(s, V3(x0 + out, y, zz), V3(x0 + in, y, zz), V3(x1 + in, y, z1), V3(x1 + out, y, z1), T_CHOD, 0, -zz / 3.4f, 1, -z1 / 3.4f);
          quad(s, V3(x0 + in, y, zz), V3(x0 + in, 0, zz), V3(x1 + in, 0, z1), V3(x1 + in, y, z1), T_TMA, 0, 0, 1, 1);
        } else {
          quad(s, V3(x0 + in, y, zz), V3(x0 + out, y, zz), V3(x1 + out, y, z1), V3(x1 + in, y, z1), T_CHOD, 0, -zz / 3.4f, 1, -z1 / 3.4f);
          quad(s, V3(x0 + in, 0, zz), V3(x0 + in, y, zz), V3(x1 + in, y, z1), V3(x1 + in, 0, z1), T_TMA, 0, 0, 1, 1);
        }
      }
    }
    // lampy: sloup, rameno, svitici hlava + kaluz svetla na silnici (zvlast, pricita se)
    for (size_t i = 0; i < lampyPoz.size(); i++) {
      const V3 h = lampyPoz[i];
      const float st = (i % 2 == 0) ? -1.f : 1.f;
      const float xs = h.x - st * 1.6f;
      kvadr(s, V3(xs - 0.12f, 0.f, h.z - 0.12f), V3(xs + 0.12f, 7.4f, h.z + 0.12f), T_CERNA, T_CERNA, 1.f, 4.f);
      kvadr(s, V3(std::min(xs, h.x) - 0.05f, 7.3f, h.z - 0.08f), V3(std::max(xs, h.x) + 0.05f, 7.45f, h.z + 0.08f), T_CERNA, T_CERNA, 1.f, 1.f);
      kvadr(s, V3(h.x - 0.5f, 7.0f, h.z - 0.28f), V3(h.x + 0.5f, 7.22f, h.z + 0.28f), T_LAMPA, T_LAMPA, 1.f, 1.f, true);
      const float cx = h.x + st * 1.4f, rx = 6.5f, rz = 8.5f;
      quad(z, V3(cx - rx, 0.03f, h.z + rz), V3(cx + rx, 0.03f, h.z + rz), V3(cx + rx, 0.03f, h.z - rz), V3(cx - rx, 0.03f, h.z - rz), T_ZARE, 0, 0, 1, 1, true);
    }
    // budovy po obou stranach, neony u ulice
    uint32_t sd = 4242;
    auto r = [&]() { sd ^= sd << 13; sd ^= sd >> 17; sd ^= sd << 5; return (float)((sd >> 8) & 0xFFFF) / 65535.f; };
    static const float FAS[4] = {T_FAS_A, T_FAS_B, T_FAS_C, T_FAS_D};
    static const float NEONY[4] = {T_NEON_NP, T_NEON_32, T_NEON_ARC, T_NEON_HOT};
    int ni = 0;
    for (int st = -1; st <= 1; st += 2) {
      float zz = 14.f;
      while (zz > -470.f) {
        const float d = 9.f + r() * 14.f;
        const float zc = zz - d * 0.5f;
        const float x0 = stredX(zc) + st * (PUL + CHOD + 0.4f);
        const float hl = 10.f + r() * 12.f;
        const float vys = 9.f + r() * 36.f;
        const float tile = FAS[(int)(r() * 3.99f)];
        const V3 a(st < 0 ? x0 - hl : x0, 0.f, zz - d), b(st < 0 ? x0 : x0 + hl, vys, zz);
        kvadr(s, a, b, tile, T_TMA, 7.f, 6.f);
        const float q = r();
        const float xf = st < 0 ? x0 + 0.06f : x0 - 0.06f;
        if (q < 0.62f) {           // vodorovna cedule nad chodnikem
          const float nt = NEONY[(ni++) % 4];
          const float zz0 = zc - 3.4f, zz1 = zc + 3.4f;
          const float y0 = 3.4f + r() * 2.5f, y1 = y0 + 3.4f;
          if (st < 0) quad(s, V3(xf, y0, zz1), V3(xf, y0, zz0), V3(xf, y1, zz0), V3(xf, y1, zz1), nt, 0, 1, 1, 0, true);
          else quad(s, V3(xf, y0, zz0), V3(xf, y0, zz1), V3(xf, y1, zz1), V3(xf, y1, zz0), nt, 0, 1, 1, 0, true);
        } else if (vys > 18.f) {   // svisla cedule na rohu (kolmo k ulici)
          const float zr = zz - 0.4f, xo = xf - st * 2.6f;
          const float y0 = 6.f, y1 = 16.f;
          quad(s, V3(std::min(xf, xo), y0, zr), V3(std::max(xf, xo), y0, zr), V3(std::max(xf, xo), y1, zr), V3(std::min(xf, xo), y1, zr),
               T_NEON_SVIS, 0, 1, 1, 0, true);
        }
        zz -= d + 0.6f + r() * 2.f;
      }
    }
    mesto.nahraj(s.v);
    zare.nahraj(z.v);
    NAPDEMO_LOG("DEMO 32bit mesto: %d trojuhelniku", (int)((s.v.size() + z.v.size()) / 3));
  }

  // Auto (vlastni tvar sportovniho kupe): profil z boku vytazeny do sirky,
  // kabina, kola, spoiler, zadni svetla a SPZ. Predek k -z.
  void postavAuto(Sit &sit, float r, float g, float b, bool hrac) {
    Stav s;
    const V3 lak(r * 0.62f, g * 0.62f, b * 0.62f), lakTm(r * 0.38f, g * 0.38f, b * 0.38f),
        lakSv(std::min(1.f, r * 0.9f + 0.1f), std::min(1.f, g * 0.9f + 0.1f), std::min(1.f, b * 0.9f + 0.1f));
    auto q = [&](V3 a, V3 bb, V3 c, V3 d, float tile, const V3 &col, bool emis = false) {
      const float e = emis ? 1.f : 0.f;
      const V3 cc = emis ? V3(0.5f, 0.5f, 0.5f) : col;
      s.vrchol(a, cc, 0, 0, tile, e); s.vrchol(bb, cc, 1, 0, tile, e); s.vrchol(c, cc, 1, 1, tile, e);
      s.vrchol(a, cc, 0, 0, tile, e); s.vrchol(c, cc, 1, 1, tile, e); s.vrchol(d, cc, 0, 1, tile, e);
    };
    struct P { float z, y; };
    const P pr[] = {{2.2f, 0.32f}, {2.25f, 0.86f}, {1.75f, 0.96f}, {-1.75f, 0.84f}, {-2.25f, 0.62f}, {-2.25f, 0.32f}};
    const int N = 6;
    const float X = 0.96f;
    for (int st = -1; st <= 1; st += 2) {
      const float x = st * X;
      for (int i = 0; i + 1 < N; i++) {
        const V3 a(x, 0.32f, pr[i].z), bb(x, 0.32f, pr[i + 1].z), c(x, pr[i + 1].y, pr[i + 1].z), d(x, pr[i].y, pr[i].z);
        if (st > 0) q(a, bb, c, d, T_LAK, lakTm); else q(bb, a, d, c, T_LAK, lakTm);
      }
    }
    for (int i = 1; i + 1 < N; i++) {
      const V3 a(-X, pr[i].y, pr[i].z), bb(X, pr[i].y, pr[i].z), c(X, pr[i + 1].y, pr[i + 1].z), d(-X, pr[i + 1].y, pr[i + 1].z);
      q(a, bb, c, d, T_LAK, i == 2 ? lakSv : lak);
    }
    q(V3(-X, 0.32f, 2.2f), V3(X, 0.32f, 2.2f), V3(X, 0.86f, 2.25f), V3(-X, 0.86f, 2.25f), T_LAK, lakTm);
    q(V3(-X + 0.05f, 0.62f, 2.27f), V3(-0.30f, 0.62f, 2.27f), V3(-0.30f, 0.80f, 2.29f), V3(-X + 0.05f, 0.80f, 2.29f), T_ZADNI, lak, true);
    q(V3(0.30f, 0.62f, 2.27f), V3(X - 0.05f, 0.62f, 2.27f), V3(X - 0.05f, 0.80f, 2.29f), V3(0.30f, 0.80f, 2.29f), T_ZADNI, lak, true);
    q(V3(-0.26f, 0.38f, 2.24f), V3(0.26f, 0.38f, 2.24f), V3(0.26f, 0.58f, 2.25f), V3(-0.26f, 0.58f, 2.25f), T_SPZ, V3(0.5f, 0.5f, 0.5f));
    // kabina: zuzena, sikma okna
    const float Xk = 0.72f, Xd = 0.88f;
    const float zz = 1.25f, zr = 0.55f, zf = -0.45f, zp = -1.05f;
    const float yb = 0.93f, yt = 1.30f;
    q(V3(-Xd, yb, zz), V3(Xd, yb, zz), V3(Xk, yt, zr), V3(-Xk, yt, zr), T_SKLO, V3(0.5f, 0.5f, 0.5f));
    q(V3(-Xk, yt, zr), V3(Xk, yt, zr), V3(Xk, yt, zf), V3(-Xk, yt, zf), T_LAK, lakSv);
    q(V3(-Xk, yt, zf), V3(Xk, yt, zf), V3(Xd, yb - 0.04f, zp), V3(-Xd, yb - 0.04f, zp), T_SKLO, V3(0.5f, 0.5f, 0.5f));
    for (int st = -1; st <= 1; st += 2) {
      const V3 a(st * Xd, yb, zz), bb(st * Xd, yb - 0.04f, zp), c(st * Xk, yt, zf), d(st * Xk, yt, zr);
      if (st > 0) q(a, bb, c, d, T_SKLO, V3(0.4f, 0.4f, 0.45f)); else q(bb, a, d, c, T_SKLO, V3(0.4f, 0.4f, 0.45f));
    }
    if (hrac) {     // zavodni pruhy: bile, podel cele delky
      const V3 bila(0.46f, 0.46f, 0.46f);
      for (int k = -1; k <= 1; k += 2) {
        const float a0 = k * 0.12f, a1 = k * 0.30f, xl = std::min(a0, a1), xr = std::max(a0, a1);
        for (int i = 1; i + 1 < N; i++)
          q(V3(xl, pr[i].y + 0.008f, pr[i].z), V3(xr, pr[i].y + 0.008f, pr[i].z), V3(xr, pr[i + 1].y + 0.008f, pr[i + 1].z),
            V3(xl, pr[i + 1].y + 0.008f, pr[i + 1].z), T_LAK, bila);
        q(V3(xl, yt + 0.008f, zr), V3(xr, yt + 0.008f, zr), V3(xr, yt + 0.008f, zf), V3(xl, yt + 0.008f, zf), T_LAK, bila);
      }
    }
    {   // spoiler
      const float y0 = 1.02f, y1 = 1.09f, z0 = 1.85f, z1 = 2.3f;
      q(V3(-X, y1, z1), V3(X, y1, z1), V3(X, y1, z0), V3(-X, y1, z0), T_LAK, lakTm);
      q(V3(-X, y0, z1), V3(X, y0, z1), V3(X, y1, z1), V3(-X, y1, z1), T_LAK, lakTm);
      for (int st = -1; st <= 1; st += 2) {
        const float x = st * 0.62f;
        q(V3(x - 0.04f, 0.9f, 1.98f), V3(x + 0.04f, 0.9f, 1.98f), V3(x + 0.04f, y0, 1.98f), V3(x - 0.04f, y0, 1.98f), T_CERNA, V3(0.3f, 0.3f, 0.3f));
      }
    }
    for (int i = 0; i < 4; i++) {   // kola
      const float xs = (i % 2 ? 1.f : -1.f) * (X + 0.02f), zc = (i < 2) ? 1.38f : -1.42f, rr = 0.34f;
      for (int k = 0; k < 8; k++) {
        const float a0 = k * 0.785398f, a1 = a0 + 0.785398f;
        const V3 c0(xs, 0.34f, zc), p0(xs, 0.34f + std::sin(a0) * rr, zc + std::cos(a0) * rr), p1(xs, 0.34f + std::sin(a1) * rr, zc + std::cos(a1) * rr);
        s.vrchol(c0, V3(0.2f, 0.2f, 0.2f), 0.5f, 0.5f, T_CERNA, 0); s.vrchol(p0, V3(0.2f, 0.2f, 0.2f), 0, 0, T_CERNA, 0); s.vrchol(p1, V3(0.2f, 0.2f, 0.2f), 1, 0, T_CERNA, 0);
        const V3 p0i(xs - (i % 2 ? 0.24f : -0.24f), p0.y, p0.z), p1i(xs - (i % 2 ? 0.24f : -0.24f), p1.y, p1.z);
        q(p0, p1, p1i, p0i, T_CERNA, V3(0.16f, 0.16f, 0.16f));
      }
    }
    q(V3(-1.15f, 0.02f, 2.5f), V3(1.15f, 0.02f, 2.5f), V3(1.15f, 0.02f, -2.4f), V3(-1.15f, 0.02f, -2.4f), T_CERNA, V3(0.05f, 0.05f, 0.06f));
    sit.nahraj(s.v);
  }

  // ---------------- HUD (v C++ do pruhledne vrstvy) ----------------
  void hBod(int x, int y, uint32_t c) {
    if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) hudPx[(size_t)y * W + x] = c;
  }
  void hText(int x, int y, const char *s, int mer, uint32_t c, bool stin = true) {
    for (; *s; s++, x += 8 * mer) {
      const int ch = (unsigned char)*s - 0x20;
      if (ch < 0 || ch >= 96) continue;
      for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < 8; xx++)
          if (pismo[ch][yy] & (0x80 >> xx))
            for (int a = 0; a < mer; a++)
              for (int b = 0; b < mer; b++) {
                const int sk = (7 - yy) / 3;     // kurziva (styl zavodu te doby)
                if (stin) hBod(x + xx * mer + b + sk + 1, y + yy * mer + a + 1, 0xFF000000u);
                hBod(x + xx * mer + b + sk, y + yy * mer + a, c);
              }
    }
  }
  void kresliHud(float t, float zH, float zS) {
    std::fill(hudPx.begin(), hudPx.end(), 0u);
    const uint32_t bila = 0xFFFFFFFFu, zluta = 0xFF30E0FFu, ruda = 0xFF3040FFu, azur = 0xFFFFE060u;
    const bool prvni = zH < zS;
    hText(10, 10, prvni ? "1ST" : "2ND", 3, prvni ? zluta : bila);
    hText(84, 18, "/8", 1, bila);
    hText(232, 10, "LAP 2/3", 1, bila);
    char b[32];
    const int ms = (int)(t * 1000.f) + 41230;
    std::snprintf(b, sizeof b, "%d'%02d\"%02d", ms / 60000, (ms / 1000) % 60, (ms / 10) % 100);
    hText(232, 22, b, 1, azur);
    const int kmh = 212 + (int)(t * 18.f) + (int)(3.f * std::sin(t * 13.f));
    std::snprintf(b, sizeof b, "%3d", kmh);
    hText(228, 196, b, 3, bila);
    hText(282, 220, "KM/H", 1, azur);
    const float ot = 0.72f + 0.2f * std::fmod(t * 0.9f, 1.f);
    for (int i = 0; i < 20; i++) {
      const float u = i / 19.f;
      if (u > ot) break;
      const uint32_t c = u < 0.6f ? azur : (u < 0.85f ? zluta : ruda);
      const int x = 214 + i * 5, h = 3 + i / 3;
      for (int y = 0; y < h; y++) for (int xx = 0; xx < 3; xx++) hBod(x + xx, 190 - y, c);
    }
    hText(194, 204, "5", 2, zluta);
    glBindTexture(GL_TEXTURE_2D, texHud);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, hudPx.data());
  }
};

}  // namespace napdemo
