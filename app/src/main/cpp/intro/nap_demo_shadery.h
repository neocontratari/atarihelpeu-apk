// nap_demo_shadery.h
// ====================================================================
//  B304: SHADERY UVODNIHO DEMA (OpenGL ES 2.0 / GLSL ES 1.00)
//
//  Sum je z textury 256x256 (kanal G = R posunute o 37,17) - na mobilech
//  presnejsi a rychlejsi nez sin-hash. Tezke veci (mraky, garaz) se
//  kresli v mensim rozliseni a pak se zvetsi - jsou mekke, rozdil neni
//  videt a telefon to utahne. Nakonec jde cely obraz pres "film":
//  zare kolem svetel (bloom), vinetace a zrno.
// ====================================================================
#pragma once

namespace napdemo {

// ---------- spolecne ----------
static const char *VS_PLNY = R"(
attribute vec2 aPos;
varying vec2 vUV;
void main(){ vUV = aPos*0.5+0.5; gl_Position = vec4(aPos,0.0,1.0); }
)";

// obdelnik v NDC (x0,y0,x1,y1) a UV (u0,v0,u1,v1)
static const char *VS_OBD = R"(
attribute vec2 aPos;
uniform vec4 uObd;
uniform vec4 uUV;
varying vec2 vUV;
void main(){
  vec2 t = aPos*0.5+0.5;
  gl_Position = vec4(mix(uObd.xy, uObd.zw, t), 0.0, 1.0);
  vUV = mix(uUV.xy, uUV.zw, t);
}
)";

static const char *SUM_GLSL = R"(
uniform sampler2D uSum;
float sum2(vec2 x){ vec2 p=floor(x); vec2 f=fract(x); f=f*f*(3.0-2.0*f); return texture2D(uSum,(p+f+0.5)/256.0).x; }
float sum3(vec3 x){
  vec3 p=floor(x); vec3 f=fract(x); f=f*f*(3.0-2.0*f);
  vec2 uv=(p.xy+vec2(37.0,17.0)*p.z)+f.xy;
  vec2 rg=texture2D(uSum,(uv+0.5)/256.0).xy;
  return mix(rg.x,rg.y,f.z);
}
float fbm2(vec2 p){ float a=0.5,s=0.0; for(int i=0;i<5;i++){ s+=a*sum2(p); p=p*2.03+vec2(1.7,9.2); a*=0.5; } return s; }
float fbm3(vec3 p){ float a=0.5,s=0.0; for(int i=0;i<4;i++){ s+=a*sum3(p); p=p*2.01+vec3(1.7,9.2,3.1); a*=0.5; } return s; }
)";

// textura (premultiplied alfa) * uAlfa
static const char *FS_TEX = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform float uAlfa;
void main(){ gl_FragColor = texture2D(uTex, vUV) * uAlfa; }
)";

// zvetseni mensiho framebufferu na obrazovku
static const char *FS_KOPIE = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform float uJas;
void main(){ gl_FragColor = vec4(texture2D(uTex, vUV).rgb * uJas, 1.0); }
)";

// jednobarevny prekryv (zatmivacka, zablesk)
static const char *FS_BARVA = R"(
uniform vec4 uBarva;
void main(){ gl_FragColor = uBarva; }
)";

// mekka kulata zare (pricita se)
static const char *FS_ZARE = R"(
varying vec2 vUV;
uniform vec3 uBarva;
uniform float uTvrdost;
void main(){
  vec2 q = vUV*2.0-1.0;
  float r2 = dot(q,q);
  float g = exp(-r2*uTvrdost) * (1.0 - smoothstep(0.85, 1.0, r2));
  gl_FragColor = vec4(uBarva*g, 0.0);
}
)";

// ---------- FILM: zare kolem svetel (bloom), vinetace, zrno ----------
// jasne casti a zmenseni (4 vzorky = prumer 4x4 bodu)
static const char *FS_JAS = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uKrok;
uniform float uPrah;
void main(){
  vec3 c = texture2D(uTex, vUV + uKrok*vec2(-1.0,-1.0)).rgb + texture2D(uTex, vUV + uKrok*vec2(1.0,-1.0)).rgb
         + texture2D(uTex, vUV + uKrok*vec2(-1.0, 1.0)).rgb + texture2D(uTex, vUV + uKrok*vec2(1.0, 1.0)).rgb;
  c *= 0.25;
  float l = max(max(c.r, c.g), c.b);
  float k = max(l - uPrah, 0.0) / max(l, 1e-4);
  gl_FragColor = vec4(c*k, 1.0);
}
)";
// Gauss 9 bodu pres 5 vzorku (linearni filtr)
static const char *FS_ROZMAZ = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uSmer;
void main(){
  vec3 c = texture2D(uTex, vUV).rgb*0.2270270;
  c += texture2D(uTex, vUV + uSmer*1.3846154).rgb*0.3162162;
  c += texture2D(uTex, vUV - uSmer*1.3846154).rgb*0.3162162;
  c += texture2D(uTex, vUV + uSmer*3.2307692).rgb*0.0702703;
  c += texture2D(uTex, vUV - uSmer*3.2307692).rgb*0.0702703;
  gl_FragColor = vec4(c, 1.0);
}
)";
static const char *FS_FINAL = R"(
varying vec2 vUV;
uniform sampler2D uScena;
uniform sampler2D uZare1;
uniform sampler2D uZare2;
uniform sampler2D uSum;
uniform vec2 uRozmer;
uniform float uZare;
uniform float uCas;
uniform float uVinet;
uniform float uZrno;
void main(){
  vec3 c = texture2D(uScena, vUV).rgb;
  vec3 b = texture2D(uZare1, vUV).rgb*0.55 + texture2D(uZare2, vUV).rgb*0.85;
  c += b*uZare;
  // vinetace (kruhova i na vysku)
  vec2 q = (vUV - 0.5)*vec2(uRozmer.x/max(uRozmer.x, uRozmer.y), uRozmer.y/max(uRozmer.x, uRozmer.y))*1.6;
  c *= 1.0 - dot(q, q)*0.55*uVinet;
  // zrno: v tmach jemne, ve stredu vic (a zaroven rozbije pruhy v prechodech)
  float n = texture2D(uSum, gl_FragCoord.xy/256.0 + vec2(fract(uCas*7.31)*31.0, fract(uCas*3.17)*17.0)).z - 0.5;
  float l = dot(c, vec3(0.299, 0.587, 0.114));
  c += n*uZrno*(0.018 + 0.16*l*(1.0 - l));
  gl_FragColor = vec4(c, 1.0);
}
)";

