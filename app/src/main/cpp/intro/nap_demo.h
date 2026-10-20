// nap_demo.h
// ====================================================================
//  B304: UVODNI DEMO N&P STUDIO - podle Reneho scenare "MASTER SCI-FI
//  PROMO MANIFEST" (45 s, 5 scen, titulky CZ + EN). B309: 59 s - Rene pridal
//  14 s pro klidnejsi ukazky her na CRT (defile 17,85 s misto 3,85 s).
//
//   0 -  7 s  tma, bzuceni CRT, sera garaz nabiha, vrata (opona) se zvedaji
//   7 - 17 s  ven do cernych valicich se mraku, hromy, ptaci, pak dest
//  17 - 27 s  obrovsky blesk - kamera s uctou najede na Atari 130XE na stole
//  27 - 51 s  B307: z klaves Atari vystreli neonove pruty, z nich vzniknou
//             konzole N&P 16 a N&P 32 (vlastni navrh s logem N&P), zaber na
//             trojici, kamera proleti k CRT monitoru a na nem defile 50 / s:
//             Reneho Pitt-Kitt Platinum (zabery ze skutecneho emulatoru Atari),
//             16bitova a 32bitova ukazka (viz nap_demo_cas.h)
//  51 - 59 s  tma, zlate logo N&P (C) 2026 VCS a bile filmove titulky
//             (B304-B308 to bylo 37 - 45 s - kresli se se stejnymi casy, posunute)
//
//  Vsechno se pocita v C++ a na GPU (OpenGL ES 2.0), v HD - zadne video
//  ani obrazky krome oficialniho loga a zaberu z Reneho hry (klip z C++
//  emulatoru Atari). Obraz je cista funkce casu, takze telefon i TV
//  kresli presne totez.
// ====================================================================
#pragma once
#include "nap_demo_gl.h"
#include "nap_demo_text.h"
#include "nap_demo_model.h"
#include "nap_demo_shadery.h"
#include "../atari/nap_atari_fonts.h"
#include "../atari/nap_atari_fonts2.h"
#include "../atari/nap_atari_roms.h"
#include "nap_demo_cas.h"
#include "nap_demo_konzole.h"
#include "nap_demo_16bit.h"
#include "nap_demo_32bit.h"
#include "nap_demo_klip.h"

namespace napdemo {

constexpr float DELKA = cas::KONEC;     // B309: 59 s (drive 45 s)

struct Titulek { float od, po; const char *cz, *en; };
static const Titulek TITULKY[8] = {
    {0.9f, 3.75f, u8"Vítejte v minulosti. Čas je jen iluze...", u8"Welcome to the past. Time is an illusion..."},
    {3.95f, 6.8f, u8"a N&P Studio se vám pokusí pootevřít dveře. Dál už je to jen na vás.",
     u8"and N&P Studio will try to crack the door open. The rest is up to you."},
    {8.0f, 12.25f, u8"Opravdu jen stačí na chvíli zpomalit. Minulost vás stvořila, přítomnost vás zkouší.",
     u8"Truly, you just need to slow down for a moment. The past built you, the present tests you."},
    {12.45f, 16.75f, u8"Zkuste vzít za kliku, nahlédnout do neznáma a věřte... budoucnost na vás čeká.",
     u8"Try to take the handle, peer into the unknown and believe... the future awaits you."},
    {18.7f, 22.45f, u8"Já nic nevymyslel. Já nic nenaprogramoval. Jsem jen snílek...",
     u8"I invented nothing. I programmed nothing. I am just a dreamer..."},
    {22.65f, 26.75f, u8"Vzal jsem kus zapomenuté NOSTALGIE, ve které nepřestal pulzovat život.",
     u8"I took a piece of forgotten NOSTALGIA where life never stopped pulsing."},
    {27.6f, 31.6f, u8"Bez poctivého snu by dnešní digitální vesmír vůbec nevznikl.",
     u8"Without a pure dream, today's digital universe wouldn't even exist."},
    {31.8f, 36.55f, u8"Jeden kód – A mnoho cest z minulosti do budoucnosti.", u8"One code – and many paths from the past to the future."},
};

enum {
  T_TIT = 0,                          // 0..7 titulky
  T_K1 = 8, T_K2, T_K3, T_K4, T_K5, T_K6,
  T_ATARI, T_PITT, T_16, T_32, T_SKIP,   // B307: popisky defile na CRT, tlacitko SKIP
  T_POCET
};

// Kam se kresli: displej telefonu nebo obraz pro TV (16:9)
struct Cil {
  int w = 0, h = 0;
  bool tv = false;
  Fbo nebe, garaz;
  Fbo scena;                          // cely snimek (s hloubkou) - pak jde pres "film"
  Fbo zare[4];                        // zare kolem svetel: 1/4 a 1/8 rozliseni
  TextTex texty[T_POCET];
  int textW = 0, textH = 0;
  float meritko = 0.5f;               // rozliseni mraku a garaze (meni se podle rychlosti GPU)
};

class Demo {
 public:
  // ---------------- priprava ----------------
  bool init(const unsigned char *logoRGBA, int logoW, int logoH) {
    pSemi.init(NAP_FONT_CHAKRA_SEMIBOLD_TTF);
    pBold.init(NAP_FONT_CHAKRA_BOLD_TTF);
    pMed.init(NAP_FONT_CHAKRA_MEDIUM_TTF);
    pItal.init(NAP_FONT_CHAKRA_BOLDITALIC_TTF);
    pMono.init(NAP_FONT_SPACEMONO_REGULAR_TTF);
    auto sum = [](const char *s) { std::string x(s); size_t i = x.find("%SUM%"); if (i != std::string::npos) x.replace(i, 5, SUM_GLSL); return x; };
    pPlny.sestav(VS_OBD, FS_TEX, "tex");
    pKopie.sestav(VS_PLNY, FS_KOPIE, "kopie");
    pBarva.sestav(VS_PLNY, FS_BARVA, "barva");
    pJas.sestav(VS_PLNY, FS_JAS, "jas");
    pRozmaz.sestav(VS_PLNY, FS_ROZMAZ, "rozmaz");
    pFinal.sestav(VS_PLNY, FS_FINAL, "film");
    pZare.sestav(VS_OBD, FS_ZARE, "zare");
    pGaraz.sestav(VS_PLNY, sum(FS_GARAZ).c_str(), "garaz");
    pNebe.sestav(VS_PLNY, sum(FS_NEBE).c_str(), "nebe");
    pDest.sestav(VS_PLNY, sum(FS_DEST).c_str(), "dest");
    pPtak.sestav(VS_OBD, FS_PTAK, "ptak");
    pPas.sestav(VS_PAS, FS_PAS, "pas");
    pModel.sestav(VS_MODEL, sum(FS_MODEL).c_str(), "model");
    pMistnost.sestav(VS_PLNY, FS_MISTNOST, "mistnost");
    pPara.sestav(VS_PLNY, sum(FS_PARA).c_str(), "para");
    pCastice.sestav(VS_CASTICE, FS_CASTICE, "castice");
    pLogo.sestav(VS_OBD, FS_LOGO, "logo");
    // B307: konzole N&P, CRT, tlacitko SKIP
    pKonzole.sestav(VS_MODEL, sum(FS_KONZOLE).c_str(), "konzole");
    pCrt.sestav(VS_CRT, FS_CRT, "crt");
    pTlacitko.sestav(VS_OBD, FS_TLACITKO, "tlacitko");
    ctverec.priprav();
    glGenBuffers(1, &vboDyn);
    // sumova textura: R nahoda, G = R posunute o (37,17) (3D sum), B dalsi nahoda
    {
      std::vector<unsigned char> n(256 * 256 * 4);
      Los los(20261120);
      for (int i = 0; i < 256 * 256; i++) { n[i * 4] = (unsigned char)(los.dalsi() >> 24); n[i * 4 + 2] = (unsigned char)(los.dalsi() >> 24); n[i * 4 + 3] = 255; }
      for (int y = 0; y < 256; y++)
        for (int x = 0; x < 256; x++) n[(y * 256 + x) * 4 + 1] = n[(((y + 17) & 255) * 256 + ((x + 37) & 255)) * 4];
      texSum = novaTextura(256, 256, GL_RGBA, n.data(), true, true);
    }
    atari.postav(pBold, pItal, pMed);
    k16.postav16(&pBold, &pSemi, &pItal);       // B313: s potisky (stitek cartridge, panel, disk)
    k32.postav32(&pBold, &pSemi, &pItal);
    crt.postav();
    postavPruty();
    // ukazky na CRT: 16 bit (procesor), 32 bit (GPU), Atari (klip)
    hra16.init();
    pismoTucne(pismo8);
    hra32ok = hra32.init(&pismo8[0][0]);
    tex16 = novaTextura(Hra16::W, Hra16::H, GL_RGBA, nullptr, false);
    tex8 = novaTextura(384, 240, GL_RGBA, nullptr, false);
    {   // stul a stena (velke plochy)
      Stavitel s;
      s.quad(V3(-160, 0, 130), V3(160, 0, 130), V3(160, 0, -95), V3(-160, 0, -95), 8);
      s.quad(V3(-170, 0, -95), V3(170, 0, -95), V3(170, 140, -95), V3(-170, 140, -95), 11);
      okoli.nahraj(s.v);
    }
    if (logoRGBA && logoW > 0 && logoH > 0) {
      // OpenGL ES 2.0 umi mipmapy jen u textur 2^n - logo (516x520) se
      // prevzorkuje na 512x512 (pomer stran se kresli podle originalu)
      std::vector<unsigned char> pot;
      const unsigned char *data = logoRGBA;
      int tw = logoW, th = logoH;
      if ((tw & (tw - 1)) || (th & (th - 1))) {
        tw = th = 512;
        pot.resize((size_t)tw * th * 4);
        for (int y = 0; y < th; y++)
          for (int x = 0; x < tw; x++) {
            const float fx = (x + 0.5f) * logoW / tw - 0.5f, fy = (y + 0.5f) * logoH / th - 0.5f;
            const int x0 = std::max(0, (int)std::floor(fx)), y0 = std::max(0, (int)std::floor(fy));
            const int x1 = std::min(logoW - 1, x0 + 1), y1 = std::min(logoH - 1, y0 + 1);
            const float ax = clampf(fx - x0, 0.f, 1.f), ay = clampf(fy - y0, 0.f, 1.f);
            for (int k = 0; k < 4; k++) {
              const float a = logoRGBA[((size_t)y0 * logoW + x0) * 4 + k], b = logoRGBA[((size_t)y0 * logoW + x1) * 4 + k];
              const float c2 = logoRGBA[((size_t)y1 * logoW + x0) * 4 + k], d = logoRGBA[((size_t)y1 * logoW + x1) * 4 + k];
              pot[((size_t)y * tw + x) * 4 + k] = (unsigned char)(lerpf(lerpf(a, b, ax), lerpf(c2, d, ax), ay) + 0.5f);
            }
          }
        data = pot.data();
      }
      texLogo = novaTextura(tw, th, GL_RGBA, data, true);
      glBindTexture(GL_TEXTURE_2D, texLogo);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
      glGenerateMipmap(GL_TEXTURE_2D);
      logoPomer = (float)logoW / logoH;
    }
    postavBlesk();
    pripraveno = pSemi.ok && pModel.ok && pGaraz.ok && pNebe.ok && pFinal.ok && pKonzole.ok && pCrt.ok;
    NAPDEMO_LOG("DEMO pripraveno: %s (pisma %d, model %d tri, konzole %d+%d tri, CRT %d tri, 32bit %s)", pripraveno ? "ANO" : "NE",
                (int)pSemi.ok, atari.sit.pocet / 3, k16.trojuhelniku(), k32.trojuhelniku(), (crt.sit.pocet + crt.sklo.pocet) / 3,
                hra32ok ? "ANO" : "NE");
    return pripraveno;
  }

