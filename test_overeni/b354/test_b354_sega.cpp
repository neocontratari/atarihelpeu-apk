// test_overeni/b354/test_b354_sega.cpp
// B354: DVA HERNI OVLADACE = DVA HRACI NA SEZE - SKUTECNY nap_sega_native_proof.cpp z appky (jadro clownmdemu
// prelozene vedle, Android = stuby z ../b291/stub; infrastruktura okopirovana z ../b320/test_b320_sega_ovladac.cpp).
// Co jadro Segy vidi, se cte jeho dotazem nap_real_input_requested(player_id 0 / 1) - tak se pta kazdy snimek.
//  1) hrac 2 (ovladacKlavesaHrac(1, ...)) = ovladac 2 Segy: vsechna tlacitka a krizek, hrac 1 nic
//  2) oba hraci naraz, pusteni jednoho nepusti druheho; dotyk na strance (setInput) jen hrac 1
//  3) hrac 2: osy (packa, krizek HAT); ovladacPust (odpojeni) pusti oba
// Preklad jako ../b320/test_b320_sega_ovladac.cpp (PRAC/clown = objekty clownmdemu, viz ../b320/vse.sh):
//   g++ -std=c++17 -O2 -pthread $SEGA -o test_b354_sega test_b354_sega.cpp PRAC/clown/*.o -l:libEGL.so.1 -l:libGLESv2.so.2
//   ./test_b354_sega                                                -> VYSTUP_test_b354_sega.txt
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

static int ovl2(int kod, bool dolu) { return Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesaHrac(&g_env, nullptr, 1, kod, dolu ? JNI_TRUE : JNI_FALSE); }
static void osy2(float lx, float ly, float hx, float hy, int maska) {
  FakeFArr a;
  a.f = {lx, ly, 0, 0, hx, hy, 0, 0};
  Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacOsyHrac(&g_env, nullptr, 1, (jfloatArray)&a, maska);
}

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.NewStringUTF = fNewStringUTF;
  g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_env.functions = &g_tbl;
  std::printf("B354 - dva herni ovladace = dva hraci na Seze (nap_sega_native_proof.cpp z appky, dotaz jadra clownmdemu)\n");

  // 1) hrac 2
  {
    struct T { int kod; unsigned b; const char *j; };
    const T TL[] = {{99, 1u << 4, "X (leve) = A"}, {96, 1u << 5, "A (spodni) = B"}, {97, 1u << 6, "B (prave) = C"}, {108, 1u << 7, "START"},
                    {19, 1u << 0, "nahoru"}, {20, 1u << 1, "dolu"}, {21, 1u << 2, "vlevo"}, {22, 1u << 3, "vpravo"},
                    {270, (1u << 0) | (1u << 3), "uhlopricka nahoru-vpravo"}};
    int ok = 0;
    std::string chyby;
    for (const T &x : TL) {
      const int r = ovl2(x.kod, true);
      const unsigned h2 = jadro(1), h1 = jadro(0);
      ovl2(x.kod, false);
      const unsigned po = jadro(1);
      if (r != 0 && h2 == x.b && h1 == 0 && po == 0) ok++;
      else chyby += std::string(" | ") + x.j + ": hrac 2 " + jm(h2) + ", hrac 1 " + jm(h1) + ", po " + jm(po);
    }
    over(ok == (int)(sizeof(TL) / sizeof(TL[0])), "1) hrac 2: " + std::to_string(ok) + " z " + std::to_string(sizeof(TL) / sizeof(TL[0])) +
                                                     " tlacitek = spravne tlacitko ovladace 2 Segy (A B C START, krizek); hrac 1 nic" + chyby);
  }
  // 2) oba naraz, dotyk jen hrac 1
  {
    ovl(108, true);       // hrac 1 START
    ovl2(96, true);       // hrac 2 B
    dotyk(4, true);       // dotyk A (hrac 1)
    const unsigned a1 = jadro(0), a2 = jadro(1);
    ovl(108, false);
    const unsigned b1 = jadro(0), b2 = jadro(1);
    dotyk(4, false);
    ovl2(96, false);
    std::printf("oba: hrac 1 %s, hrac 2 %s | hrac 1 pustil START: %s / %s\n", jm(a1).c_str(), jm(a2).c_str(), jm(b1).c_str(), jm(b2).c_str());
    over(a1 == ((1u << 7) | (1u << 4)) && a2 == (1u << 5) && b1 == (1u << 4) && b2 == (1u << 5) && jadro(0) == 0 && jadro(1) == 0,
         "2) oba naraz: hrac 1 START (ovladac) + A (dotyk), hrac 2 B; pusteni hrace 1 nepusti hrace 2; dotyk jen hrac 1");
  }
  // 3) osy hrace 2, odpojeni
  {
    osy2(0.9f, 0, 0, 0, 15);
    const unsigned p = jadro(1), p1 = jadro(0);
    osy2(0, 0, -1, 1, 15);
    const unsigned h = jadro(1);
    osy2(0, 0, 0, 0, 15);
    over(p == (1u << 3) && p1 == 0 && h == ((1u << 2) | (1u << 1)) && jadro(1) == 0,
         "3a) hrac 2: leva packa = VPRAVO (hrac 1 nic), krizek HAT = " + jm(h) + ", pusteno nic");
    ovl(99, true); ovl2(97, true); osy2(0, -0.9f, 0, 0, 15);
    const unsigned d1 = jadro(0), d2 = jadro(1);
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacPust(&g_env, nullptr);
    over(d1 == (1u << 4) && d2 == ((1u << 6) | (1u << 0)) && jadro(0) == 0 && jadro(1) == 0,
         "3b) ovladac odpojen uprostred stisku obou hracu (hrac 1 " + jm(d1) + ", hrac 2 " + jm(d2) + ") -> ovladacPust: nic nedrzi");
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
