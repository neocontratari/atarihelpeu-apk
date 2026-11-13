// test_overeni/b320/test_b320_sega_ovladac.cpp
// B320: HERNI OVLADAC NA SEZE - SKUTECNY nap_sega_native_proof.cpp z appky (jadro clownmdemu prelozene
// vedle, Android nahrazen stuby z ../b291/stub jako v ../b306/tv_sega6.cpp). JNI presne jako Java
// (NativeSegaCoreBridge: setInput = dotyk na strance, ovladacKlavesa / ovladacOsy / ovladacPust =
// herni ovladac), a co jadro Segy uvidi, se cte primo jeho dotazem na tlacitko
// (nap_real_input_requested - tak se pta clownmdemu kazdy snimek).
//
// Kontroly:
//  1) kazde tlacitko ovladace -> spravne tlacitko Segy (leve A, spodni B, prave C, L1 A, R1 C,
//     START, krizek, uhlopricky), pusteni; hrac 2 nic
//  2) osy: leva packa a krizek HAT = smer (hystereze), spouste L2 / R2 a prava packa nic
//  3) dotyk na strance (g_input) + ovladac soucasne, pusteni jednoho nepusti druhe
//  4) ovladacPust (ovladac odpojen) a shutdown (konec Segy): nic nedrzi
//
//   SEGA="-DNAP_SEGA_VENDOR_CORE_PRESENT=1 -DCC_USE_C99_INTEGERS=1 -I../b291/stub -I$INC -I$JV/include -I$JV/include/linux
//         -I$V/source -I$V/libraries/clowncommon -I$V/libraries/clown68000/source/interpreter -I$V/libraries/clownz80/source"
//   g++ -std=c++17 -O2 -pthread $SEGA -o test_b320_sega_ovladac test_b320_sega_ovladac.cpp PRAC/clown/*.o -l:libEGL.so.1 -l:libGLESv2.so.2
#include "../../app/src/main/cpp/nap_sega_native_proof.cpp"
#include <cstdio>
#include <string>
#include <vector>

// ---------- nahrada okna (displej Segy se v testu nepouziva) ----------
struct ANativeWindow { int refs = 1; };
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *) { return 0; }
int32_t ANativeWindow_getHeight(ANativeWindow *) { return 0; }
int32_t ANativeWindow_getFormat(ANativeWindow *) { return 0; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t) { return 0; }
int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *) { return -1; }
int32_t ANativeWindow_unlockAndPost(ANativeWindow *) { return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject) { return nullptr; }