  // Vysadi chybejici texty (nejvys "kusu" naraz). Vraci, kolik jich chybi.
  int pripravTexty(Cil &c, int kusu) {
    if (c.textW != c.w || c.textH != c.h) {
      for (auto &t : c.texty) t.uvolni();
      c.textW = c.w; c.textH = c.h;
    }
    int chybi = 0;
    for (int i = 0; i < T_POCET; i++) {
      if (c.texty[i].tex) continue;
      if (kusu <= 0) { chybi++; continue; }
      c.texty[i] = vysadText(c, i);
      kusu--;
    }
    return chybi;
  }

  // ---------------- jeden snimek ----------------
  void snimek(double tt, Cil &c, GLuint cilFbo) {
    if (!pripraveno || c.w <= 0 || c.h <= 0) return;
    const float t = (float)tt;
    pripravTexty(c, t < 1.0f ? 4 : 2);
    c.scena.priprav(c.w, c.h, true);
    const GLuint sc = c.scena.fbo;
    glBindFramebuffer(GL_FRAMEBUFFER, sc);
    glViewport(0, 0, c.w, c.h);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_BLEND);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    float zareSila = 1.0f;
    if (t < 8.7f) { scena1(t, c, sc); zareSila = 0.85f; }
    else if (t < 17.28f) { scena2(t, c, sc); zareSila = 1.1f; }
    else if (t < 27.0f) { scena3(t, c, sc); zareSila = 0.75f; }
    else if (t < cas::SCENA5) { scena4(t, c, sc); zareSila = 1.15f; }
    else { scena5(t - cas::POSUN5, c); zareSila = 0.9f; }    // B309: scena 5 se stejnymi casy jako drive

    // film: zare kolem svetel, vinetace, zrno -> cilovy framebuffer
    film(c, t, cilFbo, zareSila, t < cas::SCENA5 ? 1.0f : 0.5f, t < cas::SCENA5 ? 1.0f : 0.6f);

    // titulky
    for (int i = 0; i < 8; i++) {
      const Titulek &ti = TITULKY[i];
      float a = okno(t, ti.od, ti.od + 0.4f, ti.po - 0.4f, ti.po);
      if (a > 0.001f) titulek(c, c.texty[T_TIT + i], a);
    }
    // zablesk po velkem blesku (prekryje vsechno)
    float bl = zablesk(t);
    if (bl > 0.001f) prekryv(1.0f, 1.0f, 1.0f, bl);
    // B307: tlacitko SKIP (jen telefon - na TV / PC neni co mackat)
    if (!c.tv) tlacitkoSkip(c, okno(t, 1.2f, 1.9f, cas::KONEC - 1.0f, cas::KONEC - 0.4f));
    glDisable(GL_BLEND);
  }

  // B307: zabery z Reneho hry (klip z emulatoru Atari, asset intro/pittkitt.klip)
  bool nastavKlip(const unsigned char *d, size_t n) { return klip.nacti(d, n); }
  bool maKlip() const { return klip.je(); }

  // B307: kde je tlacitko SKIP (body od leveho horniho rohu) - stejne pro kresleni
  // i pro dotyk (Java posle dotyk do C++ a C++ rozhodne). Vpravo nahore.
  static void skipObd(int w, int h, float &x0, float &y0, float &x1, float &y1) {
    const float S = (float)std::min(w, h);
    const float bw = S * 0.235f, bh = S * 0.082f, m = S * 0.035f;
    x1 = (float)w - m; x0 = x1 - bw;
    y0 = m + (w < h ? S * 0.03f : 0.f); y1 = y0 + bh;
  }
  // je dotyk na tlacitku? (s rezervou pro prst)
  static bool dotykNaSkip(int w, int h, float x, float y) {
    float x0, y0, x1, y1; skipObd(w, h, x0, y0, x1, y1);
    const float r = (float)std::min(w, h) * 0.03f;
    return x >= x0 - r && x <= x1 + r && y >= y0 - r && y <= y1 + r;
  }

  bool pripraveno = false;

 private:
  Pismo pSemi, pBold, pMed, pItal, pMono;
  Prog pPlny, pKopie, pBarva, pJas, pRozmaz, pFinal, pZare, pGaraz, pNebe, pDest, pPtak, pPas, pModel, pMistnost, pCastice, pLogo;
  Prog pPara, pKonzole, pCrt, pTlacitko;
  Ctverec ctverec;
  GLuint vboDyn = 0;
  GLuint texSum = 0, texLogo = 0;
  float logoPomer = 1.f;
  ModelAtari atari;
  Sit okoli;
  std::vector<float> dyn;
  // B307: trojice N&P, CRT a defile
  ModelKonzole k16, k32;
  ModelCrt crt;
  Hra16 hra16;
  Hra32 hra32;
  bool hra32ok = false;
  KlipAtari klip;
  GLuint tex16 = 0, tex8 = 0;
  int pos16 = -1, posAtari = -1;
  float pos32 = -1.f;
  std::vector<uint32_t> buf8;
  uint8_t pismo8[96][8];
  float klavZar = 0.f;              // klavesy Atari sviti (scena 4)
  V3 energiePos{0, 0, 0}, energieBarva{-1, -1, -1};   // svetlo prutu (x < 0 = postaru)

  // ================= SCENA 1: garaz =================
  // na vysku stoji kamera kousek vlevo, at je videt stara televize na ponku
  void kameraGaraz(float t, const Cil &c, V3 &kam, V3 &cil) {
    const float a = (float)c.w / c.h;
    const float bok = -0.55f * (1.f - smooth(0.8f, 1.5f, a));
    float z;
    if (t < 7.0f) z = -1.6f + 3.0f * easeInOut(t / 7.0f);
    else z = 1.4f + 6.2f * std::pow(smooth(7.0f, 8.7f, t), 1.6f);
    const float sw = 0.02f * std::sin(t * 0.9f) * (1.f - smooth(6.5f, 8.f, t));
    const float x = bok * (1.f - smooth(4.8f, 7.6f, t)) + sw;
    kam = V3(x, 1.50f - 0.12f * smooth(5.f, 8.f, t), z);
    cil = V3(x * 0.55f + sw, 1.02f + 0.62f * smooth(6.6f, 8.7f, t), 5.6f + std::max(0.f, z - 1.0f));
  }
  float ohniskoGaraz(const Cil &c) {
    const float a = (float)c.w / c.h;
    return lerpf(1.22f, 1.95f, smooth(0.5f, 1.8f, a));
  }
  V3 zarovkaPoloha(float t) { return V3(0.035f * std::sin(t * 1.25f), 2.08f, 2.2f + 0.02f * std::cos(t * 1.25f)); }
  float televize(float t) {
    float f = smooth(0.25f, 1.0f, t);
    int k = (int)(t * 17.f);
    uint32_t h = (uint32_t)k * 2246822519u;
    if (((h >> 9) & 15) == 5) f *= 0.6f;
    return f;
  }
  float dvere(float t) { return easeInOut(smooth(4.4f, 7.2f, t)); }
  float zarovka(float t) {
    if (t < 1.15f) return 0.f;
    if (t < 1.9f) {          // nabiha jako stara zarivka - par zablikani
      int k = (int)((t - 1.15f) * 20.f);
      static const int vz[15] = {1, 0, 0, 1, 1, 0, 1, 0, 0, 0, 1, 1, 1, 0, 1};
      return vz[k % 15] ? 0.85f : 0.05f;
    }
    float f = 0.92f;
    int k = (int)(t * 23.f);
    uint32_t hsh = (uint32_t)k * 2654435761u;
    if (((hsh >> 7) & 31) == 3) f = 0.45f;
    return f;
  }

  void scena1(float t, Cil &c, GLuint cilFbo) {
    pripravAtmo(c);
    const float dv = dvere(t);
    kresliNebe(c, c.nebe, t, 0.03f + 0.1f * smooth(7.5f, 8.7f, t));
    V3 kam, cil; kameraGaraz(t, c, kam, cil);
    glBindFramebuffer(GL_FRAMEBUFFER, c.garaz.fbo);
    glViewport(0, 0, c.garaz.w, c.garaz.h);
    pGaraz.pouzij();
    pGaraz.f2("uRozmer", (float)c.garaz.w, (float)c.garaz.h);
    pGaraz.f1("uCas", t);
    pGaraz.f3("uKam", kam);
    pGaraz.f3("uCil", cil);
    pGaraz.f1("uOhnisko", ohniskoGaraz(c));
    pGaraz.f1("uDvere", dv);
    pGaraz.f1("uExpo", smooth(1.0f, 4.6f, t) * (1.f + 0.30f * smooth(7.f, 8.5f, t)));
    pGaraz.f1("uZarovka", zarovka(t));
    pGaraz.f3("uZar", zarovkaPoloha(t));
    pGaraz.f1("uTv", televize(t));
    pGaraz.tex("uNebe", 0, c.nebe.tex);
    pGaraz.tex("uSum", 1, texSum);
    ctverec.kresli();
    glActiveTexture(GL_TEXTURE0);
    glBindFramebuffer(GL_FRAMEBUFFER, cilFbo);
    glViewport(0, 0, c.w, c.h);
    kopie(c.garaz.tex, 1.f);
    // prach v paprscich svetla
    if (t > 1.2f) prachGaraz(c, t, kam, cil);
  }

  void prachGaraz(Cil &c, float t, const V3 &kam, const V3 &cil) {
    const float a = (float)c.w / c.h, f = ohniskoGaraz(c);
    V3 fw = norm(cil - kam), rt = norm(cross(V3(0, 1, 0), fw)), up = cross(fw, rt);
    dyn.clear();
    Los los(777);
    const float dv = dvere(t);
    const float S = (float)std::min(c.w, c.h);
    const V3 zp = zarovkaPoloha(t);
    for (int i = 0; i < 190; i++) {
      V3 p(los.r(-1.5f, 1.5f), los.r(0.15f, 2.45f), los.r(0.3f, 5.5f));
      float ph = los.r(0, 6.28f), sp = los.r(0.05f, 0.18f);
      p.x += std::sin(t * sp * 2.f + ph) * 0.20f;
      p.y += std::sin(t * sp * 1.3f + ph * 2.f) * 0.15f - t * 0.010f;
      p.z += std::cos(t * sp + ph) * 0.18f;
      V3 d = p - kam;
      float zc = dot(d, fw);
      if (zc < 0.3f) continue;
      float nx = dot(d, rt) / zc * f / a, ny = dot(d, up) / zc * f;
      if (std::fabs(nx) > 1.1f || std::fabs(ny) > 1.1f) continue;
      // svetlo: paprsky z vrat + blizko zarovky
      float vr = dv * smooth(1.45f, 1.0f, std::fabs(p.x)) * smooth(dv * 2.12f + 0.3f, dv * 2.12f - 0.6f, p.y + (5.6f - p.z) * 0.12f) * smooth(2.0f, 4.8f, p.z);
      V3 zb = zp - p;
      float zr = zarovka(t) * 0.55f / (1.f + dot(zb, zb) * 1.4f);
      float jas = (vr * 0.9f + zr) * smooth(1.2f, 2.5f, t);
      if (jas < 0.02f) continue;
      float vel = S * 0.0042f * (1.6f / zc) * los.r(0.6f, 1.4f) + 1.f;
      dyn.insert(dyn.end(), {nx, ny, std::min(vel, 22.f), std::min(jas, 1.f) * 0.8f});
    }
    castice(c, V3(1.0f, 0.93f, 0.82f));
  }