// ---------- SCENA 1: GARAZ (paprsky do jednoduche mistnosti) ----------
static const char *FS_GARAZ = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform float uCas;
uniform vec3 uKam;
uniform vec3 uCil;
uniform float uOhnisko;
uniform float uDvere;     // 0 zavrene .. 1 nahore
uniform float uExpo;
uniform float uZarovka;
uniform vec3 uZar;        // poloha zarovky (trochu se houpe)
uniform float uTv;        // stara televize na ponku
uniform sampler2D uNebe;
const float SX = 1.65;    // pul sirky garaze
const float VY = 2.55;    // strop
const float ZB = 5.6;     // stena s vraty
const float ZF = -2.8;    // stena za kamerou
const float DW = 1.22;    // pul sirky vrat
const float DH = 2.12;    // vyska vrat
const vec3 PA = vec3(-1.65, 0.0, 1.0);   const vec3 PB = vec3(-0.98, 0.9, 3.9);    // ponk
const vec3 TA = vec3(-1.46, 0.9, 1.32);  const vec3 TB = vec3(-0.94, 1.40, 1.86);  // televize (obrazovka na z = TA.z)
const vec3 RA = vec3(1.18, 0.0, 0.7);    const vec3 RB = vec3(1.65, 2.15, 3.3);    // regal
const vec3 SUD = vec3(0.92, 0.0, 4.72);  // sud (stred dole), polomer 0.29, vyska 0.88
)" "%SUM%" R"(
vec2 krabice(vec3 ro, vec3 rd, vec3 a, vec3 b){
  vec3 inv = 1.0/rd;
  vec3 t0 = (a-ro)*inv, t1 = (b-ro)*inv;
  vec3 mn = min(t0,t1), mx = max(t0,t1);
  return vec2(max(max(mn.x,mn.y),mn.z), min(min(mx.x,mx.y),mx.z));
}
vec3 normalaKrabice(vec3 p, vec3 a, vec3 b){
  vec3 c = (a+b)*0.5, h = (b-a)*0.5, d = (p-c)/h;
  vec3 ad = abs(d);
  if (ad.x > ad.y && ad.x > ad.z) return vec3(sign(d.x),0.0,0.0);
  if (ad.y > ad.z) return vec3(0.0,sign(d.y),0.0);
  return vec3(0.0,0.0,sign(d.z));
}
float valec(vec3 ro, vec3 rd, vec3 c, float r, float h, out vec3 n){
  vec2 o = ro.xz - c.xz;
  float A = dot(rd.xz, rd.xz), B = dot(o, rd.xz), C = dot(o,o) - r*r;
  float D = B*B - A*C;
  n = vec3(0.0, 1.0, 0.0);
  if (D < 0.0 || A < 1e-6) return -1.0;
  float t = (-B - sqrt(D))/A;
  float y = ro.y + rd.y*t;
  if (t > 0.0 && y > c.y && y < c.y + h) { vec3 p = ro + rd*t; n = vec3(p.x - c.x, 0.0, p.z - c.z)/r; return t; }
  if (rd.y < 0.0 && ro.y > c.y + h) {
    float tc = (c.y + h - ro.y)/rd.y;
    vec2 pc = ro.xz + rd.xz*tc - c.xz;
    if (dot(pc,pc) < r*r) return tc;
  }
  return -1.0;
}
float dvereDole(){ return max(uDvere*DH, 0.014); }      // i zavrenymi vraty prosvita spara
// je bod na svetle zvenku? (paprsek k obloze projde otevrenymi vraty)
float zvenku(vec3 p){
  vec3 L = vec3(0.0, 1.5, 15.0);
  if (p.z >= ZB) return 1.0;
  float k = (ZB - p.z)/(L.z - p.z);
  vec3 q = p + (L-p)*k;
  float okraj = smoothstep(DW+0.04, DW-0.20, abs(q.x));
  return okraj * smoothstep(dvereDole()+0.03, dvereDole()-0.10, q.y);
}
// stin od zarovky (tvrdy - zarovka je mala)
float stin(vec3 p, vec3 L, float dl){
  vec2 h = krabice(p, L, PA, PB);  if (h.x < h.y && h.y > 0.0 && h.x < dl) return 0.0;
  h = krabice(p, L, TA, TB);       if (h.x < h.y && h.y > 0.0 && h.x < dl) return 0.0;
  h = krabice(p, L, RA, RB);       if (h.x < h.y && h.y > 0.0 && h.x < dl) return 0.0;
  vec3 nn; float tv = valec(p, L, SUD, 0.29, 0.88, nn); if (tv > 0.0 && tv < dl) return 0.0;
  return 1.0;
}
float bloky(vec2 p){
  vec2 b = p*vec2(2.5,5.0);
  b.x += step(1.0, mod(floor(b.y),2.0))*0.5;
  vec2 f = fract(b);
  float m = smoothstep(0.0,0.04,f.x)*smoothstep(1.0,0.96,f.x)*smoothstep(0.0,0.07,f.y)*smoothstep(1.0,0.93,f.y);
  return 0.80 + 0.20*m;
}
float kapsle(vec2 p, vec2 a, vec2 b, float r){ vec2 pa=p-a, ba=b-a; float h=clamp(dot(pa,ba)/dot(ba,ba),0.0,1.0); return length(pa-ba*h)-r; }
// naradi na desce s dirkami (siluety)
float naradi(vec2 d){
  float s = 1e3;
  s = min(s, kapsle(d, vec2(0.22,0.22), vec2(0.22,0.62), 0.012));
  s = min(s, kapsle(d, vec2(0.22,0.58), vec2(0.22,0.76), 0.028));
  s = min(s, kapsle(d, vec2(0.40,0.16), vec2(0.40,0.60), 0.020));
  s = min(s, abs(length(d - vec2(0.40,0.66)) - 0.045) - 0.016);
  s = min(s, kapsle(d, vec2(0.66,0.14), vec2(0.66,0.62), 0.018));
  s = min(s, kapsle(d, vec2(0.57,0.66), vec2(0.77,0.66), 0.040));
  s = min(s, kapsle(d, vec2(0.95,0.15), vec2(0.95,0.72), 0.010));
  s = min(s, kapsle(d, vec2(1.06,0.15), vec2(1.06,0.68), 0.010));
  vec2 ps = d - vec2(1.62, 0.42);
  s = min(s, max(abs(ps.x) - 0.36, abs(ps.y + ps.x*0.16) - 0.085));
  s = min(s, kapsle(d, vec2(2.10,0.20), vec2(2.10,0.74), 0.026));
  s = min(s, abs(length(d - vec2(2.45,0.50)) - 0.13) - 0.022);
  return smoothstep(0.006, 0.0, s);
}
void main(){
  float asp = uRozmer.x/uRozmer.y;
  vec2 q = (vUV-0.5)*2.0; q.x *= asp;
  vec3 fw = normalize(uCil-uKam);
  vec3 rt = normalize(cross(vec3(0.0,1.0,0.0), fw));
  vec3 up = cross(fw, rt);
  vec3 rd = normalize(fw*uOhnisko + rt*q.x + up*q.y);
  rd += vec3(1.3e-5, 1.1e-5, 1.7e-5);
  vec3 ro = uKam;
  float dd = dvereDole();
  bool venku = false;
  float t = 1e5;
  int druh = 0;                    // 0 bok, 1 podlaha, 2 strop, 3 stena s vraty, 4 predni, 5 ponk, 6 TV, 7 regal, 8 sud
  vec3 n = vec3(0.0, 1.0, 0.0);
  if (ro.z > ZB - 0.002) {
    venku = true;                  // kamera uz vyjela ven
  } else {
    float tx = ((rd.x>0.0 ? SX : -SX) - ro.x)/rd.x;
    float ty = ((rd.y>0.0 ? VY : 0.0) - ro.y)/rd.y;
    float tz = ((rd.z>0.0 ? ZB : ZF) - ro.z)/rd.z;
    t = min(tx, min(ty, tz));
    n = vec3(-sign(rd.x),0.0,0.0);
    if (t == ty) { druh = rd.y > 0.0 ? 2 : 1; n = vec3(0.0,-sign(rd.y),0.0); }
    else if (t == tz) { druh = rd.z > 0.0 ? 3 : 4; n = vec3(0.0,0.0,-sign(rd.z)); }
    if (druh == 3) { vec3 pp = ro + rd*t; if (abs(pp.x) < DW && pp.y < dd) venku = true; }
    vec2 h = krabice(ro, rd, PA, PB);
    if (h.x < h.y && h.x > 0.0 && h.x < t) { t = h.x; druh = 5; n = normalaKrabice(ro+rd*t, PA, PB); venku = false; }
    h = krabice(ro, rd, TA, TB);
    if (h.x < h.y && h.x > 0.0 && h.x < t) { t = h.x; druh = 6; n = normalaKrabice(ro+rd*t, TA, TB); venku = false; }
    h = krabice(ro, rd, RA, RB);
    if (h.x < h.y && h.x > 0.0 && h.x < t) { t = h.x; druh = 7; n = normalaKrabice(ro+rd*t, RA, RB); venku = false; }
    vec3 ns; float ts = valec(ro, rd, SUD, 0.29, 0.88, ns);
    if (ts > 0.0 && ts < t) { t = ts; druh = 8; n = ns; venku = false; }
  }
  vec3 p = ro + rd*t;
  vec3 alb = vec3(0.40);
  vec3 emis = vec3(0.0);
  float lesk = 0.0;
  vec3 c;
  if (venku) {
    c = texture2D(uNebe, vUV).rgb;
    t = 1e5;
  } else {
    if (druh == 1) {                      // beton
      vec2 f = p.xz;
      alb = vec3(0.34,0.34,0.35) * (0.70 + 0.5*fbm2(f*1.3));
      alb *= 0.86 + 0.14*sum2(f*11.0);
      float ol = smoothstep(0.60, 0.74, fbm2(f*0.9 + 7.0));
      alb *= 1.0 - 0.55*ol;
      float j = abs(fract(p.z*0.5) - 0.5)*2.0;
      alb *= 1.0 - 0.55*smoothstep(0.985, 1.0, j);
      lesk = 0.20 + 0.55*smoothstep(0.52, 0.70, fbm2(f*0.75 + 3.0)) + 0.35*smoothstep(3.2, 5.6, p.z);
      lesk = min(lesk, 0.95) + ol*0.3;
      // kouty jsou tmavsi
      alb *= 0.55 + 0.45*smoothstep(0.0, 0.45, SX - abs(p.x));
    } else if (druh == 2) {
      alb = vec3(0.15,0.15,0.16)*(0.8 + 0.4*fbm2(p.xz*1.5));
    } else if (druh == 3) {
      if (abs(p.x) < DW && p.y < DH) {
        float pv = p.y - dd;               // vrata jedou nahoru i s panely
        float panel = fract(pv/(DH/4.0));
        float spara = smoothstep(0.0, 0.03, panel)*smoothstep(1.0, 0.97, panel);
        float zebra = 0.92 + 0.08*smoothstep(-0.3, 0.7, sin(pv*125.0));
        float spina = fbm2(vec2(p.x*2.0, pv*1.5));
        float rez = smoothstep(0.55, 0.80, fbm2(vec2(p.x*5.0, pv*3.0 + 9.0))) * smoothstep(0.9, 0.0, pv);
        alb = vec3(0.50,0.51,0.52) * spara * zebra * (0.78 + 0.32*spina);
        alb = mix(alb, vec3(0.28,0.15,0.07), rez*0.65);
        alb *= 0.3 + 0.7*smoothstep(0.0, 0.035, pv);       // gumove tesneni dole
        lesk = 0.25;
      } else {
        alb = vec3(0.40,0.40,0.41) * bloky(p.xy) * (0.80 + 0.35*fbm2(p.xy*1.9));
        float ram = step(abs(p.x), DW+0.10) * step(p.y, DH+0.10);
        alb = mix(alb, vec3(0.20,0.20,0.21), ram);
        alb *= 0.75 + 0.25*smoothstep(0.0, 0.6, p.y);
      }
    } else if (druh == 0 || druh == 4) {
      vec2 s = druh == 0 ? p.zy : p.xy;
      alb = vec3(0.39,0.39,0.40) * bloky(s) * (0.80 + 0.35*fbm2(s*1.9));
      alb *= 0.72 + 0.28*smoothstep(0.0, 0.7, p.y);
      if (druh == 0 && p.x < 0.0 && p.z > 1.0 && p.z < 3.9 && p.y > 1.02 && p.y < 2.02) {
        vec2 d = vec2(p.z - 1.0, p.y - 1.02);
        alb = vec3(0.33,0.24,0.15)*(0.85 + 0.15*sum2(d*20.0));
        vec2 dz = fract(d/0.055) - 0.5;
        alb *= 1.0 - 0.55*smoothstep(0.20, 0.12, length(dz));
        float nr = naradi(d);
        alb = mix(alb, vec3(0.09,0.09,0.10), nr);
        lesk = nr*0.5;
      }
    } else if (druh == 5) {                 // ponk - drevo
      if (n.y > 0.5) {
        float g = sum2(vec2(p.z*1.2, p.x*28.0)) + 0.5*sum2(vec2(p.z*4.0, p.x*60.0));
        alb = vec3(0.36,0.24,0.14)*(0.65 + 0.35*g);
        lesk = 0.15;
      } else {
        alb = vec3(0.17,0.19,0.16)*(0.8 + 0.3*fbm2(p.zy*3.0));
        float sup = fract((p.z - 1.0)/0.97);
        alb *= 0.6 + 0.4*smoothstep(0.0, 0.02, sup)*smoothstep(1.0, 0.98, sup);
        alb *= 0.6 + 0.4*smoothstep(0.86, 0.80, p.y);
      }
    } else if (druh == 6) {                 // stara televize
      alb = vec3(0.14,0.09,0.055)*(0.8 + 0.3*sum2(p.xy*30.0));
      lesk = 0.25;
      if (n.z < -0.5) {
        vec2 sc = vec2((p.x - (TA.x + 0.05))/0.36, (p.y - (TA.y + 0.07))/0.36);
        if (sc.x > 0.0 && sc.x < 1.0 && sc.y > 0.0 && sc.y < 1.0) {
          vec2 kr = sc - 0.5;
          float roh = smoothstep(0.50, 0.44, max(abs(kr.x),abs(kr.y)) + dot(kr,kr)*0.15);
          float zrn = sum2(sc*vec2(110.0,85.0) + vec2(uCas*530.0, uCas*210.0));
          float rad = 0.72 + 0.28*sin(sc.y*260.0);
          float pas = 0.85 + 0.15*smoothstep(0.0, 0.08, abs(fract(sc.y*0.7 - uCas*0.45) - 0.5));
          emis = vec3(0.34,0.48,0.66) * (0.45 + 0.75*zrn) * rad * pas * roh;
          alb = vec3(0.03)*(1.0 - roh) + vec3(0.01);
          lesk = 0.6;
        }
        // knofliky vpravo
        vec2 kn = p.xy - vec2(TB.x - 0.065, TA.y + 0.30);
        float k1 = length(kn) - 0.026, k2 = length(kn + vec2(0.0, 0.11)) - 0.026;
        if (min(k1, k2) < 0.0) { alb = vec3(0.05); lesk = 0.5; }
      }
    } else if (druh == 7) {                 // regal s krabicemi a plechovkami
      float pol = fract(p.y/0.53);
      float patro = floor(p.y/0.53);
      alb = vec3(0.22,0.23,0.24);
      if (n.x < -0.5) {
        if (pol > 0.07) {
          float zz = p.z/0.38 + patro*0.37;
          float kus = floor(zz);
          float fz = fract(zz);
          float r = fract(sin(kus*17.13 + patro*31.7)*43758.5453);
          float vy = 0.40 + 0.52*fract(r*7.77);
          float sir = 0.70 + 0.28*fract(r*3.33);
          bool uvnitr = pol < vy && abs(fz - 0.5) < sir*0.5;
          alb = vec3(0.035);
          if (uvnitr) {
            float hx = 1.0 - smoothstep(sir*0.5 - 0.05, sir*0.5, abs(fz - 0.5));
            float hy = 1.0 - smoothstep(vy - 0.05, vy, pol);
            if (r < 0.42) {                                     // kartonova krabice s paskou
              alb = vec3(0.40,0.29,0.17)*(0.78 + 0.3*sum2(p.zy*9.0));
              alb *= 0.65 + 0.35*hx*hy;
              alb = mix(alb, vec3(0.55,0.45,0.30), smoothstep(0.03, 0.0, abs(pol - vy*0.55))*0.6);
            } else if (r < 0.72) {                              // plechovka barvy
              float val = 0.55 + 0.45*sin((fz - 0.5)/sir*3.14159);
              alb = mix(vec3(0.42,0.08,0.05), vec3(0.10,0.22,0.42), step(0.58, r))*val;
              alb = mix(alb, vec3(0.75,0.72,0.65)*val, step(vy*0.35, pol)*step(pol, vy*0.62));   // stitek
              lesk = 0.45;
            } else {                                            // lahev / sprej
              float val = 0.5 + 0.5*sin((fz - 0.5)/sir*3.14159);
              alb = vec3(0.10,0.30,0.12)*val;
              lesk = 0.6;
            }
          }
        } else alb = vec3(0.30,0.31,0.32);
      }
    } else if (druh == 8) {                 // sud
      float zeb = 0.75 + 0.25*smoothstep(0.03, 0.0, abs(fract(p.y/0.29) - 0.5) - 0.45);
      alb = vec3(0.10,0.18,0.30)*zeb*(0.8 + 0.4*fbm2(vec2(atan(n.z, n.x)*2.0, p.y*4.0)));
      alb = mix(alb, vec3(0.26,0.14,0.06), smoothstep(0.55, 0.8, fbm2(vec2(atan(n.z, n.x)*3.0, p.y*6.0))));
      if (n.y > 0.5) alb = vec3(0.08,0.09,0.10);
      lesk = 0.35;
    }
    // ---- svetla ----
    vec3 Lb = uZar - p; float db = length(Lb); Lb /= db;
    float sb = stin(p + n*0.003, Lb, db);
    vec3 zar = vec3(1.0,0.70,0.40) * uZarovka * 2.7 * max(dot(n,Lb),0.0) / (1.0 + db*db*0.55) * sb;
    vec3 Lo = vec3(0.0, 1.3, 9.5) - p; float dlo = length(Lo);
    vec3 ven = vec3(0.52,0.60,0.76) * (0.12 + 2.1*uDvere) * zvenku(p) * max(dot(n,Lo/dlo),0.0) / (1.0 + dlo*dlo*0.025);
    vec3 Lt = vec3(-1.20, 1.15, 1.20) - p; float dt = length(Lt); Lt /= dt;
    float tvf = 0.78 + 0.22*sum2(vec2(uCas*31.0, 1.0));
    vec3 tvs = vec3(0.30,0.46,0.78) * uTv * 0.9 * tvf * max(dot(n,Lt),0.0) * (0.25 + 0.75*max(-Lt.z, 0.0)) / (1.0 + dt*dt*2.4);
    vec3 amb = vec3(0.020,0.022,0.026);
    c = alb*(amb + zar + ven)*uExpo + alb*tvs + emis*uTv;
    // lesk: zarovka v mokrem betonu
    vec3 hh = normalize(Lb - rd);
    c += vec3(1.0,0.78,0.5) * uZarovka * lesk * sb * pow(max(dot(n,hh),0.0), 60.0) * 1.2 * uExpo;
    // odraz oblohy v mokre podlaze (pres otvor vrat)
    if (druh == 1) {
      vec3 rr = reflect(rd, n);
      if (rr.z > 0.0) {
        float tw = (ZB - p.z)/rr.z;
        vec3 qq = p + rr*tw;
        if (abs(qq.x) < DW && qq.y < dd) {
          vec3 v = qq - ro;
          float z = dot(v, fw);
          vec2 s = vec2(dot(v, rt), dot(v, up))/z*uOhnisko;
          vec2 uv = vec2(s.x/asp, s.y)*0.5 + 0.5;
          float rozm = 0.5 + 0.5*sum2(p.xz*14.0);
          c += texture2D(uNebe, clamp(uv, 0.0, 1.0)).rgb * lesk * 0.75 * rozm * smoothstep(0.0, 0.04, dd - qq.y);
        }
      }
      // odraz zarovky a televize jako rozmazane skvrny
      vec3 rr2 = reflect(rd, n);
      float kz = max(dot(uZar - p, rr2), 0.0);
      float dz = length(p + rr2*kz - uZar);
      c += vec3(1.0,0.75,0.45) * uZarovka * lesk * exp(-dz*dz*14.0) * 0.55 * uExpo;
      float kt = max(dot(vec3(-1.2,1.15,1.3) - p, rr2), 0.0);
      float dtt = length(p + rr2*kt - vec3(-1.2,1.15,1.3));
      c += vec3(0.3,0.45,0.7) * uTv * lesk * exp(-dtt*dtt*22.0) * 0.4;
    }
  }
  // svetlo z vrat v prasnem vzduchu (paprsky)
  float sv = 0.0;
  for (int i = 0; i < 8; i++) {
    float z = ZB - 0.15 - float(i)*0.48;
    float k = (z - ro.z)/rd.z;
    if (k > 0.0 && k < t) {
      vec3 s = ro + rd*k;
      sv += zvenku(s) * (0.55 + 0.9*sum3(vec3(s.xy*2.6, uCas*0.35 + float(i))));
    }
  }
  // (kdyz se kamera blizi k vratum, paprsky slabnou - jinak by zakryly vyhled ven)
  c += vec3(0.48,0.55,0.70) * sv * 0.026 * (0.25 + 0.75*uDvere) * uExpo * (1.0 - smoothstep(3.6, 5.4, ro.z)) * (venku ? 0.55 : 1.0);
  // zarovka sama (malinka koule), drat ke stropu a zare ve vzduchu
  float kz = dot(uZar - ro, rd);
  if (kz > 0.0 && kz < t) {
    float d = length(ro + rd*kz - uZar);
    c += vec3(1.0,0.84,0.6) * uZarovka * (smoothstep(0.05, 0.03, d)*5.0 + exp(-d*d*45.0)*0.55 + exp(-d*d*4.0)*0.10) * max(uExpo, 0.35);
  }
  {
    // kabel: usecka uZar .. (uZar.x, VY, uZar.z)
    vec3 a = uZar + vec3(0.0, 0.05, 0.0), b = vec3(uZar.x*0.3, VY, uZar.z);
    vec3 ba = b - a, oa = ro - a;
    float bb = dot(ba,ba), br = dot(ba, rd), ob = dot(oa, ba), orr = dot(oa, rd);
    float den = bb - br*br;
    float s = clamp((ob - orr*br)/max(den, 1e-6), 0.0, 1.0);
    float tr = dot(a + ba*s - ro, rd);
    if (tr > 0.0 && tr < t) {
      float d = length(ro + rd*tr - (a + ba*s));
      float px = tr*2.0/(uRozmer.y*uOhnisko);
      c = mix(c, vec3(0.012)*uExpo + vec3(0.06,0.045,0.03)*uZarovka*uExpo, smoothstep(0.0045 + px, 0.0025, d));
    }
  }
  // televize sviti do tmy i bez zarovky
  {
    float k = dot(vec3(-1.20,1.15,1.30) - ro, rd);
    if (k > 0.0) {
      float d = length(ro + rd*k - vec3(-1.20,1.15,1.30));
      c += vec3(0.25,0.38,0.62) * uTv * exp(-d*d*9.0) * 0.06;
    }
  }
  c = vec3(1.0) - exp(-c*1.4);
  // sedy filmovy tonovy posun: stiny do modra, svetla do tepla
  float l = dot(c, vec3(0.299,0.587,0.114));
  c = mix(vec3(l), c, 0.80);
  c *= mix(vec3(0.92,0.97,1.08), vec3(1.05,1.0,0.93), smoothstep(0.05, 0.6, l));
  gl_FragColor = vec4(c, 1.0);
}
)";

