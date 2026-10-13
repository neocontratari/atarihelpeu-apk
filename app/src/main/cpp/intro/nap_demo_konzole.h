// nap_demo_konzole.h
// ====================================================================
//  B307: KONZOLE N&P 16 A N&P 32 A STARY CRT MONITOR (3D modely)
//
//  Vlastni navrh (zadna kopie cizich vyrobku, zadna cizi loga):
//   - N&P 16: 16bitova konzole na cartridge - nizka klinova skrin v barve
//     pulnocni modre, kartacovany kovovy vrch, zlaty prouzek, vlozena
//     cartridge s pestrym stitkem, posuvny vypinac, dve zdirky ovladacu.
//   - N&P 32: 32bitova konzole na CD - matne cerna skrin se stupnem,
//     leskly vrch, azurovy svetelny pas, disk napul vysunuty ze stěrbiny
//     vepredu, kulate tlacitko se svitícím prstencem.
//   - Na obou zlaty emblem N&P (oficialni logo z appky).
//   - CRT monitor: tmava skrin s hlubokou zadni casti, zaobleny ramecek,
//     vypoukla obrazovka (zvlast sit se souradnicemi pro obraz).
//  Jednotky cm, X doprava, Y nahoru, predek k +Z (ke kamere).
//
//  Material (aM.x) - shader FS_KONZOLE, barva v aM.yzw:
//   0 lak   1 leskly   2 kartacovany kov   3 svetlo (LED, pas)   4 emblem N&P
//   5 stitek cartridge   6 disk CD   7 tmava sterbina / mrizka   8 plast CRT
// ====================================================================
#pragma once
#include <cmath>
#include <utility>
#include <vector>
#include "nap_demo_gl.h"

