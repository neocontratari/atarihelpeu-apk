// nap_menu_umeni_3d.h
// ====================================================================
//  B311: KARTY S 3D MODELY Z UVODNIHO DEMA
//   3d:atari  - Atari 130XE (Reneho stroj; model z nap_demo_model.h, shader FS_MODEL)
//   3d:nap16  - konzole N&P 16 (vlastni navrh, nap_demo_konzole.h, shader FS_KONZOLE)
//   3d:nap32  - konzole N&P 32 (vlastni navrh)
//  Predmet stoji na zlatem kruhu, studiove svetlo zleva shora, zezadu
//  protisvetlo v barve karty, zespodu zare kruhu. Pomalu se otaci; kdyz
//  karta jede karuselem, natoci se ke stredu (paralaxa).
// ====================================================================
#pragma once
#include "../intro/nap_demo_konzole.h"
#include "../intro/nap_demo_model.h"
#include "nap_menu_umeni.h"

namespace napmenu {

class UmeniAtari : public Umeni {
 public:
  bool init(KontextUmeni &k) override {
    p_.sestav(napdemo::VS_MODEL, sSumem(napdemo::FS_MODEL).c_str(), "menu-atari");
    if (!k.bold || !k.ital || !k.med) return false;
    model_.postav(*k.bold, *k.ital, *k.med);
    return p_.ok && model_.sit.pocet > 0;
  }
  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    k.pozadi(r, 0.385f, 0.43f, 0.9f);
    const float t = (float)r.t;
    const float pomer = r.pomer();
    const float fov = 0.50f;
    const float vzd = Kamera3D::vzdalenostPro(44.f, 19.f, fov, pomer, 0.95f);
    const Kamera3D kam = Kamera3D::spocti(V3(0, 0, 0), 0.42f, vzd, fov, pomer, 0.f, 0.15f);
    const float yaw = 0.28f * std::sin(t * 0.42f) - r.d * 0.50f;
    const float vznos = 0.35f * std::sin(t * 1.1f) * r.fokus;
    const M4 model = M4::posun(0.f, vznos, 0.f) * M4::rotY(yaw) * M4::posun(0.f, -3.2f, 0.f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    p_.pouzij();
    p_.f3("uKamera", kam.oko);
    const V3 lampa(-45.f, 80.f, 70.f);
    p_.f3("uLampaPos", lampa);
    p_.f3("uLampaSmer", napdemo::norm(V3(0, 0, 0) - lampa));
    p_.f3("uLampa", V3(1.85f, 1.55f, 1.18f));
    p_.f3("uOknoPos", V3(55.f, 50.f, -75.f));
    p_.f3("uOkno", r.barva * 0.55f + V3(0.10f, 0.11f, 0.15f));
    p_.f1("uBlesk", 0.f);
    p_.f1("uAmb", 0.045f);
    p_.f3("uEnergiePos", V3(0.f, -6.f, 26.f));
    p_.f3("uEnergie", r.barva * (0.45f + 0.25f * r.fokus));
    // klavesy jemne sviti zespodu, kdyz je karta vybrana (jako neon v demu)
    p_.f1("uKlavZar", 0.10f * r.fokus * (0.75f + 0.25f * std::sin(t * 2.2f)));
    p_.f1("uLed", 1.f);
    p_.f1("uCas", t);
    p_.f1("uExpo", 1.22f);
    p_.f1("uObrJas", 1.f);
    p_.tex("uAtlas", 0, model_.atlas);
    p_.tex("uSum", 1, k.texSum);
    p_.tex("uObr", 2, k.texSum);
    p_.m4("uModel", model);
    p_.m4("uMVP", kam.vp * model);
    model_.sit.kresli();
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
  }
  const char *nazev() const override { return "3d:atari"; }

