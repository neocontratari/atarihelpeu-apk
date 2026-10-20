// nap_menu_shadery.h
// ====================================================================
//  B311: SHADERY HLAVNIHO MENU (OpenGL ES 2.0 / GLSL ES 1.00)
//
//  Barvy jsou "premultiplied" (rgb uz vynasobene alfou), michani
//  glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA).
//  Sum je stejny jako v uvodnim demu (textura 256x256, SUM_GLSL z nap_demo_shadery.h).
// ====================================================================
#pragma once

namespace napmenu {

// ---------- karta v karuselu: ctyruhelnik uz promitnuty na CPU (x*w, y*w, 0, w) ----------
static const char *VS_KARTA = R"(
attribute vec4 aPos;
attribute vec2 aUV;
varying vec2 vUV;
void main(){ vUV = aUV; gl_Position = aPos; }
)";

// Ramecek karty se zkosenymi rohy (jako panely v N&P VISION), zlaty zkoseny okraj,
// uvnitr obrazek karty (FBO), odlesk skla. uOdraz = 1: odraz na podlaze (prevraceny,
// slabnouci). Vse v bodech karty (uVel = sirka, vyska karty v bodech).
static const char *FS_KARTA = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uVel;        // velikost karty na obrazovce (body)
uniform float uJas;
uniform float uAlfa;
uniform vec3 uBarva;      // barva karty
uniform float uFokus;     // 1 = prostredni karta
uniform float uLesk;      // poloha odlesku (-1 .. 2)
uniform float uOdraz;     // 0 karta, > 0 odraz na podlaze: jeho delka (dil vysky karty)
uniform float uZablesk;   // spusteni karty (bily zablesk)
uniform float uCas;
uniform float uAA;        // sirka vyhlazeni hrany (body)
float tvar(vec2 p, vec2 h, float c){
  vec2 q = abs(p);
  float d = max(q.x - h.x, q.y - h.y);
  return max(d, (q.x + q.y - (h.x + h.y - c))*0.70710678);
}
void main(){
  // uv: 0..1 pres kartu, v nahoru (textura FBO karty ma v = 0 dole)
  vec2 uv = vUV;
  float odr = 0.0;
  if (uOdraz > 0.0) { odr = clamp(-vUV.y/uOdraz, 0.0, 1.0); uv.y = -vUV.y; }   // zrcadlo pod spodni hranou
  vec2 p = (uv - 0.5)*uVel;                         // body od stredu
  vec2 h = uVel*0.5;
  float roh = min(uVel.x, uVel.y)*0.075;
  float d = tvar(p, h, roh);
  float aa = uAA;
  float maska = 1.0 - smoothstep(-aa, aa*0.5, d);
  if (maska <= 0.0) discard;
  float ram = min(uVel.x, uVel.y)*0.022;            // sirka zlateho okraje
  vec3 obr = texture2D(uTex, clamp(uv, 0.0, 1.0)).rgb;
  vec3 c = obr;
  // zlaty okraj: "zkoseni" - svetlo zleva shora, odstin podle smeru normaly hrany
  float vOkraji = smoothstep(-ram - aa, -ram + aa, d);
  if (vOkraji > 0.0) {
    vec2 e = vec2(0.6, 0.0);
    float dx = tvar(p + e.xy, h, roh) - tvar(p - e.xy, h, roh);
    float dy = tvar(p + e.yx, h, roh) - tvar(p - e.yx, h, roh);
    vec2 n = normalize(vec2(dx, dy) + 1e-5);
    float t = clamp((d + ram)/ram, 0.0, 1.0);       // 0 uvnitr -> 1 venku
    float sv = dot(n, normalize(vec2(-0.55, -0.85)))*(0.5 - abs(t - 0.5))*2.0;
    vec3 zl1 = vec3(1.00, 0.86, 0.50), zl2 = vec3(0.55, 0.33, 0.08), zl3 = vec3(0.20, 0.11, 0.02);
    vec3 z = mix(zl2, zl1, clamp(0.5 + sv*0.9, 0.0, 1.0));
    z = mix(z, zl3, smoothstep(0.80, 1.0, t) + (1.0 - smoothstep(0.0, 0.18, t))*0.7);
    // jiskra po obvodu prostredni karty
    float ang = atan(p.y, p.x);
    float jiskra = pow(max(0.0, cos(ang - uCas*1.3)), 40.0)*uFokus;
    z += vec3(1.0, 0.9, 0.6)*jiskra*0.9;
    z = mix(z, z*0.7 + uBarva*0.35, 0.25*uFokus);
    c = mix(c, z, vOkraji);
  }
  // sklo: jemny sikmy odlesk, ktery se pohne s kartou
  float s = (uv.x*0.8 - uv.y*0.55) - uLesk;
  c += vec3(1.0, 0.95, 0.85)*exp(-s*s*90.0)*0.09*(1.0 - vOkraji);
  c *= uJas;
  c = mix(c, vec3(1.0, 0.95, 0.85), uZablesk);
  float a = maska*uAlfa;
  if (uOdraz > 0.0) a *= 0.16*(1.0 - odr)*(1.0 - odr);    // od spodni hrany dolu slabne
  gl_FragColor = vec4(c*a, a);
}
)";

