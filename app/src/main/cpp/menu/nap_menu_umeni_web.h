// nap_menu_umeni_web.h
// ====================================================================
//  B311: KARTA ATARIHELP.EU - digitalni zemekoule
//
//  Koule se pocita v shaderu (paprsek x koule): pevnina z 3D sumu jako
//  svitici tecky (bodova matice), oceany tmave s mrizkou poledniku
//  a rovnobezek, atmosfera kolem okraje, nekolik "pingu" (mesta, odkud
//  se lide pripojuji). Kolem obiha po zlate carkovane draze minca N&P
//  (za koulí se schova). Koule se pomalu toci.
// ====================================================================
#pragma once
#include "nap_menu_umeni.h"

namespace napmenu {

static const char *FS_WEB = R"(
varying vec2 vUV;
uniform float uCas;
uniform float uPomer;
uniform vec3 uBarva;
uniform float uFokus;
uniform vec2 uStred;       // stred koule (x od stredu karty v sirkach, y 0..1)
uniform float uR;          // polomer koule (dil vysky karty)
uniform vec3 uDraha;       // drahy: poloosy a, b (v polomerech koule), natoceni
)" "%SUM%" R"(
float sq(float x){ return x*x; }
void main(){
  vec2 uv = vUV;
  vec2 p = vec2((uv.x - 0.5)*uPomer, uv.y);
  vec2 q = (p - uStred)/uR;
  float r2 = dot(q, q);
  float r = sqrt(r2);
  vec3 atmB = mix(vec3(0.25,0.95,0.85), uBarva, 0.45);
  vec3 c = vec3(0.0);
  float a = 0.0;
  if (r2 < 1.0) {
    float z = sqrt(1.0 - r2);
    vec3 n = vec3(q, z);
    // naklon osy a otaceni
    float ct = cos(0.38), st = sin(0.38);
    vec3 m = vec3(n.x, n.y*ct - n.z*st, n.y*st + n.z*ct);
    float uh = uCas*0.22;
    float cu = cos(uh), su = sin(uh);
    m = vec3(m.x*cu + m.z*su, m.y, -m.x*su + m.z*cu);
    float lat = asin(clamp(m.y, -1.0, 1.0));
    float lon = atan(m.x, m.z);
    // pevnina: 3D sum na kouli (stejny tvar pri otaceni)
    float zem = fbm3(m*1.9 + vec3(4.1, 1.3, 7.7));
    float pevnina = smoothstep(0.50, 0.535, zem)*(1.0 - smoothstep(0.80, 0.95, abs(m.y)));
    // bodova matice (tecky po povrchu)
    vec2 g = vec2(lon*cos(lat)*13.0, lat*13.0)/0.5;
    vec2 f = fract(g) - 0.5;
    float tecka = smoothstep(0.34, 0.16, length(f));
    // mrizka poledniku a rovnobezek (po 20 stupnich)
    float mr = max(exp(-sq(fract(lat/0.349 + 0.5) - 0.5)*900.0), exp(-sq(fract(lon/0.349 + 0.5) - 0.5)*900.0*clamp(cos(lat)*1.6, 0.2, 1.0)));
    vec3 L = normalize(vec3(-0.55, 0.60, 0.65));
    float dif = max(dot(n, L), 0.0);
    vec3 ocean = vec3(0.006,0.035,0.045)*(0.45 + 0.9*dif) + atmB*mr*0.20;
    vec3 souse = mix(vec3(0.03,0.16,0.14), mix(vec3(0.45,1.0,0.85), vec3(1.0,0.85,0.45), 0.25), tecka)*(0.30 + 0.95*dif);
    c = mix(ocean, souse, pevnina);
    // pingy: par mest na pevnine, kruhy se sireji
    for (int i = 0; i < 4; i++) {
      float fi = float(i);
      vec3 mesto = normalize(vec3(sin(fi*2.4 + 0.6), 0.45*sin(fi*1.7 + 0.2), cos(fi*2.4 + 0.6)));
      float d = acos(clamp(dot(m, mesto), -1.0, 1.0));
      float fz = fract(uCas*0.45 + fi*0.27);
      float kr = exp(-sq((d - fz*0.32)/0.012))*(1.0 - fz);
      c += vec3(1.0,0.85,0.45)*(kr*0.9 + exp(-d*d*4000.0)*1.2)*step(0.0, dot(n, vec3(0.0, 0.0, 1.0)));
    }
    // okraj: atmosfera a protisvetlo
    c += atmB*pow(1.0 - z, 2.5)*0.85;
    c *= 0.85 + 0.15*uFokus;
    a = 1.0 - smoothstep(0.985, 1.0, r);
    c *= a;
  }
  // zare atmosfery venku
  c += atmB*exp(-max(r - 1.0, 0.0)*7.0)*0.22*step(1.0, r);
  // draha (elipsa) - predni pulka pres kouli, zadni se za ni schova
  float ca = cos(uDraha.z), sa = sin(uDraha.z);
  vec2 e = vec2(q.x*ca + q.y*sa, -q.x*sa + q.y*ca);
  vec2 ee = e/uDraha.xy;
  float le = length(ee);
  float d = (le - 1.0)*min(uDraha.x, uDraha.y)*uR;
  float th = atan(ee.y, ee.x);
  float carka = step(0.45, fract(th*18.0/6.2831853 - uCas*0.15));
  float vpredu = step(e.y, 0.0);
  float vidi = max(vpredu, step(1.0, r));
  float pas = (exp(-sq(d/0.0018))*0.9*(0.35 + 0.65*carka) + exp(-abs(d)*120.0)*0.10)*vidi;
  c += vec3(1.0,0.78,0.36)*pas;
  gl_FragColor = vec4(c, a);
}
)";

