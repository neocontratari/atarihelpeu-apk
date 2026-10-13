// B305: knihovna pro KONEC-KONCU test prohlizece (test_b305_prohlizec.py) - SEGA.
//
// SKUTECNY nap_sega_native_proof.cpp (Android nahrazen stuby z ../b291/stub,
// jadro clownmdemu prelozene vedle). Misto ROM (zadnou hru v repozitari nemame)
// "jadro" krmi skutecne zpetne volani VDP: nap_real_colour_updated (paleta CRAM)
// a nap_real_scanline_rendered (radek po radku, levy sloupec vypnuty jako
// u Sonica), 60x za vterinu. Na konci posledniho radku se snimek sam (B305 hacek
// v appce) preda do prenosu. Zvuk jde do front FM / PSG jako z jadra a "Java"
// (vlakno jako AudioTrack v appce) si ho bere pres skutecne JNI pullAudioStereo.
// Python server pak dela to, co Java (/sega.stream -> proudDalsiNative).
//
//   viz JAK_SPUSTIT.txt (preklad i s jadrem clownmdemu)
#include "../../app/src/main/cpp/nap_sega_native_proof.cpp"
#include "sega_hra.h"
#include <map>
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

// ---------- rucne sestaveny JNIEnv (jen to, co JNI funkce pouzivaji) ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; };
struct FakeShortArr { std::vector<int16_t> v; };
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static jsize JNICALL fGetArrayLengthS(JNIEnv *, jarray a) { return (jsize)((FakeShortArr *)a)->v.size(); }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, (size_t)n); }
static jshort *JNICALL fGetShortArrayElements(JNIEnv *, jshortArray a, jboolean *) { return (jshort *)((FakeShortArr *)a)->v.data(); }
static void JNICALL fReleaseShortArrayElements(JNIEnv *, jshortArray, jshort *, jint) {}
static JNINativeInterface_ g_tbl, g_tblA;
static JNIEnv g_envB, g_envA;      // vlakno prohlizece (pole bajtu) a vlakno zvuku (pole shortu)

static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }

// ---------- "jadro" a "Java zvuk" ----------
static std::thread g_tj, g_tz;
static std::atomic<bool> g_bezi{false};
static std::atomic<bool> g_ctverec{false};      // test zpozdeni: bily ctverec v obraze
static std::atomic<bool> g_stuj{false};         // obraz stoji (stejny snimek porad)
static std::atomic<long long> g_snimku{0};
static std::mutex g_mOcek;
static std::map<unsigned, std::vector<uint32_t>> g_ocek;   // cislo snimku (9 bitu) -> snimek, jak ho jadro dalo
static int g_W = 320, g_H = 224;

static unsigned cisloBarva(unsigned n) { return n & 0x1FF; }   // 9 bitu barvy MD
static unsigned cramZ9(unsigned c9) { return ((c9 & 7) << 1) | (((c9 >> 3) & 7) << 5) | (((c9 >> 6) & 7) << 9); }

static void jadroVlakno() {
  SegaHra hra;
  std::vector<uint8_t> I;
  auto t = std::chrono::steady_clock::now();
  std::vector<jshort> L(800), R(800), P(800, 0);
  long long zvuk = 0;
  for (unsigned n = 1; g_bezi.load(); n++) {
    if (!g_stuj.load()) hra.krok();
    hra.kresliIndexy(I);
    if (g_ctverec.load())
      for (int y = 60; y < 100; y++) for (int x = 100; x < 160; x++) I[(size_t)y * hra.W + x] = 254;
    // cislo snimku v bodu (0,0) - levy sloupec je vypnuty, takze bod 8 (posune se na 0)
    I[8] = 255;
    for (int i = 0; i < 64; i++) nap_real_colour_updated(&g_real, (cc_u16f)i, (cc_u16f)hra.cramSlovo(i));
    nap_real_colour_updated(&g_real, 254, (cc_u16f)cramZ9(0x1FF));   // bila
    nap_real_colour_updated(&g_real, 255, (cc_u16f)cramZ9(cisloBarva(n)));
    for (int y = 0; y < hra.H; y++)
      nap_real_scanline_rendered(&g_real, (cc_u16f)y, &I[(size_t)y * hra.W], 8, (cc_u16f)hra.W, (cc_u16f)hra.W, (cc_u16f)hra.H);
    {
      std::lock_guard<std::mutex> l(g_mOcek);
      g_ocek[cisloBarva(n)] = g_real.frame_argb;     // presne, co jadro dalo (po posunu leveho sloupce)
    }
    g_snimku++;
    // zvuk jednoho snimku (48000 / 60) do front jako z jadra: L/R pila, PSG ticho
    for (int i = 0; i < 800; i++, zvuk++) { L[i] = (jshort)(((zvuk * 37) % 16000) - 8000); R[i] = (jshort)(((zvuk * 53) % 12000) - 6000); }
    nap_audio_push_fm_batch(L, R);
    nap_audio_push_psg_batch(P);
    t += std::chrono::microseconds(16667);
    std::this_thread::sleep_until(t);
  }
}
static void zvukVlakno() {          // MainActivity: AudioTrack + pullAudioStereo po 1024 ramcich
  FakeShortArr a; a.v.assign(2048, 0);
  auto t = std::chrono::steady_clock::now();
  while (g_bezi.load()) {
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_pullAudioStereo(&g_envA, nullptr, (jshortArray)&a, 1024);
    t += std::chrono::microseconds(1024 * 1000000LL / 48000);
    std::this_thread::sleep_until(t);
  }
}

