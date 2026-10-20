// test_overeni/b313/test_menu_host_b313.cpp
// ====================================================================
//  B313: SKUTECNY nap_menu_gl.cpp (hlavni menu) na PC - presne tak, jak ho
//  vola Java (NativeMenuBridge): assety + karty.txt, zamky, plocha, dotyk
//  (body view se prepoctou na plochu), klavesy, proud na TV, stary snimek pro TV,
//  vypnuti a znovu zapnuti plochy. Kopie ../b311/test_menu_host.cpp, B313 navic:
//   - menuNastav (Java: obnova displeje + kvalita TV): uroven se meri podle skutecnych
//     60 Hz ("z displeje"), obraz pro TV podle kvality 854x480 / 1024x576 / 1280x720
//     i za chodu (drive vzdy 854x480),
//   - stary prohlizec (menuGrabFrame): male pole -> zaporne cislo s rozmerem -> Java
//     zvetsi pole (jako grabFrameSafe v MainActivity) -> snimek 1280x720.
//  Plocha telefonu = pbuffer 360 x 780 pres Mesa (llvmpipe kresli na CPU - pomale,
//  na telefonu kresli GPU).
//
//   g++ -std=c++17 -O2 -pthread -I../b304/stub -I../b291/stub -I$INC -I$JAVA_HOME/include
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/tvproud -o test_menu_host_b313 test_menu_host_b313.cpp
//       -l:libEGL.so.1 -l:libGLESv2.so.2
// ====================================================================
#include "../b311/tv_menu.cpp"
static void menu_nastav(float hz, int k) { Java_eu_atarihelp_emu10_NativeMenuBridge_menuNastav(&g_env, nullptr, hz, k); }
#include <cmath>

