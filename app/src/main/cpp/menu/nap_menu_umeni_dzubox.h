// nap_menu_umeni_dzubox.h
// ====================================================================
//  B311: KARTA DZUBOX (PRO DJ MIXER) - neonovy dzubox, ktery tancuje
//
//  Rene: "DZUBOX s ekvalizerama a bumnama, at to ma poradne basy a tancuje to".
//  Neonovy oblouk (magenta a azurova trubice), uvnitr dva reproduktory,
//  ktere pumpuji v rytmu basoveho bubnu (124 BPM) a pousti tlakove vlny,
//  nad nimi spektralni analyzator 16 pasem z LED dilku se spickami, ktere
//  pomalu padaji. Svetla klubu, plovouci noty, cela karta pulzuje s bubnem.
//  Hudba se nehraje (menu je tiche) - rytmus pocita C++ (kopak, virbl,
//  hi-hat a basova linka), pasma jdou do shaderu jako textura 16x1.
// ====================================================================
#pragma once
#include "nap_menu_umeni.h"

namespace napmenu {

static const char *FS_DZUBOX = R"(
varying vec2 vUV;
uniform float uCas;
uniform float uPomer;
uniform float uKopak;      // basovy buben 0..1 (obalka)
uniform float uVirbl;
uniform float uFaze;       // faze dob (0..1 mezi kopaky)
uniform vec3 uBarva;
uniform sampler2D uPasma;  // 16x1: r = uroven, g = spicka
)" "%SUM%" R"(
const float YB = 0.37;     // spodek oblouku (podlaha)
const float YA = 0.645;    // stred horniho pulkruhu
const float W = 0.262;     // polovicni sirka oblouku
float oblouk(vec2 p){
  if (p.y < YA) return max(abs(p.x) - W, YB - p.y);
  return length(p - vec2(0.0, YA)) - W;
}
vec3 neon(float d, vec3 b, float sila){
  return b*(exp(-d*d*90000.0)*1.6 + exp(-abs(d)*42.0)*0.45)*sila;
}
float nota(vec2 q){            // jednoducha nota: hlavicka + nozicka + praporek
  float h = length((q - vec2(0.0, 0.0))*vec2(1.0, 1.45)) - 0.016;
  float n = max(abs(q.x - 0.014) - 0.0028, abs(q.y - 0.04) - 0.04);
  float f = max(abs(q.x - 0.026 - (q.y - 0.07)*0.6) - 0.004, abs(q.y - 0.065) - 0.015);
  return min(min(h, n), f);
}
float sq(float x){ return x*x; }
void main(){
  vec2 uv = vUV;
  vec2 p = vec2((uv.x - 0.5)*uPomer, uv.y);
  float kop = uKopak;
  // pozadi: hluboka fialova, zare magenty za obloukem (dycha s bubnem)
  vec3 c = mix(vec3(0.010,0.004,0.018), vec3(0.030,0.010,0.045), uv.y);
  vec2 z = vec2(p.x, uv.y - 0.62);
  c += mix(vec3(0.9,0.15,0.75), uBarva, 0.4)*exp(-dot(z,z)*7.0)*(0.20 + 0.18*kop);
  // svetla klubu z hornich rohu (pomalu se otaci)
  for (int i = 0; i < 2; i++) {
    float s = i == 0 ? -1.0 : 1.0;
    vec2 zdroj = vec2(s*0.40, 1.02);
    vec2 d = p - zdroj;
    float a = atan(d.x, -d.y) - s*(0.35 + 0.25*sin(uCas*0.8 + float(i)*2.0));
    float paprsek = exp(-a*a*60.0)*smoothstep(1.0, 0.2, length(d));
    c += (i == 0 ? vec3(1.0,0.25,0.85) : vec3(0.25,0.85,1.0))*paprsek*(0.10 + 0.10*kop);
  }
  float sd = oblouk(p);
  // vnitrek oblouku: tmavsi sklo
  if (sd < 0.0) c = mix(c, vec3(0.012,0.006,0.02), 0.65);
  // ---- reproduktory (dole v oblouku): kuzel pumpuje s bubnem ----
  for (int i = 0; i < 2; i++) {
    vec2 st = vec2(i == 0 ? -0.125 : 0.125, 0.455);
    float r = length(p - st);
    float R = 0.072*(1.0 + 0.10*kop);
    float okraj = abs(r - R) - 0.006;
    float kuzel = smoothstep(R, R*0.25, r);
    vec3 rc = vec3(0.05,0.04,0.06)*(0.6 + 0.6*kuzel) + vec3(0.6,0.5,0.7)*exp(-sq((r - R*0.62)/0.004))*0.25;
    rc += vec3(0.95,0.85,1.0)*exp(-r*r/(R*R*0.03))*(0.25 + 0.5*kop);          // stredova cepicka se leskne
    if (r < R + 0.006) c = rc;
    c += vec3(1.0,0.3,0.9)*exp(-okraj*okraj*40000.0)*(0.6 + 0.8*kop);
    // tlakova vlna od bubnu
    float vlna = R + uFaze*0.30;
    float w = exp(-sq((r - vlna)/0.006))*(1.0 - uFaze)*(1.0 - uFaze);
    c += mix(vec3(1.0,0.3,0.9), vec3(0.3,0.9,1.0), uFaze)*w*0.9*step(sd, 0.03);
  }
  // ---- spektralni analyzator: 16 pasem, LED dilky, spicky ----
  float x0 = -0.215, x1 = 0.215, y0 = 0.555, vys = 0.335;
  if (p.x > x0 && p.x < x1 && p.y > y0 && sd < -0.03) {
    float f = (p.x - x0)/(x1 - x0)*16.0;
    float i = floor(f);
    float fx = fract(f);
    vec4 t = texture2D(uPasma, vec2((i + 0.5)/16.0, 0.5));
    float h = (p.y - y0)/vys;
    float dilek = step(0.26, fract(h*16.0));
    float mezera = step(0.14, fx)*step(fx, 0.86);
    if (h < t.r) {
      vec3 b = h < 0.5 ? mix(vec3(0.15,0.95,1.0), vec3(0.95,0.25,0.95), h*2.0) : mix(vec3(0.95,0.25,0.95), vec3(1.0,0.62,0.15), (h - 0.5)*2.0);
      c += b*dilek*mezera*1.25;
    }
    float sp = abs(h - t.g);
    c += vec3(1.0,0.95,1.0)*step(sp, 0.012)*mezera*0.9;
  }
  // odraz pasem v podlaze (pod obloukem)
  if (p.y < YB && p.y > YB - 0.09 && abs(p.x) < 0.215) {
    float f = (p.x + 0.215)/0.43*16.0;
    vec4 t = texture2D(uPasma, vec2((floor(f) + 0.5)/16.0, 0.5));
    float h = (YB - p.y)/0.09;
    c += vec3(0.6,0.3,0.8)*step(h*0.25, t.r*0.25)*(1.0 - h)*0.18*step(0.14, fract(f))*step(fract(f), 0.86);
  }
  // ---- neonove trubice oblouku ----
  c += neon(sd, vec3(1.0,0.22,0.85), 0.75 + 0.5*kop);
  c += neon(sd + 0.022, vec3(0.25,0.90,1.0), 0.55 + 0.25*uVirbl);
  // podlaha: odraz oblouku
  if (p.y < YB) {
    vec2 q = vec2(p.x, 2.0*YB - p.y);
    float so = oblouk(q);
    float k = smoothstep(YB, YB - 0.12, p.y);
    c += (neon(so, vec3(1.0,0.22,0.85), 0.75) + neon(so + 0.022, vec3(0.25,0.90,1.0), 0.5))*0.28*(1.0 - k);
    c += vec3(0.5,0.2,0.6)*exp(-sq((p.y - YB)/0.004))*0.5;
  }
  // plovouci noty
  for (int i = 0; i < 5; i++) {
    float fi = float(i);
    float y = fract(uCas*0.07 + fi*0.21);
    vec2 st = vec2(-0.30 + fi*0.15 + 0.03*sin(uCas*0.9 + fi*2.3), 0.42 + y*0.55);
    float a = smoothstep(0.0, 0.15, y)*smoothstep(1.0, 0.75, y);
    float d = nota(p - st);
    c += (mod(fi, 2.0) < 0.5 ? vec3(1.0,0.4,0.9) : vec3(0.35,0.9,1.0))*(exp(-max(d, 0.0)*900.0)*0.9 + exp(-max(d, 0.0)*90.0)*0.15)*a*step(0.0, sd + 0.0)*0.8;
  }
  c *= 1.0 + 0.22*kop;
  vec2 v = uv - 0.5;
  c *= 1.0 - dot(v, v)*0.9;
  gl_FragColor = vec4(c, 1.0);
}
)";