 private:
  napdemo::ModelAtari model_;
  Prog p_;
};

class UmeniKonzole : public Umeni {
 public:
  explicit UmeniKonzole(int bitu) : bitu_(bitu) {}
  bool init(KontextUmeni &k) override {
    (void)k;
    p_.sestav(napdemo::VS_MODEL, sSumem(napdemo::FS_KONZOLE).c_str(), bitu_ == 16 ? "menu-nap16" : "menu-nap32");
    if (bitu_ == 16) model_.postav16(); else model_.postav32();
    return p_.ok && model_.sit.pocet > 0;
  }
  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    k.pozadi(r, 0.39f, 0.40f, 0.9f);
    const float t = (float)r.t;
    const float pomer = r.pomer();
    const float fov = 0.50f;
    const bool k16 = bitu_ == 16;
    // N&P 16 je vyssi (cartridge), N&P 32 ma vpredu disk
    const float sirka = k16 ? 33.f : 34.f, vys = k16 ? 22.f : 21.f;
    const float vzd = Kamera3D::vzdalenostPro(sirka, vys, fov, pomer, 0.90f);
    const Kamera3D kam = Kamera3D::spocti(V3(0, 0, 0), 0.50f, vzd, fov, pomer, 0.f, 0.20f);
    const float yaw = (k16 ? 0.30f : -0.30f) * std::sin(t * 0.40f + (k16 ? 0.f : 1.7f)) - r.d * 0.50f;
    const float vznos = 0.35f * std::sin(t * 1.1f + 0.8f) * r.fokus;
    const float stredY = k16 ? -5.0f : -3.8f, stredZ = k16 ? 0.f : -2.0f;
    const M4 model = M4::posun(0.f, vznos, 0.f) * M4::rotY(yaw) * M4::posun(0.f, stredY, stredZ);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    p_.pouzij();
    p_.f3("uKamera", kam.oko);
    const V3 lampa(-45.f, 80.f, 70.f);
    p_.f3("uLampaPos", lampa);
    p_.f3("uLampaSmer", napdemo::norm(V3(0, 0, 0) - lampa));
    p_.f3("uLampa", V3(1.50f, 1.28f, 1.0f));
    p_.f3("uOknoPos", V3(55.f, 50.f, -75.f));
    p_.f3("uOkno", V3(0.14f, 0.16f, 0.22f));
    p_.f1("uBlesk", 0.f);
    p_.f1("uAmb", 0.16f);
    p_.f1("uExpo", 1.35f);
    p_.f1("uCas", t);
    // neonova svetla v barve karty (zleva zepredu) a doplnkove (zprava zezadu)
    p_.f3("uNeon1Pos", V3(-30.f, 22.f, 30.f));
    p_.f3("uNeon1", r.barva * (1.4f + 0.4f * r.fokus));
    p_.f3("uNeon2Pos", V3(32.f, 18.f, -20.f));
    p_.f3("uNeon2", (k16 ? V3(1.0f, 0.45f, 0.25f) : V3(0.25f, 0.85f, 1.0f)) * 1.1f);
    p_.f1("uOdhal", 1000.f);
    p_.f3("uOdhalBarva", r.barva);
    p_.f1("uSvit", 1.f);
    // odlesk pres zlaty emblem N&P jednou za 5 s
    const float f = std::fmod(t + (k16 ? 0.f : 2.5f), 5.f) / 1.6f;
    p_.f1("uLogoLesk", -0.5f + 2.4f * std::min(f, 1.f));
    p_.tex("uLogo", 0, k.texLogo ? k.texLogo : k.texSum);
    p_.tex("uSum", 1, k.texSum);
    p_.m4("uModel", model);
    p_.m4("uMVP", kam.vp * model);
    model_.sit.kresli();
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_DEPTH_TEST);
  }
  const char *nazev() const override { return bitu_ == 16 ? "3d:nap16" : "3d:nap32"; }

 private:
  int bitu_;
  napdemo::ModelKonzole model_;
  Prog p_;
};

}  // namespace napmenu