// hvezdy (body): aPos = x, y (NDC), velikost (body), faze
static const char *VS_HVEZDY = R"(
attribute vec4 aPos;
uniform float uCas;
uniform float uAlfa;
varying float vJas;
void main(){
  gl_Position = vec4(aPos.xy, 0.0, 1.0);
  gl_PointSize = aPos.z;
  vJas = (0.55 + 0.45*sin(uCas*(0.6 + fract(aPos.w*7.31)*2.2) + aPos.w*6.2831853))*uAlfa;
}
)";
static const char *FS_HVEZDY = R"(
varying float vJas;
uniform vec3 uBarva;
void main(){
  vec2 q = gl_PointCoord*2.0 - 1.0;
  float r = dot(q, q);
  float a = (exp(-r*6.0) + exp(-abs(q.x*q.y)*90.0 - r*2.0)*0.35)*(1.0 - smoothstep(0.6, 1.0, r));
  gl_FragColor = vec4(uBarva*a*vJas, 0.0);
}
)";

// ---------- obdelnik v bodech obrazovky (NDC) a UV ----------
static const char *VS_OBD2 = R"(
attribute vec2 aPos;
uniform vec4 uObd;
uniform vec4 uUV;
varying vec2 vUV;
varying vec2 vQ;
void main(){
  vec2 t = aPos*0.5+0.5;
  gl_Position = vec4(mix(uObd.xy, uObd.zw, t), 0.0, 1.0);
  vUV = mix(uUV.xy, uUV.zw, t);
  vQ = t;
}
)";

// text (premultiplied bila sazba se stinem) obarveny: zlaty prechod / barva, odlesk
static const char *FS_TEXT = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform sampler2D uTex;
uniform vec3 uBarva1;     // nahore
uniform vec3 uBarva2;     // dole
uniform float uAlfa;
uniform float uLesk;      // poloha odlesku (0..1 pres text), < -0.5 = bez
void main(){
  vec4 t = texture2D(uTex, vUV);
  float pokryti = t.r;                       // bily text: rgb = pokryti (premultiplied)
  vec3 b = mix(uBarva2, uBarva1, vQ.y);
  float s = (vQ.x - vQ.y*0.25) - uLesk;
  b += vec3(1.0, 0.95, 0.8)*exp(-s*s*60.0)*0.8*step(-0.5, uLesk);
  gl_FragColor = vec4(b*pokryti, t.a)*uAlfa;
}
)";

// obrazek / textura premultiplied * alfa
static const char *FS_OBR = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform sampler2D uTex;
uniform float uAlfa;
void main(){ gl_FragColor = texture2D(uTex, vUV)*uAlfa; }
)";

