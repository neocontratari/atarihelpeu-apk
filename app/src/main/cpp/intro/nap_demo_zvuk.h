// nap_demo_zvuk.h
// ====================================================================
//  B304: ZVUK UVODNIHO DEMA - cely soundtrack (45 s) spocitany v C++.
//
//  Zadna nahravka, zadny soubor. Vsechno je synteza po vzorcich:
//   - bzuceni stare obrazovky (50 Hz sit + harmonicke, snih, piskot
//     radkoveho rozkladu 15 734 Hz), cvakani zarovky, motor a rachot vrat
//   - vitr, zpev ptaku, hromy (prask + dunivy val), dest
//   - hudba: tiche plochy a zvonky pri Atari (Dm - Bb - F - C), pak
//     120 BPM (kopak, virbl, basa, arpeggio jako POKEY), zasahy energie,
//     znelky strihu (Atari, Sega FM, PS1, sit) a nakonec gong, D dur
//     a zlate zvonky pod logem
//  Na konci dozvuk (jako Freeverb) a mekky limiter.
//
//  Cela stopa se pocita dopredu ve vlakne (je mnohem rychlejsi nez
//  prehravani) - prehravac si jen bere hotove vzorky. Obraz se ridi
//  podle toho, co uz reproduktor opravdu prehral.
// ====================================================================
#pragma once
#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <vector>
#include "nap_demo_cas.h"

namespace napdemo {
namespace zv {

constexpr int SR = 44100;
constexpr float FS = 44100.f;
constexpr float PI2 = 6.2831853f;

// ---------------- zakladni stavebnice ----------------
struct SinTab {
  float t[4097];
  SinTab() { for (int i = 0; i <= 4096; i++) t[i] = std::sin(PI2 * i / 4096.f); }
};
inline const SinTab &sinTab() { static const SinTab s; return s; }
inline float sinus(float faze) {          // faze v otackach
  faze -= std::floor(faze);
  const float x = faze * 4096.f;
  const int i = (int)x;
  const float f = x - (float)i;
  const float *t = sinTab().t;
  return t[i] + (t[i + 1] - t[i]) * f;
}
inline float midi(float m) { return 440.f * std::pow(2.f, (m - 69.f) / 12.f); }
inline float hladce(float a, float b, float x) {
  float t = (x - a) / (b - a);
  t = t < 0.f ? 0.f : (t > 1.f ? 1.f : t);
  return t * t * (3.f - 2.f * t);
}
inline float okno(float x, float a, float b, float c, float d) { return hladce(a, b, x) * (1.f - hladce(c, d, x)); }

struct Sum {
  uint32_t s;
  explicit Sum(uint32_t x = 1) : s(x ? x : 1) {}
  float operator()() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (float)(int32_t)s * (1.f / 2147483648.f); }
};
// pomalu se menici nahoda (pro poryvy, valeni hromu)
struct Pomala {
  Sum s; float a = 0, b = 0, f = 0, krok = 0;
  Pomala(uint32_t seed, float hz) : s(seed), krok(hz / FS) { a = s(); b = s(); }
  float operator()() {
    f += krok;
    if (f >= 1.f) { f -= 1.f; a = b; b = s(); }
    float k = f * f * (3.f - 2.f * f);
    return a + (b - a) * k;
  }
};
struct Lp1 {
  float y = 0, a = 1;
  void nastav(float fc) { a = 1.f - std::exp(-PI2 * fc / FS); }
  float operator()(float x) { y += a * (x - y); return y; }
};
struct Hp1 {
  Lp1 l;
  void nastav(float fc) { l.nastav(fc); }
  float operator()(float x) { return x - l(x); }
};
// TPT stavovy filtr (Zavalishin) - stabilni i pri zmenach kmitoctu
struct Svf {
  float a1 = 0, a2 = 0, a3 = 0, k = 1, ic1 = 0, ic2 = 0, lp = 0, bp = 0, hp = 0;
  void nastav(float fc, float q) {
    fc = std::min(std::max(fc, 12.f), FS * 0.45f);
    const float g = std::tan(3.14159265f * fc / FS);
    k = 1.f / q; a1 = 1.f / (1.f + g * (g + k)); a2 = g * a1; a3 = g * a2;
  }
  void operator()(float x) {
    const float v3 = x - ic2, v1 = a1 * ic1 + a2 * v3, v2 = ic2 + a2 * ic1 + a3 * v3;
    ic1 = 2.f * v1 - ic1; ic2 = 2.f * v2 - ic2;
    lp = v2; bp = v1; hp = x - k * v1 - v2;
  }
};
inline float polyblep(float t, float dt) {
  if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
  if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
  return 0.f;
}
inline float pila(float faze, float dt) { return 2.f * faze - 1.f - polyblep(faze, dt); }
inline float ctverec(float faze, float dt, float strida = 0.5f) {
  float f2 = faze + (1.f - strida); f2 -= std::floor(f2);
  return (faze < strida ? 1.f : -1.f) + polyblep(faze, dt) - polyblep(f2, dt);
}

// dozvuk (Freeverb: 6 hrebenovych + 3 vsepropustne na kanal)
struct Comb {
  std::vector<float> b; int i = 0; float fb = 0.9f, d = 0.3f, z = 0;
  void init(int n) { b.assign(n, 0.f); i = 0; z = 0; }
  float operator()(float x) {
    const float y = b[i];
    z = y * (1.f - d) + z * d;
    b[i] = x + z * fb;
    if (++i >= (int)b.size()) i = 0;
    return y;
  }
};
struct Vsepro {
  std::vector<float> b; int i = 0;
  void init(int n) { b.assign(n, 0.f); i = 0; }
  float operator()(float x) {
    const float y = b[i];
    b[i] = x + y * 0.5f;
    if (++i >= (int)b.size()) i = 0;
    return y - x;
  }
};
struct Dozvuk {
  Comb cl[6], cr[6];
  Vsepro al[3], ar[3];
  void init(float velikost, float fb, float tlum) {
    static const int C[6] = {1116, 1188, 1277, 1356, 1422, 1491};
    static const int A[3] = {556, 441, 341};
    for (int i = 0; i < 6; i++) {
      cl[i].init((int)(C[i] * velikost)); cr[i].init((int)((C[i] + 23) * velikost));
      cl[i].fb = cr[i].fb = fb; cl[i].d = cr[i].d = tlum;
    }
    for (int i = 0; i < 3; i++) { al[i].init((int)(A[i] * velikost)); ar[i].init((int)((A[i] + 23) * velikost)); }
  }
  void operator()(float l, float r, float &ol, float &or_) {
    const float x = (l + r) * 0.05f;
    float sl = 0, sr = 0;
    for (int i = 0; i < 6; i++) { sl += cl[i](x); sr += cr[i](x); }
    for (int i = 0; i < 3; i++) { sl = al[i](sl); sr = ar[i](sr); }
    ol = sl; or_ = sr;
  }
};

// ---------------- noty (udalosti) ----------------
enum Druh : uint8_t {
  PAD, ZVON, PIANO, ARP, BASA, KOPAK, VIRBL, HAT, ZAP, NARAZ, FMPLECH, PING, KLAP, GONG, PODBAS, TEP, PIP, CVAK, RISER,
  SNIH, FMBASA
};
struct Nota {
  float od, delka, f, hl, pan, dozvuk;
  Druh druh;
};
struct Hlas {
  Nota n;
  float faze[6] = {0, 0, 0, 0, 0, 0};
  Svf flt, flt2;
  Sum sum;
  Lp1 lp;
  int ridici = 0;
  bool zivy = true;
};

// hrom: prask (u blizkych), rana a dlouhe valeni
struct Hrom {
  float t, sila, pan, rana, prask, tlumeny, delka;
};

class Stopa {
 public:
  static constexpr float DELKA = cas::KONEC + 0.6f;   // B309: 59,6 s (drive 45,6 s)

