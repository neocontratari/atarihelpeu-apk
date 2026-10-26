// nap_menu_umeni.h
// ====================================================================
//  B311: ZIVE OBRAZKY KARET - spolecny zaklad.
//
//  Kazda karta ma "umeni" (obrazek), ktere se kresli do vlastniho
//  framebufferu karty: zive v C++ (3D Atari 130XE, konzole N&P 16 a N&P 32,
//  kazeta, dzubox, zemekoule, kniha s navodem) nebo obrazek ze souboru
//  (png / jpg v assets) - to je cesta pro dalsi masiny: obrazek + radek
//  v karty.txt a hotovo.
//
//  Nazev a podtitul karty dole dopise menu samo (nap_menu_kresba.h),
//  proto obrazek karty NEMA mit text.
//  Bez Androidu: soubory nacita funkce, kterou dodá platforma (telefon:
//  AAssetManager, PC test: fopen).
// ====================================================================
#pragma once
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../intro/nap_demo_gl.h"
#include "../intro/nap_demo_text.h"
#include "../intro/nap_demo_shadery.h"
#include "../vendor/stb/stb_image.h"
#include "nap_menu_karty.h"
#include "nap_menu_shadery.h"

namespace napmenu {

using napdemo::Ctverec;
using napdemo::Fbo;
using napdemo::M4;
using napdemo::Pismo;
using napdemo::Prog;
using napdemo::V2;
using napdemo::V3;

inline std::string sSumem(const char *s) {
  std::string x(s);
  const size_t i = x.find("%SUM%");
  if (i != std::string::npos) x.replace(i, 5, napdemo::SUM_GLSL);
  return x;
}

inline V3 barvaV3(uint32_t rgb) {
  return V3(((rgb >> 16) & 255) / 255.f, ((rgb >> 8) & 255) / 255.f, (rgb & 255) / 255.f);
}

// nacteni souboru z assets (cesta relativne k assets, napr. "menu/karta_atari.jpg")
using NactiSoubor = std::function<std::vector<unsigned char>(const std::string &)>;

// co dostane umeni pri kresleni
struct RamecUmeni {
  int w = 0, h = 0;              // framebuffer karty (body)
  double t = 0;                  // cas menu (s)
  float d = 0;                   // vzdalenost karty od stredu karuselu (paralaxa)
  float fokus = 0;               // 1 = karta je uprostred
  V3 barva{1, 0.8f, 0.4f};       // barva karty
  const Karta *karta = nullptr;
  bool nahled = false;           // B313: obrazek pro HTML zalohu - animace v konecnem stavu (konzole zapnute)
  float pomer() const { return h > 0 ? (float)w / (float)h : 0.75f; }
};

// spolecne prostredky vsech umeni (sum, logo, pisma, obdelnik)
struct KontextUmeni {
  const Pismo *bold = nullptr, *semi = nullptr, *med = nullptr, *ital = nullptr;
  GLuint texSum = 0, texLogo = 0;
  float logoPomer = 1.f;
  Ctverec ctverec;
  Prog pPozadi;                  // FS_KARTA_POZADI (spolecne pozadi karty)
  Prog pObr;                     // obdelnik s texturou (premultiplied)
  Prog pMinca;                   // minca N&P z loga (vyrizla z ctvercoveho obrazku)
  Prog pText;                    // text obarveny prechodem (zlato) s odleskem
  Prog pUi;                      // ikony a zare (FS_UI)
  NactiSoubor nacti;
  GLuint vboObd = 0;
  bool en = false;               // B314: obrazky s textem anglicky (kniha HELP)

  bool init() {
    ctverec.priprav();
    pPozadi.sestav(napdemo::VS_PLNY, sSumem(FS_KARTA_POZADI).c_str(), "menu-karta-pozadi");
    pObr.sestav(VS_OBD2, FS_OBR, "menu-obr");
    pMinca.sestav(VS_OBD2, FS_LOGO_MENU, "menu-minca");
    pText.sestav(VS_OBD2, FS_TEXT, "menu-umeni-text");
    pUi.sestav(VS_OBD2, FS_UI, "menu-umeni-ui");
    return pPozadi.ok && pObr.ok && pMinca.ok && pText.ok && pUi.ok;
  }

