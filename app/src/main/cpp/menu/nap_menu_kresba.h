// nap_menu_kresba.h
// ====================================================================
//  B311: KRESLENI HLAVNIHO MENU (OpenGL ES 2.0, cele v C++)
//
//  Snimek:
//   1. pozadi: zlaty vesmir (mlhovina, kruhy portalu) v mensim rozliseni
//      + hvezdy jako body v plnem rozliseni
//   2. hlavicka: minca N&P, napis N&P VISION / RETRO UNIVERSE, kolecko OPTIONS
//   3. karty karuselu: kazda ma vlastni framebuffer s obrazkem (zivy 3D /
//      efekt / obrazek ze souboru) a nazvem; na obrazovku jde jako ctyruhelnik
//      v perspektive (zlaty ramecek se zkosenymi rohy, odlesk skla, odraz na
//      podlaze), od nejvzdalenejsi po prostredni
//   4. pod kartami: popis vybrane karty (prolina se), tlacitko, tecky, podpis
//   5. animace: nastup menu, spusteni karty (prilet + zatmeni), zamcena
//      karta (zatrese se), kolecko (otoci se)
//  Obrazky karet se prekresluji jen kdyz je potreba (prostredni kazdy snimek,
//  ostatni podle vykonu - "uroven" 0..3 nastavuje vlakno podle casu snimku).
//
//  Bez Androidu (testuje se pres Mesa na PC: test_overeni/b311).
// ====================================================================
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "../atari/nap_atari_fonts.h"
#include "../atari/nap_atari_fonts2.h"
#include "nap_menu_karusel.h"
#include "nap_menu_umeni_vse.h"

namespace napmenu {

using napdemo::Odstavec;
using napdemo::Sazec;
using napdemo::TextTex;

inline float easeOut3(float t) { t = std::min(std::max(t, 0.f), 1.f); return 1.f - (1.f - t) * (1.f - t) * (1.f - t); }
inline float easeIn2(float t) { t = std::min(std::max(t, 0.f), 1.f); return t * t; }

// stav menu pro jeden snimek (kopie z JNI pod zamkem)
struct StavMenu {
  double t = 0;
  float poz = 0;
  bool pohyb = false;              // karusel jede nebo je prst dole
  int spustena = -1;               // spoustena karta (animace)
  double tSpusteni = -1;
  int zamekKlep = -1;              // klepnuti na zamcenou kartu
  double tZamek = -1;
  double tOzub = -1;               // klepnuti na kolecko
  double tPrichod = -10;           // nastup menu
};

constexpr float SPUSTENI_S = 0.30f;   // delka animace spusteni (Java po ni prepne stranku)

// kam se kresli: displej telefonu nebo obraz pro TV (16:9)
struct CilMenu {
  int w = 0, h = 0;
  bool tv = false;
  GLuint fbo = 0;
  bool hlavni = true;              // podle hlavniho cile se urci velikost obrazku karet
  // --- vnitrni ---
  Rozlozeni roz;
  int rozW = -1, rozH = -1, rozN = -1;
  TextTex titul, slogan, podpis, napoveda;
  std::vector<TextTex> popis, zamek, tlac;
  uint32_t textVerze = 0xFFFFFFFFu;
  int textW = -1, textH = -1;
  Fbo pozadi;
  int snimku = 0;
  std::vector<float> hvezdy;
  GLuint vboHvezdy = 0;
  int hvezdW = -1, hvezdH = -1;
  void uvolniTexty() {
    titul.uvolni(); slogan.uvolni(); podpis.uvolni(); napoveda.uvolni();
    for (auto &t : popis) t.uvolni();
    for (auto &t : zamek) t.uvolni();
    for (auto &t : tlac) t.uvolni();
    popis.clear(); zamek.clear(); tlac.clear();
    textW = textH = -1;
  }
  void uvolni() {
    uvolniTexty();
    pozadi.uvolni();
    if (vboHvezdy) glDeleteBuffers(1, &vboHvezdy);
    vboHvezdy = 0;
    hvezdW = hvezdH = -1;
    rozW = rozH = -1;
  }
};

class Kresba {
 public:
  // ---------------- priprava ----------------
  bool init(const unsigned char *logoRGBA, int logoW, int logoH, NactiSoubor nacti, bool npotMipmapy) {
    pBold_.init(NAP_FONT_CHAKRA_BOLD_TTF);
    pSemi_.init(NAP_FONT_CHAKRA_SEMIBOLD_TTF);
    pMed_.init(NAP_FONT_CHAKRA_MEDIUM_TTF);
    pItal_.init(NAP_FONT_CHAKRA_BOLDITALIC_TTF);
    k_.bold = &pBold_; k_.semi = &pSemi_; k_.med = &pMed_; k_.ital = &pItal_;
    k_.nacti = nacti;
    mipmapy_ = npotMipmapy;
    bool ok = k_.init();
    pKarta_.sestav(VS_KARTA, FS_KARTA, "menu-karta");
    pText_.sestav(VS_OBD2, FS_TEXT, "menu-text");
    pUi_.sestav(VS_OBD2, FS_UI, "menu-ui");
    pLogo_.sestav(VS_OBD2, FS_LOGO_MENU, "menu-logo");
    pPozadi_.sestav(napdemo::VS_PLNY, sSumem(FS_POZADI).c_str(), "menu-pozadi");
    pKopie_.sestav(VS_OBD2, FS_KOPIE_MENU, "menu-kopie");
    pHvezdy_.sestav(VS_HVEZDY, FS_HVEZDY, "menu-hvezdy");
    pStitek_.sestav(VS_OBD2, FS_STITEK, "menu-stitek");
    pZamceno_.sestav(VS_OBD2, FS_ZAMCENO, "menu-zamceno");
    glGenBuffers(1, &vboKarta_);
    // sum: stejny jako v uvodnim demu (R nahoda, G = R posunute o 37,17, B dalsi)
    {
      std::vector<unsigned char> n(256 * 256 * 4);
      napdemo::Los los(20261120);
      for (int i = 0; i < 256 * 256; i++) { n[i * 4] = (unsigned char)(los.dalsi() >> 24); n[i * 4 + 2] = (unsigned char)(los.dalsi() >> 24); n[i * 4 + 3] = 255; }
      for (int y = 0; y < 256; y++)
        for (int x = 0; x < 256; x++) n[(y * 256 + x) * 4 + 1] = n[(((y + 17) & 255) * 256 + ((x + 37) & 255)) * 4];
      k_.texSum = napdemo::novaTextura(256, 256, GL_RGBA, n.data(), true, true);
    }
    if (logoRGBA && logoW > 0 && logoH > 0) nastavLogo(logoRGBA, logoW, logoH);
    ok = ok && pBold_.ok && pMed_.ok && pKarta_.ok && pText_.ok && pUi_.ok && pPozadi_.ok && pKopie_.ok && pHvezdy_.ok && pStitek_.ok && pZamceno_.ok;
    pripraveno_ = ok;
    NAPDEMO_LOG("MENU kresba pripravena: %s (pisma %d, logo %s, mipmapy karet %s)", ok ? "ANO" : "NE", (int)pBold_.ok, k_.texLogo ? "ano" : "NE",
                mipmapy_ ? "ano" : "ne");
    return ok;
  }