// ---------- SCENA 2: CERNE VALICI SE MRAKY ----------
static const char *FS_NEBE = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform float uCas;
uniform float uTma;        // 0..1 atmosfera houstne
uniform float uPohled;     // nahoru/dolu
uniform float uVitr;
uniform float uSvit;       // svit za obzorem
uniform vec4 uBlesk;       // x,y (uv), sila, polomer
uniform float uExpo;
)" "%SUM%" R"(
void main(){
  float asp = uRozmer.x/uRozmer.y;
  vec2 q = (vUV-0.5)*vec2(asp,1.0);
  vec3 rd = normalize(vec3(q.x, q.y + uPohled, 1.0));
  float hz = rd.y;
  // obloha za mraky: zatazeno, u obzoru svit
  vec3 svit = mix(vec3(0.42,0.38,0.35), vec3(0.20,0.20,0.23), uTma) * uSvit;
  vec3 zenit = mix(vec3(0.16,0.165,0.18), vec3(0.055,0.058,0.068), uTma);
  vec3 c = mix(svit, zenit, smoothstep(-0.01, 0.40, hz));
  vec2 bq = (vUV - uBlesk.xy)*vec2(asp,1.0);
  float bl = uBlesk.z * exp(-dot(bq,bq)/(uBlesk.w*uBlesk.w));
  if (hz > 0.004) {
    float t = 1.0/hz;
    vec2 p = rd.xz*t;
    float v = uCas*(0.22 + 0.60*uVitr);
    // horni masa - vali se ke kamere a prevraci
    vec2 p1 = p*0.45 + vec2(0.0, v*0.45);
    vec2 w = vec2(fbm2(p1*0.6 + vec2(0.0, uCas*0.04)), fbm2(p1*0.6 + vec2(4.3, 2.1) - vec2(uCas*0.035, 0.0)));
    float d1 = fbm2(p1 + w*2.0);
    float d1s = fbm2(p1 + w*2.0 + vec2(0.0, 0.09));
    float pokr = mix(0.34, 0.22, uTma);
    float h1 = smoothstep(pokr, pokr + 0.22, d1);           // kde je mrak
    float tl = smoothstep(pokr, pokr + 0.50, d1);           // jak je hluboky (tmavsi spodek)
    float sv = clamp(0.5 + (d1s - d1)*7.0, 0.0, 1.0);       // okraje osvetlene od obzoru
    vec3 m1 = mix(vec3(0.30,0.30,0.32), vec3(0.030,0.031,0.036), tl);
    m1 += vec3(0.24,0.22,0.21)*sv*(1.0 - tl)*0.9;
    m1 *= mix(1.0, 0.55, uTma);
    // spodni cary - rychle, skoro cerne
    vec2 p2 = p*1.0 + vec2(uCas*0.05, v*1.4);
    float d2 = fbm2(p2 + w*0.8);
    float h2 = smoothstep(0.55, 0.80, d2) * (0.30 + 0.70*uTma);
    float mlha = exp(-t*0.09);
    c = mix(c, m1, h1*(0.35 + 0.65*mlha));
    c = mix(c, vec3(0.018,0.019,0.022), h2*(0.30 + 0.70*mlha));
    c += vec3(0.62,0.68,0.95) * bl * (0.15 + 1.3*h1*(0.5 + 0.5*(1.0 - tl)));
  } else {
    c += vec3(0.5,0.55,0.75) * bl * 0.12;
  }
  c += vec3(0.42,0.47,0.66) * uBlesk.z * 0.04;
  // krajina: kopce a koruny stromu (silueta proti svitu)
  float hor = -uPohled;
  float kop = 0.018 + 0.045*fbm2(vec2(q.x*1.1 + 3.0, 0.5));
  // koruny stromu: pulkruhy ruzne velikosti ve dvou radach, mezi lesiky pole
  float les = smoothstep(0.36, 0.58, fbm2(vec2(q.x*3.5, 2.0)));
  float kor = 0.0;
  for (int r = 0; r < 2; r++) {
    float k = r == 0 ? 24.0 : 39.0;
    float xx = q.x*k + float(r)*7.3;
    float bunka = floor(xx);
    float f = fract(xx) - 0.5;
    float rr = fract(sin(bunka*12.9898 + float(r)*4.1)*43758.5453);
    float vys = (0.009 + 0.020*rr)*step(0.18, rr);
    kor = max(kor, vys*sqrt(max(0.0, 1.0 - 3.6*f*f)));
  }
  kop += kor*les*(1.0 + 0.18*(sum2(vec2(q.x*260.0, 1.0)) - 0.5));
  float y = hor + kop - 0.035;
  float aa = 1.6/uRozmer.y;
  float zem = smoothstep(-aa, aa, y - q.y);
  vec3 tmz = vec3(0.006,0.0065,0.008) + svit*0.02 + vec3(0.3,0.33,0.45)*uBlesk.z*0.015;
  c = mix(c, tmz, zem);
  c *= uExpo;
  gl_FragColor = vec4(c, 1.0);
}
)";