namespace napdemo {

static const char *FS_KONZOLE = R"(
varying vec3 vP;
varying vec3 vN;
varying vec2 vUV;
varying vec4 vM;
uniform vec3 uKamera;
uniform vec3 uLampaPos;
uniform vec3 uLampaSmer;
uniform vec3 uLampa;
uniform vec3 uOknoPos;
uniform vec3 uOkno;
uniform float uBlesk;
uniform float uAmb;
uniform float uExpo;
uniform float uCas;
uniform vec3 uNeon1Pos;
uniform vec3 uNeon1;
uniform vec3 uNeon2Pos;
uniform vec3 uNeon2;
uniform float uOdhal;        // svetova vyska: nad ni konzole jeste neni (zhmotneni z neonu)
uniform vec3 uOdhalBarva;
uniform float uSvit;         // LED a svetelne pasy
uniform float uLogoLesk;     // odlesk pres emblem (poloha pasu)
uniform sampler2D uLogo;
)" "%SUM%" R"(
vec3 duha(float h){ vec3 k = mod(h*6.0 + vec3(5.0,3.0,1.0), 6.0); return clamp(min(k, 4.0-k), 0.0, 1.0); }
void main(){
  float dOdhal = uOdhal - vP.y;
  if (dOdhal < 0.0) discard;
  int m = int(vM.x + 0.5);
  vec3 N = normalize(vN);
  vec3 V = normalize(uKamera - vP);
  vec3 alb = vM.yzw;
  float spec = 0.35, shin = 40.0, odraz = 0.12;
  vec3 emis = vec3(0.0);
  if (m == 0) {
    // lak s jemnou metalizou (trpyt)
    float tr = step(0.93, sum2(vP.xz*9.0 + vP.y*3.0));
    spec = 0.45 + tr*0.8; shin = 50.0; odraz = 0.18;
  } else if (m == 1) {
    spec = 1.1; shin = 110.0; odraz = 0.45;
  } else if (m == 2) {
    // kartacovany kov: ryhy podel x
    float ry = sum2(vec2(vP.x*0.4, vP.z*38.0));
    alb *= 0.82 + 0.3*ry;
    spec = 0.45; shin = 36.0; odraz = 0.18;
  } else if (m == 3) {
    emis = alb*uSvit*2.2;
    alb *= 0.15;
    spec = 0.5; shin = 60.0;
  } else if (m == 4) {
    vec4 l = texture2D(uLogo, vUV);
    float zl = clamp((l.r - l.b)*2.6, 0.0, 1.0);
    alb = mix(alb, l.rgb*1.1, l.a);
    spec = mix(spec, 1.2, l.a); shin = mix(shin, 60.0, l.a); odraz = mix(odraz, 0.5, l.a);
    float x = vUV.x + (1.0 - vUV.y)*0.6;
    emis = vec3(1.0,0.86,0.55)*exp(-(x - uLogoLesk)*(x - uLogoLesk)*40.0)*zl*1.4 + l.rgb*zl*0.10*uSvit;
  } else if (m == 5) {
    // stitek cartridge: vecerni nebe, slunce, hory a pruhy (vlastni obrazek)
    vec2 u = vUV;
    vec3 c = mix(vec3(0.98,0.45,0.20), vec3(0.30,0.08,0.45), u.y);
    float sl = length((u - vec2(0.62, 0.42))*vec2(1.0, 1.4));
    c = mix(c, vec3(1.0,0.85,0.30), smoothstep(0.22, 0.20, sl));
    float hory = 0.30 + 0.08*sin(u.x*17.0) + 0.05*sin(u.x*41.0);
    if (u.y < hory) c = vec3(0.12,0.05,0.25);
    if (u.y > 0.80) c = vec3(0.95,0.93,0.88);
    if (u.y > 0.83 && u.y < 0.95 && u.x > 0.08 && u.x < 0.92) c = vec3(0.10,0.10,0.18)*(0.6 + 0.4*step(0.5, fract(u.x*16.0)));
    alb = c; spec = 0.5; shin = 50.0; odraz = 0.2;
  } else if (m == 6) {
    // disk CD: duhove lomy podle uhlu a pohledu, stredova dira
    vec2 q = vUV - 0.5;
    float r = length(q)*2.0;
    if (r < 0.14) discard;
    float a = atan(q.y, q.x);
    vec3 R = reflect(-V, N);
    float h = fract(a*0.159 + dot(R, vec3(0.3,0.5,0.2))*1.3 + r*0.6);
    alb = mix(vec3(0.75,0.77,0.80), duha(h), 0.55*smoothstep(0.35, 0.5, r));
    if (r < 0.32) alb = vec3(0.55,0.57,0.62);
    spec = 1.3; shin = 80.0; odraz = 0.6;
    emis = duha(h)*0.06*uSvit;
  } else if (m == 7) {
    spec = 0.05; shin = 8.0; odraz = 0.0;
  } else if (m == 8) {
    alb *= 0.95 + 0.08*sum2(vP.xy*4.0);
    spec = 0.18; shin = 18.0; odraz = 0.05;
  }
  vec3 c = alb*uAmb;
  // lampa (reflektor zleva shora - siroky kuzel, at dosvitne na celou trojici)
  vec3 L1 = uLampaPos - vP; float d1 = length(L1); L1 /= d1;
  float kuzel = smoothstep(0.70, 0.95, dot(-L1, uLampaSmer));
  float a1 = (0.10 + 0.90*kuzel)/(1.0 + d1*d1*0.00008);
  c += alb*uLampa*max(dot(N,L1),0.0)*a1;
  c += uLampa*spec*pow(max(dot(N, normalize(L1+V)),0.0), shin)*a1;
  // okno
  vec3 L2 = uOknoPos - vP; float d2 = length(L2); L2 /= d2;
  vec3 ok = uOkno*(1.0 + uBlesk*7.0);
  c += alb*ok*max(dot(N,L2),0.0);
  c += ok*spec*pow(max(dot(N, normalize(L2+V)),0.0), shin*1.4);
  float rim = pow(1.0 - max(dot(N,V),0.0), 3.0);
  c += ok*rim*0.25;
  // protisvetlo zezadu shora (obrys proti tme)
  vec3 L5 = normalize(vec3(0.15, 0.75, -0.65));
  c += vec3(0.30,0.36,0.50)*(max(dot(N,L5),0.0)*0.35 + rim*max(dot(N,L5),0.0)*1.6)*(m == 8 ? 0.4 : 1.0);
  // neonova svetla (pruty)
  vec3 L3 = uNeon1Pos - vP; float d3 = length(L3); L3 /= d3;
  vec3 L4 = uNeon2Pos - vP; float d4 = length(L4); L4 /= d4;
  vec3 n1 = uNeon1/(1.0 + d3*d3*0.0025), n2 = uNeon2/(1.0 + d4*d4*0.0025);
  c += (alb + vec3(0.08))*(n1*max(dot(N,L3),0.0) + n2*max(dot(N,L4),0.0));
  c += (n1*pow(max(dot(N, normalize(L3+V)),0.0), shin) + n2*pow(max(dot(N, normalize(L4+V)),0.0), shin))*spec;
  if (odraz > 0.0) {
    vec3 R = reflect(-V, N);
    float ko = max(dot(R, L2), 0.0);
    c += ok*odraz*(pow(ko, 40.0)*2.2 + pow(ko, 6.0)*0.25);
    c += uLampa*odraz*pow(max(dot(R, L1), 0.0), 200.0)*a1*3.0;
    c += (n1*pow(max(dot(R, L3), 0.0), 30.0) + n2*pow(max(dot(R, L4), 0.0), 30.0))*odraz*2.0;
  }
  c += emis;
  // zhmotneni: svitici pas na hranici
  float pas = 1.0 - smoothstep(0.0, 1.4, dOdhal);
  c += uOdhalBarva*pas*pas*2.5;
  c = vec3(1.0) - exp(-c*uExpo);
  gl_FragColor = vec4(c, 1.0);
}
)";