  Stopa() { celkemRamcu = (int)(DELKA * FS); }
  int celkem() const { return celkemRamcu; }
  // B307: zvuk ze zaznamu hry (mono 44,1 kHz) - hraje od casu "od" s hlasitosti hl
  void nastavZaznam(const int16_t *p, size_t n, float od, float hl) {
    zaznam.assign(p, p + n); zaznamOd = od; zaznamHl = hl;
  }
  int hotovo() const { return hotovoRamcu.load(std::memory_order_acquire); }
  const int16_t *data() const { return pcm.data(); }
  void zastav() { stop.store(true); }

  // Spocita celou stopu (volat ve vlastnim vlakne). Prubezne zvysuje
  // hotovo(), takze prehravani muze zacit hned.
  void vyrob() {
    pcm.assign((size_t)celkemRamcu * 2, 0);
    sestavPartituru();
    dozvuk.init(1.25f, 0.90f, 0.32f);
    const int BLOK = 1024;
    std::vector<float> L(BLOK), R(BLOK), SL(BLOK), SR(BLOK);
    for (int z = 0; z < celkemRamcu && !stop.load(); z += BLOK) {
      const int n = std::min(BLOK, celkemRamcu - z);
      for (int i = 0; i < n; i++) { L[i] = R[i] = SL[i] = SR[i] = 0.f; }
      vrstvy(z, n, L.data(), R.data(), SL.data(), SR.data());
      noty(z, n, L.data(), R.data(), SL.data(), SR.data());
      hromy(z, n, L.data(), R.data(), SL.data(), SR.data());
      if (!zaznam.empty()) {
        const long long od = (long long)(zaznamOd * FS);
        for (int i = 0; i < n; i++) {
          const long long k = (long long)(z + i) - od;
          if (k < 0 || k >= (long long)zaznam.size()) continue;
          // kratke nabehy a dobehy (bez cvaknuti)
          const float env = std::min(1.f, std::min((float)k, (float)((long long)zaznam.size() - k)) / 300.f);
          const float x = zaznam[(size_t)k] * (1.f / 32768.f) * zaznamHl * env;
          L[i] += x; R[i] += x; SL[i] += x * 0.15f; SR[i] += x * 0.15f;
        }
      }
      for (int i = 0; i < n; i++) {
        float dl, dr;
        dozvuk(SL[i], SR[i], dl, dr);
        float l = L[i] + dl, r = R[i] + dr;
        // konec: dozneni do ticha
        const float t = (float)(z + i) / FS;
        const float konec = 1.f - hladce(44.2f + cas::POSUN5, 44.95f + cas::POSUN5, t);   // s obrazem; po 59 s uz je ticho
        l *= 0.80f * konec; r *= 0.80f * konec;
        // mekky limiter
        l = l / (1.f + std::fabs(l) * 0.35f); r = r / (1.f + std::fabs(r) * 0.35f);
        l = std::max(-1.f, std::min(1.f, l)); r = std::max(-1.f, std::min(1.f, r));
        pcm[(size_t)(z + i) * 2] = (int16_t)std::lrint(l * 32000.f);
        pcm[(size_t)(z + i) * 2 + 1] = (int16_t)std::lrint(r * 32000.f);
      }
      hotovoRamcu.store(z + n, std::memory_order_release);
    }
  }

 private:
  int celkemRamcu = 0;
  std::atomic<int> hotovoRamcu{0};
  std::atomic<bool> stop{false};
  std::vector<int16_t> pcm;
  std::vector<int16_t> zaznam;          // B307: zvuk Reneho hry (Pitt-Kitt) z emulatoru
  float zaznamOd = 0.f, zaznamHl = 0.f;
  std::vector<Nota> part;
  size_t dalsiNota = 0;
  std::vector<Hlas> hlasy;
  std::vector<Hrom> hromList;
  Dozvuk dozvuk;

  // ---- stav souvislych vrstev ----
  Sum sA{11}, sB{23}, sC{37}, sD{41}, sE{53}, sF{67}, sG{79};
  float fHum = 0, fWhine = 0;
  Svf crtSnih, crtBzuk, mistnost, vrataRachot, vrataMotor, vitrL, vitrR, vitrL2, vitrR2, svist, destL, destR, destTl, dronF;
  Lp1 crtLp, kapkyLp;
  Pomala poryv{101, 0.35f}, poryv2{202, 0.9f}, poryvP{303, 0.23f};
  float fMotor = 0, fDron1 = 0, fDron2 = 0, fDron3 = 0;
  float kapka = 0, kapkaF = 0, kapkaFaze = 0;
  float retez = 0, retezF = 0;
  struct Ptak { float od, f0, pan, rych; uint32_t seed; };
  std::vector<Ptak> ptaci;
  float ptFaze[6] = {0, 0, 0, 0, 0, 0};
  Svf ptLp[6];

  // ================= partitura =================
  void pridej(float od, float delka, float f, float hl, float pan, float dz, Druh d) {
    part.push_back({od, delka, f, hl, pan, dz, d});
  }
  void akord(float od, float delka, std::initializer_list<int> m, float hl, float dz, Druh d) {
    int k = 0;
    for (int x : m) { pridej(od, delka, midi((float)x), hl, (k % 2 ? 0.25f : -0.25f) * (k ? 1.f : 0.f), dz, d); k++; }
  }