  // text (bila sazba TextTex) se stredem (cx, cy) v NDC, vyska textu h (NDC), obarveny
  void textNdc(const napdemo::TextTex &t, float cx, float cy, float h, float pomer, const V3 &b1, const V3 &b2, float alfa, float lesk) {
    if (!t.tex) return;
    const float w = h * (float)t.w / (float)t.h / pomer;
    pText.pouzij();
    pText.f3("uBarva1", b1);
    pText.f3("uBarva2", b2);
    pText.f1("uAlfa", alfa);
    pText.f1("uLesk", lesk);
    pText.tex("uTex", 0, t.tex);
    obd(pText, cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f, 0.f, 1.f, 1.f, 0.f);
  }
  // mekka zare (pricita se) se stredem v NDC, polomery v NDC
  void zare(float cx, float cy, float rx, float ry, const V3 &barva) {
    pUi.pouzij();
    pUi.i1("uTyp", 6);
    pUi.f2("uVel", 100.f, 100.f * ry / std::max(rx, 1e-4f));
    pUi.f3("uBarva", barva);
    pUi.f1("uAlfa", 1.f);
    pUi.f1("uUhel", 0.f);
    pUi.f1("uSvit", 0.f);
    obd(pUi, cx - rx, cy - ry, cx + rx, cy + ry);
  }

  // minca N&P se stredem (cx, cy) a polomerem r - v NDC cile (pomer = sirka/vyska cile,
  // r je v jednotkach vysky); lesk = poloha odlesku
  void minca(float cx, float cy, float r, float pomer, float alfa, float lesk) {
    if (!texLogo) return;
    const float RX = 0.4165f, RY = 0.4135f, CX = 0.497f, CY = 0.493f;
    const float lw = r / RX / pomer, lh = r / RY;           // cely obrazek loga v NDC
    const float x0 = cx - lw * CX * 2.f, y1 = cy + lh * CY * 2.f;   // levy horni roh
    pMinca.pouzij();
    pMinca.f1("uAlfa", alfa);
    pMinca.f1("uLesk", lesk);
    pMinca.f4("uKruh", CX, CY, RX, RY);
    pMinca.f1("uAA", 0.02f);
    pMinca.tex("uTex", 0, texLogo);
    obd(pMinca, x0, y1 - lh * 2.f, x0 + lw * 2.f, y1, 0.f, 1.f, 1.f, 0.f);
  }

  // obdelnik v NDC (x0,y0)-(x1,y1) s texturou (u0,v0)-(u1,v1); program uz musi byt pouzity
  void obd(Prog &p, float x0, float y0, float x1, float y1, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1) {
    p.f4("uObd", x0, y0, x1, y1);
    p.f4("uUV", u0, v0, u1, v1);
    ctverec.kresli();
  }

  // Spolecne pozadi karty: zare v barve karty, kuzel svetla, zlaty kruh na podlaze,
  // stin pod predmetem. podlaha = y stredu kruhu (0 dole .. 1 nahore), kruh = polomer
  // (dil sirky), stin = 0..1. vlna = svitici zvukova vlna za predmetem (kazeta).
  void pozadi(const RamecUmeni &r, float podlaha, float kruh, float stin, float vlna = 0.f) {
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    pPozadi.pouzij();
    pPozadi.f3("uBarva", r.barva);
    pPozadi.f1("uCas", (float)r.t);
    pPozadi.f1("uPomer", r.pomer());
    pPozadi.f1("uPodlaha", podlaha);
    pPozadi.f1("uKruh", kruh);
    pPozadi.f1("uStin", stin);
    pPozadi.f1("uFokus", r.fokus);
    pPozadi.f1("uVlna", vlna);
    pPozadi.tex("uSum", 0, texSum);
    ctverec.kresli();
  }
};

// Obrazek karty (zivy nebo ze souboru)
class Umeni {
 public:
  virtual ~Umeni() {}
  virtual bool init(KontextUmeni &k) = 0;
  // kresli do prave nastaveneho framebufferu (s hloubkou), viewport uz je nastaveny
  virtual void kresli(KontextUmeni &k, const RamecUmeni &r) = 0;
  // meni se v case? (staticky obrazek se kresli jen jednou)
  virtual bool zive() const { return true; }
  virtual const char *nazev() const = 0;
};

// ---- obrazek ze souboru (png / jpg): "cover" - vyplni kartu, prebytek se orizne ----
class UmeniObrazek : public Umeni {
 public:
  explicit UmeniObrazek(const std::string &cesta) : cesta_(cesta) {}
  ~UmeniObrazek() override { if (tex_) glDeleteTextures(1, &tex_); }
  bool init(KontextUmeni &k) override {
    if (!k.nacti) return false;
    std::vector<unsigned char> d = k.nacti(cesta_);
    if (d.empty()) { NAPDEMO_LOG("MENU obrazek %s nenalezen", cesta_.c_str()); return false; }
    int c = 0;
    unsigned char *px = stbi_load_from_memory(d.data(), (int)d.size(), &w_, &h_, &c, 4);
    if (!px) { NAPDEMO_LOG("MENU obrazek %s se nepodarilo dekodovat", cesta_.c_str()); return false; }
    // premultiplied alfa (png s pruhlednosti)
    for (size_t i = 0; i < (size_t)w_ * h_; i++) {
      const unsigned a = px[i * 4 + 3];
      if (a < 255) for (int j = 0; j < 3; j++) px[i * 4 + j] = (unsigned char)(px[i * 4 + j] * a / 255);
    }
    tex_ = napdemo::novaTextura(w_, h_, GL_RGBA, px, true);
    stbi_image_free(px);
    return tex_ != 0;
  }
  void kresli(KontextUmeni &k, const RamecUmeni &r) override {
    k.pozadi(r, 0.33f, 0.36f, 0.f);
    if (!tex_) return;
    // cover: zachovat pomer stran obrazku
    const float pk = r.pomer(), po = h_ > 0 ? (float)w_ / h_ : 1.f;
    float u0 = 0, u1 = 1, v0 = 0, v1 = 1;
    if (po > pk) { const float s = pk / po; u0 = 0.5f - s * 0.5f; u1 = 0.5f + s * 0.5f; }
    else { const float s = po / pk; v0 = 0.5f - s * 0.5f; v1 = 0.5f + s * 0.5f; }
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    k.pObr.pouzij();
    k.pObr.f1("uAlfa", 1.f);
    k.pObr.tex("uTex", 0, tex_);
    // textura je shora dolu (radek 0 = nahore) -> v obracene
    k.obd(k.pObr, -1, -1, 1, 1, u0, v1, u1, v0);
    glDisable(GL_BLEND);
  }
  bool zive() const override { return false; }
  const char *nazev() const override { return "obrazek"; }

