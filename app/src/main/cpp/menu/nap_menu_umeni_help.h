// nap_menu_umeni_help.h
// ====================================================================
//  B311: KARTA HELP - otevrena kniha s navodem (3D) a zlaty otaznik
//
//  Kniha lezi otevrena na zlatem kruhu: kozene desky, dva bloky stranek
//  (stranky se ke hrbetu prohybaji jako u skutecne knihy), vlevo obsah
//  navodu (kapitoly), vpravo ovladani s kreslenym ovladacem, zlata
//  zalozka. Kazdych 5 s se otoci stranka (stoci se a preleti doleva).
//  Nad knihou se vznasi zlaty otaznik s odleskem.
//  Jednotky cm: hrbet na ose Z (x = 0), stranky v rovine x-z, nahoru +Y.
// ====================================================================
#pragma once
#include "../intro/nap_demo_konzole.h"
#include "nap_menu_platno.h"
#include "nap_menu_umeni.h"

namespace napmenu {

class UmeniHelp : public Umeni {
 public:
  ~UmeniHelp() override {
    if (tex_) glDeleteTextures(1, &tex_);
    otaznik_.uvolni();
  }

  bool init(KontextUmeni &k) override {
    p_.sestav(napdemo::VS_MODEL, sSumem(FS_STUDIO).c_str(), "menu-kniha");
    if (!p_.ok || !k.bold || !k.semi || !k.med) return false;
    postavStranky(k);
    postavKnihu();
    napdemo::Odstavec o;
    o.text = "?";
    o.pismo = k.bold;
    o.px = 300.f;
    o.radek = 1.0f;
    otaznik_ = napdemo::Sazec::vysazej({o}, 10000.f, 0, 14.f, 0.6f);
    return tex_ != 0 && kniha_.pocet > 0;
  }

  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    k.pozadi(r, 0.355f, 0.44f, 0.8f);
    const float t = (float)r.t;
    const float pomer = r.pomer();
    const float fov = 0.50f;
    const float vzd = Kamera3D::vzdalenostPro(31.f, 16.f, fov, pomer, 0.94f);
    const Kamera3D kam = Kamera3D::spocti(V3(0, 0, 0), 0.70f, vzd, fov, pomer, 0.f, 0.04f);
    const float yaw = 0.16f * std::sin(t * 0.33f) - r.d * 0.45f;
    const M4 M = M4::rotY(yaw) * M4::posun(0.f, 0.f, 0.5f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    glDisable(GL_BLEND);
    p_.pouzij();
    nastavStudio(p_, kam.oko, r.barva, 1.05f);
    p_.tex("uAtlas", 0, tex_);
    p_.tex("uSum", 1, k.texSum);
    p_.m4("uModel", M);
    p_.m4("uMVP", kam.vp * M);
    kniha_.kresli();
    // otaceni stranky: kazdych 5 s, trva 1,3 s
    const float CYKL = 5.f, OTOC = 1.3f;
    const float f = std::fmod(t + 2.0f, CYKL);
    if (f < OTOC) {
      const float u = f / OTOC;
      const float phi = 3.14159f * (u * u * (3.f - 2.f * u));
      postavStranku(phi);
      glEnable(GL_CULL_FACE);
      glCullFace(GL_BACK);
      glFrontFace(GL_CCW);
      stranka_.kresli();
      glDisable(GL_CULL_FACE);
    }
    glDisable(GL_DEPTH_TEST);
    // zlaty otaznik nad knihou (vznasi se) a zare za nim
    glEnable(GL_BLEND);
    const float y = 0.50f + 0.022f * std::sin(t * 1.6f);
    glBlendFunc(GL_ONE, GL_ONE);
    k.zare(0.f, y, 0.42f, 0.30f, napdemo::lerp(V3(1.0f, 0.70f, 0.25f), r.barva, 0.3f) * (0.35f + 0.15f * r.fokus));
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    const float lesk = -0.6f + 2.4f * std::min(1.f, std::fmod(t, 3.5f) / 1.1f);
    k.textNdc(otaznik_, 0.f, y, 0.46f, pomer, V3(1.0f, 0.93f, 0.62f), V3(0.86f, 0.52f, 0.12f), 1.f, lesk);
    glDisable(GL_BLEND);
    glActiveTexture(GL_TEXTURE0);
  }
  const char *nazev() const override { return "fx:help"; }