// Obrazovka CRT: obraz (textura) s vypouklym sklem, radky, maskou, sumem
// pri prepinani a odlesky. Vypnuta = tmave sklo s odlesky.
static const char *VS_CRT = R"(
attribute vec3 aPos;
attribute vec3 aN;
attribute vec2 aUV;
uniform mat4 uMVP;
varying vec3 vP;
varying vec3 vN;
varying vec2 vUV;
void main(){ vP = aPos; vN = aN; vUV = aUV; gl_Position = uMVP*vec4(aPos,1.0); }
)";
static const char *FS_CRT = R"(
varying vec3 vP;
varying vec3 vN;
varying vec2 vUV;
uniform sampler2D uObr;
uniform sampler2D uSum;
uniform vec4 uMapa;       // obraz: uv = uMapa.xy + q*uMapa.zw (q 0..1, y nahoru)
uniform vec2 uZdroj;      // rozliseni zdroje (sirka, radky)
uniform float uZap;       // 0 vypnuto .. 1 zapnuto (rozsviceni pres caru)
uniform float uSnih;      // sum pri prepinani
uniform float uRoll;      // svisly posun (prepinani)
uniform float uJas;
uniform vec3 uKamera;
uniform vec3 uLampaPos;
uniform vec3 uLampa;
uniform float uCas;
void main(){
  vec2 q = vUV;
  // zakriveni obrazu (hrany se ztraci za rameckem)
  vec2 c2 = q*2.0 - 1.0;
  c2 *= 1.0 + vec2(0.035, 0.045)*dot(c2, c2);
  q = c2*0.5 + 0.5;
  vec3 c = vec3(0.0);
  float uvnitr = step(0.0, q.x)*step(q.x, 1.0)*step(0.0, q.y)*step(q.y, 1.0);
  if (uZap > 0.001 && uvnitr > 0.5) {
    // zapnuti: nejdriv vodorovna cara, pak se roztahne
    float roz = smoothstep(0.0, 0.45, uZap);
    float vy = (q.y - 0.5)/max(roz, 0.004) + 0.5;
    float qy = fract(vy + uRoll);
    vec2 t = uMapa.xy + vec2(q.x, qy)*uMapa.zw;
    vec3 o = texture2D(uObr, t).rgb;
    // radky: mezi radky tmavsi (podle rozliseni zdroje)
    float r = fract(qy*uZdroj.y);
    float radek = 0.62 + 0.38*smoothstep(0.0, 0.35, r)*smoothstep(1.0, 0.65, r);
    // maska (svisle RGB pruhy)
    float mx = fract(gl_FragCoord.x/3.0);
    vec3 maska = vec3(mx < 0.34 ? 1.12 : 0.92, (mx >= 0.34 && mx < 0.67) ? 1.12 : 0.92, mx >= 0.67 ? 1.12 : 0.92);
    o = o*radek*maska;
    float sn = texture2D(uSum, q*vec2(3.1, 2.3) + vec2(fract(uCas*13.7), fract(uCas*7.3))).r;
    o = mix(o, vec3(sn), uSnih);
    o *= step(abs(vy - 0.5), 0.5);
    float cara = (1.0 - smoothstep(0.0, 0.5, uZap))*exp(-abs(q.y - 0.5)*120.0)*3.0;
    o += vec3(0.9,0.95,1.0)*cara;
    // vinetace trubice
    vec2 vg = q - 0.5;
    o *= 1.0 - dot(vg*vec2(1.2, 1.4), vg*vec2(1.2, 1.4))*0.9;
    c = o*uJas*smoothstep(0.0, 0.08, uZap);
  }
  // sklo: tmave zelenosede, odlesk lampy a okna
  vec3 N = normalize(vN);
  vec3 V = normalize(uKamera - vP);
  vec3 L = normalize(uLampaPos - vP);
  c += vec3(0.020,0.024,0.024)*(1.0 - uZap*0.6);
  c += uLampa*pow(max(dot(N, normalize(L+V)), 0.0), 400.0)*0.20;
  float okr = smoothstep(0.42, 0.5, max(abs(vUV.x - 0.5), abs(vUV.y - 0.5)));
  c *= 1.0 - okr*0.75;
  gl_FragColor = vec4(c, 1.0);
}
)";