extern "C" {
int sega_start() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.GetShortArrayElements = fGetShortArrayElements; g_tbl.ReleaseShortArrayElements = fReleaseShortArrayElements;
  g_tblA = g_tbl; g_tblA.GetArrayLength = fGetArrayLengthS;
  g_envB.functions = &g_tbl; g_envA.functions = &g_tblA;
  {
    std::lock_guard<std::mutex> lock(g_real_mutex);
    nap_real_reset_state_locked();
  }
  nap_audio_clear();
  g_bezi = true;
  g_tj = std::thread(jadroVlakno);
  g_tz = std::thread(zvukVlakno);
  return 1;
}
void sega_stop() {
  g_bezi = false;
  if (g_tj.joinable()) g_tj.join();
  if (g_tz.joinable()) g_tz.join();
}
void sega_ctverec(int z) { g_ctverec = z != 0; }
void sega_stuj(int z) { g_stuj = z != 0; }
long long sega_snimku() { return g_snimku.load(); }
// ocekavany snimek podle barvy bodu (0,0) v prohlizeci; vraci W<<16|H, 0 = nezname
int sega_ocekavany(unsigned barva, uint32_t *out) {
  // barva (RGB z canvasu) -> 9bitove cislo snimku (stejny prevod jako nap_md_colour_to_argb)
  const unsigned r = (barva >> 16) & 255, g = (barva >> 8) & 255, b = barva & 255;
  const unsigned c9 = (r >> 5) | ((g >> 5) << 3) | ((b >> 5) << 6);
  std::lock_guard<std::mutex> l(g_mOcek);
  auto it = g_ocek.find(c9);
  if (it == g_ocek.end() || it->second.size() != (size_t)g_W * g_H) return 0;
  std::memcpy(out, it->second.data(), it->second.size() * 4);
  return (g_W << 16) | g_H;
}
// /sega.stream -> proudOtevri / Dalsi / Zavri (Java napTvWebJadroProud)
int sega_otevri() { return Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudOtevriNative(&g_envB, nullptr); }
int sega_dalsi(int id, uint8_t *out, int kap, int cekejMs, int zapisMs) {
  thread_local FakeArr a;
  if ((int)a.b.size() != kap) a.b.assign((size_t)kap, 0);
  const jint n = Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudDalsiNative(&g_envB, nullptr, id, (jbyteArray)&a, cekejMs, zapisMs);
  if (n > 0) std::memcpy(out, a.b.data(), (size_t)n);
  return n;
}
void sega_zavri(int id) { Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudZavriNative(&g_envB, nullptr, id); }
int sega_stav() { return Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudStavNative(&g_envB, nullptr); }
const char *sega_log() { thread_local std::string r; r = jstr(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_proudLogNative(&g_envB, nullptr)); return r.c_str(); }
}

// JVM test (jvm/TestPodpisu.java): knihovna nactena skutecnou tridou NativeSegaCoreBridge
// - "jadro" se rozbehne samo, at proudDalsiNative ma co balit
extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
  if (getenv("NAP_JVM_TEST")) { sega_start(); atexit(sega_stop); }   // pri System.exit zastavit vlakna
  return JNI_VERSION_1_6;
}