  bool pripraveno() const { return pripraveno_; }
  const std::string &log() const { return log_; }
  void vymazLog() { log_.clear(); }

  // seznam karet (z karty.txt) - umeni se pripravi az pri prvnim kresleni karty
  void nastavKarty(const std::vector<Karta> &karty) {
    karty_ = karty;
    for (auto &s : sloty_) uvolniSlot(s);
    sloty_.clear();
    sloty_.resize(karty_.size());
    verze_++;
  }
  int pocetKaret() const { return (int)karty_.size(); }
  const Karta &karta(int i) const { return karty_[(size_t)i]; }

  void nastavZamek(int i, bool zamceno, const std::string &text) {
    if (i < 0 || i >= (int)karty_.size()) return;
    Karta &k = karty_[(size_t)i];
    if (k.zamceno == zamceno && k.zamekText == text) return;
    k.zamceno = zamceno;
    k.zamekText = text;
    verze_++;
  }

  // vykon: 3 nejvic (zive i karty vedle pri posouvani) .. 0 nejmin
  void nastavUroven(int u) { uroven_ = std::min(3, std::max(0, u)); }
  int uroven() const { return uroven_; }

  // kolik obrazku karet se v poslednim snimku prekreslilo (pro test a log)
  int prekreslenoMinule() const { return prekresleno_; }
  int slotuSFbo() const { int n = 0; for (auto &s : sloty_) if (s.fbo) n++; return n; }
  // velikost obrazku karet (body)
  int umeniW() const { return umW_; }
  int umeniH() const { return umH_; }
  std::string umeniNazvy() const {
    std::string s;
    for (size_t i = 0; i < sloty_.size(); i++) {
      if (i) s += ", ";
      s += karty_[i].id + "=" + (sloty_[i].umeni ? sloty_[i].umeni->nazev() : "-");
    }
    return s;
  }