  // ================= SCENA 2: mraky =================
  void kresliNebe(Cil &c, Fbo &f, float t, float pohled) {
    glBindFramebuffer(GL_FRAMEBUFFER, f.fbo);
    glViewport(0, 0, f.w, f.h);
    pNebe.pouzij();
    pNebe.f2("uRozmer", (float)f.w, (float)f.h);
    pNebe.f1("uCas", t);
    pNebe.f1("uTma", smooth(9.0f, 16.8f, t));
    pNebe.f1("uPohled", pohled);
    pNebe.f1("uVitr", 0.2f + 0.8f * smooth(10.f, 17.f, t));
    pNebe.f1("uSvit", 1.0f - 0.35f * smooth(10.f, 16.5f, t));
    float bx, by, bs, br; bleskVMracich(t, (float)f.w / f.h, bx, by, bs, br);
    pNebe.f4("uBlesk", bx, by, bs, br);
    pNebe.f1("uExpo", 1.0f);
    pNebe.tex("uSum", 0, texSum);
    ctverec.kresli();
    (void)c;
  }
  // zablesky v mracich: x v jednotkach vysky obrazu (0 = stred), y v NDC;
  // pro shader se prepocte na uv podle pomeru stran
  void bleskVMracich(float t, float a, float &x, float &y, float &s, float &r) {
    struct B { float t, x, y, s, r; };
    static const B bb[] = {{9.55f, -0.15f, 0.55f, 0.75f, 0.28f}, {11.25f, 0.25f, 0.45f, 0.55f, 0.26f},
                           {12.55f, 0.22f, 0.62f, 1.10f, 0.30f}, {14.15f, -0.30f, 0.55f, 0.85f, 0.32f},
                           {14.95f, -0.20f, 0.70f, 1.35f, 0.36f}, {15.95f, 0.15f, 0.60f, 1.20f, 0.40f},
                           {16.94f, 0.05f, 0.90f, 1.70f, 0.60f}};
    x = 0.5f; y = 0.8f; s = 0.f; r = 0.3f;
    for (const B &b : bb) {
      float d = t - b.t;
      if (d < 0.f || d > 0.6f) continue;
      float e = std::exp(-d * 9.f) * (0.65f + 0.35f * std::sin(d * 95.f)) + (d > 0.12f && d < 0.2f ? 0.5f : 0.f);
      if (e * b.s > s) { s = e * b.s; x = 0.5f + b.x / (2.f * a); y = 0.5f + b.y * 0.5f; r = b.r; }
    }
  }
  float pohledNebe(float t) { return 0.13f + 0.12f * smooth(8.6f, 12.f, t) - 0.08f * smooth(14.f, 17.f, t); }

  void scena2(float t, Cil &c, GLuint cilFbo) {
    pripravAtmo(c);
    kresliNebe(c, c.nebe, t, pohledNebe(t));
    glBindFramebuffer(GL_FRAMEBUFFER, cilFbo);
    glViewport(0, 0, c.w, c.h);
    kopie(c.nebe.tex, 1.f);
    ptaci(c, t);
    // vzdalene blesky do zeme (male, nad kopci)
    maleBlesky(c, t);
    float d = smooth(13.4f, 16.0f, t);
    if (d > 0.01f) {
      glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      pDest.pouzij();
      pDest.f2("uRozmer", (float)c.w, (float)c.h);
      pDest.f1("uCas", t);
      pDest.f1("uSila", d);
      pDest.tex("uSum", 0, texSum);
      ctverec.kresli();
      glDisable(GL_BLEND);
    }
    if (t > 16.9f) kresliBlesk(c, t);
  }

  void ptaci(Cil &c, float t) {
    float vid = okno(t, 8.6f, 9.4f, 12.6f, 13.8f);
    if (vid < 0.01f) return;
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    pPtak.pouzij();
    Los los(4242);
    const float a = (float)c.w / c.h;
    for (int i = 0; i < 11; i++) {
      float ox = los.r(-0.15f, 0.25f), oy = los.r(-0.06f, 0.07f), v = los.r(0.85f, 1.15f), ph = los.r(0, 6.28f);
      // x v jednotkach vysky obrazu (pravy okraj = a), letí zprava doleva
      float x = a + 0.08f - (t - 8.6f) * 0.17f * v + ox * 0.8f;
      float y = 0.30f + oy + 0.012f * std::sin(t * 1.3f + ph);
      float sz = 0.016f * los.r(0.7f, 1.2f);
      float mav = t * 11.f * v + ph;
      float x0 = (x - sz * 1.6f) / a, x1 = (x + sz * 1.6f) / a;
      float y0 = y - sz, y1 = y + sz;
      if (x1 < -1.f || x0 > 1.f) continue;
      pPtak.f1("uMav", mav);
      pPtak.f1("uAlfa", vid * 0.9f);
      pPtak.f4("uObd", x0, y0, x1, y1);
      pPtak.f4("uUV", 0, 0, 1, 1);
      ctverec.kresli();
    }
    glDisable(GL_BLEND);
  }

  // ================= BLESKY =================
  // body v souradnicich: x v jednotkach vysky obrazu (0 = stred), y = NDC
  struct BodBlesku { float x, y, sirka, sila; };
  typedef std::vector<std::vector<BodBlesku>> Blesk;
  Blesk blesk, malyBlesk[2];

  // Kanal blesku: nahodna chuze shora dolu - kratke useky s ostrymi zlomy
  // a mirnym tahem k cili, pak jemne rozkmitani (jako skutecny blesk).
  static void kanal(Los &los, V2 a, V2 b, float krok, std::vector<V2> &ven) {
    std::vector<V2> body;
    body.push_back(a);
    V2 p = a;
    for (int g = 0; g < 500 && p.y > b.y; g++) {
      const float zbyva = p.y - b.y;
      const float dy = std::min(zbyva, krok * los.r(0.35f, 1.5f));
      const float tah = (b.x - p.x) / std::max(zbyva, 0.04f);
      float dx = dy * (los.r(-1.0f, 1.0f) * 0.85f + tah * 0.8f);
      if (los.f() < 0.10f) dx *= 2.6f;                       // obcas ostry zlom
      p.x += dx; p.y -= dy;
      body.push_back(p);
    }
    for (int k = 0; k < 2; k++) {                             // jemne rozkmitani
      std::vector<V2> nb;
      for (size_t i = 0; i + 1 < body.size(); i++) {
        V2 u = body[i], w = body[i + 1];
        float ex = w.x - u.x, ey = w.y - u.y, l = std::sqrt(ex * ex + ey * ey);
        float o = los.r(-1.f, 1.f) * 0.18f * l;
        V2 m = {(u.x + w.x) * 0.5f - ey / std::max(l, 1e-6f) * o, (u.y + w.y) * 0.5f + ex / std::max(l, 1e-6f) * o};
        nb.push_back(u); nb.push_back(m);
      }
      nb.push_back(body.back());
      body.swap(nb);
    }
    ven = body;
  }
  static Blesk postavJedenBlesk(uint32_t seed, V2 od, V2 kam, float sirka, int vetvi, float krok) {
    Los los(seed);
    Blesk b;
    std::vector<V2> hl;
    kanal(los, od, kam, krok, hl);
    std::vector<BodBlesku> v;
    for (size_t i = 0; i < hl.size(); i++) {
      float s = (float)i / (hl.size() - 1);
      v.push_back({hl[i].x, hl[i].y, sirka * (1.f - 0.30f * s), 1.f});
    }
    b.push_back(v);
    for (int k = 0; k < vetvi; k++) {
      size_t z = (size_t)(los.r(0.05f, 0.72f) * (hl.size() - 1));
      V2 p = hl[z];
      const float strana = los.f() < 0.5f ? -1.f : 1.f;
      const float delka = los.r(0.10f, 0.32f) * (od.y - kam.y);
      V2 cil = {p.x + strana * delka * los.r(0.5f, 1.1f), p.y - delka};
      std::vector<V2> vb;
      kanal(los, p, cil, krok * 0.6f, vb);
      std::vector<BodBlesku> w;
      for (size_t i = 0; i < vb.size(); i++) {
        float s = (float)i / (vb.size() - 1);
        w.push_back({vb[i].x, vb[i].y, sirka * 0.55f * (1.f - s * 0.7f), 0.85f * (1.f - s * 0.75f)});
      }
      b.push_back(w);
    }
    return b;
  }
  void postavBlesk() {
    // konce nad kopci (obzor je v case blesku na y = -2 * pohledNebe)
    blesk = postavJedenBlesk(170, {0.05f, 1.12f}, {-0.04f, -0.27f}, 0.0034f, 8, 0.045f);
    malyBlesk[0] = postavJedenBlesk(333, {0.22f, 0.62f}, {0.27f, -0.43f}, 0.0018f, 3, 0.03f);
    malyBlesk[1] = postavJedenBlesk(911, {-0.20f, 0.70f}, {-0.15f, -0.39f}, 0.0020f, 4, 0.03f);
  }
  void kresliJedenBlesk(const Cil &c, const Blesk &bb, float postup, float dx, float dy) {
    const float a = (float)c.w / c.h;
    for (size_t b = 0; b < bb.size(); b++) {
      const auto &v = bb[b];
      std::vector<V2> pts; std::vector<float> wv, sv;
      size_t n = (size_t)(v.size() * (b == 0 ? postup : smooth(0.4f, 1.0f, postup)));
      for (size_t i = 0; i < n; i++) {
        pts.push_back({v[i].x / a + dx, v[i].y + dy});
        wv.push_back(std::max(v[i].sirka * c.h * 0.5f, 0.8f));
        sv.push_back(v[i].sila);
      }
      if (pts.size() < 2) continue;
      pruh(c, pts, wv, sv, 3.2f, 1.0f, 0.f, 2.5f);        // barevna zare tesne kolem
      pruh(c, pts, wv, sv, 1.3f, 2.2f, 0.f, 0.f);         // ostre bile jadro (zbytek zare udela "film")
    }
  }
  void maleBlesky(Cil &c, float t) {
    static const float KDY[2] = {12.55f, 14.95f};
    for (int i = 0; i < 2; i++) {
      float d = t - KDY[i];
      if (d < 0.f || d > 0.32f) continue;
      float vid = d < 0.05f ? 1.f : (d < 0.09f ? 0.25f : (d < 0.16f ? 0.9f : std::max(0.f, 1.f - (d - 0.16f) * 6.f)));
      dyn.clear();
      kresliJedenBlesk(c, malyBlesk[i], smooth(0.f, 0.05f, d), 0.f, 0.f);
      pasyKresli(V3(1.0f, 1.0f, 1.0f), V3(0.50f, 0.56f, 1.0f), V3(0.9f, 0.7f, 0.3f), vid * 0.8f);
    }
  }