  void sestavPartituru() {
    part.clear();
    // ---- SCENA 1: cvakani zarovky (stejny vzor jako blikani v obraze) ----
    {
      static const int vz[15] = {1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1};
      int pred = 0;
      for (int k = 0; k < 15; k++) {
        if (vz[k] != pred) pridej(1.15f + k / 20.f, 0.05f, 0, vz[k] ? 0.30f : 0.16f, 0.2f, 0.25f, CVAK);
        pred = vz[k];
      }
      pridej(1.90f, 0.05f, 0, 0.22f, 0.2f, 0.25f, CVAK);
    }
    // ---- SCENA 2: nic (vrstvy a hromy) ----
    // ---- SCENA 3: plochy Dm - Bb - F - C a zvonky ----
    {
      struct A { float od; int m[4]; int bas; };
      static const A ak[4] = {{17.55f, {50, 53, 57, 62}, 38}, {19.95f, {50, 53, 58, 62}, 34},
                              {22.35f, {48, 53, 57, 60}, 41}, {24.75f, {48, 52, 55, 60}, 36}};
      for (int i = 0; i < 4; i++) {
        const float d = 2.6f + (i == 3 ? 0.5f : 0.f);
        for (int j = 0; j < 4; j++) pridej(ak[i].od, d, midi((float)ak[i].m[j]), 0.085f, j % 2 ? 0.3f : -0.3f, 0.55f, PAD);
        pridej(ak[i].od, d, midi((float)ak[i].bas), 0.07f, 0.f, 0.3f, PAD);
        pridej(ak[i].od, d, midi((float)ak[i].m[0] + 12), 0.035f, 0.f, 0.6f, PAD);
      }
      // zvonky (rozlozene akordy, kazdych 0.6 s)
      static const int arp[4][4] = {{62, 65, 69, 74}, {62, 65, 70, 74}, {60, 65, 69, 72}, {60, 64, 67, 72}};
      int k = 0;
      for (float t = 18.8f; t < 27.0f; t += 0.6f, k++) {
        int ai = t < 19.95f ? 0 : (t < 22.35f ? 1 : (t < 24.75f ? 2 : 3));
        static const int vzor[8] = {0, 2, 1, 3, 2, 1, 3, 2};
        pridej(t, 1.8f, midi((float)arp[ai][vzor[k % 8]] + 12), 0.075f, (k % 2 ? 0.45f : -0.45f), 0.7f, ZVON);
      }
      // tep zivota od rozsviceni kontrolky
      static const float tepy[4] = {25.6f, 26.4f, 27.15f, 27.62f};
      for (float tp : tepy) { pridej(tp, 0.3f, 52.f, 0.55f, 0.f, 0.15f, TEP); pridej(tp + 0.21f, 0.3f, 48.f, 0.36f, 0.f, 0.15f, TEP); }
    }
    // ---- SCENA 4 (B307): neonove pruty, zhmotneni konzoli N&P, CRT a defile ----
    {
      using namespace cas;
      const float BEAT = 0.5f;
      // klavesy se rozzari: narustajici trpyt a tichy akord
      pridej(KLAVESY - 0.1f, PRUTY - KLAVESY + 0.1f, 0, 0.26f, 0.f, 0.45f, RISER);
      akord(KLAVESY + 0.05f, 1.6f, {74, 77, 81, 86}, 0.045f, 0.75f, ZVON);
      // groove 120 BPM od 27,5 do odletu kamery (kopak, virbl, hihat, basa v osminach, arpeggio jako POKEY)
      struct A { float od; int koren; int m[3]; };
      static const A ak[4] = {{27.5f, 38, {62, 65, 69}}, {29.5f, 34, {62, 65, 70}}, {30.5f, 41, {65, 69, 72}}, {31.5f, 36, {64, 67, 72}}};
      auto akordV = [&](float t) { int i = 0; for (int k = 0; k < 4; k++) if (t >= ak[k].od - 0.001f) i = k; return i; };
      const float KONEC_GROOVE = PRULET + 0.35f;
      for (int b = 0; ; b++) {
        const float t = 27.5f + b * BEAT;
        if (t >= KONEC_GROOVE) break;
        pridej(t, 0.35f, 0, 0.95f, 0.f, 0.06f, KOPAK);
        if (b % 2 == 1) pridej(t, 0.25f, 0, 0.50f, 0.05f, 0.18f, VIRBL);
        pridej(t + 0.25f, 0.06f, 0, 0.16f, 0.35f, 0.05f, HAT);
        for (int o = 0; o < 2; o++) {
          const float to = t + o * 0.25f;
          if (to >= KONEC_GROOVE) break;
          const A &a = ak[akordV(to)];
          pridej(to, 0.22f, midi((float)(a.koren + ((b * 2 + o) % 4 == 2 ? 12 : 0))), 0.30f, 0.f, 0.05f, BASA);
        }
      }
      static const int vzor[8] = {0, 1, 2, 1, 2, 0, 1, 2};
      for (int k = 0; ; k++) {
        const float t = 27.5f + k * 0.125f;
        if (t >= KONEC_GROOVE) break;
        const A &a = ak[akordV(t)];
        const int okt = (k / 8) % 2 ? 12 : 0;
        // pri kresleni obrysu arpeggio hlasiteji a o oktavu vys (rychle "kresleni")
        const bool drat = t >= DRAT && t < ODHAL;
        pridej(t, 0.11f, midi((float)a.m[vzor[k % 8]] + okt + (drat ? 12 : 0)), drat ? 0.11f : 0.075f, (k % 2 ? 0.4f : -0.4f), 0.25f, ARP);
      }
      // pruty vystreli z klaves: vlevo / vpravo stridave
      for (int i = 0; i < 7; i++) {
        const float st = PRUTY + i * 0.055f;
        pridej(st, 0.55f, 260.f + i * 35.f, 0.17f, (i % 2 ? 0.55f : -0.55f), 0.35f, ZAP);
      }
      // prilet ke konzolim: rana a dve cinknuti (vlevo azurova, vpravo ruzova)
      pridej(PRILET, 1.0f, 0, 0.42f, 0.f, 0.35f, NARAZ);
      pridej(PRILET + 0.02f, 1.3f, midi(86.f), 0.12f, -0.7f, 0.6f, PING);
      pridej(PRILET + 0.07f, 1.3f, midi(89.f), 0.12f, 0.7f, 0.6f, PING);
      // zhmotneni: narustajici sum, pak zapnuti konzoli (dva zvonky)
      pridej(ODHAL - 0.05f, ODHAL_KONEC - ODHAL + 0.05f, 0, 0.30f, 0.f, 0.45f, RISER);
      akord(ODHAL_KONEC, 1.8f, {69, 74, 77, 81}, 0.07f, 0.7f, ZVON);
      akord(ODHAL_KONEC + 0.15f, 1.8f, {74, 78, 81, 86}, 0.06f, 0.7f, ZVON);
      akord(TROJICE, 2.0f, {50, 57, 62, 65}, 0.07f, 0.55f, PAD);
      // odlesky na emblemech N&P
      pridej(30.75f, 1.2f, midi(98.f), 0.04f, -0.5f, 0.8f, ZVON);
      pridej(31.70f, 1.2f, midi(101.f), 0.04f, 0.5f, 0.8f, ZVON);
      // prulet ke CRT: zrychleny virbl a svist
      for (float t = PRULET; t < CRT_ZAP;) {
        pridej(t, 0.1f, 0, 0.18f + 0.2f * hladce(PRULET, CRT_ZAP, t), 0.f, 0.2f, VIRBL);
        t += t < PRULET + 0.2f ? 0.0625f : 0.03125f;
      }
      pridej(PRULET - 0.1f, CRT_ZAP - PRULET + 0.12f, 0, 0.30f, 0.f, 0.4f, RISER);
      // CRT se zapne: cvak, hluboke "dum" a sum, ktery dozni do obrazu
      pridej(CRT_ZAP, 0.05f, 0, 0.30f, -0.1f, 0.2f, CVAK);
      pridej(CRT_ZAP, 1.2f, 70.f, 0.55f, 0.f, 0.1f, PODBAS);
      pridej(CRT_ZAP, STRIH_ATARI - CRT_ZAP + 0.05f, 0, 0.22f, 0.f, 0.15f, SNIH);
      // ---- defile ----
      // B309: defile trva 17,85 s (drive 3,85 s) - kazda ukazka ma vlastni hudbu
      // v tempu 120 BPM (takt = 2 s), at to neni zmet strihu, ale klidne prehlidka.
      const float TAKT = 2.0f;
      // ATARI: rana a znelka jako z POKEY (ctvercove tony nahoru)
      pridej(STRIH_ATARI, 1.0f, 0, 0.40f, 0.f, 0.3f, NARAZ);
      {
        static const int ZN[6] = {72, 76, 79, 84, 88, 91};
        for (int i = 0; i < 6; i++) pridej(STRIH_ATARI + 0.02f + i * 0.05f, 0.16f, midi((float)ZN[i]), 0.10f, (i % 2 ? 0.2f : -0.2f), 0.2f, ARP);
        // titulek a kapitola hry: tichy "POKEY" doprovod (arpeggio Dm, basa, hihat)
        static const int AR[8] = {62, 65, 69, 74, 69, 65, 62, 57};
        for (int k = 0; ; k++) {
          const float t = STRIH_ATARI + 0.5f + k * 0.125f;
          if (t >= ATARI_JIZDA - 0.05f) break;
          pridej(t, 0.11f, midi((float)AR[k % 8] + ((k / 8) % 2 ? 12.f : 0.f)), 0.085f, (k % 2 ? 0.35f : -0.35f), 0.25f, ARP);
          if (k % 4 == 0) pridej(t, 0.22f, midi(k % 16 < 8 ? 38.f : 34.f), 0.28f, 0.f, 0.05f, BASA);
          if (k % 4 == 2) pridej(t, 0.05f, 0, 0.13f, 0.3f, 0.05f, HAT);
        }
        pridej(ATARI_KAP, 0.10f, midi(84.f), 0.09f, 0.f, 0.2f, ARP);
        pridej(ATARI_KAP + 0.08f, 0.16f, midi(79.f), 0.09f, 0.f, 0.2f, ARP);
        // jizda: hraje zvuk hry (POKEY ze zaznamu, nastavZaznam) - hudba mlci,
        // jen na konci narustajici sum do strihu
        pridej(STRIH_16 - 0.55f, 0.55f, 0, 0.20f, 0.f, 0.35f, RISER);
      }
      // 16 BIT: sum prepnuti, FM fanfara, pak cela FM skladba (basa, bici, melodie)
      pridej(STRIH_16, 0.10f, 0, 0.30f, 0.f, 0.1f, SNIH);
      pridej(STRIH_16, 0.9f, 0, 0.36f, 0.f, 0.3f, NARAZ);
      akord(STRIH_16 + 0.02f, 0.35f, {58, 62, 65, 70}, 0.11f, 0.3f, FMPLECH);
      akord(STRIH_16 + 0.40f, 0.30f, {60, 64, 67, 72}, 0.10f, 0.3f, FMPLECH);
      akord(STRIH_16 + 0.75f, 0.42f, {62, 65, 70, 74}, 0.11f, 0.35f, FMPLECH);
      {
        // akordy po pul taktu: Bb C | Dm Bb | C A  (koreny basy)
        static const int KOR[6] = {34, 36, 38, 34, 36, 33};
        static const int VZ[8] = {0, 0, 12, 0, 7, 0, 12, 7};      // FM basa v osminach
        const float konec16 = STRIH_32 - 0.5f;
        for (int b = 0; ; b++) {                                    // osminy
          const float t = STRIH_16 + b * 0.25f;
          if (t >= konec16) break;
          const int pul = std::min(5, (int)((t - STRIH_16) / (TAKT * 0.5f)));
          pridej(t, 0.22f, midi((float)(KOR[pul] + VZ[b % 8])), 0.24f, 0.f, 0.05f, FMBASA);
          const int dob = b / 2;                                    // dob v taktu (0..3)
          if (b % 2 == 0 && (dob % 4 == 0 || dob % 4 == 2)) pridej(t, 0.32f, 0, 0.70f, 0.f, 0.06f, KOPAK);
          if (b % 2 == 0 && (dob % 4 == 1 || dob % 4 == 3)) pridej(t, 0.25f, 0, 0.36f, 0.05f, 0.18f, VIRBL);
          if (b % 8 == 7) pridej(t, 0.30f, 0, 0.40f, 0.f, 0.06f, KOPAK);   // predraz pred dalsim taktem
          pridej(t + 0.125f, 0.05f, 0, 0.11f, 0.35f, 0.05f, HAT);
        }
        // plechy na zacatku kazdeho pultaktu (krome fanfary)
        for (int pul = 2; pul < 6; pul++) {
          static const int CH[6][3] = {{58, 62, 65}, {60, 64, 67}, {62, 65, 69}, {58, 62, 65}, {60, 64, 67}, {57, 61, 64}};
          const float t = STRIH_16 + pul * TAKT * 0.5f;
          for (int k2 = 0; k2 < 3; k2++) pridej(t, 0.30f, midi((float)CH[pul][k2]), 0.06f, (k2 - 1) * 0.3f, 0.3f, FMPLECH);
        }
        // melodie (FM plech, o oktavu vys) - hrdinsky motiv 16bit strilecky
        struct Mel { float od, d; int m; };
        static const Mel MEL[] = {
            {1.20f, 0.25f, 79}, {1.45f, 0.25f, 81}, {1.70f, 0.30f, 84},
            {2.00f, 0.50f, 86}, {2.50f, 0.25f, 89}, {2.75f, 0.25f, 88}, {3.00f, 0.50f, 86}, {3.50f, 0.45f, 81},
            {4.00f, 0.50f, 82}, {4.50f, 0.25f, 84}, {4.75f, 0.25f, 86}, {5.00f, 0.45f, 88}, {5.50f, 0.50f, 85}};
        for (const Mel &m : MEL) pridej(STRIH_16 + m.od, m.d, midi((float)m.m), 0.085f, 0.f, 0.32f, FMPLECH);
        // virbl do strihu
        for (float t = STRIH_32 - 0.5f; t < STRIH_32 - 0.02f; t += 0.0625f)
          pridej(t, 0.1f, 0, 0.16f + 0.18f * hladce(STRIH_32 - 0.5f, STRIH_32, t), 0.f, 0.2f, VIRBL);
        pridej(STRIH_32 - 0.5f, 0.5f, 0, 0.24f, 0.f, 0.35f, RISER);
      }
      // 32 BIT: sum prepnuti, velky akord (CD kvalita), pak nocni zavod: breakbeat,
      // basa, plochy Dm9 - Bbmaj7 - C - Dm - A, zvonky a svist pri predjeti
      pridej(STRIH_32, 0.10f, 0, 0.30f, 0.f, 0.1f, SNIH);
      pridej(STRIH_32, 0.9f, 0, 0.36f, 0.f, 0.3f, NARAZ);
      akord(STRIH_32 + 0.02f, 1.6f, {50, 57, 62, 65, 69}, 0.075f, 0.6f, PAD);
      akord(STRIH_32 + 0.05f, 1.3f, {78, 81, 86, 90}, 0.05f, 0.7f, ZVON);
      {
        struct Pl { float od, d; int m[5]; int bas; };
        static const Pl PL[5] = {{0.0f, 2.0f, {50, 57, 62, 65, 69}, 38}, {2.0f, 1.0f, {46, 53, 58, 62, 65}, 34},
                                 {3.0f, 1.0f, {48, 55, 60, 64, 67}, 36}, {4.0f, 1.0f, {50, 57, 62, 65, 69}, 38},
                                 {5.0f, 1.25f, {45, 52, 57, 61, 64}, 33}};
        for (int i = 1; i < 5; i++)
          for (int k2 = 0; k2 < 5; k2++)
            pridej(STRIH_32 + PL[i].od, PL[i].d + 0.3f, midi((float)PL[i].m[k2]), 0.058f, (k2 % 2 ? 0.3f : -0.3f), 0.55f, PAD);
        auto plochaV = [&](float t) { int i = 0; for (int k2 = 0; k2 < 5; k2++) if (t - STRIH_32 >= PL[k2].od - 0.001f) i = k2; return i; };
        const float konec32 = STRIHY_KONEC - 0.55f;
        // breakbeat (16 sestnactin na takt): kopak 1, 2a, 3 / virbl 2, 4 / hihat sestnactiny
        static const int KOP[16] = {1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 1};
        static const int VIR[16] = {0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0};
        for (int k = 0; ; k++) {
          const float t = STRIH_32 + k * 0.125f;
          if (t >= konec32) break;
          const int s16 = k % 16;
          if (KOP[s16]) pridej(t, 0.32f, 0, 0.72f, 0.f, 0.06f, KOPAK);
          if (VIR[s16]) pridej(t, 0.25f, 0, 0.40f, 0.05f, 0.22f, VIRBL);
          pridej(t, 0.04f, 0, (s16 % 4 == 2) ? 0.12f : 0.06f, (k % 2 ? 0.4f : -0.2f), 0.05f, HAT);
          if (k % 2 == 0) {                                          // basa v osminach
            const Pl &pl = PL[plochaV(t)];
            pridej(t, 0.20f, midi((float)(pl.bas + ((k / 2) % 4 == 3 ? 12 : 0))), 0.26f, 0.f, 0.05f, BASA);
          }
          if (k % 4 == 0) {                                          // zvonky: rozlozeny akord nahore
            const Pl &pl = PL[plochaV(t)];
            pridej(t + 0.02f, 0.9f, midi((float)pl.m[1 + (k / 4) % 4] + 24.f), 0.028f, ((k / 4) % 2 ? 0.5f : -0.5f), 0.7f, ZVON);
          }
        }
        // predjeti (ukazka: modry souper ~2,0 s vpravo, zluty ~4,4 s vlevo): svist kolem
        pridej(STRIH_32 + 2.0f - 0.45f, 0.5f, 0, 0.22f, 0.45f, 0.3f, RISER);
        pridej(STRIH_32 + 4.41f - 0.45f, 0.5f, 0, 0.22f, -0.45f, 0.3f, RISER);
        pridej(STRIH_32 + 4.41f, 0.9f, midi(93.f), 0.035f, 0.f, 0.7f, ZVON);    // 1. misto
      }
      // zaverecna vlna do loga: virbl se zrychluje
      for (float t = STRIHY_KONEC - 0.55f; t < SCENA5 - 0.05f;) {
        const float d = t < STRIHY_KONEC - 0.2f ? 0.125f : (t < STRIHY_KONEC + 0.1f ? 0.0625f : 0.03125f);
        pridej(t, 0.12f, 0, 0.20f + 0.25f * hladce(STRIHY_KONEC - 0.55f, SCENA5, t), 0.f, 0.2f, VIRBL);
        t += d;
      }
      pridej(STRIHY_KONEC - 0.4f, SCENA5 - STRIHY_KONEC + 0.35f, 0, 0.35f, 0.f, 0.4f, RISER);
    }
    // ---- SCENA 5: gong, D dur, zlate zvonky ----
    // B309: stejne jako B304-B308, jen o cas::POSUN5 (14 s) pozdeji
    {
      const float P = cas::POSUN5;
      pridej(P + 37.3f, 0.55f, 0, 0.30f, 0.f, 0.5f, RISER);
      pridej(P + 37.85f, 7.0f, midi(38.f), 0.55f, 0.f, 0.55f, GONG);
      pridej(P + 37.85f, 3.0f, 62.f, 0.85f, 0.f, 0.1f, PODBAS);
      pridej(P + 37.85f, 1.5f, 0, 0.45f, 0.f, 0.6f, NARAZ);
      for (int m : {50, 54, 57, 62, 69}) pridej(P + 38.0f, 7.0f, midi((float)m), 0.075f, (m % 2 ? 0.3f : -0.3f), 0.6f, PAD);
      pridej(P + 38.0f, 7.0f, midi(38.f), 0.06f, 0.f, 0.3f, PAD);
      // trpyt pri odlesku loga
      static const int tr[6] = {86, 90, 93, 98, 102, 105};
      for (int k = 0; k < 14; k++) {
        pridej(P + 38.5f + k * 0.1f, 1.4f, midi((float)tr[k % 6] - (k >= 6 ? 0.f : 12.f)), 0.035f, (k % 2 ? 0.5f : -0.5f), 0.8f, ZVON);
        pridej(P + 41.8f + k * 0.09f, 1.4f, midi((float)tr[k % 6] - (k >= 6 ? 0.f : 12.f)), 0.030f, (k % 2 ? -0.5f : 0.5f), 0.8f, ZVON);
      }
      // titulky: tiche klavirni tony
      static const float kdy[6] = {39.2f, 39.85f, 40.35f, 41.05f, 41.95f, 42.75f};
      static const int ton[6] = {69, 74, 78, 76, 74, 69};
      for (int i = 0; i < 6; i++) pridej(P + kdy[i], 2.5f, midi((float)ton[i]), 0.11f, 0.f, 0.55f, PIANO);
      akord(P + 43.4f, 2.2f, {62, 66, 69, 74}, 0.05f, 0.7f, PIANO);
    }
    std::sort(part.begin(), part.end(), [](const Nota &a, const Nota &b) { return a.od < b.od; });
    dalsiNota = 0;
    hlasy.clear();
    hlasy.reserve(96);

    // ---- hromy ----
    hromList = {
        {9.55f + 1.6f, 0.45f, -0.4f, 0.3f, 0.f, 0.f, 4.5f},  {11.25f + 1.3f, 0.40f, 0.5f, 0.3f, 0.f, 0.f, 4.0f},
        {12.55f + 0.7f, 0.75f, 0.45f, 0.6f, 0.5f, 0.f, 5.0f}, {14.15f + 1.1f, 0.55f, -0.5f, 0.4f, 0.f, 0.f, 4.5f},
        {14.95f + 0.5f, 0.85f, -0.3f, 0.7f, 0.65f, 0.f, 5.0f}, {15.95f + 0.9f, 0.60f, 0.3f, 0.45f, 0.f, 0.f, 4.0f},
        {16.94f, 1.30f, 0.05f, 1.0f, 1.0f, 0.f, 7.5f},
        {19.95f + 1.4f, 0.40f, -0.4f, 0.4f, 0.f, 1.f, 4.0f},   {22.75f + 0.8f, 0.60f, -0.3f, 0.6f, 0.3f, 1.f, 4.5f},
        {25.35f + 1.6f, 0.35f, -0.4f, 0.3f, 0.f, 1.f, 3.5f},
    };
    hromStav.assign(hromList.size(), HromStav());
    for (size_t i = 0; i < hromStav.size(); i++) hromStav[i].init(1000u + (uint32_t)i * 77u);

    // ---- ptaci (zpivaji za vraty a pak venku, nez prijde boure) ----
    ptaci = {{7.7f, 3400.f, -0.6f, 1.0f, 5u}, {8.3f, 4200.f, 0.5f, 1.3f, 9u}, {8.9f, 2800.f, 0.2f, 0.8f, 13u},
             {9.6f, 5200.f, -0.3f, 1.6f, 17u}, {10.4f, 3700.f, 0.7f, 1.1f, 21u}, {11.0f, 4600.f, -0.7f, 1.4f, 29u}};
    for (int i = 0; i < 6; i++) ptLp[i].nastav(7000.f, 0.7f);

    crtSnih.nastav(3200.f, 0.8f);
    crtBzuk.nastav(600.f, 0.7f);
    crtLp.nastav(1800.f);
    mistnost.nastav(140.f, 0.7f);
    vrataMotor.nastav(320.f, 1.2f);
    destTl.nastav(1300.f, 0.6f);
    kapkyLp.nastav(5000.f);
  }