class UmeniWeb : public Umeni {
 public:
  bool init(KontextUmeni &k) override {
    (void)k;
    p_.sestav(napdemo::VS_PLNY, sSumem(FS_WEB).c_str(), "menu-web");
    return p_.ok;
  }
  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    k.pozadi(r, 0.37f, 0.36f, 0.35f);
    const float t = (float)r.t;
    const float pomer = r.pomer();
    const float R = 0.235f;                                   // polomer koule (dil vysky karty)
    const float sx = 0.f, sy = 0.62f;
    const float A = 1.55f, B = 0.36f, U = -0.28f;            // draha
    // minca na draze: uhel, poloha, je vpredu?
    const float th = t * 0.55f + 1.2f;
    const float ex = std::cos(th) * A, ey = std::sin(th) * B;
    const float qx = ex * std::cos(U) - ey * std::sin(U), qy = ex * std::sin(U) + ey * std::cos(U);
    const bool vpredu = ey < 0.f;
    // souradnice jako v shaderu: v jednotkach vysky karty, x od stredu, y od spodku
    const float mx = (sx + qx * R) / pomer, my = sy + qy * R;
    const float mr = R * 0.30f * (vpredu ? 1.f : 0.78f);           // polomer mince (dil vysky karty)
    auto minca = [&]() {
      glEnable(GL_BLEND);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      const float lesk = -0.6f + 2.6f * std::min(1.f, std::fmod(t, 4.f) / 1.0f);
      k.minca(mx * 2.f, my * 2.f - 1.f, mr, pomer, vpredu ? 1.f : 0.7f, lesk);
    };
    if (!vpredu) minca();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    p_.pouzij();
    p_.f1("uCas", t);
    p_.f1("uPomer", pomer);
    p_.f3("uBarva", r.barva);
    p_.f1("uFokus", r.fokus);
    p_.f2("uStred", sx, sy);
    p_.f1("uR", R);
    p_.f3("uDraha", A, B, U);
    p_.tex("uSum", 0, k.texSum);
    k.ctverec.kresli();
    if (vpredu) minca();
    glDisable(GL_BLEND);
  }
  const char *nazev() const override { return "fx:web"; }

 private:
  Prog p_;
};

}  // namespace napmenu
