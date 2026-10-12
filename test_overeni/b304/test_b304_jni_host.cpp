// test_overeni/b304/test_b304_jni_host.cpp
// ====================================================================
//  B304: KONEC-KONCU test SKUTECNEHO nap_intro_gl.cpp (most dema do Androidu)
//  na PC - stejny princip jako test_overeni/b292/test_b292_jni_host.cpp:
//  .cpp se vlozi cely, Android je nahrazen malymi nahradami a test dela to,
//  co v appce dela Java (NapDemoView, NativeIntroBridge, AudioTrack).
//
//   - plocha: misto okna telefonu pbuffer pres Mesa (EGL_PLATFORM=surfaceless)
//   - AssetManager: logo se cte z app/src/main/assets
//   - JNIEnv: rucne sestaveny (pole short/int, retezce, globalni reference)
//   - "AudioTrack": zapisuje po 1024 ramcich, blokuje jako skutecny (buffer
//     100 ms) a hlasi, kolik uz "prehral" (introCasZvuku)
//
//  Overuje:
//   1) demo se pripravi (shadery, texty) a do pripravy dostava Java ticho
//   2) cas obrazu (introCasMs) jde za tim, co "reproduktor" prehral
//   3) TV snimky 854x480 chodi, kdyz si je Java bere (introGrabFrame)
//   4) zvuk, ktery Java dostala, je PRESNE stopa dema (bit po bitu)
//   5) konec: po 45 s introHotovo, po cele stope introNaplnZvuk = -1
//   6) preskoceni: plocha pryc + introStop -> vlakna skonci, zvuk -1
//
//  g++ -std=c++17 -O2 -pthread -Istub -I../b291/stub -I$INC -I$JAVA_HOME/include
//      -I$JAVA_HOME/include/linux -o test_b304_jni_host test_b304_jni_host.cpp
//      -l:libEGL.so.1 -l:libGLESv2.so.2                  (vse na jednom radku)
//  ./test_b304_jni_host
// ====================================================================
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <jni.h>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

// ---------- okno telefonu = pbuffer ----------
#include "android/native_window.h"
struct ANativeWindow { int w = 360, h = 780; int refs = 1; };
static ANativeWindow g_okno;
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *w) { return w->w; }
int32_t ANativeWindow_getHeight(ANativeWindow *w) { return w->h; }
int32_t ANativeWindow_getFormat(ANativeWindow *) { return 1; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *, int32_t, int32_t, int32_t) { return 0; }
int32_t ANativeWindow_lock(ANativeWindow *, ANativeWindow_Buffer *, ARect *) { return -1; }
int32_t ANativeWindow_unlockAndPost(ANativeWindow *) { return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject s) { if (!s) return nullptr; g_okno.refs++; return &g_okno; }

#undef EGL_WINDOW_BIT
#define EGL_WINDOW_BIT EGL_PBUFFER_BIT
static EGLSurface testPlocha(EGLDisplay d, EGLConfig c, ANativeWindow *w) {
  const EGLint a[] = {EGL_WIDTH, w->w, EGL_HEIGHT, w->h, EGL_NONE};
  return eglCreatePbufferSurface(d, c, a);
}
#define eglCreateWindowSurface(d, c, w, a) testPlocha(d, c, (ANativeWindow *)(w))

// ---------- AssetManager: soubory z app/src/main/assets ----------
#include "android/asset_manager.h"
struct AAssetManager { int x; };
struct AAsset { std::vector<char> d; };
AAssetManager *AAssetManager_fromJava(JNIEnv *, jobject) { static AAssetManager m; return &m; }
AAsset *AAssetManager_open(AAssetManager *, const char *jm, int) {
  std::ifstream f(std::string("../../app/src/main/assets/") + jm, std::ios::binary);
  if (!f) return nullptr;
  AAsset *a = new AAsset;
  a->d.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
  return a;
}
const void *AAsset_getBuffer(AAsset *a) { return a->d.data(); }
off_t AAsset_getLength(AAsset *a) { return (off_t)a->d.size(); }
void AAsset_close(AAsset *a) { delete a; }