// dest (pres cely obraz, alfa)
static const char *FS_DEST = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform float uCas;
uniform float uSila;
)" "%SUM%" R"(
float vrstva(vec2 uv, float m, float rych, float seed){
  vec2 p = uv*vec2(m*uRozmer.x/uRozmer.y, m*0.42);
  p.y += uCas*rych;
  p.x += p.y*0.10;
  vec2 b = floor(p);
  float r = sum2(b*vec2(1.0,7.0) + seed*13.0);
  vec2 f = fract(p);
  float car = smoothstep(0.07, 0.0, abs(f.x - 0.5 - (r-0.5)*0.6));
  float del = smoothstep(0.0, 0.25, f.y)*smoothstep(1.0, 0.55, f.y);
  return car * del * step(0.60, r);
}
void main(){
  float d = vrstva(vUV, 26.0, 2.3, 1.0)*0.55 + vrstva(vUV, 48.0, 3.4, 2.0)*0.35 + vrstva(vUV, 90.0, 5.0, 3.0)*0.22;
  float a = d * uSila * 0.5;
  gl_FragColor = vec4(vec3(0.60,0.64,0.72)*a, a*0.5);
}
)";

// ptak (silueta) - obdelnik s tvarem kridel
static const char *FS_PTAK = R"(
varying vec2 vUV;
uniform float uMav;
uniform float uAlfa;
void main(){
  vec2 q = vUV*2.0-1.0;
  float k = 0.15 + 0.55*sin(uMav);
  float x = abs(q.x);
  float y = k*x - 0.35*x*x*(k+0.3);
  float d = abs(q.y - y) - 0.11*(1.0 - x) - 0.02;
  float a = smoothstep(0.06, 0.0, d) * step(x, 1.0) * uAlfa;
  gl_FragColor = vec4(vec3(0.012)*a, a);
}
)";

