// test_overeni/b355/test_b355_sega.cpp
// B355: TLACITKA OVLADACE NA SEZE - SKUTECNY nap_sega_native_proof.cpp z appky (jadro clownmdemu prelozene
// vedle, Android = stuby z ../b291/stub; infrastruktura z ../b354/test_b354_sega.cpp). Co jadro vidi, se cte
// jeho dotazem nap_real_input_requested(player_id 0 / 1) - tak se pta kazdy snimek (i na X Y Z MODE).
//  1) vychozi mapa = do B354 (leve A, spodni B, prave C, L1 A, R1 C, START, krizek) + ovladac se 6 tlacitky:
//     horni = Y, L2 = X, R2 = Z (bojovky 1 na 1); SELECT a HOME (MENU) jadro nic
//  2) vlastni mapa z Javy (ovladacMapa): jadro vrati mapu; prohozene A / C, START = MODE, L1 = START ...
//  3) hrac 2 se stejnou mapou (X Y Z ovladace 2); analogova spoust R2 podle mapy; ovladacPust; "" = vychozi
// Preklad jako ../b354/test_b354_sega.cpp:
//   g++ -std=c++17 -O2 -pthread $SEGA -o test_b355_sega test_b355_sega.cpp PRAC/clown/*.o -l:libEGL.so.1 -l:libGLESv2.so.2
//   ./test_b355_sega                                                -> VYSTUP_test_b355_sega.txt
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
// B355: + 8 X 9 Y 10 Z 11 MODE (ovladac se 6 tlacitky - clownmdemu se pta i na ne)
static unsigned jadro(int hrac = 0) {
  static const ClownMDEmu_Button B[12] = {CLOWNMDEMU_BUTTON_UP, CLOWNMDEMU_BUTTON_DOWN, CLOWNMDEMU_BUTTON_LEFT, CLOWNMDEMU_BUTTON_RIGHT,
                                          CLOWNMDEMU_BUTTON_A,  CLOWNMDEMU_BUTTON_B,    CLOWNMDEMU_BUTTON_C,    CLOWNMDEMU_BUTTON_START,
                                          CLOWNMDEMU_BUTTON_X,  CLOWNMDEMU_BUTTON_Y,    CLOWNMDEMU_BUTTON_Z,    CLOWNMDEMU_BUTTON_MODE};
  unsigned r = 0;
  for (int i = 0; i < 12; i++) if (nap_real_input_requested(nullptr, (cc_u8f)hrac, B[i])) r |= 1u << i;
  return r;
}
static std::string jm(unsigned b) {
  static const char *J[12] = {"NAHORU", "DOLU", "VLEVO", "VPRAVO", "A", "B", "C", "START", "X", "Y", "Z", "MODE"};
  std::string s;
  for (int i = 0; i < 12; i++) if (b & (1u << i)) { if (!s.empty()) s += '+'; s += J[i]; }
  return s.empty() ? "nic" : s;
}