// logo N&P (minca): odlesk, ktery jednou za chvili prejede. Obrazek loga ma tmave
// ctvercove pozadi - vyrizne se jen minca (kruh uKruh: stred x, y a polomer x, y v UV).
static const char *FS_LOGO_MENU = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform sampler2D uTex;
uniform float uAlfa;
uniform float uLesk;
uniform vec4 uKruh;
uniform float uAA;        // vyhlazeni okraje (v UV)
void main(){
  vec4 t = texture2D(uTex, vUV);
  vec2 d = (vUV - uKruh.xy)/uKruh.zw;
  float r = length(d);
  float m = 1.0 - smoothstep(1.0 - uAA, 1.0 + uAA*0.5, r);
  vec3 c = t.rgb;
  float zl = clamp((t.r - t.b)*2.2, 0.0, 1.0);
  float s = (vQ.x + vQ.y*0.6) - uLesk;
  c += vec3(1.0, 0.92, 0.7)*exp(-s*s*70.0)*zl*0.9;
  // tenky tmavy lem, at minca nesplyne s pozadim
  c *= mix(1.0, 0.55, smoothstep(0.93, 1.0, r));
  gl_FragColor = vec4(c*m, m)*uAlfa;
}
)";

// ikony a UI v jednom shaderu (SDF): 1 ozubene kolecko v osmiuhelniku, 2 tlacitko (zlata
// tableta se zkosenymi rohy), 3 zamek, 4 tecka (kosoctverec), 5 trojuhelnik "play",
// 6 mekka zare (kruh), 7 aktivni tecka (protahla)
static const char *FS_UI = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform int uTyp;
uniform vec2 uVel;        // velikost v bodech
uniform vec3 uBarva;
uniform float uAlfa;
uniform float uUhel;      // natoceni kolecka
uniform float uSvit;      // zare / puls
float osmi(vec2 p, vec2 h, float c){ vec2 q = abs(p); return max(max(q.x - h.x, q.y - h.y), (q.x + q.y - (h.x + h.y - c))*0.70710678); }
vec3 zlato(float t){ return mix(vec3(0.50,0.30,0.07), vec3(1.0,0.86,0.50), clamp(t,0.0,1.0)); }
void main(){
  vec2 p = (vQ - 0.5)*uVel;            // body od stredu, y nahoru
  float r = min(uVel.x, uVel.y)*0.5;
  vec4 o = vec4(0.0);
  if (uTyp == 1) {
    // ramecek: osmiuhelnik se zlatym okrajem, uvnitr tmave sklo
    float d = osmi(p, vec2(r*0.92), r*0.55);
    float m = 1.0 - smoothstep(-1.0, 0.5, d);
    float okr = smoothstep(-r*0.13 - 1.0, -r*0.13 + 1.0, d);
    vec3 c = mix(vec3(0.05,0.03,0.015), zlato(0.55 + p.y/r*0.45), okr);
    // ozubene kolo
    vec2 q = p;
    float a = atan(q.y, q.x) + uUhel;
    float rr = length(q);
    float zub = smoothstep(0.15, 0.35, abs(fract(a*8.0/6.2831853) - 0.5));
    float R0 = r*0.42, R1 = r*0.56;
    float kolo = rr - mix(R1, R0, zub);
    float km = (1.0 - smoothstep(-1.0, 1.0, kolo))*smoothstep(r*0.17 - 1.0, r*0.17 + 1.0, rr);
    vec3 kc = zlato(0.6 + q.y/r*0.5) + vec3(1.0,0.9,0.6)*uSvit*0.5;
    c = mix(c, kc, km);
    o = vec4(c*m, m);
  } else if (uTyp == 2) {
    // tlacitko: zlata tableta, zkosene rohy, svetlejsi nahore; uSvit = dychani
    vec2 h = uVel*0.5 - 1.5;
    float d = osmi(p, h, h.y*0.9);
    float m = 1.0 - smoothstep(-1.0, 0.5, d);
    float t = vQ.y;
    vec3 c = mix(vec3(0.62,0.40,0.10), vec3(1.0,0.84,0.45), t);
    c = mix(c, uBarva, 0.12);
    c *= 1.0 + 0.10*sin(uSvit*6.2831853);
    float hrana = smoothstep(-3.0, -0.5, d);
    c = mix(c, vec3(1.0,0.93,0.70), hrana*0.6*step(0.0, p.y));
    c = mix(c, vec3(0.30,0.17,0.04), hrana*0.7*step(p.y, 0.0));
    o = vec4(c*m, m);
  } else if (uTyp == 3) {
    // zamek (zlaty): telo + oblouk
    vec2 q = p/r;
    float telo = osmi(q - vec2(0.0, -0.22), vec2(0.62, 0.46), 0.12);
    float obl = abs(length(q - vec2(0.0, 0.20)) - 0.36) - 0.10;
    obl = max(obl, -(q.y - 0.20));
    float d = min(telo, obl)*r;
    float m = 1.0 - smoothstep(-1.0, 0.8, d);
    vec3 c = zlato(0.45 + q.y*0.5);
    float dirka = length(q - vec2(0.0, -0.24)) - 0.11;
    c *= mix(0.25, 1.0, smoothstep(-0.02, 0.02, dirka));
    o = vec4(c*m, m);
  } else if (uTyp == 4) {
    // tecka (kosoctverec)
    vec2 q = abs(p);
    float d = (q.x + q.y) - r*0.95;
    float m = 1.0 - smoothstep(-1.0, 1.0, d);
    o = vec4(uBarva*m, m);
  } else if (uTyp == 5) {
    // trojuhelnik "play" (tmavy na tlacitku)
    vec2 q = p/r;
    float d = max(-q.x*0.5 - 0.43 + abs(q.y)*0.0, max(abs(q.y)*0.866 + q.x*0.5 - 0.43, -q.x - 0.5));
    float m = 1.0 - smoothstep(-0.03, 0.03, d);
    o = vec4(uBarva*m, m);
  } else if (uTyp == 6) {
    // mekka zare (pricita se) - elipsa pres cely obdelnik
    float k = length((vQ - 0.5)*2.0);
    float g = exp(-k*k*3.2)*(1.0 - smoothstep(0.85, 1.0, k));
    o = vec4(uBarva*g, 0.0);
  } else if (uTyp == 7) {
    // aktivni tecka: protahla tableta (spicate konce jako kosoctverec)
    vec2 h = uVel*0.5 - 0.5;
    vec2 q = abs(p);
    float d = max(q.x - (h.x - h.y), 0.0) + q.y - h.y;
    float m = 1.0 - smoothstep(-1.0, 1.0, d);
    vec3 c = mix(uBarva, vec3(1.0,0.95,0.8), 0.35);
    o = vec4(c*m, m);
  } else if (uTyp == 8) {
    // plna barva (zatmeni)
    o = vec4(uBarva, 1.0);
  }
  gl_FragColor = o*uAlfa;
}
)";