struct StavK {
  std::vector<Vrchol> v;
  void tri(const V3 &a, const V3 &b, const V3 &c, const V3 &n, V2 ua, V2 ub, V2 uc, float mat, const V3 &barva) {
    const V3 *p[3] = {&a, &b, &c};
    const V2 *u[3] = {&ua, &ub, &uc};
    for (int i = 0; i < 3; i++) {
      Vrchol x;
      x.p[0] = p[i]->x; x.p[1] = p[i]->y; x.p[2] = p[i]->z;
      x.n[0] = n.x; x.n[1] = n.y; x.n[2] = n.z;
      x.uv[0] = u[i]->x; x.uv[1] = u[i]->y;
      x.m[0] = mat; x.m[1] = barva.x; x.m[2] = barva.y; x.m[3] = barva.z;
      v.push_back(x);
    }
  }
  // a,b,c,d proti smeru hodin pri pohledu zvenku (normala ven)
  void quad(const V3 &a, const V3 &b, const V3 &c, const V3 &d, float mat, const V3 &barva, V2 ua = {0, 0}, V2 ub = {1, 0},
            V2 uc = {1, 1}, V2 ud = {0, 1}) {
    const V3 n = norm(cross(b - a, d - a));
    tri(a, b, c, n, ua, ub, uc, mat, barva);
    tri(a, c, d, n, ua, uc, ud, mat, barva);
  }
  // kvadr se vsemi stenami ven (a < b)
  void kvadr(const V3 &a, const V3 &b, float mat, const V3 &barva, bool spodek = false) {
    const V3 p000(a.x, a.y, a.z), p100(b.x, a.y, a.z), p110(b.x, b.y, a.z), p010(a.x, b.y, a.z);
    const V3 p001(a.x, a.y, b.z), p101(b.x, a.y, b.z), p111(b.x, b.y, b.z), p011(a.x, b.y, b.z);
    quad(p001, p101, p111, p011, mat, barva);
    quad(p100, p000, p010, p110, mat, barva);
    quad(p000, p001, p011, p010, mat, barva);
    quad(p101, p100, p110, p111, mat, barva);
    quad(p011, p111, p110, p010, mat, barva);
    if (spodek) quad(p000, p100, p101, p001, mat, barva);
  }
  // hranol z obrysu (proti smeru hodin pri pohledu shora, x-z), vyska y0..y1,
  // horni hrana zkosena o "zk" (dovnitr i dolu)
  void hranol(const std::vector<V2> &o, float y0, float y1, float zk, float matBok, const V3 &bBok, float matVrch, const V3 &bVrch,
              float matHrana, const V3 &bHrana) {
    const size_t n = o.size();
    V2 c{0, 0};
    for (auto &p : o) { c.x += p.x / n; c.y += p.y / n; }
    std::vector<V2> in(n);
    for (size_t i = 0; i < n; i++) {
      // posun vrcholu dovnitr podel osy uhlu
      const V2 &p0 = o[(i + n - 1) % n], &p1 = o[i], &p2 = o[(i + 1) % n];
      V2 e1{p1.x - p0.x, p1.y - p0.y}, e2{p2.x - p1.x, p2.y - p1.y};
      float l1 = std::sqrt(e1.x * e1.x + e1.y * e1.y), l2 = std::sqrt(e2.x * e2.x + e2.y * e2.y);
      V2 n1{-e1.y / l1, e1.x / l1}, n2{-e2.y / l2, e2.x / l2};   // vnejsi normaly v rovine (x, z)
      V2 s{n1.x + n2.x, n1.y + n2.y};
      float ls = std::sqrt(s.x * s.x + s.y * s.y);
      float k = zk / std::max(0.3f, (s.x * n1.x + s.y * n1.y) / std::max(ls, 1e-4f));
      in[i] = {p1.x - s.x / std::max(ls, 1e-4f) * k, p1.y - s.y / std::max(ls, 1e-4f) * k};
    }
    for (size_t i = 0; i < n; i++) {
      const V2 &a = o[i], &b = o[(i + 1) % n];
      const V2 &ai = in[i], &bi = in[(i + 1) % n];
      // bok
      quad(V3(a.x, y0, a.y), V3(b.x, y0, b.y), V3(b.x, y1 - zk, b.y), V3(a.x, y1 - zk, a.y), matBok, bBok);
      // zkosena hrana
      if (zk > 0.f) quad(V3(a.x, y1 - zk, a.y), V3(b.x, y1 - zk, b.y), V3(bi.x, y1, bi.y), V3(ai.x, y1, ai.y), matHrana, bHrana);
    }
    // vrch (vejir ze stredu)
    for (size_t i = 0; i < n; i++) {
      const V2 &a = in[i], &b = in[(i + 1) % n];
      tri(V3(c.x, y1, c.y), V3(b.x, y1, b.y), V3(a.x, y1, a.y), V3(0, 1, 0), {0.5f, 0.5f}, {0, 0}, {0, 0}, matVrch, bVrch);
    }
  }
  // svisly valec (tlacitko, knofliky) - plast + vrch
  void valec(const V3 &st, float r, float h, int seg, float mat, const V3 &barva, float matVrch, const V3 &bVrch, bool osaZ = false) {
    for (int i = 0; i < seg; i++) {
      const float a0 = 6.2831853f * i / seg, a1 = 6.2831853f * (i + 1) / seg;
      if (!osaZ) {
        V3 p0(st.x + std::cos(a0) * r, st.y, st.z + std::sin(a0) * r), p1(st.x + std::cos(a1) * r, st.y, st.z + std::sin(a1) * r);
        V3 q0 = p0, q1 = p1; q0.y += h; q1.y += h;
        quad(p1, p0, q0, q1, mat, barva);
        tri(V3(st.x, st.y + h, st.z), q1, q0, V3(0, 1, 0), {0.5f, 0.5f}, {0.5f + 0.5f * std::cos(a1), 0.5f + 0.5f * std::sin(a1)},
            {0.5f + 0.5f * std::cos(a0), 0.5f + 0.5f * std::sin(a0)}, matVrch, bVrch);
      } else {
        V3 p0(st.x + std::cos(a0) * r, st.y + std::sin(a0) * r, st.z), p1(st.x + std::cos(a1) * r, st.y + std::sin(a1) * r, st.z);
        V3 q0 = p0, q1 = p1; q0.z += h; q1.z += h;
        quad(p0, p1, q1, q0, mat, barva);
        tri(V3(st.x, st.y, st.z + h), q0, q1, V3(0, 0, 1), {0.5f, 0.5f}, {0.5f + 0.5f * std::cos(a0), 0.5f - 0.5f * std::sin(a0)},
            {0.5f + 0.5f * std::cos(a1), 0.5f - 0.5f * std::sin(a1)}, matVrch, bVrch);
      }
    }
  }
  // plochy kotouc (emblem, disk) v rovine s normalou n
  void kotouc(const V3 &st, float r, const V3 &osaU, const V3 &osaV, int seg, float mat, const V3 &barva) {
    const V3 n = norm(cross(osaU, osaV));
    for (int i = 0; i < seg; i++) {
      const float a0 = 6.2831853f * i / seg, a1 = 6.2831853f * (i + 1) / seg;
      V3 p0 = st + osaU * (std::cos(a0) * r) + osaV * (std::sin(a0) * r);
      V3 p1 = st + osaU * (std::cos(a1) * r) + osaV * (std::sin(a1) * r);
      tri(st, p0, p1, n, {0.5f, 0.5f}, {0.5f + 0.5f * std::cos(a0), 0.5f - 0.5f * std::sin(a0)},
          {0.5f + 0.5f * std::cos(a1), 0.5f - 0.5f * std::sin(a1)}, mat, barva);
    }
  }
};

