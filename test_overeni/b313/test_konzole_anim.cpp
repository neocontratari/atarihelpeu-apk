// test_overeni/b313/test_konzole_anim.cpp
// ====================================================================
//  B313: ANIMACE KONZOLI N&P 16 A N&P 32 (logika bez GPU) - stejny kod pro menu i intro
//  (intro/nap_demo_konzole.h: ModelKonzole::anim16 / anim32, matice dilu).
//
//  N&P 16: cartridge venku se vznasi nad sterbinou (klapky zavrene, vypnuto) -> sjede dolu,
//          zasune se (klapky se otevrou), cvakne, vypinac doprava, POWER sviti, CART zablika 3x.
//  N&P 32: viko otevrene, disk nad mechanikou -> disk dosedne na hridel, viko se zavre,
//          rozsviti se POWER, kontrolky po sobe, disk se roztoci (rozmazani potisku).
//  Navic geometrie: disk nad mechanikou nenarazi do otevreneho vika, cartridge nad vezi
//  nenarazi do ramecku, zasunuta cartridge nekonci pod dnem sterbiny.
//
//   g++ -std=c++17 -O2 -I$INC -o test_konzole_anim test_konzole_anim.cpp -l:libGLESv2.so.2 && ./test_konzole_anim
// ====================================================================
#include <GLES2/gl2.h>
#include <cmath>
#include <cstdio>
#include <string>

#define NAPDEMO_LOG(...) ((void)0)
#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#include "../../app/src/main/cpp/intro/nap_demo_konzole.h"

using namespace napdemo;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
}
static std::string f2(float x) { char b[32]; std::snprintf(b, sizeof b, "%.2f", x); return b; }