  // ---------------- jeden snimek ----------------
  void snimek(const StavMenu &s, CilMenu &c) {
    if (!pripraveno_ || c.w <= 0 || c.h <= 0 || karty_.empty()) return;
    const int n = (int)karty_.size();
    if (c.rozW != c.w || c.rozH != c.h || c.rozN != n) {
      c.roz = spoctiRozlozeni(c.w, c.h, c.tv, n);
      c.rozW = c.w; c.rozH = c.h; c.rozN = n;
    }
    const Rozlozeni &r = c.roz;
    if (c.hlavni) pripravVelikostUmeni(r);
    pripravTexty(c);
    pripravHvezdy(c);
    c.snimku++;

    // ---- animace ----
    const float t = (float)s.t;
    const float prichod = easeOut3((float)((s.t - s.tPrichod) / 0.65));
    const float uiAlfa = napdemo::smooth(0.f, 0.45f, (float)(s.t - s.tPrichod));
    float spust = 0.f;
    if (s.spustena >= 0 && s.tSpusteni >= 0) spust = std::min(1.f, (float)((s.t - s.tSpusteni) / SPUSTENI_S));
    // nastup: karty prijedou zprava
    const float pozVid = s.poz - (1.f - prichod) * 1.6f;

    // ---- obrazky karet (do vlastnich framebufferu) ----
    std::vector<Poza> pozy((size_t)n);
    for (int i = 0; i < n; i++) pozy[(size_t)i] = spoctiPozu(r, i, pozVid);
    if (c.hlavni) aktualizujUmeni(s, pozy, r);

    // ---- obrazovka ----
    glBindFramebuffer(GL_FRAMEBUFFER, c.fbo);
    glViewport(0, 0, c.w, c.h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    const float vybrF = std::min(std::max(s.poz, 0.f), (float)(n - 1));
    const int i0 = (int)std::floor(vybrF), i1 = std::min(n - 1, i0 + 1);
    const float fr = vybrF - (float)i0;
    const V3 barva = napdemo::lerp(barvaV3(karty_[(size_t)i0].barva), barvaV3(karty_[(size_t)i1].barva), fr);
    kresliPozadi(c, s, barva);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    kresliHvezdy(c, t, uiAlfa);

    // ---- karty: od nejvzdalenejsi ----
    std::vector<int> poradi;
    for (int i = 0; i < n; i++) if (pozy[(size_t)i].videt) poradi.push_back(i);
    std::sort(poradi.begin(), poradi.end(), [&](int a, int b) { return std::fabs(pozy[(size_t)a].d) > std::fabs(pozy[(size_t)b].d); });
    // zare za prostredni kartou
    {
      const int st = std::min(std::max((int)std::lround(pozVid), 0), n - 1);
      const Poza &p = pozy[(size_t)st];
      const float fok = 1.f - std::min(1.f, std::fabs(p.d) * 1.6f);
      if (fok > 0.01f && p.videt) {
        glBlendFunc(GL_ONE, GL_ONE);
        const float rw = r.cw * 1.25f, rh = r.ch * 1.08f;
        ui(c, 6, p.x - rw * 0.5f, p.y - rh * 0.5f, rw, rh, barvaV3(karty_[(size_t)st].barva) * (0.30f * fok * uiAlfa * (1.f - spust)), 1.f);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      }
    }
    for (int i : poradi) {
      Poza p = pozy[(size_t)i];
      float zablesk = 0.f, alfa = p.alfa * std::min(1.f, prichod * 1.4f);
      if (spust > 0.f) {
        if (i == s.spustena) {
          // spoustena karta: priblizi se a zablesk
          const float k = easeIn2(spust);
          p.s *= 1.f + 0.12f * k;
          zablesk = 0.55f * std::sin(std::min(1.f, spust * 1.3f) * 3.14159f);
          for (int q = 0; q < 4; q++) {
            const float u = (q == 0 || q == 3) ? -1.f : 1.f, v = q < 2 ? -1.f : 1.f;
            promitni(r, p, u, v, p.rx[q], p.ry[q], p.rw[q]);
          }
        } else {
          alfa *= 1.f - spust;
        }
      }
      if (s.zamekKlep == i && s.tZamek >= 0) {
        const float k = (float)((s.t - s.tZamek) / 0.5);
        if (k >= 0.f && k < 1.f) {
          const float dx = std::sin(k * 3.14159f * 6.f) * (1.f - k) * r.cw * 0.035f;
          p.x += dx;
          for (int q = 0; q < 4; q++) p.rx[q] += dx;
        }
      }
      kresliKartu(c, p, i, alfa, zablesk, t);
    }

    // ---- hlavicka ----
    kresliHlavicku(c, s, uiAlfa * (1.f - spust));
    // ---- pod kartami ----
    kresliSpodek(c, s, pozVid, barva, uiAlfa * (1.f - spust));
    // ---- zatmeni pri spusteni ----
    if (spust > 0.f) {
      const float a = napdemo::smooth(0.35f, 1.0f, spust);
      if (a > 0.f) ui(c, 0, 0, 0, (float)c.w, (float)c.h, V3(0, 0, 0), a);
    }
    glDisable(GL_BLEND);
  }

  // co je na obrazovce pro test: karta a jeji poloha
  const KontextUmeni &kontext() const { return k_; }

  // Export obrazku karty (test, obrazky pro HTML zalohu): karta i v case t,
  // velikost w x h, bez nazvu (sNazvem = false). Vraci framebuffer s obrazkem.
  GLuint exportKarty(int i, double t, int w, int h, bool sNazvem) {
    if (i < 0 || i >= (int)karty_.size()) return 0;
    nastavVelikostUmeni(w, h, (float)w, (float)h);
    jenObrazek_ = !sNazvem;
    StavMenu s;
    s.t = t;
    s.poz = (float)i;
    Poza p;
    p.i = i;
    p.d = 0.f;
    kresliUmeni(i, s, p);
    jenObrazek_ = false;
    return sloty_[(size_t)i].fbo;
  }

 private:
  // ----- slot obrazku karty -----
  struct Slot {
    std::unique_ptr<Umeni> umeni;
    bool pokus = false;              // umeni uz se zkouselo vyrobit
    GLuint tex = 0, fbo = 0;
    int w = 0, h = 0;
    bool platne = false;
    double tKresleno = -1;
    uint32_t verze = 0;
    TextTex nazev, podtitul, zamekNapis, zamekDuvod;
    int textW = -1;
    uint32_t textVerze = 0xFFFFFFFFu;
  };

  KontextUmeni k_;
  Pismo pBold_, pSemi_, pMed_, pItal_;
  Prog pKarta_, pText_, pUi_, pLogo_, pPozadi_, pKopie_, pHvezdy_, pStitek_, pZamceno_;
  GLuint vboKarta_ = 0;
  bool pripraveno_ = false, mipmapy_ = false;
  std::vector<Karta> karty_;
  std::vector<Slot> sloty_;
  uint32_t verze_ = 1;
  int uroven_ = 2;
  int umW_ = 0, umH_ = 0;
  GLuint hloubka_ = 0;              // spolecny hloubkovy buffer obrazku karet
  GLuint tmpTex_ = 0, tmpFbo_ = 0;  // pomocny framebuffer (zamcena karta)
  int prekresleno_ = 0;
  bool jenObrazek_ = false;         // export: obrazek karty bez nazvu
  unsigned snimekCislo_ = 0;
  int kolo_ = 0;                    // ktera vedlejsi karta je na rade
  std::string log_;

  void nastavLogo(const unsigned char *rgba, int w, int h) {
    // mipmapy jen u 2^n: logo (516x520) prevzorkovat na 512x512
    std::vector<unsigned char> pot;
    const unsigned char *data = rgba;
    int tw = w, th = h;
    if ((tw & (tw - 1)) || (th & (th - 1))) {
      tw = th = 512;
      pot.resize((size_t)tw * th * 4);
      for (int y = 0; y < th; y++)
        for (int x = 0; x < tw; x++) {
          const float fx = (x + 0.5f) * w / tw - 0.5f, fy = (y + 0.5f) * h / th - 0.5f;
          const int x0 = std::max(0, (int)std::floor(fx)), y0 = std::max(0, (int)std::floor(fy));
          const int x1 = std::min(w - 1, x0 + 1), y1 = std::min(h - 1, y0 + 1);
          const float ax = napdemo::clampf(fx - x0, 0.f, 1.f), ay = napdemo::clampf(fy - y0, 0.f, 1.f);
          for (int q = 0; q < 4; q++) {
            const float a = rgba[((size_t)y0 * w + x0) * 4 + q], b = rgba[((size_t)y0 * w + x1) * 4 + q];
            const float c2 = rgba[((size_t)y1 * w + x0) * 4 + q], d = rgba[((size_t)y1 * w + x1) * 4 + q];
            pot[((size_t)y * tw + x) * 4 + q] = (unsigned char)(napdemo::lerpf(napdemo::lerpf(a, b, ax), napdemo::lerpf(c2, d, ax), ay) + 0.5f);
          }
        }
      data = pot.data();
    }
    k_.texLogo = napdemo::novaTextura(tw, th, GL_RGBA, data, true);
    glBindTexture(GL_TEXTURE_2D, k_.texLogo);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glGenerateMipmap(GL_TEXTURE_2D);
    k_.logoPomer = (float)w / (float)h;
  }

  void uvolniSlot(Slot &s) {
    if (s.tex) glDeleteTextures(1, &s.tex);
    if (s.fbo) glDeleteFramebuffers(1, &s.fbo);
    s.tex = s.fbo = 0; s.w = s.h = 0; s.platne = false;
    s.nazev.uvolni(); s.podtitul.uvolni(); s.zamekNapis.uvolni(); s.zamekDuvod.uvolni();
    s.textW = -1;
  }

  // velikost obrazku karet podle prostredni karty na hlavnim cili (1:1, nejvys 800 bodu)
  void pripravVelikostUmeni(const Rozlozeni &r) {
    int w = (int)std::ceil(r.cw), h = (int)std::ceil(r.ch);
    if (w > 800) { h = (int)std::ceil(h * 800.f / w); w = 800; }
    nastavVelikostUmeni(w, h, r.cw, r.ch);
  }
  void nastavVelikostUmeni(int w, int h, float cw, float ch) {
    w = std::max(64, (w + 7) & ~7);
    h = std::max(64, (h + 7) & ~7);
    if (w == umW_ && h == umH_) return;
    const Rozlozeni r = [&] { Rozlozeni x; x.cw = cw; x.ch = ch; return x; }();
    umW_ = w; umH_ = h;
    for (auto &s : sloty_) uvolniSlot(s);
    if (hloubka_) glDeleteRenderbuffers(1, &hloubka_);
    glGenRenderbuffers(1, &hloubka_);
    glBindRenderbuffer(GL_RENDERBUFFER, hloubka_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, w, h);
    if (tmpTex_) glDeleteTextures(1, &tmpTex_);
    if (tmpFbo_) glDeleteFramebuffers(1, &tmpFbo_);
    tmpTex_ = tmpFbo_ = 0;
    char b[160];
    std::snprintf(b, sizeof b, "obrazky karet %dx%d bodu (karta na obrazovce %.0fx%.0f)\n", w, h, r.cw, r.ch);
    log_ += b;
  }

  bool pripravFbo(GLuint &tex, GLuint &fbo, bool mip) {
    if (fbo) return true;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, umW_, umH_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, mip ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, hloubka_);
    const GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
      char b[96];
      std::snprintf(b, sizeof b, "framebuffer karty %dx%d neni hotovy: 0x%x\n", umW_, umH_, st);
      log_ += b;
      return false;
    }
    return true;
  }

