// test_overeni/b363/test_b363_atari_online.cpp
// B363: ONLINE ATARI 130XE - SKUTECNY nap_atari_native.cpp (vlakno emulace a kresleni jako v telefonu, Android
// nahrazen stuby z ../b291/stub), dva "telefony" = dva procesy, spojeni pres engine (mono NapOnlineTest = trida
// NapOnline z NapServer.cs v12.4.5 - engine se NEMENI). Jen JNI jako z Javy (devStart, devLoadXex, devPad, online*).
//   1) START ZE STAVU: telefon 1 hraje zatezovy program (XEX: POKEY 2 stereo, VBXE, rozsirena pamet, hraci a kolize,
//      preruseni casovace), telefon 2 ma Atari VYPNUTE a zadnou hru - po pripojeni dostane cely stav od telefonu 1 a
//      hraje se. Spicky site na obou telefonech, stisky (joystick, FIRE, START/SELECT/OPTION) se meni kazdych 5-85 ms:
//      odhady a opravy, otisky stavu po celou hru STEJNE, otisk stroje ve snimku 660 u obou stejny, STATISTIKA.
//   2) stroj telefonu 2 se ve snimku 300 "splete" + spicky -> synchronizace (stav od telefonu 1), hra pokracuje
//   3) telefon 2 zamrzne (SIGSTOP) -> telefon 1 jen par snimku s odhadem, pak stoji, po 15 s konec s duvodem
//   4) telefon 2 je "pomaly" (jadro +16 ms na snimek) -> SETRNE KRESLENI (obraz 25/s), hra jede, otisky stejne
//      (B364: prah setrneho kresleni 14 ms misto 10 ms - pomaly telefon proto +16 ms, drive +12 ms)
//   5) B364: telefon 2 ma jadro +9 ms (asi 12 ms na snimek - tolik meli S8 a A12 v testu Reneho 25. 11. a 50/s
//      stihaly) -> setrne kresleni se NEZAPNE (do B363 se zaplo od 10 ms a obraz byl zbytecne 25/s)
//   6) volitelne (jen na PC, hry nejsou v repozitari): hra z prikazove radky misto zatezoveho programu (XEX/CAR/ATR)
// Preklad (z test_overeni/b363, JAVA_HOME = JDK kvuli jni.h):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b363_atari_online test_b363_atari_online.cpp -lz   (vse na jednom radku)
//   ./test_b363_atari_online <port enginu> [hra.xex|hra.car|hra.atr]
#include <atomic>
#include <cstdint>
static void test_po_snimku(uint32_t snimek);
static void test_sit();
static std::atomic<uint32_t> g_test_zatez{0};
#define NAP_ATARI_ONLINE_TEST_PO_SNIMKU(snimek) test_po_snimku(snimek)
#define NAP_ATARI_ONLINE_TEST_ZATEZ_US (g_test_zatez.load())
#define NAP_ONLINE_TEST_SIT() test_sit()
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include "test_b363_stres.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <random>
#include <string>
#include <vector>

// ---------- nahrady OpenSL: zvuk "neni k dispozici" ----------
static const SLInterfaceID_ iidE{1}, iidP{2}, iidQ{3};
const SLInterfaceID SL_IID_ENGINE = &iidE;
const SLInterfaceID SL_IID_PLAY = &iidP;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &iidQ;
SLresult slCreateEngine(SLObjectItf *, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) { return 1; }

// ---------- nahrada okna displeje (kresleni bezi jako v telefonu) ----------
struct ANativeWindow { int w = 0, h = 0, fmt = 0; std::vector<uint32_t> buf; int refs = 1; int posts = 0; };
static ANativeWindow g_fakeWin;
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *w) { return w->w; }
int32_t ANativeWindow_getHeight(ANativeWindow *w) { return w->h; }
int32_t ANativeWindow_getFormat(ANativeWindow *w) { return w->fmt; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *w, int32_t width, int32_t height, int32_t format) {
  w->w = width; w->h = height; w->fmt = format; w->buf.assign((size_t)width * height, 0); return 0;
}
int32_t ANativeWindow_lock(ANativeWindow *w, ANativeWindow_Buffer *b, ARect *) {
  b->width = w->w; b->height = w->h; b->stride = w->w; b->format = w->fmt;
  w->buf.resize((size_t)b->stride * w->h); b->bits = w->buf.data(); return 0;
}
int32_t ANativeWindow_unlockAndPost(ANativeWindow *w) { w->posts++; return 0; }
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject s) { if (!s) return nullptr; g_fakeWin.refs++; return &g_fakeWin; }