// obdelnik se zkosenymi rohy jako obrys (x-z), proti smeru hodin shora
inline std::vector<V2> obrysObd(float x0, float z0, float x1, float z1, float roh) {
  // shora (osa y k divakovi): x doprava, z dolu (k divakovi). Proti smeru hodin.
  return {{x0 + roh, z1}, {x1 - roh, z1}, {x1, z1 - roh}, {x1, z0 + roh}, {x1 - roh, z0}, {x0 + roh, z0}, {x0, z0 + roh}, {x0, z1 - roh}};
}

struct ModelKonzole {
  Sit sit;
  std::vector<std::pair<V3, V3>> hrany;   // obrys pro neonove pruty (v souradnicich modelu)
  V3 led{0, 0, 0}, emblem{0, 0, 0};
  float vyska = 8.f;

  // N&P 16: klinova skrin na cartridge
  void postav16() {
    StavK s;
    const V3 modra(0.035f, 0.055f, 0.16f), kov(0.30f, 0.32f, 0.36f), zlato(0.80f, 0.58f, 0.22f), tma(0.02f, 0.02f, 0.03f);
    const float X = 13.f, Z = 10.f;
    // spodni cast (modry lak), zkosene rohy
    s.hranol(obrysObd(-X, -Z, X, Z, 1.6f), 0.4f, 4.6f, 0.6f, 0, modra, 0, modra, 0, modra);
    // klinovy vrch: kovova deska stoupajici dozadu (4.6 vpredu -> 6.6 vzadu)
    {
      const float yf = 4.6f, yb = 6.6f, x = X - 1.4f;
      const V3 a(-x, yf, Z - 1.2f), b(x, yf, Z - 1.2f), c(x, yb, -Z + 1.2f), d(-x, yb, -Z + 1.2f);
      s.quad(a, b, c, d, 2, kov);
      // boky klinu
      s.quad(V3(-x, 4.6f, -Z + 1.2f), V3(-x, 4.6f, Z - 1.2f), a, d, 0, modra);
      s.quad(V3(x, 4.6f, Z - 1.2f), V3(x, 4.6f, -Z + 1.2f), c, b, 0, modra);
      s.quad(V3(x, 4.6f, -Z + 1.2f), V3(-x, 4.6f, -Z + 1.2f), d, c, 0, modra);
    }
    // zlaty prouzek po predni hrane
    s.quad(V3(-X + 1.2f, 3.55f, Z + 0.01f), V3(X - 1.2f, 3.55f, Z + 0.01f), V3(X - 1.2f, 3.85f, Z + 0.01f), V3(-X + 1.2f, 3.85f, Z + 0.01f), 1, zlato);
    // predni tmavy pas se zdirkami a vypinacem
    s.quad(V3(-X + 1.2f, 1.2f, Z + 0.02f), V3(X - 1.2f, 1.2f, Z + 0.02f), V3(X - 1.2f, 3.1f, Z + 0.02f), V3(-X + 1.2f, 3.1f, Z + 0.02f), 7, tma);
    for (int i = 0; i < 2; i++) {      // zdirky ovladacu
      const float x0 = 4.5f + i * 3.9f;
      s.kvadr(V3(x0, 1.5f, Z + 0.02f), V3(x0 + 3.0f, 2.8f, Z + 0.25f), 1, V3(0.06f, 0.06f, 0.08f));
      s.quad(V3(x0 + 0.3f, 1.75f, Z + 0.26f), V3(x0 + 2.7f, 1.75f, Z + 0.26f), V3(x0 + 2.7f, 2.55f, Z + 0.26f), V3(x0 + 0.3f, 2.55f, Z + 0.26f), 7, tma);
    }
    // posuvny vypinac (chrom) + LED
    s.kvadr(V3(-10.4f, 1.6f, Z + 0.02f), V3(-8.2f, 2.7f, Z + 0.45f), 2, kov);
    s.kvadr(V3(-7.2f, 1.9f, Z + 0.02f), V3(-6.6f, 2.4f, Z + 0.20f), 3, V3(1.0f, 0.12f, 0.05f));
    led = V3(-6.9f, 2.15f, Z + 0.25f);
    // sterbina cartridge vzadu nahore + vlozena cartridge se stitkem
    {
      const float yb = 6.6f, zs = -Z + 3.4f;
      const float ys = 4.6f + (yb - 4.6f) * ((Z - 1.2f) - zs) / ((Z - 1.2f) - (-Z + 1.2f));
      s.kvadr(V3(-6.8f, ys - 0.2f, zs - 1.2f), V3(6.8f, ys + 0.12f, zs + 1.2f), 7, tma);
      const float cw = 6.0f, ch = 6.8f;
      s.kvadr(V3(-cw, ys, zs - 0.75f), V3(cw, ys + ch, zs + 0.75f), 8, V3(0.10f, 0.10f, 0.12f));
      // stitek na predni strane cartridge
      s.quad(V3(-cw + 0.6f, ys + 1.0f, zs + 0.76f), V3(cw - 0.6f, ys + 1.0f, zs + 0.76f), V3(cw - 0.6f, ys + ch - 0.6f, zs + 0.76f),
             V3(-cw + 0.6f, ys + ch - 0.6f, zs + 0.76f), 5, V3(1, 1, 1));
      // ryhy na horni hrane cartridge (uchop)
      for (int k = 0; k < 5; k++) s.kvadr(V3(-4.f + k * 2.f, ys + ch, zs - 0.6f), V3(-3.4f + k * 2.f, ys + ch + 0.15f, zs + 0.6f), 8, V3(0.16f, 0.16f, 0.18f));
      vyska = ys + ch + 0.2f;
    }
    // vetraci drazky na bocich
    for (int k = 0; k < 6; k++)
      for (int st = -1; st <= 1; st += 2) {
        const float z0 = -6.f + k * 2.0f;
        const float x = st * (X + 0.01f);
        if (st < 0) s.quad(V3(x, 1.6f, z0), V3(x, 1.6f, z0 + 0.8f), V3(x, 3.4f, z0 + 0.8f), V3(x, 3.4f, z0), 7, tma);
        else s.quad(V3(x, 1.6f, z0 + 0.8f), V3(x, 1.6f, z0), V3(x, 3.4f, z0), V3(x, 3.4f, z0 + 0.8f), 7, tma);
      }
    // zlaty emblem N&P vpredu vlevo na kovove desce (lezi na sikme plose)
    {
      const float zc = Z - 4.0f, yc = 4.6f + 2.0f * ((Z - 1.2f) - zc) / (2.f * (Z - 1.2f)) + 0.03f;
      const V3 osaU(1, 0, 0), osaV = norm(V3(0, 2.0f, -(2.f * (Z - 1.2f))));
      s.kotouc(V3(-7.2f, yc, zc), 2.6f, osaU, osaV, 40, 4, V3(0.06f, 0.07f, 0.12f));
      emblem = V3(-7.2f, yc, zc);
    }
    // nozicky
    for (int i = 0; i < 4; i++) s.kvadr(V3((i % 2 ? 9.f : -11.f), 0.f, (i < 2 ? 6.f : -8.f)), V3((i % 2 ? 11.f : -9.f), 0.4f, (i < 2 ? 8.f : -6.f)), 7, tma);
    sit.nahraj(s.v);
    // obrys pro pruty: spodni a horni obdelnik, svisle hrany, cartridge
    hrany.clear();
    auto obd = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
      V3 p[8] = {V3(x0, y0, z0), V3(x1, y0, z0), V3(x1, y0, z1), V3(x0, y0, z1), V3(x0, y1, z0), V3(x1, y1, z0), V3(x1, y1, z1), V3(x0, y1, z1)};
      static const int E[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
      for (auto &e : E) hrany.push_back({p[e[0]], p[e[1]]});
    };
    obd(-X, 0.4f, -Z, X, 4.6f, Z);
    hrany.push_back({V3(-X + 1.4f, 4.6f, Z - 1.2f), V3(-X + 1.4f, 6.6f, -Z + 1.2f)});
    hrany.push_back({V3(X - 1.4f, 4.6f, Z - 1.2f), V3(X - 1.4f, 6.6f, -Z + 1.2f)});
    obd(-6.f, 6.0f, -Z + 2.6f, 6.f, vyska, -Z + 4.2f);
  }