  // ================= souvisle vrstvy =================
  void vrstvy(int z, int n, float *L, float *R, float *SL, float *SR) {
    for (int i = 0; i < n; i++) {
      const float t = (float)(z + i) / FS;
      float l = 0, r = 0, sl = 0, sr = 0;
      const int ctl = (z + i) & 31;     // ridici kmitocet (filtry s promennym kmitoctem)

      // --- stara obrazovka: 50 Hz sit, snih, piskot radku (z dalky) ---
      const float crt = hladce(0.25f, 1.1f, t) * (1.f - hladce(5.0f, 8.2f, t));
      if (crt > 0.f) {
        fHum += 50.f / FS; if (fHum > 1.f) fHum -= 1.f;
        fWhine += 15734.f / FS; if (fWhine > 1.f) fWhine -= 1.f;
        const float am = 0.85f + 0.15f * sinus(t * 0.7f);
        float hum = sinus(fHum) * 0.5f + sinus(fHum * 2.f) * 0.38f + sinus(fHum * 3.f) * 0.22f + sinus(fHum * 5.f) * 0.10f +
                    sinus(fHum * 7.f) * 0.05f;
        crtSnih(sA());
        float snih = crtSnih.bp * 0.35f * (0.8f + 0.2f * sinus(t * 23.f));
        // bzuk: tvrdsi 100 Hz s vyssimi harmonickymi (aby byl slyset i z reproduktoru telefonu)
        float bz = sinus(fHum * 2.f);
        bz = bz * bz * bz;
        crtBzuk(bz);
        float x = crtLp(hum * am * 0.9f + snih + crtBzuk.bp * 0.55f) + sinus(fWhine) * 0.012f;
        x *= crt * 0.15f;
        l += x * 1.25f; r += x * 0.75f;              // televize je vlevo
        sl += x * 0.6f; sr += x * 0.6f;
      }
      // --- ticho mistnosti ---
      const float mis = okno(t, 0.8f, 2.0f, 6.0f, 8.0f);
      if (mis > 0.f) {
        mistnost(sB());
        const float x = mistnost.lp * 0.22f * mis;
        l += x; r += x;
      }
      // --- vrata: motor, rachot, retez, bouchnuti nahore ---
      const float vr = okno(t, 4.3f, 4.6f, 6.9f, 7.25f);
      if (vr > 0.f) {
        fMotor += (58.f + 6.f * hladce(4.4f, 5.0f, t)) / FS; if (fMotor > 1.f) fMotor -= 1.f;
        vrataMotor(pila(fMotor, 60.f / FS) + 0.3f * sC());
        float m = vrataMotor.lp * 0.35f;
        if (ctl == 0) vrataRachot.nastav(1100.f + 400.f * sinus(t * 3.1f), 4.f);
        vrataRachot(sC());
        float tr = std::pow(std::fabs(sinus(t * 11.f)), 6.f) * 0.8f + 0.2f;
        float rach = vrataRachot.bp * tr * 0.45f;
        // retez: kratke kovove cinknuti ~14x za vterinu
        retezF += 14.3f / FS;
        if (retezF >= 1.f) { retezF -= 1.f; retez = 1.f; }
        retez *= 0.9985f;
        float cink = sinus(t * 2650.f) * retez * retez * 0.25f;
        float x = (m + rach + cink) * vr * 0.55f;
        l += x * 0.9f; r += x * 1.1f;
        sl += x * 0.35f; sr += x * 0.35f;
      }
      // --- vitr (ven vrata, venku silny, uvnitr pokoje tlumeny) ---
      float vit = hladce(4.6f, 6.8f, t) * 0.35f + hladce(6.8f, 9.5f, t) * 0.65f;
      vit *= 1.f + 0.4f * hladce(11.f, 16.f, t);
      if (t > 17.28f) vit = 0.30f * (1.f - hladce(26.8f, 28.4f, t));
      if (vit > 0.f) {
        const float g = 0.55f + 0.45f * poryv() + 0.25f * poryv2();
        if (ctl == 0) {
          const float fc = (t > 17.28f ? 380.f : 520.f) + (t > 17.28f ? 120.f : 650.f) * std::max(0.f, g);
          vitrL.nastav(fc, 0.9f); vitrR.nastav(fc * 1.13f, 0.9f);
          vitrL2.nastav(fc * 2.6f, 2.5f); vitrR2.nastav(fc * 2.3f, 2.5f);
        }
        const float nl = sD(), nr = sE();
        vitrL(nl); vitrR(nr); vitrL2(nl); vitrR2(nr);
        const float gp = 0.6f + 0.4f * poryvP();
        const float hlas = t > 17.28f ? 0.55f : 0.90f;
        float xl = (vitrL.lp * 0.9f + vitrL2.bp * 0.30f) * vit * std::max(0.1f, g) * hlas;
        float xr = (vitrR.lp * 0.9f + vitrR2.bp * 0.30f) * vit * std::max(0.1f, g * gp) * hlas;
        l += xl; r += xr;
        sl += xl * 0.25f; sr += xr * 0.25f;
      }
      // --- svist: kamera vyleti ven ---
      const float sv = okno(t, 6.9f, 8.3f, 8.5f, 9.0f);
      if (sv > 0.f) {
        if (ctl == 0) svist.nastav(300.f + 2600.f * hladce(7.0f, 8.6f, t), 1.6f);
        svist(sF());
        const float x = svist.bp * sv * 0.32f;
        l += x; r += x;
        sl += x * 0.3f; sr += x * 0.3f;
      }
      // --- ptaci ---
      const float ptk = okno(t, 7.4f, 8.6f, 12.4f, 13.9f);
      if (ptk > 0.f) {
        for (int p = 0; p < 6; p++) {
          const Ptak &pt = ptaci[p];
          if (t < pt.od) continue;
          float x = zpev(p, t - pt.od, pt);
          x *= ptk * (t < 8.7f ? 0.55f : 1.f);
          l += x * (1.f - pt.pan) * 0.5f; r += x * (1.f + pt.pan) * 0.5f;
          sl += x * 0.45f; sr += x * 0.45f;
        }
      }
      // --- tichy temny dron (atmosfera houstne) ---
      const float dr = hladce(11.0f, 15.5f, t) * (t < 16.94f ? 1.f : 0.f);
      if (dr > 0.f) {
        fDron1 += midi(26.f) / FS; if (fDron1 > 1.f) fDron1 -= 1.f;
        fDron2 += midi(33.f) * 1.003f / FS; if (fDron2 > 1.f) fDron2 -= 1.f;
        fDron3 += midi(38.f) * 0.997f / FS; if (fDron3 > 1.f) fDron3 -= 1.f;
        if (ctl == 0) dronF.nastav(180.f + 420.f * hladce(11.f, 16.9f, t), 1.3f);
        dronF(pila(fDron1, midi(26.f) / FS) + pila(fDron2, midi(33.f) / FS) * 0.7f + pila(fDron3, midi(38.f) / FS) * 0.5f);
        const float x = dronF.lp * dr * 0.11f;
        l += x; r += x;
        sl += x * 0.4f; sr += x * 0.4f;
      }
      // --- dest: venku zive, uvnitr za oknem tlumene ---
      float de = hladce(13.4f, 16.2f, t);
      bool uvnitr = t > 17.28f;
      if (uvnitr) de = 0.55f * (1.f - hladce(26.5f, 28.4f, t));
      if (de > 0.f) {
        if (ctl == 0) { destL.nastav(2600.f, 0.6f); destR.nastav(2900.f, 0.6f); }
        destL(sG()); destR(sA());
        float xl = destL.bp, xr = destR.bp;
        // kapky: kratka cinknuti nahodne
        kapkaFaze += 1.f / FS;
        if (sB() > 0.9965f - de * 0.003f) { kapka = 1.f; kapkaF = 2200.f + 2600.f * std::fabs(sC()); }
        kapka *= 0.993f;
        float kp = sinus(t * kapkaF) * kapka * 0.4f;
        float x = (xl * 0.6f + kp);
        float y = (xr * 0.6f + kp * 0.6f);
        if (uvnitr) { destTl(x + y); x = destTl.lp * 1.2f; y = x; }
        x *= de * 0.32f; y *= de * 0.32f;
        l += x; r += y;
        sl += x * 0.2f; sr += y * 0.2f;
      }
      L[i] += l; R[i] += r; SL[i] += sl; SR[i] += sr;
    }
  }