// ---------- porucha stroje a otisk ve snimku 660 (jen tenhle test; vola vlakno emulace pod g_mStroj) ----------
static int g_test_porucha = -1;
static std::atomic<uint64_t> g_test_otisk660{0};
static std::atomic<uint32_t> g_test_snimku660{0};
static void test_po_snimku(uint32_t snimek) {
  if ((int)snimek == g_test_porucha && g_stroj) g_stroj->mem.ram[0x0700] ^= 0x10;   // program $0700 nepouziva = trvaly rozdil
  if (snimek == 660 && g_stroj) { g_test_otisk660.store(nap::atariOnlineOtisk(*g_stroj, g_onlineKontext, nullptr)); g_test_snimku660++; }
}

// ---------- spicky site: vlakno site obcas "usne" na 60-220 ms (jako Wi-Fi telefonu v logu S8 z 24. 11.) ----------
static bool g_test_spicky = false;
static std::mt19937 g_test_sit_rng(5);
static std::chrono::steady_clock::time_point g_test_dalsi_spicka;
static std::atomic<uint32_t> g_test_spicek{0}, g_test_spicky_ms{0};
static void test_sit() {
  if (!g_test_spicky) return;
  const auto t = std::chrono::steady_clock::now();
  if (g_test_dalsi_spicka.time_since_epoch().count() == 0) g_test_dalsi_spicka = t + std::chrono::milliseconds(800);
  if (t < g_test_dalsi_spicka) return;
  const int ms = 60 + (int)(g_test_sit_rng() % 161);
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
  g_test_spicek++;
  g_test_spicky_ms += (uint32_t)ms;
  g_test_dalsi_spicka = std::chrono::steady_clock::now() + std::chrono::milliseconds(500 + g_test_sit_rng() % 1000);
}

