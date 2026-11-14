// web/menu/nap_menu_web.cpp
// ====================================================================
//  B322: HLAVNI MENU N&P VISION NA WEBU atarihelp.eu
//
//  Rene: "ty html predelej, ja tam nahazel gif, ale ty mas celou grafiku
//  a animaci v kodu apky".
//
//  PRESNE ten samy C++ jako hlavni menu v appce (app/src/main/cpp/menu:
//  karty z karty.txt, karusel, 3D Atari 130XE, konzole N&P 16 a N&P 32,
//  kazeta, dzubox, web, help, pisma Chakra Petch, logo N&P) - prelozeny
//  Emscriptenem do WebAssembly, kresli v prohlizeci pres WebGL 2 (jinak 1).
//  Tady je jen "most" misto Androidu (nap_menu_gl.cpp): kontext WebGL na
//  platne stranky, smycka snimku (requestAnimationFrame) a prst / mys.
//
//  Na strance muze byt vic panelu (kazdy = jedno platno, vlastni kontext,
//  vlastni karusel), kazdy zacina na jine karte. Kresli se jen viditelne
//  panely (stranka hlasi napViditelny podle IntersectionObserver).
//
//  JavaScript (stranka):
//    napPanel(selektor, karta, en, w, h) -> index panelu (-1 = WebGL nejde)
//    napVelikost(i, w, h), napViditelny(i, 0|1)
//    napDotyk(i, akce 0 dolu / 1 tah / 2 nahoru / 3 zruseno, x, y, casMs)
//      -> 0 nic, 1000 + k spustit kartu k (po animaci 0,3 s), 2000 kolecko
//    napKlavesa(i, 1 vlevo / 2 vpravo / 3 potvrd), napPust(i) (po spusteni zpet)
//    napKarta(k) -> "id|cil", napPocet()
//  Preklad: web/menu/vyrob.sh (emcc, soubory assets se vlozi do .js).
// ====================================================================
#include <emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "../../app/src/main/cpp/vendor/stb/stb_truetype.h"
#undef STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#include "../../app/src/main/cpp/vendor/stb/stb_image.h"
#undef STB_IMAGE_IMPLEMENTATION

#include "../../app/src/main/cpp/menu/nap_menu_kresba.h"

using namespace napmenu;

namespace {

std::vector<unsigned char> nactiSoubor(const std::string &cesta) {
  std::vector<unsigned char> v;
  FILE *f = std::fopen(("/assets/" + cesta).c_str(), "rb");
  if (!f) return v;
  std::fseek(f, 0, SEEK_END);
  long n = std::ftell(f);
  std::fseek(f, 0, SEEK_SET);
  if (n > 0) { v.resize((size_t)n); if (std::fread(v.data(), 1, (size_t)n, f) != (size_t)n) v.clear(); }
  std::fclose(f);
  return v;
}

double tedS() { return emscripten_get_now() / 1000.0; }

struct Panel {
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = 0;
  Kresba k;
  Karusel kar;
  StavMenu ef;
  CilMenu cil;
  int w = 0, h = 0;
  bool vid = true, ok = false, rozlozeno = false;
  int verze = 2;
};

std::vector<Panel *> g_panely;
std::vector<Karta> g_karty;
std::string g_kartaText;

bool aktivni(int i) { return i >= 0 && i < (int)g_panely.size() && g_panely[(size_t)i]->ok; }

int zpracujAkci(Panel &p, int a, double t) {
  if (a >= AKCE_KARTA && a < AKCE_NASTAVENI) {
    const int k = a - AKCE_KARTA;
    if (k < 0 || k >= (int)g_karty.size() || p.ef.spustena >= 0) return AKCE_NIC;
    p.ef.spustena = k;
    p.ef.tSpusteni = t;
    return a;
  }
  if (a == AKCE_NASTAVENI) {
    if (p.ef.spustena >= 0) return AKCE_NIC;
    p.ef.tOzub = t;
    return a;
  }
  return AKCE_NIC;
}

void snimek() {
  const double t = tedS();
  for (Panel *p : g_panely) {
    if (!p->ok || !p->vid || p->w <= 0 || p->h <= 0) continue;
    if (emscripten_webgl_make_context_current(p->ctx) != EMSCRIPTEN_RESULT_SUCCESS) continue;
    p->kar.krok(t);
    StavMenu s = p->ef;
    s.t = t;
    s.poz = p->kar.poz();
    s.pohyb = p->kar.stav() != Karusel::KLID || p->kar.prstDole();
    p->cil.w = p->w; p->cil.h = p->h; p->cil.tv = false; p->cil.hlavni = true; p->cil.fbo = 0;
    p->k.snimek(s, p->cil);
    p->kar.nastavRozlozeni(p->cil.roz);
    p->rozlozeno = true;
  }
}

}  // namespace