  // ---- ktere obrazky karet prekreslit ----
  void aktualizujUmeni(const StavMenu &s, const std::vector<Poza> &pozy, const Rozlozeni &r) {
    snimekCislo_++;
    prekresleno_ = 0;
    const int n = (int)karty_.size();
    const int stred = std::min(std::max((int)std::lround(s.poz), 0), n - 1);
    // karty mimo dosah: uvolnit framebuffer, kdyz je karet hodne (budouci rozsireni)
    if (n > 9) {
      for (int i = 0; i < n; i++)
        if (std::fabs(pozy[(size_t)i].d) > r.dosah + 1.5f && sloty_[(size_t)i].fbo) uvolniSlot(sloty_[(size_t)i]);
    }
    // 1) co musi byt (viditelna karta bez obrazku, zmena zamku) - vzdy
    for (int i = 0; i < n; i++) {
      const Poza &p = pozy[(size_t)i];
      if (!p.videt && std::fabs(p.d) > r.dosah + 0.6f) continue;
      Slot &sl = sloty_[(size_t)i];
      if (!sl.platne || sl.verze != verze_) kresliUmeni(i, s, p);
    }
    // 2) zive obrazky: prostredni podle urovne, ostatni po jednom dokola
    const int kazdy = uroven_ >= 2 ? 1 : (uroven_ == 1 ? 2 : 3);
    auto zivy = [&](int i) { const Slot &sl = sloty_[(size_t)i]; return sl.umeni && sl.umeni->zive() && !karty_[(size_t)i].zamceno; };
    if (zivy(stred) && pozy[(size_t)stred].videt && (snimekCislo_ % (unsigned)kazdy) == 0 && sloty_[(size_t)stred].tKresleno != s.t)
      kresliUmeni(stred, s, pozy[(size_t)stred]);
    int dalsich = s.pohyb ? (uroven_ >= 3 ? 2 : (uroven_ == 2 ? 1 : 0)) : (uroven_ >= 2 ? 1 : 0);
    for (int k = 0; k < n && dalsich > 0; k++) {
      kolo_ = (kolo_ + 1) % n;
      const int i = kolo_;
      if (i == stred || !pozy[(size_t)i].videt || !zivy(i)) continue;
      if (sloty_[(size_t)i].tKresleno == s.t) continue;
      // v klidu staci vedlejsi karty obcas (pomale otaceni), pri posouvani hned (paralaxa)
      if (!s.pohyb && s.t - sloty_[(size_t)i].tKresleno < 0.10) continue;
      kresliUmeni(i, s, pozy[(size_t)i]);
      dalsich--;
    }
  }

  void kresliUmeni(int i, const StavMenu &s, const Poza &p) {
    Slot &sl = sloty_[(size_t)i];
    const Karta &kar = karty_[(size_t)i];
    if (!sl.pokus) {
      sl.pokus = true;
      sl.umeni = vytvorUmeni(k_, kar, log_);
      log_ += "karta " + kar.id + ": " + sl.umeni->nazev() + "\n";
    }
    if (!pripravFbo(sl.tex, sl.fbo, mipmapy_)) return;
    sl.w = umW_; sl.h = umH_;
    pripravTextyKarty(sl, kar);
    RamecUmeni ram;
    ram.w = umW_; ram.h = umH_;
    ram.t = s.t;
    ram.d = std::max(-1.5f, std::min(1.5f, p.d));
    ram.fokus = 1.f - std::min(1.f, std::fabs(p.d));
    ram.barva = barvaV3(kar.barva);
    ram.karta = &kar;
    const bool zam = kar.zamceno;
    if (zam && !pripravFbo(tmpTex_, tmpFbo_, false)) return;
    glBindFramebuffer(GL_FRAMEBUFFER, zam ? tmpFbo_ : sl.fbo);
    glViewport(0, 0, umW_, umH_);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    if (zam) ram.fokus = 0.f;
    sl.umeni->kresli(k_, ram);
    glDisable(GL_DEPTH_TEST);
    glActiveTexture(GL_TEXTURE0);
    if (zam) {
      glBindFramebuffer(GL_FRAMEBUFFER, sl.fbo);
      glViewport(0, 0, umW_, umH_);
      pZamceno_.pouzij();
      pZamceno_.tex("uTex", 0, tmpTex_);
      k_.obd(pZamceno_, -1, -1, 1, 1);
    }
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (!jenObrazek_) kresliStitek(sl, kar, ram);
    if (zam) kresliZamekNaKarte(sl, ram);
    glDisable(GL_BLEND);
    if (mipmapy_) {
      glBindTexture(GL_TEXTURE_2D, sl.tex);
      glGenerateMipmap(GL_TEXTURE_2D);
    }
    sl.platne = true;
    sl.verze = verze_;
    sl.tKresleno = s.t;
    prekresleno_++;
  }

  // ---- texty v karte (nazev, podtitul, zamek) pro velikost obrazku ----
  static float sirkaTextu(const Pismo &p, const std::string &s, float px, float proklad) {
    float w = 0;
    const float sc = p.meritko(px);
    for (const char *q = s.c_str(); *q;) {
      const int cp = napdemo::utf8Dalsi(q);
      int adv, lsb;
      stbtt_GetCodepointHMetrics(&p.info, cp, &adv, &lsb);
      w += adv * sc + proklad * px;
    }
    return w;
  }
  static TextTex text1(const Pismo &p, const std::string &s, float px, float proklad, float maxW, float stin, float r = 1, float g = 1, float b = 1) {
    // zmensit, aby se veslo na jeden radek
    float w = sirkaTextu(p, s, px, proklad);
    if (w > maxW && w > 0) px *= maxW / w * 0.995f;
    Odstavec o;
    o.text = s; o.pismo = &p; o.px = px; o.proklad = proklad; o.radek = 1.18f;
    o.r = r; o.g = g; o.b = b;
    return Sazec::vysazej({o}, 100000.f, 0, stin);
  }
  void pripravTextyKarty(Slot &sl, const Karta &kar) {
    if (sl.textW == umW_ && sl.textVerze == verze_ && sl.nazev.tex) return;
    sl.nazev.uvolni(); sl.podtitul.uvolni(); sl.zamekNapis.uvolni(); sl.zamekDuvod.uvolni();
    const float W = (float)umW_;
    sl.nazev = text1(pBold_, kar.nazev, W * 0.118f, 0.015f, W * 0.86f, W * 0.012f);
    if (!kar.podtitul.empty()) sl.podtitul = text1(pSemi_, kar.podtitul, W * 0.040f, 0.16f, W * 0.86f, W * 0.008f);
    // zamcena karta: v karte jen zamek a ZAMCENO (proc - to je pod kartami, at neni textu moc)
    if (kar.zamceno) sl.zamekNapis = text1(pBold_, u8"ZAMČENO", W * 0.085f, 0.10f, W * 0.8f, W * 0.010f);
    sl.textW = umW_;
    sl.textVerze = verze_;
  }