int main() {
  std::printf("B313 - animace konzoli N&P 16 a N&P 32 (menu i intro)\n\n");
  // ---------------- N&P 16 ----------------
  {
    bool venkuOk = true;
    for (float t = 0.f; t < 10.f; t += 0.37f) {
      const AnimKonzole a = ModelKonzole::anim16(-1.f, t);
      if (a.kazetaY < 2.7f || a.klapky != 0.f || a.posuvnik != 0.f || a.led[0] != 0.f || a.led[1] != 0.f) venkuOk = false;
    }
    over(venkuOk, "1) N&P 16 mimo stred menu: cartridge se vznasi nad sterbinou (>= 2,7 cm), klapky zavrene, vypnuto, kontrolky zhasnute");
    // sjizdi bez couvani az do cvaknuti
    float pred = 99.f, nejniz = 99.f;
    bool klesa = true;
    for (float tv = 0.f; tv <= 1.20f; tv += 0.01f) {
      const AnimKonzole a = ModelKonzole::anim16(tv, 5.f);
      if (a.kazetaY > pred + 1e-4f) klesa = false;
      pred = a.kazetaY;
    }
    for (float tv = 1.20f; tv <= 1.40f; tv += 0.005f) nejniz = std::min(nejniz, ModelKonzole::anim16(tv, 5.f).kazetaY);
    const AnimKonzole k = ModelKonzole::anim16(2.0f, 5.f);
    over(klesa && nejniz < -5.1f && std::fabs(k.kazetaY + 5.f) < 1e-4f && k.klapky == 1.f,
         "2) N&P 16: cartridge sjede plynule dolu (zadne couvani), cvakne (" + f2(nejniz) + " cm) a zustane zasunuta (" + f2(k.kazetaY) +
             " cm pod vrchem veze), klapky otevrene");
    // klapky se oteviraji az kdyz spodek cartridge projizdi sterbinou
    const float kNad = ModelKonzole::anim16(0.80f, 5.f).klapky, kPul = ModelKonzole::anim16(1.02f, 5.f).klapky;
    over(kNad == 0.f && kPul > 0.3f, "3) N&P 16: klapky se otevrou, az kdyz cartridge vjede do sterbiny (nad ni " + f2(kNad) + ", v pulce " + f2(kPul) + ")");
    // zapnuti: vypinac, POWER, CART blika 3x a pak sviti
    const AnimKonzole z = ModelKonzole::anim16(1.9f, 5.f);
    int bliku = 0;
    float predCart = 0.f;
    for (float tv = 1.80f; tv < 3.0f; tv += 0.01f) {
      const float c = ModelKonzole::anim16(tv, 5.f).led[1];
      if (c > 0.5f && predCart <= 0.5f) bliku++;
      predCart = c;
    }
    const AnimKonzole po = ModelKonzole::anim16(4.0f, 5.f);
    over(z.posuvnik > 0.99f && z.led[0] == 1.f && bliku == 3 && po.led[1] > 0.4f && po.led[1] < 0.7f && po.led[0] == 1.f,
         "4) N&P 16: po zasunuti vypinac doprava, POWER sviti, CART zablika " + std::to_string(bliku) + "x a pak sviti slabe (" + f2(po.led[1]) + ")");
    // geometrie: zasunuta cartridge nad dnem sterbiny (1,9 cm), venku nad zlatym rameckem (vez 7,2 + 0,13)
    ModelKonzole m;     // jen cisla kloubu (bez GPU se site nestavi)
    const float spodekVlozene = m.vezY + ModelKonzole::anim16(5.f, 0.f).kazetaY;
    float spodekVenku = 99.f;
    for (float t = 0.f; t < 10.f; t += 0.1f) spodekVenku = std::min(spodekVenku, m.vezY + ModelKonzole::anim16(-1.f, t).kazetaY);
    over(spodekVlozene > 1.95f && spodekVenku > m.vezY + 0.13f + 2.0f,
         "5) N&P 16: zasunuta cartridge konci " + f2(spodekVlozene - 1.9f) + " cm nad dnem sterbiny, vznasejici se " + f2(spodekVenku - m.vezY - 0.13f) +
             " cm nad rameckem");
  }
  // ---------------- N&P 32 ----------------
  {
    bool venkuOk = true;
    for (float t = 0.f; t < 10.f; t += 0.37f) {
      const AnimKonzole a = ModelKonzole::anim32(-1.f, t);
      if (a.viko != 1.f || a.diskY < 5.5f || a.led[0] != 0.f || a.rozmaz != 0.f) venkuOk = false;
    }
    over(venkuOk, "6) N&P 32 mimo stred menu: viko otevrene, disk nad mechanikou (>= 5,5 cm), vypnuto, disk se jen pomalu otaci");
    const AnimKonzole d = ModelKonzole::anim32(0.76f, 5.f), v0 = ModelKonzole::anim32(0.84f, 5.f), vp = ModelKonzole::anim32(1.07f, 5.f),
                      vz = ModelKonzole::anim32(1.6f, 5.f);
    over(d.diskY < 0.12f && v0.viko == 1.f && vp.viko > 0.2f && vp.viko < 0.95f && vz.viko == 0.f,
         "7) N&P 32: disk dosedne na hridel (" + f2(d.diskY) + " cm), pak se viko zavre (" + f2(v0.viko) + " -> " + f2(vp.viko) + " -> " + f2(vz.viko) + ")");
    // roztoceni: rychlost roste, pak je stala; rozmazani potisku roste s rychlosti
    auto rychlost = [](float tv) { return (ModelKonzole::anim32(tv + 0.01f, 5.f).uhelDisku - ModelKonzole::anim32(tv, 5.f).uhelDisku) / 0.01f; };
    const float w1 = rychlost(1.7f), w2 = rychlost(2.4f), w3 = rychlost(4.0f), w4 = rychlost(6.0f);
    const float r1 = ModelKonzole::anim32(1.7f, 5.f).rozmaz, r3 = ModelKonzole::anim32(4.0f, 5.f).rozmaz;
    over(w1 > 0.5f && w2 > w1 * 1.5f && std::fabs(w3 - w4) < 0.05f && w3 > 9.f && r1 > 0.f && r3 > r1 && r3 <= 0.55f,
         "8) N&P 32: disk se roztoci (" + f2(w1) + " -> " + f2(w2) + " -> " + f2(w3) + " rad/s, pak stale), potisk se rozmaze (" + f2(r1) + " -> " + f2(r3) +
             " rad)");
    // kontrolky: POWER, pak rada 4 po sobe, pas a prstenec okenka svitiji
    float zac[4] = {-1, -1, -1, -1};
    for (float tv = 1.4f; tv < 3.f; tv += 0.005f) {
      const AnimKonzole a = ModelKonzole::anim32(tv, 5.f);
      for (int i = 0; i < 4; i++) if (zac[i] < 0 && a.led[4 + i] > 0.5f) zac[i] = tv;
    }
    const AnimKonzole s = ModelKonzole::anim32(5.f, 5.f);
    over(zac[0] > 0 && zac[0] < zac[1] && zac[1] < zac[2] && zac[2] < zac[3] && s.led[0] == 1.f && s.led[2] > 0.5f && s.led[3] > 0.4f,
         "9) N&P 32: POWER sviti, rada kontrolek se rozsviti po sobe (" + f2(zac[0]) + ", " + f2(zac[1]) + ", " + f2(zac[2]) + ", " + f2(zac[3]) +
             " s), pas dole a prstenec okenka sviti");
    // geometrie: disk nad mechanikou nenarazi do otevreneho vika (rovina spodku vika)
    ModelKonzole m;                       // klouby jako po postav32 (vychozi hodnoty)
    const M4 mv = m.maticeVika(1.f);
    const V3 a0 = ModelKonzole::bod(mv, V3(0.f, 4.9f, -9.6f)), a1 = ModelKonzole::bod(mv, V3(10.f, 4.9f, -9.6f)), a2 = ModelKonzole::bod(mv, V3(0.f, 4.9f, 3.6f));
    const V3 n = norm(cross(a1 - a0, a2 - a0));
    float nejbliz = 1e9f;
    for (float t = 0.f; t < 10.f; t += 0.13f) {
      const M4 md = m.maticeDisku(ModelKonzole::anim32(-1.f, t));
      for (int k = 0; k < 64; k++) {
        const float u = k * 6.2831853f / 64.f;
        const V3 p = ModelKonzole::bod(md, V3(std::cos(u) * 6.f, 0.f, std::sin(u) * 6.f));
        if (p.y > m.vikoOsaY + 0.3f) nejbliz = std::min(nejbliz, dot(p - a0, n));   // viko je jen nad pantem; kladne = pred vikem
      }
    }
    // pri padani disku (viko jeste otevrene)
    for (float tv = 0.f; tv < 0.84f; tv += 0.02f) {
      const M4 md = m.maticeDisku(ModelKonzole::anim32(tv, 5.f));
      for (int k = 0; k < 64; k++) {
        const float u = k * 6.2831853f / 64.f;
        const V3 p = ModelKonzole::bod(md, V3(std::cos(u) * 6.f, 0.f, std::sin(u) * 6.f));
        if (p.y > m.vikoOsaY + 0.3f) nejbliz = std::min(nejbliz, dot(p - a0, n));
      }
    }
    over(nejbliz > 0.4f, "10) N&P 32: disk nad mechanikou i pri padani nenarazi do otevreneho vika (nejbliz " + f2(nejbliz) + " cm)");
  }
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
