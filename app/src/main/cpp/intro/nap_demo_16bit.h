// nap_demo_16bit.h
// ====================================================================
//  B307: 16BITOVA UKAZKA PRO UVODNI DEMO (konzole N&P 16)
//
//  Vlastni hra ve stylu 16bitovych konzoli - kresli ji procesor presne
//  tak, jak to delal videocip te doby: 320 x 224 bodu, barvy jen z 9bitove
//  palety (8 urovni na slozku), vrstvy pozadi posouvane po RADCICH
//  (paralaxa, vlnici se more), sprity s obrysem a hra simulovana snimek
//  po snimku (60 / s). Zadna cizi grafika - lod, drony i vybuchy se
//  "vyrenderuji" pri startu z 3D tvaru (jako predrenderovane sprity tehdy),
//  pozadi je spocitane.
//
//  Obraz je cista funkce cisla snimku: telefon i TV kresli totez.
// ====================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace napdemo {

// Pismo 8x8 pro 16/32bit ukazky: z ROM pisma Atari, ztucnene (radek | radek
// posunuty o bod) - vypada jako pismo arkad. 96 znaku od mezery.
inline void pismoTucne(uint8_t p[96][8]) {
  const unsigned char *rom = NAP_OS_ROM + 0x2000;
  for (int c = 0; c < 96; c++) {
    const int a = 0x20 + c;
    const int kod = (a >= 0x20 && a <= 0x5F) ? a - 0x20 : ((a >= 0x60 && a <= 0x7F) ? a : 0);
    for (int y = 0; y < 8; y++) { const uint8_t b = rom[kod * 8 + y]; p[c][y] = (uint8_t)(b | (b >> 1)); }
  }
}

class Hra16 {
 public:
  static constexpr int W = 320, H = 224;
  static constexpr int HORIZONT = 120;     // radek obzoru (more pod nim)
  std::vector<uint32_t> obr;               // RGBA (R v nejnizsim bajtu), W x H

  void init() {
    obr.assign((size_t)W * H, 0xFF000000u);
    postavLod();
    postavDron();
    postavMoznost();
    postavPismo();
    postavMesto();
    simN = -1;
  }

  // n = cislo snimku od zacatku ukazky (60 snimku / s)
  void snimek(int n) {
    if (n < 0) n = 0;
    if (n > 600) n = 600;
    simuluj(n);
    pozadi(n);
    sprity(n);
    popredi(n);
    hud(n);
  }

 private:
  // ---------------- barvy ----------------
  // 9bitova paleta: 8 urovni na slozku
  static uint8_t uroven(int u) {
    static const uint8_t L[8] = {0, 36, 73, 109, 146, 182, 219, 255};
    return L[u < 0 ? 0 : (u > 7 ? 7 : u)];
  }
  static uint32_t b9(int r, int g, int b) {
    return 0xFF000000u | ((uint32_t)uroven(b) << 16) | ((uint32_t)uroven(g) << 8) | (uint32_t)uroven(r);
  }
  // spojita barva (0..1) -> nejblizsi 9bitova, s volitelnym rozkladem (dither)
  static uint32_t b9f(float r, float g, float b, float d = 0.f) {
    auto q = [&](float x) { return (int)std::floor(x * 7.f + 0.5f + d); };
    return b9(q(r), q(g), q(b));
  }

  // ---------------- sprity ----------------
  struct Sprite {
    int w = 0, h = 0;
    std::vector<uint32_t> px;   // 0 = pruhledne
  };
  Sprite lod[3];        // naklon nahoru / rovne / dolu
  Sprite dron[4];       // otaceni prstence
  Sprite moznost[4];    // svitici koule (pomocnici lodi) - pulzuje
  uint8_t pismo[96][8]; // 8x8, tucne