#include "../../app/src/main/cpp/intro/nap_intro_gl.cpp"

// ---------- rucne sestaveny JNIEnv ----------
struct FakePole { std::vector<jshort> s; std::vector<jint> i; bool jeShort = true; };
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) {
  FakePole *p = (FakePole *)a;
  return p->jeShort ? (jsize)p->s.size() : (jsize)p->i.size();
}
static jshort *JNICALL fGetShortArrayElements(JNIEnv *, jshortArray a, jboolean *) { return ((FakePole *)a)->s.data(); }
static void JNICALL fReleaseShortArrayElements(JNIEnv *, jshortArray, jshort *, jint) {}
static jint *JNICALL fGetIntArrayElements(JNIEnv *, jintArray a, jboolean *) { return ((FakePole *)a)->i.data(); }
static void JNICALL fReleaseIntArrayElements(JNIEnv *, jintArray, jint *, jint) {}
static jobject JNICALL fNewGlobalRef(JNIEnv *, jobject o) { return o; }
static void JNICALL fDeleteGlobalRef(JNIEnv *, jobject) {}
static std::string g_posledniString;
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { g_posledniString = c ? c : ""; return (jstring)&g_posledniString; }

static int g_ok = 0, g_chyb = 0;
static void kontrola(bool ok, const char *co, const std::string &detail = "") {
  std::printf("%s  %s%s%s\n", ok ? "OK   " : "CHYBA", co, detail.empty() ? "" : "  -- ", detail.c_str());
  (ok ? g_ok : g_chyb)++;
}
static double tedS() { return std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count(); }