  // vic jasu nez obrazovka umi -> zablesk pres cely obraz
  float zablesk(float t) {
    if (t < 17.0f || t > 18.6f) {
      float s = 0.f;
      // B307: drobne zablesky - pruty dorazi ke konzolim, strihy na CRT
      static const float KDY[3] = {cas::PRILET + 0.08f, cas::STRIH_16, cas::STRIH_32};
      static const float SILA[3] = {0.30f, 0.22f, 0.22f};
      for (int i = 0; i < 3; i++) {
        float d = t - KDY[i];
        if (d >= 0.f && d < 0.16f) s = std::max(s, SILA[i] * (1.f - d / 0.16f));
      }
      return s;
    }
    if (t < 17.10f) return 0.30f * smooth(17.0f, 17.10f, t);
    if (t < 17.16f) return 0.30f + 0.66f * smooth(17.10f, 17.16f, t);
    if (t < 17.34f) return 0.94f + 0.06f * std::sin(t * 120.f);
    return std::exp(-(t - 17.34f) * 4.0f);
  }
  float bleskViditelnost(float t) {
    float d = t - 16.94f;
    if (d < 0.f) return 0.f;
    if (d < 0.07f) return 1.f;
    if (d < 0.10f) return 0.35f;
    if (d < 0.18f) return 1.f;
    if (d < 0.22f) return 0.2f;
    if (d < 0.34f) return 0.95f;
    return std::max(0.f, 1.f - (d - 0.34f) * 4.f);
  }
  void kresliBlesk(Cil &c, float t) {
    float vid = bleskViditelnost(t);
    if (vid <= 0.01f) return;
    dyn.clear();
    kresliJedenBlesk(c, blesk, smooth(16.94f, 16.99f, t), 0.f, 0.f);
    pasyKresli(V3(1.0f, 1.0f, 1.0f), V3(0.55f, 0.6f, 1.0f), V3(0.9f, 0.7f, 0.3f), vid);
  }

  // ================= SCENA 3: Atari 130XE na stole =================
  struct Kamera { V3 oko, cil; float fov; };
  // na sirku miri kamera o kus niz, aby pocitac nezakryvaly titulky dole
  Kamera kameraAtari(float t, float a) {
    Kamera k;
    const float sir = smooth(0.9f, 1.5f, a);
    V3 A(0, 78, 150), Ac(0, 4 - 2.f * sir, 0);
    V3 B(17, 27, 50), Bc(1, 5 - 3.5f * sir, 0);
    V3 D(0, 64, 112), Dc(0, 22 - 7.f * sir, -8);
    V3 E(0, 40, 70), Ec(0, 12 - 5.f * sir, -4);
    if (t < 19.3f) { float s = easeOut(smooth(17.28f, 19.3f, t)); k.oko = lerp(A, B, s); k.cil = lerp(Ac, Bc, s); }
    else if (t < 27.0f) {
      // pomaly oblouk zprava doleva a dolu az k napisu ATARI a kontrolce
      float s = easeInOut(smooth(19.3f, 27.0f, t));
      const float uB = std::atan2(B.x, B.z), uC = std::atan2(-15.f, 29.f);
      const float rB = std::sqrt(B.x * B.x + B.z * B.z), rC = std::sqrt(15.f * 15.f + 29.f * 29.f);
      const float u = lerpf(uB, uC, s), r = lerpf(rB, rC, s);
      k.oko = V3(std::sin(u) * r, lerpf(B.y, 14.5f, s), std::cos(u) * r);
      k.cil = lerp(Bc, V3(-4.5f, 4.5f - 3.5f * sir, 0.5f), s);
    }
    else if (t < 30.5f) {
      float s = easeInOut(smooth(27.0f, 28.8f, t));
      const float uC = std::atan2(-15.f, 29.f), rC = std::sqrt(15.f * 15.f + 29.f * 29.f);
      const V3 C2(std::sin(uC) * rC, 14.5f, std::cos(uC) * rC), Cc2(-4.5f, 4.5f - 3.5f * sir, 0.5f);
      k.oko = lerp(C2, D, s); k.cil = lerp(Cc2, Dc, s);
    }
    else { float s = easeInOut(smooth(35.4f, 37.0f, t)); k.oko = lerp(D, E, s); k.cil = lerp(Dc, Ec, s); }
    k.fov = 0.80f;
    return k;
  }
  float bleskOkno(float t) {
    struct B { float t, s; };
    static const B bb[] = {{19.95f, 0.55f}, {22.75f, 1.0f}, {25.35f, 0.45f}};
    float s = 0.f;
    for (const B &b : bb) {
      float d = t - b.t;
      if (d < 0.f || d > 0.5f) continue;
      s = std::max(s, b.s * std::exp(-d * 10.f) * (0.7f + 0.3f * std::sin(d * 80.f)));
    }
    return s;
  }
  float ledSvetlo(float t) { return smooth(25.6f, 25.95f, t) * (0.85f + 0.15f * std::sin(t * 3.0f)); }

  void kresliModel(Cil &c, float t, const Kamera &k, float bl, float energie) {
    const float a = (float)c.w / c.h;
    // na vysku sirsi zorny uhel, at se Atari vejde
    float fov = a < 1.f ? 2.f * std::atan(std::tan(k.fov * 0.5f) / std::max(a, 0.45f) * 0.62f) : k.fov;
    M4 proj = M4::perspektiva(fov, a, 2.0f, 900.f);
    M4 view = M4::pohled(k.oko, k.cil, V3(0, 1, 0));
    vp = proj * view;
    posledniFov = fov;
    // pozadi
    pMistnost.pouzij();
    pMistnost.f1("uBlesk", bl);
    pMistnost.f1("uSila", 1.f);
    ctverec.kresli();
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glClear(GL_DEPTH_BUFFER_BIT);
    pModel.pouzij();
    pModel.f3("uKamera", k.oko);
    const V3 lampa(-42, 66, 48);
    pModel.f3("uLampaPos", lampa);
    pModel.f3("uLampaSmer", norm(V3(0, 3, 0) - lampa));
    pModel.f3("uLampa", V3(1.45f, 1.18f, 0.88f));
    pModel.f3("uOknoPos", V3(-22, 40, -88));
    pModel.f3("uOkno", V3(0.15f, 0.20f, 0.29f));
    pModel.f1("uBlesk", bl);
    pModel.f1("uAmb", 0.05f);
    if (energieBarva.x >= 0.f) {        // B307: neonove pruty (scena 4)
      pModel.f3("uEnergiePos", energiePos);
      pModel.f3("uEnergie", energieBarva);
    } else {
      pModel.f3("uEnergiePos", atari.vyduch + V3(0, 6, 0));
      pModel.f3("uEnergie", V3(0.35f, 0.75f, 1.0f) * energie);
    }
    pModel.f1("uKlavZar", klavZar);
    pModel.f1("uLed", ledSvetlo(t));
    pModel.f1("uCas", t);
    pModel.f1("uExpo", 1.35f);
    pModel.f1("uObrJas", 1.f);
    pModel.tex("uAtlas", 0, atari.atlas);
    pModel.tex("uSum", 1, texSum);
    pModel.tex("uObr", 2, texSum);
    M4 jed = M4::jednotka();
    pModel.m4("uModel", jed);
    pModel.m4("uMVP", vp);
    okoli.kresli();
    atari.sit.kresli();
  }

  // svetlo lampy a okna v prasnem vzduchu + zrnka prachu v kuzelu lampy
  void paraLampy(Cil &c, float t, const Kamera &k, float bl, float sila, GLuint cilFbo) {
    const V3 lampa(-42, 66, 48);
    V3 fw = norm(k.cil - k.oko), rt = norm(cross(fw, V3(0, 1, 0))), up = cross(rt, fw);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    pripravAtmo(c);
    glBindFramebuffer(GL_FRAMEBUFFER, c.garaz.fbo);
    glViewport(0, 0, c.garaz.w, c.garaz.h);
    pPara.pouzij();
    pPara.f2("uRozmer", (float)c.garaz.w, (float)c.garaz.h);
    pPara.f3("uKamera", k.oko);
    pPara.f3("uFw", fw); pPara.f3("uRt", rt); pPara.f3("uUp", up);
    pPara.f1("uTan", std::tan(posledniFov * 0.5f));
    pPara.f3("uLampaPos", lampa);
    pPara.f3("uLampaSmer", norm(V3(0, 3, 0) - lampa));
    pPara.f3("uLampa", V3(1.45f, 1.18f, 0.88f));
    pPara.f1("uSila", sila);
    pPara.f1("uCas", t);
    pPara.f3("uOknoPos", V3(-22, 40, -95));
    pPara.f3("uOkno", V3(0.15f, 0.20f, 0.29f));
    pPara.f1("uBlesk", bl);
    pPara.tex("uSum", 0, texSum);
    ctverec.kresli();
    glBindFramebuffer(GL_FRAMEBUFFER, cilFbo);
    glViewport(0, 0, c.w, c.h);
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    kopie(c.garaz.tex, 1.f);
    glDisable(GL_BLEND);
    // zrnka prachu, ktera se tocí ve svetle
    dyn.clear();
    Los los(2468);
    const float S = (float)std::min(c.w, c.h);
    const V3 smer = norm(V3(0, 3, 0) - lampa);
    for (int i = 0; i < 160; i++) {
      V3 p(los.r(-40.f, 40.f), los.r(3.f, 62.f), los.r(-35.f, 45.f));
      float ph = los.r(0, 6.28f), sp = los.r(0.15f, 0.5f);
      p.x += std::sin(t * sp + ph) * 4.f;
      p.y += std::sin(t * sp * 0.7f + ph * 2.f) * 3.f - std::fmod(t * 0.6f + ph, 6.f);
      p.z += std::cos(t * sp * 0.8f + ph) * 4.f;
      V3 L = p - lampa;
      float d = delka(L);
      float kuzel = smooth(0.84f, 0.97f, dot(L * (1.f / d), smer));
      if (kuzel < 0.02f) continue;
      float x, y, z;
      if (!vp.promitni(p, x, y, z) || z > 1.f || std::fabs(x) > 1.1f || std::fabs(y) > 1.1f) continue;
      float vz = delka(p - k.oko);
      float vel = S * 0.010f * (40.f / std::max(vz, 8.f)) * los.r(0.5f, 1.3f) + 1.f;
      float jisk = 0.6f + 0.4f * std::sin(t * los.r(1.f, 3.f) + ph);
      dyn.insert(dyn.end(), {x, y, std::min(vel, 26.f), kuzel * jisk * 0.55f * sila});
    }
    castice(c, V3(1.0f, 0.88f, 0.66f));
  }