extern "C" {

EMSCRIPTEN_KEEPALIVE int napPocet() { return (int)g_karty.size(); }

EMSCRIPTEN_KEEPALIVE const char *napKarta(int k) {
  g_kartaText.clear();
  if (k >= 0 && k < (int)g_karty.size()) g_kartaText = g_karty[(size_t)k].id + "|" + g_karty[(size_t)k].cil;
  return g_kartaText.c_str();
}

/** Novy panel na platne (selektor "#id"), zacina na karte 'karta'. -1 = WebGL nejde. */
EMSCRIPTEN_KEEPALIVE int napPanel(const char *selektor, int karta, int en, int w, int h) {
  EmscriptenWebGLContextAttributes a;
  emscripten_webgl_init_context_attributes(&a);
  a.alpha = false; a.depth = true; a.stencil = false; a.antialias = true;
  a.premultipliedAlpha = false; a.preserveDrawingBuffer = false;
  a.powerPreference = EM_WEBGL_POWER_PREFERENCE_DEFAULT;
  a.majorVersion = 2; a.minorVersion = 0;
  int verze = 2;
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context(selektor, &a);
  if (ctx <= 0) { a.majorVersion = 1; verze = 1; ctx = emscripten_webgl_create_context(selektor, &a); }
  if (ctx <= 0) return -1;
  emscripten_webgl_make_context_current(ctx);
  Panel *p = new Panel();
  p->ctx = ctx;
  p->verze = verze;
  const char *ext = (const char *)glGetString(GL_EXTENSIONS);
  const bool npot = verze >= 2 || (ext && std::strstr(ext, "GL_OES_texture_npot"));
  std::vector<unsigned char> ls = nactiSoubor("design/nap_logo_original.png");
  int lw = 0, lh = 0, lc = 0;
  unsigned char *logo = ls.empty() ? nullptr : stbi_load_from_memory(ls.data(), (int)ls.size(), &lw, &lh, &lc, 4);
  std::vector<Karta> karty = g_karty;
  pouzijJazyk(karty, en != 0);
  p->k.nastavJazyk(en != 0);
  p->ok = p->k.init(logo, lw, lh, nactiSoubor, npot);
  if (logo) stbi_image_free(logo);
  if (!p->ok) { std::printf("NAP MENU: kresba se nepripravila (WebGL %d)\n%s", verze, p->k.log().c_str()); delete p; return -1; }
  p->k.nastavKarty(karty);
  p->k.nastavUroven(3);
  p->kar.nastavPocet((int)karty.size());
  p->kar.skoc(karta);
  p->kar.nastavPrah(10.f);
  p->ef.tPrichod = tedS();    // nastup karet jako v appce
  p->w = w; p->h = h;
  g_panely.push_back(p);
  std::printf("NAP MENU: panel %d na %s (WebGL %d, karet %d, karta %d)\n", (int)g_panely.size() - 1, selektor, verze, (int)karty.size(), karta);
  return (int)g_panely.size() - 1;
}

EMSCRIPTEN_KEEPALIVE void napVelikost(int i, int w, int h) {
  if (!aktivni(i)) return;
  g_panely[(size_t)i]->w = w;
  g_panely[(size_t)i]->h = h;
}

EMSCRIPTEN_KEEPALIVE void napViditelny(int i, int v) {
  if (aktivni(i)) g_panely[(size_t)i]->vid = v != 0;
}

/** Prst / mys v bodech platna (stejne jako NativeMenuBridge.menuDotyk v appce). */
EMSCRIPTEN_KEEPALIVE int napDotyk(int i, int akce, float x, float y, double casMs) {
  if (!aktivni(i)) return 0;
  Panel &p = *g_panely[(size_t)i];
  if (!p.rozlozeno || p.ef.spustena >= 0) return 0;
  const double t = tedS(), tUd = casMs * 1e-3;
  int a = AKCE_NIC;
  switch (akce) {
    case 0: p.kar.krok(t); p.kar.dolu(x, y, tUd); break;
    case 1: p.kar.tah(x, y, tUd); break;
    case 2: p.kar.krok(t); a = p.kar.nahoru(x, y, tUd); break;
    default: p.kar.zrus(); break;
  }
  return zpracujAkci(p, a, t);
}

EMSCRIPTEN_KEEPALIVE int napKlavesa(int i, int kl) {
  if (!aktivni(i)) return 0;
  Panel &p = *g_panely[(size_t)i];
  if (!p.rozlozeno || p.ef.spustena >= 0) return 0;
  const double t = tedS();
  p.kar.krok(t);
  return zpracujAkci(p, p.kar.klavesa(kl), t);
}

/** Po spusteni karty (stranka otevrela okno / odkaz): karta zpet na misto. */
EMSCRIPTEN_KEEPALIVE void napPust(int i) {
  if (!aktivni(i)) return;
  Panel &p = *g_panely[(size_t)i];
  p.ef.spustena = -1;
  p.ef.tSpusteni = -1;
  p.ef.tPrichod = tedS() - 0.45;
}

}  // extern "C"

int main() {
  std::vector<unsigned char> kt = nactiSoubor("menu/karty.txt");
  std::string chyby;
  g_karty = nactiKarty((const char *)kt.data(), kt.size(), &chyby);
  if (g_karty.empty()) g_karty = nactiKarty(KARTY_ZALOHA, std::strlen(KARTY_ZALOHA));
  std::printf("NAP MENU: %d karet z karty.txt\n", (int)g_karty.size());
  emscripten_set_main_loop(snimek, 0, 0);
  EM_ASM({ if (Module.napPripraveno) Module.napPripraveno(); });
  return 0;
}
