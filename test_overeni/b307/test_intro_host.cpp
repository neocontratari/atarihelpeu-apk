// test_overeni/b307/test_intro_host.cpp
// ====================================================================
//  B307: SKUTECNY nap_intro_gl.cpp (uvodni demo) na PC - tak, jak ho od B307
//  vola Java (introStart pusti i zvuk, introStop vse zastavi, introDotyk).
//  (plocha telefonu = pbuffer pres Mesa, assety ze souboru, OpenSL ES = "zvukova
//   karta" z tv_intro.cpp: vlakno, ktere si kazdych 23,2 ms vezme blok 1024 ramcu,
//   zavola zpetne volani appky a GetPosition vraci, kolik uz prehrala)
//
//  1) introStart: zvuk pusti C++ samo (OpenSL ES) a zvuk Reneho hry je ve stope;
//     GL vlakno nacte zabery Pitt-Kitt (do logu appky "zabery Pitt-Kitt ano")
//  2) cas obrazu (introCasMs) jde za tim, co OpenSL prehral (GetPosition)
//  3) dotyk: o tlacitku SKIP rozhoduje C++ - stred tlacitka = SKIP, jinde = jinam,
//     i kdyz Java posle body v jine velikosti, nez je plocha; hned po startu nic
//  4) zastaveni (introStop): prehravac zniceny, zpetna volani skoncila, okno
//     uvolnene; do reproduktoru sla PRESNE stopa (bit po bitu), pred pripravou ticho
//  5) novy start: cas zase od nuly, dotyk hned po startu se nepocita
//  6) CELA stopa (45,6 s; karta 8x rychleji, NAP_RYCHLOST): bit po bitu = stopa dema + zvuk
//     hry Pitt-Kitt od 33,40 s (jizda Las Vegas), zvuk hry tam opravdu je, nic
//     neorezava; demo samo dobehne (introHotovo)
//  7) OpenSL nejde: demo bezi podle hodin bez zvuku, zadny prehravac
//  8) chybi klip: demo jede dal, stopa bez zvuku hry (bit po bitu), v logu "NE"
//
//   g++ -std=c++17 -O2 -pthread -I../b306/stub -I../b304/stub -I../b291/stub -I$INC -I$JAVA_HOME/include
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/tvproud -o test_intro_host test_intro_host.cpp
//       -l:libEGL.so.1 -l:libGLESv2.so.2
//   ./test_intro_host [ADRESAR_BEZ_KLIPU]   (adresar s design/nap_logo_original.png a bez intro/)
// ====================================================================
#include "tv_intro.cpp"
#include <cmath>
#include <sys/stat.h>

static int g_ok = 0, g_ch = 0;
static void kontrola(bool ok, const char *co, const std::string &detail = "") {
  std::printf("%s  %s%s%s\n", ok ? "OK   " : "CHYBA", co, detail.empty() ? "" : "  -- ", detail.c_str());
  std::fflush(stdout);
  (ok ? g_ok : g_ch)++;
}
static double tedS() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }
static bool cekej(bool (*f)(), double s) {
  const double t0 = tedS();
  while (!f() && tedS() - t0 < s) std::this_thread::sleep_for(std::chrono::milliseconds(20));
  return f();
}
static const std::string ASSETY = "../../app/src/main/assets/";

// porovnani toho, co slo do reproduktoru (od ramce zac), se stopou
struct Shoda { long long porovnano = 0, rozdil = 0; bool ticho = true; };
static Shoda porovnej(const std::vector<int16_t> &s, long long zac, const napdemo::zv::Stopa &st) {
  Shoda r;
  for (long long i = 0; i < st.celkem() && (zac + i) * 2 + 1 < (long long)s.size(); i++) {
    r.porovnano++;
    if (s[(size_t)(zac + i) * 2] != st.data()[i * 2] || s[(size_t)(zac + i) * 2 + 1] != st.data()[i * 2 + 1]) r.rozdil++;
  }
  for (long long i = 0; i < zac * 2 && i < (long long)s.size(); i++) if (s[(size_t)i]) { r.ticho = false; break; }
  return r;
}