  void scena3(float t, Cil &c, GLuint cilFbo) {
    Kamera k = kameraAtari(t, (float)c.w / c.h);
    float bl = bleskOkno(t);
    klavZar = 0.f;
    kresliModel(c, t, k, bl, 0.f);
    kresliKonzole(c, t, k, bl);          // B307: vzadu stoji CRT (vypnuty), konzole jeste nejsou
    kresliCrt(c, t, k);
    glDisable(GL_DEPTH_TEST);
    paraLampy(c, t, k, bl, 1.f, cilFbo);
    ledZare(c, t);
  }

  void ledZare(Cil &c, float t) {
    float l = ledSvetlo(t);
    if (l < 0.01f) return;
    float x, y, z;
    if (!vp.promitni(atari.led, x, y, z)) return;
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    float r = 0.03f * std::min(c.w, c.h) * 2.f / c.h;
    zareNDC(c, x, y, r, V3(1.0f, 0.15f, 0.08f) * (0.9f * l), 3.0f);
    glDisable(GL_BLEND);
  }

  // ================= SCENA 4 (B307): neonove pruty, trojice N&P, defile na CRT =================
  // Na stole: Atari uprostred, N&P 16 vlevo, N&P 32 vpravo (natocene mirne
  // ke stredu), za Atari stoji CRT monitor. Casy v nap_demo_cas.h.
  static V3 P16() { return V3(-41.f, 0.f, 2.f); }
  static V3 P32() { return V3(41.f, 0.f, 2.f); }
  static V3 PCRT() { return V3(0.f, 0.f, -54.f); }
  static M4 maticeKonzole(int i) {
    return i == 0 ? M4::posun(-41.f, 0.f, 2.f) * M4::rotY(0.34f) : M4::posun(41.f, 0.f, 2.f) * M4::rotY(-0.34f);
  }
  static V3 transformuj(const M4 &m, const V3 &p) {
    return V3(m.m[0] * p.x + m.m[4] * p.y + m.m[8] * p.z + m.m[12], m.m[1] * p.x + m.m[5] * p.y + m.m[9] * p.z + m.m[13],
              m.m[2] * p.x + m.m[6] * p.y + m.m[10] * p.z + m.m[14]);
  }
  // zorny uhel jako v kresliModel (na vysku sirsi) a vzdalenost, aby se w x h veslo
  static float fovPro(float a) { return a < 1.f ? 2.f * std::atan(std::tan(0.4f) / std::max(a, 0.45f) * 0.62f) : 0.8f; }
  static float vzdalenostPro(float a, float w, float h) {
    const float t2 = 2.f * std::tan(fovPro(a) * 0.5f);
    return std::max(h / t2, w / (t2 * a));
  }
  V3 stredObrazovky() const { return PCRT() + crt.stred + V3(0.f, 0.f, 1.0f); }

  // kamera sceny 4: hladka krivka (Hermite) pres klicove zabery
  Kamera kamera4(float t, float a) {
    struct K { float t; V3 oko, cil; bool stuj = false; };   // stuj = kamera tu zastavi (nulova tecna)
    const float Dt = vzdalenostPro(a, 128.f, 50.f);     // cela trojice
    const float Dc = vzdalenostPro(a, 36.f, 35.f);       // obrazovka CRT (nahore misto na popisek)
    const V3 sc = stredObrazovky();
    auto smer = [](float x, float y, float z) { return norm(V3(x, y, z)); };
    const Kamera k0 = kameraAtari(cas::KLAVESY, a);
    K ks[10];
    ks[0] = {cas::KLAVESY, k0.oko, k0.cil};
    { V3 c(0.f, 12.f, -2.f); ks[1] = {28.15f, c + smer(-0.08f, 0.45f, 1.f) * std::max(48.f, Dt * 0.45f), c}; }
    { V3 c(0.f, 9.f, -6.f); ks[2] = {29.6f, c + smer(0.f, 0.42f, 1.f) * (Dt * 1.06f), c}; }
    // prujezd podel trojice: zblizka N&P 16 (vlevo), pres Atari k N&P 32 (vpravo)
    const float Dk = vzdalenostPro(a, 44.f, 22.f);
    { V3 c = P16() + V3(1.f, 5.f, 0.f); ks[3] = {30.8f, c + smer(-0.42f, 0.36f, 1.f) * Dk, c}; }
    { V3 c = P32() + V3(-1.f, 5.f, 0.f); ks[4] = {31.85f, c + smer(0.40f, 0.33f, 1.f) * Dk, c}; }
    ks[5] = {cas::STRIH_ATARI, sc + smer(0.f, 0.03f, 1.f) * Dc, sc, true};
    // B309: defile trva 18 s - kamera se behem nej pomalu prenese zleva doprava
    // (o par stupnu) a priblizi, at zaber neni mrtvy; obrazovka zustava cela videt
    ks[6] = {cas::STRIH_16, sc + smer(-0.045f, 0.03f, 1.f) * (Dc * 0.975f), sc};
    ks[7] = {cas::STRIH_32, sc + smer(0.045f, 0.035f, 1.f) * (Dc * 0.955f), sc};
    ks[8] = {cas::STRIHY_KONEC, sc + smer(0.f, 0.03f, 1.f) * (Dc * 0.94f), sc, true};
    ks[9] = {cas::SCENA5, sc + smer(0.f, 0.02f, 1.f) * (Dc * 0.42f), sc};
    const int n = 10;
    Kamera k; k.fov = 0.8f;
    if (t <= ks[0].t) { k.oko = ks[0].oko; k.cil = ks[0].cil; return k; }
    if (t >= ks[n - 1].t) { k.oko = ks[n - 1].oko; k.cil = ks[n - 1].cil; return k; }
    int i = 0;
    while (i < n - 2 && t > ks[i + 1].t) i++;
    const float h = ks[i + 1].t - ks[i].t, u = (t - ks[i].t) / h;
    auto tecna = [&](int j, bool oko) {
      if (j <= 0 || j >= n - 1 || ks[j].stuj) return V3(0, 0, 0);
      const V3 &a0 = oko ? ks[j - 1].oko : ks[j - 1].cil, &a1 = oko ? ks[j + 1].oko : ks[j + 1].cil;
      return (a1 - a0) * (1.f / (ks[j + 1].t - ks[j - 1].t));
    };
    const float u2 = u * u, u3 = u2 * u;
    const float h00 = 2 * u3 - 3 * u2 + 1, h10 = u3 - 2 * u2 + u, h01 = -2 * u3 + 3 * u2, h11 = u3 - u2;
    k.oko = ks[i].oko * h00 + tecna(i, true) * (h10 * h) + ks[i + 1].oko * h01 + tecna(i + 1, true) * (h11 * h);
    k.cil = ks[i].cil * h00 + tecna(i, false) * (h10 * h) + ks[i + 1].cil * h01 + tecna(i + 1, false) * (h11 * h);
    return k;
  }

  // ---- neonove pruty: z kláves Atari do vzduchu a k obema konzolim ----
  struct Prut { V3 p0, p1, p2, p3; float start, delka; int strana; };
  std::vector<Prut> pruty;
  void postavPruty() {
    pruty.clear();
    Los los(3077);
    for (int i = 0; i < 14; i++) {
      const int strana = i < 7 ? 0 : 1, j = i % 7;
      const float x = (strana == 0 ? -1.f : 1.f) * (1.2f + j * 1.95f) + los.r(-0.4f, 0.4f);
      const float z = -1.05f + (float)(j % 4) * 1.9f;
      const V3 p0(x, 4.6f, z);
      const M4 m = maticeKonzole(strana);
      const float vys = strana == 0 ? 7.f : 6.f;
      const V3 cil = transformuj(m, V3(los.r(-7.f, 7.f), vys + los.r(0.f, 2.f), los.r(-6.f, 6.f)));
      Prut p;
      p.p0 = p0;
      p.p1 = p0 + V3(x * 0.25f, 24.f + los.r(0.f, 14.f), los.r(-6.f, 2.f));
      p.p2 = cil + V3(0.f, 26.f + los.r(0.f, 12.f), los.r(-4.f, 4.f));
      p.p3 = cil;
      p.start = cas::PRUTY + j * 0.055f + strana * 0.03f;
      p.delka = 1.05f + los.r(0.f, 0.12f);
      p.strana = strana;
      pruty.push_back(p);
    }
  }
  static V3 bezier(const Prut &p, float s) {
    const float u = 1.f - s;
    return p.p0 * (u * u * u) + p.p1 * (3.f * u * u * s) + p.p2 * (3.f * u * s * s) + p.p3 * (s * s * s);
  }
  // kde je ted nejjasnejsi cast prutu (pro svetlo na modelech)
  V3 stredPrutu(float t, float &sila) const {
    V3 s(0, 0, 0); int n = 0; sila = 0.f;
    for (const Prut &p : pruty) {
      const float hl = (t - p.start) / p.delka;
      if (hl <= 0.f || hl > 1.6f) continue;
      s = s + bezier(p, std::min(1.f, hl)); n++;
    }
    if (n) { s = s * (1.f / n); sila = std::min(1.f, n / 6.f); }
    return n ? s : V3(0, 20, 0);
  }
  void kresliPruty(Cil &c, float t) {
    dyn.clear();
    for (const Prut &p : pruty) {
      const float hl = (t - p.start) / p.delka;
      if (hl <= 0.f) continue;
      const float sH = std::min(1.f, std::pow(hl, 1.15f));
      float sT = std::max(0.f, sH - 0.55f);
      if (hl > 1.f) sT = std::min(1.f, sT + (hl - 1.f) * 2.2f);      // po priletu se ocas stahne do konzole
      if (sT >= 0.995f) continue;
      const int N = 34;
      std::vector<V2> pts; std::vector<float> wv, sv;
      for (int j = 0; j <= N; j++) {
        const float u = (float)j / N;
        const V3 q = bezier(p, sT + (sH - sT) * u);
        float x, y, z;
        if (!vp.promitni(q, x, y, z) || z > 1.f) continue;
        pts.push_back({x, y});
        const float hlava = hl < 1.f ? std::exp(-(1.f - u) * 14.f) * 1.6f : 0.f;
        wv.push_back(c.h * 0.0023f * (0.65f + 0.6f * u + hlava * 0.6f));
        sv.push_back((0.22f + 0.78f * u) + hlava);
      }
      if (pts.size() < 2) continue;
      pruh(c, pts, wv, sv, 5.0f, 1.0f, (float)p.strana, 5.f);
      pruh(c, pts, wv, sv, 1.2f, 1.6f, (float)p.strana, 0.f);
    }
    pasyKresli(V3(1.0f, 1.0f, 1.0f), V3(0.22f, 0.85f, 1.0f), V3(1.0f, 0.28f, 0.85f), 1.f);
  }
  // ---- drateny obrys konzoli (roste z mista priletu, pak zhasne) ----
  void kresliDrat(Cil &c, float t) {
    dyn.clear();
    for (int i = 0; i < 2; i++) {
      const ModelKonzole &kk = i == 0 ? k16 : k32;
      const M4 m = maticeKonzole(i);
      const float t0 = cas::DRAT + i * 0.08f;
      const float zhasne = 1.f - smooth(cas::ODHAL_KONEC - 0.2f + i * 0.15f, cas::ODHAL_KONEC + 0.35f + i * 0.15f, t);
      if (t < t0 || zhasne <= 0.f) continue;
      for (size_t e = 0; e < kk.hrany.size(); e++) {
        const V3 a = transformuj(m, kk.hrany[e].first), b = transformuj(m, kk.hrany[e].second);
        const float zp = t0 + 0.018f * (float)e;
        const float g = smooth(zp, zp + 0.26f, t);
        if (g <= 0.f) continue;
        const V3 bb = a + (b - a) * g;
        float xa, ya, za, xb, yb, zb;
        if (!vp.promitni(a, xa, ya, za) || !vp.promitni(bb, xb, yb, zb)) continue;
        std::vector<V2> pts = {{xa, ya}, {xb, yb}};
        const float hlava = g < 1.f ? 1.4f : 0.f;
        std::vector<float> wv = {c.h * 0.0015f, c.h * 0.0015f * (1.f + hlava)};
        std::vector<float> sv = {0.85f * zhasne, (0.85f + hlava) * zhasne};
        pruh(c, pts, wv, sv, 4.0f, 1.0f, (float)i, 4.f);
        pruh(c, pts, wv, sv, 1.1f, 1.8f, (float)i, 0.f);
      }
    }
    pasyKresli(V3(1.0f, 1.0f, 1.0f), V3(0.22f, 0.85f, 1.0f), V3(1.0f, 0.28f, 0.85f), 1.f);
  }