static int ovl2(int kod, bool dolu) { return Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesaHrac(&g_env, nullptr, 1, kod, dolu ? JNI_TRUE : JNI_FALSE); }
static void osyH(int hrac, float l2, float r2) {
  FakeFArr a;
  a.f = {0, 0, 0, 0, 0, 0, l2, r2};
  Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacOsyHrac(&g_env, nullptr, hrac, (jfloatArray)&a, 15);
}
static const char *g_mapaStr = nullptr;
static std::string mapa(const char *m) {
  FakeStr s{m};
  jstring r = Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacMapa(&g_env, nullptr, (jstring)&s);
  return r ? ((FakeStr *)r)->s : std::string("(null)");
}
static const char *JSTR(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JREL(JNIEnv *, jstring, const char *) {}
// jedno tlacitko hrace h: co jadro vidi pri stisku (a po pusteni musi byt nic)
static unsigned zmackni(int h, int kod, bool *pusteno = nullptr) {
  if (h) ovl2(kod, true); else ovl(kod, true);
  const unsigned r = jadro(h);
  if (h) ovl2(kod, false); else ovl(kod, false);
  if (pusteno) *pusteno = jadro(h) == 0;
  return r;
}

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.NewStringUTF = fNewStringUTF;
  g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetFloatArrayRegion = fGetFloatArrayRegion;
  g_tbl.GetStringUTFChars = JSTR;
  g_tbl.ReleaseStringUTFChars = JREL;
  g_env.functions = &g_tbl;
  (void)g_mapaStr;
  std::printf("B355 - tlacitka herniho ovladace na Seze (nap_sega_native_proof.cpp z appky, dotaz jadra clownmdemu vc. X Y Z MODE)\n");

  // 1) vychozi mapa
  {
    struct T { int kod; unsigned b; const char *j; };
    const T TL[] = {{99, 1u << 4, "X (leve) = A"}, {96, 1u << 5, "A (spodni) = B"}, {97, 1u << 6, "B (prave) = C"}, {108, 1u << 7, "START"},
                    {102, 1u << 4, "L1 = A"}, {103, 1u << 6, "R1 = C"}, {98, 1u << 6, "C = C"},
                    {100, 1u << 9, "Y (horni) = Y"}, {104, 1u << 8, "L2 = X"}, {105, 1u << 10, "R2 = Z"}, {101, 1u << 10, "Z = Z"},
                    {109, 0, "SELECT = nic"}, {110, 0, "HOME = MENU (jadro nic)"}, {106, 0, "L3 = nic"},
                    {19, 1u << 0, "nahoru"}, {22, 1u << 3, "vpravo"}};
    int ok = 0;
    std::string chyby;
    for (const T &x : TL) {
      bool pust = false;
      const unsigned r = zmackni(0, x.kod, &pust);
      if (r == x.b && pust) ok++;
      else chyby += std::string(" | ") + x.j + ": " + jm(r);
    }
    over(ok == (int)(sizeof(TL) / sizeof(TL[0])), "1) vychozi mapa: " + std::to_string(ok) + " z " + std::to_string(sizeof(TL) / sizeof(TL[0])) +
                                                     " tlacitek - jako do B354 (A B C START, L1 A, R1 C) + 6 tlacitek: horni Y, L2 X, R2 Z; SELECT a HOME nic" + chyby);
  }
  // 2) vlastni mapa
  {
    const std::string n = mapa("A=C;B=A;X=B;START=MODE;L1=START;Y=X;SELECT=Y;MODE=Z;R1=NAHORU;L2=FIRE");
    std::printf("mapa -> jadro: %s\n", n.c_str());
    bool pust = true, p = false;
    const unsigned a = zmackni(0, 96, &p); pust = pust && p;
    const unsigned b = zmackni(0, 97, &p); pust = pust && p;
    const unsigned x = zmackni(0, 99, &p); pust = pust && p;
    const unsigned st = zmackni(0, 108, &p); pust = pust && p;
    const unsigned l1 = zmackni(0, 102, &p); pust = pust && p;
    const unsigned y = zmackni(0, 100, &p); pust = pust && p;
    const unsigned se = zmackni(0, 109, &p); pust = pust && p;
    const unsigned h = zmackni(0, 110, &p); pust = pust && p;
    const unsigned r1 = zmackni(0, 103, &p); pust = pust && p;
    const unsigned l2 = zmackni(0, 104, &p); pust = pust && p;
    std::printf("A %s | B %s | X %s | START %s | L1 %s | Y %s | SELECT %s | HOME %s | R1 %s | L2 %s\n", jm(a).c_str(), jm(b).c_str(), jm(x).c_str(),
                jm(st).c_str(), jm(l1).c_str(), jm(y).c_str(), jm(se).c_str(), jm(h).c_str(), jm(r1).c_str(), jm(l2).c_str());
    over(n == "A=C;B=A;X=B;Y=X;L1=START;R1=NAHORU;L2=X;R2=Z;L3=0;R3=0;START=MODE;SELECT=Y;MODE=Z" &&
             a == (1u << 6) && b == (1u << 4) && x == (1u << 5) && st == (1u << 11) && l1 == (1u << 7) && y == (1u << 8) &&
             se == (1u << 9) && h == (1u << 10) && r1 == (1u << 0) && l2 == (1u << 8) && pust,
         "2) vlastni mapa: jadro ji vrati (FIRE neni cil Segy - L2 zustal vychozi X); A = C, B = A, X = B, START = MODE, L1 = START, "
         "Y = X, SELECT = Y, HOME = Z, R1 = nahoru; po pusteni nic");
  }
  // 3) hrac 2, spoust, pusteni, navrat
  {
    const unsigned y2 = zmackni(1, 100), h2 = zmackni(1, 110), a2 = zmackni(1, 96);
    osyH(1, 0.f, 0.9f); const unsigned r2 = jadro(1), r2h1 = jadro(0);
    osyH(1, 0.f, 0.f); const unsigned r2p = jadro(1);
    std::printf("hrac 2: Y %s, HOME %s, A %s, spoust R2 %s (hrac 1 %s), pustena %s\n", jm(y2).c_str(), jm(h2).c_str(), jm(a2).c_str(),
                jm(r2).c_str(), jm(r2h1).c_str(), jm(r2p).c_str());
    over(y2 == (1u << 8) && h2 == (1u << 10) && a2 == (1u << 6) && r2 == (1u << 10) && r2h1 == 0 && r2p == 0,
         "3a) hrac 2 se stejnou mapou: Y = X, HOME = Z, A = C (ovladac 2); analogova spoust R2 podle mapy = Z, hrac 1 nic");
    ovl(99, true); ovl2(100, true);
    const unsigned d1 = jadro(0), d2 = jadro(1);
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacPust(&g_env, nullptr);
    const unsigned p1 = jadro(0), p2 = jadro(1);
    const std::string v = mapa("");
    const unsigned yv = zmackni(0, 100), hv = zmackni(0, 110);
    std::printf("drzeno %s / %s -> ovladacPust %s / %s | mapa \"\" -> %s | Y %s, HOME %s\n", jm(d1).c_str(), jm(d2).c_str(), jm(p1).c_str(), jm(p2).c_str(),
                v.c_str(), jm(yv).c_str(), jm(hv).c_str());
    over(d1 == (1u << 5) && d2 == (1u << 8) && p1 == 0 && p2 == 0 &&
             v == "A=B;B=C;X=A;Y=Y;L1=A;R1=C;L2=X;R2=Z;L3=0;R3=0;START=START;SELECT=0;MODE=MENU" && yv == (1u << 9) && hv == 0,
         "3b) ovladacPust pusti oba (i X / Y ovladace se 6 tlacitky); mapa \"\" = zase vychozi (horni Y, HOME = MENU)");
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