// ---------- rucne sestaveny JNIEnv (retezce, pole bajtu, pole retezcu) ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; std::vector<FakeStr *> p; bool retezce = false; };
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { const FakeArr *x = (const FakeArr *)a; return (jsize)(x->retezce ? x->p.size() : x->b.size()); }
static void JNICALL fGetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, jbyte *buf) { std::memcpy(buf, ((FakeArr *)a)->b.data() + st, n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.resize(n); return (jbyteArray)a; }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, n); }
static jobject JNICALL fGetObjectArrayElement(JNIEnv *, jobjectArray a, jsize i) { return (jobject)((FakeArr *)a)->p[(size_t)i]; }
static void JNICALL fDeleteLocalRef(JNIEnv *, jobject) {}
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static void pripravEnv() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.NewStringUTF = fNewStringUTF;
  g_tbl.GetStringUTFChars = fGetStringUTFChars;
  g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion;
  g_tbl.NewByteArray = fNewByteArray;
  g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.GetObjectArrayElement = fGetObjectArrayElement;
  g_tbl.DeleteLocalRef = fDeleteLocalRef;
  g_env.functions = &g_tbl;
}
static std::string vezmi(jstring j) {
  if (!j) return "(null)";
  std::string s = ((FakeStr *)j)->s;
  delete (FakeStr *)j;
  return s;
}

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static void spi(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

static std::string stav() { return vezmi(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStav(&g_env, nullptr)); }
static std::string pole(const std::string &s, const std::string &klic) {
  const std::string k = klic + "=";
  size_t i = 0;
  while (i < s.size()) {
    size_t e = s.find('\n', i);
    if (e == std::string::npos) e = s.size();
    if (s.compare(i, k.size(), k) == 0) return s.substr(i + k.size(), e - i - k.size());
    i = e + 1;
  }
  return "";
}
static std::vector<uint8_t> nacti(const std::string &f) {
  std::vector<uint8_t> d;
  FILE *fp = std::fopen(f.c_str(), "rb");
  if (!fp) return d;
  uint8_t b[65536];
  size_t n;
  while ((n = std::fread(b, 1, sizeof b, fp)) > 0) d.insert(d.end(), b, b + n);
  std::fclose(fp);
  return d;
}
static std::vector<uint8_t> xexStres() {
  std::vector<uint8_t> x = {0xFF, 0xFF, (uint8_t)(B363S_ORG & 0xFF), (uint8_t)(B363S_ORG >> 8)};
  const int konec = B363S_ORG + (int)sizeof(B363S_KOD) - 1;
  x.push_back((uint8_t)(konec & 0xFF)); x.push_back((uint8_t)(konec >> 8));
  x.insert(x.end(), B363S_KOD, B363S_KOD + sizeof(B363S_KOD));
  const uint8_t run[6] = {0xE0, 0x02, 0xE1, 0x02, (uint8_t)(B363S_START & 0xFF), (uint8_t)(B363S_START >> 8)};
  x.insert(x.end(), run, run + 6);
  return x;
}
static std::string logy() {   // radky logu appky (devPollLogNative)
  std::string r;
  for (;;) {
    jstring j = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
    if (!j) break;
    const std::string s = vezmi(j);
    if (s.empty()) break;
    r += s;
    if (r.back() != '\n') r += '\n';
  }
  return r;
}

// =====================================================================================================
//  jeden "telefon" (proces): ./test hrac <port> <1|2> <kod|-> <seed> <snimku> <porucha|-> <spicky 0/1> <zatez us> <hra|->
// =====================================================================================================
static int telefon(int port, bool hrac1, const std::string &kod, uint32_t seed, uint32_t snimku, int porucha, bool spicky, uint32_t zatez,
                   const std::string &hra) {
  pripravEnv();
  g_test_porucha = porucha;
  g_test_spicky = spicky;
  g_test_sit_rng.seed(seed * 7 + 1);
  FakeStr plocha{"plocha"};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&plocha, 1080, 2160);
  spi(1500);
  std::string popisHry = "zatezovy program";
  if (hrac1) {
    FakeArr a;
    std::string jm = "b363_stres.xex";
    if (hra != "-") { a.b = nacti(hra); jm = hra.substr(hra.find_last_of('/') + 1); popisHry = jm; }
    else a.b = xexStres();
    FakeStr n{jm};
    std::string pr = jm.size() > 4 ? jm.substr(jm.size() - 4) : jm;
    for (char &c : pr) c = (char)std::tolower((unsigned char)c);
    if (pr == ".car") std::printf("NAHRANO %s\n", vezmi(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadCarNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n)).c_str());
    else if (pr == ".atr") std::printf("NAHRANO %s\n", vezmi(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n)).c_str());
    else std::printf("NAHRANO %d\n", (int)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n));
    spi(hra != "-" ? 18000 : 4000);                  // hra se nahraje a rozbehne (titulka)
  } else {
    // telefon 2: Atari VYPNUTE (POWER), zadna hra - vse prijde ve stavu od telefonu 1
    { std::lock_guard<std::mutex> l(g_mEv); nap::dev::Ev e; e.t = nap::dev::Ev::POWER_OFF; e.v = 0; g_evQ.push_back(e); }
    spi(300);
    std::printf("VYPNUTO %d\n", g_strojBezi.load() ? 0 : 1);
  }
  {
    FakeStr info{hrac1 ? "TEST-S8 Android 9, Wi-Fi HIGH_PERF" : "TEST-A12 Android 12, Wi-Fi -61 dBm 2.4 GHz LOW_LATENCY"};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineTelefon(&g_env, nullptr, (jstring)&info);
  }
  std::string cesta;
  if (hrac1) cesta = "/api/v1/online/zaloz?system=atari&hra=B363%20TEST&hra_id=&otisk=0000000000000000000000000000000000000000&nastaveni=0&jmeno=Telefon%201";
  else cesta = "/api/v1/online/pripoj?kod=" + kod + "&jmeno=Telefon%202";
  FakeStr a{"http://127.0.0.1:" + std::to_string(port)}, c{cesta}, j{hrac1 ? "Telefon 1" : "Telefon 2"};
  FakeStr h1{"User-Agent: NapTest-B363"}, h2{std::string("X-NaP-Zarizeni: test-telefon-") + (hrac1 ? "1" : "2")};
  FakeArr hl;
  hl.retezce = true;
  hl.p = {&h1, &h2};
  const std::string st = vezmi(Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStart(&g_env, nullptr, (jstring)&a, (jstring)&c, (jobjectArray)&hl,
                                                                                        hrac1 ? JNI_TRUE : JNI_FALSE, (jstring)&j, JNI_FALSE));
  std::printf("START %s\n", st.c_str());
  std::fflush(stdout);
  auto ted = [] { return std::chrono::steady_clock::now(); };
  const auto t0 = ted();
  if (hrac1) {
    std::string k;
    while ((k = pole(stav(), "kod")).empty() && ted() - t0 < std::chrono::seconds(10)) spi(20);
    std::printf("KOD %s\n", k.empty() ? "-" : k.c_str());
    std::fflush(stdout);
  }
  std::string s;
  while (ted() - t0 < std::chrono::seconds(40)) {
    s = stav();
    if (pole(s, "hraje") == "1" || pole(s, "faze") == "KONEC" || pole(s, "aktivni") == "0") break;
    spi(10);
  }
  g_test_zatez.store(zatez);
  std::string videnoSync, videnoSetrne;
  if (pole(s, "hraje") == "1") {
    std::printf("HRAJE\n");
    std::fflush(stdout);
    std::mt19937 g(seed);
    int maska = 0;
    auto dalsiZmena = ted();
    const auto tHra = ted();
    while (ted() - tHra < std::chrono::seconds(70)) {
      if (ted() >= dalsiZmena) {
        maska ^= 1 << (g() % 8);                       // joystick, FIRE, START / SELECT / OPTION (devPad jako Java)
        if ((maska & 3) == 3) maska &= ~2;
        if ((maska & 12) == 12) maska &= ~8;
        Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, maska);
        dalsiZmena = ted() + std::chrono::milliseconds(5 + g() % 80);
      }
      spi(2);
      s = stav();
      if (pole(s, "sync") == "1") videnoSync = "1";
      if (pole(s, "setrne") == "1") videnoSetrne = "1";
      if (pole(s, "aktivni") != "1") break;
      if (hrac1 && (uint32_t)std::stoul("0" + pole(s, "snimek")) >= snimku) {
        Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStop(&g_env, nullptr);
        break;
      }
    }
    const auto tk = ted();
    while (!hrac1 && pole(stav(), "aktivni") == "1" && ted() - tk < std::chrono::seconds(10)) spi(10);
  }
  spi(200);
  s = stav();
  const std::string log = logy();
  auto vLogu = [&](const char *t) { return log.find(t) != std::string::npos ? 1 : 0; };
  int pc = 0, kod2000 = 0;
  {
    std::lock_guard<std::mutex> l(g_mStroj);
    if (g_stroj) { pc = g_stroj->cpu.pc; kod2000 = g_stroj->mem.ram[0x2000] | (g_stroj->mem.ram[0x2001] << 8); }
  }
  std::printf("VYSLEDEK hrac=%d overeno=%s snimek=%s zpozdeni=%s zpozdeni_ted=%s cekani=%s cekani_ms=%s synchronizaci=%s synchronizovano=%s "
              "rozjeti=%s rozjeti_snimek=%s stav_kb=%s otisk660=%016llx sync_videno=%s faze=%s start_stav=%s setrne_videno=%s setrne_c=%s "
              "kod2000=%04X\n",
              hrac1 ? 1 : 2, pole(s, "overeno").c_str(), pole(s, "snimek").c_str(), pole(s, "zpozdeni").c_str(), pole(s, "zpozdeni_ted").c_str(),
              pole(s, "cekani").c_str(), pole(s, "cekani_ms").c_str(), pole(s, "synchronizaci").c_str(), pole(s, "synchronizovano").c_str(),
              pole(s, "rozjeti").c_str(), pole(s, "rozjeti_snimek").c_str(), pole(s, "stav_kb").c_str(), (unsigned long long)g_test_otisk660.load(),
              videnoSync.empty() ? "0" : "1", pole(s, "faze").c_str(), pole(s, "start_stav").c_str(), videnoSetrne.empty() ? "0" : "1",
              pole(s, "setrne_c").c_str(), kod2000);
  std::printf("ODHAD odhad=%s sedelo=%s oprav=%s oprava_max=%s oprava_max_ms=%s prepocteno=%s odhad_max=%s chyb_opravy=%s druhy_info_c=%s "
              "posledni_zprava=%s spicek=%u spicky_ms=%u navrat_ze=%s navrat_na=%s\n",
              pole(s, "odhad").c_str(), pole(s, "odhad_sedelo").c_str(), pole(s, "oprav").c_str(), pole(s, "oprava_max").c_str(),
              pole(s, "oprava_max_ms").c_str(), pole(s, "prepocteno").c_str(), pole(s, "odhad_max").c_str(), pole(s, "chyb_opravy").c_str(),
              pole(s, "druhy_info_c").c_str(), pole(s, "posledni_zprava").c_str(), g_test_spicek.load(), g_test_spicky_ms.load(),
              pole(s, "navrat_ze").c_str(), pole(s, "navrat_na").c_str());
  std::printf("LOG start_odeslan=%d start_nahran=%d stav_ok=%d setrne_zap=%d konec=%d pc=%04X\n", vLogu("START: stav Atari pred snimkem 0 odeslan"),
              vLogu("START: stav Atari pred snimkem 0 nahran"), vLogu("hrac 2 stav nahral (STAV_OK"), vLogu("SETRNE KRESLENI zapnuto"),
              vLogu("B363 ATARI ONLINE konec"), pc);
  std::printf("DRUHY %s\n", pole(s, "druhy_info").c_str());
  std::printf("CASTI %s\nKONEC_TEXT %s\nCHYBA_TEXT %s\n", pole(s, "rozjeti_casti").c_str(), pole(s, "konec").c_str(), pole(s, "chyba").c_str());
  for (const char *k : {"START: stav", "synchronizace: stav", "SETRNE KRESLENI", "B363 ATARI ONLINE konec"}) {
    size_t i = log.find(k);
    if (i == std::string::npos) continue;
    const size_t z = log.rfind('\n', i), e = log.find('\n', i);
    std::printf("LOGRADEK %s\n", log.substr(z == std::string::npos ? 0 : z + 1, e == std::string::npos ? std::string::npos : e - (z == std::string::npos ? 0 : z + 1)).c_str());
  }
  std::fflush(stdout);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_onlineStop(&g_env, nullptr);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  return 0;
}