// pas svetla - blesk a energie (2D v NDC, pricita se)
static const char *VS_PAS = R"(
attribute vec2 aPos;
attribute vec2 aUV;
attribute vec4 aM;
varying vec2 vUV;
varying vec4 vM;
void main(){ vUV = aUV; vM = aM; gl_Position = vec4(aPos, 0.0, 1.0); }
)";
static const char *FS_PAS = R"(
varying vec2 vUV;
varying vec4 vM;      // x sila, y ostrost jadra, z sila halo, w barva (0 blesk .. 1 zlata)
uniform vec3 uJadro;
uniform vec3 uHalo;
uniform vec3 uHalo2;
void main(){
  float v = vUV.y;
  float jadro = exp(-v*v*vM.y);
  float halo = exp(-v*v*3.0) * vM.z;
  vec3 h = mix(uHalo, uHalo2, vM.w);
  vec3 c = uJadro*jadro + h*halo;
  gl_FragColor = vec4(c*vM.x, 0.0);
}
)";

// ---------- 3D: Atari 130XE, stul, mobily ----------
static const char *VS_MODEL = R"(
attribute vec3 aPos;
attribute vec3 aN;
attribute vec2 aUV;
attribute vec4 aM;
uniform mat4 uMVP;
uniform mat4 uModel;
varying vec3 vP;
varying vec3 vN;
varying vec2 vUV;
varying vec4 vM;
void main(){
  vec4 w = uModel*vec4(aPos,1.0);
  vP = w.xyz;
  vN = (uModel*vec4(aN,0.0)).xyz;
  vUV = aUV; vM = aM;
  gl_Position = uMVP*vec4(aPos,1.0);
}
)";
static const char *FS_MODEL = R"(
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
uniform vec3 uEnergiePos;
uniform vec3 uEnergie;
uniform float uLed;
uniform float uCas;
uniform float uExpo;
uniform float uObrJas;
uniform sampler2D uAtlas;
uniform sampler2D uObr;
)" "%SUM%" R"(
void main(){
  int m = int(vM.x + 0.5);
  vec3 N = normalize(vN);
  vec3 V = normalize(uKamera - vP);
  vec3 alb = vec3(0.70,0.69,0.665);
  float spec = 0.16, shin = 24.0, rimK = 1.0, odraz = 0.0;
  vec3 emis = vec3(0.0);
  if (m == 0) {
    alb *= 0.965 + 0.035*sum2(vP.xz*2.5);
  } else if (m == 1) {
    alb = vec3(0.26,0.26,0.255);          // deska mezi klavesami (ve stinu klaves)
    spec = 0.05;
  } else if (m == 2) {
    alb = vec3(0.68,0.665,0.63)*(0.38 + 0.62*vUV.y);   // bok klavesy - dole zastineny
  } else if (m == 3) {
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(vec3(0.83,0.81,0.76), a.rgb, a.a*0.92);
    spec = 0.28; shin = 34.0;
  } else if (m == 4) {
    // sikma zebra (vetrani) - vzor z polohy na desce (x, z puvodni plochy v cm)
    vec2 p = vUV;
    float r = p.x - (p.y - 6.0)*1.333;
    float okraj = smoothstep(-13.4, -12.8, r) * smoothstep(15.8, 15.2, r);
    float w = 6.2831853*(-0.6*p.x + 0.8*p.y)/0.82;
    float sn = sin(w);
    N = normalize(N - vec3(-0.6, 0.0, -0.8)*cos(w)*0.9*okraj);
    alb *= 1.0 - okraj*0.5*smoothstep(0.25, -0.85, sn);
  } else if (m == 5) {
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(alb, a.rgb, a.a);
    spec = 0.25;
  } else if (m == 6) {
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(vec3(0.56,0.555,0.54), a.rgb, a.a*0.9);
  } else if (m == 7) {
    alb = vec3(0.22,0.03,0.02);
    emis = vec3(1.0,0.10,0.05)*uLed*2.4;
    spec = 0.6;
  } else if (m == 8) {
    // stul - orech pod lakem
    vec2 p = vP.xz;
    float vl = sum2(vec2(p.x*0.035, p.y*0.6)) + 0.5*sum2(vec2(p.x*0.09, p.y*1.7));
    float leta = sin(vl*22.0 + p.y*0.9)*0.5 + 0.5;
    float jemne = sum2(vec2(p.x*0.6, p.y*9.0));
    alb = mix(vec3(0.11,0.055,0.028), vec3(0.34,0.18,0.085), leta*0.65 + 0.35*jemne);
    alb *= 0.85 + 0.3*smoothstep(0.3, 0.9, sum2(vec2(p.x*0.02, p.y*0.25)));
    spec = 0.55; shin = 70.0; rimK = 0.0; odraz = 0.55;
    // kontaktni stin pod pocitacem a mobily
    vec2 e = vP.xz/vec2(20.5,13.0);
    float st = smoothstep(1.30, 0.80, length(e));
    alb *= 1.0 - 0.75*st;
    spec *= 1.0 - 0.8*st;
  } else if (m == 9) {
    alb = vec3(0.035,0.038,0.042);
    spec = vM.y > 0.5 ? 1.1 : 0.7; shin = vM.y > 0.5 ? 90.0 : 48.0;
    if (vM.y > 1.5) alb = vec3(0.14,0.145,0.15);
    odraz = vM.y > 0.5 && vM.y < 1.5 ? 0.25 : 0.0;
  } else if (m == 10) {
    alb = vec3(0.0);
    emis = texture2D(uObr, vUV).rgb * uObrJas;
    spec = 1.2; shin = 120.0; odraz = 0.15;
  } else if (m == 11) {
    // stena s oknem v dalce
    rimK = 0.0;
    alb = vec3(0.07,0.065,0.06)*(0.8 + 0.4*sum2(vP.xy*0.05));
    vec2 o = (vP.xy - vec2(-22.0, 40.0))/vec2(30.0, 36.0);
    if (abs(o.x) < 1.0 && abs(o.y) < 1.0) {
      float ramy = step(0.05, abs(o.x)) * step(0.04, abs(o.y)) * step(abs(o.x), 0.94) * step(abs(o.y), 0.95);
      vec3 venku = vec3(0.045,0.055,0.085) * (1.0 + 0.8*(1.0 - abs(o.y)));
      // rozmazana svetla mesta v dalce
      vec2 g = vP.xy*vec2(0.16, 0.20); vec2 gi = floor(g); vec2 gf = fract(g) - 0.5;
      float r = fract(sin(dot(gi, vec2(12.9898,78.233)))*43758.5453);
      float r2 = fract(r*91.7);
      vec2 posun = vec2(fract(r*13.1), fract(r*29.3)) - 0.5;
      float kr = smoothstep(0.22 + 0.12*r2, 0.04, length(gf - posun*0.5)) * step(0.62, r) * smoothstep(0.15, -0.6, o.y);
      venku += mix(vec3(1.0,0.62,0.28), vec3(0.55,0.75,1.0), step(0.5, r2)) * kr * 0.20;
      float dest = sum2(vec2(vP.x*1.6, vP.y*0.12 + uCas*9.0));
      venku *= 0.75 + 0.5*dest;
      venku *= 1.0 + uBlesk*14.0;
      emis = venku * ramy;
      alb = vec3(0.015)*(1.0 - ramy) + alb*0.2;
    }
  }
  vec3 c = alb*uAmb;
  // lampa - reflektor zleva shora, kuzel miri na pocitac
  vec3 L1 = uLampaPos - vP; float d1 = length(L1); L1 /= d1;
  float kuzel = smoothstep(0.80, 0.965, dot(-L1, uLampaSmer));
  float a1 = (0.06 + 0.94*kuzel)/(1.0 + d1*d1*0.00008);
  c += alb*uLampa*max(dot(N,L1),0.0)*a1;
  c += uLampa*spec*pow(max(dot(N, normalize(L1+V)),0.0), shin)*a1;
  // okno (studene, za blesku silne)
  vec3 L2 = uOknoPos - vP; float d2 = length(L2); L2 /= d2;
  vec3 ok = uOkno*(1.0 + uBlesk*7.0);
  c += alb*ok*max(dot(N,L2),0.0);
  c += ok*spec*pow(max(dot(N, normalize(L2+V)),0.0), shin*1.4);
  float rim = pow(1.0 - max(dot(N,V),0.0), 3.0);
  c += ok*rim*0.30*rimK;
  // odraz okna v laku stolu / skle displeje
  if (odraz > 0.0) {
    vec3 R = reflect(-V, N);
    float ko = max(dot(R, L2), 0.0);
    c += ok*odraz*(pow(ko, 40.0)*2.2 + pow(ko, 6.0)*0.25);
    c += uLampa*odraz*pow(max(dot(R, L1), 0.0), 200.0)*a1*3.0;
  }
  // energie z Atari (scena 4)
  vec3 L3 = uEnergiePos - vP; float d3 = length(L3); L3 /= d3;
  c += (alb + vec3(0.15))*uEnergie*max(dot(N,L3),0.0)/(1.0 + d3*d3*0.0015);
  c += emis;
  c = vec3(1.0) - exp(-c*uExpo);
  gl_FragColor = vec4(c, 1.0);
}
)";