 private:
  std::string cesta_;
  GLuint tex_ = 0;
  int w_ = 0, h_ = 0;
};

// Studiove svetlo pro FS_STUDIO (kazeta, kniha): hlavni svetlo zleva shora,
// doplnkove zprava, protisvetlo zezadu v barve karty.
inline void nastavStudio(Prog &p, const V3 &oko, const V3 &barva, float jas = 1.f) {
  p.f3("uKamera", oko);
  p.f3("uKlic", napdemo::norm(V3(-0.55f, 0.75f, 0.60f)));
  p.f3("uKlicBarva", V3(1.30f, 1.12f, 0.90f) * jas);
  p.f3("uVypln", napdemo::norm(V3(0.80f, 0.20f, 0.55f)));
  p.f3("uVyplnBarva", V3(0.22f, 0.25f, 0.32f) * jas);
  p.f3("uKontra", napdemo::norm(V3(0.30f, 0.50f, -0.80f)));
  p.f3("uKontraBarva", barva * (0.85f * jas));
  p.f3("uAmb", V3(0.10f, 0.09f, 0.08f));
  p.f1("uExpo", 1.25f);
}

// Kamera pro 3D predmet v karte: perspektiva + posun obrazu (predmet nemusi byt
// uprostred karty), vraci matici pohled*projekce a polohu oka.
struct Kamera3D {
  M4 vp;
  V3 oko;
  // cil = bod, na ktery se kamera diva; vyska = uhel nad vodorovnou (rad);
  // vzdal = vzdalenost; fov = svisly zorny uhel; posunY = kam do karty (NDC, + nahoru)
  static Kamera3D spocti(const V3 &cil, float vyska, float vzdal, float fov, float pomer, float posunX, float posunY) {
    Kamera3D k;
    k.oko = V3(cil.x, cil.y + std::sin(vyska) * vzdal, cil.z + std::cos(vyska) * vzdal);
    const M4 proj = M4::posun(posunX, posunY, 0.f) * M4::perspektiva(fov, pomer, std::max(1.f, vzdal * 0.2f), vzdal * 4.f);
    k.vp = proj * M4::pohled(k.oko, cil, V3(0, 1, 0));
    return k;
  }
  // vzdalenost, aby se predmet sirky w (a vysky h) vesel do dilu "vyplnit" karty
  static float vzdalenostPro(float sirka, float vyskaP, float fov, float pomer, float vyplnit) {
    const float t = std::tan(fov * 0.5f);
    return std::max(vyskaP / (2.f * t * vyplnit), sirka / (2.f * t * pomer * vyplnit));
  }
};

}  // namespace napmenu