// ---------- pozadi menu: vesmir v zlatem tonu, mlhovina, zlate kruhy "portalu" ----------
static const char *FS_POZADI = R"(
varying vec2 vUV;
uniform vec2 uRozmer;     // body (pomer stran)
uniform float uCas;
uniform vec3 uBarva;      // barva prave vybrane karty (prelina se)
uniform vec2 uStred;      // stred kruhu (0..1, y nahoru)
uniform float uPortal;    // velikost kruhu (dil sirky)
)" "%SUM%" R"(
float sq(float x){ return x*x; }
void main(){
  vec2 uv = vUV;
  float pomer = uRozmer.x/uRozmer.y;
  vec2 p = vec2((uv.x - 0.5)*pomer, uv.y - 0.5);
  // zaklad: temne hnedocerna, nahore o kousek svetlejsi
  vec3 c = mix(vec3(0.010,0.006,0.004), vec3(0.030,0.018,0.008), uv.y);
  // mlhovina: pomalu plynouci zlate mraky (fbm), do barvy karty
  vec2 q = p*2.2 + vec2(uCas*0.010, -uCas*0.006);
  float n = fbm2(q*1.3 + vec2(fbm2(q*0.7 + uCas*0.012)*1.6, 0.0));
  float mlha = smoothstep(0.38, 0.92, n);
  vec3 barvaMlhy = mix(vec3(0.95,0.48,0.10), uBarva, 0.45);
  c += barvaMlhy*mlha*0.20*(0.55 + 0.45*uv.y);
  // svitici jadro za kartami
  vec2 s = vec2((uv.x - uStred.x)*pomer, uv.y - uStred.y);
  float r = length(s*vec2(1.0, 1.25));
  c += mix(vec3(1.0,0.55,0.12), uBarva, 0.35)*exp(-r*r*9.0)*0.30;
  // zlate kruhy portalu (elipsy na "podlaze" pod kartami), carkovane, pomalu se toci
  vec2 e = vec2((uv.x - uStred.x)*pomer, (uv.y - (uStred.y - 0.02))*3.2);
  float re = length(e)/uPortal;
  float ang = atan(e.y, e.x);
  float kruhy = 0.0;
  for (int i = 0; i < 4; i++) {
    float fi = float(i);
    float rr = 0.55 + fi*0.18;
    float w = 0.006 + fi*0.0015;
    float carka = step(0.35 + 0.1*fi, fract(ang*(6.0 + fi*4.0)/6.2831853 + uCas*(0.02 - fi*0.012)));
    kruhy += exp(-sq((re - rr)/w))*(0.35 + 0.65*carka)*(1.0 - fi*0.18);
  }
  kruhy *= smoothstep(0.0, 0.25, -e.y + 0.6) * 0.85;     // zadni cast kruhu je za kartami - slabsi
  c += mix(vec3(1.0,0.70,0.28), uBarva, 0.25)*kruhy*0.55;
  // (hvezdy se kresli zvlast jako body v plnem rozliseni - tohle pozadi je v mensim)
  // vinetace
  vec2 v = uv - 0.5;
  c *= 1.0 - dot(v*vec2(0.9, 0.7), v*vec2(0.9, 0.7))*1.1;
  gl_FragColor = vec4(c, 1.0);
}
)";