  // ---- 3D tvar -> sprite (jako predrenderovana grafika) ----
  struct V { float x, y, z; };
  static V v(float x, float y, float z) { return {x, y, z}; }
  static V sub(V a, V b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
  static float dot(V a, V b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
  static float len(V a) { return std::sqrt(dot(a, a)); }
  static V nrm(V a) { float l = len(a); return l > 1e-6f ? V{a.x / l, a.y / l, a.z / l} : V{0, 0, 1}; }
  static float smin(float a, float b, float k) {
    float h = std::max(k - std::fabs(a - b), 0.f) / k;
    return std::min(a, b) - h * h * k * 0.25f;
  }
  static float roundCone(V p, V a, V b, float r1, float r2) {
    // zaobleny kuzel mezi body a (polomer r1) a b (r2)
    V ba = sub(b, a);
    float l2 = dot(ba, ba), rr = r1 - r2, a2 = l2 - rr * rr, il2 = 1.f / l2;
    V pa = sub(p, a);
    float y = dot(pa, ba), z = y - l2;
    V xv = {pa.x * l2 - ba.x * y, pa.y * l2 - ba.y * y, pa.z * l2 - ba.z * y};
    float x2 = dot(xv, xv), y2 = y * y * l2, z2 = z * z * l2;
    float k = (rr > 0 ? 1.f : -1.f) * rr * rr * x2;
    if ((z > 0 ? 1.f : -1.f) * a2 * z2 > k) return std::sqrt(x2 + z2) * il2 - r2;
    if ((y > 0 ? 1.f : -1.f) * a2 * y2 < k) return std::sqrt(x2 + y2) * il2 - r1;
    return (std::sqrt(x2 * a2 * il2) + y * rr) * il2 - r1;
  }
  static float elipsoid(V p, V r) {
    float k0 = len({p.x / r.x, p.y / r.y, p.z / r.z});
    float k1 = len({p.x / (r.x * r.x), p.y / (r.y * r.y), p.z / (r.z * r.z)});
    return k0 * (k0 - 1.f) / std::max(k1, 1e-6f);
  }
  static float kvadr(V p, V b, float r) {
    V q = {std::fabs(p.x) - b.x, std::fabs(p.y) - b.y, std::fabs(p.z) - b.z};
    V m = {std::max(q.x, 0.f), std::max(q.y, 0.f), std::max(q.z, 0.f)};
    return len(m) + std::min(std::max(q.x, std::max(q.y, q.z)), 0.f) - r;
  }

  // Lod (bokorys, nos doprava): trup, kabina, kridla, kyl, motor.
  // mat: 0 trup 1 kabina 2 kridlo 3 motor 4 cerveny pruh
  float lodSdf(V p, float naklon, int &mat) const {
    const float c = std::cos(naklon), s = std::sin(naklon);
    V q = {p.x, p.y * c - p.z * s, p.y * s + p.z * c};
    float trup = roundCone(q, v(-15.f, 0.2f, 0), v(16.f, -0.9f, 0), 3.7f, 0.7f);
    float trup2 = roundCone(q, v(-17.f, 0.f, 0), v(-7.f, 0.f, 0), 3.3f, 3.5f);
    trup = smin(trup, trup2, 1.5f);
    float kab = elipsoid(sub(q, v(5.0f, 2.5f, 0)), v(6.8f, 2.4f, 2.3f));
    V w = sub(q, v(-6.f, -1.6f, 0));
    float sik = w.x + std::fabs(w.z) * 0.55f;
    float kridlo = kvadr({sik, w.y, w.z}, v(5.5f, 0.45f, 9.5f), 0.3f);
    kridlo = std::max(kridlo, -(w.x + 11.f));
    V kk = sub(q, v(8.5f, -0.9f, 0));
    float kachna = kvadr({kk.x + std::fabs(kk.z) * 0.6f, kk.y, kk.z}, v(2.2f, 0.3f, 4.2f), 0.2f);
    V f = sub(q, v(-12.5f, 3.7f, 0));
    float kyl = kvadr({f.x + f.y * 0.7f, f.y, f.z}, v(2.7f, 2.7f, 0.45f), 0.2f);
    float motor = roundCone(q, v(-19.5f, -0.2f, 0), v(-15.5f, -0.2f, 0), 2.6f, 3.0f);
    float d = trup; mat = 0;
    if (kab < d) { d = kab; mat = 1; }
    float kr = std::min(kridlo, kachna);
    if (kr < d) { d = kr; mat = 2; }
    if (kyl < d) { d = kyl; mat = 0; }
    if (motor < d) { d = motor; mat = 3; }
    if (mat == 0 && q.y > -1.7f && q.y < -0.5f && q.x > -11.f && q.x < 12.5f && q.z > 1.4f) mat = 4;
    if (mat == 2 && std::fabs(sik + 2.f) < 0.9f) mat = 4;         // pruh na kridle
    return d;
  }
  // Dron: koule s prstencem a rudym okem (diva se doleva, na hrace)
  float dronSdf(V p, float uhel, int &mat) const {
    float koule = len(p) - 5.6f;
    const float c = std::cos(0.35f), s = std::sin(0.35f);
    V r = {p.x * c - p.y * s, p.x * s + p.y * c, p.z};
    float qx = std::sqrt(r.x * r.x + r.z * r.z) - 7.6f;
    float prsten = std::sqrt(qx * qx + r.y * r.y) - 1.0f;
    float oko = len(sub(p, v(-4.6f, 0.6f, 2.0f))) - 2.0f;
    float d = koule; mat = 0;
    if (prsten < d) { d = prsten; mat = 1; }
    if (oko < d + 0.6f) { d = std::min(d, oko); mat = 2; }
    if (mat == 1) {
      float a = std::atan2(r.z, r.x) + uhel;
      if (std::fmod(a * 6.f / 6.2831853f + 100.f, 1.f) < 0.5f) mat = 3;
    }
    return d;
  }

  template <class F>
  void renderSprite(Sprite &sp, int w, int h, float cx, float cy, float meritko, F sdf, const uint32_t ramp[][6], int emis) {
    sp.w = w; sp.h = h;
    sp.px.assign((size_t)w * h, 0);
    const V L = nrm(v(-0.45f, 0.75f, 0.55f));
    for (int y = 0; y < h; y++)
      for (int x = 0; x < w; x++) {
        int hit = 0; float jas = 0; int mat = 0;
        for (int sy = 0; sy < 2; sy++)
          for (int sx = 0; sx < 2; sx++) {
            const float u = ((float)x + 0.25f + sx * 0.5f - cx) * meritko;
            const float vv = (cy - ((float)y + 0.25f + sy * 0.5f)) * meritko;
            float z = 14.f; int m = 0; bool zas = false;
            for (int i = 0; i < 72; i++) {
              float d = sdf(v(u, vv, z), m);
              if (d < 0.02f) { zas = true; break; }
              z -= std::max(d, 0.05f);
              if (z < -14.f) break;
            }
            if (!zas) continue;
            hit++;
            const float e = 0.05f; int mm;
            V n = nrm(v(sdf(v(u + e, vv, z), mm) - sdf(v(u - e, vv, z), mm), sdf(v(u, vv + e, z), mm) - sdf(v(u, vv - e, z), mm),
                        sdf(v(u, vv, z + e), mm) - sdf(v(u, vv, z - e), mm)));
            float dif = std::max(dot(n, L), 0.f);
            V hv = nrm(v(L.x, L.y, L.z + 1.f));
            float spec = std::pow(std::max(dot(n, hv), 0.f), 24.f);
            jas += 0.22f + 0.78f * dif + spec * 0.9f;
            mat = m;
          }
        if (hit < 2) continue;
        jas /= (float)hit;
        int k = (int)std::floor(jas * 5.2f);
        k = std::max(0, std::min(5, k));
        if (mat == emis) k = std::max(k, 4);
        sp.px[(size_t)y * w + x] = ramp[mat][k];
      }
    obrys(sp);
  }
  void obrys(Sprite &sp) {
    std::vector<uint32_t> o = sp.px;
    const uint32_t OBRYS = b9(0, 0, 1);
    for (int y = 0; y < sp.h; y++)
      for (int x = 0; x < sp.w; x++) {
        if (sp.px[(size_t)y * sp.w + x]) continue;
        bool vedle = false;
        for (int k = 0; k < 4 && !vedle; k++) {
          int xx = x + (k == 0) - (k == 1), yy = y + (k == 2) - (k == 3);
          if (xx >= 0 && yy >= 0 && xx < sp.w && yy < sp.h && sp.px[(size_t)yy * sp.w + xx]) vedle = true;
        }
        if (vedle) o[(size_t)y * sp.w + x] = OBRYS;
      }
    sp.px.swap(o);
  }

  void postavLod() {
    static uint32_t R[5][6];
    R[0][0] = b9(1, 1, 2); R[0][1] = b9(2, 2, 3); R[0][2] = b9(3, 3, 4); R[0][3] = b9(4, 4, 5); R[0][4] = b9(6, 6, 6); R[0][5] = b9(7, 7, 7);
    R[1][0] = b9(0, 1, 3); R[1][1] = b9(0, 2, 4); R[1][2] = b9(1, 4, 6); R[1][3] = b9(2, 5, 7); R[1][4] = b9(5, 7, 7); R[1][5] = b9(7, 7, 7);
    R[2][0] = b9(1, 1, 2); R[2][1] = b9(1, 2, 3); R[2][2] = b9(2, 3, 4); R[2][3] = b9(3, 4, 5); R[2][4] = b9(5, 5, 6); R[2][5] = b9(6, 6, 7);
    R[3][0] = b9(1, 1, 1); R[3][1] = b9(2, 2, 2); R[3][2] = b9(3, 3, 3); R[3][3] = b9(4, 4, 4); R[3][4] = b9(5, 5, 5); R[3][5] = b9(6, 6, 6);
    R[4][0] = b9(2, 0, 0); R[4][1] = b9(4, 0, 0); R[4][2] = b9(5, 1, 1); R[4][3] = b9(6, 1, 1); R[4][4] = b9(7, 3, 2); R[4][5] = b9(7, 5, 4);
    static const float NAKLON[3] = {0.42f, 0.f, -0.42f};
    for (int i = 0; i < 3; i++) {
      const float nk = NAKLON[i];
      renderSprite(lod[i], 58, 28, 30.f, 14.f, 0.68f, [&](V p, int &m) { return lodSdf(p, nk, m); }, R, -1);
    }
  }
  void postavDron() {
    static uint32_t R[4][6];
    R[0][0] = b9(1, 1, 1); R[0][1] = b9(2, 1, 2); R[0][2] = b9(3, 2, 3); R[0][3] = b9(4, 3, 5); R[0][4] = b9(6, 5, 6); R[0][5] = b9(7, 7, 7);
    R[1][0] = b9(1, 1, 0); R[1][1] = b9(3, 2, 0); R[1][2] = b9(5, 3, 0); R[1][3] = b9(6, 5, 1); R[1][4] = b9(7, 6, 3); R[1][5] = b9(7, 7, 5);
    R[2][0] = b9(7, 1, 1); R[2][1] = b9(7, 1, 1); R[2][2] = b9(7, 2, 1); R[2][3] = b9(7, 3, 2); R[2][4] = b9(7, 5, 4); R[2][5] = b9(7, 6, 5);
    R[3][0] = b9(0, 0, 1); R[3][1] = b9(1, 1, 2); R[3][2] = b9(2, 1, 3); R[3][3] = b9(3, 2, 4); R[3][4] = b9(4, 4, 5); R[3][5] = b9(5, 5, 6);
    for (int i = 0; i < 4; i++) {
      const float u = i * 0.2618f;
      renderSprite(dron[i], 26, 24, 13.f, 12.f, 0.70f, [&](V p, int &m) { return dronSdf(p, u, m); }, R, 2);
    }
  }
  // pomocna koule: svitici jadro s prstenci (pulzuje - 4 faze)
  void postavMoznost() {
    for (int f = 0; f < 4; f++) {
      Sprite &s = moznost[f];
      s.w = s.h = 14;
      s.px.assign(14 * 14, 0);
      const float puls = 0.85f + 0.15f * std::sin(f * 1.5708f);
      for (int y = 0; y < 14; y++)
        for (int x = 0; x < 14; x++) {
          const float dx = x - 6.5f, dy = y - 6.5f, d = std::sqrt(dx * dx + dy * dy) / puls;
          if (d > 6.2f) continue;
          uint32_t c;
          if (d < 2.0f) c = b9(7, 7, 7);
          else if (d < 3.4f) c = b9(5, 7, 7);
          else if (d < 4.8f) c = b9(2, 5, 7);
          else c = b9(1, 2, 5);
          // odlesk vlevo nahore
          if (dx < -1.f && dy < -1.f && d > 2.f && d < 4.f) c = b9(7, 7, 7);
          s.px[(size_t)y * 14 + x] = c;
        }
      obrys(s);
    }
  }
  void postavPismo() { pismoTucne(pismo); }

  // ---------------- mesto na obzoru ----------------
  struct Dum { int x, w, h, okna; };
  std::vector<Dum> domy;
  static constexpr int MESTO_SIR = 640;          // mesto se opakuje po 640 bodech
  void postavMesto() {
    domy.clear();
    uint32_t s = 7771;
    auto r = [&]() { s ^= s << 13; s ^= s >> 17; s ^= s << 5; return (s >> 8) & 0xFFFF; };
    int x = 0;
    while (x < MESTO_SIR) {
      Dum d; d.x = x; d.w = 9 + (int)(r() % 20); d.h = 6 + (int)(r() % 20);
      if (r() % 6 == 0) d.h += 16;           // obcas vez
      d.okna = (int)(r() & 0xFFFF);
      domy.push_back(d);
      x += d.w + (int)(r() % 5);
    }
  }

  // ---------------- simulace hry ----------------
  struct Dr { float x, y, by, amp, faze; bool zivy; };
  struct Las { float x, y; bool maly; };
  struct Vyb { float x, y; int od; };
  struct Str { float x, y, vx, vy; };          // strely dronu (ruzove koule)
  std::vector<Dr> drony;
  std::vector<Las> lasery;
  std::vector<Vyb> vybuchy;
  std::vector<Str> strely;
  std::vector<float> stopaX, stopaY;     // kudy lod letela (pomocne koule jdou po ni)
  float lodX = 0, lodY = 0, lodVy = 0;
  int skore = 0;
  int simN = -1;

  void simuluj(int n) {
    if (n == simN) return;
    drony.clear(); lasery.clear(); vybuchy.clear(); strely.clear(); stopaX.clear(); stopaY.clear();
    skore = 128400;
    // vlna 1: had zprava, kmita pres drahu lodi; vlna 2: vys; vlna 3: nizko nad vodou
    for (int i = 0; i < 7; i++) drony.push_back({334.f + i * 24.f, 0.f, 146.f, 42.f, i * 0.55f, true});
    for (int i = 0; i < 6; i++) drony.push_back({480.f + i * 22.f, 0.f, 92.f, 24.f, 1.3f + i * 0.5f, true});
    for (int i = 0; i < 5; i++) drony.push_back({600.f + i * 26.f, 0.f, 184.f, 18.f, 2.6f + i * 0.6f, true});
    float lx = 26.f, ly = 150.f, vy = 0.f;
    for (int f = 0; f <= n; f++) {
      for (auto &d : drony) {
        if (!d.zivy) continue;
        d.x -= 2.5f;
        d.y = d.by + d.amp * std::sin(d.x * 0.030f + d.faze);
        // obcas vystreli na lod
        if (d.x < W - 30 && d.x > 120 && ((f + (int)(d.faze * 10.f)) % 47) == 0) {
          const float dx = lx - d.x, dy = ly - d.y, l = std::sqrt(dx * dx + dy * dy) + 1e-3f;
          strely.push_back({d.x - 8.f, d.y, dx / l * 3.2f, dy / l * 3.2f});
        }
      }
      for (auto &st : strely) { st.x += st.vx; st.y += st.vy; }
      strely.erase(std::remove_if(strely.begin(), strely.end(), [](const Str &q) { return q.x < -10 || q.y < -10 || q.y > H + 10; }), strely.end());
      // lod: najede zleva a miri na nejblizsi dron pred sebou (pomalu, at akce projde pres obraz)
      const float cilX = 70.f + 8.f * std::sin(f * 0.04f);
      lx += (cilX - lx) * 0.06f;
      float cilY = 150.f, nej = 1e9f;
      for (auto &d : drony)
        if (d.zivy && d.x > lx + 30.f && d.x < W - 30.f && d.x - lx < nej) { nej = d.x - lx; cilY = d.y; }
      cilY = std::max(88.f, std::min(200.f, cilY));
      const float chce = std::max(-1.6f, std::min(1.6f, (cilY - ly) * 0.08f));
      vy += (chce - vy) * 0.3f;
      ly += vy;
      stopaX.push_back(lx); stopaY.push_back(ly);
      // strelba v davkach: 14 snimku strili, 10 ne
      if (f >= 2 && ((f - 2) % 24) < 14 && (f - 2) % 4 == 0) {
        lasery.push_back({lx + 26.f, ly - 2.f, false});
        lasery.push_back({lx + 24.f, ly + 4.f, false});
        for (int k = 1; k <= 2; k++) {
          float ox, oy; moznostPoloha(k, (int)stopaX.size() - 1, ox, oy);
          lasery.push_back({ox + 8.f, oy, true});
        }
      }
      for (auto &l : lasery) l.x += 12.f;
      for (auto &l : lasery) {
        if (l.x > W + 20) continue;
        for (auto &d : drony) {
          if (!d.zivy || d.x > W - 6) continue;
          if (std::fabs(l.x - d.x) < 13.f && std::fabs(l.y - d.y) < 11.f) {
            d.zivy = false;
            vybuchy.push_back({d.x, d.y, f});
            skore += 500;
            l.x = 9999.f;
            break;
          }
        }
      }
      lasery.erase(std::remove_if(lasery.begin(), lasery.end(), [](const Las &l) { return l.x > W + 20; }), lasery.end());
    }
    lodX = lx; lodY = ly; lodVy = vy;
    simN = n;
  }
  // pomocna koule k (1, 2) jde po stope lodi se zpozdenim
  void moznostPoloha(int k, int i, float &x, float &y) const {
    const int zp = std::max(0, i - 10 * k);
    x = stopaX.empty() ? 0.f : stopaX[(size_t)zp] - 18.f * k;
    y = stopaY.empty() ? 0.f : stopaY[(size_t)zp] + (k == 1 ? -15.f : 15.f);
  }

  // ---------------- kresleni ----------------
  static float hash1(int x) {
    uint32_t h = (uint32_t)x * 2654435761u; h ^= h >> 15; h *= 2246822519u; h ^= h >> 13;
    return (float)(h & 0xFFFF) / 65535.f;
  }
  static float sumX(float x) {        // hladky 1D sum
    int i = (int)std::floor(x); float f = x - (float)i; f = f * f * (3.f - 2.f * f);
    return hash1(i) * (1.f - f) + hash1(i + 1) * f;
  }
  inline void bod(int x, int y, uint32_t c) {
    if ((unsigned)x < (unsigned)W && (unsigned)y < (unsigned)H) obr[(size_t)y * W + x] = c;
  }

  static constexpr int SX = 214, SY = HORIZONT - 26, SR = 44;   // slunce

  void pozadi(int n) {
    const float t = (float)n;
    // ---- obloha: barva po radcich (rastrovy prechod - typicky pro 16bit) ----
    for (int y = 0; y < HORIZONT; y++) {
      const float k = (float)y / HORIZONT;          // 0 nahore .. 1 u obzoru
      float r, g, b;
      if (k < 0.45f) { float u = k / 0.45f; r = 0.06f + 0.26f * u; g = 0.03f + 0.06f * u; b = 0.16f + 0.22f * u; }
      else if (k < 0.75f) { float u = (k - 0.45f) / 0.30f; r = 0.32f + 0.48f * u; g = 0.09f + 0.10f * u; b = 0.38f - 0.06f * u; }
      else { float u = (k - 0.75f) / 0.25f; r = 0.80f + 0.20f * u; g = 0.19f + 0.50f * u; b = 0.32f - 0.12f * u; }
      uint32_t *radek = &obr[(size_t)y * W];
      for (int x = 0; x < W; x++) radek[x] = b9f(r, g, b, ((x + y) & 1) ? 0.18f : -0.18f);
    }
    // hvezdy (jen nahore)
    for (int i = 0; i < 70; i++) {
      int x = (int)(hash1(i * 7 + 1) * 960.f - t * 0.12f) % W; if (x < 0) x += W;
      int y = 14 + (int)(hash1(i * 7 + 3) * 44.f);
      if (hash1(i * 7 + 5 + (n / 9)) > 0.25f) bod(x, y, hash1(i) > 0.7f ? b9(5, 6, 7) : b9(4, 4, 6));
    }
    // ---- slunce s pruhy ----
    for (int y = SY - SR; y < HORIZONT; y++) {
      const float dy = (float)(y - SY);
      const float sir = std::sqrt(std::max(0.f, (float)(SR * SR) - dy * dy));
      if (sir <= 0.f) continue;
      if (dy > -4.f) {               // mezery v dolni casti (cim niz, tim sirsi), pomalu jedou dolu
        const float per = 7.f, faze = std::fmod((dy + 4.f) + t * 0.22f, per);
        if (faze < 1.f + (dy + 4.f) * 0.12f) continue;
      }
      const float u = (dy + SR) / (2.f * SR);
      const uint32_t c = b9f(1.0f, 0.97f - 0.62f * u, 0.50f - 0.38f * u, ((y & 1) ? 0.2f : -0.2f));
      for (int x = (int)(SX - sir); x <= (int)(SX + sir); x++) bod(x, y, c);
    }
    // ---- mraky: tenke pasy osvetlene zespodu (kazdy pas jinou rychlosti) ----
    struct Pas { int y0, y1; float rych; float sila; };
    static const Pas PASY[4] = {{24, 31, 0.35f, 0.5f}, {42, 49, 0.55f, 0.6f}, {60, 67, 0.8f, 0.75f}, {76, 82, 1.1f, 0.9f}};
    for (const Pas &p : PASY) {
      for (int y = p.y0; y <= p.y1; y++) {
        const float vy = (float)(y - p.y0) / (p.y1 - p.y0);
        for (int x = 0; x < W; x++) {
          const float wx = (x + t * p.rych) * 0.045f;
          float m = sumX(wx) * 0.65f + sumX(wx * 2.7f + 5.f) * 0.35f;
          m -= 0.46f + std::fabs(vy - 0.5f) * 0.7f;
          if (m <= 0.f) continue;
          const uint32_t c = b9f(0.45f + 0.5f * vy * p.sila, 0.18f + 0.32f * vy * p.sila, 0.42f + 0.1f * vy, ((x + y) & 1) ? 0.22f : -0.22f);
          if (m > 0.06f || ((x ^ y) & 1)) bod(x, y, c);
        }
      }
    }
    // ---- vzdalene hory (pomalu) ----
    {
      const float s1 = t * 0.5f;
      for (int x = 0; x < W; x++) {
        const float wx = (x + s1);
        const float h = 10.f + 10.f * sumX(wx * 0.018f) + 6.f * sumX(wx * 0.05f + 3.f) + 2.f * sumX(wx * 0.2f);
        const int top = HORIZONT - (int)h;
        for (int y = top; y < HORIZONT; y++) {
          uint32_t c = (y == top) ? b9(4, 2, 4) : b9(2, 1, 3);
          if (y > top && y < top + 3 && ((x + y) & 1)) c = b9(3, 1, 3);
          bod(x, y, c);
        }
      }
    }
    // ---- mesto (stredni rychlost) s okny a blikajicimi svetly ----
    {
      const int s2 = (int)(t * 1.6f);
      for (const Dum &d : domy) {
        for (int rep = -1; rep <= 1; rep++) {
          const int x0 = d.x - (s2 % MESTO_SIR) + rep * MESTO_SIR;
          if (x0 + d.w < 0 || x0 >= W) continue;
          const int top = HORIZONT - d.h;
          for (int y = top; y < HORIZONT; y++)
            for (int x = x0; x < x0 + d.w; x++) {
              if (x < 0 || x >= W) continue;
              uint32_t c = b9(1, 0, 2);
              if (x == x0 + d.w - 1) c = b9(0, 0, 1);
              if (y == top && x < x0 + d.w - 1) c = b9(3, 1, 3);
              const int ox = x - x0 - 2, oy = y - top - 3;
              if (ox >= 0 && oy >= 0 && ox < d.w - 4 && oy < d.h - 5 && (ox % 3) == 0 && (oy % 4) < 2) {
                const int bit = ((ox / 3) * 7 + (oy / 4) * 13 + d.okna) & 31;
                if (bit < 11) c = (bit < 3) ? b9(7, 6, 3) : (bit < 7 ? b9(6, 4, 2) : b9(3, 3, 6));
              }
              obr[(size_t)y * W + x] = c;
            }
          if (d.h > 26) {      // anteny s cervenym svetlem
            const int ax = x0 + d.w / 2;
            for (int y = top - 5; y < top; y++) bod(ax, y, b9(1, 0, 2));
            if (((n / 20) + d.x) % 3 != 0) { bod(ax, top - 6, b9(7, 1, 1)); bod(ax - 1, top - 6, b9(4, 0, 1)); bod(ax + 1, top - 6, b9(4, 0, 1)); }
          }
        }
      }
    }
    // ---- more: kazdy radek se posouva jinou rychlosti (paralaxa po radcich) ----
    for (int y = HORIZONT; y < H; y++) {
      const float k = (float)(y - HORIZONT) / (H - HORIZONT);     // 0 obzor .. 1 dole
      const float rych = 0.8f + 9.0f * k * k + 2.0f * k;
      const float posun = t * rych;
      // zakladni barva v pasech (radkovy rozklad mezi pasy - jako tehdy)
      const float pas = k * 5.f + ((y & 1) ? 0.3f : -0.3f);
      static const int PR[6][3] = {{3, 1, 4}, {3, 1, 4}, {2, 1, 4}, {1, 1, 3}, {1, 0, 3}, {0, 0, 2}};
      const int pi = std::max(0, std::min(5, (int)pas));
      const uint32_t zaklad = b9(PR[pi][0], PR[pi][1], PR[pi][2]);
      const int pl = k < 0.3f ? 1 : 2;             // u obzoru jemnejsi hrebeny
      const uint32_t hreben = b9(std::min(7, PR[pi][0] + pl), std::min(7, PR[pi][1] + pl), std::min(7, PR[pi][2] + pl));
      uint32_t *radek = &obr[(size_t)y * W];
      const float frek = 0.07f / (0.12f + k);
      const float sl = 10.f + 52.f * k;            // sirka odrazu slunce
      for (int x = 0; x < W; x++) {
        uint32_t c = zaklad;
        const float wx = (x + posun) * frek;
        const float vl = sumX(wx + y * 3.1f);
        if (vl > 0.80f + 0.08f * (1.f - k)) c = hreben;   // ridke hrebeny vln
        const float dx = std::fabs((float)x - SX);
        if (dx < sl) {                              // odraz slunce: ciste carky
          const float lesk = sumX((x + posun * 0.6f) * 0.22f + y * 5.7f);
          const float prah = 0.42f + 0.45f * (dx / sl) + 0.15f * k;
          if (lesk > prah + 0.12f) c = k < 0.35f ? b9(7, 7, 4) : b9(7, 6, 2);
          else if (lesk > prah) c = b9(6, 3, 3);
        }
        radek[x] = c;
      }
    }
    // rychlostni cary na vode (blizko)
    for (int i = 0; i < 14; i++) {
      const int y = HORIZONT + 34 + (int)(hash1(i * 3 + 11) * (H - HORIZONT - 38));
      const float k = (float)(y - HORIZONT) / (H - HORIZONT);
      const float rych = 6.f + 14.f * k;
      const int delka = 6 + (int)(18.f * k);
      int x = (int)(hash1(i * 3 + 13) * 900.f - t * rych) % (W + 60); if (x < 0) x += W + 60;
      x -= 30;
      for (int j = 0; j < delka; j++) bod(x + j, y, j < 2 ? b9(5, 5, 7) : b9(3, 3, 6));
    }
  }

  // skaly v popredi u spodniho okraje - nejrychlejsi vrstva (pocit rychlosti)
  void popredi(int n) {
    const float t = (float)n;
    const int P = 760;                                  // opakovani
    const int sx = (int)(t * 7.0f) % P;
    for (int x = 0; x < W; x++) {
      const int wx = (x + sx) % P;
      // dve skupiny skal na obdobi
      float h = 0.f;
      for (int g = 0; g < 2; g++) {
        const float c0 = 180.f + g * 380.f, sir = 70.f + g * 30.f;
        const float d = std::fabs((float)wx - c0) / sir;
        if (d < 1.f) h = std::max(h, (1.f - d * d) * (16.f + 10.f * g) + 6.f * sumX(wx * 0.15f + g * 9.f) + 3.f * sumX(wx * 0.5f));
      }
      if (h < 1.f) continue;
      const int top = H - (int)h;
      for (int y = top; y < H; y++) {
        uint32_t c = b9(0, 0, 1);
        if (y == top) c = b9(3, 1, 3);
        else if (y == top + 1 && ((x + n) & 1)) c = b9(2, 1, 2);
        bod(x, y, c);
      }
    }
  }

  void kresliSprite(const Sprite &s, int x0, int y0) {
    for (int y = 0; y < s.h; y++) {
      const int yy = y0 + y;
      if (yy < 0 || yy >= H) continue;
      for (int x = 0; x < s.w; x++) {
        const uint32_t c = s.px[(size_t)y * s.w + x];
        if (c) bod(x0 + x, yy, c);
      }
    }
  }

  void vybuch(const Vyb &v, int n) {
    const int f = n - v.od;
    if (f < 0 || f > 30) return;
    const int cx = (int)v.x, cy = (int)v.y;
    if (f < 3) {                          // bily zablesk
      const int r = 6 + f * 4;
      for (int y = -r; y <= r; y++)
        for (int x = -r; x <= r; x++)
          if (x * x + y * y <= r * r) bod(cx + x, cy + y, f == 0 ? b9(7, 7, 7) : b9(7, 7, 5));
      return;
    }
    const float u = (f - 3) / 27.f;
    const float R = 7.f + 13.f * std::sqrt(u);
    for (int y = (int)-R - 2; y <= (int)R + 2; y++)
      for (int x = (int)-R - 2; x <= (int)R + 2; x++) {
        const float d = std::sqrt((float)(x * x + y * y));
        const float sum = hash1((x + 40) * 97 + (y + 40) * 31 + f / 3) * 0.35f;
        const float m = d / R + sum;
        if (m > 1.f) continue;
        uint32_t c;
        if (u < 0.45f) {
          c = m < 0.35f ? b9(7, 7, 5) : (m < 0.6f ? b9(7, 6, 1) : (m < 0.8f ? b9(7, 3, 0) : b9(5, 1, 0)));
        } else {
          if (((x + y + f) & 1) && u > 0.75f) continue;     // dym ridne
          c = m < 0.5f ? b9(4, 2, 1) : b9(2, 1, 1);
          if (u < 0.6f && m < 0.3f) c = b9(7, 4, 0);
        }
        bod(cx + x, cy + y, c);
      }
    if (u < 0.6f) {                       // prstenec razove vlny
      const float rr = 9.f + 36.f * u;
      for (int a = 0; a < 72; a++) {
        const float an = a * 0.0872665f;
        const int x = cx + (int)(std::cos(an) * rr), y = cy + (int)(std::sin(an) * rr * 0.8f);
        if ((a + f) & 1) bod(x, y, b9(7, 7, 6));
      }
    }
    for (int k = 0; k < 9; k++) {         // trosky
      const float an = hash1(v.od * 13 + k) * 6.2831853f, sp = 1.2f + 2.2f * hash1(v.od * 7 + k);
      const int x = cx + (int)(std::cos(an) * sp * f), y = cy + (int)(std::sin(an) * sp * f + 0.05f * f * f);
      if (f < 22) { bod(x, y, b9(7, 6, 3)); bod(x + 1, y, b9(5, 2, 0)); }
    }
  }

  void laser(const Las &l) {
    const int x = (int)l.x, y = (int)l.y;
    const int del = l.maly ? 9 : 16;
    for (int j = -del; j <= 0; j++) {
      bod(x + j, y, j > -3 ? b9(7, 7, 7) : (l.maly ? b9(7, 6, 3) : b9(5, 7, 7)));
      if (!l.maly && j > -13) { bod(x + j, y - 1, b9(1, 5, 7)); bod(x + j, y + 1, b9(1, 5, 7)); }
    }
  }

  void sprity(int n) {
    for (const Dr &d : drony) {
      if (!d.zivy) continue;
      const Sprite &s = dron[(n / 4 + (int)(d.faze * 3.f)) & 3];
      kresliSprite(s, (int)d.x - s.w / 2, (int)d.y - s.h / 2);
    }
    for (const Vyb &v : vybuchy) vybuch(v, n);
    for (const Las &l : lasery) laser(l);
    for (const Str &st : strely) {          // ruzove strely dronu (blikaji)
      const int x = (int)st.x, y = (int)st.y;
      const uint32_t a = (n & 2) ? b9(7, 3, 6) : b9(7, 5, 7), b = b9(4, 0, 3);
      for (int yy = -2; yy <= 2; yy++)
        for (int xx = -2; xx <= 2; xx++) {
          const int d = xx * xx + yy * yy;
          if (d <= 1) bod(x + xx, y + yy, b9(7, 7, 7)); else if (d <= 4) bod(x + xx, y + yy, a); else if (d <= 5) bod(x + xx, y + yy, b);
        }
    }
    // pomocne koule
    for (int k = 1; k <= 2; k++) {
      float ox, oy; moznostPoloha(k, (int)stopaX.size() - 1, ox, oy);
      const Sprite &s = moznost[(n / 3 + k) & 3];
      kresliSprite(s, (int)ox - 7, (int)oy - 7);
    }
    // lod: naklon podle pohybu nahoru / dolu
    const int ki = lodVy < -0.6f ? 0 : (lodVy > 0.6f ? 2 : 1);
    const Sprite &s = lod[ki];
    const int lx = (int)lodX - s.w / 2, ly = (int)lodY - s.h / 2;
    {                                      // plamen motoru (3 faze)
      const int fx = lx + 3, fy = (int)lodY + 1;
      const int del = 8 + ((n * 7) % 3) * 3;
      for (int j = 0; j < del; j++) {
        const int tl = j < 3 ? 2 : (j < del - 3 ? 1 : 0);
        const uint32_t c = j < 3 ? b9(7, 7, 7) : (j < del / 2 ? b9(7, 6, 2) : b9(7, 3, 0));
        for (int k = -tl; k <= tl; k++) bod(fx - j, fy + k, c);
      }
    }
    kresliSprite(s, lx, ly);
  }

  void text(int x, int y, const char *s, uint32_t c, uint32_t stin) {
    for (; *s; s++, x += 8) {
      const int ch = (unsigned char)*s - 0x20;
      if (ch < 0 || ch >= 96) continue;
      for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < 8; xx++)
          if (pismo[ch][yy] & (0x80 >> xx)) bod(x + xx + 1, y + yy + 1, stin);
      for (int yy = 0; yy < 8; yy++)
        for (int xx = 0; xx < 8; xx++)
          if (pismo[ch][yy] & (0x80 >> xx)) bod(x + xx, y + yy, c);
    }
  }

  void hud(int n) {
    char b[32];
    std::snprintf(b, sizeof b, "1P %07d", skore);
    text(10, 6, b, b9(7, 7, 7), b9(0, 0, 1));
    text(132, 6, "HI 0500000", b9(7, 6, 2), b9(0, 0, 1));
    for (int i = 0; i < 3; i++) {          // zivoty: male lodicky
      const int x0 = 262 + i * 18, y0 = 6;
      for (int y = 0; y < 7; y++)
        for (int x = 0; x < 14; x++) {
          const bool tvar = (y >= 2 && y <= 4 && x < 13) || (y == 1 && x >= 4 && x < 9) || (y == 5 && x >= 2 && x < 7);
          if (tvar) bod(x0 + x, y0 + y, (y == 2) ? b9(7, 7, 7) : b9(4, 5, 6));
        }
    }
    (void)n;
  }
};

}  // namespace napdemo