 private:
  static constexpr float PW = 13.f, ZH = 9.f, G = 0.15f, TB = 1.05f;   // stranka, pul vysky, mezera u hrbetu, tloustka bloku
  static constexpr int SEG = 26;
  static constexpr float VP = 716.f / 1024.f;                         // vyska stranky v atlasu
  Prog p_;
  napdemo::Sit kniha_, stranka_;
  GLuint tex_ = 0;
  napdemo::TextTex otaznik_;

  // vyska horni stranky ve vzdalenosti d od hrbetu (prohnuti ke hrbetu)
  static float h(float d) {
    const float s = std::min(1.f, std::max(0.f, d / 3.6f));
    return TB * s * s * (3.f - 2.f * s) + 0.30f * std::sin(3.14159f * d / PW);
  }
  static float dh(float d) { return (h(d + 0.01f) - h(d - 0.01f)) / 0.02f; }

  void postavKnihu() {
    napdemo::StavK s;
    const V3 kuze(0.26f, 0.07f, 0.045f), zlato(0.80f, 0.52f, 0.14f);
    // desky (kozene) pod obema bloky
    s.kvadr(V3(-(G + PW + 0.45f), -0.42f, -ZH - 0.55f), V3(G + PW + 0.45f, 0.f, ZH + 0.55f), 8, kuze, false);
    for (int strana = -1; strana <= 1; strana += 2) {
      // horni stranka (prohnuta), uv z atlasu: vlevo obsah, vpravo ovladani
      for (int i = 0; i < SEG; i++) {
        const float d0 = PW * i / SEG, d1 = PW * (i + 1) / SEG;
        const float x0 = strana * (G + d0), x1 = strana * (G + d1);
        const float y0 = h(d0), y1 = h(d1);
        const float u0 = strana > 0 ? 0.5f + 0.5f * d0 / PW : 0.5f * (1.f - d0 / PW);
        const float u1 = strana > 0 ? 0.5f + 0.5f * d1 / PW : 0.5f * (1.f - d1 / PW);
        // predni hrana (z = +ZH) = spodek stranky (v = VP), zadni = vrsek (v = 0)
        if (strana > 0)
          s.quad(V3(x0, y0, ZH), V3(x1, y1, ZH), V3(x1, y1, -ZH), V3(x0, y0, -ZH), 7, V3(1, 1, 1), {u0, VP}, {u1, VP}, {u1, 0.f}, {u0, 0.f});
        else
          s.quad(V3(x1, y1, ZH), V3(x0, y0, ZH), V3(x0, y0, -ZH), V3(x1, y1, -ZH), 7, V3(1, 1, 1), {u1, VP}, {u0, VP}, {u0, 0.f}, {u1, 0.f});
        // orizka vpredu a vzadu (profil bloku)
        if (strana > 0) {
          s.quad(V3(x0, 0.f, ZH), V3(x1, 0.f, ZH), V3(x1, y1, ZH), V3(x0, y0, ZH), 11, V3(1, 1, 1));
          s.quad(V3(x1, 0.f, -ZH), V3(x0, 0.f, -ZH), V3(x0, y0, -ZH), V3(x1, y1, -ZH), 11, V3(1, 1, 1));
        } else {
          s.quad(V3(x1, 0.f, ZH), V3(x0, 0.f, ZH), V3(x0, y0, ZH), V3(x1, y1, ZH), 11, V3(1, 1, 1));
          s.quad(V3(x0, 0.f, -ZH), V3(x1, 0.f, -ZH), V3(x1, y1, -ZH), V3(x0, y0, -ZH), 11, V3(1, 1, 1));
        }
      }
      // vnejsi orizka
      const float xe = strana * (G + PW), ye = h(PW);
      if (strana > 0) s.quad(V3(xe, 0.f, ZH), V3(xe, 0.f, -ZH), V3(xe, ye, -ZH), V3(xe, ye, ZH), 11, V3(1, 1, 1));
      else s.quad(V3(xe, 0.f, -ZH), V3(xe, 0.f, ZH), V3(xe, ye, ZH), V3(xe, ye, -ZH), 11, V3(1, 1, 1));
    }
    // zlata zalozka: pres pravou stranku a dolu pres predni orizku
    {
      const float xa = 5.2f, xb = 6.0f;
      const int N = 12;
      for (int i = 0; i < N; i++) {
        const float z0 = -ZH + 2.f * ZH * i / N, z1 = -ZH + 2.f * ZH * (i + 1) / N;
        const float ya = h(xa - G) + 0.02f, yb = h(xb - G) + 0.02f;
        s.quad(V3(xa, ya, z1), V3(xb, yb, z1), V3(xb, yb, z0), V3(xa, ya, z0), 3, zlato);
      }
      const float ya = h(xa - G) + 0.02f, yb = h(xb - G) + 0.02f;
      s.quad(V3(xa, -1.6f, ZH + 0.35f), V3(xb, -1.6f, ZH + 0.35f), V3(xb, yb, ZH + 0.02f), V3(xa, ya, ZH + 0.02f), 3, zlato * 0.85f);
    }
    kniha_.nahraj(s.v);
  }

