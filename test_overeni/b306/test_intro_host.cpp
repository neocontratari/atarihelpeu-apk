// test_overeni/b306/test_intro_host.cpp
// ====================================================================
//  B306: SKUTECNY nap_intro_gl.cpp (uvodni demo) na PC - zvuk prehrava C++.
//  (plocha telefonu = pbuffer pres Mesa, assety ze souboru, OpenSL ES = "zvukova
//   karta" z tv_intro.cpp: vlakno, ktere si kazdych 23,2 ms vezme blok 1024 ramcu,
//   zavola zpetne volani appky a GetPosition vraci, kolik uz prehrala)
//
//  1) OpenSL (B306): introZvukStartNative -> C++ prehrava, do reproduktoru jde
//     PRESNE stopa dema (bit po bitu), pred pripravou ticho
//  2) cas obrazu (introCasMs) jde za tim, co OpenSL prehral (GetPosition)
//  3) zastaveni: prehravac zniceny, zpetna volani skoncila, vlakna konec
//  4) novy start po zastaveni: cas zase od nuly (stary prehravac nesmi plest cas)
//  5) zaloha Java AudioTrack (introNaplnZvuk / introCasZvuku) dal funguje jako v B304
//
//   g++ -std=c++17 -O2 -pthread -Istub -I../b304/stub -I../b291/stub -I$INC -I$JAVA_HOME/include
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/tvproud -o test_intro_host test_intro_host.cpp
//       -l:libEGL.so.1 -l:libGLESv2.so.2
// ====================================================================
#include "tv_intro.cpp"
#include <cmath>

struct FakePole { std::vector<jshort> s; };
static jsize JNICALL hGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakePole *)a)->s.size(); }
static jshort *JNICALL hGetShortArrayElements(JNIEnv *, jshortArray a, jboolean *) { return ((FakePole *)a)->s.data(); }
static void JNICALL hReleaseShortArrayElements(JNIEnv *, jshortArray, jshort *, jint) {}

static int g_ok = 0, g_ch = 0;
static void kontrola(bool ok, const char *co, const std::string &detail = "") {
  std::printf("%s  %s%s%s\n", ok ? "OK   " : "CHYBA", co, detail.empty() ? "" : "  -- ", detail.c_str());
  (ok ? g_ok : g_ch)++;
}
static double tedS() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