  // N&P 32: cerna skrin se stupnem a diskem ve sterbine
  void postav32() {
    StavK s;
    const V3 cerna(0.022f, 0.022f, 0.026f), lesk(0.03f, 0.03f, 0.035f), azur(0.25f, 0.85f, 1.0f), tma(0.01f, 0.01f, 0.012f);
    const float X = 13.5f, Z = 10.5f;
    // zadni vyssi cast a predni nizsi (stupen)
    s.hranol(obrysObd(-X, -Z, X, Z, 1.2f), 0.4f, 5.2f, 0.7f, 0, cerna, 1, lesk, 0, cerna);
    s.hranol(obrysObd(-X + 0.6f, -Z + 0.6f, X - 0.6f, 1.5f, 1.0f), 5.2f, 7.4f, 0.6f, 0, cerna, 1, lesk, 1, lesk);
    // azurovy svetelny pas po hrane stupne a po predni hrane
    s.quad(V3(-X + 1.2f, 5.25f, 1.6f + 0.01f), V3(X - 1.2f, 5.25f, 1.6f + 0.01f), V3(X - 1.2f, 5.55f, 1.6f + 0.01f), V3(-X + 1.2f, 5.55f, 1.6f + 0.01f), 3, azur);
    s.quad(V3(-X + 1.0f, 0.9f, Z + 0.01f), V3(X - 1.0f, 0.9f, Z + 0.01f), V3(X - 1.0f, 1.12f, Z + 0.01f), V3(-X + 1.0f, 1.12f, Z + 0.01f), 3, azur);
    // sterbina na disk vpredu (tmava) a disk napul venku
    s.quad(V3(-7.f, 2.9f, Z + 0.02f), V3(7.f, 2.9f, Z + 0.02f), V3(7.f, 3.45f, Z + 0.02f), V3(-7.f, 3.45f, Z + 0.02f), 7, tma);
    s.quad(V3(-7.f, 2.95f, Z + 0.03f), V3(7.f, 2.95f, Z + 0.03f), V3(7.f, 3.02f, Z + 0.03f), V3(-7.f, 3.02f, Z + 0.03f), 3, azur * 0.6f);
    // disk (prumer 12 cm) vodorovne, vystrceny ze sterbiny
    s.kotouc(V3(0.f, 3.2f, Z + 1.2f), 6.0f, V3(1, 0, 0), V3(0, 0, -1), 48, 6, V3(1, 1, 1));
    // kulate tlacitko se svitícím prstencem (vpravo)
    s.valec(V3(10.2f, 3.2f, Z), 1.25f, 0.35f, 24, 3, azur, 1, lesk, true);
    s.valec(V3(10.2f, 3.2f, Z + 0.35f), 1.0f, 0.12f, 24, 1, lesk, 1, lesk, true);
    led = V3(10.2f, 3.2f, Z + 0.5f);
    // zdirky ovladacu (vlevo)
    for (int i = 0; i < 2; i++) {
      const float x0 = -12.f + i * 3.3f;
      s.kvadr(V3(x0, 2.3f, Z + 0.02f), V3(x0 + 2.6f, 3.7f, Z + 0.2f), 1, V3(0.05f, 0.05f, 0.06f));
      s.quad(V3(x0 + 0.3f, 2.6f, Z + 0.21f), V3(x0 + 2.3f, 2.6f, Z + 0.21f), V3(x0 + 2.3f, 3.4f, Z + 0.21f), V3(x0 + 0.3f, 3.4f, Z + 0.21f), 7, tma);
    }
    // mrizka na vrchu (vetrani) - tmave pruhy
    for (int k = 0; k < 9; k++) {
      const float x0 = -2.f + k * 1.4f;
      s.quad(V3(x0, 7.41f, -4.5f), V3(x0 + 0.7f, 7.41f, -4.5f), V3(x0 + 0.7f, 7.41f, -8.0f), V3(x0, 7.41f, -8.0f), 7, tma);
    }
    // zlaty emblem N&P vlevo na hornim stupni
    s.kotouc(V3(-8.2f, 7.43f, -3.6f), 2.6f, V3(1, 0, 0), V3(0, 0, -1), 40, 4, V3(0.03f, 0.03f, 0.04f));
    emblem = V3(-8.2f, 7.43f, -3.6f);
    for (int i = 0; i < 4; i++) s.kvadr(V3((i % 2 ? 10.f : -12.f), 0.f, (i < 2 ? 7.f : -9.f)), V3((i % 2 ? 12.f : -10.f), 0.4f, (i < 2 ? 9.f : -7.f)), 7, tma);
    vyska = 7.6f;
    sit.nahraj(s.v);
    hrany.clear();
    auto obd = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
      V3 p[8] = {V3(x0, y0, z0), V3(x1, y0, z0), V3(x1, y0, z1), V3(x0, y0, z1), V3(x0, y1, z0), V3(x1, y1, z0), V3(x1, y1, z1), V3(x0, y1, z1)};
      static const int E[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
      for (auto &e : E) hrany.push_back({p[e[0]], p[e[1]]});
    };
    obd(-X, 0.4f, -Z, X, 5.2f, Z);
    obd(-X + 0.6f, 5.2f, -Z + 0.6f, X - 0.6f, 7.4f, 1.5f);
    // obrys disku (osmiuhelnik)
    for (int k = 0; k < 8; k++) {
      const float a0 = k * 0.785398f, a1 = a0 + 0.785398f;
      hrany.push_back({V3(std::cos(a0) * 6.f, 3.2f, Z + 1.2f + std::sin(a0) * 6.f), V3(std::cos(a1) * 6.f, 3.2f, Z + 1.2f + std::sin(a1) * 6.f)});
    }
  }
};

