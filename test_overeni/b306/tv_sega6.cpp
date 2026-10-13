// B306: knihovna pro KONEC-KONCU test prohlizece (test_b306_prohlizec.py) - SEGA.
//
// SKUTECNY nap_sega_native_proof.cpp (Android nahrazen stuby z ../b291/stub,
// jadro clownmdemu prelozene vedle) - jako ../b305/tv_sega.cpp, ale krok "jadra"
// je ted presne jako ve vlakne emulace appky (nap_real_worker_thread):
//
//   1) radky snimku (skutecne nap_real_scanline_rendered, paleta CRAM, levy sloupec vypnuty)
//   2) zvuk tehoz snimku do front FM / PSG (skutecne nap_audio_push_fm_batch / _psg_batch -
//      B306: odtud jde zvuk na TV HNED, ne az za frontou reproduktoru)
//   3) nap_tv_po_kroku - B306: snimek az po celem kroku (paket = obraz + zvuk tehoz okamziku)
//
// Zvuk: FM L = sinus 440 Hz, FM R = 660 Hz, PSG = 1000 Hz (smichani FM + PSG jako pro
// reproduktor). ZNACKA pro mereni A/V: dokud je v obraze bily ctverec, je FM 2000 Hz
// a 16000 (hlasite) - zacne ve STEJNEM kroku jadra jako ctverec.
// "Java AudioTrack" si dal bere zvuk pres skutecne JNI pullAudioStereo (reproduktor
// telefonu); fronta pred nim muze byt predplnena jako na S8 (sega_fronta_s8), at je
// videt, ze na TV uz nepridava zpozdeni.
#include "../../app/src/main/cpp/nap_sega_native_proof.cpp"
#include "../b305/sega_hra.h"
#include <cmath>
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
static std::atomic<bool> g_ctverec{false};      // bily ctverec v obraze + znacka ve zvuku
static std::atomic<bool> g_stuj{false};
static std::atomic<long long> g_snimku{0};
static std::atomic<int> g_s8{0};                 // fronta reproduktoru predplnena jako na S8
static std::atomic<long long> g_znackaSeq{0};    // cislo snimku (v paketu), kde se ctverec objevil poprve
static std::mutex g_mOcek;
static std::map<unsigned, std::vector<uint32_t>> g_ocek;
static int g_W = 320, g_H = 224;

static unsigned cisloBarva(unsigned n) { return n & 0x1FF; }
static unsigned cramZ9(unsigned c9) { return ((c9 & 7) << 1) | (((c9 >> 3) & 7) << 5) | (((c9 >> 6) & 7) << 9); }

static void jadroVlakno() {
  SegaHra hra;
  std::vector<uint8_t> I;
  auto t = std::chrono::steady_clock::now();
  std::vector<jshort> L(800), R(800), P(800);
  long long n0 = 0;                   // absolutni cislo vzorku (spojita faze)
  const double PI2 = 6.283185307179586;
  for (unsigned n = 1; g_bezi.load(); n++) {
    if (!g_stuj.load()) hra.krok();
    hra.kresliIndexy(I);
    const bool znacka = g_ctverec.load();
    if (znacka)
      for (int y = 60; y < 100; y++) for (int x = 100; x < 160; x++) I[(size_t)y * hra.W + x] = 254;
    I[8] = 255;
    for (int i = 0; i < 64; i++) nap_real_colour_updated(&g_real, (cc_u16f)i, (cc_u16f)hra.cramSlovo(i));
    nap_real_colour_updated(&g_real, 254, (cc_u16f)cramZ9(0x1FF));
    nap_real_colour_updated(&g_real, 255, (cc_u16f)cramZ9(cisloBarva(n)));
    for (int y = 0; y < hra.H; y++)
      nap_real_scanline_rendered(&g_real, (cc_u16f)y, &I[(size_t)y * hra.W], 8, (cc_u16f)hra.W, (cc_u16f)hra.W, (cc_u16f)hra.H);
    {
      std::lock_guard<std::mutex> l(g_mOcek);
      g_ocek[cisloBarva(n)] = g_real.frame_argb;
    }
    g_snimku++;
    // zvuk tehoz snimku (48000 / 60)
    for (int i = 0; i < 800; i++) {
      const double s = (double)(n0 + i) / 48000.0;
      if (znacka) { L[i] = (jshort)(16000 * std::sin(PI2 * 2000 * s)); R[i] = L[i]; }
      else { L[i] = (jshort)(6000 * std::sin(PI2 * 440 * s)); R[i] = (jshort)(5000 * std::sin(PI2 * 660 * s)); }
      P[i] = (jshort)(1000 * std::sin(PI2 * 1000 * s));
    }
    n0 += 800;
    nap_audio_push_fm_batch(L, R);
    nap_audio_push_psg_batch(P);
#ifndef NAP_STARY_B305
    nap_tv_po_kroku(&g_real);        // B306: jako vlakno emulace po ClownMDEmu_Iterate
#endif                               // (B305 pro srovnani: snimek uz v poslednim radku, zvuk z pullAudioStereo)
    static bool minula = false;
    if (znacka && !minula) g_znackaSeq = (long long)g_segaProud.posledniSeq();
    minula = znacka;
    t += std::chrono::microseconds(16667);
    std::this_thread::sleep_until(t);
  }
}
static void zvukVlakno() {          // MainActivity: AudioTrack + pullAudioStereo po 1024 ramcich
  FakeShortArr a; a.v.assign(2048, 0);
  auto t = std::chrono::steady_clock::now();
  while (g_bezi.load()) {
    // na S8 drzi appka ve fronte pred reproduktorem ~6000 vzorku (~130 ms) - tady se
    // bere jen to, co je nad tim (B305 by tu frontu poslal i na TV)
    if (g_s8.load() && nap_audio_min_fifo() < nap_audio_target_fifo() + 1024) {
      t += std::chrono::microseconds(1024 * 1000000LL / 48000);
      std::this_thread::sleep_until(t);
      continue;
    }
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
void sega_s8(int z) { g_s8 = z != 0; }
long long sega_znacka_seq() { return g_znackaSeq.load(); }
int sega_fronta() { return (int)nap_audio_min_fifo(); }     // vzorku ve fronte reproduktoru telefonu
long long sega_snimku() { return g_snimku.load(); }
int sega_ocekavany(unsigned barva, uint32_t *out) {
  const unsigned r = (barva >> 16) & 255, g = (barva >> 8) & 255, b = barva & 255;
  const unsigned c9 = (r >> 5) | ((g >> 5) << 3) | ((b >> 5) << 6);
  std::lock_guard<std::mutex> l(g_mOcek);
  auto it = g_ocek.find(c9);
  if (it == g_ocek.end() || it->second.size() != (size_t)g_W * g_H) return 0;
  std::memcpy(out, it->second.data(), it->second.size() * 4);
  return (g_W << 16) | g_H;
}
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

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *, void *) {
  if (getenv("NAP_JVM_TEST")) { sega_start(); atexit(sega_stop); }
  return JNI_VERSION_1_6;
}
