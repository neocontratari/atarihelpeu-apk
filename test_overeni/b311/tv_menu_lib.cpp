// test_overeni/b311/tv_menu_lib.cpp
// ====================================================================
//  B311: SKUTECNY nap_menu_gl.cpp jako knihovna pro test prohlizece (Python + ctypes):
//  hlavni menu kresli pres Mesa do "plochy telefonu" a posila proud na TV / PC
//  (/menu.stream) - presne ty JNI funkce, ktere vola Java (NativeMenuBridge).
//
//   g++ -std=c++17 -O2 -shared -fPIC -pthread -w -I../b304/stub -I../b291/stub -I$INC
//       -I$JAVA_HOME/include -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/tvproud
//       -o libmenu_b311.so tv_menu_lib.cpp -l:libEGL.so.1 -l:libGLESv2.so.2
// ====================================================================
#include "tv_menu.cpp"

extern "C" {

/** Zapne menu jako Java (assety, zamky Segy a PS1, plocha w x h). Vraci pocet karet. */
int menu_start(const char *assety, int w, int h) {
  pripravEnv();
  g_okno.w = w;
  g_okno.h = h;
  const int n = menu_assety(assety);
  menu_zamek("sega", true, u8"Napiš v Atari ještě 45 řádků programu (řádky s číslem, počítají se při RUN nebo LIST).");
  menu_zamek("ps1", true, u8"Nejdřív odemkni Segu, pak 30 minut hraní na Seze.");
  menu_prah(8.f);
  menu_ukaz(1);
  menu_plocha(true);
  return n;
}
int menu_pripraveno_c() { return menu_pripraveno() ? 1 : 0; }
int menu_klavesa_c(int k) { return menu_klavesa(k); }
int menu_vybrana_c() { return menu_vybrana(); }
long long menu_snimku_c() { return menu_snimku(); }
void menu_stop() { menu_plocha(false); }

// ---- proud na TV (jako Java napTvWebJadroProud: proudOtevri / proudDalsi / proudZavri) ----
int menu_otevri() { return proud_otevri(); }
/** pocet bajtu (4 B delka + paket), 0 = nic noveho, zaporne = male pole, MIN = chyba */
int menu_dalsi(int id, void *ven, int kap, int cekejMs, int zapisMs) {
  thread_local FakeArr a;
  if ((int)a.b.size() != kap) a.b.assign((size_t)kap, 0);
  const int n = Java_eu_atarihelp_emu10_NativeMenuBridge_proudDalsiNative(&g_env, nullptr, id, (jbyteArray)&a, cekejMs, zapisMs);
  if (n > 0) std::memcpy(ven, a.b.data(), (size_t)n);
  return n;
}
void menu_zavri(int id) { proud_zavri(id); }
int menu_stav() { return proud_stav(); }
const char *menu_log_c() {
  static std::string s;
  s = proud_log();
  return s.c_str();
}

}  // extern "C"