// CRT monitor (vlastni navrh) na nizkem podstavci; obrazovka zvlast (sklo)
struct ModelCrt {
  Sit sit, sklo;
  V3 stred{0, 0, 0};          // stred obrazovky (v souradnicich modelu)
  float sw = 26.4f, sh = 19.8f;
  V3 led{0, 0, 0};

  void postav() {
    StavK s;
    const V3 plast(0.075f, 0.075f, 0.082f), tmavy(0.03f, 0.03f, 0.034f), tma(0.01f, 0.01f, 0.012f);
    // podstavec
    s.hranol(obrysObd(-11.f, -12.f, 11.f, 10.f, 2.f), 0.f, 3.2f, 0.6f, 8, tmavy, 8, tmavy, 8, tmavy);
    // predni cast skrine (ramecek) 34 x 30, hloubka 9
    const float X = 17.f, y0 = 3.4f, y1 = 33.4f, zf = 12.f, zb = 3.f;
    s.kvadr(V3(-X, y0, zb), V3(X, y1, zf), 8, plast, true);
    // zadni zuzena cast (trubice) - komoli jehlan
    {
      const float xb = 11.f, yb0 = 7.f, yb1 = 29.f, zbb = -18.f;
      const V3 A(-X + 1.f, y0 + 1.f, zb), B(X - 1.f, y0 + 1.f, zb), C(X - 1.f, y1 - 1.f, zb), D(-X + 1.f, y1 - 1.f, zb);
      const V3 a(-xb, yb0, zbb), b(xb, yb0, zbb), c(xb, yb1, zbb), d(-xb, yb1, zbb);
      s.quad(C, D, d, c, 8, plast);          // vrch
      s.quad(A, B, b, a, 8, plast);          // spodek
      s.quad(B, C, c, b, 8, plast);          // pravy bok
      s.quad(D, A, a, d, 8, plast);          // levy bok
      s.quad(b, c, d, a, 8, tmavy);          // zadek
      // vetraci drazky nahore
      for (int k = 0; k < 7; k++) {
        const float z0 = zb - 2.f - k * 2.4f, t0 = (z0 - zb) / (zbb - zb), t1 = (z0 - 1.f - zb) / (zbb - zb);
        const float yA = (y1 - 1.f) + (yb1 - (y1 - 1.f)) * t0 + 0.02f, yB = (y1 - 1.f) + (yb1 - (y1 - 1.f)) * t1 + 0.02f;
        const float xA = (X - 1.f) + (xb - (X - 1.f)) * t0 - 2.f, xB = (X - 1.f) + (xb - (X - 1.f)) * t1 - 2.f;
        s.quad(V3(-xA, yA, z0), V3(xA, yA, z0), V3(xB, yB, z0 - 1.f), V3(-xB, yB, z0 - 1.f), 7, tma);
      }
    }
    // zapustene okoli obrazovky (tmave)
    stred = V3(0.f, y0 + 16.2f, zf + 0.05f);
    const float mw = sw * 0.5f + 1.4f, mh = sh * 0.5f + 1.4f;
    s.quad(V3(-mw, stred.y - mh, zf + 0.02f), V3(mw, stred.y - mh, zf + 0.02f), V3(mw, stred.y + mh, zf + 0.02f), V3(-mw, stred.y + mh, zf + 0.02f), 7, tma);
    // spodni lista s knofliky a zelenou LED
    for (int i = 0; i < 2; i++) s.valec(V3(9.5f + i * 3.2f, y0 + 1.6f, zf), 0.9f, 0.6f, 16, 8, tmavy, 2, V3(0.5f, 0.5f, 0.52f), true);
    s.kvadr(V3(-14.f, y0 + 1.3f, zf), V3(-13.f, y0 + 1.9f, zf + 0.2f), 3, V3(0.2f, 1.0f, 0.3f));
    led = V3(-13.5f, y0 + 1.6f, zf + 0.25f);
    sit.nahraj(s.v);
    // sklo: vypoukla sit 24 x 18 s uv (0..1, v nahoru)
    StavK g;
    const int NX = 24, NY = 18;
    auto bod = [&](int i, int j) {
      const float u = (float)i / NX, v = (float)j / NY;
      const float x = (u - 0.5f) * sw, y = (v - 0.5f) * sh;
      const float bz = 1.3f * (1.f - (4.f * (u - 0.5f) * (u - 0.5f))) * (1.f - (4.f * (v - 0.5f) * (v - 0.5f)));
      return V3(stred.x + x, stred.y + y, stred.z + bz);
    };
    for (int j = 0; j < NY; j++)
      for (int i = 0; i < NX; i++) {
        const V3 a = bod(i, j), b = bod(i + 1, j), c = bod(i + 1, j + 1), d = bod(i, j + 1);
        // normala z vypoukleho tvaru
        const float u = (i + 0.5f) / NX - 0.5f, v = (j + 0.5f) / NY - 0.5f;
        const V3 n = norm(V3(u * 0.35f, v * 0.35f, 1.f));
        const V2 ua{(float)i / NX, (float)j / NY}, ub{(float)(i + 1) / NX, (float)j / NY}, uc{(float)(i + 1) / NX, (float)(j + 1) / NY},
            ud{(float)i / NX, (float)(j + 1) / NY};
        g.tri(a, b, c, n, ua, ub, uc, 0, V3(0, 0, 0));
        g.tri(a, c, d, n, ua, uc, ud, 0, V3(0, 0, 0));
      }
    sklo.nahraj(g.v);
  }
};

}  // namespace napdemo