// svetlo lampy v prasnem vzduchu (kuzel), pricita se pres 3D scenu.
// Paprsek konci na stole, na stene nebo na pocitaci (krabice).
static const char *FS_PARA = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform vec3 uKamera;
uniform vec3 uFw;
uniform vec3 uRt;
uniform vec3 uUp;
uniform float uTan;
uniform vec3 uLampaPos;
uniform vec3 uLampaSmer;
uniform vec3 uLampa;
uniform float uSila;
uniform float uCas;
uniform vec3 uOknoPos;
uniform vec3 uOkno;
uniform float uBlesk;
)" "%SUM%" R"(
vec2 krabice(vec3 ro, vec3 rd, vec3 a, vec3 b){
  vec3 inv = 1.0/rd;
  vec3 t0 = (a-ro)*inv, t1 = (b-ro)*inv;
  vec3 mn = min(t0,t1), mx = max(t0,t1);
  return vec2(max(max(mn.x,mn.y),mn.z), min(min(mx.x,mx.y),mx.z));
}
void main(){
  vec2 q = vUV*2.0 - 1.0;
  vec3 rd = normalize(uFw + uRt*q.x*uTan*uRozmer.x/uRozmer.y + uUp*q.y*uTan) + vec3(1.1e-5, 1.3e-5, 1.7e-5);
  vec3 ro = uKamera;
  float tm = 500.0;
  if (rd.y < 0.0) tm = min(tm, -ro.y/rd.y);
  if (rd.z < 0.0) tm = min(tm, (-95.0 - ro.z)/rd.z);
  vec2 h = krabice(ro, rd, vec3(-18.8, 0.0, -11.6), vec3(18.8, 6.3, 11.6));
  if (h.x < h.y && h.x > 0.0) tm = min(tm, h.x);
  float dt = tm/14.0;
  float j = texture2D(uSum, gl_FragCoord.xy/256.0).z;
  float s = 0.0, s2 = 0.0;
  for (int i = 0; i < 14; i++) {
    vec3 p = ro + rd*(dt*(float(i) + j));
    vec3 L = p - uLampaPos; float d = length(L);
    float kuzel = smoothstep(0.80, 0.97, dot(L/d, uLampaSmer));
    float prach = 0.55 + 0.9*sum3(p*0.06 + vec3(0.0, uCas*0.08, uCas*0.03));
    s += kuzel/(1.0 + d*d*0.00015)*prach;
    // studene svetlo z okna (hlavne pri blesku)
    vec3 Lo = p - uOknoPos;
    float ko = smoothstep(0.86, 0.98, dot(normalize(Lo), normalize(vec3(0.18, -0.35, 1.0))));
    s2 += ko*prach;
  }
  vec3 c = uLampa*s*dt*0.0016*uSila + uOkno*(0.25 + uBlesk*6.0)*s2*dt*0.0012*uSila;
  gl_FragColor = vec4(c, 0.0);
}
)";

// pozadi 3D sceny (tmava mistnost, kdyz kamera kouka mimo stenu)
static const char *FS_MISTNOST = R"(
varying vec2 vUV;
uniform float uBlesk;
uniform float uSila;
void main(){
  float g = smoothstep(1.1, 0.0, length((vUV-vec2(0.5,0.35))*vec2(1.0,1.3)));
  vec3 c = mix(vec3(0.003,0.003,0.005), vec3(0.03,0.022,0.016), g) * uSila;
  c += vec3(0.25,0.3,0.42)*uBlesk*0.10*(1.0-vUV.y*0.5);
  gl_FragColor = vec4(c, 1.0);
}
)";

// castice (prach v garazi, zlaty prach u loga) - GL_POINTS
static const char *VS_CASTICE = R"(
attribute vec2 aPos;
attribute vec2 aUV;      // x velikost v bodech, y alfa
varying float vA;
void main(){ vA = aUV.y; gl_PointSize = aUV.x; gl_Position = vec4(aPos,0.0,1.0); }
)";
static const char *FS_CASTICE = R"(
varying float vA;
uniform vec3 uBarva;
void main(){
  vec2 q = gl_PointCoord*2.0-1.0;
  float a = exp(-dot(q,q)*3.0)*vA;
  gl_FragColor = vec4(uBarva*a, 0.0);
}
)";

// ---------- LOGO N&P ----------
static const char *FS_LOGO = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform float uOdhal;      // 0..1 jak moc je videt
uniform float uPas;        // poloha odlesku
uniform float uCas;
void main(){
  vec4 c = texture2D(uTex, vUV);
  vec2 q = vUV - 0.5;
  float r = length(q)*2.0;
  float maska = smoothstep(1.02, 0.80, r);
  float zlato = clamp((c.r - c.b)*2.6, 0.0, 1.0) * smoothstep(0.98, 0.84, r);
  float x = (vUV.x + (1.0-vUV.y)*0.55);
  float pas = exp(-(x - uPas)*(x - uPas)*55.0) * zlato;
  float tep = 0.94 + 0.06*sin(uCas*2.1);
  vec3 col = c.rgb * uOdhal * tep * (0.55 + 0.45*maska) + vec3(1.0,0.92,0.68)*pas*1.35*uOdhal;
  float a = c.a * maska * clamp(uOdhal*1.4, 0.0, 1.0);
  gl_FragColor = vec4(col*a, a);
}
)";

// ---------- OBRAZOVKY MOBILU A STRIHY ----------
// ATARI: text BASICu (pismo z ROM, vysazene v C++) na modre obrazovce,
// po RUN duha (meni se barva pozadi po radcich, jak to dela POKE 710)
static const char *FS_ATARI = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform sampler2D uText;
uniform float uCas;
uniform float uDuha;
uniform float uJas;
vec3 hsv(float h){ vec3 k = mod(h*6.0 + vec3(5.0,3.0,1.0), 6.0); return clamp(min(k, 4.0-k), 0.0, 1.0); }
void main(){
  float asp = uRozmer.x/uRozmer.y;
  vec2 p = vUV - (asp > 1.3333 ? vec2(0.5, 0.45) : vec2(0.5));
  vec2 roz = asp > 1.3333 ? vec2(0.66*(4.0/3.0)/asp, 0.66) : vec2(0.92, 0.92*asp*(3.0/4.0));
  vec2 s = p/roz + 0.5;
  float radek = floor(vUV.y*uRozmer.y/3.0);
  vec3 okraj = mix(vec3(0.05,0.12,0.33), hsv(fract(radek*0.012 - uCas*0.35)), uDuha);
  vec3 c = okraj;
  if (s.x > 0.0 && s.x < 1.0 && s.y > 0.0 && s.y < 1.0) {
    vec4 t = texture2D(uText, vec2(s.x, 1.0 - s.y));
    vec3 poz = mix(vec3(0.10,0.27,0.65), hsv(fract(s.y*0.9 - uCas*0.25))*0.75, uDuha);
    c = mix(poz, vec3(0.80,0.88,1.0), t.a);
  }
  c *= 0.86 + 0.14*sin(gl_FragCoord.y*3.14159*0.5);
  gl_FragColor = vec4(c*uJas, 1.0);
}
)";