// zvetseni pozadi (FBO v mensim rozliseni) na obrazovku
static const char *FS_KOPIE_MENU = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform sampler2D uTex;
uniform float uJas;
void main(){ gl_FragColor = vec4(texture2D(uTex, vUV).rgb*uJas, 1.0); }
)";

// ---------- spolecne pozadi karty (uvnitr karty, do FBO karty) ----------
// Tmavy "showroom": zare v barve karty, kuzel svetla shora, zlaty kruh na podlaze
// (pod predmetem) se stinem, jiskry. uPodlaha = vyska podlahy (0..1, y nahoru).
static const char *FS_KARTA_POZADI = R"(
varying vec2 vUV;
uniform vec3 uBarva;
uniform float uCas;
uniform float uPomer;     // sirka / vyska karty
uniform float uPodlaha;   // y stredu kruhu na podlaze
uniform float uKruh;      // polomer kruhu (dil sirky)
uniform float uStin;      // sila stinu pod predmetem
uniform float uFokus;
uniform float uVlna;      // svitici zvukova vlna za predmetem (kazeta)
)" "%SUM%" R"(
float sq(float x){ return x*x; }
void main(){
  vec2 uv = vUV;
  vec2 p = vec2((uv.x - 0.5)*uPomer, uv.y);
  vec3 c = mix(vec3(0.006,0.005,0.007), vec3(0.020,0.014,0.010), uv.y);
  // zare v barve karty (za predmetem)
  vec2 z = vec2(p.x, uv.y - 0.60);
  c += uBarva*exp(-dot(z,z)*5.5)*0.28;
  c += vec3(1.0,0.8,0.5)*exp(-dot(z,z)*14.0)*0.05;
  // zvukova vlna (azurova, jako na displeji prehravace)
  if (uVlna > 0.0) {
    float x = uv.x;
    float obal = sin(x*3.14159);
    float y0 = 0.64 + obal*0.055*(sin(x*23.0 - uCas*5.0)*0.6 + sin(x*61.0 + uCas*7.3)*0.3 + sin(x*7.0 - uCas*2.1)*0.4);
    float dv = abs(uv.y - y0);
    c += vec3(0.25,0.80,1.0)*(exp(-dv*dv*90000.0)*0.9 + exp(-dv*dv*2500.0)*0.22)*uVlna*obal;
  }
  // kuzel svetla shora
  float sirka = 0.10 + (1.0 - uv.y)*0.32;
  c += vec3(1.0,0.86,0.62)*exp(-p.x*p.x/(sirka*sirka))*smoothstep(0.15, 1.0, uv.y)*0.07;
  // podlaha: elipsa se zlatym kruhem a ryskami (pod predmetem)
  vec2 e = vec2(p.x, (uv.y - uPodlaha)*4.2);
  float r = length(e)/uKruh;
  float a = atan(e.y, e.x);
  float kr = exp(-sq((r - 1.0)/0.025))*0.9 + exp(-sq((r - 0.82)/0.012))*0.45;
  float ryska = step(0.55, fract(a*36.0/6.2831853 + uCas*0.03))*exp(-sq((r - 1.12)/0.03));
  float predni = 0.55 + 0.45*smoothstep(0.05, -0.15, e.y);
  vec3 zl = mix(vec3(1.0,0.72,0.30), uBarva, 0.30);
  c += zl*(kr + ryska*0.6)*predni*(0.55 + 0.45*uFokus);
  c += zl*exp(-r*r*1.6)*0.10;                            // svit podlahy
  c *= 1.0 - uStin*exp(-r*r*3.0)*0.75;                   // stin pod predmetem
  // jiskry
  vec2 g = uv*vec2(uPomer, 1.0)*90.0;
  vec2 gi = floor(g);
  float h = texture2D(uSum, (gi + 0.5)/256.0).x;
  if (h > 0.975) {
    vec2 f = fract(g) - 0.5;
    float tw = 0.5 + 0.5*sin(uCas*2.0 + h*50.0);
    c += vec3(1.0,0.9,0.7)*exp(-dot(f,f)*20.0)*(h - 0.975)*28.0*tw*0.25;
  }
  vec2 v = uv - 0.5;
  c *= 1.0 - dot(v, v)*0.9;
  gl_FragColor = vec4(c, 1.0);
}
)";