int main() {
  setenv("EGL_PLATFORM", "surfaceless", 1);
  JNINativeInterface_ tab;
  std::memset(&tab, 0, sizeof tab);
  tab.GetArrayLength = fGetArrayLength;
  tab.GetShortArrayElements = fGetShortArrayElements;
  tab.ReleaseShortArrayElements = fReleaseShortArrayElements;
  tab.GetIntArrayElements = fGetIntArrayElements;
  tab.ReleaseIntArrayElements = fReleaseIntArrayElements;
  tab.NewGlobalRef = fNewGlobalRef;
  tab.DeleteGlobalRef = fDeleteGlobalRef;
  tab.NewStringUTF = fNewStringUTF;
  JNIEnv_ envS;
  envS.functions = &tab;
  JNIEnv *env = &envS;
  int falesnyAm = 1, falesnaPlocha = 2;

  FakePole zvuk; zvuk.s.assign(1024 * 2, 0);
  FakePole tvPole; tvPole.jeShort = false; tvPole.i.assign(16, 0);    // schvalne male: C++ rekne, kolik je potreba

  // ===== 1. beh: cele demo =====
  Java_eu_atarihelp_emu10_NativeIntroBridge_introNastavAssety(env, nullptr, (jobject)&falesnyAm);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(env, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(env, nullptr, (jobject)&falesnaPlocha);

  const int LAT = 4410;                  // buffer AudioTrack ~100 ms
  std::vector<jshort> slyseno;           // vse, co Java dostala
  long long zapsano = 0;
  double hlava = 0;                      // "prehrane" ramce
  double t0 = tedS(), posl = t0, rychlost = 1.0;
  int tichoPred = 0, tvSnimku = 0, tvVelikost = 0, tvCerne = 0, tvSvetle = 0;
  double maxOdch = 0, sumOdch = 0; int nOdch = 0;
  bool videlPripraveno = false, hotovoPredKoncem = false, konecVidenHotovo = false;
  double dalsiKontrola = t0 + 0.25, dalsiTv = t0 + 0.1;
  int vysledek = 0;
  for (;;) {
    const bool prip = Java_eu_atarihelp_emu10_NativeIntroBridge_introPripraveno(env, nullptr) == JNI_TRUE;
    vysledek = Java_eu_atarihelp_emu10_NativeIntroBridge_introNaplnZvuk(env, nullptr, (jshortArray)&zvuk, 1024);
    if (vysledek < 0) {
      konecVidenHotovo = Java_eu_atarihelp_emu10_NativeIntroBridge_introHotovo(env, nullptr) == JNI_TRUE;
      break;
    }
    if (!prip && !videlPripraveno) {
      bool ticho = true;
      for (jshort v : zvuk.s) if (v) { ticho = false; break; }
      if (ticho) tichoPred++;
    }
    if (prip) videlPripraveno = true;
    slyseno.insert(slyseno.end(), zvuk.s.begin(), zvuk.s.end());
    zapsano += vysledek;
    // blokujici zapis: ceka, az v bufferu bude misto (reproduktor hraje v case)
    for (;;) {
      const double ted = tedS();
      hlava += (ted - posl) * 44100.0 * rychlost;
      posl = ted;
      if (hlava > zapsano) hlava = (double)zapsano;
      if (zapsano - hlava <= LAT) break;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    Java_eu_atarihelp_emu10_NativeIntroBridge_introCasZvuku(env, nullptr, (jlong)hlava);
    const double ted = tedS();
    // 2) cas obrazu proti zvuku (jen pri normalni rychlosti a az po 2 s)
    if (ted >= dalsiKontrola) {
      dalsiKontrola = ted + 0.25;
      const long long zac = g_zacatekStopy.load();
      if (zac >= 0 && hlava > zac) {
        const double ocek = (hlava - zac) / 44100.0;
        const double obraz = Java_eu_atarihelp_emu10_NativeIntroBridge_introCasMs(env, nullptr) / 1000.0;
        if (ocek > 2.0 && ocek < 12.0 && rychlost == 1.0) {
          const double o = std::fabs(obraz - ocek);
          maxOdch = std::max(maxOdch, o); sumOdch += o; nOdch++;
        }
        if (ocek < 44.5 && Java_eu_atarihelp_emu10_NativeIntroBridge_introHotovo(env, nullptr) == JNI_TRUE) hotovoPredKoncem = true;
        if (ocek > 12.0) rychlost = 3.0;           // zbytek rychleji (test konce)
      }
    }
    // 3) TV snimky
    if (ted >= dalsiTv) {
      dalsiTv = ted + 0.1;
      jint r = Java_eu_atarihelp_emu10_NativeIntroBridge_introGrabFrame(env, nullptr, (jintArray)&tvPole);
      if (r < 0) { tvPole.i.assign((size_t)((-r) >> 16) * ((-r) & 0xFFFF), 0); }
      else if (r > 0) {
        tvSnimku++;
        tvVelikost = r;
        double jas = 0;
        const int n = (r >> 16) * (r & 0xFFFF);
        for (int i = 0; i < n; i += 97) { unsigned px = (unsigned)tvPole.i[i]; jas += ((px >> 16) & 255) + ((px >> 8) & 255) + (px & 255); }
        jas /= (n / 97) * 3.0;
        if (jas < 2.0) tvCerne++; else tvSvetle++;
      }
    }
    if (ted - t0 > 120) { std::printf("pojistka: demo nedobehlo za 120 s\n"); break; }
  }
  // po posledni davce zvuku ma demo do chvilky hlasit hotovo (obraz uz je za 45 s)
  for (int i = 0; i < 200 && !konecVidenHotovo; i++) {
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    konecVidenHotovo = Java_eu_atarihelp_emu10_NativeIntroBridge_introHotovo(env, nullptr) == JNI_TRUE;
  }
  const double trvalo = tedS() - t0;
  const long long snimku = Java_eu_atarihelp_emu10_NativeIntroBridge_introSnimku(env, nullptr);
  jstring info = Java_eu_atarihelp_emu10_NativeIntroBridge_introInfo(env, nullptr);
  (void)info;
  std::printf("beh: %.1f s, snimku telefonu %lld, info: %s\n", trvalo, snimku, g_posledniString.c_str());

  kontrola(videlPripraveno, "1) demo se pripravilo (shadery, texty, logo z assets)");
  kontrola(tichoPred > 0, "1) do pripravy dostava Java ticho", std::to_string(tichoPred) + " davek ticha");
  char b[200];
  std::snprintf(b, sizeof b, "prumer %.0f ms, nejvic %.0f ms (%d mereni)", nOdch ? sumOdch / nOdch * 1000 : -1.0, maxOdch * 1000, nOdch);
  kontrola(nOdch > 10 && maxOdch < 0.35 && sumOdch / std::max(1, nOdch) < 0.15, "2) cas obrazu jde za prehranym zvukem", b);
  std::snprintf(b, sizeof b, "%d snimku %dx%d, svetlych %d, cernych %d", tvSnimku, tvVelikost >> 16, tvVelikost & 0xFFFF, tvSvetle, tvCerne);
  kontrola(tvSnimku > 5 && tvVelikost == ((854 << 16) | 480) && tvSvetle > 3, "3) TV snimky 854x480 chodi", b);
  // 4) zvuk bit po bitu proti samostatne spocitane stope
  {
    napdemo::zv::Stopa st;
    st.vyrob();
    const long long zac = g_zacatekStopy.load();
    long long rozdil = 0, porovnano = 0;
    for (long long i = 0; i < st.celkem() && (zac + i) * 2 + 1 < (long long)slyseno.size(); i++) {
      porovnano++;
      if (slyseno[(size_t)(zac + i) * 2] != st.data()[i * 2] || slyseno[(size_t)(zac + i) * 2 + 1] != st.data()[i * 2 + 1]) rozdil++;
    }
    bool predTicho = true;
    for (long long i = 0; i < zac * 2 && i < (long long)slyseno.size(); i++) if (slyseno[(size_t)i]) { predTicho = false; break; }
    std::snprintf(b, sizeof b, "porovnano %lld ramcu (stopa %d), rozdilnych %lld, pred stopou %lld ramcu ticha", porovnano, st.celkem(), rozdil, zac);
    kontrola(porovnano == st.celkem() && rozdil == 0 && predTicho, "4) Java dostala presne stopu dema", b);
  }
  kontrola(konecVidenHotovo && !hotovoPredKoncem, "5) po cele stope introNaplnZvuk = -1 a demo hlasi hotovo (ne driv)");
  // Java: zvukStop, odebrat plochu (surfaceDestroyed), introStop
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(env, nullptr, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStop(env, nullptr);
  kontrola(!g_vlakno.joinable() && !g_zvukVlakno.joinable() && g_okno.refs == 1, "5) po konci vlakna skoncila, okno uvolnene",
           "refs=" + std::to_string(g_okno.refs));

  // ===== 2. beh: preskoceni po 3 s =====
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStart(env, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(env, nullptr, (jobject)&falesnaPlocha);
  t0 = tedS();
  while (Java_eu_atarihelp_emu10_NativeIntroBridge_introPripraveno(env, nullptr) != JNI_TRUE && tedS() - t0 < 20)
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  std::this_thread::sleep_for(std::chrono::milliseconds(3000));
  Java_eu_atarihelp_emu10_NativeIntroBridge_introSetDisplaySurface(env, nullptr, nullptr);
  Java_eu_atarihelp_emu10_NativeIntroBridge_introStop(env, nullptr);
  const long long sn1 = Java_eu_atarihelp_emu10_NativeIntroBridge_introSnimku(env, nullptr);   // az po zastaveni
  const int r2 = Java_eu_atarihelp_emu10_NativeIntroBridge_introNaplnZvuk(env, nullptr, (jshortArray)&zvuk, 1024);
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  const long long sn2 = Java_eu_atarihelp_emu10_NativeIntroBridge_introSnimku(env, nullptr);
  std::snprintf(b, sizeof b, "snimku za 3 s: %lld, po zastaveni pribylo %lld, zvuk vraci %d", sn1, sn2 - sn1, r2);
  kontrola(sn1 > 0 && sn2 == sn1 && r2 == -1 && !g_vlakno.joinable() && g_okno.refs == 1, "6) preskoceni: kresleni i zvuk stoji", b);

  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
