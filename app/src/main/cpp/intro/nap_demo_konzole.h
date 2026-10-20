// nap_demo_konzole.h
// ====================================================================
//  B307: KONZOLE N&P 16 A N&P 32 A STARY CRT MONITOR (3D modely)
//  B313: konzole uplne znovu - vic detailu, pohyblive dily, potisky, lepsi
//  materialy a svetlo, animace (Rene: "vic detailu na tele, kardrydz s nalepkou,
//  oteviraci viko na CD, kontrolky, lepsi materialy a svetlo, animace
//  (zasunuti kazety, roztoceni CD) v menu i v introdukci").
//
//  Vlastni navrh (zadna kopie cizich vyrobku, zadna cizi loga):
//   - N&P 16: dvoupatrova skrin v barve pulnocni modre s metalizou, zlaty pasek
//     kolem dokola, kartacovana hlinikova deska vpredu, vzadu vysoka "vez"
//     se sterbinou na cartridge (zlaty ramecek, dve gumove klapky, ktere se pri
//     zasunuti otevrou dovnitr). Cartridge s potistenym stitkem (vymyslena hra
//     STAR RUNNER), zlatymi kontakty a ryhami na uchop. Vpredu potisteny panel:
//     posuvny vypinac, RESET, kontrolky POWER (zelena) a CART (oranzova, blika
//     pri zasunuti), dve zdirky ovladacu se zlatymi ramecky. Vetraci drazky,
//     sroubky, gumove nozicky.
//   - N&P 32: grafitova skrin, leskla cerna deska, vpravo ctvercove viko na
//     panty s kulatym okenkem z kouroveho skla (pod nim je videt tocici se disk),
//     chromovy ramecek okenka a azurovy svitici prstenec. Pod vikem mechanika
//     (hridel, laser na kolejnici). Disk s potiskem N&P 32 (pri otaceni se
//     potisk rozmaze). Vpredu cerny stit s potiskem: tlacitko POWER se svitici
//     prstencem, OPEN, rada 4 kontrolek, dve zdirky ovladacu a dve sterbiny na
//     pametove karty, azurovy pas dole.
//   - Na obou zlaty emblem N&P (oficialni logo z appky).
//   - Animace (AnimKonzole, menu i intro): N&P 16 - cartridge sjede shora do
//     sterbiny, klapky se otevrou, cvaknuti, rozsviti se POWER a CART zablika;
//     N&P 32 - disk sjede do otevrene mechaniky, viko se zavre, rozsviti se
//     POWER a kontrolky, disk se roztoci (potisk se rozmaze).
//   - CRT monitor: tmava skrin s hlubokou zadni casti, zaobleny ramecek,
//     vypoukla obrazovka (zvlast sit se souradnicemi pro obraz).
//  Jednotky cm, X doprava, Y nahoru, predek k +Z (ke kamere).
//
//  Material (aM.x) - shader FS_KONZOLE, barva v aM.yzw:
//   0 lak s metalizou   1 leskly plast   2 kartacovany kov   3 svetlo (uSvit)
//   4 emblem N&P   5 potisk (atlas)   6 disk - spodni duhova strana
//   7 tmava sterbina / mrizka   8 plast CRT   9 kourove sklo (pruhledne)
//   10 guma   11 chrom / zlato (barva = odstin kovu)   12 grafit (mat, zrno)
//   13 potisk disku (atlas, rozmazani pri otaceni)   14 potisk na kartacovanem kovu
//   20-27 kontrolky (uLedA, uLedB)
// ====================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>
#include "nap_demo_gl.h"
#include "nap_demo_text.h"
#include "../menu/nap_menu_platno.h"

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
uniform float uSvit;         // svetelne pasy (material 3)
uniform float uLogoLesk;     // odlesk pres emblem (poloha pasu)
uniform sampler2D uLogo;
uniform sampler2D uAtlas;    // B313: potisky (stitek cartridge, panel, disk)
uniform vec4 uLedA;          // B313: jas kontrolek 20..23
uniform vec4 uLedB;          // B313: jas kontrolek 24..27
uniform vec4 uDiskAtlas;     // B313: potisk disku v atlasu (u0, v0, du, dv)
uniform float uRozmaz;       // B313: rozmazani potisku disku pri otaceni (rad)
uniform float uPodlaha;      // B313: vyska podlahy ve svete (zastineni u podlahy)
uniform float uStudio;       // B313: sila studiovych odlesku (softboxy)
)" "%SUM%" R"(
vec3 duha(float h){ vec3 k = mod(h*6.0 + vec3(5.0,3.0,1.0), 6.0); return clamp(min(k, 4.0-k), 0.0, 1.0); }
// studio: velky mekky softbox nahore vlevo, uzky svetelny pruh vpravo, horizont, tmava podlaha
vec3 studio(vec3 R){
  float box = smoothstep(0.30, 0.78, R.y)*(0.45 + 0.55*smoothstep(0.15, -0.70, R.x));
  float pruh = exp(-(R.x - 0.80)*(R.x - 0.80)*45.0)*smoothstep(-0.15, 0.35, R.y);
  float hor = exp(-R.y*R.y*28.0);
  return vec3(1.0,0.95,0.88)*box*1.15 + vec3(0.70,0.82,1.0)*pruh*0.75 + vec3(0.42,0.38,0.34)*hor*0.35;
}
void main(){
  float dOdhal = uOdhal - vP.y;
  if (dOdhal < 0.0) discard;
  int m = int(vM.x + 0.5);
  vec3 N = normalize(vN);
  vec3 V = normalize(uKamera - vP);
  if (dot(N, V) < 0.0) N = -N;        // tenke dily (klapky, disk) z obou stran
  vec3 alb = vM.yzw;
  float spec = 0.35, shin = 40.0, odraz = 0.12, lesk = 0.0, alfa = 1.0;
  vec3 emis = vec3(0.0);
  vec3 kov = vec3(1.0);               // barva studiovych odlesku (kov = vlastni barva)
  if (m == 0) {
    // lak s jemnou metalizou (trpyt) a cirym lakem navrchu
    float tr = step(0.93, sum2(vP.xz*9.0 + vP.y*3.0));
    spec = 0.55 + tr*0.9; shin = 60.0; odraz = 0.20; lesk = 0.40;
  } else if (m == 1) {
    spec = 1.1; shin = 110.0; odraz = 0.45; lesk = 0.60;
  } else if (m == 2) {
    // kartacovany kov: ryhy podel x
    float ry = sum2(vec2(vP.x*0.4, vP.z*38.0));
    alb *= 0.82 + 0.3*ry;
    spec = 0.40; shin = 30.0; odraz = 0.14; lesk = 0.30; kov = alb*1.6 + 0.15;
  } else if (m == 3) {
    emis = alb*uSvit*2.2;
    alb *= 0.15;
    spec = 0.5; shin = 60.0;
  } else if (m == 4) {
    vec4 l = texture2D(uLogo, vUV);
    float zl = clamp((l.r - l.b)*2.6, 0.0, 1.0);
    alb = mix(alb, l.rgb*1.1, l.a);
    spec = mix(spec, 0.45, l.a); shin = mix(shin, 90.0, l.a); odraz = mix(odraz, 0.18, l.a); lesk = 0.20*l.a;
    kov = mix(vec3(1.0), vec3(1.0,0.80,0.45), zl);
    float x = vUV.x + (1.0 - vUV.y)*0.6;
    emis = vec3(1.0,0.86,0.55)*exp(-(x - uLogoLesk)*(x - uLogoLesk)*40.0)*zl*0.9 + l.rgb*zl*0.10*uSvit;
  } else if (m == 5) {
    // potisk z atlasu (stitek, panel) - lakovany papir / plast
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(alb, a.rgb, a.a);
    spec = 0.6; shin = 70.0; odraz = 0.22; lesk = 0.30;
  } else if (m == 6) {
    // disk CD (spodni strana): duhove lomy podle uhlu a pohledu, stredova dira
    vec2 q = vUV - 0.5;
    float r = length(q)*2.0;
    if (r < 0.125) discard;
    float a = atan(q.y, q.x);
    vec3 R = reflect(-V, N);
    float h = fract(a*0.159 + dot(R, vec3(0.3,0.5,0.2))*1.3 + r*0.6);
    alb = mix(vec3(0.75,0.77,0.80), duha(h), 0.55*smoothstep(0.35, 0.5, r));
    if (r < 0.32) alb = vec3(0.55,0.57,0.62);
    spec = 1.3; shin = 80.0; odraz = 0.6; lesk = 0.8;
    emis = duha(h)*0.06*uSvit;
  } else if (m == 7) {
    spec = 0.05; shin = 8.0; odraz = 0.0;
  } else if (m == 8) {
    alb *= 0.95 + 0.08*sum2(vP.xy*4.0);
    spec = 0.18; shin = 18.0; odraz = 0.05;
  } else if (m == 9) {
    // kourove sklo: tmavy nadech + odlesky (pruhledne, kresli se posledni)
    alb = vec3(0.010,0.013,0.018);
    spec = 1.6; shin = 180.0; odraz = 0.9; lesk = 1.0;
    alfa = 0.42 + 0.45*pow(1.0 - max(dot(N, V), 0.0), 3.0);
  } else if (m == 10) {
    // guma: mat, jemne zrno
    alb *= 0.85 + 0.25*sum2(vP.xz*30.0 + vP.y*7.0);
    spec = 0.10; shin = 12.0; odraz = 0.0;
  } else if (m == 11) {
    // chrom / zlato: skoro jen odrazy (barva kovu z vrcholu)
    kov = alb;
    alb *= 0.30;
    spec = 1.6; shin = 160.0; odraz = 1.0; lesk = 1.0;
  } else if (m == 12) {
    // grafit: matny plast s jemnym zrnem
    alb *= 0.88 + 0.16*sum2(vP.xz*22.0 + vP.y*5.0);
    spec = 0.28; shin = 24.0; odraz = 0.07; lesk = 0.12;
  } else if (m == 13) {
    // potisk disku: pri otaceni se rozmaze po obvodu (5 vzorku)
    vec2 q = vUV - 0.5;
    float r = length(q)*2.0;
    if (r < 0.125) discard;
    vec3 s = vec3(0.0);
    for (int i = 0; i < 5; i++) {
      float u = (float(i) - 2.0)*0.25*uRozmaz;
      float cu = cos(u), su = sin(u);
      vec2 qq = vec2(q.x*cu - q.y*su, q.x*su + q.y*cu) + 0.5;
      s += texture2D(uAtlas, uDiskAtlas.xy + qq*uDiskAtlas.zw).rgb;
    }
    alb = s*0.2;
    if (r < 0.30) alb = vec3(0.50,0.52,0.56);                 // pruhledny stred disku
    spec = 0.8; shin = 70.0; odraz = 0.30; lesk = 0.45;
  } else if (m == 14) {
    // potisk na kartacovanem kovu (ryhy zustanou i pod barvou)
    float ry = sum2(vec2(vP.x*0.4, vP.z*38.0));
    alb *= 0.82 + 0.3*ry;
    vec4 a = texture2D(uAtlas, vUV);
    alb = mix(alb, a.rgb, a.a);
    spec = 0.55; shin = 36.0; odraz = 0.18; lesk = 0.30 - 0.15*a.a; kov = mix(alb*1.6 + 0.15, vec3(1.0), a.a);
  } else if (m >= 20) {
    // kontrolka: sviti podle uLedA / uLedB
    float l = m == 20 ? uLedA.x : (m == 21 ? uLedA.y : (m == 22 ? uLedA.z : (m == 23 ? uLedA.w :
              (m == 24 ? uLedB.x : (m == 25 ? uLedB.y : (m == 26 ? uLedB.z : uLedB.w))))));
    emis = alb*l*2.8;
    alb = alb*0.10 + vec3(0.03);
    spec = 0.9; shin = 90.0; odraz = 0.2; lesk = 0.35;
  }
  // zastineni u podlahy (konzole "sedi", ne plave)
  float ao = mix(0.45, 1.0, smoothstep(uPodlaha, uPodlaha + 2.5, vP.y));
  vec3 c = alb*uAmb*ao;
  // lampa (reflektor zleva shora - siroky kuzel, at dosvitne na celou trojici)
  vec3 L1 = uLampaPos - vP; float d1 = length(L1); L1 /= d1;
  float kuzel = smoothstep(0.70, 0.95, dot(-L1, uLampaSmer));
  float a1 = (0.10 + 0.90*kuzel)/(1.0 + d1*d1*0.00008);
  c += alb*uLampa*max(dot(N,L1),0.0)*a1;
  c += uLampa*spec*pow(max(dot(N, normalize(L1+V)),0.0), shin)*a1*kov;
  // okno
  vec3 L2 = uOknoPos - vP; float d2 = length(L2); L2 /= d2;
  vec3 ok = uOkno*(1.0 + uBlesk*7.0);
  c += alb*ok*max(dot(N,L2),0.0)*ao;
  c += ok*spec*pow(max(dot(N, normalize(L2+V)),0.0), shin*1.4)*kov;
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
  vec3 R = reflect(-V, N);
  if (odraz > 0.0) {
    float ko = max(dot(R, L2), 0.0);
    c += ok*odraz*(pow(ko, 40.0)*2.2 + pow(ko, 6.0)*0.25)*kov;
    c += uLampa*odraz*pow(max(dot(R, L1), 0.0), 200.0)*a1*3.0*kov;
    c += (n1*pow(max(dot(R, L3), 0.0), 30.0) + n2*pow(max(dot(R, L4), 0.0), 30.0))*odraz*2.0;
  }
  // B313: studiove odlesky (softboxy) - silnejsi pri pohledu z boku (Fresnel)
  if (lesk > 0.0) {
    float fr = 0.06 + 0.94*pow(1.0 - max(dot(N,V),0.0), 5.0);
    c += studio(R)*uStudio*lesk*mix(0.35, 1.0, fr)*kov*ao;
  }
  c += emis;
  // zhmotneni: svitici pas na hranici
  float pas = 1.0 - smoothstep(0.0, 1.4, dOdhal);
  c += uOdhalBarva*pas*pas*2.5;
  c = vec3(1.0) - exp(-c*uExpo);
  gl_FragColor = vec4(c*alfa, alfa);
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
  // obrys posunuty dovnitr o "zk" (podel osy uhlu) - horni plocha zkoseneho hranolu
  static std::vector<V2> odsad(const std::vector<V2> &o, float zk) {
    const size_t n = o.size();
    std::vector<V2> in(n);
    for (size_t i = 0; i < n; i++) {
      const V2 &p0 = o[(i + n - 1) % n], &p1 = o[i], &p2 = o[(i + 1) % n];
      V2 e1{p1.x - p0.x, p1.y - p0.y}, e2{p2.x - p1.x, p2.y - p1.y};
      float l1 = std::sqrt(e1.x * e1.x + e1.y * e1.y), l2 = std::sqrt(e2.x * e2.x + e2.y * e2.y);
      l1 = std::max(l1, 1e-6f); l2 = std::max(l2, 1e-6f);
      V2 n1{-e1.y / l1, e1.x / l1}, n2{-e2.y / l2, e2.x / l2};   // vnejsi normaly v rovine (x, z)
      V2 s{n1.x + n2.x, n1.y + n2.y};
      float ls = std::sqrt(s.x * s.x + s.y * s.y);
      float k = zk / std::max(0.3f, (s.x * n1.x + s.y * n1.y) / std::max(ls, 1e-4f));
      in[i] = {p1.x - s.x / std::max(ls, 1e-4f) * k, p1.y - s.y / std::max(ls, 1e-4f) * k};
    }
    return in;
  }
  // hranol z obrysu (proti smeru hodin pri pohledu shora, x-z), vyska y0..y1,
  // horni hrana zkosena o "zk" (dovnitr i dolu); vrch = false: bez horni plochy
  void hranol(const std::vector<V2> &o, float y0, float y1, float zk, float matBok, const V3 &bBok, float matVrch, const V3 &bVrch,
              float matHrana, const V3 &bHrana, bool vrch = true) {
    const size_t n = o.size();
    V2 c{0, 0};
    for (auto &p : o) { c.x += p.x / n; c.y += p.y / n; }
    const std::vector<V2> in = odsad(o, zk);
    for (size_t i = 0; i < n; i++) {
      const V2 &a = o[i], &b = o[(i + 1) % n];
      const V2 &ai = in[i], &bi = in[(i + 1) % n];
      // bok
      quad(V3(a.x, y0, a.y), V3(b.x, y0, b.y), V3(b.x, y1 - zk, b.y), V3(a.x, y1 - zk, a.y), matBok, bBok);
      // zkosena hrana
      if (zk > 0.f) quad(V3(a.x, y1 - zk, a.y), V3(b.x, y1 - zk, b.y), V3(bi.x, y1, bi.y), V3(ai.x, y1, ai.y), matHrana, bHrana);
    }
    if (!vrch) return;
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

  // ---------------- B313 ----------------
  // potisk z atlasu: a (vlevo dole), b (vpravo dole), c (vpravo nahore), d (vlevo nahore)
  // pri pohledu zvenku; v atlasu (A x A bodu) obdelnik x0,y0 (levy horni) .. x1,y1
  void potisk(const V3 &a, const V3 &b, const V3 &c, const V3 &d, float x0, float y0, float x1, float y1, float A, const V3 &podklad, float mat = 5) {
    quad(a, b, c, d, mat, podklad, {x0 / A, y1 / A}, {x1 / A, y1 / A}, {x1 / A, y0 / A}, {x0 / A, y0 / A});
  }
  // kde paprsek ze stredu c pod uhlem th protne obrys (konvexni, obsahuje c)
  static V2 paprsek(const std::vector<V2> &o, V2 c, float th) {
    const float dx = std::cos(th), dz = std::sin(th);
    float nej = 1e30f;
    for (size_t i = 0; i < o.size(); i++) {
      const V2 &a = o[i], &b = o[(i + 1) % o.size()];
      const float ex = b.x - a.x, ez = b.y - a.y;
      const float den = dx * ez - dz * ex;
      if (std::fabs(den) < 1e-9f) continue;
      const float px = a.x - c.x, pz = a.y - c.y;
      const float t = (px * ez - pz * ex) / den;
      const float s = (px * dz - pz * dx) / den;
      if (t > 1e-6f && s >= -1e-4f && s <= 1.f + 1e-4f && t < nej) nej = t;
    }
    if (nej > 1e29f) nej = 0.f;
    return V2{c.x + dx * nej, c.y + dz * nej};
  }
  // vodorovna plocha ve vysce y mezi vnejsim a vnitrnim obrysem (oba konvexni, kolem c):
  // deska s dirou (kruhovou i obdelnikovou). ny = +1 nahoru, -1 dolu
  void deskaMezi(const std::vector<V2> &ven, const std::vector<V2> &dov, V2 c, float y, float ny, float mat, const V3 &barva) {
    std::vector<float> uhly;
    for (int i = 0; i < 48; i++) uhly.push_back(6.2831853f * i / 48.f);
    auto pridej = [&](const std::vector<V2> &o) {
      for (const V2 &p : o) {
        float a = std::atan2(p.y - c.y, p.x - c.x);
        if (a < 0.f) a += 6.2831853f;
        uhly.push_back(a);
      }
    };
    pridej(ven);
    pridej(dov);
    std::sort(uhly.begin(), uhly.end());
    std::vector<float> u;
    for (float a : uhly) if (u.empty() || a - u.back() > 1e-5f) u.push_back(a);
    const V3 n(0, ny, 0);
    for (size_t i = 0; i < u.size(); i++) {
      const float a0 = u[i], a1 = (i + 1 < u.size()) ? u[i + 1] : u[0] + 6.2831853f;
      const V2 i0 = paprsek(dov, c, a0), i1 = paprsek(dov, c, a1), o0 = paprsek(ven, c, a0), o1 = paprsek(ven, c, a1);
      const V3 A(i0.x, y, i0.y), B(o0.x, y, o0.y), C(o1.x, y, o1.y), D(i1.x, y, i1.y);
      tri(A, B, C, n, {0, 0}, {0, 0}, {0, 0}, mat, barva);
      tri(A, C, D, n, {0, 0}, {0, 0}, {0, 0}, mat, barva);
    }
  }
  // svisla stena kruhu (polomer r, y0..y1): dovnitr = normaly do stredu (dira), jinak ven (okraj disku)
  void stenaKruhu(float cx, float cz, float r, float y0, float y1, int seg, bool dovnitr, float mat, const V3 &barva) {
    for (int i = 0; i < seg; i++) {
      const float a0 = 6.2831853f * i / seg, a1 = 6.2831853f * (i + 1) / seg, am = (a0 + a1) * 0.5f;
      const V3 n = V3(std::cos(am), 0, std::sin(am)) * (dovnitr ? -1.f : 1.f);
      const V3 p0(cx + std::cos(a0) * r, y0, cz + std::sin(a0) * r), p1(cx + std::cos(a1) * r, y0, cz + std::sin(a1) * r);
      const V3 q0(p0.x, y1, p0.z), q1(p1.x, y1, p1.z);
      tri(p0, p1, q1, n, {0, 0}, {0, 0}, {0, 0}, mat, barva);
      tri(p0, q1, q0, n, {0, 0}, {0, 0}, {0, 0}, mat, barva);
    }
  }
  // vodorovny kotouc s uv 0..1 (u = +x, v = +z: horni okraj obrazku je vzadu), normala +-y
  void diskUV(float cx, float y, float cz, float r, int seg, float ny, float mat, const V3 &barva) {
    const V3 n(0, ny, 0), st(cx, y, cz);
    for (int i = 0; i < seg; i++) {
      const float a0 = 6.2831853f * i / seg, a1 = 6.2831853f * (i + 1) / seg;
      const V3 p0(cx + std::cos(a0) * r, y, cz + std::sin(a0) * r), p1(cx + std::cos(a1) * r, y, cz + std::sin(a1) * r);
      tri(st, p0, p1, n, {0.5f, 0.5f}, {0.5f + 0.5f * std::cos(a0), 0.5f + 0.5f * std::sin(a0)}, {0.5f + 0.5f * std::cos(a1), 0.5f + 0.5f * std::sin(a1)},
          mat, barva);
    }
  }
};

// obdelnik se zkosenymi rohy jako obrys (x-z), proti smeru hodin shora
inline std::vector<V2> obrysObd(float x0, float z0, float x1, float z1, float roh) {
  // shora (osa y k divakovi): x doprava, z dolu (k divakovi). Proti smeru hodin.
  return {{x0 + roh, z1}, {x1 - roh, z1}, {x1, z1 - roh}, {x1, z0 + roh}, {x1 - roh, z0}, {x0 + roh, z0}, {x0, z0 + roh}, {x0, z1 - roh}};
}
// B313: obdelnik se zaoblenymi rohy (seg useku na roh), stejne poradi jako obrysObd
inline std::vector<V2> obrysZaobleny(float x0, float z0, float x1, float z1, float r, int seg) {
  std::vector<V2> o;
  const V2 st[4] = {{x1 - r, z1 - r}, {x1 - r, z0 + r}, {x0 + r, z0 + r}, {x0 + r, z1 - r}};
  for (int k = 0; k < 4; k++) {
    const float f0 = 1.5707963f - k * 1.5707963f;
    for (int i = 0; i <= seg; i++) {
      const float f = f0 - 1.5707963f * i / seg;
      o.push_back({st[k].x + r * std::cos(f), st[k].y + r * std::sin(f)});
    }
  }
  return o;
}
// B313: kruh jako obrys (stejny smer obehu jako obrysObd)
inline std::vector<V2> obrysKruh(float cx, float cz, float r, int seg) {
  std::vector<V2> o;
  for (int i = 0; i < seg; i++) {
    const float f = -6.2831853f * i / seg;
    o.push_back({cx + r * std::cos(f), cz + r * std::sin(f)});
  }
  return o;
}

inline float hladce01(float x) { x = std::min(1.f, std::max(0.f, x)); return x * x * (3.f - 2.f * x); }

// ====================================================================
//  B313: STAV POHYBLIVYCH DILU A KONTROLEK (menu i intro)
// ====================================================================
struct AnimKonzole {
  // N&P 16
  float kazetaY = -5.f;       // spodek cartridge nad horni plochou veze (cm; zaporne = ve sterbine)
  float kazetaNaklon = 0.f;   // naklon cartridge kolem osy x (rad)
  float klapky = 1.f;         // klapky sterbiny 0 zavrene .. 1 otevrene
  float posuvnik = 1.f;       // vypinac 0 vlevo (vypnuto) .. 1 vpravo (zapnuto)
  // N&P 32
  float viko = 0.f;           // 0 zavrene .. 1 otevrene
  float diskY = 0.f;          // disk nad hrideli (cm)
  float diskNaklon = 0.f;     // naklon padajiciho disku (rad)
  float uhelDisku = 0.f;      // natoceni disku (rad)
  float rozmaz = 0.f;         // rozmazani potisku disku (rad)
  // kontrolky 20..27 (jas 0..1)
  float led[8] = {0, 0, 0, 0, 0, 0, 0, 0};
};

// ====================================================================
//  B313: POTISKY (atlas 512 x 512 kresleny na CPU pres napmenu::Platno)
// ====================================================================
namespace potisk {
using napmenu::Barva;
using napmenu::Platno;
constexpr int A = 512;
// N&P 16 - kde co v atlasu je (bodu: x0, y0, x1, y1)
constexpr float ST16[4] = {0, 0, 384, 224};       // stitek cartridge vpredu
constexpr float HR16[4] = {0, 224, 384, 256};     // stitek na horni hrane cartridge
constexpr float PN16[4] = {0, 256, 512, 320};     // panel vpredu
constexpr float ZD16[4] = {384, 0, 512, 96};      // stitek cartridge vzadu
constexpr float DK16[4] = {384, 96, 512, 160};    // potisk na kartacovane desce
// N&P 32
constexpr float DS32[4] = {0, 0, 384, 384};       // potisk disku (kruh)
constexpr float PN32[4] = {0, 384, 512, 448};     // cerny stit vpredu
constexpr float DK32[4] = {0, 448, 512, 512};     // napis na desce vlevo vzadu

// panel N&P 16: x -10.6..10.6 cm, y 0.80..3.00 cm
constexpr float P16X0 = -10.6f, P16X1 = 10.6f, P16Y0 = 0.80f, P16Y1 = 3.00f;
// stit N&P 32: x -11.9..11.9 cm, y 1.30..3.50 cm
constexpr float P32X0 = -11.9f, P32X1 = 11.9f, P32Y0 = 1.30f, P32Y1 = 3.50f;

inline std::vector<unsigned char> atlas16(const Pismo *bold, const Pismo *semi, const Pismo *ital) {
  Platno pl(A, A);
  const Barva cerna(0.035f, 0.035f, 0.045f), bila(0.95f, 0.95f, 0.97f), zlato(1.0f, 0.80f, 0.36f), seda(0.62f, 0.64f, 0.70f);
  // ---------- stitek cartridge (384 x 224): synthwave zapad slunce, vymyslena hra STAR RUNNER ----------
  pl.obd(0, 0, 384, 224, 9, cerna);
  if (ital) pl.text(*ital, "N&P", 14, 33, 31, zlato, 1, 0.f);
  if (semi) pl.text(*semi, "16-BIT  CARTRIDGE", 370, 29, 15, bila, 2, 0.10f);
  const float ox0 = 10, oy0 = 44, ox1 = 374, hor = 146, oy1 = 198;
  pl.obdPrechod(ox0, oy0, ox1, hor, 3, Barva(0.16f, 0.04f, 0.34f), Barva(0.99f, 0.40f, 0.38f));
  // slunce s vodorovnymi prouzky dole
  {
    const float cx = 252, cy = 120, r = 46;
    pl.tvar(cx - r, cy - r, cx + r, cy + r, [=](float x, float y) { return Platno::sdfKruh(x, y, cx, cy, r); },
            [=](float, float y) {
              const float k = (y - (cy - r)) / (2 * r);
              Barva b = napmenu::mixB(Barva(1.0f, 0.93f, 0.35f), Barva(1.0f, 0.36f, 0.48f), k);
              if (y > cy - 6) {
                const float d = (y - (cy - 6)) / (r + 6);
                const float f = std::fmod(y - (cy - 6), 9.f) / 9.f;
                if (f < 0.12f + d * 0.55f) b.a = 0.f;
              }
              return b;
            });
  }
  // hory (tmave fialove siluety)
  pl.tvar(ox0, 90, ox1, hor, [=](float x, float y) {
            const float h = hor - (10.f + 22.f * std::fabs(std::sin(x * 0.019f + 0.4f)) + 12.f * std::fabs(std::sin(x * 0.061f + 1.3f)) +
                                   5.f * std::sin(x * 0.17f));
            return std::max(h - y, y - hor);
          },
          [=](float, float y) { return napmenu::mixB(Barva(0.30f, 0.07f, 0.40f), Barva(0.10f, 0.02f, 0.18f), (y - 100.f) / 46.f); });
  // podlaha s perspektivni mrizkou
  pl.obd(ox0, hor, ox1, oy1, 2, Barva(0.04f, 0.015f, 0.10f));
  for (int k = 0; k < 7; k++) {
    const float y = hor + 2.f + (oy1 - hor) * (k * k) / 36.f;
    pl.cara(ox0, y, ox1, y, 1.3f, Barva(1.0f, 0.25f, 0.85f, 0.85f));
  }
  for (int k = -9; k <= 9; k++) pl.cara(192.f + k * 8.f, hor, 192.f + k * 52.f, oy1, 1.2f, Barva(1.0f, 0.25f, 0.85f, 0.75f));
  pl.obd(ox0, hor - 1.f, ox1, hor + 1.5f, 0, Barva(1.0f, 0.55f, 0.85f, 0.9f));
  // nazev hry
  if (ital) {
    pl.text(*ital, "STAR RUNNER", 112, 102, 40, Barva(0.05f, 0.02f, 0.12f, 0.85f), 0, 0.02f, 200);
    pl.text(*ital, "STAR RUNNER", 110, 99, 40, Barva(1.0f, 0.88f, 0.42f), 0, 0.02f, 200);
  }
  if (semi) pl.text(*semi, u8"N&P VISION  ·  1-2 PLAYERS  ·  16 BIT", 192, 216, 12, seda, 0, 0.12f);
  // ---------- horni hrana cartridge (384 x 32) ----------
  pl.obd(0, 224, 384, 256, 4, cerna);
  if (ital) pl.text(*ital, "STAR RUNNER", 12, 247, 19, zlato, 1, 0.02f);
  if (semi) pl.text(*semi, "N&P 16", 372, 246, 17, bila, 2, 0.08f);
  // ---------- panel vpredu (512 x 64): x -10.6..10.6, y 0.8..3.0 cm ----------
  {
    auto X = [](float x) { return (x - P16X0) / (P16X1 - P16X0) * 512.f; };
    auto Y = [](float y) { return 256.f + (P16Y1 - y) / (P16Y1 - P16Y0) * 64.f; };
    pl.obd(0, 256, 512, 320, 0, Barva(0.025f, 0.025f, 0.035f));
    pl.obd(0, 257, 512, 259, 0, Barva(0.20f, 0.21f, 0.26f));                  // jemna hrana nahore
    // drazka vypinace + popisky
    pl.obd(X(-9.75f), Y(2.55f), X(-7.35f), Y(2.05f), 3, Barva(0.005f, 0.005f, 0.008f));
    if (semi) {
      pl.text(*semi, "POWER", X(-8.55f), Y(1.12f), 10, seda, 0, 0.10f);
      pl.text(*semi, "RESET", X(-6.2f), Y(1.12f), 10, seda, 0, 0.10f);
      pl.text(*semi, "ON", X(-4.05f), Y(2.48f), 9, seda, 1, 0.10f);
      pl.text(*semi, "CART", X(-4.05f), Y(1.58f), 9, seda, 1, 0.10f);
      pl.text(*semi, "16-BIT ENTERTAINMENT SYSTEM", X(0.6f), Y(1.12f), 9, seda, 0, 0.14f);
    }
    if (ital) pl.text(*ital, "N&P 16", X(0.6f), Y(1.62f), 31, Barva(0.86f, 0.87f, 0.92f), 0, 0.04f);
    // zdirky ovladacu: tmavy lichobeznik s 9 zlatymi kolicky
    for (int i = 0; i < 2; i++) {
      const float cx = i == 0 ? 5.6f : 8.7f, cy = 1.95f;
      const float xa = X(cx - 1.15f), xb = X(cx + 1.15f), ya = Y(cy + 0.52f), yb = Y(cy - 0.52f);
      const float sk = (xb - xa) * 0.10f;
      pl.tvar(xa, ya, xb, yb, [=](float x, float y) {
                const float t = (y - ya) / (yb - ya);
                const float l = xa + sk * t, r = xb - sk * t;
                return std::max(std::max(l - x, x - r), std::max(ya - y, y - yb)) - 0.f;
              },
              [=](float, float) { return Barva(0.008f, 0.008f, 0.010f); });
      for (int k = 0; k < 9; k++) {
        const bool horni = k < 5;
        const float px = horni ? xa + (xb - xa) * (0.18f + 0.16f * k) : xa + (xb - xa) * (0.26f + 0.16f * (k - 5));
        const float py = horni ? ya + (yb - ya) * 0.32f : ya + (yb - ya) * 0.70f;
        pl.kruh(px, py, 1.9f, Barva(0.95f, 0.72f, 0.30f));
      }
      if (bold) pl.text(*bold, i == 0 ? "1" : "2", (xa + xb) * 0.5f, Y(cy + 0.62f), 9, seda, 0, 0.f);
    }
  }
  // ---------- stitek vzadu (128 x 96) ----------
  pl.obd(384, 0, 512, 96, 5, Barva(0.80f, 0.80f, 0.82f));
  if (bold) pl.text(*bold, "STAR RUNNER", 448, 22, 13, Barva(0.10f, 0.10f, 0.14f), 0, 0.04f);
  if (semi) {
    pl.text(*semi, "N&P 16 CARTRIDGE", 448, 40, 9, Barva(0.20f, 0.20f, 0.25f), 0, 0.08f);
    pl.text(*semi, "FOR N&P 16 CONSOLE", 448, 56, 8, Barva(0.25f, 0.25f, 0.30f), 0, 0.02f);
    pl.text(*semi, u8"© 2026 N&P VISION", 448, 72, 8, Barva(0.25f, 0.25f, 0.30f), 0, 0.04f);
  }
  pl.obd(392, 80, 504, 88, 2, Barva(0.10f, 0.10f, 0.14f));
  // ---------- potisk na kartacovane desce (128 x 64, pruhledny) ----------
  if (bold) pl.text(*bold, "16-BIT", 448, 132, 36, Barva(0.10f, 0.11f, 0.16f, 0.92f), 0, 0.02f, 120);
  if (semi) pl.text(*semi, "HIGH SPEED SYSTEM", 448, 152, 10, Barva(0.10f, 0.11f, 0.16f, 0.85f), 0, 0.14f, 120);
  return pl.rgba8();
}

inline std::vector<unsigned char> atlas32(const Pismo *bold, const Pismo *semi, const Pismo *ital) {
  Platno pl(A, A);
  const Barva bila(0.95f, 0.95f, 0.97f), zlato(1.0f, 0.80f, 0.36f), seda(0.60f, 0.63f, 0.70f), azur(0.35f, 0.88f, 1.0f);
  // ---------- potisk disku (384 x 384, stred 192,192, polomer 190) ----------
  {
    const float cx = 192, cy = 192, R = 190;
    pl.tvar(cx - R, cy - R, cx + R, cy + R, [=](float x, float y) { return Platno::sdfKruh(x, y, cx, cy, R); },
            [=](float x, float y) {
              const float dx = x - cx, dy = y - cy, r = std::sqrt(dx * dx + dy * dy) / R, a = std::atan2(dy, dx);
              Barva b = napmenu::mixB(Barva(0.06f, 0.10f, 0.26f), Barva(0.015f, 0.03f, 0.09f), r);
              const float vl = 0.5f + 0.5f * std::sin(a * 2.f + 0.6f);
              b = napmenu::mixB(b, napmenu::mixB(Barva(0.10f, 0.55f, 0.85f), Barva(0.70f, 0.12f, 0.75f), vl), 0.55f * hladce01((r - 0.3f) / 0.5f));
              return b;
            });
    // paprsky
    for (int k = 0; k < 14; k++) {
      const float a = k * 6.2831853f / 14.f + 0.2f;
      pl.cara(cx + std::cos(a) * 64, cy + std::sin(a) * 64, cx + std::cos(a) * 186, cy + std::sin(a) * 186, k % 2 ? 1.2f : 2.4f,
              Barva(0.6f, 0.95f, 1.0f, 0.22f));
    }
    // "polygonovy krystal" (32 bit = polygony) vlevo dole
    {
      const float px = 92, py = 262, s = 34;
      const float bx[6] = {0.f, 0.86f, 0.86f, 0.f, -0.86f, -0.86f}, by[6] = {-1.f, -0.5f, 0.5f, 1.f, 0.5f, -0.5f};
      for (int i = 0; i < 6; i++) {
        pl.cara(px + bx[i] * s, py + by[i] * s, px + bx[(i + 1) % 6] * s, py + by[(i + 1) % 6] * s, 2.2f, Barva(0.8f, 1.0f, 1.0f, 0.9f));
        pl.cara(px, py, px + bx[i] * s, py + by[i] * s, 1.3f, Barva(0.8f, 1.0f, 1.0f, 0.55f));
      }
    }
    // vnejsi prstenec
    pl.prstenec(cx, cy, R - 7, 3.f, Barva(0.75f, 0.95f, 1.0f, 0.55f));
    if (ital) pl.text(*ital, "N&P", cx, 112, 64, zlato, 0, 0.f);
    if (bold) pl.text(*bold, "32", 262, 300, 92, Barva(0.92f, 0.97f, 1.0f), 0, 0.f);
    if (semi) {
      pl.text(*semi, "SYSTEM DISC", cx, 150, 19, bila, 0, 0.16f);
      pl.text(*semi, u8"COMPACT DISC  ·  32-BIT  ·  N&P VISION", cx, 352, 11, Barva(0.80f, 0.90f, 1.0f), 0, 0.10f);
    }
  }
  // ---------- cerny stit vpredu (512 x 64): x -11.9..11.9, y 1.3..3.5 cm ----------
  {
    auto X = [](float x) { return (x - P32X0) / (P32X1 - P32X0) * 512.f; };
    auto Y = [](float y) { return 384.f + (P32Y1 - y) / (P32Y1 - P32Y0) * 64.f; };
    pl.obd(0, 384, 512, 448, 0, Barva(0.012f, 0.012f, 0.016f));
    pl.obd(0, 385, 512, 386.5f, 0, Barva(0.22f, 0.24f, 0.30f));
    // zdirky ovladacu (zaoblene obdelniky se sterbinou) a sterbiny pametovych karet
    for (int i = 0; i < 2; i++) {
      const float cx = i == 0 ? -9.8f : -6.6f;
      pl.obd(X(cx - 1.1f), Y(2.6f), X(cx + 1.1f), Y(1.7f), 5, Barva(0.004f, 0.004f, 0.006f));
      pl.obd(X(cx - 0.75f), Y(2.25f), X(cx + 0.75f), Y(2.05f), 1.5f, Barva(0.55f, 0.57f, 0.62f));
      pl.obd(X(cx - 0.95f), Y(3.13f), X(cx + 0.95f), Y(2.98f), 1, Barva(0.0f, 0.0f, 0.0f));
      if (bold) pl.text(*bold, i == 0 ? "1" : "2", X(cx), Y(1.42f), 9, seda, 0, 0.f);
    }
    if (semi) {
      pl.text(*semi, "MEMORY", X(-8.2f), Y(3.28f) + 6, 8, seda, 0, 0.16f);
      pl.text(*semi, "DISC ACCESS", X(-0.6f), Y(2.25f), 8, seda, 0, 0.14f);
      pl.text(*semi, "OPEN", X(7.3f), Y(1.62f), 9, seda, 0, 0.10f);
      pl.text(*semi, "POWER", X(9.8f), Y(1.38f), 9, seda, 0, 0.10f);
    }
    if (ital) pl.text(*ital, "N&P 32", X(3.7f), Y(1.98f), 28, Barva(0.86f, 0.88f, 0.94f), 0, 0.04f);
    pl.obd(X(1.6f), Y(1.62f), X(5.8f), Y(1.56f), 0, Barva(azur.r, azur.g, azur.b, 0.8f));
  }
  // ---------- napis na desce (512 x 64, pruhledny) ----------
  if (ital) pl.text(*ital, "N&P 32", 8, 500, 46, Barva(1.0f, 0.82f, 0.40f, 0.95f), 1, 0.02f);
  if (semi) pl.text(*semi, "32-BIT  CD  SYSTEM", 504, 496, 22, Barva(0.70f, 0.74f, 0.82f, 0.9f), 2, 0.16f);
  return pl.rgba8();
}
}  // namespace potisk

// ====================================================================
//  B313: KONZOLE N&P 16 A N&P 32 (skrin + pohyblive dily + potisky)
// ====================================================================
struct ModelKonzole {
  Sit sit;                          // skrin (nehybne)
  Sit kazeta, klapkaP, klapkaZ, posuvnik;   // N&P 16
  Sit viko, disk, sklo;             // N&P 32 (sklo okenka je pruhledne - kresli se posledni)
  GLuint atlas = 0;                 // potisky
  int bitu = 16;
  std::vector<std::pair<V3, V3>> hrany;   // obrys pro neonove pruty (v souradnicich modelu)
  struct Led { V3 p; int i; V3 barva; };
  std::vector<Led> ledy;            // kontrolky (pro zari kolem nich)
  V3 led{0, 0, 0}, emblem{0, 0, 0};
  float vyska = 8.f;                // nejvyssi bod (i s cartridge / diskem nahore) - pro zhmotneni v intru
  // klouby
  float vezY = 7.2f, sterbinaZ = -4.75f, sterbinaPul = 1.05f;   // N&P 16
  float vikoOsaY = 5.3f, vikoOsaZ = -9.6f;                       // N&P 32
  V3 diskStred{6.f, 4.51f, -2.7f};
  float diskAtlas[4] = {0, 0, 0.75f, 0.75f};
  float podlaha = 0.f;              // spodek modelu (y) - pro zastineni u podlahy

  void uvolni() {
    if (atlas) glDeleteTextures(1, &atlas);
    atlas = 0;
  }
  int trojuhelniku() const {
    return (sit.pocet + kazeta.pocet + klapkaP.pocet + klapkaZ.pocet + posuvnik.pocet + viko.pocet + disk.pocet + sklo.pocet) / 3;
  }

  static void nahrajAtlas(GLuint &tex, const std::vector<unsigned char> &px) {
    tex = novaTextura(potisk::A, potisk::A, GL_RGBA, px.data(), true);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
  }

  // ---------------- N&P 16 ----------------
  void postav16(const Pismo *bold = nullptr, const Pismo *semi = nullptr, const Pismo *ital = nullptr) {
    bitu = 16;
    StavK s;
    const V3 modra(0.030f, 0.050f, 0.150f), zlato(1.0f, 0.74f, 0.36f), kartac(0.34f, 0.36f, 0.40f), guma(0.035f, 0.035f, 0.04f),
        tma(0.012f, 0.012f, 0.016f), chrom(0.85f, 0.87f, 0.92f);
    const float X = 12.8f, Z = 9.8f;
    // gumove nozicky
    for (int i = 0; i < 4; i++) s.valec(V3(i % 2 ? 10.2f : -10.2f, 0.f, i < 2 ? 7.2f : -7.2f), 0.9f, 0.42f, 16, 10, guma, 10, guma);
    // spodni dil (modry lak), zaoblene rohy, zkosena horni hrana
    s.hranol(obrysZaobleny(-X, -Z, X, Z, 1.8f, 4), 0.4f, 3.5f, 0.45f, 0, modra, 0, modra, 0, modra);
    // zlaty pasek kolem dokola
    s.hranol(obrysZaobleny(-X + 0.30f, -Z + 0.30f, X - 0.30f, Z - 0.30f, 1.55f, 4), 3.5f, 3.66f, 0.f, 11, zlato, 11, zlato, 11, zlato);
    // horni dil (modry lak)
    s.hranol(obrysZaobleny(-X + 0.55f, -Z + 0.55f, X - 0.55f, Z - 0.55f, 1.35f, 4), 3.66f, 4.5f, 0.32f, 0, modra, 0, modra, 0, modra);
    // kartacovana hlinikova deska vpredu (0,1 cm silna) + sroubky
    s.kvadr(V3(-X + 1.7f, 4.5f, -0.9f), V3(X - 1.7f, 4.6f, Z - 1.5f), 2, kartac);
    for (int i = 0; i < 4; i++) s.valec(V3(i % 2 ? X - 2.25f : -X + 2.25f, 4.6f, i < 2 ? Z - 2.05f : 0.2f), 0.22f, 0.05f, 10, 11, chrom, 11, chrom);
    // vez se sterbinou na cartridge (vzadu)
    const float vz0 = -Z + 0.9f, vz1 = -0.6f;
    sterbinaZ = (vz0 + vz1) * 0.5f;
    vezY = 7.2f;
    const std::vector<V2> vez = obrysZaobleny(-8.6f, vz0, 8.6f, vz1, 1.2f, 3);
    s.hranol(vez, 4.5f, vezY, 0.5f, 0, modra, 0, modra, 0, modra, false);
    const float hx = 6.35f, hz = sterbinaPul;
    const std::vector<V2> dira = obrysObd(-hx, sterbinaZ - hz, hx, sterbinaZ + hz, 0.f);
    const std::vector<V2> ramec = obrysObd(-hx - 0.55f, sterbinaZ - hz - 0.5f, hx + 0.55f, sterbinaZ + hz + 0.5f, 0.25f);
    s.deskaMezi(StavK::odsad(vez, 0.5f), ramec, V2{0.f, sterbinaZ}, vezY, 1.f, 0, modra);
    // zlaty ramecek sterbiny (vystouply)
    s.deskaMezi(ramec, dira, V2{0.f, sterbinaZ}, vezY + 0.13f, 1.f, 11, zlato);
    for (size_t i = 0; i < ramec.size(); i++) {
      const V2 &a = ramec[i], &b = ramec[(i + 1) % ramec.size()];
      s.quad(V3(a.x, vezY, a.y), V3(b.x, vezY, b.y), V3(b.x, vezY + 0.13f, b.y), V3(a.x, vezY + 0.13f, a.y), 11, zlato);
    }
    // vnitrek sterbiny (tmavy) - steny a dno
    {
      const float y0 = 1.9f, y1 = vezY + 0.13f, za = sterbinaZ - hz, zb = sterbinaZ + hz;
      s.quad(V3(hx, y0, zb), V3(-hx, y0, zb), V3(-hx, y1, zb), V3(hx, y1, zb), 7, tma);      // predni stena (normala dozadu)
      s.quad(V3(-hx, y0, za), V3(hx, y0, za), V3(hx, y1, za), V3(-hx, y1, za), 7, tma);      // zadni
      s.quad(V3(-hx, y0, zb), V3(-hx, y0, za), V3(-hx, y1, za), V3(-hx, y1, zb), 7, tma);
      s.quad(V3(hx, y0, za), V3(hx, y0, zb), V3(hx, y1, zb), V3(hx, y1, za), 7, tma);
      s.quad(V3(-hx, y0, za), V3(-hx, y0, zb), V3(hx, y0, zb), V3(hx, y0, za), 7, tma);
    }
    // zlata linka vpredu na vezi
    s.quad(V3(-7.2f, 5.55f, vz1 + 0.012f), V3(7.2f, 5.55f, vz1 + 0.012f), V3(7.2f, 5.70f, vz1 + 0.012f), V3(-7.2f, 5.70f, vz1 + 0.012f), 11, zlato);
    // drazky na bocich veze
    for (int k = 0; k < 3; k++)
      for (int st = -1; st <= 1; st += 2) {
        const float y = 5.3f + k * 0.55f, x = st * 8.61f, za = vz0 + 1.4f, zb = vz1 - 1.4f;
        if (st < 0) s.quad(V3(x, y, za), V3(x, y, zb), V3(x, y + 0.14f, zb), V3(x, y + 0.14f, za), 7, tma);
        else s.quad(V3(x, y, zb), V3(x, y, za), V3(x, y + 0.14f, za), V3(x, y + 0.14f, zb), 7, tma);
      }
    // panel vpredu (potisk) a jeho prvky
    {
      const float z = Z + 0.012f;
      s.potisk(V3(potisk::P16X0, potisk::P16Y0, z), V3(potisk::P16X1, potisk::P16Y0, z), V3(potisk::P16X1, potisk::P16Y1, z),
               V3(potisk::P16X0, potisk::P16Y1, z), potisk::PN16[0], potisk::PN16[1], potisk::PN16[2], potisk::PN16[3], potisk::A, tma);
      // RESET
      s.valec(V3(-6.2f, 2.3f, Z), 0.52f, 0.10f, 18, 11, chrom, 11, chrom, true);
      s.valec(V3(-6.2f, 2.3f, Z + 0.1f), 0.40f, 0.20f, 18, 1, V3(0.05f, 0.05f, 0.07f), 1, V3(0.05f, 0.05f, 0.07f), true);
      // kontrolky POWER (zelena) a CART (oranzova)
      s.kvadr(V3(-4.75f, 2.42f, Z), V3(-4.25f, 2.64f, Z + 0.12f), 20, V3(0.25f, 1.0f, 0.35f));
      s.kvadr(V3(-4.75f, 1.52f, Z), V3(-4.25f, 1.74f, Z + 0.12f), 21, V3(1.0f, 0.55f, 0.10f));
      ledy = {{V3(-4.5f, 2.53f, Z + 0.14f), 0, V3(0.25f, 1.0f, 0.35f)}, {V3(-4.5f, 1.63f, Z + 0.14f), 1, V3(1.0f, 0.55f, 0.10f)}};
      led = ledy[0].p;
      // zlate ramecky zdirek
      for (int i = 0; i < 2; i++) {
        const float cx = i == 0 ? 5.6f : 8.7f, cy = 1.95f, w = 1.30f, h = 0.62f, t = 0.10f;
        s.kvadr(V3(cx - w, cy + h - t, Z), V3(cx + w, cy + h, Z + 0.10f), 11, zlato);
        s.kvadr(V3(cx - w, cy - h, Z), V3(cx + w, cy - h + t, Z + 0.10f), 11, zlato);
        s.kvadr(V3(cx - w, cy - h, Z), V3(cx - w + t, cy + h, Z + 0.10f), 11, zlato);
        s.kvadr(V3(cx + w - t, cy - h, Z), V3(cx + w, cy + h, Z + 0.10f), 11, zlato);
      }
    }
    // vetraci drazky na bocich
    for (int k = 0; k < 8; k++)
      for (int st = -1; st <= 1; st += 2) {
        const float z0 = -6.2f + k * 1.6f, x = st * (X + 0.012f);
        if (st < 0) s.quad(V3(x, 1.25f, z0), V3(x, 1.25f, z0 + 0.55f), V3(x, 2.75f, z0 + 0.55f), V3(x, 2.75f, z0), 7, tma);
        else s.quad(V3(x, 1.25f, z0 + 0.55f), V3(x, 1.25f, z0), V3(x, 2.75f, z0), V3(x, 2.75f, z0 + 0.55f), 7, tma);
      }
    // zlaty emblem N&P a potisk 16-BIT na kartacovane desce
    s.kotouc(V3(-7.6f, 4.612f, 4.4f), 2.25f, V3(1, 0, 0), V3(0, 0, -1), 40, 4, V3(0.06f, 0.07f, 0.12f));
    emblem = V3(-7.6f, 4.612f, 4.4f);
    s.potisk(V3(3.6f, 4.606f, 6.6f), V3(9.6f, 4.606f, 6.6f), V3(9.6f, 4.606f, 3.6f), V3(3.6f, 4.606f, 3.6f), potisk::DK16[0], potisk::DK16[1],
             potisk::DK16[2], potisk::DK16[3], potisk::A, kartac, 14);
    sit.nahraj(s.v);
    podlaha = 0.f;

    // ---- vypinac (dil: posouva se doprava, kdyz se zapne) ----
    {
      StavK p;
      p.kvadr(V3(-0.45f, -0.28f, 0.f), V3(0.45f, 0.28f, 0.32f), 11, chrom);
      for (int k = 0; k < 3; k++) p.kvadr(V3(-0.30f + k * 0.25f, -0.20f, 0.32f), V3(-0.22f + k * 0.25f, 0.20f, 0.36f), 1, V3(0.10f, 0.10f, 0.12f));
      posuvnik.nahraj(p.v);
    }
    // ---- klapky sterbiny (guma): v soustave pantu, predni smeruje dozadu (-z), zadni dopredu ----
    {
      StavK p, q;
      p.kvadr(V3(-hx + 0.08f, -0.07f, -hz + 0.03f), V3(hx - 0.08f, 0.f, 0.f), 10, V3(0.06f, 0.06f, 0.07f), true);
      q.kvadr(V3(-hx + 0.08f, -0.07f, 0.f), V3(hx - 0.08f, 0.f, hz - 0.03f), 10, V3(0.06f, 0.06f, 0.07f), true);
      klapkaP.nahraj(p.v);
      klapkaZ.nahraj(q.v);
    }
    // ---- cartridge (pocatek = stred spodni hrany) ----
    {
      StavK c;
      const V3 plast(0.075f, 0.075f, 0.085f);
      const float cw = 6.0f, cd = 0.78f, ch = 8.2f;
      c.hranol(obrysZaobleny(-cw, -cd, cw, cd, 0.22f, 2), 0.f, ch, 0.3f, 1, plast, 1, plast, 1, plast);
      c.quad(V3(-cw + 0.2f, 0.f, -cd + 0.1f), V3(cw - 0.2f, 0.f, -cd + 0.1f), V3(cw - 0.2f, 0.f, cd - 0.1f), V3(-cw + 0.2f, 0.f, cd - 0.1f), 1, plast);
      // stitek vpredu, vzadu a na horni hrane
      const float zf = cd + 0.004f;
      c.potisk(V3(-5.4f, 1.45f, zf), V3(5.4f, 1.45f, zf), V3(5.4f, 7.75f, zf), V3(-5.4f, 7.75f, zf), potisk::ST16[0], potisk::ST16[1], potisk::ST16[2],
               potisk::ST16[3], potisk::A, plast);
      c.potisk(V3(2.0f, 3.0f, -zf), V3(-2.0f, 3.0f, -zf), V3(-2.0f, 6.0f, -zf), V3(2.0f, 6.0f, -zf), potisk::ZD16[0], potisk::ZD16[1], potisk::ZD16[2],
               potisk::ZD16[3], potisk::A, plast);
      c.potisk(V3(-5.2f, ch + 0.003f, 0.42f), V3(5.2f, ch + 0.003f, 0.42f), V3(5.2f, ch + 0.003f, -0.42f), V3(-5.2f, ch + 0.003f, -0.42f), potisk::HR16[0],
               potisk::HR16[1], potisk::HR16[2], potisk::HR16[3], potisk::A, plast);
      // ryhy na uchop (boky nahore)
      for (int k = 0; k < 4; k++)
        for (int st = -1; st <= 1; st += 2) {
          const float y = 6.1f + k * 0.42f;
          if (st < 0) c.kvadr(V3(-cw - 0.10f, y, -0.5f), V3(-cw + 0.02f, y + 0.16f, 0.5f), 1, V3(0.12f, 0.12f, 0.14f));
          else c.kvadr(V3(cw - 0.02f, y, -0.5f), V3(cw + 0.10f, y + 0.16f, 0.5f), 1, V3(0.12f, 0.12f, 0.14f));
        }
      // kontakty dole (zelena deska, zlate plosky) vpredu i vzadu
      for (int st = -1; st <= 1; st += 2) {
        const float z = st * (cd + 0.006f);
        if (st > 0) c.quad(V3(-5.0f, 0.05f, z), V3(5.0f, 0.05f, z), V3(5.0f, 0.85f, z), V3(-5.0f, 0.85f, z), 7, V3(0.02f, 0.07f, 0.04f));
        else c.quad(V3(5.0f, 0.05f, z), V3(-5.0f, 0.05f, z), V3(-5.0f, 0.85f, z), V3(5.0f, 0.85f, z), 7, V3(0.02f, 0.07f, 0.04f));
        for (int k = 0; k < 14; k++) {
          const float x0 = -4.75f + k * 0.69f, z2 = st * (cd + 0.010f);
          if (st > 0) c.quad(V3(x0, 0.12f, z2), V3(x0 + 0.42f, 0.12f, z2), V3(x0 + 0.42f, 0.72f, z2), V3(x0, 0.72f, z2), 11, zlato);
          else c.quad(V3(x0 + 0.42f, 0.12f, z2), V3(x0, 0.12f, z2), V3(x0, 0.72f, z2), V3(x0 + 0.42f, 0.72f, z2), 11, zlato);
        }
      }
      kazeta.nahraj(c.v);
    }
    vyska = vezY + 3.2f + 0.4f + 8.2f + 0.3f;     // cartridge nad sterbinou (v intru se zhmotni i ona)
    // atlas potisku
    nahrajAtlas(atlas, potisk::atlas16(bold, semi, ital));
    // obrys pro pruty: spodni dil, horni dil, vez, cartridge nahore
    hrany.clear();
    auto obd = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
      V3 p[8] = {V3(x0, y0, z0), V3(x1, y0, z0), V3(x1, y0, z1), V3(x0, y0, z1), V3(x0, y1, z0), V3(x1, y1, z0), V3(x1, y1, z1), V3(x0, y1, z1)};
      static const int E[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
      for (auto &e : E) hrany.push_back({p[e[0]], p[e[1]]});
    };
    obd(-X, 0.4f, -Z, X, 3.5f, Z);
    obd(-X + 0.55f, 3.66f, -Z + 0.55f, X - 0.55f, 4.5f, Z - 0.55f);
    obd(-8.6f, 4.5f, vz0, 8.6f, vezY, vz1);
    const float yc = vezY + 3.2f;
    obd(-6.0f, yc, sterbinaZ - 0.78f, 6.0f, yc + 8.2f, sterbinaZ + 0.78f);
  }

  // ---------------- N&P 32 ----------------
  void postav32(const Pismo *bold = nullptr, const Pismo *semi = nullptr, const Pismo *ital = nullptr) {
    bitu = 32;
    StavK s;
    const V3 grafit(0.095f, 0.098f, 0.108f), cerna(0.016f, 0.016f, 0.020f), guma(0.035f, 0.035f, 0.04f), tma(0.010f, 0.010f, 0.013f),
        chrom(0.82f, 0.85f, 0.90f), azur(0.25f, 0.85f, 1.0f);
    const float X = 13.5f, Z = 10.5f;
    const float dX = 6.0f, dZ = -2.7f;       // stred mechaniky (a okenka ve viku) - kousek pred stredem vika, at disk nezavadi o viko
    for (int i = 0; i < 4; i++) s.valec(V3(i % 2 ? 10.8f : -10.8f, 0.f, i < 2 ? 7.8f : -7.8f), 0.9f, 0.42f, 16, 10, guma, 10, guma);
    // spodni dil (grafit)
    s.hranol(obrysZaobleny(-X, -Z, X, Z, 1.4f, 4), 0.4f, 4.4f, 0.5f, 12, grafit, 12, grafit, 12, grafit);
    // leskla cerna deska s dirou pro mechaniku
    const std::vector<V2> deska = obrysZaobleny(-X + 0.45f, -Z + 0.45f, X - 0.45f, Z - 0.45f, 1.0f, 4);
    s.hranol(deska, 4.4f, 4.9f, 0.22f, 1, cerna, 1, cerna, 1, cerna, false);
    s.deskaMezi(StavK::odsad(deska, 0.22f), obrysKruh(dX, dZ, 6.3f, 48), V2{dX, dZ}, 4.9f, 1.f, 1, cerna);
    // mechanika: stena, dno, hridel, laser na kolejnici
    s.stenaKruhu(dX, dZ, 6.3f, 3.9f, 4.9f, 48, true, 7, tma);
    s.diskUV(dX, 3.9f, dZ, 6.3f, 48, 1.f, 7, V3(0.02f, 0.02f, 0.025f));
    s.valec(V3(dX, 3.9f, dZ), 0.9f, 0.40f, 20, 11, chrom, 11, chrom);
    s.valec(V3(dX, 4.3f, dZ), 0.55f, 0.15f, 16, 11, chrom, 11, chrom);
    s.quad(V3(dX + 1.6f, 3.905f, dZ + 0.5f), V3(dX + 5.6f, 3.905f, dZ + 0.5f), V3(dX + 5.6f, 3.905f, dZ - 0.5f), V3(dX + 1.6f, 3.905f, dZ - 0.5f), 7,
           V3(0.004f, 0.004f, 0.006f));
    s.kvadr(V3(dX + 2.9f, 3.9f, dZ - 0.45f), V3(dX + 4.0f, 4.05f, dZ + 0.45f), 1, V3(0.08f, 0.08f, 0.09f));
    s.kotouc(V3(dX + 3.45f, 4.055f, dZ), 0.30f, V3(1, 0, 0), V3(0, 0, -1), 16, 1, V3(0.10f, 0.20f, 0.55f));
    // levy dil desky: emblem, mrizka, napis
    s.kotouc(V3(-9.4f, 4.905f, -4.9f), 2.3f, V3(1, 0, 0), V3(0, 0, -1), 40, 4, V3(0.03f, 0.03f, 0.04f));
    emblem = V3(-9.4f, 4.905f, -4.9f);
    for (int k = 0; k < 9; k++) {
      const float x0 = -11.9f + k * 1.0f;
      s.quad(V3(x0, 4.904f, 3.6f), V3(x0 + 0.45f, 4.904f, 3.6f), V3(x0 + 0.45f, 4.904f, 0.6f), V3(x0, 4.904f, 0.6f), 7, tma);
    }
    s.potisk(V3(-12.6f, 4.906f, -8.3f), V3(-3.0f, 4.906f, -8.3f), V3(-3.0f, 4.906f, -9.5f), V3(-12.6f, 4.906f, -9.5f), potisk::DK32[0], potisk::DK32[1],
             potisk::DK32[2], potisk::DK32[3], potisk::A, cerna);
    // stit vpredu (potisk) a jeho prvky
    {
      const float z = Z + 0.012f;
      s.potisk(V3(potisk::P32X0, potisk::P32Y0, z), V3(potisk::P32X1, potisk::P32Y0, z), V3(potisk::P32X1, potisk::P32Y1, z),
               V3(potisk::P32X0, potisk::P32Y1, z), potisk::PN32[0], potisk::PN32[1], potisk::PN32[2], potisk::PN32[3], potisk::A, cerna);
      // azurovy pas dole
      s.quad(V3(-X + 1.6f, 0.62f, z), V3(X - 1.6f, 0.62f, z), V3(X - 1.6f, 0.80f, z), V3(-X + 1.6f, 0.80f, z), 22, azur);
      // POWER: svitici prstenec, chromovy lem, cerne tlacitko
      s.valec(V3(9.8f, 2.4f, Z), 0.98f, 0.12f, 28, 20, azur, 20, azur, true);
      s.valec(V3(9.8f, 2.4f, Z + 0.12f), 0.82f, 0.16f, 28, 11, chrom, 11, chrom, true);
      s.valec(V3(9.8f, 2.4f, Z + 0.28f), 0.64f, 0.14f, 28, 1, V3(0.03f, 0.03f, 0.035f), 1, V3(0.03f, 0.03f, 0.035f), true);
      // OPEN
      s.kvadr(V3(6.7f, 2.12f, Z), V3(7.9f, 2.68f, Z + 0.26f), 1, V3(0.05f, 0.05f, 0.06f));
      // kontrolky: rada 4 (azurove) a pristup na disk (oranzova)
      ledy.clear();
      for (int i = 0; i < 4; i++) {
        const float x = -1.8f + i * 0.8f;
        s.kvadr(V3(x - 0.24f, 2.72f, Z), V3(x + 0.24f, 2.92f, Z + 0.10f), 24 + i, azur);
        ledy.push_back({V3(x, 2.82f, Z + 0.12f), 4 + i, azur});
      }
      s.kvadr(V3(1.16f, 2.72f, Z), V3(1.64f, 2.92f, Z + 0.10f), 21, V3(1.0f, 0.60f, 0.15f));
      ledy.push_back({V3(1.4f, 2.82f, Z + 0.12f), 1, V3(1.0f, 0.60f, 0.15f)});
      ledy.push_back({V3(9.8f, 2.4f, Z + 0.14f), 0, azur});
      led = V3(9.8f, 2.4f, Z + 0.3f);
      // chromove ramecky zdirek a sterbin karet
      for (int i = 0; i < 2; i++) {
        const float cx = i == 0 ? -9.8f : -6.6f, cy = 2.15f, w = 1.20f, h = 0.52f, t = 0.08f;
        s.kvadr(V3(cx - w, cy + h - t, Z), V3(cx + w, cy + h, Z + 0.08f), 11, chrom);
        s.kvadr(V3(cx - w, cy - h, Z), V3(cx + w, cy - h + t, Z + 0.08f), 11, chrom);
        s.kvadr(V3(cx - w, cy - h, Z), V3(cx - w + t, cy + h, Z + 0.08f), 11, chrom);
        s.kvadr(V3(cx + w - t, cy - h, Z), V3(cx + w, cy + h, Z + 0.08f), 11, chrom);
        s.kvadr(V3(cx - 1.05f, 2.92f, Z), V3(cx + 1.05f, 3.2f, Z + 0.12f), 1, V3(0.04f, 0.04f, 0.05f));
        s.quad(V3(cx - 0.95f, 3.0f, Z + 0.125f), V3(cx + 0.95f, 3.0f, Z + 0.125f), V3(cx + 0.95f, 3.11f, Z + 0.125f), V3(cx - 0.95f, 3.11f, Z + 0.125f), 7, tma);
      }
    }
    // vetraci drazky na bocich
    for (int k = 0; k < 10; k++)
      for (int st = -1; st <= 1; st += 2) {
        const float z0 = -7.0f + k * 1.4f, x = st * (X + 0.012f);
        if (st < 0) s.quad(V3(x, 1.4f, z0), V3(x, 1.4f, z0 + 0.45f), V3(x, 3.2f, z0 + 0.45f), V3(x, 3.2f, z0), 7, tma);
        else s.quad(V3(x, 1.4f, z0 + 0.45f), V3(x, 1.4f, z0), V3(x, 3.2f, z0), V3(x, 3.2f, z0 + 0.45f), 7, tma);
      }
    sit.nahraj(s.v);
    podlaha = 0.f;

    // ---- viko (souradnice modelu; otaci se kolem pantu vzadu) ----
    {
      StavK v;
      const std::vector<V2> o = obrysZaobleny(-0.6f, -9.6f, 12.6f, 3.9f, 1.4f, 4);
      const std::vector<V2> okno = obrysKruh(dX, dZ, 4.6f, 48);
      v.hranol(o, 4.9f, 5.7f, 0.3f, 1, cerna, 1, cerna, 1, cerna, false);
      v.deskaMezi(StavK::odsad(o, 0.3f), okno, V2{dX, dZ}, 5.7f, 1.f, 1, cerna);
      v.deskaMezi(o, okno, V2{dX, dZ}, 4.9f, -1.f, 1, cerna);
      v.stenaKruhu(dX, dZ, 4.6f, 4.9f, 5.7f, 48, true, 1, cerna);
      // chromovy ramecek okenka a azurovy prstenec
      v.deskaMezi(obrysKruh(dX, dZ, 5.0f, 48), okno, V2{dX, dZ}, 5.712f, 1.f, 11, chrom);
      v.deskaMezi(obrysKruh(dX, dZ, 5.28f, 48), obrysKruh(dX, dZ, 5.0f, 48), V2{dX, dZ}, 5.706f, 1.f, 23, azur);
      ledy.push_back({V3(dX - 5.1f, 5.72f, dZ + 0.5f), 3, azur});
      viko.nahraj(v.v);
      StavK g;
      g.diskUV(dX, 5.55f, dZ, 4.62f, 48, 1.f, 9, V3(0, 0, 0));
      sklo.nahraj(g.v);
    }
    // ---- disk (pocatek = stred disku) ----
    {
      StavK d;
      d.diskUV(0.f, 0.06f, 0.f, 6.0f, 64, 1.f, 13, V3(1, 1, 1));
      d.diskUV(0.f, -0.06f, 0.f, 6.0f, 64, -1.f, 6, V3(1, 1, 1));
      d.stenaKruhu(0.f, 0.f, 6.0f, -0.06f, 0.06f, 64, false, 11, V3(0.70f, 0.73f, 0.78f));
      disk.nahraj(d.v);
    }
    diskStred = V3(dX, 4.51f, dZ);
    vikoOsaY = 5.3f; vikoOsaZ = -9.6f;
    vyska = 18.8f;                 // otevrene viko (86 stupnu) je nejvys
    diskAtlas[0] = potisk::DS32[0] / potisk::A; diskAtlas[1] = potisk::DS32[1] / potisk::A;
    diskAtlas[2] = (potisk::DS32[2] - potisk::DS32[0]) / potisk::A; diskAtlas[3] = (potisk::DS32[3] - potisk::DS32[1]) / potisk::A;
    nahrajAtlas(atlas, potisk::atlas32(bold, semi, ital));
    // obrys pro pruty: telo, deska, viko (otevrene - tak se v intru zhmotni), disk nahore
    hrany.clear();
    auto obd = [&](float x0, float y0, float z0, float x1, float y1, float z1) {
      V3 p[8] = {V3(x0, y0, z0), V3(x1, y0, z0), V3(x1, y0, z1), V3(x0, y0, z1), V3(x0, y1, z0), V3(x1, y1, z0), V3(x1, y1, z1), V3(x0, y1, z1)};
      static const int E[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
      for (auto &e : E) hrany.push_back({p[e[0]], p[e[1]]});
    };
    obd(-X, 0.4f, -Z, X, 4.4f, Z);
    obd(-X + 0.45f, 4.4f, -Z + 0.45f, X - 0.45f, 4.9f, Z - 0.45f);
    {
      const M4 mv = maticeVika(1.f);
      const V3 r[8] = {V3(-0.6f, 4.9f, -9.6f), V3(12.6f, 4.9f, -9.6f), V3(12.6f, 4.9f, 3.9f), V3(-0.6f, 4.9f, 3.9f),
                       V3(-0.6f, 5.7f, -9.6f), V3(12.6f, 5.7f, -9.6f), V3(12.6f, 5.7f, 3.9f), V3(-0.6f, 5.7f, 3.9f)};
      V3 p[8];
      for (int i = 0; i < 8; i++) p[i] = bod(mv, r[i]);
      static const int E[12][2] = {{0, 1}, {1, 2}, {2, 3}, {3, 0}, {4, 5}, {5, 6}, {6, 7}, {7, 4}, {0, 4}, {1, 5}, {2, 6}, {3, 7}};
      for (auto &e : E) hrany.push_back({p[e[0]], p[e[1]]});
    }
    {
      const AnimKonzole a = anim32(-1.f, 0.f);
      const M4 md = maticeDisku(a);
      for (int k = 0; k < 10; k++) {
        const float a0 = k * 0.6283185f, a1 = a0 + 0.6283185f;
        hrany.push_back({bod(md, V3(std::cos(a0) * 6.f, 0.f, std::sin(a0) * 6.f)), bod(md, V3(std::cos(a1) * 6.f, 0.f, std::sin(a1) * 6.f))});
      }
    }
  }

  // ---------------- animace ----------------
  // tv = cas od zacatku vkladani (s), < 0 = cartridge venku (vznasi se nad sterbinou); t = cas (vznaseni, blikani)
  static AnimKonzole anim16(float tv, float t) {
    AnimKonzole a;
    const float VEN = 3.2f, VLOZENO = -5.0f;
    if (tv < 0.f) {
      a.kazetaY = VEN + 0.35f * std::sin(t * 1.6f);
      a.kazetaNaklon = -0.10f + 0.03f * std::sin(t * 1.1f);
      a.klapky = 0.f;
      a.posuvnik = 0.f;
      return a;
    }
    const float t1 = 0.85f, t2 = 1.20f, t3 = 1.36f;
    float y;
    if (tv < t1) {
      const float k = hladce01(tv / t1);
      y = VEN + (0.12f - VEN) * k;
      a.kazetaNaklon = -0.10f * (1.f - k);
    } else if (tv < t2) {
      const float k = (tv - t1) / (t2 - t1);
      y = 0.12f + (VLOZENO - 0.12f) * (k * k * (3.f - 2.f * k));
    } else if (tv < t3) {
      y = VLOZENO - 0.22f * std::sin((tv - t2) / (t3 - t2) * 3.14159265f);   // cvak
    } else {
      y = VLOZENO;
    }
    a.kazetaY = y;
    a.klapky = std::min(1.f, std::max(0.f, -y / 0.9f));
    // CART blika uz pri zasouvani (kontakt), po zapnuti 3x a pak sviti slabe
    const float tp = tv - 1.55f;
    a.posuvnik = hladce01(tp / 0.20f);
    if (tv > t1 && tv < t3) a.led[1] = std::fmod(tv, 0.10f) < 0.05f ? 0.9f : 0.f;
    if (tp > 0.f) a.led[0] = tp < 0.14f ? (std::fmod(tp, 0.06f) < 0.03f ? 1.f : 0.3f) : 1.f;
    const float tc = tv - 1.80f;
    if (tc > 0.f) a.led[1] = tc < 1.2f ? (std::fmod(tc, 0.4f) < 0.2f ? 1.f : 0.f) : 0.55f + 0.08f * std::sin(t * 3.0f);
    return a;
  }
  // tv = cas od zacatku vkladani disku, < 0 = viko otevrene a disk se vznasi nad mechanikou
  static AnimKonzole anim32(float tv, float t) {
    AnimKonzole a;
    const float NAHORE = 6.0f, OMEGA = 11.f, ROZJEZD = 1.4f;
    if (tv < 0.f) {
      a.viko = 1.f;
      a.diskY = NAHORE + 0.35f * std::sin(t * 1.4f);
      a.diskNaklon = 0.30f;
      a.uhelDisku = t * 0.5f;
      return a;
    }
    const float t1 = 0.75f, tv0 = 0.85f, tv1 = 1.30f, tz = 1.45f;
    const float uhelPristani = 0.f;
    if (tv < t1) {
      const float k = hladce01(tv / t1);
      a.diskY = NAHORE * (1.f - k);
      a.diskNaklon = 0.30f * (1.f - k);
      a.uhelDisku = uhelPristani - (1.f - k) * 0.6f;
    } else {
      a.diskY = tv < t1 + 0.12f ? 0.10f * std::sin((tv - t1) / 0.12f * 3.14159265f) : 0.f;
      a.uhelDisku = uhelPristani;
    }
    if (tv < tv0) a.viko = 1.f;
    else if (tv < tv1) { const float k = (tv - tv0) / (tv1 - tv0); a.viko = 1.f - k * k; }
    else a.viko = tv < tv1 + 0.14f ? 0.035f * std::sin((tv - tv1) / 0.14f * 3.14159265f) : 0.f;
    const float x = tv - tz;
    if (x > 0.f) {
      const float w = OMEGA * std::min(1.f, x / ROZJEZD);
      a.uhelDisku = uhelPristani + (x < ROZJEZD ? OMEGA * x * x / (2.f * ROZJEZD) : OMEGA * (x - ROZJEZD * 0.5f));
      a.rozmaz = std::min(0.55f, w * 0.045f);
      a.led[0] = hladce01(x / 0.2f);                                        // POWER
      a.led[1] = x < 2.0f ? (std::fmod(x, 0.16f) < 0.08f ? 1.f : 0.15f) : (std::fmod(t * 1.7f, 1.f) < 0.12f ? 0.8f : 0.2f);   // pristup na disk
      a.led[2] = hladce01((x - 0.1f) / 0.4f) * (0.80f + 0.20f * std::sin(t * 1.5f));          // pas dole
      a.led[3] = hladce01((x - 0.2f) / 0.5f) * (0.75f + 0.25f * std::sin(t * 2.2f));          // prstenec okenka
      for (int i = 0; i < 4; i++) a.led[4 + i] = hladce01((x - 0.15f - i * 0.12f) / 0.1f);
    }
    return a;
  }

  // ---------------- kresleni ----------------
  static V3 bod(const M4 &m, const V3 &p) {
    return V3(m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12], m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
              m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]);
  }
  M4 maticeVika(float viko) const {
    return M4::posun(0.f, vikoOsaY, vikoOsaZ) * M4::rotX(-viko * 1.50f) * M4::posun(0.f, -vikoOsaY, -vikoOsaZ);
  }
  M4 maticeDisku(const AnimKonzole &a) const {
    return M4::posun(diskStred.x, diskStred.y + a.diskY, diskStred.z + a.diskY * 0.18f) * M4::rotX(a.diskNaklon) * M4::rotY(a.uhelDisku);
  }
  M4 maticeKazety(const AnimKonzole &a) const { return M4::posun(0.f, vezY + a.kazetaY, sterbinaZ) * M4::rotX(a.kazetaNaklon); }

  // Nakresli celou konzoli: model = poloha ve svete, vp = pohled * projekce. Program uz je
  // pouzity a ma nastavena svetla; tady se nastavi kontrolky, atlas (jednotka 2) a dily.
  void kresli(Prog &p, const M4 &model, const M4 &vp, const AnimKonzole &a) const {
    p.f4("uLedA", a.led[0], a.led[1], a.led[2], a.led[3]);
    p.f4("uLedB", a.led[4], a.led[5], a.led[6], a.led[7]);
    p.f4("uDiskAtlas", diskAtlas[0], diskAtlas[1], diskAtlas[2], diskAtlas[3]);
    p.f1("uRozmaz", a.rozmaz);
    p.tex("uAtlas", 2, atlas);
    glActiveTexture(GL_TEXTURE0);
    auto dil = [&](const Sit &s, const M4 &m) {
      if (!s.pocet) return;
      p.m4("uModel", m);
      p.m4("uMVP", vp * m);
      s.kresli();
    };
    dil(sit, model);
    if (bitu == 16) {
      dil(kazeta, model * maticeKazety(a));
      const float u = a.klapky * 1.45f;
      dil(klapkaP, model * M4::posun(0.f, vezY - 0.02f, sterbinaZ + sterbinaPul) * M4::rotX(-u));
      dil(klapkaZ, model * M4::posun(0.f, vezY - 0.02f, sterbinaZ - sterbinaPul) * M4::rotX(u));
      dil(posuvnik, model * M4::posun(-9.30f + 1.50f * a.posuvnik, 2.3f, 9.8f + 0.012f));
    } else {
      const M4 mv = model * maticeVika(a.viko);
      dil(viko, mv);
      dil(disk, model * maticeDisku(a));
      // kourove sklo okenka: pruhledne, bez zapisu hloubky, nakonec
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      glDepthMask(GL_FALSE);
      dil(sklo, mv);
      glDepthMask(GL_TRUE);
      glDisable(GL_BLEND);
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