  // zpev ptaka: kratke trylky a hvizdy (frekvencni oblouky)
  float zpev(int p, float tt, const Ptak &pt) {
    // fraze 1.4 - 2.6 s, pauzy mezi nimi
    const float delkaFraze = 1.9f / pt.rych + 0.6f;
    const int fr = (int)(tt / delkaFraze);
    const float tf = tt - fr * delkaFraze;
    uint32_t h = pt.seed * 2654435761u + (uint32_t)fr * 40503u;
    auto nahoda = [&](int k) { uint32_t x = h ^ (uint32_t)(k * 2246822519u); x ^= x >> 15; x *= 2654435761u; x ^= x >> 13; return (x & 0xFFFF) / 65535.f; };
    const int tonu = 3 + (int)(nahoda(1) * 5.f);
    const float delTon = 0.07f + nahoda(2) * 0.10f;
    const int k = (int)(tf / (delTon * 1.35f));
    if (k >= tonu) return 0.f;
    const float tk = tf - k * delTon * 1.35f;
    if (tk > delTon) return 0.f;
    const float u = tk / delTon;
    const int typ = (int)(nahoda(10 + k) * 3.f);
    float f;
    if (typ == 0) f = pt.f0 * (1.f + 0.35f * u);                       // vzestupny hvizd
    else if (typ == 1) f = pt.f0 * (1.25f - 0.4f * u);                 // sestupny
    else f = pt.f0 * (1.05f + 0.12f * sinus(u * 3.f + 0.25f));          // obloucek
    f *= 0.92f + 0.16f * nahoda(20 + k);
    ptFaze[p] += f / FS; if (ptFaze[p] > 1.f) ptFaze[p] -= 1.f;
    float tryl = typ == 2 ? (0.6f + 0.4f * sinus(tk * 38.f)) : 1.f;
    const float ob = std::sin(3.14159f * u);
    float x = sinus(ptFaze[p]) * ob * ob * tryl * 0.075f;
    ptLp[p](x);
    return ptLp[p].lp;
  }