  // B313: animace konzoli ve scene 4 - hned po zhmotneni: N&P 16 zasune cartridge (kamera
  // je u ni v 30,8 s), N&P 32 vlozi disk a zavre viko; kdyz k ni kamera dojede (31,85 s),
  // rozsviti se a disk se roztoci - pak kamera leti k CRT, ktery se zapne ve 32,35 s
  static AnimKonzole animKonzole(int i, float t) {
    return i == 0 ? ModelKonzole::anim16(t - cas::ODHAL_KONEC, t) : ModelKonzole::anim32(t - (cas::ODHAL_KONEC + 0.05f), t);
  }
  // zare kolem svitici kontrolek konzoli (jako LED Atari)
  void konzoleZare(Cil &c, float t) {
    if (t < cas::ODHAL_KONEC) return;
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    for (int i = 0; i < 2; i++) {
      const ModelKonzole &kk = i == 0 ? k16 : k32;
      const AnimKonzole a = animKonzole(i, t);
      const M4 m = maticeKonzole(i);
      for (const auto &l : kk.ledy) {
        const float jas = a.led[l.i];
        if (jas < 0.04f) continue;
        float x, y, z;
        if (!vp.promitni(transformuj(m, l.p), x, y, z) || z > 1.f) continue;
        zareNDC(c, x, y, 0.022f, l.barva * (0.45f * jas), 3.0f);
      }
    }
    glDisable(GL_BLEND);
  }

  // ---- konzole a CRT (stejna lampa a okno jako u Atari) ----
  void kresliKonzole(Cil &c, float t, const Kamera &k, float bl) {
    (void)c;
    const V3 lampa(-42, 66, 48);
    pKonzole.pouzij();
    pKonzole.f3("uKamera", k.oko);
    pKonzole.f3("uLampaPos", lampa);
    pKonzole.f3("uLampaSmer", norm(V3(0, 3, 0) - lampa));
    pKonzole.f3("uLampa", V3(1.45f, 1.18f, 0.88f));
    pKonzole.f3("uOknoPos", V3(-22, 40, -88));
    pKonzole.f3("uOkno", V3(0.15f, 0.20f, 0.29f));
    pKonzole.f1("uBlesk", bl);
    pKonzole.f1("uAmb", 0.14f);
    pKonzole.f1("uExpo", 1.35f);
    pKonzole.f1("uCas", t);
    const float neon = smooth(cas::PRILET - 0.25f, cas::ODHAL, t) * (1.f - 0.30f * smooth(cas::ODHAL_KONEC, cas::TROJICE + 0.7f, t)) *
                       (1.f - 0.7f * smooth(cas::PRULET, cas::CRT_ZAP, t));
    pKonzole.f3("uNeon1Pos", transformuj(maticeKonzole(0), V3(0, 18, 10)));
    pKonzole.f3("uNeon1", V3(0.22f, 0.85f, 1.0f) * (1.7f * neon));
    pKonzole.f3("uNeon2Pos", transformuj(maticeKonzole(1), V3(0, 18, 10)));
    pKonzole.f3("uNeon2", V3(1.0f, 0.28f, 0.85f) * (1.7f * neon));
    // odlesk pres emblem N&P - kdyz kolem kamera projizdi
    pKonzole.tex("uLogo", 0, texLogo ? texLogo : texSum);
    pKonzole.tex("uSum", 1, texSum);
    pKonzole.f1("uPodlaha", 0.f);           // B313: stul je v y = 0
    pKonzole.f1("uStudio", 0.22f);          // B313: jemne studiove odlesky (v mistnosti sviti hlavne lampa)
    glActiveTexture(GL_TEXTURE0);
    for (int i = 0; i < 2; i++) {
      const ModelKonzole &kk = i == 0 ? k16 : k32;
      const float od = cas::ODHAL + i * 0.15f;
      const float h = (t - od) / (cas::ODHAL_KONEC - cas::ODHAL);
      if (h <= 0.f) continue;                               // jeste neni (jen drat)
      pKonzole.f1("uOdhal", h >= 1.25f ? 1000.f : -0.6f + (kk.vyska + 1.6f) * h);
      pKonzole.f3("uOdhalBarva", i == 0 ? V3(0.25f, 0.9f, 1.0f) : V3(1.0f, 0.32f, 0.9f));
      pKonzole.f1("uSvit", smooth(cas::ODHAL_KONEC - 0.1f + i * 0.15f, cas::ODHAL_KONEC + 0.3f + i * 0.15f, t));
      pKonzole.f1("uLogoLesk", -0.5f + 2.4f * smooth(i == 0 ? 30.45f : 31.40f, i == 0 ? 31.05f : 32.0f, t));
      kk.kresli(pKonzole, maticeKonzole(i), vp, animKonzole(i, t));     // B313: skrin + pohyblive dily
    }
    const M4 mc = M4::posun(PCRT().x, PCRT().y, PCRT().z);
    pKonzole.f1("uOdhal", 1000.f);
    pKonzole.f1("uLogoLesk", -1.f);
    pKonzole.f1("uSvit", smooth(cas::CRT_ZAP - 0.05f, cas::CRT_ZAP + 0.1f, t));
    pKonzole.m4("uModel", mc);
    pKonzole.m4("uMVP", vp * mc);
    crt.sit.kresli();
  }