// ---------- rucne sestaveny JNIEnv ----------
struct FakeStr { std::string s; };
struct FakeFArr { std::vector<float> f; };
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeFArr *)a)->f.size(); }
static void JNICALL fGetFloatArrayRegion(JNIEnv *, jfloatArray a, jsize st, jsize n, jfloat *buf) { std::memcpy(buf, ((FakeFArr *)a)->f.data() + st, (size_t)n * sizeof(float)); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

static int ovl(int kod, bool dolu) { return Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesa(&g_env, nullptr, kod, dolu ? JNI_TRUE : JNI_FALSE); }
static void osy(float lx, float ly, float rx, float ry, float hx, float hy, float l2, float r2, int maska) {
  FakeFArr a;
  a.f = {lx, ly, rx, ry, hx, hy, l2, r2};
  Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacOsy(&g_env, nullptr, (jfloatArray)&a, maska);
}
static void dotyk(int i, bool dolu) { Java_eu_atarihelp_emu10_NativeSegaCoreBridge_setInput(&g_env, nullptr, i, dolu ? JNI_TRUE : JNI_FALSE); }
// co jadro Segy vidi (hrac 1): bity 0 U 1 D 2 L 3 R 4 A 5 B 6 C 7 START
static unsigned jadro(int hrac = 0) {
  static const ClownMDEmu_Button B[8] = {CLOWNMDEMU_BUTTON_UP, CLOWNMDEMU_BUTTON_DOWN, CLOWNMDEMU_BUTTON_LEFT, CLOWNMDEMU_BUTTON_RIGHT,
                                         CLOWNMDEMU_BUTTON_A,  CLOWNMDEMU_BUTTON_B,    CLOWNMDEMU_BUTTON_C,    CLOWNMDEMU_BUTTON_START};
  unsigned r = 0;
  for (int i = 0; i < 8; i++) if (nap_real_input_requested(nullptr, (cc_u8f)hrac, B[i])) r |= 1u << i;
  return r;
}
static std::string jm(unsigned b) {
  static const char *J[8] = {"NAHORU", "DOLU", "VLEVO", "VPRAVO", "A", "B", "C", "START"};
  std::string s;
  for (int i = 0; i < 8; i++) if (b & (1u << i)) { if (!s.empty()) s += '+'; s += J[i]; }
  return s.empty() ? "nic" : s;
}

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.NewStringUTF = fNewStringUTF;
  g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_env.functions = &g_tbl;
  std::printf("B320 - herni ovladac na Seze (nap_sega_native_proof.cpp z appky, dotaz jadra clownmdemu na tlacitka)\n");

  // ======== 1) tlacitka ========
  struct T { int kod; unsigned b; const char *j; };
  const T TL[] = {{99, 1u << 4, "X (leve) = A"}, {96, 1u << 5, "A (spodni) = B"}, {97, 1u << 6, "B (prave) = C"},
                  {102, 1u << 4, "L1 = A"}, {103, 1u << 6, "R1 = C"}, {98, 1u << 6, "C = C"}, {108, 1u << 7, "START"},
                  {19, 1u << 0, "krizek nahoru"}, {20, 1u << 1, "dolu"}, {21, 1u << 2, "vlevo"}, {22, 1u << 3, "vpravo"},
                  {269, (1u << 1) | (1u << 2), "uhlopricka dolu-vlevo"}, {23, 1u << 5, "DPAD_CENTER = B"}};
  int ok = 0;
  std::string chyby;
  for (const T &x : TL) {
    const int r = ovl(x.kod, true);
    const unsigned dole = jadro(), hrac2 = jadro(1);
    ovl(x.kod, false);
    const unsigned po = jadro();
    if (r != 0 && dole == x.b && hrac2 == 0 && po == 0) ok++;
    else chyby += std::string(" | ") + x.j + ": " + jm(dole) + ", po " + jm(po);
  }
  over(ok == (int)(sizeof(TL) / sizeof(TL[0])),
       "1) " + std::to_string(ok) + " z " + std::to_string(sizeof(TL) / sizeof(TL[0])) +
           " tlacitek ovladace = spravne tlacitko Segy (leve A, spodni B, prave C, L1 A, R1 C, START, krizek); hrac 2 nic; pusteni" + chyby);
  const int y = ovl(100, true), sel = ovl(109, true);
  const unsigned ys = jadro();
  ovl(100, false); ovl(109, false);
  over(y != 0 && sel != 0 && ys == 0 && ovl(4, true) == 0, "1b) Y a SELECT na Seze nic (Mega Drive je nema); ZPET neni tlacitko ovladace");
  ovl(4, false);

  // ======== 2) osy ========
  {
    const int M = 15;
    osy(-0.9f, 0, 0, 0, 0, 0, 0, 0, M);
    const unsigned a = jadro();
    osy(-0.4f, 0, 0, 0, 0, 0, 0, 0, M);
    const unsigned b = jadro();
    osy(0, 0, 0, 0, 1, -1, 0, 0, M);
    const unsigned h = jadro();
    osy(0, 0, 1, 1, 0, 0, 1, 1, M);
    const unsigned nic = jadro();
    osy(0, 0, 0, 0, 0, 0, 0, 0, M);
    over(a == (1u << 2) && b == (1u << 2) && h == ((1u << 3) | (1u << 0)) && nic == 0 && jadro() == 0,
         "2) leva packa -0,9 = VLEVO, -0,4 porad (hystereze); krizek HAT = " + jm(h) + "; prava packa a spouste nic; pusteno nic");
  }

  // ======== 3) dotyk + ovladac ========
  {
    dotyk(7, true);       // START dotykem
    ovl(96, true);        // B ovladacem
    const unsigned oboje = jadro();
    ovl(96, false);
    const unsigned jenDotyk = jadro();
    ovl(96, true);
    dotyk(7, false);
    const unsigned jenOvl = jadro();
    ovl(96, false);
    over(oboje == ((1u << 7) | (1u << 5)) && jenDotyk == (1u << 7) && jenOvl == (1u << 5) && jadro() == 0,
         "3) dotyk na strance (START) + ovladac (B) soucasne: " + jm(oboje) + "; pusteni jednoho nepusti druhe");
  }

  // ======== 4) odpojeni a konec Segy ========
  {
    ovl(99, true); ovl(22, true); osy(0, 0.9f, 0, 0, 0, 0, 0, 0, 15);
    const unsigned drzi = jadro();
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacPust(&g_env, nullptr);
    const unsigned po = jadro();
    ovl(97, true);
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_shutdown(&g_env, nullptr);
    const unsigned poKonci = jadro();
    over(drzi == ((1u << 4) | (1u << 3) | (1u << 1)) && po == 0 && poKonci == 0,
         "4) drzeno " + jm(drzi) + ": ovladac odpojen (ovladacPust) = nic; drzene C pri konci Segy (shutdown) = nic");
  }
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