  // ================= noty =================
  void noty(int z, int n, float *L, float *R, float *SL, float *SR) {
    const float t0 = (float)z / FS, t1 = (float)(z + n) / FS;
    while (dalsiNota < part.size() && part[dalsiNota].od < t1) {
      Hlas h;
      h.n = part[dalsiNota++];
      h.sum = Sum(1234u + (uint32_t)dalsiNota * 7919u);
      hlasy.push_back(h);
    }
    (void)t0;
    for (auto &h : hlasy) {
      for (int i = 0; i < n; i++) {
        const float t = (float)(z + i) / FS;
        const float tt = t - h.n.od;
        if (tt < 0.f) continue;
        float x = vzorek(h, tt);
        if (!h.zivy) break;
        const float pl = 0.5f - h.n.pan * 0.5f, pr = 0.5f + h.n.pan * 0.5f;
        const float xl = x * pl * 2.f, xr = x * pr * 2.f;
        L[i] += xl; R[i] += xr;
        SL[i] += xl * h.n.dozvuk; SR[i] += xr * h.n.dozvuk;
      }
    }
    hlasy.erase(std::remove_if(hlasy.begin(), hlasy.end(), [](const Hlas &h) { return !h.zivy; }), hlasy.end());
  }

  float vzorek(Hlas &h, float tt) {
    const Nota &n = h.n;
    const bool ctl = (h.ridici++ & 31) == 0;
    switch (n.druh) {
      case PAD: {
        const float att = 0.9f, rel = 1.4f;
        if (tt > n.delka + rel) { h.zivy = false; return 0.f; }
        float env = hladce(0.f, att, tt) * (1.f - hladce(n.delka, n.delka + rel, tt));
        float x = 0;
        static const float det[3] = {0.996f, 1.0f, 1.0045f};
        for (int k = 0; k < 3; k++) {
          const float f = n.f * det[k];
          h.faze[k] += f / FS; if (h.faze[k] > 1.f) h.faze[k] -= 1.f;
          x += pila(h.faze[k], f / FS);
        }
        h.faze[3] += n.f * 0.5f / FS; if (h.faze[3] > 1.f) h.faze[3] -= 1.f;
        x = x * 0.33f + sinus(h.faze[3]) * 0.35f;
        if (ctl) h.flt.nastav(std::min(n.f * 5.f, 2400.f) * (0.6f + 0.4f * env), 0.8f);
        h.flt(x);
        return h.flt.lp * env * n.hl;
      }
      case ZVON: {        // FM zvon: nosna 1, modulator 3.5, index klesa
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float idx = 3.2f * std::exp(-tt * 6.f) + 0.4f;
        h.faze[1] += n.f * 3.5f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        const float x = sinus(h.faze[0] + idx * sinus(h.faze[1]) * 0.159f);
        const float env = std::exp(-tt * 3.2f / std::max(0.4f, n.delka * 0.6f)) * hladce(0.f, 0.004f, tt) *
                          (1.f - hladce(n.delka * 0.8f, n.delka, tt));
        return x * env * n.hl;
      }
      case PIANO: {       // mekky klavir (FM 1:1 + oktava), dlouhe doznivani
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float idx = 1.6f * std::exp(-tt * 4.f) + 0.15f;
        h.faze[1] += n.f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        h.faze[0] += n.f * 1.0015f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        h.faze[2] += n.f * 2.001f / FS; if (h.faze[2] > 1.f) h.faze[2] -= 1.f;
        float x = sinus(h.faze[0] + idx * sinus(h.faze[1]) * 0.159f) + sinus(h.faze[2]) * 0.25f * std::exp(-tt * 3.f);
        const float env = std::exp(-tt * 1.6f) * hladce(0.f, 0.006f, tt) * (1.f - hladce(n.delka * 0.7f, n.delka, tt));
        return x * env * n.hl;
      }
      case ARP: {         // ctverec jako POKEY, rychle zhasina
        if (tt > n.delka + 0.05f) { h.zivy = false; return 0.f; }
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        float x = ctverec(h.faze[0], n.f / FS, 0.5f - 0.2f * std::exp(-tt * 20.f));
        if (ctl) h.flt.nastav(800.f + 5200.f * std::exp(-tt * 25.f), 1.1f);
        h.flt(x);
        const float env = std::exp(-tt * 18.f) * hladce(0.f, 0.002f, tt);
        return h.flt.lp * env * n.hl;
      }
      case BASA: {
        if (tt > n.delka + 0.05f) { h.zivy = false; return 0.f; }
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        h.faze[1] += n.f * 0.5f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        float x = pila(h.faze[0], n.f / FS) * 0.7f + sinus(h.faze[1]) * 0.6f;
        if (ctl) h.flt.nastav(160.f + 1400.f * std::exp(-tt * 22.f), 1.4f);
        h.flt(x);
        const float env = hladce(0.f, 0.004f, tt) * (1.f - hladce(n.delka - 0.04f, n.delka, tt));
        return h.flt.lp * env * n.hl;
      }
      case KOPAK: {
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float f = 45.f + 120.f * std::exp(-tt * 38.f);
        h.faze[0] += f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        float x = sinus(h.faze[0]) * std::exp(-tt * 9.f);
        x += h.sum() * std::exp(-tt * 300.f) * 0.35f;      // cvak paliky
        return std::tanh(x * 1.6f) * n.hl;
      }
      case VIRBL: {
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        if (ctl) h.flt.nastav(1800.f, 0.9f);
        h.flt(h.sum());
        h.faze[0] += 190.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        float x = h.flt.bp * 1.2f * std::exp(-tt * 18.f) + sinus(h.faze[0]) * 0.5f * std::exp(-tt * 30.f);
        return x * n.hl;
      }
      case HAT: {
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        if (ctl) h.flt.nastav(8000.f, 0.8f);
        h.flt(h.sum());
        return h.flt.hp * std::exp(-tt * 80.f) * n.hl;
      }
      case ZAP: {         // energie vystreli: FM vzestup
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float u = tt / n.delka;
        const float f = n.f * std::pow(9.f, u);
        h.faze[0] += f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        h.faze[1] += f * 1.5f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        float x = sinus(h.faze[0] + sinus(h.faze[1]) * 0.25f);
        const float env = hladce(0.f, 0.02f, tt) * (1.f - u) * (1.f - u);
        return x * env * n.hl;
      }
      case PING: {        // zasah: jasne cinknuti s trpytem
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        h.faze[1] += n.f * 2.76f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        float x = sinus(h.faze[0]) + sinus(h.faze[1]) * 0.4f * std::exp(-tt * 8.f);
        if (ctl) h.flt.nastav(9000.f, 0.7f);
        h.flt(h.sum());
        x += h.flt.hp * 0.5f * std::exp(-tt * 25.f);
        return x * std::exp(-tt * 4.5f) * n.hl;
      }
      case NARAZ: {       // strih: hluboka rana + sum (cinel)
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float f = 38.f + 70.f * std::exp(-tt * 14.f);
        h.faze[0] += f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        float x = sinus(h.faze[0]) * std::exp(-tt * 5.f) * 0.9f;
        if (ctl) h.flt.nastav(5500.f, 0.6f);
        h.flt(h.sum());
        x += h.flt.hp * 0.45f * std::exp(-tt * 6.f);
        return x * n.hl;
      }
      case FMPLECH: {     // Sega: dvou-operatorova FM "plechova" sekce
        if (tt > n.delka + 0.2f) { h.zivy = false; return 0.f; }
        const float idx = 2.6f * hladce(0.f, 0.03f, tt) * (0.55f + 0.45f * std::exp(-tt * 5.f));
        h.faze[1] += n.f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        float x = sinus(h.faze[0] + idx * sinus(h.faze[1]) * 0.159f);
        const float env = hladce(0.f, 0.025f, tt) * (1.f - hladce(n.delka, n.delka + 0.2f, tt));
        return x * env * n.hl;
      }
      case KLAP: {        // klavesa Atari: kratke cvaknuti
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        if (ctl) h.flt.nastav(3200.f, 2.0f);
        h.flt(h.sum());
        h.faze[0] += 1800.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        return (h.flt.bp * 1.5f + sinus(h.faze[0]) * 0.3f) * std::exp(-tt * 160.f) * n.hl;
      }
      case CVAK: {        // zarovka: elektricke cvaknuti a bzuk
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        if (ctl) h.flt.nastav(2500.f, 1.5f);
        h.flt(h.sum());
        h.faze[0] += 100.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        float x = h.flt.bp * 2.f * std::exp(-tt * 140.f) + pila(h.faze[0], 100.f / FS) * 0.25f * std::exp(-tt * 50.f);
        return x * n.hl;
      }
      case GONG: {        // neharmonicke parcialni tony, dlouhy dozvuk
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        static const float P[6] = {1.0f, 2.01f, 2.76f, 4.07f, 5.40f, 8.93f};
        static const float A[6] = {1.0f, 0.6f, 0.45f, 0.30f, 0.22f, 0.12f};
        static const float D[6] = {0.45f, 0.6f, 0.8f, 1.1f, 1.5f, 2.4f};
        float x = 0;
        for (int k = 0; k < 6; k++) {
          h.faze[k] += n.f * P[k] / FS; if (h.faze[k] > 1.f) h.faze[k] -= 1.f;
          x += sinus(h.faze[k]) * A[k] * std::exp(-tt * D[k]);
        }
        const float env = hladce(0.f, 0.008f, tt) * (1.f - hladce(n.delka - 1.5f, n.delka, tt));
        return x * env * n.hl * 0.45f;
      }
      case PODBAS: {      // hluboky propad pod logem
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float f = 30.f + (n.f - 30.f) * std::exp(-tt * 2.2f);
        h.faze[0] += f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        return sinus(h.faze[0]) * std::exp(-tt * 1.4f) * hladce(0.f, 0.01f, tt) * n.hl;
      }
      case TEP: {         // tlukot srdce (zivot v Atari)
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float f = n.f * (1.f + 0.6f * std::exp(-tt * 30.f));
        h.faze[0] += f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        return sinus(h.faze[0]) * std::exp(-tt * 14.f) * hladce(0.f, 0.005f, tt) * n.hl;
      }
      case PIP: {
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        return sinus(h.faze[0]) * okno(tt, 0.f, 0.004f, n.delka - 0.01f, n.delka) * n.hl;
      }
      case SNIH: {        // B307: sum obrazovky (zapnuti, prepnuti kanalu) - pasmovy sum a bzuk 50 Hz
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        if (ctl) h.flt.nastav(3000.f, 0.7f);
        h.flt(h.sum());
        h.faze[0] += 50.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        const float env = hladce(0.f, 0.004f, tt) * (1.f - hladce(n.delka * 0.6f, n.delka, tt));
        return (h.flt.bp * 1.3f + pila(h.faze[0], 50.f / FS) * 0.12f) * env * n.hl;
      }
      case FMBASA: {      // B307: FM basa (2 operatory, kratky utlum indexu) - 16bit styl
        if (tt > n.delka + 0.05f) { h.zivy = false; return 0.f; }
        const float idx = 4.0f * std::exp(-tt * 14.f) + 0.8f;
        h.faze[1] += n.f / FS; if (h.faze[1] > 1.f) h.faze[1] -= 1.f;
        h.faze[0] += n.f / FS; if (h.faze[0] > 1.f) h.faze[0] -= 1.f;
        const float x = sinus(h.faze[0] + idx * sinus(h.faze[1]) * 0.159f);
        const float env = hladce(0.f, 0.003f, tt) * (1.f - hladce(n.delka - 0.03f, n.delka + 0.05f, tt));
        return x * env * n.hl;
      }
      case RISER: {       // narustajici sum (cinel pozpatku)
        if (tt > n.delka) { h.zivy = false; return 0.f; }
        const float u = tt / n.delka;
        if (ctl) h.flt.nastav(600.f + 7000.f * u * u, 1.2f);
        h.flt(h.sum());
        return h.flt.bp * std::pow(u, 2.5f) * n.hl * 1.6f;
      }
    }
    h.zivy = false;
    return 0.f;
  }