// =====================================================================================================
//  dva telefony (procesy) a engine
// =====================================================================================================
struct Dite {
  pid_t pid = -1;
  FILE *ven = nullptr;
  std::vector<std::string> radky;
};
static Dite spust(const std::vector<std::string> &arg) {
  int p[2];
  Dite d;
  if (pipe(p) != 0) return d;
  d.pid = fork();
  if (d.pid == 0) {
    dup2(p[1], 1);
    close(p[0]);
    close(p[1]);
    std::vector<char *> av;
    static std::string self = "/proc/self/exe";
    av.push_back((char *)self.c_str());
    for (const auto &x : arg) av.push_back((char *)x.c_str());
    av.push_back(nullptr);
    execv("/proc/self/exe", av.data());
    _exit(127);
  }
  close(p[1]);
  d.ven = fdopen(p[0], "r");
  return d;
}
static bool radek(Dite &d, std::string &r) {
  char b[8192];
  if (!d.ven || !fgets(b, sizeof(b), d.ven)) return false;
  r = b;
  while (!r.empty() && (r.back() == '\n' || r.back() == '\r')) r.pop_back();
  d.radky.push_back(r);
  return true;
}
static void dobehni(Dite &d) {
  std::string r;
  while (radek(d, r)) {}
  if (d.ven) fclose(d.ven);
  d.ven = nullptr;
  int st = 0;
  if (d.pid > 0) waitpid(d.pid, &st, 0);
}
static std::string hodnota(const Dite &d, const std::string &zacatek, const std::string &klic = "") {
  for (const auto &r : d.radky) {
    if (r.compare(0, zacatek.size(), zacatek) != 0) continue;
    if (klic.empty()) return r.substr(zacatek.size());
    const std::string k = " " + klic + "=";
    size_t i = r.find(k);
    if (i == std::string::npos) return "";
    i += k.size();
    return r.substr(i, r.find(' ', i) - i);
  }
  return "";
}
static int cislo(const std::string &s) { return s.empty() ? -1 : std::atoi(s.c_str()); }