static int g_ok = 0, g_ch = 0;
static void kontrola(bool ok, const char *co, const std::string &detail = "") {
  std::printf("%s  %s%s%s\n", ok ? "OK   " : "CHYBA", co, detail.empty() ? "" : "  -- ", detail.c_str());
  std::fflush(stdout);
  (ok ? g_ok : g_ch)++;
}
static double hodinyS() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static bool cekej(bool (*f)(), double s) {
  const double t0 = hodinyS();
  while (!f() && hodinyS() - t0 < s) std::this_thread::sleep_for(std::chrono::milliseconds(20));
  return f();
}
static void spi(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
// "uptimeMillis" pro udalosti dotyku (jen rozdily jsou dulezite)
static long long g_ud = 5000000;
static long long ud() { return g_ud; }

// tah prstem po bodech (jako MotionEvent: DOWN, MOVE..., UP), body view = w x h;
// drzetMs = prst se pred zvednutim zastavi (pomaly tah, ne svih)
static int tahni(float x0, float y0, float x1, float y1, int kroku, int msNaKrok, int w, int h, int drzetMs = 0) {
  menu_dotyk(0, x0, y0, ud(), w, h);
  for (int i = 1; i <= kroku; i++) {
    g_ud += msNaKrok;
    spi(msNaKrok);
    menu_dotyk(1, x0 + (x1 - x0) * i / kroku, y0 + (y1 - y0) * i / kroku, ud(), w, h);
  }
  for (int t = 0; t < drzetMs; t += 16) { g_ud += 16; spi(16); menu_dotyk(1, x1, y1, ud(), w, h); }
  g_ud += 4;
  return menu_dotyk(2, x1, y1, ud(), w, h);
}
static int klepni(float x, float y, int w, int h) {
  menu_dotyk(0, x, y, ud(), w, h);
  g_ud += 70;
  spi(70);
  return menu_dotyk(2, x, y, ud(), w, h);
}

int main() {
  char b[600];
  pripravEnv();
  const int W = g_okno.w, H = g_okno.h;
  const napmenu::Rozlozeni R = napmenu::spoctiRozlozeni(W, H, false, 7);

  // ===== 1) karty =====
  const int n = menu_assety("../../app/src/main/assets/");
  std::string k0 = menu_karta(0), k1 = menu_karta(1), k6 = menu_karta(6);
  std::snprintf(b, sizeof b, "karet %d: [%s] [%s] [%s]", n, k0.c_str(), k1.c_str(), k6.c_str());
  kontrola(n == 7 && k0 == "atari|ATARI 130XE|emu_atari_cpp/index.html|" && k1.find("|emu_sega/index.html|sega") != std::string::npos &&
               k6.find("HELP|napoveda/index.html|") != std::string::npos,
           "1) karty z assets/menu/karty.txt (Java dostane id, nazev, cil a zamek)", b);
  menu_zamek("sega", true, u8"Napiš v Atari ještě 45 řádků programu (počítá se při RUN nebo LIST).");
  menu_zamek("ps1", true, u8"Nejdřív odemkni Segu.");

  // ===== 2) plocha: vlakno kresli =====
  menu_prah(8.f);
  menu_ukaz(1);
  const double t0 = hodinyS();
  menu_plocha(true);
  const bool prip = cekej(menu_pripraveno, 60);
  const double priprava = hodinyS() - t0;
  spi(1500);
  const long long s1 = menu_snimku();
  spi(1000);
  const long long s2 = menu_snimku();
  const std::string info = menu_info();
  std::snprintf(b, sizeof b, "pripraveno za %.1f s (Mesa na CPU), snimku za 1 s: %lld, chyba %d; %s", priprava, s2 - s1, (int)menu_chyba(), info.substr(0, 230).c_str());
  kontrola(prip && !menu_chyba() && s2 > s1 && info.find("GPU") != std::string::npos && info.find("karet 7") != std::string::npos,
           "2) plocha: GL vlakno kresli menu (do logu appky GPU, priprava, rychlost)", b);
  const std::string lg = menu_log();
  kontrola(lg.find("karta atari: 3d:atari") != std::string::npos && lg.find("karta sega: 3d:nap16") != std::string::npos,
           "2) karty maji zive obrazky (3D Atari, N&P 16 ...)", lg.substr(0, 200));

  // ===== 3) tah prstem o kartu doleva (prst se zastavi) -> SEGA; rychly svih -> o dve dal =====
  const int a1 = tahni(300.f, R.cy, 300.f - 8.f - R.r0, R.cy, 30, 16, W, H, 120);
  spi(1200);
  std::snprintf(b, sizeof b, "akce %d, vybrana %d", a1, menu_vybrana());
  kontrola(a1 == 0 && menu_vybrana() == 1, "3) tah prstem o rozestup karet doleva (pomalu) = dalsi karta (SEGA)", b);
  tahni(300.f, R.cy, 300.f - 8.f - R.r0, R.cy, 12, 16, W, H);       // ~5 karet/s
  spi(1200);
  const int poSvihu = menu_vybrana();
  menu_klavesa(napmenu::KL_ZACATEK);
  spi(1000);
  menu_klavesa(napmenu::KL_VPRAVO);
  spi(1000);
  std::snprintf(b, sizeof b, "svih 5 karet/s z karty 1 -> %d, pak Home a sipka vpravo -> %d", poSvihu, menu_vybrana());
  kontrola(poSvihu == 3 && menu_vybrana() == 1, "3) rychly svih dojede dal (o 2 karty), Home + sipka zpet na SEGU", b);

  // ===== 4) klepnuti na zamcenou SEGU =====
  const int a2 = klepni(R.cx, R.cy, W, H);
  std::snprintf(b, sizeof b, "akce %d (3001 = zamcena karta 1)", a2);
  kontrola(a2 == napmenu::AKCE_ZAMCENO + 1, "4) klepnuti na zamcenou SEGU: nespusti, C++ ukaze proc (zatrese)", b);

  // ===== 5) dotyk v jine velikosti view (2x) - prepocet na plochu =====
  const int a3 = klepni((R.ozubX) * 2.f, R.ozubY * 2.f, W * 2, H * 2);
  std::snprintf(b, sizeof b, "kolecko klepnuto v view 2x vetsim: akce %d", a3);
  kontrola(a3 == napmenu::AKCE_NASTAVENI, "5) kolecko OPTIONS (body view 2x vetsi nez plocha - prepocet)", b);

  // ===== 6) klavesy: vlevo, Enter = spustit ATARI; behem spousteni nic dalsiho =====
  menu_klavesa(napmenu::KL_VLEVO);
  spi(1200);
  const int vyb = menu_vybrana();
  const int a4 = menu_klavesa(napmenu::KL_POTVRD);
  const int a5 = klepni(R.cx, R.cy, W, H);
  const int a6 = menu_klavesa(napmenu::KL_VPRAVO);
  std::snprintf(b, sizeof b, "po sipce vlevo vybrana %d, Enter %d, pak klepnuti %d a sipka %d (behem spousteni)", vyb, a4, a5, a6);
  kontrola(vyb == 0 && a4 == napmenu::AKCE_KARTA + 0 && a5 == 0 && a6 == 0, "6) klavesy: sipka vlevo, Enter = spustit ATARI; behem spousteni se nic dalsiho nespusti", b);
  menu_ukaz(0);
  const int a7 = klepni(R.cx, R.cy, W, H);
  std::snprintf(b, sizeof b, "po menuUkaz klepnuti na ATARI: %d", a7);
  kontrola(a7 == napmenu::AKCE_KARTA + 0, "6) menuUkaz (navrat do menu) spusteni zrusi - klepnuti zase spusti", b);
  menu_ukaz(0);

  // ===== 7) svih pres vsechny karty az na HELP =====
  tahni(330.f, R.cy, 30.f, R.cy, 6, 12, W, H);
  spi(1800);
  const int v7 = menu_vybrana();
  menu_klavesa(napmenu::KL_KONEC);
  spi(1200);
  std::snprintf(b, sizeof b, "po svihu %d, po End %d", v7, menu_vybrana());
  kontrola(v7 >= 2 && menu_vybrana() == 6, "7) svih jede pres vic karet, End = posledni karta HELP", b);

  // ===== 8) proud na TV / PC (/menu.stream) - velikost podle kvality TV (B313) =====
  {
    const int id = proud_otevri();
    // prvni paket, ktery ma rozmer w x h (po zmene kvality prijde do par snimku)
    auto cekejRozmer = [&](int cw, int ch, int &nB, int &ver, bool &znacka) {
      const double z = hodinyS();
      while (hodinyS() - z < 10.0) {
        std::vector<uint8_t> buf(8 * 1024 * 1024);
        const int r = proud_dalsi(id, buf, 250);
        if (r <= 16) continue;
        const uint8_t *p = buf.data() + 4;
        znacka = p[0] == 'N' && p[1] == 'A';
        ver = p[2];
        const int w = p[4] | (p[5] << 8), h = p[6] | (p[7] << 8);
        nB = r;
        if (w == cw && h == ch) return true;
      }
      return false;
    };
    int nB = 0, ver = 0;
    bool znacka = false;
    menu_nastav(60.f, 2);
    const bool v = cekejRozmer(1280, 720, nB, ver, znacka);
    const int st = proud_stav();
    menu_nastav(60.f, 1);
    const bool s1 = cekejRozmer(1024, 576, nB, ver, znacka);
    menu_nastav(60.f, 0);
    const bool s0 = cekejRozmer(854, 480, nB, ver, znacka);
    menu_nastav(60.f, 2);
    const bool v2 = cekejRozmer(1280, 720, nB, ver, znacka);
    proud_zavri(id);
    const std::string pl = proud_log();
    std::snprintf(b, sizeof b, "VYSOKA 1280x720 %s, STREDNI 1024x576 %s, NIZKA 854x480 %s, zpet VYSOKA %s; znacka %s, verze %d, stav %d; %s", v ? "ano" : "NE",
                  s1 ? "ano" : "NE", s0 ? "ano" : "NE", v2 ? "ano" : "NE", znacka ? "NA" : "?", ver, st, pl.substr(0, 120).c_str());
    kontrola(v && s1 && s0 && v2 && znacka && ver == 3 && (st & 3) == 3,
             "8) TV / PC: menu proudem z C++ (JPEG v3), velikost podle kvality TV z OPTIONS i za chodu (B313)", b);
  }
  // ===== 9) stary prohlizec: snimek pro Javu (male pole -> zvetsit, jako grabFrameSafe v Jave) =====
  {
    std::vector<jint> px(854 * 480);
    int r = menu_grab(px);
    int zvetseno = 0;
    for (int i = 0; i < 100 && r <= 0; i++) {
      if (r < 0) { px.assign((size_t)((-r) >> 16) * ((-r) & 0xFFFF), 0); zvetseno++; }   // jako Java: vetsi pole
      else spi(40);
      r = menu_grab(px);
    }
    long long jas = 0;
    for (jint v : px) jas += ((v >> 16) & 255) + ((v >> 8) & 255) + (v & 255);
    std::snprintf(b, sizeof b, "menuGrabFrame %d (%dx%d), pole zvetseno %dx, prumerny jas %.1f", r, r >> 16, r & 0xFFFF, zvetseno, jas / 3.0 / px.size());
    kontrola(r == ((1280 << 16) | 720) && zvetseno >= 1 && jas > 0, "9) stary prohlizec bez ReadableStream: snimek menu 1280x720 pro Javu (male pole se zvetsi)", b);
  }
  // ===== 9b) vykon: perioda displeje ze skutecnych 60 Hz (Java menuNastav), ne odhad =====
  {
    spi(1500);
    const std::string inf = menu_info();
    std::snprintf(b, sizeof b, "%s", inf.substr(inf.find("||") == std::string::npos ? 0 : inf.find("||")).substr(0, 200).c_str());
    kontrola(inf.find("16.7 ms = 60 Hz z displeje") != std::string::npos, "9b) vykon menu: perioda displeje z obnovovaci frekvence (60 Hz z Javy), ne odhad (B313)", b);
  }

  // ===== 10) plocha pryc a zpet: vlakno skonci, okno uvolnene; karta zustane =====
  const int pred = menu_vybrana();
  menu_plocha(false);
  std::snprintf(b, sizeof b, "okno refs %d, vlakno %s, assety otevreno %d / zavreno %d", g_okno.refs, g_vlakno.joinable() ? "BEZI" : "konec",
                g_assetuOtevreno.load(), g_assetuZavreno.load());
  kontrola(g_okno.refs == 1 && !g_vlakno.joinable() && g_assetuOtevreno.load() == g_assetuZavreno.load(), "10) plocha odpojena: vlakno skoncilo, okno a assety uvolnene", b);
  menu_ukaz(0);
  menu_plocha(true);
  const bool prip2 = cekej(menu_pripraveno, 60);
  spi(500);
  std::snprintf(b, sizeof b, "znovu pripraveno %s, vybrana pred %d / po %d", prip2 ? "ano" : "NE", pred, menu_vybrana());
  kontrola(prip2 && menu_vybrana() == pred, "10) plocha znovu (navrat z emulatoru): menu jede, porad na stejne karte", b);
  menu_plocha(false);

  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_ch);
  return g_ch ? 1 : 0;
}