  // stranka, ktera se otaci (phi 0 = lezi vpravo .. pi = lezi vlevo), stoci se
  void postavStranku(float phi) {
    napdemo::StavK s;
    const int N = 24;
    std::vector<V3> P((size_t)N + 1), T((size_t)N + 1);
    const float kappa = 0.95f * std::sin(phi);
    // zatoceny pruh od hrbetu (pivot nad hrbetem)
    float x = 0.f, y = 0.18f;
    for (int i = 0; i <= N; i++) {
      const float sd = PW * i / N;
      const float th = phi - kappa * (sd / PW);
      T[(size_t)i] = V3(std::cos(th), std::sin(th), 0.f);
      P[(size_t)i] = V3(x, y, 0.f);
      x += std::cos(th) * PW / N;
      y += std::sin(th) * PW / N;
    }
    // na zacatku a na konci splyne s lezici strankou (prohnuti bloku)
    const float wz = napdemo::smooth(0.f, 0.45f, phi), wk = napdemo::smooth(3.14159f, 3.14159f - 0.45f, phi);
    for (int i = 0; i <= N; i++) {
      const float sd = PW * i / N;
      const V3 lezR(G + sd, h(sd) + 0.012f, 0.f), lezL(-(G + sd), h(sd) + 0.012f, 0.f);
      V3 &p = P[(size_t)i];
      if (phi < 1.5708f) p = napdemo::lerp(lezR, p, wz);
      else p = napdemo::lerp(lezL, p, wk);
    }
    for (int i = 0; i < N; i++) {
      const V3 &a = P[(size_t)i], &b = P[(size_t)i + 1];
      const float d0 = PW * i / N / PW, d1 = PW * (i + 1) / N / PW;
      const V3 t = napdemo::norm(b - a);
      const V3 n(-t.y, t.x, 0.f);                                  // predni strana (nahoru, kdyz lezi vpravo)
      const V3 o = n * 0.006f;
      // predni strana: prava stranka atlasu
      s.quad(V3(a.x, a.y, ZH) + o, V3(b.x, b.y, ZH) + o, V3(b.x, b.y, -ZH) + o, V3(a.x, a.y, -ZH) + o, 7, V3(1, 1, 1),
             {0.5f + 0.5f * d0, VP}, {0.5f + 0.5f * d1, VP}, {0.5f + 0.5f * d1, 0.f}, {0.5f + 0.5f * d0, 0.f});
      // zadni strana (obracene poradi): leva stranka atlasu
      s.quad(V3(a.x, a.y, -ZH) - o, V3(b.x, b.y, -ZH) - o, V3(b.x, b.y, ZH) - o, V3(a.x, a.y, ZH) - o, 7, V3(1, 1, 1),
             {0.5f * (1.f - d0), 0.f}, {0.5f * (1.f - d1), 0.f}, {0.5f * (1.f - d1), VP}, {0.5f * (1.f - d0), VP});
    }
    stranka_.nahraj(s.v);
  }