static void dvojice(int port, const std::string &seedA, const std::string &seedB, const std::string &snimku, const std::string &poruchaB,
                    Dite &a, Dite &b, bool zmrazB = false, bool spicky = false, const std::string &zatezB = "0", const std::string &hra = "-") {
  const std::string sp = spicky ? "1" : "0";
  a = spust({"hrac", std::to_string(port), "1", "-", seedA, snimku, "-", sp, "0", hra});
  std::string r, kod;
  while (radek(a, r)) if (r.rfind("KOD ", 0) == 0) { kod = r.substr(4); break; }
  b = spust({"hrac", std::to_string(port), "2", kod.empty() ? "0000" : kod, seedB, snimku, poruchaB, sp, zatezB, "-"});
  if (zmrazB) {   // telefon 2: az hraje, po 3 s zamrznout cely proces (jako telefon, ktery prestal odpovidat)
    std::string rb;
    while (radek(b, rb)) if (rb == "HRAJE") break;
    std::this_thread::sleep_for(std::chrono::seconds(3));
    kill(b.pid, SIGSTOP);
    dobehni(a);                 // telefon 1 musi skoncit sam (ticho 15 s)
    kill(b.pid, SIGCONT);
    dobehni(b);
  } else {
    dobehni(a);
    dobehni(b);
  }
  for (const auto &x : a.radky) std::printf("   telefon 1 | %s\n", x.c_str());
  for (const auto &x : b.radky) std::printf("   telefon 2 | %s\n", x.c_str());
}

