// nap_menu_platno.h
// ====================================================================
//  B311: MALE PLATNO NA CPU - kresleni textur pro predmety v kartach
//  (potisk kazety, stranky knihy). Tvary pres vzdalenost od okraje (SDF)
//  s vyhlazenim, text pres stb_truetype (pisma z appky).
//  Bez OpenGL - vysledek se nahraje jako textura (RGBA, nepremultiplied).
// ====================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <functional>
#include <string>
#include <vector>
#include "../intro/nap_demo_text.h"

namespace napmenu {

struct Barva {
  float r = 0, g = 0, b = 0, a = 1;
  Barva() {}
  Barva(float R, float G, float B, float A = 1.f) : r(R), g(G), b(B), a(A) {}
  Barva sAlfou(float A) const { return Barva(r, g, b, a * A); }
};
inline Barva mixB(const Barva &x, const Barva &y, float t) {
  return Barva(x.r + (y.r - x.r) * t, x.g + (y.g - x.g) * t, x.b + (y.b - x.b) * t, x.a + (y.a - x.a) * t);
}

class Platno {
 public:
  Platno(int w, int h) : w_(w), h_(h), p_((size_t)w * h * 4, 0.f) {}
  int w() const { return w_; }
  int h() const { return h_; }

  // jeden bod (premultiplied "src over")
  void bod(int x, int y, const Barva &c, float pokryti) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    const float a = c.a * pokryti;
    if (a <= 0.f) return;
    float *q = &p_[((size_t)y * w_ + x) * 4];
    q[0] = c.r * a + q[0] * (1.f - a);
    q[1] = c.g * a + q[1] * (1.f - a);
    q[2] = c.b * a + q[2] * (1.f - a);
    q[3] = a + q[3] * (1.f - a);
  }
  // vyrazit (alfa na 0 podle pokryti) - okenka
  void vyrazit(int x, int y, float pokryti) {
    if (x < 0 || y < 0 || x >= w_ || y >= h_) return;
    float *q = &p_[((size_t)y * w_ + x) * 4];
    const float k = 1.f - std::min(1.f, std::max(0.f, pokryti));
    q[0] *= k; q[1] *= k; q[2] *= k; q[3] *= k;
  }

  // obecny tvar: sdf(x, y) v bodech (< 0 uvnitr), barva podle polohy
  void tvar(float x0, float y0, float x1, float y1, const std::function<float(float, float)> &sdf,
            const std::function<Barva(float, float)> &barva) {
    const int a = std::max(0, (int)std::floor(x0) - 1), b = std::max(0, (int)std::floor(y0) - 1);
    const int c = std::min(w_ - 1, (int)std::ceil(x1) + 1), d = std::min(h_ - 1, (int)std::ceil(y1) + 1);
    for (int y = b; y <= d; y++)
      for (int x = a; x <= c; x++) {
        const float px = x + 0.5f, py = y + 0.5f;
        const float cov = std::min(1.f, std::max(0.f, 0.5f - sdf(px, py)));
        if (cov > 0.f) bod(x, y, barva(px, py), cov);
      }
  }
  void tvarVyrazit(float x0, float y0, float x1, float y1, const std::function<float(float, float)> &sdf) {
    const int a = std::max(0, (int)std::floor(x0) - 1), b = std::max(0, (int)std::floor(y0) - 1);
    const int c = std::min(w_ - 1, (int)std::ceil(x1) + 1), d = std::min(h_ - 1, (int)std::ceil(y1) + 1);
    for (int y = b; y <= d; y++)
      for (int x = a; x <= c; x++) {
        const float cov = std::min(1.f, std::max(0.f, 0.5f - sdf(x + 0.5f, y + 0.5f)));
        if (cov > 0.f) vyrazit(x, y, cov);
      }
  }

  // ---- SDF tvaru ----
  static float sdfObd(float px, float py, float x0, float y0, float x1, float y1, float r) {
    const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f, hx = (x1 - x0) * 0.5f - r, hy = (y1 - y0) * 0.5f - r;
    const float qx = std::fabs(px - cx) - hx, qy = std::fabs(py - cy) - hy;
    const float ox = std::max(qx, 0.f), oy = std::max(qy, 0.f);
    return std::sqrt(ox * ox + oy * oy) + std::min(std::max(qx, qy), 0.f) - r;
  }
  static float sdfKruh(float px, float py, float cx, float cy, float r) { return std::sqrt((px - cx) * (px - cx) + (py - cy) * (py - cy)) - r; }