  // atlas stranek: vlevo obsah navodu, vpravo ovladani
  void postavStranky(KontextUmeni &k) {
    const int TW = 1024, TH = 1024, PHp = 716;
    Platno pl(TW, PHp);
    const Barva papir(0.955f, 0.925f, 0.845f), inkoust(0.16f, 0.10f, 0.06f), zlato(0.72f, 0.48f, 0.12f), seda(0.55f, 0.50f, 0.44f);
    for (int st = 0; st < 2; st++) {
      const float x0 = st * 512.f;
      // papir: ke hrbetu tmavsi (prohnuti)
      pl.tvar(x0, 0, x0 + 512, (float)PHp, [&](float x, float y) { return Platno::sdfObd(x, y, x0, 0, x0 + 512, (float)PHp, 0.f); },
              [&](float x, float) {
                const float k2 = st == 0 ? (x - x0) / 512.f : 1.f - (x - x0) / 512.f;      // 1 u hrbetu
                const float tm = 1.f - 0.28f * std::pow(std::max(0.f, k2 - 0.72f) / 0.28f, 2.f);
                return Barva(papir.r * tm, papir.g * tm, papir.b * tm);
              });
    }
    // ---- leva: OBSAH ----
    pl.text(*k.bold, u8"NÁVOD", 256.f, 112.f, 74.f, inkoust, 0, 0.06f);
    pl.text(*k.semi, u8"N&P VISION · RETRO UNIVERSE", 256.f, 152.f, 19.f, zlato, 0, 0.22f);
    pl.cara(120.f, 178.f, 392.f, 178.f, 3.f, zlato);
    const char *kap[8] = {u8"ATARI 130XE", u8"SEGA MEGA DRIVE", u8"PS1 PLAYSTATION", u8"MP3 PŘEHRÁVAČ",
                          u8"DŽUBOX · DJ MIXER", u8"ATARIHELP.EU", u8"OPTIONS A WEB TV", u8"ČASTÉ OTÁZKY"};
    for (int i = 0; i < 8; i++) {
      const float y = 238.f + i * 56.f;
      pl.kruh(78.f, y - 10.f, 15.f, zlato);
      pl.text(*k.bold, std::to_string(i + 1), 78.f, y - 1.f, 20.f, papir, 0);
      pl.text(*k.med, kap[i], 108.f, y, 30.f, inkoust, 1, 0.02f, 360.f);
      pl.cara(108.f, y + 14.f, 440.f, y + 14.f, 1.2f, Barva(0.6f, 0.5f, 0.4f, 0.35f));
    }
    pl.text(*k.semi, u8"— 1 —", 256.f, 690.f, 18.f, seda, 0, 0.1f);
    // ---- prava: OVLADANI (kresleny ovladac a radky textu) ----
    const float px0 = 512.f;
    pl.text(*k.bold, u8"OVLÁDÁNÍ", px0 + 256.f, 100.f, 50.f, inkoust, 0, 0.05f);
    pl.cara(px0 + 150.f, 124.f, px0 + 362.f, 124.f, 3.f, zlato);
    // ovladac: obrys, krizovy ovladac, tlacitka
    {
      const float cx = px0 + 256.f, cy = 240.f;
      pl.tvar(cx - 170, cy - 75, cx + 170, cy + 75, [&](float x, float y) { return std::fabs(Platno::sdfObd(x, y, cx - 165, cy - 70, cx + 165, cy + 70, 62.f)) - 3.5f; },
              [&](float, float) { return inkoust; });
      pl.obd(cx - 112, cy - 9, cx - 52, cy + 9, 3.f, inkoust);
      pl.obd(cx - 91, cy - 30, cx - 73, cy + 30, 3.f, inkoust);
      pl.kruh(cx + 70, cy + 18, 13.f, zlato);
      pl.kruh(cx + 100, cy - 6, 13.f, zlato);
      pl.kruh(cx + 40, cy - 6, 13.f, zlato);
      pl.kruh(cx + 70, cy - 30, 13.f, zlato);
      pl.obd(cx - 26, cy + 30, cx - 4, cy + 38, 3.f, seda);
      pl.obd(cx + 4, cy + 30, cx + 26, cy + 38, 3.f, seda);
    }
    // radky "textu"
    for (int i = 0; i < 9; i++) {
      const float y = 372.f + i * 30.f;
      const float delka = (i % 3 == 2) ? 250.f : 380.f - (i * 37 % 60);
      pl.obd(px0 + 66.f, y - 5.f, px0 + 66.f + delka, y + 5.f, 5.f, Barva(0.45f, 0.40f, 0.34f, 0.55f));
    }
    pl.text(*k.semi, u8"— 2 —", px0 + 256.f, 690.f, 18.f, seda, 0, 0.1f);
    std::vector<unsigned char> v = pl.rgba8();
    v.resize((size_t)TW * TH * 4, 0);
    tex_ = napdemo::novaTextura(TW, TH, GL_RGBA, v.data(), true);
    glBindTexture(GL_TEXTURE_2D, tex_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
  }
};

}  // namespace napmenu