int main() {
  char b[300];
  // ===== 1) + 2) OpenSL =====
  karta::nahrano.clear();
  karta::nahravat = true;
  const int nat = intro_start("../../app/src/main/assets/");
  kontrola(nat == 1, "1) introZvukStartNative: zvuk prehrava C++ (OpenSL ES)");
  const double t0 = tedS();
  while (!intro_pripraveno() && tedS() - t0 < 20) std::this_thread::sleep_for(std::chrono::milliseconds(20));
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
  const long long zac = g_zacatekStopy.load();
  intro_stop();
  {
    napdemo::zv::Stopa st;
    st.vyrob();
    const std::vector<int16_t> &s = karta::nahrano;
    long long porovnano = 0, rozdil = 0;
    for (long long i = 0; i < st.celkem() && (zac + i) * 2 + 1 < (long long)s.size(); i++) {
      porovnano++;
      if (s[(size_t)(zac + i) * 2] != st.data()[i * 2] || s[(size_t)(zac + i) * 2 + 1] != st.data()[i * 2 + 1]) rozdil++;
    }
    bool ticho = true;
    for (long long i = 0; i < zac * 2 && i < (long long)s.size(); i++) if (s[(size_t)i]) { ticho = false; break; }
    std::snprintf(b, sizeof b, "do OpenSL zarazeno %zu ramcu, stopa od ramce %lld, porovnano %lld (%.1f s), rozdilnych %lld, pred stopou ticho %s",
                  s.size() / 2, zac, porovnano, porovnano / 44100.0, rozdil, ticho ? "ano" : "NE");
    kontrola(porovnano > 44100 * 6 && rozdil == 0 && ticho && zac > 0, "1) do reproduktoru jde PRESNE stopa dema (bit po bitu), pred pripravou ticho", b);
  }
  std::snprintf(b, sizeof b, "prehravacu vytvoreno %d, zniceno %d, okno refs %d, vlakno kresleni %s", karta::vytvoreno.load(), karta::zniceno.load(),
                g_okno.refs, g_vlakno.joinable() ? "BEZI" : "konec");
  kontrola(karta::vytvoreno.load() == 1 && karta::zniceno.load() == 1 && !karta::hraje.load() && g_okno.refs == 1 && !g_vlakno.joinable(),
           "3) zastaveni: prehravac OpenSL zniceny, zpetna volani skoncila, okno uvolnene", b);
  // ===== 4) novy start - cas od nuly =====
  intro_start("../../app/src/main/assets/");
  const double t1 = tedS();
  while (!intro_pripraveno() && tedS() - t1 < 20) std::this_thread::sleep_for(std::chrono::milliseconds(20));
  std::this_thread::sleep_for(std::chrono::milliseconds(1500));
  const int cas2 = intro_cas_ms();
  std::snprintf(b, sizeof b, "cas dema 1,5 s po pripraveni: %d ms, prehravacu %d / zniceno %d", cas2, karta::vytvoreno.load(), karta::zniceno.load());
  kontrola(cas2 > 300 && cas2 < 2500 && karta::vytvoreno.load() == 2, "4) druhy start: novy prehravac, cas dema zase od zacatku", b);
  intro_stop();

  // ===== 5) zaloha: Java AudioTrack (bez OpenSL) jako v B304 =====
  JNINativeInterface_ tab = g_tbl;
  tab.GetArrayLength = hGetArrayLength;
  tab.GetShortArrayElements = hGetShortArrayElements;
  tab.ReleaseShortArrayElements = hReleaseShortArrayElements;
  JNIEnv_ envS;
  envS.functions = &tab;
  JNIEnv *env = &envS;
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(env, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(env, nullptr, (jobject)&g_plocha);
  FakePole zvuk;
  zvuk.s.assign(2048, 0);
  std::vector<jshort> slyseno;
  long long zapsano = 0;
  double hlava = 0, posl = tedS();
  const double t2 = tedS();
  double maxO = 0;
  int nO = 0;
  while (tedS() - t2 < 8.0) {
    const int r = Java_eu_atarihelp_emu10_NativeIntroBridge_introNaplnZvuk(env, nullptr, (jshortArray)&zvuk, 1024);
    if (r < 0) break;
    slyseno.insert(slyseno.end(), zvuk.s.begin(), zvuk.s.end());
    zapsano += r;
    for (;;) {                                         // blokujici zapis AudioTracku (buffer ~100 ms)
      const double ted = tedS();
      hlava += (ted - posl) * 44100.0;
      posl = ted;
      if (hlava > zapsano) hlava = (double)zapsano;
      if (zapsano - hlava <= 4410) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    Java_eu_atarihelp_emu10_NativeIntroBridge_introCasZvuku(env, nullptr, (jlong)hlava);
    const long long z = g_zacatekStopy.load();
    if (z >= 0 && hlava - z > 44100) { maxO = std::max(maxO, std::fabs(intro_cas_ms() / 1000.0 - (hlava - z) / 44100.0)); nO++; }
  }
  const long long z5 = g_zacatekStopy.load();
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(env, nullptr, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStop(env, nullptr);
  {
    napdemo::zv::Stopa st;
    st.vyrob();
    long long porovnano = 0, rozdil = 0;
    for (long long i = 0; i < st.celkem() && (z5 + i) * 2 + 1 < (long long)slyseno.size(); i++) {
      porovnano++;
      if (slyseno[(size_t)(z5 + i) * 2] != st.data()[i * 2] || slyseno[(size_t)(z5 + i) * 2 + 1] != st.data()[i * 2 + 1]) rozdil++;
    }
    std::snprintf(b, sizeof b, "porovnano %lld ramcu, rozdilnych %lld, cas obrazu proti zvuku nejvic %.0f ms (%d mereni)", porovnano, rozdil, maxO * 1000, nO);
    kontrola(porovnano > 44100 * 5 && rozdil == 0 && nO > 10 && maxO < 0.35, "5) zaloha Java AudioTrack: presne stopa, cas obrazu za zvukem (jako B304)", b);
  }
  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_ch);
  return g_ch ? 1 : 0;
}
