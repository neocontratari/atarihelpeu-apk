// nap_menu_umeni_kazeta.h
// ====================================================================
//  B311: KARTA MP3 PREHRAVAC - kazeta N&P (vlastni navrh, 3D)
//
//  Cerna leskla kazeta se stitkem N&P RETRO CASSETTE (pruhy v barvach
//  80. let, velke A, C-90), okenkem jsou videt civky s paskem - obe se
//  toci (pasek jede zleva doprava, 4,76 cm/s jako ve skutecnem kazetaku,
//  takze plna civka se toci pomaleji). Za kazetou azurova zvukova vlna.
//  Jednotky cm, predek k +Z (ke kamere), stred kazety v pocatku.
// ====================================================================
#pragma once
#include "../intro/nap_demo_konzole.h"
#include "nap_menu_platno.h"
#include "nap_menu_umeni.h"

namespace napmenu {

class UmeniKazeta : public Umeni {
 public:
  ~UmeniKazeta() override { if (tex_) glDeleteTextures(1, &tex_); }

  bool init(KontextUmeni &k) override {
    p_.sestav(napdemo::VS_MODEL, sSumem(FS_STUDIO).c_str(), "menu-kazeta");
    if (!p_.ok || !k.bold || !k.ital || !k.semi) return false;
    postavPotisk(k);
    postavSkrin();
    postavCivku();
    postavPasek();
    return tex_ != 0 && skrin_.pocet > 0;
  }

  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    k.pozadi(r, 0.38f, 0.38f, 0.75f, 1.f);
    const float t = (float)r.t;
    const float pomer = r.pomer();
    const float fov = 0.50f;
    const float vzd = Kamera3D::vzdalenostPro(10.6f, 7.4f, fov, pomer, 0.86f);
    const Kamera3D kam = Kamera3D::spocti(V3(0, 0, 0), 0.10f, vzd, fov, pomer, 0.f, 0.21f);
    const float yaw = 0.34f * std::sin(t * 0.47f) - r.d * 0.55f;
    const float naklon = -0.20f + 0.05f * std::sin(t * 0.61f);
    const float vznos = 0.10f * std::sin(t * 1.15f) * r.fokus;
    const M4 M = M4::posun(0.f, 0.15f + vznos, 0.f) * M4::rotY(yaw) * M4::rotX(naklon) * M4::rotZ(0.05f * std::sin(t * 0.37f));
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    p_.pouzij();
    nastavStudio(p_, kam.oko, r.barva);
    p_.tex("uAtlas", 0, tex_);
    p_.tex("uSum", 1, k.texSum);
    auto kresliSit = [&](const napdemo::Sit &s, const M4 &m) {
      p_.m4("uModel", m);
      p_.m4("uMVP", kam.vp * m);
      s.kresli();
    };
    kresliSit(skrin_, M);
    // civky: plna vlevo (R 1,55), navijejici vpravo (R 0,95); obe proti smeru hodin
    const float V = 4.76f;                       // cm/s
    const float R1 = 1.55f, R2 = 0.95f;
    const float a1 = t * V / R1, a2 = t * V / R2;
    for (int i = 0; i < 2; i++) {
      const float x = i == 0 ? -CIVKA_X : CIVKA_X;
      const float R = i == 0 ? R1 : R2;
      kresliSit(pasek_, M * M4::posun(x, CIVKA_Y, 0.25f) * M4::meritko(R, R, 1.f));
      kresliSit(civka_, M * M4::posun(x, CIVKA_Y, 0.27f) * M4::rotZ(i == 0 ? a1 : a2));
    }
    // sklo okenka (pruhledne, az nakonec)
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);
    kresliSit(sklo_, M);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
  }
  const char *nazev() const override { return "fx:kazeta"; }

 private:
  static constexpr float KW = 5.0f, KH = 3.2f, KD = 0.6f;     // polovina sirky, vysky, hloubky
  static constexpr float CIVKA_X = 2.13f, CIVKA_Y = 0.45f;
  static constexpr float ROH = 0.32f;
  static constexpr int TW = 1024, TH = 1024, PH = 655;        // textura, vyska potisku v ni
  Prog p_;
  napdemo::Sit skrin_, civka_, pasek_, sklo_;
  GLuint tex_ = 0;

  // obrys kazety (zaoblene rohy) proti smeru hodin pri pohledu zepredu
  static std::vector<V2> obrys() {
    std::vector<V2> o;
    const float cx[4] = {KW - ROH, -KW + ROH, -KW + ROH, KW - ROH}, cy[4] = {KH - ROH, KH - ROH, -KH + ROH, -KH + ROH};
    const float a0[4] = {0.f, 1.5708f, 3.14159f, 4.71239f};
    for (int k = 0; k < 4; k++)
      for (int j = 0; j <= 4; j++) {
        const float a = a0[k] + 1.5708f * j / 4.f;
        o.push_back({cx[k] + ROH * std::cos(a), cy[k] + ROH * std::sin(a)});
      }
    return o;
  }

  void postavSkrin() {
    napdemo::StavK s;
    const V3 cerna(0.035f, 0.035f, 0.04f);
    const std::vector<V2> o = obrys();
    const size_t n = o.size();
    // boky
    for (size_t i = 0; i < n; i++) {
      const V2 &a = o[i], &b = o[(i + 1) % n];
      s.quad(V3(a.x, a.y, -KD), V3(b.x, b.y, -KD), V3(b.x, b.y, KD), V3(a.x, a.y, KD), 0, cerna);
    }
    // zadni strana (vejir)
    for (size_t i = 0; i < n; i++) {
      const V2 &a = o[i], &b = o[(i + 1) % n];
      s.tri(V3(0, 0, -KD), V3(b.x, b.y, -KD), V3(a.x, a.y, -KD), V3(0, 0, -1), {0, 0}, {0, 0}, {0, 0}, 0, cerna);
    }
    // predni strana s potiskem (material 10: diry = okenko)
    const float vMax = (float)PH / TH;
    s.quad(V3(-KW, -KH, KD), V3(KW, -KH, KD), V3(KW, KH, KD), V3(-KW, KH, KD), 10, V3(1, 1, 1), {0.f, vMax}, {1.f, vMax}, {1.f, 0.f},
           {0.f, 0.f});
    // vnitrek za okenkem (tmave)
    s.quad(V3(-3.4f, -0.6f, -0.40f), V3(3.4f, -0.6f, -0.40f), V3(3.4f, 1.5f, -0.40f), V3(-3.4f, 1.5f, -0.40f), 0, V3(0.05f, 0.05f, 0.055f));
    skrin_.nahraj(s.v);
    // sklo okenka tesne za prednim stitkem
    napdemo::StavK g;
    g.quad(V3(-3.1f, -0.5f, 0.55f), V3(3.1f, -0.5f, 0.55f), V3(3.1f, 1.4f, 0.55f), V3(-3.1f, 1.4f, 0.55f), 2, V3(1, 1, 1));
    sklo_.nahraj(g.v);
  }

  // civka: bily kruh se 6 zuby uvnitr (stred v pocatku, lici k +Z)
  void postavCivku() {
    napdemo::StavK s;
    const V3 bila(0.90f, 0.88f, 0.84f);
    const float ro = 0.44f, ri = 0.25f;
    const int N = 40;
    for (int k = 0; k < N; k++) {
      const float a0 = 6.2831853f * k / N, a1 = 6.2831853f * (k + 1) / N;
      const V3 A(ro * std::cos(a0), ro * std::sin(a0), 0), B(ro * std::cos(a1), ro * std::sin(a1), 0);
      const V3 C(ri * std::cos(a1), ri * std::sin(a1), 0), D(ri * std::cos(a0), ri * std::sin(a0), 0);
      s.quad(A, B, C, D, 3, bila);
      // bok (valec dozadu)
      s.quad(V3(A.x, A.y, -0.10f), V3(B.x, B.y, -0.10f), B, A, 3, bila * 0.8f);
    }
    // zuby dovnitr
    for (int z = 0; z < 6; z++) {
      const float a = 6.2831853f * z / 6.f;
      const V3 u(std::cos(a), std::sin(a), 0), v(-std::sin(a), std::cos(a), 0);
      const float w = 0.045f, r0 = ri - 0.085f;
      const V3 A = u * r0 - v * w, B = u * ri - v * w, C = u * ri + v * w, D = u * r0 + v * w;
      s.quad(A + V3(0, 0, 0.002f), B + V3(0, 0, 0.002f), C + V3(0, 0, 0.002f), D + V3(0, 0, 0.002f), 3, bila);
    }
    // tmave paprsky civky (vypadaji jako otvory v cive - toceni je videt)
    for (int z = 0; z < 3; z++) {
      const float a = 6.2831853f * z / 3.f + 0.5f;
      const V3 u(std::cos(a), std::sin(a), 0), v(-std::sin(a), std::cos(a), 0);
      const float w = 0.05f;
      const V3 A = u * (ri + 0.03f) - v * w, B = u * (ro - 0.04f) - v * w, C = u * (ro - 0.04f) + v * w, D = u * (ri + 0.03f) + v * w;
      s.quad(A + V3(0, 0, 0.003f), B + V3(0, 0, 0.003f), C + V3(0, 0, 0.003f), D + V3(0, 0, 0.003f), 0, V3(0.25f, 0.24f, 0.23f));
    }
    civka_.nahraj(s.v);
  }

  // pasek navinuty na civce: valec polomeru 1 (meritko podle navinuti), hnedy leskly
  void postavPasek() {
    napdemo::StavK s;
    const V3 hneda(0.20f, 0.11f, 0.05f);
    const int N = 48;
    for (int k = 0; k < N; k++) {
      const float a0 = 6.2831853f * k / N, a1 = 6.2831853f * (k + 1) / N;
      const V3 A(std::cos(a0), std::sin(a0), 0), B(std::cos(a1), std::sin(a1), 0);
      s.tri(V3(0, 0, 0), A, B, V3(0, 0, 1), {0, 0}, {0, 0}, {0, 0}, 4, hneda);
      s.quad(V3(A.x, A.y, -0.30f), V3(B.x, B.y, -0.30f), B, A, 4, hneda * 0.8f);
    }
    pasek_.nahraj(s.v);
  }

  // potisk prední strany: cerny plast, stitek N&P RETRO CASSETTE, okenko (pruhledne)
  void postavPotisk(KontextUmeni &k) {
    const float S = TW / (2.f * KW);                  // bodu na cm
    auto X = [&](float x) { return (x + KW) * S; };
    auto Y = [&](float y) { return (KH - y) * S; };
    Platno pl(TW, PH);
    // plast: cerny, nahore o chlup svetlejsi; zaoblene rohy jako obrys
    pl.obdPrechod(0, 0, (float)TW, (float)PH, ROH * S, Barva(0.085f, 0.085f, 0.09f), Barva(0.035f, 0.035f, 0.04f));
    // svetly lem po obvodu (lisovana hrana)
    pl.tvar(0, 0, (float)TW, (float)PH,
            [&](float x, float y) { return std::fabs(Platno::sdfObd(x, y, 0.12f * S, 0.12f * S, TW - 0.12f * S, PH - 0.12f * S, ROH * S * 0.6f)) - 1.2f; },
            [](float, float) { return Barva(0.20f, 0.20f, 0.21f, 0.8f); });
    // ---- stitek ----
    const float lx0 = X(-4.55f), lx1 = X(4.55f), ly0 = Y(2.95f), ly1 = Y(-1.15f);
    const Barva krem(0.95f, 0.91f, 0.81f);
    pl.obd(lx0, ly0, lx1, ly1, 0.20f * S, krem);
    // pruhy 80. let (pres celou sirku stitku)
    const Barva pruhy[5] = {Barva(1.0f, 0.78f, 0.22f), Barva(1.0f, 0.55f, 0.16f), Barva(0.93f, 0.30f, 0.22f), Barva(0.72f, 0.18f, 0.42f),
                            Barva(0.38f, 0.17f, 0.52f)};
    const float py0 = Y(1.85f), py1 = Y(-0.85f), ph = (py1 - py0) / 5.f;
    for (int i = 0; i < 5; i++) pl.obd(lx0, py0 + ph * i, lx1, py0 + ph * (i + 1) + 0.6f, 0.f, pruhy[i]);
    // napisy nahore
    const Barva tmava(0.12f, 0.07f, 0.04f);
    pl.text(*k.ital, "N&P", X(-4.20f), Y(2.10f), 0.78f * S, tmava, 1, 0.0f);
    pl.text(*k.semi, "RETRO CASSETTE", X(4.25f), Y(2.55f), 0.30f * S, tmava, 2, 0.12f);
    pl.text(*k.semi, u8"C-90 · TYPE II · HIGH BIAS", X(4.25f), Y(2.17f), 0.17f * S, Barva(0.35f, 0.25f, 0.18f), 2, 0.10f);
    pl.cara(X(-2.0f), Y(1.98f), X(4.25f), Y(1.98f), 0.022f * S, Barva(0.35f, 0.25f, 0.18f, 0.7f));
    // velke A vlevo a 90 vpravo (na pruzich)
    pl.text(*k.bold, "A", X(-3.75f), Y(0.08f), 1.15f * S, Barva(1.0f, 0.97f, 0.90f), 0);
    pl.text(*k.bold, "90", X(3.78f), Y(0.20f), 0.62f * S, Barva(1.0f, 0.97f, 0.90f), 0);
    pl.text(*k.semi, u8"N&P VISION · RETRO UNIVERSE", X(0.f), Y(-1.04f), 0.15f * S, Barva(0.35f, 0.25f, 0.18f), 0, 0.18f);
    // ---- okenko: tmavy ramecek a pak dira (kapsle pres obe civky) ----
    const float kx0 = X(-CIVKA_X), kx1 = X(CIVKA_X), ky = Y(CIVKA_Y);
    auto kapsle = [&](float x, float y, float r) {
      const float t = std::min(1.f, std::max(0.f, (x - kx0) / (kx1 - kx0)));
      const float qx = x - (kx0 + (kx1 - kx0) * t), qy = y - ky;
      return std::sqrt(qx * qx + qy * qy) - r;
    };
    const float rOk = 0.80f * S;
    pl.tvar(kx0 - rOk - 12, ky - rOk - 12, kx1 + rOk + 12, ky + rOk + 12, [&](float x, float y) { return kapsle(x, y, rOk + 0.10f * S); },
            [](float, float) { return Barva(0.05f, 0.05f, 0.055f); });
    pl.tvarVyrazit(kx0 - rOk - 4, ky - rOk - 4, kx1 + rOk + 4, ky + rOk + 4, [&](float x, float y) { return kapsle(x, y, rOk); });
    // ---- spodni cast: lichobeznik s otvory pro hlavu, kladky ----
    pl.tvar(X(-3.7f), Y(-1.32f), X(3.7f), Y(-3.15f),
            [&](float x, float y) {
              const float yy = (y - Y(-1.32f)) / (Y(-3.15f) - Y(-1.32f));         // 0 nahore .. 1 dole
              const float pul = (3.7f - 0.45f * yy) * S;
              return std::max(std::max(std::fabs(x - X(0.f)) - pul, Y(-1.32f) - y), y - Y(-3.15f));
            },
            [](float, float y) { (void)y; return Barva(0.10f, 0.10f, 0.11f); });
    const Barva dira(0.012f, 0.012f, 0.014f);
    pl.obd(X(-0.62f), Y(-2.40f), X(0.62f), Y(-2.98f), 0.05f * S, dira);
    for (int st = -1; st <= 1; st += 2) {
      pl.obd(X(st * 1.35f - 0.32f), Y(-2.55f), X(st * 1.35f + 0.32f), Y(-2.98f), 0.04f * S, dira);
      pl.kruh(X(st * 2.55f), Y(-2.62f), 0.24f * S, dira);
      pl.kruh(X(st * 3.30f), Y(-2.78f), 0.12f * S, dira);
    }
    // sroubky
    auto sroub = [&](float x, float y) {
      const float r = 0.14f * S;
      pl.tvar(X(x) - r, Y(y) - r, X(x) + r, Y(y) + r, [&](float px, float py) { return Platno::sdfKruh(px, py, X(x), Y(y), r); },
              [&](float px, float py) {
                const float d = ((px - X(x)) * -0.6f + (py - Y(y)) * -0.8f) / r;
                const float v = 0.55f + 0.35f * d;
                return Barva(v, v * 0.98f, v * 0.95f);
              });
      pl.cara(X(x) - r * 0.6f, Y(y) - r * 0.2f, X(x) + r * 0.6f, Y(y) + r * 0.2f, r * 0.22f, Barva(0.15f, 0.15f, 0.16f));
    };
    sroub(-4.70f, -2.92f);
    sroub(4.70f, -2.92f);
    sroub(0.f, -1.80f);
    // do textury 2^n (mipmapy), potisk nahore
    std::vector<unsigned char> v = pl.rgba8();
    v.resize((size_t)TW * TH * 4, 0);
    tex_ = napdemo::novaTextura(TW, TH, GL_RGBA, v.data(), true);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
};

}  // namespace napmenu