  // obdelnik v bodech cile (y dolu) -> NDC
  static void ndc(float W, float H, float x0, float y0, float x1, float y1, float &a, float &b, float &c, float &d) {
    a = x0 / W * 2.f - 1.f; c = x1 / W * 2.f - 1.f;
    b = 1.f - y1 / H * 2.f; d = 1.f - y0 / H * 2.f;
  }

  // text (TextTex) na pozici: stred (x,y) v bodech cile; barva nahore/dole, lesk
  void textNa(float W, float H, const TextTex &t, float cx, float cy, const V3 &b1, const V3 &b2, float alfa, float lesk = -1.f, float meritko = 1.f) {
    if (!t.tex || alfa <= 0.002f) return;
    const float w = t.w * meritko, h = t.h * meritko;
    float a, b, c, d;
    ndc(W, H, cx - w * 0.5f, cy - h * 0.5f, cx + w * 0.5f, cy + h * 0.5f, a, b, c, d);
    pText_.pouzij();
    pText_.f3("uBarva1", b1);
    pText_.f3("uBarva2", b2);
    pText_.f1("uAlfa", alfa);
    pText_.f1("uLesk", lesk);
    pText_.tex("uTex", 0, t.tex);
    // TextTex je shora dolu -> v obracene
    k_.obd(pText_, a, b, c, d, 0.f, 1.f, 1.f, 0.f);
  }

  void kresliStitek(Slot &sl, const Karta &kar, const RamecUmeni &ram) {
    const float W = (float)umW_, H = (float)umH_;
    // tmavy prechod dole (nazev musi byt citelny pres jakykoli obrazek)
    pStitek_.pouzij();
    pStitek_.f3("uBarva", ram.barva);
    pStitek_.f1("uAlfa", 1.f);
    k_.obd(pStitek_, -1.f, -1.f, 1.f, -1.f + 2.f * 0.30f);
    (void)kar;
    // FBO je y nahoru: textNa pocita y dolu od horniho okraje -> prevest
    const V3 zl1(1.0f, 0.90f, 0.58f), zl2(0.86f, 0.55f, 0.16f);
    const float yNazev = H * (1.f - 0.150f), yPod = H * (1.f - 0.068f);
    textNaFbo(W, H, sl.nazev, W * 0.5f, yNazev, zl1, zl2, 1.f, -1.f);
    const V3 az = napdemo::lerp(V3(0.55f, 0.86f, 1.0f), ram.barva, 0.25f);
    textNaFbo(W, H, sl.podtitul, W * 0.5f, yPod, az, az * 0.85f, 1.f, -1.f);
  }

  // jako textNa, ale do FBO (y v bodech od horniho okraje karty)
  void textNaFbo(float W, float H, const TextTex &t, float cx, float cy, const V3 &b1, const V3 &b2, float alfa, float lesk) {
    if (!t.tex) return;
    const float w = (float)t.w, h = (float)t.h;
    // FBO: NDC y nahoru, obraz se pak na karte ukazuje se stejnou orientaci (v nahoru)
    const float x0 = cx - w * 0.5f, x1 = cx + w * 0.5f;
    const float yTop = cy - h * 0.5f, yBot = cy + h * 0.5f;     // od horniho okraje
    const float a = x0 / W * 2.f - 1.f, c = x1 / W * 2.f - 1.f;
    const float d = 1.f - yTop / H * 2.f, b = 1.f - yBot / H * 2.f;
    pText_.pouzij();
    pText_.f3("uBarva1", b1);
    pText_.f3("uBarva2", b2);
    pText_.f1("uAlfa", alfa);
    pText_.f1("uLesk", lesk);
    pText_.tex("uTex", 0, t.tex);
    k_.obd(pText_, a, b, c, d, 0.f, 1.f, 1.f, 0.f);
  }

  void kresliZamekNaKarte(Slot &sl, const RamecUmeni &ram) {
    const float W = (float)umW_, H = (float)umH_;
    (void)ram;
    // zamek uprostred obrazku
    const float r = W * 0.13f;
    const float cx = W * 0.5f, cy = H * 0.36f;
    uiFbo(W, H, 3, cx - r, cy - r, r * 2.f, r * 2.f, V3(1, 1, 1), 1.f);
    const V3 zl1(1.0f, 0.90f, 0.58f), zl2(0.86f, 0.55f, 0.16f);
    textNaFbo(W, H, sl.zamekNapis, cx, cy + r * 1.35f, zl1, zl2, 1.f, -1.f);
  }

  // UI prvek (FS_UI) do FBO karty, y od horniho okraje
  void uiFbo(float W, float H, int typ, float x, float y, float w, float h, const V3 &barva, float alfa) {
    const float a = x / W * 2.f - 1.f, c = (x + w) / W * 2.f - 1.f;
    const float d = 1.f - y / H * 2.f, b = 1.f - (y + h) / H * 2.f;
    pUi_.pouzij();
    pUi_.i1("uTyp", typ);
    pUi_.f2("uVel", w, h);
    pUi_.f3("uBarva", barva);
    pUi_.f1("uAlfa", alfa);
    pUi_.f1("uUhel", 0.f);
    pUi_.f1("uSvit", 0.f);
    k_.obd(pUi_, a, b, c, d);
  }

  // UI prvek na cil (bod x,y = levy horni roh, y dolu); typ 0 = plna barva (zatmeni)
  void ui(CilMenu &c, int typ, float x, float y, float w, float h, const V3 &barva, float alfa, float uhel = 0.f, float svit = 0.f) {
    float a, b, cc, d;
    ndc((float)c.w, (float)c.h, x, y, x + w, y + h, a, b, cc, d);
    pUi_.pouzij();
    pUi_.i1("uTyp", typ == 0 ? 8 : typ);       // 0 = plna barva (zatmeni) = typ 8 v shaderu
    pUi_.f2("uVel", w, h);
    pUi_.f3("uBarva", barva);
    pUi_.f1("uAlfa", alfa);
    pUi_.f1("uUhel", uhel);
    pUi_.f1("uSvit", svit);
    k_.obd(pUi_, a, b, cc, d);
  }