  // ================= hromy =================
  struct HromStav {
    Sum s{1};
    Svf val, rana, prask, stred;
    Pomala mod{1, 3.f}, mod2{2, 7.f};
    void init(uint32_t seed) {
      s = Sum(seed); mod = Pomala(seed + 1, 2.6f); mod2 = Pomala(seed + 2, 6.5f);
      rana.nastav(90.f, 0.9f); prask.nastav(2500.f, 0.6f); stred.nastav(520.f, 0.6f);
    }
  };
  std::vector<HromStav> hromStav;

  void hromy(int z, int n, float *L, float *R, float *SL, float *SR) {
    for (size_t k = 0; k < hromList.size(); k++) {
      const Hrom &h = hromList[k];
      HromStav &st = hromStav[k];
      for (int i = 0; i < n; i++) {
        const float t = (float)(z + i) / FS;
        const float tt = t - h.t;
        if (tt < 0.f || tt > h.delka) continue;
        const float x0 = st.s();
        // valeni: tmavy sum s menicim se kmitoctem a nepravidelnou silou
        if (((z + i) & 31) == 0) {
          float fc = (h.tlumeny > 0.5f ? 140.f : 260.f) + 180.f * std::max(0.f, st.mod2()) * (1.f - tt / h.delka);
          st.val.nastav(fc, 0.85f);
        }
        st.val(x0);
        st.stred(x0);
        float roll = 0.55f + 0.45f * st.mod() + 0.25f * st.mod2();
        roll = std::max(0.f, roll);
        const float envV = hladce(0.f, 0.25f + 0.2f * (1.f - h.rana), tt) * std::exp(-tt * 3.0f / h.delka) *
                           (1.f - hladce(h.delka * 0.75f, h.delka, tt));
        float x = (st.val.lp * 1.6f + st.stred.bp * (h.tlumeny > 0.5f ? 0.15f : 0.45f)) * roll * envV;
        // rana (blizky uder)
        st.rana(x0);
        x += st.rana.lp * h.rana * std::exp(-tt * 3.5f) * hladce(0.f, 0.01f, tt) * 3.0f;
        // prask (blizke blesky) - par trhanych zablesku
        if (h.prask > 0.f && tt < 0.6f) {
          st.prask(x0);
          float pr = std::exp(-tt * 9.f) + 0.6f * std::exp(-std::fabs(tt - 0.11f) * 40.f) + 0.4f * std::exp(-std::fabs(tt - 0.23f) * 50.f);
          x += st.prask.hp * pr * h.prask * 0.9f;
        }
        x *= h.sila * 0.70f;
        const float pl = 0.5f - h.pan * 0.5f, pr2 = 0.5f + h.pan * 0.5f;
        L[i] += x * pl * 2.f; R[i] += x * pr2 * 2.f;
        SL[i] += x * 0.5f; SR[i] += x * 0.5f;
      }
    }
  }
};

}  // namespace zv
}  // namespace napdemo