int main(int argc, char **argv) {
  char b[400];
  const std::string bezKlipu = argc > 1 ? std::string(argv[1]) + "/" : "";
  // karta v testech 6 a 8 bezi rychleji nez realny cas (pod TSan se stopa pocita pomaleji -> NAP_RYCHLOST=2)
  const int rychle = std::max(1, std::getenv("NAP_RYCHLOST") ? std::atoi(std::getenv("NAP_RYCHLOST")) : 8);

  // ===== 1) + 2) + 3) + 4) =====
  karta::nahrano.clear();
  karta::nahravat = true;
  const int st1 = intro_start(ASSETY.c_str());
  kontrola(st1 == 3, "1) introStart: zvuk pusti C++ samo (OpenSL ES) a zvuk Reneho hry je ve stope", "bity " + std::to_string(st1));
  cekej([] { return intro_pripraveno() != 0; }, 20);
  {
    const std::string info = intro_info();
    kontrola(intro_klip_obraz() == 1 && info.find("zabery Pitt-Kitt ano, zvuk hry ano") != std::string::npos,
             "1) GL vlakno nacetlo zabery Pitt-Kitt (do logu appky)", info.substr(0, 160));
  }
  // 3) dotyk - plocha 360 x 780 (telefon na vysku); prvnich 0,7 s po startu se dotyk nepocita
  std::this_thread::sleep_for(std::chrono::milliseconds(800));
  {
    float x0, y0, x1, y1;
    napdemo::Demo::skipObd(g_okno.w, g_okno.h, x0, y0, x1, y1);
    const float cx = (x0 + x1) * 0.5f, cy = (y0 + y1) * 0.5f;
    const int a = intro_dotyk(cx, cy, g_okno.w, g_okno.h);
    const int c = intro_dotyk(g_okno.w * 0.5f, g_okno.h * 0.5f, g_okno.w, g_okno.h);
    const int d = intro_dotyk(cx * 2, cy * 2, g_okno.w * 2, g_okno.h * 2);          // Java: view 2x vetsi nez plocha
    const int e = intro_dotyk(x0 - 0.5f * g_okno.w * 0.03f, cy, g_okno.w, g_okno.h); // kousek vedle (rezerva pro prst)
    const int f = intro_dotyk(x0 - 0.06f * g_okno.w, cy, g_okno.w, g_okno.h);        // uz mimo
    const int g = intro_dotyk(cx, y1 + 0.05f * g_okno.w, g_okno.w, g_okno.h);        // pod tlacitkem
    std::snprintf(b, sizeof b, "tlacitko [%.0f,%.0f]-[%.0f,%.0f]: stred %d, stred plochy %d, view 2x %d, rezerva %d, vedle %d, pod %d",
                  x0, y0, x1, y1, a, c, d, e, f, g);
    kontrola(a == 1 && c == 2 && d == 1 && e == 1 && f == 2 && g == 2, "3) dotyk: o tlacitku SKIP rozhoduje C++ (1 = SKIP, 2 = jinam)", b);
  }
  double maxOdch = 0, sum = 0;
  int n = 0;
  for (int i = 0; i < 40; i++) {                       // 8 s, kazdych 200 ms
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    const long long zac = g_zacatekStopy.load(), preh = karta::prehrano.load();
    if (zac < 0 || preh <= zac) continue;
    const double ocek = (preh - zac) / 44100.0, obraz = intro_cas_ms() / 1000.0;
    if (ocek > 1.0) { const double o = std::fabs(obraz - ocek); maxOdch = std::max(maxOdch, o); sum += o; n++; }
  }
  std::snprintf(b, sizeof b, "prumer %.0f ms, nejvic %.0f ms (%d mereni; blok OpenSL = 23 ms)", n ? sum / n * 1000 : -1.0, maxOdch * 1000, n);
  kontrola(n > 20 && maxOdch < 0.12 && sum / std::max(1, n) < 0.05, "2) cas obrazu jde za tim, co OpenSL prehral (GetPosition)", b);
  karta::nahravat = false;
  long long zac = g_zacatekStopy.load();
  intro_stop();
  napdemo::zv::Stopa ref;                                   // stopa + zvuk hry (zadani B307)
  const bool refHra = referencniStopa(ref, ASSETY);
  napdemo::zv::Stopa bez;                                   // stopa bez zvuku hry (jako B306)
  bez.vyrob();
  {
    const Shoda s = porovnej(karta::nahrano, zac, ref);
    std::snprintf(b, sizeof b, "do OpenSL zarazeno %zu ramcu, stopa od ramce %lld, porovnano %lld (%.1f s), rozdilnych %lld, pred stopou ticho %s",
                  karta::nahrano.size() / 2, zac, s.porovnano, s.porovnano / 44100.0, s.rozdil, s.ticho ? "ano" : "NE");
    kontrola(refHra && s.porovnano > 44100 * 6 && s.rozdil == 0 && s.ticho && zac > 0,
             "4) do reproduktoru jde PRESNE stopa dema (bit po bitu), pred pripravou ticho", b);
  }
  std::snprintf(b, sizeof b, "prehravacu vytvoreno %d, zniceno %d, okno refs %d, vlakno kresleni %s, assety otevreno %d / zavreno %d",
                karta::vytvoreno.load(), karta::zniceno.load(), g_okno.refs, g_vlakno.joinable() ? "BEZI" : "konec",
                g_assetuOtevreno.load(), g_assetuZavreno.load());
  kontrola(karta::vytvoreno.load() == 1 && karta::zniceno.load() == 1 && !karta::hraje.load() && g_okno.refs == 1 && !g_vlakno.joinable() &&
               g_assetuOtevreno.load() == g_assetuZavreno.load(),
           "4) introStop: prehravac OpenSL zniceny, zpetna volani skoncila, okno uvolnene, assety zavrene", b);

  // ===== 5) novy start - cas od nuly, dotyk hned po startu nic =====
  intro_start(ASSETY.c_str());
  const int hned = intro_dotyk(g_okno.w * 0.5f, g_okno.h * 0.5f, g_okno.w, g_okno.h);
  cekej([] { return intro_pripraveno() != 0; }, 20);
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  const int cas2 = intro_cas_ms();
  std::snprintf(b, sizeof b, "dotyk hned po startu %d, cas dema 1,5 s po pripraveni: %d ms, prehravacu %d / zniceno %d", hned, cas2,
                karta::vytvoreno.load(), karta::zniceno.load());
  kontrola(hned == 0 && cas2 > 300 && cas2 < 2500 && karta::vytvoreno.load() == 2 && karta::zniceno.load() == 1,
           "5) druhy start: novy prehravac, cas dema zase od zacatku, prst ze spusteni nepreskoci", b);
  intro_stop();

  // ===== 6) CELA stopa vc. zvuku hry (karta 8x rychleji) =====
  karta::rychlost = rychle;
  karta::nahrano.clear();
  karta::nahravat = true;
  const int st6 = intro_start(ASSETY.c_str());
  cekej([] { return intro_pripraveno() != 0; }, 20);
  const double t6 = tedS();
  while (tedS() - t6 < 60.0 / rychle + 10) {
    const long long z = g_zacatekStopy.load();
    if (z >= 0 && karta::prehrano.load() > z + ref.celkem() + 44100) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  const bool hotovo6 = intro_hotovo() != 0;
  karta::nahravat = false;
  zac = g_zacatekStopy.load();
  intro_stop();
  karta::rychlost = 1;
  {
    const Shoda s = porovnej(karta::nahrano, zac, ref);
    std::snprintf(b, sizeof b, "bity %d, porovnano %lld ramcu (%.2f s z %.2f s), rozdilnych %lld, demo dobehlo %s", st6, s.porovnano,
                  s.porovnano / 44100.0, ref.celkem() / 44100.0, s.rozdil, hotovo6 ? "ano" : "NE");
    kontrola(st6 == 3 && s.porovnano == ref.celkem() && s.rozdil == 0 && hotovo6,
             "6) CELA stopa do reproduktoru bit po bitu = stopa dema + zvuk hry Pitt-Kitt; demo samo dobehne", b);
    // zvuk hry: od jizdy (33,40 s) se stopa s hrou lisi od stopy bez hry; pred ni vubec,
    // po konci jizdy (34,25 s) uz jen dozvuk (sal 1,25 s), ktery doznie
    const long long a0 = (long long)(napdemo::cas::ATARI_JIZDA * 44100), a1 = (long long)(napdemo::cas::STRIH_16 * 44100);
    double eh = 0, ej = 0, d1 = 0, d2 = 0;
    long long orez = 0, pred = 0, posledni = -1;
    for (long long i = 0; i < ref.celkem(); i++) {
      const double dl = (double)ref.data()[i * 2] - bez.data()[i * 2];
      if (dl != 0) { if (i < a0) pred++; posledni = i; }
      if (i >= a0 && i < a1) { eh += dl * dl; ej += (double)ref.data()[i * 2] * ref.data()[i * 2]; }
      if (i >= a1 && i < a1 + 22050) d1 += dl * dl;                      // dozvuk prvni pul vteriny
      if (i >= a1 + 88200 && i < a1 + 110250) d2 += dl * dl;             // dozvuk po 2 s
      if (std::abs(ref.data()[i * 2]) >= 32767 || std::abs(ref.data()[i * 2 + 1]) >= 32767) orez++;
    }
    const double rmsHra = std::sqrt(eh / std::max(1LL, a1 - a0)), rmsCelk = std::sqrt(ej / std::max(1LL, a1 - a0));
    const double r1 = std::sqrt(d1 / 22050), r2 = std::sqrt(d2 / 22050);
    std::snprintf(b, sizeof b, "zvuk hry 33,40-34,25 s: RMS %.0f (%.1f dBFS), cela stopa tam %.1f dBFS; pred jizdou rozdilnych vzorku %lld; "
                  "dozvuk po jizde RMS %.0f -> %.1f za 2 s, posledni rozdil v %.2f s; na hranici (orez) %lld",
                  rmsHra, 20 * std::log10(std::max(1.0, rmsHra) / 32768.0), 20 * std::log10(std::max(1.0, rmsCelk) / 32768.0), pred,
                  r1, r2, posledni / 44100.0, orez);
    kontrola(rmsHra > 300 && pred == 0 && r2 < r1 * 0.2 && orez == 0,
             "6) zvuk hry Pitt-Kitt je ve stope od jizdy Las Vegas (pak jen doznivajici dozvuk), nic neorezava", b);
  }

  // ===== 7) OpenSL nejde =====
  karta::selhat = true;
  const int st7 = intro_start(ASSETY.c_str());
  cekej([] { return intro_pripraveno() != 0; }, 20);
  std::this_thread::sleep_for(std::chrono::milliseconds(1100));
  const int c7a = intro_cas_ms();
  std::this_thread::sleep_for(std::chrono::milliseconds(2000));
  const int c7b = intro_cas_ms();
  intro_stop();
  karta::selhat = false;
  std::snprintf(b, sizeof b, "bity %d, cas dema %d -> %d ms za 2 s, prehravacu vytvoreno %d (porad 3), okno refs %d", st7, c7a, c7b,
                karta::vytvoreno.load(), g_okno.refs);
  kontrola((st7 & 1) == 0 && c7b - c7a > 1500 && c7b - c7a < 2500 && karta::vytvoreno.load() == 3 && g_okno.refs == 1,
           "7) OpenSL nejde: demo bezi podle hodin (bez zvuku), zadny prehravac, nic nespadne", b);

  // ===== 8) chybi klip (asset intro/pittkitt.klip) =====
  struct stat sb;
  if (!bezKlipu.empty() && stat((bezKlipu + "design/nap_logo_original.png").c_str(), &sb) == 0 &&
      stat((bezKlipu + "intro/pittkitt.klip").c_str(), &sb) != 0) {
    karta::rychlost = rychle;
    karta::nahrano.clear();
    karta::nahravat = true;
    const int st8 = intro_start(bezKlipu.c_str());
    cekej([] { return intro_pripraveno() != 0; }, 20);
    const double t8 = tedS();
    while (tedS() - t8 < 60.0 / rychle + 10) {
      const long long z = g_zacatekStopy.load();
      if (z >= 0 && karta::prehrano.load() > z + bez.celkem() + 44100) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    karta::nahravat = false;
    zac = g_zacatekStopy.load();
    const std::string info = intro_info();
    const int obraz8 = intro_klip_obraz();
    intro_stop();
    karta::rychlost = 1;
    g_assety = ASSETY;
    const Shoda s = porovnej(karta::nahrano, zac, bez);
    std::snprintf(b, sizeof b, "bity %d, zabery %d, porovnano %lld ramcu (%.2f s), rozdilnych %lld; log: %s", st8, obraz8, s.porovnano,
                  s.porovnano / 44100.0, s.rozdil, info.substr(info.find("zabery"), 40).c_str());
    kontrola(st8 == 1 && obraz8 == 0 && s.porovnano == bez.celkem() && s.rozdil == 0 && info.find("zabery Pitt-Kitt NE, zvuk hry NE") != std::string::npos,
             "8) chybi klip: demo jede dal, stopa bez zvuku hry (bit po bitu), v logu appky NE", b);
  } else {
    kontrola(false, "8) chybi klip: zadej adresar s design/nap_logo_original.png a bez intro/ (argument)");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_ch);
  return g_ch ? 1 : 0;
}