// SEGA: 16bitova zavodni honicka - zapad slunce, hory, palmy, silnice, auto
static const char *FS_SEGA = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform float uCas;
uniform float uJas;
)" "%SUM%" R"(
float obd(vec2 p, vec2 a, vec2 b){ return step(a.x, p.x)*step(p.x, b.x)*step(a.y, p.y)*step(p.y, b.y); }
void main(){
  float asp = uRozmer.x/uRozmer.y;
  vec2 res = asp > 1.0 ? vec2(320.0, 320.0/asp) : vec2(224.0, 224.0/asp);
  vec2 uv = (floor(vUV*res) + 0.5)/res;
  float x = (uv.x - 0.5)*asp;
  float obzor = asp > 1.0 ? 0.50 : 0.55;
  float zat = sin(uCas*0.55)*0.55;                 // zataceni silnice
  vec3 c;
  if (uv.y > obzor) {
    float h = uv.y - obzor;
    c = mix(vec3(1.0,0.62,0.28), vec3(0.78,0.22,0.42), smoothstep(0.0, 0.16, h));
    c = mix(c, vec3(0.16,0.07,0.36), smoothstep(0.12, 0.40, h));
    // slunce s pruhy
    vec2 sp = vec2(x + zat*0.10, h - 0.10);
    float r = length(sp);
    if (r < 0.13) {
      vec3 sc = mix(vec3(1.0,0.92,0.40), vec3(1.0,0.38,0.42), smoothstep(0.10, -0.10, sp.y));
      float mez = sp.y < 0.03 ? step(0.32 + (0.03 - sp.y)*2.6, fract(sp.y*44.0)) : 0.0;
      c = mix(sc, c, mez);
    }
    // hvezdy
    float hv = step(0.985, sum2(floor(uv*res)*0.91 + 3.0)) * smoothstep(0.25, 0.4, h);
    c += vec3(0.8)*hv*0.6;
    // hory - dve vrstvy
    float xo = x + zat*0.25;
    float m1 = 0.075*(1.0 - abs(fract(xo*1.6 + 0.3)*2.0 - 1.0)) + 0.03*sum2(vec2(xo*9.0, 1.0));
    if (h < m1) c = vec3(0.30,0.16,0.42);
    float m2 = 0.045*(1.0 - abs(fract(xo*2.7 + 0.7)*2.0 - 1.0)) + 0.02*sum2(vec2(xo*17.0, 5.0));
    if (h < m2) c = vec3(0.14,0.08,0.26);
  } else {
    float h = obzor - uv.y;
    float z = 0.28/max(h, 0.001);
    float zz = z + uCas*14.0;
    float stred = zat*(1.0 - h/obzor)*(1.0 - h/obzor)*0.9;
    float sir = 2.1*h + 0.004;
    float dx = abs(x - stred);
    float pruh = mod(floor(zz*0.45), 2.0);
    c = mix(vec3(0.20,0.62,0.30), vec3(0.15,0.52,0.24), pruh);              // trava
    if (dx < sir*1.13) c = mix(vec3(0.90,0.12,0.12), vec3(0.95,0.95,0.95), pruh);   // obrubnik
    if (dx < sir) {
      c = mix(vec3(0.42,0.42,0.46), vec3(0.38,0.38,0.42), pruh);            // asfalt
      if (dx < sir*0.025 + 0.002 && mod(floor(zz*0.45), 2.0) < 0.5) c = vec3(0.95);
    }
    c *= 0.55 + 0.45*smoothstep(0.0, 0.15, h);
    // palmy u silnice (priblizuji se)
    for (int i = 0; i < 6; i++) {
      float zs = 4.0 + float(i)*5.0 - mod(uCas*14.0, 5.0);
      float hs = 0.28/zs;
      float yb = obzor - hs;
      float sc = hs/obzor;
      float strana = mod(float(i), 2.0) < 0.5 ? -1.0 : 1.0;
      float xc = zat*(1.0 - hs/obzor)*(1.0 - hs/obzor)*0.9 + strana*(2.1*hs*1.55 + 0.01);
      vec2 lp = vec2((x - xc)/max(sc, 0.001), (uv.y - yb)/max(sc, 0.001));
      if (lp.y > 0.0 && lp.y < 1.25 && abs(lp.x) < 0.8) {
        float kmen = step(abs(lp.x - lp.y*lp.y*0.12*strana), 0.035)*step(lp.y, 0.95);
        vec2 kv = lp - vec2(0.11*strana, 0.97);
        float a = atan(kv.y, kv.x);
        float lis = step(length(kv), 0.42*(0.55 + 0.45*abs(sin(a*3.5))))*step(-0.30, kv.y + kv.x*kv.x*0.6);
        if (kmen > 0.5) c = vec3(0.35,0.20,0.10);
        if (lis > 0.5) c = mix(vec3(0.05,0.35,0.15), vec3(0.10,0.50,0.20), step(0.0, kv.y));
      }
    }
  }
  // auto (zezadu) dole uprostred
  vec2 ap = vec2(x + sin(uCas*0.55)*0.05, uv.y);
  float s = asp > 1.0 ? 1.0 : 0.85;
  vec2 a = (ap - vec2(0.0, 0.07))/s;
  if (obd(a, vec2(-0.16, 0.0), vec2(0.16, 0.075)) > 0.5) c = vec3(0.85,0.08,0.10);
  if (obd(a, vec2(-0.11, 0.075), vec2(0.11, 0.125)) > 0.5) c = vec3(0.70,0.05,0.08);
  if (obd(a, vec2(-0.085, 0.082), vec2(0.085, 0.118)) > 0.5) c = vec3(0.20,0.35,0.55);
  if (obd(a, vec2(-0.15, 0.035), vec2(-0.09, 0.055)) > 0.5 || obd(a, vec2(0.09, 0.035), vec2(0.15, 0.055)) > 0.5)
    c = vec3(1.0,0.85,0.2)*(0.8 + 0.2*step(0.5, fract(uCas*4.0)));
  if (obd(a, vec2(-0.17, -0.025), vec2(-0.10, 0.012)) > 0.5 || obd(a, vec2(0.10, -0.025), vec2(0.17, 0.012)) > 0.5)
    c = vec3(0.05);
  c = floor(c*7.0 + 0.5)/7.0;              // 512 barev jako Mega Drive
  gl_FragColor = vec4(c*uJas, 1.0);
}
)";

// PS1: plosne stinovany krystal nad mrizkou, ditheriing a 15bit barvy
static const char *FS_PS1 = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform float uCas;
uniform float uJas;
float oktaedr(vec3 p){ p = abs(p); return (p.x+p.y+p.z-1.0)*0.57735; }
mat2 rot(float a){ float c=cos(a), s=sin(a); return mat2(c,-s,s,c); }
float mapa(vec3 p){
  vec3 q = p - vec3(0.0, 0.62, 0.0);
  q.xz = rot(uCas*1.1)*q.xz;
  q.xy = rot(0.35)*q.xy;
  return oktaedr(q*vec3(1.0,0.62,1.0)/0.55)*0.55*0.62;
}
void main(){
  float asp = uRozmer.x/uRozmer.y;
  vec2 res = vec2(320.0, 320.0/asp);
  vec2 uv = (floor(vUV*res)+0.5)/res;
  vec2 q = (uv-0.5)*vec2(asp,1.0);
  float f = asp < 1.0 ? 0.78 : 1.12;
  vec3 ro = vec3(0.0, 0.9, -3.4);
  vec3 rd = normalize(vec3(q.x, q.y - 0.06, f));
  float t = 0.0; float hit = 0.0;
  for (int i = 0; i < 48; i++) {
    float d = mapa(ro + rd*t);
    if (d < 0.002) { hit = 1.0; break; }
    t += d;
    if (t > 9.0) break;
  }
  vec3 c;
  if (hit > 0.5) {
    vec3 p = ro + rd*t;
    vec2 e = vec2(0.003, 0.0);
    vec3 n = normalize(vec3(mapa(p+e.xyy)-mapa(p-e.xyy), mapa(p+e.yxy)-mapa(p-e.yxy), mapa(p+e.yyx)-mapa(p-e.yyx)));
    float sv = 0.30 + 0.70*max(dot(n, normalize(vec3(0.5,0.8,-0.6))), 0.0);
    c = mix(vec3(0.20,0.30,1.0), vec3(1.0,0.30,0.70), 0.5 + 0.5*n.y) * sv;
    c += vec3(1.0)*pow(max(dot(reflect(rd, n), normalize(vec3(0.5,0.8,-0.6))), 0.0), 24.0)*0.6;
  } else if (rd.y < -0.02) {
    float k = -ro.y/rd.y;
    vec2 g = (ro + rd*k).xz + vec2(0.0, uCas*1.4);
    vec2 fg = abs(fract(g) - 0.5);
    float m = smoothstep(0.44, 0.48, max(fg.x, fg.y));
    c = mix(vec3(0.04,0.03,0.10), vec3(0.15,0.80,0.95), m) * exp(-k*0.10);
    // odlesk krystalu v podlaze
    vec3 pp = ro + rd*k;
    c += vec3(0.6,0.3,0.9)*exp(-dot(pp.xz, pp.xz)*1.8)*0.35;
  } else {
    c = mix(vec3(0.10,0.04,0.20), vec3(0.01,0.01,0.05), smoothstep(0.0, 0.5, rd.y));
    float hv = step(0.992, fract(sin(dot(floor(uv*res), vec2(12.9898,78.233)))*43758.5453));
    c += vec3(0.7)*hv*smoothstep(0.05, 0.3, rd.y);
  }
  // ordered dither + 5 bitu na slozku
  vec2 dp = mod(floor(vUV*res), 2.0);
  float dth = (dp.x*2.0 + dp.y - 1.5)/4.0;
  c = floor(c*31.0 + 0.5 + dth)/31.0;
  gl_FragColor = vec4(c*uJas, 1.0);
}
)";