  // ---- karta na obrazovce (a jeji odraz) ----
  void kresliKartu(CilMenu &c, const Poza &p, int i, float alfa, float zablesk, float t) {
    if (alfa <= 0.002f) return;
    const Rozlozeni &r = c.roz;
    Slot &sl = sloty_[(size_t)i];
    if (!sl.platne || !sl.tex) return;
    const float hw = r.cw * p.s * 0.5f, hh = r.ch * p.s * 0.5f;
    const float okraj = 3.f;                       // body navic pro vyhlazeni hrany
    const float eu = 1.f + okraj / std::max(hw, 1.f), ev = 1.f + okraj / std::max(hh, 1.f);
    const float W = (float)c.w, H = (float)c.h;
    auto vrchol = [&](float u, float v, float tu, float tv, float *out) {
      float X, Y, w;
      promitni(r, p, u, v, X, Y, w);
      const float nx = X / W * 2.f - 1.f, ny = 1.f - Y / H * 2.f;
      out[0] = nx * w; out[1] = ny * w; out[2] = 0.f; out[3] = w;
      out[4] = tu; out[5] = tv;
    };
    // odraz na podlaze (na vysku): pod spodni hranou, v = 1 .. 1 + 2*odraz
    const float jas = p.jas;
    pKarta_.pouzij();
    pKarta_.tex("uTex", 0, sl.tex);
    pKarta_.f2("uVel", hw * 2.f, hh * 2.f);
    pKarta_.f1("uJas", jas);
    pKarta_.f3("uBarva", barvaV3(karty_[(size_t)i].barva));
    pKarta_.f1("uFokus", 1.f - std::min(1.f, std::fabs(p.d)));
    pKarta_.f1("uLesk", 0.35f - p.d * 0.6f + 0.04f * std::sin(t * 0.5f));
    pKarta_.f1("uZablesk", zablesk);
    pKarta_.f1("uCas", t);
    // vyhlazeni: ~1,2 bodu na blizsi strane, na vzdalenejsi vic (perspektiva)
    pKarta_.f1("uAA", 1.2f * std::max(p.rw[0], p.rw[1]) / std::min(p.rw[0], p.rw[1]));
    float v[4 * 6];
    if (r.odraz > 0.f) {
      const float L = r.odraz;                   // delka odrazu (dil vysky karty)
      // u: -eu..eu, v (dolu): 1 .. 1 + 2L ; textura: u, v = 0 .. -L
      vrchol(-eu, 1.f, 0.5f - 0.5f * eu, 0.f, v + 0);
      vrchol(eu, 1.f, 0.5f + 0.5f * eu, 0.f, v + 6);
      vrchol(-eu, 1.f + 2.f * L, 0.5f - 0.5f * eu, -L, v + 12);
      vrchol(eu, 1.f + 2.f * L, 0.5f + 0.5f * eu, -L, v + 18);
      pKarta_.f1("uOdraz", L);
      pKarta_.f1("uAlfa", alfa);
      kresliCtyruhelnik(v);
    }
    // karta: u -eu..eu, v -ev..ev; textura v nahoru (horni hrana v = 1)
    vrchol(-eu, -ev, 0.5f - 0.5f * eu, 0.5f + 0.5f * ev, v + 0);
    vrchol(eu, -ev, 0.5f + 0.5f * eu, 0.5f + 0.5f * ev, v + 6);
    vrchol(-eu, ev, 0.5f - 0.5f * eu, 0.5f - 0.5f * ev, v + 12);
    vrchol(eu, ev, 0.5f + 0.5f * eu, 0.5f - 0.5f * ev, v + 18);
    pKarta_.f1("uOdraz", 0.f);
    pKarta_.f1("uAlfa", alfa);
    kresliCtyruhelnik(v);
  }