  void obd(float x0, float y0, float x1, float y1, float r, const Barva &c) {
    tvar(x0, y0, x1, y1, [=](float x, float y) { return sdfObd(x, y, x0, y0, x1, y1, r); }, [=](float, float) { return c; });
  }
  // obdelnik se svislym prechodem barvy
  void obdPrechod(float x0, float y0, float x1, float y1, float r, const Barva &nahore, const Barva &dole) {
    tvar(x0, y0, x1, y1, [=](float x, float y) { return sdfObd(x, y, x0, y0, x1, y1, r); },
         [=](float, float y) { return mixB(nahore, dole, std::min(1.f, std::max(0.f, (y - y0) / std::max(1.f, y1 - y0)))); });
  }
  void kruh(float cx, float cy, float r, const Barva &c) {
    tvar(cx - r, cy - r, cx + r, cy + r, [=](float x, float y) { return sdfKruh(x, y, cx, cy, r); }, [=](float, float) { return c; });
  }
  void prstenec(float cx, float cy, float r, float tl, const Barva &c) {
    const float R = r + tl * 0.5f;
    tvar(cx - R, cy - R, cx + R, cy + R, [=](float x, float y) { return std::fabs(sdfKruh(x, y, cx, cy, r)) - tl * 0.5f; },
         [=](float, float) { return c; });
  }
  void cara(float x0, float y0, float x1, float y1, float tl, const Barva &c) {
    const float minx = std::min(x0, x1) - tl, maxx = std::max(x0, x1) + tl, miny = std::min(y0, y1) - tl, maxy = std::max(y0, y1) + tl;
    tvar(minx, miny, maxx, maxy,
         [=](float x, float y) {
           const float dx = x1 - x0, dy = y1 - y0;
           const float l2 = dx * dx + dy * dy;
           float t = l2 > 0 ? ((x - x0) * dx + (y - y0) * dy) / l2 : 0.f;
           t = std::min(1.f, std::max(0.f, t));
           const float ex = x - (x0 + dx * t), ey = y - (y0 + dy * t);
           return std::sqrt(ex * ex + ey * ey) - tl * 0.5f;
         },
         [=](float, float) { return c; });
  }

  // ---- text: zarovnani 0 = stred, 1 = vlevo, 2 = vpravo (x je stred / levy / pravy okraj),
  //      y = uctari (baseline); proklad v em ----
  float sirkaTextu(const napdemo::Pismo &p, const std::string &s, float px, float proklad) const {
    const float sc = p.meritko(px);
    float w = 0;
    for (const char *q = s.c_str(); *q;) {
      const int cp = napdemo::utf8Dalsi(q);
      int adv, lsb;
      stbtt_GetCodepointHMetrics(&p.info, cp, &adv, &lsb);
      w += adv * sc + proklad * px;
    }
    return w - proklad * px;
  }
  void text(const napdemo::Pismo &p, const std::string &s, float x, float y, float px, const Barva &c, int zarovnani = 0, float proklad = 0.f,
            float maxSirka = 0.f) {
    if (!p.ok || s.empty()) return;
    float sirka = sirkaTextu(p, s, px, proklad);
    if (maxSirka > 0.f && sirka > maxSirka) { px *= maxSirka / sirka; sirka = maxSirka; }
    const float sc = p.meritko(px);
    float xx = zarovnani == 0 ? x - sirka * 0.5f : (zarovnani == 2 ? x - sirka : x);
    std::vector<unsigned char> g;
    for (const char *q = s.c_str(); *q;) {
      const int cp = napdemo::utf8Dalsi(q);
      const float fx = xx - std::floor(xx);
      int x0, y0, x1, y1;
      stbtt_GetCodepointBitmapBoxSubpixel(&p.info, cp, sc, sc, fx, 0.f, &x0, &y0, &x1, &y1);
      const int gw = x1 - x0, gh = y1 - y0;
      if (gw > 0 && gh > 0 && cp != ' ') {
        g.assign((size_t)gw * gh, 0);
        stbtt_MakeCodepointBitmapSubpixel(&p.info, g.data(), gw, gh, gw, sc, sc, fx, 0.f, cp);
        const int ox = (int)std::floor(xx) + x0, oy = (int)std::floor(y) + y0;
        for (int a = 0; a < gh; a++)
          for (int b = 0; b < gw; b++) {
            const unsigned char v = g[(size_t)a * gw + b];
            if (v) bod(ox + b, oy + a, c, v / 255.f);
          }
      }
      int adv, lsb;
      stbtt_GetCodepointHMetrics(&p.info, cp, &adv, &lsb);
      xx += adv * sc + proklad * px;
    }
  }

  // RGBA 8 bit, rgb NEpremultiplied (shader michá podle alfy)
  std::vector<unsigned char> rgba8() const {
    std::vector<unsigned char> v((size_t)w_ * h_ * 4);
    for (size_t i = 0; i < (size_t)w_ * h_; i++) {
      const float a = p_[i * 4 + 3];
      for (int k = 0; k < 3; k++) {
        const float c = a > 1e-4f ? p_[i * 4 + k] / a : 0.f;
        v[i * 4 + k] = (unsigned char)std::lround(std::min(1.f, std::max(0.f, c)) * 255.f);
      }
      v[i * 4 + 3] = (unsigned char)std::lround(std::min(1.f, std::max(0.f, a)) * 255.f);
    }
    return v;
  }

 private:
  int w_, h_;
  std::vector<float> p_;
};

}  // namespace napmenu