// telefon v rade (5. strih): cerny ram s leskem, uvnitr zivy displej
static const char *FS_TELEFON = R"(
varying vec2 vUV;
uniform sampler2D uTex;
uniform vec2 uVel;      // velikost telefonu v bodech
uniform float uAlfa;
float rrect(vec2 p, vec2 b, float r){ vec2 q = abs(p)-b+r; return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r; }
void main(){
  vec2 p = (vUV - 0.5)*uVel;
  float r = uVel.x*0.13;
  float d = rrect(p, uVel*0.5, r);
  float a = 1.0 - smoothstep(-1.0, 0.5, d);
  vec3 c = vec3(0.025,0.025,0.03);
  c += vec3(0.75,0.62,0.40)*smoothstep(-2.2, -0.6, d)*(0.35 + 0.25*vUV.y);
  vec2 m = vec2(uVel.x*0.05, uVel.y*0.045);
  float ds = rrect(p, uVel*0.5 - m, r*0.55);
  if (ds < 0.0) {
    vec2 uv = (p + uVel*0.5 - m)/(uVel - 2.0*m);
    c = texture2D(uTex, uv).rgb*1.1;
    c += vec3(1.0)*0.07*smoothstep(0.18, 0.0, abs(uv.x*0.8 + uv.y - 1.15));
  }
  gl_FragColor = vec4(c, 1.0)*a*uAlfa;
}
)";

// MENU N&P - zlaty ramecek; texty a logo se kresli navrch
static const char *FS_MENU = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform float uCas;
uniform float uJas;
void main(){
  vec2 p = vUV*uRozmer;
  vec3 c = mix(vec3(0.02,0.015,0.01), vec3(0.09,0.06,0.025), smoothstep(1.0, 0.0, vUV.y));
  for (int i = 0; i < 3; i++) {
    float y0 = uRozmer.y*(0.10 + float(i)*0.17), y1 = y0 + uRozmer.y*0.13;
    float x0 = uRozmer.x*0.08, x1 = uRozmer.x*0.92;
    if (p.x > x0 && p.x < x1 && p.y > y0 && p.y < y1) {
      float okr = min(min(p.x-x0, x1-p.x), min(p.y-y0, y1-p.y));
      c = mix(vec3(0.06,0.045,0.025), vec3(0.85,0.66,0.30), smoothstep(3.0, 1.0, okr));
      c += vec3(0.25,0.18,0.06)*0.3*(1.0-(p.y-y0)/(y1-y0));
    }
  }
  float zar = sin(vUV.y*30.0 - uCas*3.0)*0.5+0.5;
  c += vec3(0.6,0.45,0.15)*0.04*zar;
  gl_FragColor = vec4(c*uJas, 1.0);
}
)";

// TV na LAN - mobil posila obraz Atari do televize (vlny jako Wi-Fi)
static const char *FS_TVLAN = R"(
varying vec2 vUV;
uniform vec2 uRozmer;
uniform sampler2D uText;
uniform float uCas;
vec3 hsv(float h){ vec3 k = mod(h*6.0 + vec3(5.0,3.0,1.0), 6.0); return clamp(min(k, 4.0-k), 0.0, 1.0); }
float rrect(vec2 p, vec2 b, float r){ vec2 q = abs(p)-b+r; return length(max(q,0.0)) + min(max(q.x,q.y),0.0) - r; }
void main(){
  float asp = uRozmer.x/uRozmer.y;
  vec2 p = (vUV-0.5)*vec2(asp,1.0);
  float px = 1.0/uRozmer.y;
  vec3 c = mix(vec3(0.012,0.012,0.018), vec3(0.05,0.04,0.035), smoothstep(-0.5, 0.5, p.y));
  bool sir = asp > 1.0;
  // televize 16:9
  vec2 tvS = sir ? vec2(0.20, -0.01) : vec2(0.0, 0.17);
  float tw = sir ? 0.37 : min(0.46, asp*0.47);
  vec2 tvB = vec2(tw, tw*9.0/16.0);
  float dt = rrect(p - tvS, tvB, 0.012);
  float dsc = rrect(p - tvS, tvB - vec2(0.012), 0.004);
  c += vec3(0.20,0.30,0.55)*0.10*exp(-max(dt, 0.0)*18.0);      // svit kolem TV
  // noha
  vec2 np = p - (tvS + vec2(0.0, -tvB.y - 0.03));
  if (rrect(np, vec2(0.012, 0.03), 0.004) < 0.0 || rrect(np + vec2(0.0, 0.03), vec2(tw*0.32, 0.006), 0.004) < 0.0) c = vec3(0.10,0.10,0.11);
  if (dt < 0.0) c = vec3(0.035,0.035,0.04);
  if (dsc < 0.0) {
    vec2 s = (p - tvS + tvB - vec2(0.012))/(2.0*(tvB - vec2(0.012)));
    vec2 roz = vec2(0.80*(3.0/4.0)*(16.0/9.0), 0.80);
    vec2 a = (s - 0.5)/roz + 0.5;
    float radek = floor(s.y*120.0);
    vec3 okr = hsv(fract(radek*0.02 - uCas*0.35));
    c = okr;
    if (a.x > 0.0 && a.x < 1.0 && a.y > 0.0 && a.y < 1.0) {
      vec4 t = texture2D(uText, vec2(a.x, 1.0 - a.y));
      vec3 poz = hsv(fract(a.y*0.9 - uCas*0.25))*0.72;
      c = mix(poz, vec3(0.85,0.9,1.0), t.a);
    }
    c *= 0.92 + 0.08*sin(gl_FragCoord.y*1.5708);
  }
  // mobil
  vec2 mS = sir ? vec2(-0.47, -0.10) : vec2(0.0, -0.27);
  vec2 mB = vec2(0.055, 0.112);
  float dm = rrect(p - mS, mB, 0.016);
  if (dm < 0.0) {
    c = vec3(0.02);
    float ds = rrect(p - mS, mB - vec2(0.007), 0.010);
    if (ds < 0.0) {
      vec2 s = (p - mS)/mB;
      c = mix(vec3(0.10,0.07,0.03), vec3(0.45,0.32,0.12), smoothstep(-1.0, 1.0, s.y));
      float kr = abs(length(s - vec2(0.0, 0.25)) - 0.45);
      c += vec3(1.0,0.8,0.4)*smoothstep(0.08, 0.0, kr)*0.6;
    }
  }
  c += vec3(0.85,0.65,0.30)*0.5*smoothstep(2.0*px, 0.0, abs(dm))*0.6;
  // vlny od mobilu k TV
  vec2 cil = tvS - vec2(0.0, sir ? 0.0 : tvB.y);
  vec2 sm = normalize(cil - mS);
  vec2 d = p - mS;
  float r = length(d);
  float smer = dot(d/max(r, 1e-4), sm);
  float del = length(cil - mS);
  float vl = abs(fract(r*9.0 - uCas*2.4) - 0.5);
  float vlna = 1.0 - smoothstep(1.2*px*9.0, 2.8*px*9.0, vl);
  float maska = smoothstep(0.86, 0.96, smer) * smoothstep(del*0.80, del*0.55, r) * smoothstep(mB.y + 0.02, mB.y + 0.06, r);
  c += vec3(1.0,0.80,0.40) * vlna * maska * 0.9;
  gl_FragColor = vec4(c, 1.0);
}
)";

}  // namespace napdemo