// stitek dole na karte (tmavy prechod pod nazvem)
static const char *FS_STITEK = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform vec3 uBarva;
uniform float uAlfa;
float sq(float x){ return x*x; }
void main(){
  float a = smoothstep(1.0, 0.25, vQ.y)*0.88;
  vec3 c = vec3(0.012,0.008,0.004);
  // tenka zlata linka nahore
  float l = exp(-sq((vQ.y - 0.80)/0.008))*smoothstep(0.0, 0.25, vQ.x)*smoothstep(1.0, 0.75, vQ.x);
  c += mix(vec3(1.0,0.78,0.36), uBarva, 0.3)*l*0.9;
  a = max(a, l*0.9);
  gl_FragColor = vec4(c*a, a)*uAlfa;
}
)";

// zamcena karta: odbarvit a ztmavit (kopie obrazku karty)
static const char *FS_ZAMCENO = R"(
varying vec2 vUV;
varying vec2 vQ;
uniform sampler2D uTex;
void main(){
  vec3 c = texture2D(uTex, vUV).rgb;
  float y = dot(c, vec3(0.30, 0.59, 0.11));
  c = mix(vec3(y), c, 0.18)*0.42;
  gl_FragColor = vec4(c, 1.0);
}
)";

// ---------- 3D predmety v kartach (kazeta, kniha): studiove svetlo ----------
// Material (aM.x), barva v aM.yzw:
//  0 plast   1 potisk (atlas)   2 sklo okenka (pruhledne)   3 bily plast (civky)
//  4 pasek (leskly hnedy)   5 kov (sroubky)   6 svitici   7 papir (atlas s textem)
//  8 kuze / platno desek   9 zlata razba (atlas na deskach)
static const char *FS_STUDIO = R"(
varying vec3 vP;
varying vec3 vN;
varying vec2 vUV;
varying vec4 vM;
uniform vec3 uKamera;
uniform vec3 uKlic;
uniform vec3 uKlicBarva;
uniform vec3 uVypln;
uniform vec3 uVyplnBarva;
uniform vec3 uKontra;
uniform vec3 uKontraBarva;
uniform vec3 uAmb;
uniform float uExpo;
uniform sampler2D uAtlas;
)" "%SUM%" R"(
void main(){
  int m = int(vM.x + 0.5);
  vec3 N = normalize(vN);
  vec3 V = normalize(uKamera - vP);
  if (dot(N, V) < 0.0) N = -N;
  vec3 alb = vM.yzw;
  float spec = 0.35, shin = 36.0, odraz = 0.0, alfa = 1.0;
  vec3 emis = vec3(0.0);
  if (m == 0) {
    alb *= 0.94 + 0.06*sum2(vP.xy*30.0);
    spec = 0.45; shin = 48.0; odraz = 0.08;
  } else if (m == 1) {
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(alb, a.rgb, a.a);
    spec = 0.30; shin = 30.0; odraz = 0.05;
  } else if (m == 2) {
    alb = vec3(0.01, 0.012, 0.015);
    spec = 1.6; shin = 140.0; odraz = 0.55; alfa = 0.30;
  } else if (m == 3) {
    spec = 0.6; shin = 50.0;
  } else if (m == 4) {
    alb *= 0.85 + 0.15*sum2(vec2(length(vP.xy)*260.0, 0.0));
    spec = 1.0; shin = 70.0; odraz = 0.15;
  } else if (m == 5) {
    spec = 1.3; shin = 90.0; odraz = 0.45;
  } else if (m == 6) {
    emis = alb*2.2; alb *= 0.12;
  } else if (m == 7) {
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(vec3(0.94, 0.90, 0.81), a.rgb, a.a);
    alb *= 0.93 + 0.07*sum2(vUV*vec2(300.0, 400.0));
    spec = 0.05; shin = 6.0;
  } else if (m == 8) {
    alb *= 0.85 + 0.15*sum2(vP.xz*60.0 + vP.y*20.0);
    spec = 0.25; shin = 18.0;
  } else if (m == 9) {
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(alb, vec3(0.95, 0.72, 0.30), a.a);
    spec = mix(0.25, 1.4, a.a); shin = mix(18.0, 70.0, a.a); odraz = 0.5*a.a;
  } else if (m == 11) {
    // orizka stranek (tenke vrstvy papiru)
    float l = fract(vP.y*55.0);
    alb = vec3(0.92, 0.88, 0.78)*(0.80 + 0.20*smoothstep(0.2, 0.5, l));
    spec = 0.06; shin = 8.0;
  } else if (m == 10) {
    // predni strana s potiskem a s dirami (okenka): pruhledne misto se nekresli
    vec4 a = texture2D(uAtlas, vUV);
    if (a.a < 0.5) discard;
    alb = a.rgb;
    float papir = step(0.45, dot(alb, vec3(0.33)));      // svetly stitek je matny, cerny plast leskly
    spec = mix(0.70, 0.20, papir); shin = mix(70.0, 20.0, papir); odraz = mix(0.22, 0.04, papir);
  }
  vec3 c = alb*uAmb;
  c += alb*uKlicBarva*max(dot(N, uKlic), 0.0);
  c += uKlicBarva*spec*pow(max(dot(N, normalize(uKlic + V)), 0.0), shin);
  c += alb*uVyplnBarva*max(dot(N, uVypln), 0.0);
  c += uVyplnBarva*spec*0.4*pow(max(dot(N, normalize(uVypln + V)), 0.0), shin);
  float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
  c += uKontraBarva*(rim*0.9 + max(dot(N, uKontra), 0.0)*0.35);
  if (odraz > 0.0) {
    vec3 R = reflect(-V, N);
    float nebe = smoothstep(-0.3, 0.9, R.y);
    vec3 env = mix(vec3(0.02, 0.015, 0.01), vec3(0.85, 0.70, 0.50), nebe*nebe)*0.5;
    env += uKlicBarva*pow(max(dot(R, uKlic), 0.0), 80.0)*2.5;
    env += uKontraBarva*pow(max(dot(R, uKontra), 0.0), 20.0)*0.8;
    c += env*odraz;
  }
  c += emis;
  c = vec3(1.0) - exp(-c*uExpo);
  gl_FragColor = vec4(c*alfa, alfa);
}
)";

}  // namespace napmenu