int main(int argc, char **argv) {
  if (argc >= 11 && std::string(argv[1]) == "hrac")
    return telefon(std::atoi(argv[2]), std::string(argv[3]) == "1", argv[4], (uint32_t)std::atoi(argv[5]), (uint32_t)std::atoi(argv[6]),
                   std::string(argv[7]) == "-" ? -1 : std::atoi(argv[7]), std::string(argv[8]) == "1", (uint32_t)std::atoi(argv[9]), argv[10]);
  pripravEnv();
  std::printf("B363 - online Atari 130XE (skutecny nap_atari_native.cpp, start ze stavu, odhad a oprava, setrne kresleni; protokol verze %u)\n",
              (unsigned)nap::online::VERZE);
  if (argc < 2) { std::printf("chybi port enginu\nVYSLEDEK: 0 OK, 1 CHYBA\n"); return 1; }
  const int port = std::atoi(argv[1]);
  const std::string hra = argc > 2 ? argv[2] : "-";

  std::printf("== 1) start ze stavu (telefon 2 ma Atari vypnute a zadnou hru), spicky site, stisky se meni porad\n");
  {
    Dite a, b;
    dvojice(port, "71", "72", "1260", "-", a, b, false, true, "0", hra);
    const int sa = cislo(hodnota(a, "VYSLEDEK", "snimek")), sb = cislo(hodnota(b, "VYSLEDEK", "snimek"));
    over(hodnota(b, "VYPNUTO ") == "1" && cislo(hodnota(a, "LOG", "start_odeslan")) == 1 && cislo(hodnota(b, "LOG", "start_nahran")) == 1 &&
             cislo(hodnota(a, "LOG", "stav_ok")) == 1 && hodnota(b, "VYSLEDEK", "start_stav") == "4",
         "start ze stavu: telefon 2 mel Atari vypnute, telefon 1 poslal cely stav (" + hodnota(a, "VYSLEDEK", "stav_kb") + " KB zlib), telefon 2 ho nahral, STAV_OK" +
             (hra == "-" ? ", na $2000 ma telefon 2 kod zatezoveho programu (" + hodnota(b, "VYSLEDEK", "kod2000") + ")" : ""));
    const int oprav = cislo(hodnota(a, "ODHAD", "oprav")) + cislo(hodnota(b, "ODHAD", "oprav"));
    const int odhad = cislo(hodnota(a, "ODHAD", "odhad")) + cislo(hodnota(b, "ODHAD", "odhad"));
    over(sa >= 1260 && sb >= 1200 && odhad > 0 && oprav > 0 && cislo(hodnota(a, "ODHAD", "chyb_opravy")) == 0 && cislo(hodnota(b, "ODHAD", "chyb_opravy")) == 0,
         "hra jela: telefon 1 " + std::to_string(sa) + " snimku, telefon 2 " + std::to_string(sb) + "; spicek " + hodnota(a, "ODHAD", "spicek") + " + " +
             hodnota(b, "ODHAD", "spicek") + " (" + hodnota(a, "ODHAD", "spicky_ms") + " + " + hodnota(b, "ODHAD", "spicky_ms") + " ms); odhadnuto " +
             hodnota(a, "ODHAD", "odhad") + " + " + hodnota(b, "ODHAD", "odhad") + " snimku, oprav " + hodnota(a, "ODHAD", "oprav") + " + " +
             hodnota(b, "ODHAD", "oprav") + " (nejdel " + hodnota(a, "ODHAD", "oprava_max") + " / " + hodnota(b, "ODHAD", "oprava_max") +
             " snimku, " + hodnota(a, "ODHAD", "oprava_max_ms") + " / " + hodnota(b, "ODHAD", "oprava_max_ms") + " ms), ulozeny stav vzdy byl");
    const std::string oa = hodnota(a, "VYSLEDEK", "otisk660"), ob = hodnota(b, "VYSLEDEK", "otisk660");
    over(oa == ob && oa != "0000000000000000" && cislo(hodnota(a, "VYSLEDEK", "synchronizaci")) == 0 && cislo(hodnota(b, "VYSLEDEK", "synchronizaci")) == 0 &&
             cislo(hodnota(a, "VYSLEDEK", "rozjeti")) == 0 && cislo(hodnota(a, "VYSLEDEK", "overeno")) >= 15,
         "oprava skutecneho Atari je presna: overeno " + hodnota(a, "VYSLEDEK", "overeno") + " otisku, rozjeti 0, synchronizaci 0, otisk stroje ve snimku 660 u obou " +
             oa + (oa == ob ? " (STEJNY)" : " / " + ob));
    over(cislo(hodnota(a, "VYSLEDEK", "zpozdeni")) <= 3,
         "zpozdeni ovladani " + hodnota(a, "VYSLEDEK", "zpozdeni") + " -> " + hodnota(a, "VYSLEDEK", "zpozdeni_ted") + " / " +
             hodnota(b, "VYSLEDEK", "zpozdeni_ted") + " snimku, Atari stalo " + hodnota(a, "VYSLEDEK", "cekani") + "x (" + hodnota(a, "VYSLEDEK", "cekani_ms") +
             " ms) / " + hodnota(b, "VYSLEDEK", "cekani") + "x (" + hodnota(b, "VYSLEDEK", "cekani_ms") + " ms), limit odhadu " +
             hodnota(a, "ODHAD", "odhad_max") + " / " + hodnota(b, "ODHAD", "odhad_max") + " snimku");
    const std::string da = hodnota(a, "DRUHY ");
    std::printf("       telefon 1 vi o telefonu 2: %s\n", da.c_str());
    over(cislo(hodnota(a, "ODHAD", "druhy_info_c")) >= 2 && da.find("TEST-A12 Android 12") != std::string::npos && da.find("snimku/s (obraz") != std::string::npos &&
             da.find("odhad ") != std::string::npos && hodnota(b, "DRUHY ").find("TEST-S8") != std::string::npos,
         "STATISTIKA: telefon 1 dostal od telefonu 2 " + hodnota(a, "ODHAD", "druhy_info_c") + "x jeho cisla (vykon jadra a obrazu, odhady, opravy, info z Javy)");
  }

  std::printf("== 2) stroj telefonu 2 se ve snimku 300 splete + spicky site -> synchronizace, hra pokracuje\n");
  {
    Dite a, b;
    dvojice(port, "11", "22", "780", "300", a, b, false, true, "0", hra);
    const int sa = cislo(hodnota(a, "VYSLEDEK", "snimek")), sb = cislo(hodnota(b, "VYSLEDEK", "snimek"));
    over(cislo(hodnota(a, "VYSLEDEK", "synchronizaci")) == 1 && cislo(hodnota(b, "VYSLEDEK", "synchronizovano")) == 1 &&
             hodnota(a, "VYSLEDEK", "rozjeti_snimek") == "300" && hodnota(a, "CASTI ") == "RAM 64K",
         "rozjeti poznano v otisku " + hodnota(a, "VYSLEDEK", "rozjeti_snimek") + " (" + hodnota(a, "CASTI ") + "), synchronizace hotova (" +
             hodnota(a, "VYSLEDEK", "synchronizovano") + " / " + hodnota(b, "VYSLEDEK", "synchronizovano") + "), stav " + hodnota(a, "VYSLEDEK", "stav_kb") +
             " KB" + (cislo(hodnota(b, "ODHAD", "navrat_ze")) > 0 ? ", telefon 2 vracen ze snimku " + hodnota(b, "ODHAD", "navrat_ze") + " na " + hodnota(b, "ODHAD", "navrat_na") : ""));
    const std::string oa = hodnota(a, "VYSLEDEK", "otisk660"), ob = hodnota(b, "VYSLEDEK", "otisk660");
    over(sa >= 780 && sb >= 700 && oa == ob && oa != "0000000000000000" && cislo(hodnota(a, "VYSLEDEK", "overeno")) >= 8,
         "hra pokracovala: " + std::to_string(sa) + " / " + std::to_string(sb) + " snimku, otisk ve snimku 660 u obou " + oa + (oa == ob ? " (STEJNY)" : " / " + ob) +
             ", overeno " + hodnota(a, "VYSLEDEK", "overeno") + ", oprav " + hodnota(a, "ODHAD", "oprav") + " + " + hodnota(b, "ODHAD", "oprav"));
  }

  std::printf("== 3) telefon 2 zamrzne -> telefon 1 jen par snimku s odhadem, pak stoji, po 15 s konec s duvodem\n");
  {
    Dite a, b;
    const auto t0 = std::chrono::steady_clock::now();
    dvojice(port, "55", "66", "100000", "-", a, b, true, false, "0", hra);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const std::string k = hodnota(a, "KONEC_TEXT ");
    over(k.rfind("od druheho hrace dlouho nic neprislo", 0) == 0 && hodnota(a, "VYSLEDEK", "faze") == "KONEC" &&
             cislo(hodnota(a, "ODHAD", "odhad")) <= 60 && cislo(hodnota(a, "LOG", "konec")) == 1,
         "telefon 1: '" + k + "' (cele trvani " + std::to_string((int)s) + " s), s odhadem jen " + hodnota(a, "ODHAD", "odhad") +
             " snimku, posledni zprava od druheho: " + hodnota(a, "ODHAD", "posledni_zprava") + ", Atari bezi dal samo");
  }

  std::printf("== 4) telefon 2 je pomaly (jadro +16 ms na snimek) -> setrne kresleni, hra jede dal stejne\n");
  {
    Dite a, b;
    dvojice(port, "31", "32", "900", "-", a, b, false, false, "16000", hra);
    const std::string oa = hodnota(a, "VYSLEDEK", "otisk660"), ob = hodnota(b, "VYSLEDEK", "otisk660");
    const std::string da = hodnota(a, "DRUHY ");
    over(hodnota(b, "VYSLEDEK", "setrne_videno") == "1" && cislo(hodnota(b, "LOG", "setrne_zap")) == 1 && hodnota(a, "VYSLEDEK", "setrne_videno") == "0" &&
             da.find("setrne") != std::string::npos,
         "telefon 2 si sam zapnul setrne kresleni (obraz 25/s, emulace 50/s), telefon 1 ne; telefon 1 to vi ze statistiky: " +
             (da.find("obraz") != std::string::npos ? da.substr(da.find("snimku/s") - 5, 60) : da));
    over(oa == ob && oa != "0000000000000000" && cislo(hodnota(a, "VYSLEDEK", "rozjeti")) == 0 && cislo(hodnota(a, "VYSLEDEK", "snimek")) >= 900,
         "hra s pomalym telefonem jede: " + hodnota(a, "VYSLEDEK", "snimek") + " snimku, otisk ve snimku 660 u obou " + oa + (oa == ob ? " (STEJNY)" : " / " + ob) +
             ", limit odhadu na pomalem telefonu " + hodnota(b, "ODHAD", "odhad_max"));
  }
  std::printf("== 5) B364: telefon 2 ma jadro +9 ms (asi 12 ms na snimek jako S8 / A12 v testu Reneho) -> 50/s stiha, setrne kresleni se NEZAPNE\n");
  {
    Dite a, b;
    dvojice(port, "41", "42", "900", "-", a, b, false, false, "9000", hra);
    const std::string oa = hodnota(a, "VYSLEDEK", "otisk660"), ob = hodnota(b, "VYSLEDEK", "otisk660");
    const std::string da = hodnota(a, "DRUHY ");
    const size_t j = da.find("jadro ");
    const std::string jadro = j == std::string::npos ? da : da.substr(j, da.find(')', j) == std::string::npos ? 30 : da.find(')', j) - j + 1);
    double ms = 0;
    if (j != std::string::npos) ms = std::atof(da.c_str() + j + 6);
    over(hodnota(b, "VYSLEDEK", "setrne_videno") == "0" && cislo(hodnota(b, "LOG", "setrne_zap")) == 0 && hodnota(a, "VYSLEDEK", "setrne_videno") == "0" &&
             ms > 10.0 && ms < 14.0 && da.find("obraz 50/s") != std::string::npos,
         "telefon 2 (" + jadro + ") setrne kresleni nezapnul - obraz dal 50/s (prah 14 ms; do B363 by se zaplo uz od 10 ms)");
    over(oa == ob && oa != "0000000000000000" && cislo(hodnota(a, "VYSLEDEK", "rozjeti")) == 0 && cislo(hodnota(a, "VYSLEDEK", "snimek")) >= 900,
         "hra jede: " + hodnota(a, "VYSLEDEK", "snimek") + " snimku, otisk ve snimku 660 u obou " + oa + (oa == ob ? " (STEJNY)" : " / " + ob));
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