class UmeniDzubox : public Umeni {
 public:
  ~UmeniDzubox() override { if (tex_) glDeleteTextures(1, &tex_); }
  bool init(KontextUmeni &k) override {
    (void)k;
    p_.sestav(napdemo::VS_PLNY, sSumem(FS_DZUBOX).c_str(), "menu-dzubox");
    std::vector<unsigned char> nul(16 * 4, 0);
    tex_ = napdemo::novaTextura(16, 1, GL_RGBA, nul.data(), false);
    return p_.ok && tex_;
  }

  // rytmus: 124 BPM, kopak na kazdou dobu, virbl na 2 a 4, hi-hat na osminy
  static void rytmus(double t, float &kopak, float &virbl, float &hihat, float &faze, int &doba) {
    const double P = 60.0 / 124.0;
    const double f = std::fmod(t, P);
    faze = (float)(f / P);
    doba = (int)std::floor(t / P);
    kopak = (float)std::exp(-f * 9.0);
    virbl = (doba & 1) ? (float)std::exp(-f * 12.0) : 0.f;
    const double f8 = std::fmod(t, P * 0.5);
    hihat = (float)std::exp(-f8 * 22.0);
  }

  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    float kop, vir, hh, faze;
    int doba;
    rytmus(r.t, kop, vir, hh, faze, doba);
    aktualizujPasma(r.t, kop, vir, hh, doba);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    p_.pouzij();
    p_.f1("uCas", (float)r.t);
    p_.f1("uPomer", r.pomer());
    p_.f1("uKopak", kop * (0.35f + 0.65f * r.fokus));
    p_.f1("uVirbl", vir);
    p_.f1("uFaze", faze);
    p_.f3("uBarva", r.barva);
    p_.tex("uPasma", 0, tex_);
    p_.tex("uSum", 1, k.texSum);
    k.ctverec.kresli();
    glActiveTexture(GL_TEXTURE0);
  }
  const char *nazev() const override { return "fx:dzubox"; }

 private:
  Prog p_;
  GLuint tex_ = 0;
  float uroven_[16] = {}, spicka_[16] = {}, drzeni_[16] = {};
  double posl_ = -1.0;

  static float hash(int a, int b) {
    uint32_t h = (uint32_t)a * 374761393u + (uint32_t)b * 668265263u;
    h = (h ^ (h >> 13)) * 1274126177u;
    return (float)((h ^ (h >> 16)) & 0xFFFF) / 65535.f;
  }

  void aktualizujPasma(double t, float kop, float vir, float hh, int doba) {
    double dt = posl_ < 0 ? 1.0 : t - posl_;
    posl_ = t;
    if (dt < 0 || dt > 0.25) dt = 0.25;
    // basova linka: kazde 2 doby jiny ton (vyska basu)
    const float bas = 0.55f + 0.30f * hash(doba / 2, 7);
    const int osmina = (int)std::floor(t / (30.0 / 124.0));
    unsigned char px[16 * 4];
    for (int i = 0; i < 16; i++) {
      const float f = i / 15.f;
      float cil = 0.10f + 0.06f * hash(osmina, i);
      cil += kop * (0.95f * std::max(0.f, 1.f - f * 3.2f));                        // kopak: basy
      cil += bas * 0.45f * std::max(0.f, 1.f - std::fabs(f - 0.15f) * 4.f);         // basova linka
      cil += vir * 0.65f * std::exp(-(f - 0.55f) * (f - 0.55f) * 14.f);             // virbl: stredy
      cil += hh * 0.50f * std::max(0.f, (f - 0.62f) * 2.6f);                         // hi-hat: vysky
      cil += 0.12f * hash(osmina * 3 + 1, i * 7) * (0.4f + f);
      cil = std::min(0.98f, cil);
      // nabeh hned, dozvuk ~0,12 s
      uroven_[i] = cil > uroven_[i] ? cil : uroven_[i] + (cil - uroven_[i]) * (1.f - (float)std::exp(-dt / 0.12));
      // spicka: drzi 0,3 s, pak pada 1,1 / s
      if (uroven_[i] >= spicka_[i]) { spicka_[i] = uroven_[i]; drzeni_[i] = 0.30f; }
      else if (drzeni_[i] > 0.f) drzeni_[i] -= (float)dt;
      else spicka_[i] = std::max(uroven_[i], spicka_[i] - 1.1f * (float)dt);
      px[i * 4 + 0] = (unsigned char)(uroven_[i] * 255.f);
      px[i * 4 + 1] = (unsigned char)(std::min(1.f, spicka_[i] + 0.03f) * 255.f);
      px[i * 4 + 2] = 0;
      px[i * 4 + 3] = 255;
    }
    glBindTexture(GL_TEXTURE_2D, tex_);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 16, 1, GL_RGBA, GL_UNSIGNED_BYTE, px);
  }
};

}  // namespace napmenu