  // ---- obsah CRT: Pitt-Kitt (klip), 16 bit (procesor), 32 bit (GPU) ----
  struct ObsahCrt { GLuint tex = 0; float mapa[4] = {0, 0, 1, 1}; float zdroj[2] = {320, 240}; float snih = 0, roll = 0; };
  ObsahCrt obsah;
  // Volat PRED kreslenim sceny (32bit kresli do vlastniho framebufferu).
  void pripravObsahCrt(float t) {
    ObsahCrt o;
    auto prepnuti = [&](float kdy) {
      const float d = t - kdy;
      if (d >= 0.f && d < 0.10f) { o.snih = std::max(o.snih, 1.f - d / 0.10f); o.roll = 0.35f * (1.f - d / 0.10f); }
    };
    if (t < cas::STRIH_16) {
      // Atari: titulek N&P, kapitola (Las Vegas 1982), jizda - 50 snimku / s
      if (klip.je() && klip.useky.size() >= 3) {
        int k;
        if (t < cas::ATARI_KAP) { const auto &u = klip.useky[0]; k = u.prvni + std::max(0, std::min(u.pocet - 1, (int)((t - cas::CRT_ZAP) * klip.fps))); }
        else if (t < cas::ATARI_JIZDA) { const auto &u = klip.useky[1]; k = u.prvni + std::min(u.pocet - 1, (int)((t - cas::ATARI_KAP) * klip.fps)); }
        else { const auto &u = klip.useky[2]; k = u.prvni + std::min(u.pocet - 1, 6 + (int)((t - cas::ATARI_JIZDA) * klip.fps)); }
        if (k != posAtari && klip.rgba(k, buf8)) {
          glBindTexture(GL_TEXTURE_2D, tex8);
          glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, klip.W, klip.H, GL_RGBA, GL_UNSIGNED_BYTE, buf8.data());
          posAtari = k;
        }
        o.tex = tex8;
        // vyrez 352 x 240 z 384 x 240 (bez okraju), radky shora dolu
        o.mapa[0] = 16.f / 384.f; o.mapa[1] = 1.f; o.mapa[2] = 352.f / 384.f; o.mapa[3] = -1.f;
        o.zdroj[0] = 352.f; o.zdroj[1] = 240.f;
      } else {
        o.tex = 0; o.snih = 1.f;                          // bez klipu: sum
      }
      if (t < cas::STRIH_ATARI) o.snih = std::max(o.snih, 0.45f * (1.f - smooth(cas::CRT_ZAP + 0.1f, cas::STRIH_ATARI, t)));
    } else if (t < cas::STRIH_32) {
      const int n = 36 + (int)((t - cas::STRIH_16) * 60.f);
      if (n != pos16) {
        hra16.snimek(n);
        glBindTexture(GL_TEXTURE_2D, tex16);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, Hra16::W, Hra16::H, GL_RGBA, GL_UNSIGNED_BYTE, hra16.obr.data());
        pos16 = n;
      }
      o.tex = tex16;
      o.mapa[0] = 0.f; o.mapa[1] = 1.f; o.mapa[2] = 1.f; o.mapa[3] = -1.f;
      o.zdroj[0] = (float)Hra16::W; o.zdroj[1] = (float)Hra16::H;
      prepnuti(cas::STRIH_16);
    } else {
      const float tt = 0.05f + (t - cas::STRIH_32);
      if (hra32ok) {
        if (tt != pos32) { hra32.snimek(tt); pos32 = tt; }
        o.tex = hra32.fbo.tex;
      } else {
        o.snih = 1.f;
      }
      o.mapa[0] = 0.f; o.mapa[1] = 0.f; o.mapa[2] = 1.f; o.mapa[3] = 1.f;
      o.zdroj[0] = (float)Hra32::W; o.zdroj[1] = (float)Hra32::H;
      prepnuti(cas::STRIH_32);
    }
    obsah = o;
  }
  void kresliCrt(Cil &c, float t, const Kamera &k) {
    (void)c;
    const float zap = smooth(cas::CRT_ZAP, cas::CRT_ZAP + 0.42f, t);
    const V3 pc = PCRT();
    const M4 mc = M4::posun(pc.x, pc.y, pc.z);
    pCrt.pouzij();
    pCrt.m4("uMVP", vp * mc);
    pCrt.tex("uObr", 0, (zap > 0.f && obsah.tex) ? obsah.tex : texSum);
    pCrt.tex("uSum", 1, texSum);
    pCrt.f4("uMapa", obsah.mapa[0], obsah.mapa[1], obsah.mapa[2], obsah.mapa[3]);
    pCrt.f2("uZdroj", obsah.zdroj[0], obsah.zdroj[1]);
    pCrt.f1("uZap", zap);
    pCrt.f1("uSnih", zap > 0.f ? obsah.snih : 0.f);
    pCrt.f1("uRoll", obsah.roll);
    pCrt.f1("uJas", 0.95f);
    pCrt.f3("uKamera", k.oko - pc);
    pCrt.f3("uLampaPos", V3(-42, 66, 48) - pc);
    pCrt.f3("uLampa", V3(1.45f, 1.18f, 0.88f));
    pCrt.f1("uCas", t);
    crt.sklo.kresli();
    glActiveTexture(GL_TEXTURE0);
  }

  float klavesyZar(float t) {
    return smooth(cas::KLAVESY, cas::KLAVESY + 0.45f, t) * (1.f - smooth(cas::PRILET - 0.1f, cas::ODHAL + 0.2f, t) * 0.85f);
  }

  void scena4(float t, Cil &c, GLuint cilFbo) {
    const float a = (float)c.w / c.h;
    pripravObsahCrt(t);
    glBindFramebuffer(GL_FRAMEBUFFER, cilFbo);
    glViewport(0, 0, c.w, c.h);
    Kamera k = kamera4(t, a);
    klavZar = klavesyZar(t);
    float silaPrutu = 0.f;
    const V3 sp = stredPrutu(t, silaPrutu);
    // energie (neon) sviti na Atari a stul
    energiePos = sp;
    energieBarva = V3(0.40f, 0.55f, 1.0f) * (silaPrutu * 0.85f + klavZar * 0.22f);
    kresliModel(c, t, k, 0.f, 1.f);
    energiePos = V3(0, 0, 0); energieBarva = V3(-1, -1, -1);
    kresliKonzole(c, t, k, 0.f);
    kresliCrt(c, t, k);
    glDisable(GL_DEPTH_TEST);
    paraLampy(c, t, k, 0.f, t < cas::PRULET ? 0.6f : 0.6f * (1.f - smooth(cas::PRULET, cas::STRIH_ATARI, t) * 0.6f), cilFbo);
    ledZare(c, t);
    konzoleZare(c, t);                   // B313
    if (t < cas::ODHAL_KONEC + 0.6f) {
      kresliPruty(c, t);
      kresliDrat(c, t);
      // zablesk v miste priletu prutu
      for (int i = 0; i < 2; i++) {
        const float d = t - (cas::PRILET + i * 0.05f);
        if (d < 0.f || d > 0.45f) continue;
        float x, y, z;
        if (!vp.promitni(transformuj(maticeKonzole(i), V3(0, 6, 0)), x, y, z)) continue;
        glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
        zareNDC(c, x, y, 0.06f + d * 0.5f, (i == 0 ? V3(0.4f, 0.9f, 1.0f) : V3(1.0f, 0.45f, 0.9f)) * (1.f - d / 0.45f), 2.2f);
        glDisable(GL_BLEND);
      }
    }
    // popisky defile (nahore nad obrazovkou)
    {
      const int TX[3] = {T_ATARI, T_16, T_32};
      const float OD[3] = {cas::STRIH_ATARI, cas::STRIH_16, cas::STRIH_32};
      const float DO[3] = {cas::STRIH_16, cas::STRIH_32, cas::STRIHY_KONEC};
      for (int i = 0; i < 3; i++) {
        const float al = smooth(OD[i] + 0.04f, OD[i] + 0.20f, t) * (1.f - smooth(DO[i] - 0.14f, DO[i] - 0.02f, t));
        if (al <= 0.001f) continue;
        const TextTex &tx = c.texty[TX[i]];
        const float y = c.h * (c.w > c.h ? 0.035f : 0.10f) + tx.h * 0.5f;
        if (tx.tex) textKresli(c, tx, c.w * 0.5f, y, al);
        if (i == 0) {
          const TextTex &t2 = c.texty[T_PITT];
          if (t2.tex) textKresli(c, t2, c.w * 0.5f, y + tx.h * 0.5f + t2.h * 0.45f, al);
        }
      }
    }
    if (t > cas::STRIHY_KONEC) prekryv(1, 1, 1, smooth(cas::STRIHY_KONEC, cas::SCENA5 - 0.02f, t) * 0.85f);
  }

  // ---- tlacitko SKIP (jen telefon) ----
  void tlacitkoSkip(const Cil &c, float alfa) {
    if (alfa <= 0.001f) return;
    float x0, y0, x1, y1;
    skipObd(c.w, c.h, x0, y0, x1, y1);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    pTlacitko.pouzij();
    obdPx(c, x0, y0, x1, y1, 0, 0, 1, 1, pTlacitko);
    pTlacitko.f2("uVel", x1 - x0, y1 - y0);
    pTlacitko.f1("uAlfa", alfa);
    ctverec.kresli();
    glDisable(GL_BLEND);
    const TextTex &tx = c.texty[T_SKIP];
    if (tx.tex) textKresli(c, tx, (x0 + x1) * 0.5f, (y0 + y1) * 0.5f, alfa);
  }

  // ================= SCENA 5: logo a titulky =================
  void scena5(float t, Cil &c) {
    if (t < 37.85f) return;            // tma
    const bool naSirku = c.w > c.h;
    float L, cx, cy;
    if (naSirku) { L = std::min(c.h * 0.62f, c.w * 0.40f); cx = c.w * 0.29f; cy = c.h * 0.5f; }
    else { L = std::min(c.w * 0.66f, c.h * 0.36f); cx = c.w * 0.5f; cy = c.h * 0.07f + L * 0.5f; }
    const float konec = 1.f - smooth(44.2f, 44.95f, t);
    const float odhal = smooth(37.9f, 39.6f, t) * konec;
    // zlata zare za logem
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    {
      float r = L * 0.95f;
      Cil &cc = c;
      float x0 = cx - r, x1 = cx + r, y0 = cy - r, y1 = cy + r;
      pZare.pouzij();
      obdPx(cc, x0, y0, x1, y1, 0, 0, 1, 1, pZare);
      pZare.f3("uBarva", V3(0.55f, 0.38f, 0.12f) * (0.55f * odhal * (0.9f + 0.1f * std::sin(t * 2.f))));
      pZare.f1("uTvrdost", 2.6f);
      ctverec.kresli();
    }
    // logo
    if (texLogo) {
      glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
      float sk = 0.94f + 0.06f * easeOut(smooth(37.9f, 41.f, t));
      float lw = L * sk, lh = L * sk / logoPomer;
      pLogo.pouzij();
      obdPx(c, cx - lw * 0.5f, cy - lh * 0.5f, cx + lw * 0.5f, cy + lh * 0.5f, 0, 1, 1, 0, pLogo);
      pLogo.tex("uTex", 0, texLogo);
      pLogo.f1("uOdhal", odhal);
      float pas = -0.4f + 2.4f * smooth(38.5f, 39.9f, t);
      if (t > 41.8f) pas = -0.4f + 2.4f * smooth(41.8f, 43.1f, t);
      pLogo.f1("uPas", pas);
      pLogo.f1("uCas", t);
      ctverec.kresli();
    }
    // zlaty prach
    {
      dyn.clear();
      Los los(99);
      for (int i = 0; i < 110; i++) {
        float zr = los.r(37.9f, 44.5f), zivot = los.r(2.0f, 4.2f);
        float st = t - zr;
        if (st < 0.f || st > zivot) continue;
        float x = cx + los.r(-0.55f, 0.55f) * L + std::sin(st * 1.3f + i) * L * 0.04f;
        float y = cy + L * 0.45f - st * L * los.r(0.08f, 0.22f);
        float a = std::sin(st / zivot * 3.14159f) * odhal;
        float vel = std::min(c.w, c.h) * los.r(0.004f, 0.010f);
        dyn.insert(dyn.end(), {x / c.w * 2.f - 1.f, 1.f - y / c.h * 2.f, vel, a * 0.9f});
      }
      castice(c, V3(1.0f, 0.78f, 0.38f));
      glEnable(GL_BLEND);
    }
    // titulky
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    static const float OD[6] = {39.2f, 39.85f, 40.35f, 41.05f, 41.95f, 42.75f};
    float vys[6], celk = 0;
    const float mez = std::min(c.w, c.h) * 0.022f;
    for (int i = 0; i < 6; i++) { vys[i] = (float)c.texty[T_K1 + i].h - c.texty[T_K1 + i].okraj * 2.f; celk += vys[i] + mez; }
    float y, x;
    if (naSirku) { x = c.w * 0.72f; y = c.h * 0.5f - celk * 0.5f; }
    else { x = c.w * 0.5f; y = cy + L * 0.5f + c.h * 0.045f; }
    for (int i = 0; i < 6; i++) {
      const TextTex &tx = c.texty[T_K1 + i];
      float a = smooth(OD[i], OD[i] + 0.7f, t) * konec;
      float posun = (1.f - easeOut(smooth(OD[i], OD[i] + 0.9f, t))) * mez * 1.2f;
      if (tx.tex && a > 0.001f) textKresli(c, tx, x, y + vys[i] * 0.5f + posun, a);
      y += vys[i] + mez;
    }
    glDisable(GL_BLEND);
  }

  // ================= texty =================
  TextTex vysadText(Cil &c, int id) {
    const float S = (float)std::min(c.w, c.h);
    const bool naSirku = c.w > c.h;
    const float maxW = naSirku ? c.w * 0.72f : c.w * 0.88f;
    std::vector<Odstavec> o;
    auto od = [&](const char *t, const Pismo &p, float px, float r, float g, float b, float a, float proklad = 0.f,
                  float pred = 0.f, float radek = 1.22f) {
      Odstavec x; x.text = t; x.pismo = &p; x.px = px; x.r = r; x.g = g; x.b = b; x.a = a; x.proklad = proklad;
      x.mezeraPred = pred; x.radek = radek;
      o.push_back(x);
    };
    const float tit = S * (c.tv ? 0.050f : 0.043f);
    if (id >= T_TIT && id < T_TIT + 8) {
      const Titulek &ti = TITULKY[id - T_TIT];
      od(ti.cz, pSemi, tit, 1, 1, 1, 1);
      od(ti.en, pMed, tit * 0.80f, 0.93f, 0.82f, 0.56f, 0.95f, 0.f, tit * 0.25f);
      return Sazec::vysazej(o, maxW, 0, tit * 0.30f, 0.85f);
    }
    const float credW = naSirku ? c.w * 0.44f : c.w * 0.86f;
    const float k = S * (naSirku ? 0.040f : 0.036f);
    switch (id) {
      case T_K1: od(u8"OFICIÁLNÍ VYDÁNÍ APLIKACE EMULÁTORU", pSemi, k * 0.78f, 1, 1, 1, 0.92f, 0.12f);
                 od(u8"ATARI, SEGA MEGA DRIVE, PS1:", pBold, k * 0.98f, 1, 1, 1, 1, 0.08f, k * 0.15f); break;
      case T_K2: od(u8"20. LISTOPADU 2026", pBold, k * 1.55f, 1, 1, 1, 1, 0.06f); break;
      case T_K3: od(u8"– exkluzivně na atarihelp.eu", pMed, k * 0.95f, 1, 1, 1, 0.95f, 0.03f); break;
      case T_K4: od(u8"„It's a small step for a man…", pItal, k * 0.78f, 1, 1, 1, 0.88f);
                 od(u8"And do you have some spare change for parking, Neil?“", pItal, k * 0.78f, 1, 1, 1, 0.88f); break;
      case T_K5: od(u8"Deep gratitude to:", pMed, k * 0.70f, 1, 1, 1, 0.75f, 0.10f);
                 od(u8"MELBOURNE HOUSE & my AI Wingman", pSemi, k * 0.95f, 1, 1, 1, 1, 0.06f, k * 0.1f); break;
      case T_K6: od(u8"(C) 2026 N&P Studio. Všechna práva vyhrazena.", pMed, k * 0.62f, 1, 1, 1, 0.70f, 0.05f); break;
      case T_ATARI: od("ATARI 130XE", pBold, S * 0.070f, 0.98f, 0.84f, 0.50f, 1, 0.12f); break;
      case T_PITT: od("PITT-KITT PLATINUM", pSemi, S * 0.034f, 1, 1, 1, 0.92f, 0.18f); break;
      case T_16: od("16-BIT", pBold, S * 0.070f, 0.98f, 0.84f, 0.50f, 1, 0.12f); break;
      case T_32: od("32-BIT", pBold, S * 0.070f, 0.98f, 0.84f, 0.50f, 1, 0.12f); break;
      case T_SKIP: od(u8"SKIP ›", pBold, S * 0.036f, 1, 1, 1, 1, 0.08f); break;
      default: break;
    }
    if (id >= T_K1 && id <= T_K6) return Sazec::vysazej(o, credW, naSirku ? 0 : 0, k * 0.28f, 0.6f);
    return Sazec::vysazej(o, maxW, 0, S * 0.012f, 0.8f);
  }

  // ================= kresleni - pomocne =================
  void pripravAtmo(Cil &c) {
    int w = std::max(16, (int)(c.w * c.meritko)), h = std::max(16, (int)(c.h * c.meritko));
    c.nebe.priprav(w, h);
    c.garaz.priprav(w, h);
  }
  void kopie(GLuint tex, float jas) {
    pKopie.pouzij();
    pKopie.tex("uTex", 0, tex);
    pKopie.f1("uJas", jas);
    ctverec.kresli();
  }
  void prekryv(float r, float g, float b, float a) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    pBarva.pouzij();
    pBarva.f4("uBarva", r, g, b, a);
    ctverec.kresli();
    glDisable(GL_BLEND);
  }
  // "film": jasne casti se rozmazou a prictou (zare kolem svetel), pak
  // vinetace a zrno. Zare se pocita v 1/4 a 1/8 rozliseni - je to levne.
  void film(Cil &c, float t, GLuint cilFbo, float zare, float vinet, float zrn) {
    const int w4 = std::max(8, c.w / 4), h4 = std::max(8, c.h / 4);
    const int w8 = std::max(8, c.w / 8), h8 = std::max(8, c.h / 8);
    c.zare[0].priprav(w4, h4); c.zare[1].priprav(w4, h4);
    c.zare[2].priprav(w8, h8); c.zare[3].priprav(w8, h8);
    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);
    glBindFramebuffer(GL_FRAMEBUFFER, c.zare[0].fbo);
    glViewport(0, 0, w4, h4);
    pJas.pouzij();
    pJas.tex("uTex", 0, c.scena.tex);
    pJas.f2("uKrok", 1.f / c.w, 1.f / c.h);
    pJas.f1("uPrah", 0.55f);
    ctverec.kresli();
    rozmaz(c.zare[0], c.zare[1]);
    glBindFramebuffer(GL_FRAMEBUFFER, c.zare[2].fbo);
    glViewport(0, 0, w8, h8);
    pJas.pouzij();
    pJas.tex("uTex", 0, c.zare[0].tex);
    pJas.f2("uKrok", 1.f / w4, 1.f / h4);
    pJas.f1("uPrah", 0.0f);
    ctverec.kresli();
    rozmaz(c.zare[2], c.zare[3]);
    glBindFramebuffer(GL_FRAMEBUFFER, cilFbo);
    glViewport(0, 0, c.w, c.h);
    pFinal.pouzij();
    pFinal.tex("uScena", 0, c.scena.tex);
    pFinal.tex("uZare1", 1, c.zare[0].tex);
    pFinal.tex("uZare2", 2, c.zare[2].tex);
    pFinal.tex("uSum", 3, texSum);
    pFinal.f2("uRozmer", (float)c.w, (float)c.h);
    pFinal.f1("uZare", zare);
    pFinal.f1("uCas", t);
    pFinal.f1("uVinet", vinet);
    pFinal.f1("uZrno", zrn);
    ctverec.kresli();
    glActiveTexture(GL_TEXTURE0);
  }
  void rozmaz(Fbo &a, Fbo &b) {
    pRozmaz.pouzij();
    glBindFramebuffer(GL_FRAMEBUFFER, b.fbo);
    glViewport(0, 0, b.w, b.h);
    pRozmaz.tex("uTex", 0, a.tex);
    pRozmaz.f2("uSmer", 1.f / a.w, 0.f);
    ctverec.kresli();
    glBindFramebuffer(GL_FRAMEBUFFER, a.fbo);
    glViewport(0, 0, a.w, a.h);
    pRozmaz.tex("uTex", 0, b.tex);
    pRozmaz.f2("uSmer", 0.f, 1.f / b.h);
    ctverec.kresli();
  }
  // obdelnik v bodech (pocatek vlevo nahore) -> uniformy uObd/uUV programu
  void obdPx(const Cil &c, float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1, Prog &p) {
    float nx0 = x0 / c.w * 2.f - 1.f, nx1 = x1 / c.w * 2.f - 1.f;
    float ny0 = 1.f - y1 / c.h * 2.f, ny1 = 1.f - y0 / c.h * 2.f;
    p.f4("uObd", nx0, ny0, nx1, ny1);
    p.f4("uUV", u0, v0, u1, v1);
  }
  void textKresli(const Cil &c, const TextTex &t, float cx, float cy, float alfa) {
    if (!t.tex) return;
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    pPlny.pouzij();
    float x0 = std::floor(cx - t.w * 0.5f), y0 = std::floor(cy - t.h * 0.5f);
    obdPx(c, x0, y0, x0 + t.w, y0 + t.h, 0, 1, 1, 0, pPlny);
    pPlny.tex("uTex", 0, t.tex);
    pPlny.f1("uAlfa", alfa);
    ctverec.kresli();
    glDisable(GL_BLEND);
  }
  void titulek(const Cil &c, const TextTex &t, float alfa) {
    if (!t.tex) return;
    // stmavnuti pod titulkem kvuli citelnosti
    float spod = c.h * (c.w > c.h ? 0.055f : 0.065f);
    float cy = c.h - spod - t.h * 0.5f;
    textKresli(c, t, c.w * 0.5f, cy, alfa);
  }
  void zareNDC(const Cil &c, float x, float y, float r, const V3 &barva, float tvrdost) {
    float a = (float)c.w / c.h;
    pZare.pouzij();
    pZare.f4("uObd", x - r / a, y - r, x + r / a, y + r);
    pZare.f4("uUV", 0, 0, 1, 1);
    pZare.f3("uBarva", barva);
    pZare.f1("uTvrdost", tvrdost);
    ctverec.kresli();
  }
  // body v dyn: x, y (NDC), velikost, alfa
  void castice(const Cil &c, const V3 &barva) {
    (void)c;
    if (dyn.empty()) return;
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    pCastice.pouzij();
    pCastice.f3("uBarva", barva);
    glBindBuffer(GL_ARRAY_BUFFER, vboDyn);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(dyn.size() * sizeof(float)), dyn.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 16, (void *)0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 16, (void *)8);
    glDrawArrays(GL_POINTS, 0, (GLsizei)(dyn.size() / 4));
    glDisableVertexAttribArray(0); glDisableVertexAttribArray(2);
    glDisable(GL_BLEND);
  }
  // pruh svetla podel lomene cary (NDC): sirky v bodech, sila, nasobic sirky
  void pruh(const Cil &c, const std::vector<V2> &p, const std::vector<float> &w, const std::vector<float> &s, float kSir, float ostrost,
            float barva, float halo) {
    const float a = (float)c.w / c.h;
    for (size_t i = 0; i + 1 < p.size(); i++) {
      // smer v "ctvercovych" souradnicich (x * pomer)
      float dx = (p[i + 1].x - p[i].x) * a, dy = p[i + 1].y - p[i].y;
      float l = std::sqrt(dx * dx + dy * dy);
      if (l < 1e-6f) continue;
      float nx = -dy / l, ny = dx / l;
      float w0 = w[i] * kSir * 2.f / c.h, w1 = w[i + 1] * kSir * 2.f / c.h;
      float ax = p[i].x + nx * w0 / a, ay = p[i].y + ny * w0, bx = p[i].x - nx * w0 / a, by = p[i].y - ny * w0;
      float cx2 = p[i + 1].x + nx * w1 / a, cy2 = p[i + 1].y + ny * w1, dx2 = p[i + 1].x - nx * w1 / a, dy2 = p[i + 1].y - ny * w1;
      float m0[4] = {s[i], halo > 0 ? 3.f : 8.f * ostrost, halo > 0 ? halo * 0.1f : 0.f, barva};
      float m1[4] = {s[i + 1], m0[1], m0[2], barva};
      auto vrch = [&](float x, float y, float u, float v, const float *m) {
        dyn.insert(dyn.end(), {x, y, u, v, m[0], m[1], m[2], m[3]});
      };
      vrch(ax, ay, 0, 1, m0); vrch(bx, by, 0, -1, m0); vrch(cx2, cy2, 1, 1, m1);
      vrch(bx, by, 0, -1, m0); vrch(dx2, dy2, 1, -1, m1); vrch(cx2, cy2, 1, 1, m1);
    }
  }
  void pasyKresli(const V3 &jadro, const V3 &halo, const V3 &halo2, float sila) {
    if (dyn.empty()) return;
    glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE);
    pPas.pouzij();
    pPas.f3("uJadro", jadro * sila);
    pPas.f3("uHalo", halo * sila);
    pPas.f3("uHalo2", halo2 * sila);
    glBindBuffer(GL_ARRAY_BUFFER, vboDyn);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(dyn.size() * sizeof(float)), dyn.data(), GL_STREAM_DRAW);
    glEnableVertexAttribArray(0); glEnableVertexAttribArray(2); glEnableVertexAttribArray(3);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 32, (void *)0);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 32, (void *)8);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 32, (void *)16);
    glDrawArrays(GL_TRIANGLES, 0, (GLsizei)(dyn.size() / 8));
    glDisableVertexAttribArray(0); glDisableVertexAttribArray(2); glDisableVertexAttribArray(3);
    glDisable(GL_BLEND);
    dyn.clear();
  }
  M4 vp = M4::jednotka();
  float posledniFov = 0.8f;
};

}  // namespace napdemo