  void kresliCtyruhelnik(const float *v) {
    glBindBuffer(GL_ARRAY_BUFFER, vboKarta_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 24, v, GL_STREAM_DRAW);
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(sizeof(float) * 4));
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(2);
  }

  // ---- pozadi ----
  void kresliPozadi(CilMenu &c, const StavMenu &s, const V3 &barva) {
    const Rozlozeni &r = c.roz;
    const int dw = std::max(32, c.w / 3), dh = std::max(32, c.h / 3);
    const bool novy = c.pozadi.priprav(dw, dh, false);
    const int kazdy = uroven_ >= 2 ? 2 : 3;
    if (novy || (c.snimku % kazdy) == 0) {
      glBindFramebuffer(GL_FRAMEBUFFER, c.pozadi.fbo);
      glViewport(0, 0, dw, dh);
      glDisable(GL_BLEND);
      pPozadi_.pouzij();
      pPozadi_.f2("uRozmer", (float)c.w, (float)c.h);
      pPozadi_.f1("uCas", (float)s.t);
      pPozadi_.f3("uBarva", barva);
      pPozadi_.f2("uStred", r.cx / c.w, 1.f - (r.cy + r.ch * 0.40f) / c.h);
      pPozadi_.f1("uPortal", (r.naSirku ? r.cw * 1.9f : r.cw * 0.9f) / c.w * ((float)c.w / c.h));
      pPozadi_.tex("uSum", 0, k_.texSum);
      k_.ctverec.kresli();
      glBindFramebuffer(GL_FRAMEBUFFER, c.fbo);
      glViewport(0, 0, c.w, c.h);
    }
    pKopie_.pouzij();
    pKopie_.f1("uJas", 1.f);
    pKopie_.tex("uTex", 0, c.pozadi.tex);
    k_.obd(pKopie_, -1, -1, 1, 1);
  }

  void pripravHvezdy(CilMenu &c) {
    if (c.hvezdW == c.w && c.hvezdH == c.h && c.vboHvezdy) return;
    c.hvezdW = c.w; c.hvezdH = c.h;
    c.hvezdy.clear();
    napdemo::Los los(1982);
    const float S = (float)std::min(c.w, c.h);
    const int pocet = (int)(c.w * (float)c.h / (S * S) * 70.f) + 40;
    for (int i = 0; i < pocet; i++) {
      const float x = los.r(-1.f, 1.f), y = los.r(-1.f, 1.f);
      const float vel = S * los.r(0.006f, 0.016f) * (los.f() < 0.12f ? 1.8f : 1.f);
      c.hvezdy.insert(c.hvezdy.end(), {x, y, vel, los.f()});
    }
    if (!c.vboHvezdy) glGenBuffers(1, &c.vboHvezdy);
    glBindBuffer(GL_ARRAY_BUFFER, c.vboHvezdy);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(c.hvezdy.size() * sizeof(float)), c.hvezdy.data(), GL_STATIC_DRAW);
  }

  void kresliHvezdy(CilMenu &c, float t, float alfa) {
    if (!c.vboHvezdy || c.hvezdy.empty()) return;
    glBlendFunc(GL_ONE, GL_ONE);
    pHvezdy_.pouzij();
    pHvezdy_.f1("uCas", t);
    pHvezdy_.f1("uAlfa", alfa * 0.85f);
    pHvezdy_.f3("uBarva", V3(1.0f, 0.88f, 0.70f));
    glBindBuffer(GL_ARRAY_BUFFER, c.vboHvezdy);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, nullptr);
    glDrawArrays(GL_POINTS, 0, (GLsizei)(c.hvezdy.size() / 4));
    glDisableVertexAttribArray(0);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  // ---- texty menu pro velikost cile ----
  void pripravTexty(CilMenu &c) {
    if (c.textW == c.w && c.textH == c.h && c.textVerze == verze_) return;
    c.uvolniTexty();
    const Rozlozeni &r = c.roz;
    c.titul = text1(pBold_, "N&P VISION", r.titulPx, 0.03f, c.w * 0.80f, r.titulPx * 0.10f);
    c.slogan = text1(pSemi_, "RETRO UNIVERSE", r.sloganPx, 0.55f, c.w * 0.80f, r.sloganPx * 0.25f);
    if (r.podpisY > 0.f) c.podpis = text1(pItal_, u8"René Frank", r.podpisPx, 0.02f, c.w * 0.6f, r.podpisPx * 0.15f);
    if (c.tv) c.napoveda = text1(pSemi_, u8"ŠIPKAMI LISTUJ  ·  ENTER OTEVŘE", r.popisPx * 0.85f, 0.10f, c.w * 0.9f, r.popisPx * 0.1f);
    for (const Karta &k : karty_) {
      Odstavec o;
      o.text = k.popis; o.pismo = &pMed_; o.px = r.popisPx; o.radek = 1.30f;
      TextTex t = Sazec::vysazej({o}, r.popisW, 0, r.popisPx * 0.18f);
      // na sirku jen jeden radek - kdyz se nevejde, zmensit
      const int maxRadku = r.naSirku ? 1 : 2;
      for (int pokus = 0; pokus < 6 && t.tex && t.obsahH > o.px * o.radek * maxRadku * 1.05f; pokus++) {
        t.uvolni();
        o.px *= 0.92f;
        t = Sazec::vysazej({o}, r.popisW, 0, r.popisPx * 0.18f);
      }
      c.popis.push_back(t);
      if (k.zamceno) {
        Odstavec z;
        z.text = std::string(u8"ZAMČENO · ") + (k.zamekText.empty() ? std::string(u8"zatím nejde otevřít") : k.zamekText);
        z.pismo = &pSemi_; z.px = r.popisPx; z.radek = 1.30f;
        TextTex tz = Sazec::vysazej({z}, r.popisW, 0, r.popisPx * 0.18f);
        for (int pokus = 0; pokus < 6 && tz.tex && tz.obsahH > z.px * z.radek * maxRadku * 1.05f; pokus++) {
          tz.uvolni();
          z.px *= 0.92f;
          tz = Sazec::vysazej({z}, r.popisW, 0, r.popisPx * 0.18f);
        }
        c.zamek.push_back(tz);
      } else {
        c.zamek.push_back(TextTex());
      }
      c.tlac.push_back(text1(pBold_, k.zamceno ? std::string(u8"ZAMČENO") : k.tlacitko, r.tlacPx, 0.12f, c.w * 0.62f, 0.f));
    }
    c.textW = c.w; c.textH = c.h; c.textVerze = verze_;
  }

  // ---- hlavicka: minca, N&P VISION, RETRO UNIVERSE, kolecko ----
  void kresliHlavicku(CilMenu &c, const StavMenu &s, float alfa) {
    if (alfa <= 0.002f) return;
    const Rozlozeni &r = c.roz;
    const float W = (float)c.w, H = (float)c.h, t = (float)s.t;
    // zare za minci
    glBlendFunc(GL_ONE, GL_ONE);
    ui(c, 6, r.logoX - r.logoR * 2.2f, r.logoY - r.logoR * 2.2f, r.logoR * 4.4f, r.logoR * 4.4f, V3(1.0f, 0.62f, 0.18f) * (0.40f * alfa), 1.f);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (k_.texLogo) {
      // minca v obrazku loga (516x520): stred 256,5 / 256,5, polomer ~215 bodu
      const float RX = 0.4165f, RY = 0.4135f, CX = 0.497f, CY = 0.493f;
      const float lw = r.logoR / RX, lh = r.logoR / RY;     // cely obrazek, aby minca mela polomer logoR
      const float x0 = r.logoX - lw * (CX), y0 = r.logoY - lh * (CY);
      float a, b, cc, d;
      ndc(W, H, x0, y0, x0 + lw, y0 + lh, a, b, cc, d);
      pLogo_.pouzij();
      pLogo_.f1("uAlfa", alfa);
      const float f = std::fmod(t, 6.0f) / 1.2f;
      pLogo_.f1("uLesk", -0.6f + 2.6f * std::min(1.f, f));
      pLogo_.f4("uKruh", CX, CY, RX, RY);
      pLogo_.f1("uAA", 1.5f / std::max(r.logoR, 1.f));
      pLogo_.tex("uTex", 0, k_.texLogo);
      k_.obd(pLogo_, a, b, cc, d, 0.f, 1.f, 1.f, 0.f);
    }
    const V3 zl1(1.0f, 0.92f, 0.62f), zl2(0.85f, 0.50f, 0.12f);
    const float leskT = std::fmod(t + 3.f, 6.0f) / 1.4f;
    if (r.naSirku) {
      // na sirku: napis vedle mince
      const float x = r.logoX + r.logoR * 1.35f;
      textNa(W, H, c.titul, x + c.titul.w * 0.5f, r.titulY, zl1, zl2, alfa, -0.5f + 2.f * std::min(1.f, leskT));
      textNa(W, H, c.slogan, x + c.slogan.w * 0.5f + r.titulPx * 0.04f, r.sloganY, V3(0.95f, 0.80f, 0.50f), V3(0.80f, 0.60f, 0.30f), alfa);
    } else {
      textNa(W, H, c.titul, r.cx, r.titulY, zl1, zl2, alfa, -0.5f + 2.f * std::min(1.f, leskT));
      textNa(W, H, c.slogan, r.cx, r.sloganY, V3(0.95f, 0.80f, 0.50f), V3(0.80f, 0.60f, 0.30f), alfa);
    }
    // kolecko OPTIONS (na TV neni co mackat)
    if (!c.tv) {
      float uhel = 0.f, svit = 0.f;
      if (s.tOzub >= 0) {
        const float k = (float)((s.t - s.tOzub) / 0.45);
        if (k >= 0.f && k < 1.f) { uhel = easeOut3(k) * 1.5708f; svit = 1.f - k; }
      }
      const float R = r.ozubR;
      ui(c, 1, r.ozubX - R, r.ozubY - R, R * 2.f, R * 2.f, V3(1, 1, 1), alfa, uhel, svit);
    }
  }

  // ---- popis, tlacitko, tecky, podpis ----
  void kresliSpodek(CilMenu &c, const StavMenu &s, float pozVid, const V3 &barva, float alfa) {
    if (alfa <= 0.002f) return;
    const Rozlozeni &r = c.roz;
    const float W = (float)c.w, H = (float)c.h, t = (float)s.t;
    const int n = (int)karty_.size();
    const float pz = std::min(std::max(pozVid, 0.f), (float)(n - 1));
    const int k = std::min(std::max((int)std::lround(pz), 0), n - 1);
    const float blizko = std::fabs(pz - (float)k);
    const float a = alfa * napdemo::smooth(0.40f, 0.08f, blizko);
    const Karta &kar = karty_[(size_t)k];
    // popis (nebo proc je zamcena)
    const bool zam = kar.zamceno && c.zamek[(size_t)k].tex;
    const TextTex &tp = zam ? c.zamek[(size_t)k] : c.popis[(size_t)k];
    if (tp.tex) {
      const float y = r.popisY - r.popisPx * 0.7f + tp.h * 0.5f - tp.okraj;
      const V3 bp = zam ? V3(1.0f, 0.80f, 0.42f) : V3(0.94f, 0.90f, 0.81f);
      textNa(W, H, tp, r.cx, y, bp, bp, a);
    }
    // tlacitko
    {
      const TextTex &tl = c.tlac[(size_t)k];
      const float ikona = r.tlacH * 0.42f;
      const float sirka = std::max(r.tlacW, (tl.tex ? tl.w : 0) + ikona * 2.2f + r.tlacH * 0.9f);
      float x0 = r.tlacX - sirka * 0.5f;
      const float y0 = r.tlacY - r.tlacH * 0.5f;
      // zamek: tlacitko zatrese a zablikne
      float trh = 0.f, zablik = 0.f;
      if (s.zamekKlep == k && s.tZamek >= 0) {
        const float q = (float)((s.t - s.tZamek) / 0.5);
        if (q >= 0.f && q < 1.f) { trh = std::sin(q * 3.14159f * 6.f) * (1.f - q) * r.tlacH * 0.18f; zablik = 1.f - q; }
      }
      x0 += trh;
      // zare pod tlacitkem (dycha)
      const float dech = 0.5f + 0.5f * std::sin(t * 2.4f);
      glBlendFunc(GL_ONE, GL_ONE);
      ui(c, 6, x0 - r.tlacH * 0.6f, y0 - r.tlacH * 0.7f, sirka + r.tlacH * 1.2f, r.tlacH * 2.4f,
         (kar.zamceno ? V3(0.5f, 0.25f, 0.05f) : napdemo::lerp(V3(1.0f, 0.65f, 0.2f), barva, 0.3f)) * ((0.16f + 0.10f * dech) * a + zablik * 0.4f), 1.f);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      const V3 bt = kar.zamceno ? V3(0.45f, 0.42f, 0.38f) : barva;
      ui(c, 2, x0, y0, sirka, r.tlacH, bt, alfa * (kar.zamceno ? 0.75f : 1.f), 0.f, t * 0.4f);
      // ikona (play / zamek) a text
      const float sirkaObsahu = (tl.tex ? tl.w - 2.f * tl.okraj : 0.f) + ikona * 1.6f;
      const float xi = x0 + sirka * 0.5f - sirkaObsahu * 0.5f + ikona * 0.5f;
      if (kar.zamceno) ui(c, 3, xi - ikona * 0.6f, r.tlacY - ikona * 0.6f, ikona * 1.2f, ikona * 1.2f, V3(1, 1, 1), alfa);
      else ui(c, 5, xi - ikona * 0.5f, r.tlacY - ikona * 0.5f, ikona, ikona, V3(0.16f, 0.08f, 0.02f), alfa);
      if (tl.tex) {
        const float xt = xi + ikona * 1.1f + (tl.w - 2.f * tl.okraj) * 0.5f;
        const V3 tb = V3(0.13f, 0.065f, 0.015f);
        textNa(W, H, tl, xt, r.tlacY + r.tlacPx * 0.04f, tb, tb, std::max(a, alfa * 0.35f));
      }
    }
    // tecky: kosoctverce, aktivni protahla tableta jede s karuselem
    if (n > 1) {
      const float krok = r.teckaKrok;
      const float x0 = r.cx - krok * (float)(n - 1) * 0.5f;
      for (int i = 0; i < n; i++) {
        const float x = x0 + krok * (float)i;
        const V3 b = karty_[(size_t)i].zamceno ? V3(0.35f, 0.30f, 0.25f) : V3(0.55f, 0.42f, 0.22f);
        ui(c, 4, x - r.teckaR, r.teckyY - r.teckaR, r.teckaR * 2.f, r.teckaR * 2.f, b, alfa);
      }
      const float xa = x0 + krok * pz;
      const float sw = r.teckaR * 3.6f, sh = r.teckaR * 2.0f;
      glBlendFunc(GL_ONE, GL_ONE);
      ui(c, 6, xa - sw, r.teckyY - sh * 1.6f, sw * 2.f, sh * 3.2f, barva * (0.5f * alfa), 1.f);
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      ui(c, 7, xa - sw * 0.5f, r.teckyY - sh * 0.5f, sw, sh, barva, alfa);
    }
    // podpis / napoveda pro TV
    if (c.podpis.tex && r.podpisY > 0.f)
      textNa(W, H, c.podpis, r.cx, r.podpisY, V3(1.0f, 0.86f, 0.52f), V3(0.78f, 0.52f, 0.18f), alfa * 0.85f);
    if (c.tv && c.napoveda.tex) textNa(W, H, c.napoveda, r.cx, r.napovedaY, V3(0.75f, 0.72f, 0.65f), V3(0.75f, 0.72f, 0.65f), alfa * 0.8f);
  }
};

}  // namespace napmenu
