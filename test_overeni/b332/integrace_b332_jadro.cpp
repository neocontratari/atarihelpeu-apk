// B332: KRESLENI PS1 VE VLAKNE NA DALSIM RYCHLEM JADRE - SE SKUTECNYM JADREM PS1 I VYKRESLOVACEM.
//
// Rene: "moc dobre vim, ze nepouzivas vsechna jadra k PS1". Log B330 (SpongeBob, S8): "8 cpu core(s)
// detected", "kresleni ve vlakne NE" - kresleni 2x (10-11 ms ze 12-13 ms snimku) na stejnem jadre jako
// emulace (cpu7), cpu4-6 bez prace. B332: jadro kresli ve vlastnim vlakne (gpulib gpu_async, volba
// pcsx_rearmed_gpu_thread_rendering = "auto" jako RetroArch), appka to vlakno da na ostatni rychla jadra.
// Tenhle test dokazuje (SKUTECNE: jadro PCSX-ReARMed z appky VCETNE gpulib + gpu_neon + gpu_async,
// nap_ps1_native.cpp z appky; program kresleni_test.exe kresli jako hra - retez paketu pres DMA, 2x):
//  M) vyber jader pro kresleni (falesne /sys jako S8, Retroid Pocket 6, Pixel, 2 jadra, 1 hlavni jadro);
//  V) vlakna jadra hlasi start a konec (TID kresleni), jadro opravdu kresli ve vlakne;
//  O) OBRAZ STEJNY: kazdy snimek (podle cisla snimku v obraze) s kreslenim ve vlakne bajt po bajtu
//     stejny jako bez nej, 2x (640x480) v obou; cteni VRAM zpet (GPUREAD) da stejna data;
//  T) emulace uz nekresli: cas vlakna emulace v GPU klesne, kresli kreslici vlakno (jeho cas procesoru);
//  S) ulozeni a nahrani pozice behem kresleni ve vlakne: po nahrani stejne snimky;
//  P) cesta telefonu (ps1BootDoMonitoru + vlakno emulace): emulace na cpu1, kresleni pripnute na cpu0,
//     radek VYKON s casem kresliciho vlakna, po zmene jader (jako Android) do 2 s znovu, konec vlakna.
// Kontejner ma 2 jadra (cpu0, cpu1) bez cpufreq - takt se podstrci falesnou slozkou.
//
// Preklad (PRAC/jadro_gpu = stavba_jadra_gpu.sh PRAC, kresleni_test.exe = udelej_kresleni_exe.py PRAC):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/integrace_b332 integrace_b332_jadro.cpp PRAC/jadro_gpu/*.o
//       -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b332 PRAC PRAC/kresleni_test.exe
#include "../../app/src/main/cpp/nap_ps1_native.cpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <map>
#include <sstream>
#include <sys/stat.h>

// vykreslovac je SKUTECNY (gpulib + gpu_neon z jadra) - zadne nahrady GPU jako v B317-B329

// ---------- okno Androidu (nepouziva se) ----------
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

// ---------- OpenSL ES: zvukova karta jako v testu B308 ----------
static const SLInterfaceID_ iidE{1}, iidP{2}, iidQ{3};
const SLInterfaceID SL_IID_ENGINE = &iidE;
const SLInterfaceID SL_IID_PLAY = &iidP;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &iidQ;
namespace sl {
static slAndroidSimpleBufferQueueCallback cb = nullptr;
static void *cbCtx = nullptr;
static std::thread karta;
static std::atomic<bool> hraje{false};
static SLresult realize(SLObjectItf, SLboolean) { return SL_RESULT_SUCCESS; }
static void destroy(SLObjectItf);
static SLresult objGetInterface(SLObjectItf self, const SLInterfaceID iid, void *p);
static const SLObjectItf_ objTbl = {realize, objGetInterface, destroy};
static const SLObjectItf_ *engineObj = &objTbl, *mixObj = &objTbl, *playerObj = &objTbl;
static SLresult createPlayer(SLEngineItf, SLObjectItf *p, SLDataSource *, SLDataSink *, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &playerObj; return SL_RESULT_SUCCESS; }
static SLresult createMix(SLEngineItf, SLObjectItf *p, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &mixObj; return SL_RESULT_SUCCESS; }
static const SLEngineItf_ engTbl = {createPlayer, createMix};
static const SLEngineItf_ *engItf = &engTbl;
static void spustKartu();
static SLresult setPlayState(SLPlayItf, SLuint32 st) { if (st == SL_PLAYSTATE_PLAYING) spustKartu(); return SL_RESULT_SUCCESS; }
static const SLPlayItf_ playTbl = {setPlayState};
static const SLPlayItf_ *playItf = &playTbl;
static SLresult enqueue(SLAndroidSimpleBufferQueueItf, const void *, SLuint32) { return SL_RESULT_SUCCESS; }
static SLresult registerCb(SLAndroidSimpleBufferQueueItf, slAndroidSimpleBufferQueueCallback c, void *ctx) { cb = c; cbCtx = ctx; return SL_RESULT_SUCCESS; }
static const SLAndroidSimpleBufferQueueItf_ qTbl = {enqueue, registerCb};
static const SLAndroidSimpleBufferQueueItf_ *qItf = &qTbl;
static SLresult objGetInterface(SLObjectItf, const SLInterfaceID iid, void *p) {
  if (iid == SL_IID_ENGINE) *(SLEngineItf *)p = &engItf;
  else if (iid == SL_IID_PLAY) *(SLPlayItf *)p = &playItf;
  else if (iid == SL_IID_ANDROIDSIMPLEBUFFERQUEUE) *(SLAndroidSimpleBufferQueueItf *)p = &qItf;
  else return 1;
  return SL_RESULT_SUCCESS;
}
static void spustKartu() {
  if (hraje.exchange(true)) return;
  karta = std::thread([] {
    auto t = std::chrono::steady_clock::now();
    while (hraje.load()) {
      t += std::chrono::microseconds(1024 * 1000000LL / 44100);
      std::this_thread::sleep_until(t);
      if (cb && hraje.load()) cb(&qItf, cbCtx);
    }
  });
}
static void destroy(SLObjectItf self) {
  if (self == &playerObj && hraje.exchange(false) && karta.joinable()) karta.join();
}
}  // namespace sl
SLresult slCreateEngine(SLObjectItf *p, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) { *p = &sl::engineObj; return SL_RESULT_SUCCESS; }

// ---------- rucne sestaveny JNIEnv (jako B308) ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, (size_t)n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.assign((size_t)n, 0); return (jbyteArray)a; }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static std::string jstr(jstring s) { if (!s) return std::string(); std::string r = ((FakeStr *)s)->s; delete (FakeStr *)s; return r; }
static void pripravEnv() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_tbl.NewByteArray = fNewByteArray;
  g_env.functions = &g_tbl;
}

// ---------- JNI presne jako Java (NativePs1CoreBridge) ----------
static std::string j_mysNastav(int rezim, int citlivost) {
  return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysNastav(&g_env, nullptr, rezim, citlivost));
}
static std::string j_mysStav() { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysStav(&g_env, nullptr)); }
static int j_dotyk(int akce, int id, float x, float y) {
  return Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1MysDotyk(&g_env, nullptr, akce, id, x, y, 732.f, 412.f);
}
static void j_tlacitko(int id, bool dolu) { Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetInput(&g_env, nullptr, id, dolu ? JNI_TRUE : JNI_FALSE); }
static std::string j_denik() {
  FakeArr *a = (FakeArr *)Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Denik(&g_env, nullptr);
  std::string r(a->b.begin(), a->b.end());
  delete a;
  return r;
}
static std::string j_pcMys(int dx, int dy, int tl) { return jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1PcMys(&g_env, nullptr, dx, dy, tl)); }

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string d2(double v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); return b; }
static std::string hx(uint64_t v) { char b[32]; snprintf(b, sizeof(b), "%016llx", (unsigned long long)v); return b; }

// falesna slozka /sys/devices/system/cpu: maxKhz[i] pro cpu0..n-1 (0 = jadro bez cpufreq)
static void falesneCpu(const std::string &koren, const std::vector<long> &maxKhz, long tedKhz = 0, const char *gov = nullptr) {
  std::string rm = "rm -rf '" + koren + "'";
  if (std::system(rm.c_str()) != 0) {}
  mkdir(koren.c_str(), 0755);
  for (size_t i = 0; i < maxKhz.size(); i++) {
    const std::string c = koren + "/cpu" + std::to_string(i);
    mkdir(c.c_str(), 0755);
    if (maxKhz[i] <= 0) continue;
    mkdir((c + "/cpufreq").c_str(), 0755);
    std::ofstream(c + "/cpufreq/cpuinfo_max_freq") << maxKhz[i] << "\n";
    std::ofstream(c + "/cpufreq/scaling_max_freq") << maxKhz[i] << "\n";
    std::ofstream(c + "/cpufreq/scaling_cur_freq") << (tedKhz > 0 ? tedKhz : maxKhz[i]) << "\n";
    if (gov) std::ofstream(c + "/cpufreq/scaling_governor") << gov << "\n";
  }
}
// vsem vlaknum procesu krome volajiciho nastavit jadra (jako Android pri zmene skupiny procesu)
static int vsemVlaknumJadra(unsigned long maska) {
  int n = 0;
  const long ja = syscall(__NR_gettid);
  if (DIR *d = opendir("/proc/self/task")) {
    while (dirent *e = readdir(d)) {
      const long tid = std::atol(e->d_name);
      if (tid <= 0 || tid == ja) continue;
      unsigned long m[256 / (8 * sizeof(unsigned long))] = {0};
      m[0] = maska;
      if (syscall(__NR_sched_setaffinity, tid, sizeof(m), m) == 0) n++;
    }
    closedir(d);
  }
  return n;
}
static unsigned long maskaVlakna(int tid) {
  unsigned long m[256 / (8 * sizeof(unsigned long))] = {0};
  if (syscall(__NR_sched_getaffinity, tid, sizeof(m), m) <= 0) return 0;
  return m[0];
}
static std::string poStartu(const std::string &dn) {
  const size_t p = dn.rfind("==================== PS1 START");
  return p == std::string::npos ? dn : dn.substr(p);
}
static std::string radekS(const std::string &dn, const std::string &co) {
  size_t v = dn.rfind(co);
  return v == std::string::npos ? std::string() : dn.substr(v, dn.find('\n', v) - v);
}
static std::string maskaPro(const std::vector<long> &khz, int emu, const std::string &f) {
  falesneCpu(f, khz);
  return ps1_maska_text(ps1_maska_kresleni(f.c_str(), emu));
}

// ---------- snimky: cislo snimku ze znacky vlevo nahore (program ho tam kresli), otisk celeho obrazu ----------
struct Snimek { int w = 0, h = 0, cislo = -1; uint64_t otisk = 0; };
static Snimek snimek() {
  Snimek s;
  std::lock_guard<std::mutex> l(g_frame_mutex);
  s.w = g_fw.load(); s.h = g_fh.load();
  if (s.w <= 0 || s.h <= 0 || g_frame_argb.size() < (size_t)s.w * s.h) return s;
  const uint32_t px = g_frame_argb[(size_t)2 * s.w + 2];   // uvnitr znacky (16x8, ve 2x 32x16)
  s.cislo = (int)(((px >> 16) & 0xff) >> 3) | (int)((((px >> 8) & 0xff) >> 3) << 5);
  uint64_t h = 1469598103934665603ull;
  const uint8_t *b = (const uint8_t *)g_frame_argb.data();
  for (size_t i = 0; i < (size_t)s.w * s.h * 4; i++) { h ^= b[i]; h *= 1099511628211ull; }
  s.otisk = h;
  return s;
}
static uint32_t ram32(uint32_t adr) {
  const uint8_t *ram = (const uint8_t *)retro_get_memory_data(2);
  uint32_t v = 0;
  std::memcpy(&v, ram + (adr & 0x1fffff), 4);
  return v;
}
struct Beh {
  std::map<int, uint64_t> otisky;              // cislo snimku -> otisk obrazu (prvni vyskyt)
  std::map<uint32_t, uint32_t> cteni;          // pocet cteni VRAM -> soucet (GPUREAD)
  int sirka = 0, vyska = 0, snimku = 0, spatnaVelikost = 0;
  double gpuMsNaKrok = 0, krokMs = 0, vlaknoMs = 0;
  int tid = 0, veVlakne = 0;
};
// N kroku jadra presne jako vlakno emulace (pod zamkem jadra); po kazdem snimek a vysledky programu
static void kroky(int n, Beh &b) {
  unsigned long long g0 = 0, o0 = 0, s0 = 0, v0 = 0, g1 = 0, o1 = 0, s1 = 0, v1 = 0;
  nap_mereni_stav(&g0, &o0, &s0, &v0);
  const int tid = g_ps1_tid_vlakna[2].load();
  const long long k0 = ps1_cas_vlakna_ns(tid);
  const auto t0 = std::chrono::steady_clock::now();
  for (int i = 0; i < n; i++) {
    { std::lock_guard<std::mutex> l(g_core_mutex); retro_run(); }
    const Snimek s = snimek();
    if (s.cislo < 0) continue;
    b.snimku++;
    if (b.sirka == 0) { b.sirka = s.w; b.vyska = s.h; }
    if (s.w != b.sirka || s.h != b.vyska) b.spatnaVelikost++;
    if (!b.otisky.count(s.cislo)) b.otisky[s.cislo] = s.otisk;
    const uint32_t pocet = ram32(0x80180008u);
    if (pocet && !b.cteni.count(pocet)) b.cteni[pocet] = ram32(0x80180004u);
  }
  const auto t1 = std::chrono::steady_clock::now();
  nap_mereni_stav(&g1, &o1, &s1, &v1);
  const long long k1 = ps1_cas_vlakna_ns(tid);
  b.gpuMsNaKrok += (double)(g1 - g0) / 1e6 / n;
  b.krokMs += std::chrono::duration<double, std::milli>(t1 - t0).count() / n;
  if (tid > 0 && k0 >= 0 && k1 >= 0) b.vlaknoMs += (double)(k1 - k0) / 1e6 / n;
  b.tid = tid;
  b.veVlakne = nap_ps1_kresleni_ve_vlakne ? nap_ps1_kresleni_ve_vlakne() : -1;
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b332 PRAC kresleni_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B332 - PS1 kresleni ve vlakne na dalsim rychlem jadre, SKUTECNE jadro PS1 vcetne vykreslovace "
              "(PCSX-ReARMed z appky: gpulib + gpu_neon + gpu_async, BIOS HLE, kresleni_test.exe)\n");
  const std::string sys = prac + "/system", karta = prac + "/karta_b332", f = prac + "/falesne_cpu";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  FakeStr diag{prac + "/nap_tv_session_log_b332.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;

  // ---- M: jadra pro kresleni ----
  std::string m = maskaPro({1690000, 1690000, 1690000, 1690000, 2314000, 2314000, 2314000, 2314000}, 7, f);
  over(m == "cpu4-6", "M1) Galaxy S8 (4 x 1690 + 4 x 2314 MHz), emulace cpu7: kresleni na " + m);
  m = maskaPro({2016000, 2016000, 2016000, 2803200, 2803200, 2803200, 2803200, 3187200}, 7, f);
  over(m == "cpu3-6", "M2) Retroid Pocket 6 / Snapdragon 8 Gen 2 (3 + 4 + 1 hlavni), emulace cpu7: kresleni na " + m);
  m = maskaPro({1800000, 1800000, 1800000, 1800000, 2350000, 2350000, 2850000, 2850000}, 7, f);
  over(m == "cpu4-6", "M3) Pixel (4 male + 2 stredni + 2 velka), emulace cpu7: kresleni na " + m + " (ne male cpu0-3)");
  m = maskaPro({1800000, 1800000, 1800000, 1800000, 1800000, 1800000, 1800000, 2400000}, 7, f);
  over(m == "cpu0-6", "M4) jen jedno rychle jadro (7 x 1800 + 1 x 2400), emulace cpu7: kresleni na ostatnich " + m);
  m = maskaPro({1000000, 2000000}, 1, f);
  over(m == "cpu0", "M5) dve jadra (jako tenhle kontejner s falesnym taktem), emulace cpu1: kresleni na " + m);
  Ps1Maska mk;
  for (int c : {0, 1, 2, 5}) { mk.m[0] |= 1UL << c; mk.pocet++; }
  over(ps1_maska_text(mk) == "cpu0-2,5" && ps1_maska_text(Ps1Maska()) == "zadna", "M6) text masky: " + ps1_maska_text(mk));

  // ---- V + O + T: primo pres jadro (ps1EglBoot + kroky), kresleni ve vlakne a bez nej ----
  FakeStr jsys{sys}, jhra{exe};
  const int N = 420;
  j_denik();
  g_ps1_kresleni_volba = "auto";
  int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  std::string dn = j_denik();
  const int tidA = g_ps1_tid_vlakna[2].load();
  over(boot >= 0 && tidA > 0 && nap_ps1_kresleni_ve_vlakne() == 1 && dn.find("VLAKNO JADRA kresleni: start (tid " + std::to_string(tidA)) != std::string::npos,
       "V1) volba \"auto\" (2 jadra): jadro kresli ve vlakne, vlakno se ohlasilo: \"" + radekS(dn, "VLAKNO JADRA kresleni") + "\"");
  char nast[400] = "";
  nap_ps1_nastaveni_popis(nast, sizeof(nast));
  over(std::string(nast).find("kresleni ve vlakne AUTO, zvuk ve vlakne NE") != std::string::npos,
       std::string("V2) nastaveni jadra: ...") + (std::strstr(nast, "kresleni") ? std::strstr(nast, "kresleni") : nast));
  Beh a;
  kroky(N, a);
  const uint32_t hotovoA = ram32(0x80180000u), znackaA = ram32(0x8018000cu);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  dn = j_denik();
  over(g_ps1_tid_vlakna[2].load() == 0 && dn.find("VLAKNO JADRA kresleni: konec (tid " + std::to_string(tidA)) != std::string::npos,
       "V3) po konci hry kreslici vlakno skoncilo a ohlasilo se: \"" + radekS(dn, "VLAKNO JADRA kresleni: konec") + "\"");

  g_ps1_kresleni_volba = "disabled";
  int boot2 = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot2 >= 0 && g_ps1_tid_vlakna[2].load() == 0 && nap_ps1_kresleni_ve_vlakne() == 0,
       "V4) volba \"disabled\" (srovnani): kresli vlakno emulace, zadne kreslici vlakno");
  Beh b;
  kroky(N, b);
  const uint32_t hotovoB = ram32(0x80180000u);
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  g_ps1_kresleni_volba = "auto";

  int spolecnych = 0, jinych = 0, prvniJiny = -1;
  for (const auto &p : a.otisky) {
    auto q = b.otisky.find(p.first);
    if (q == b.otisky.end()) continue;
    spolecnych++;
    if (q->second != p.second) { jinych++; if (prvniJiny < 0) prvniJiny = p.first; }
  }
  std::printf("   program: znacka %08x, snimku hotovo ve vlakne %u, bez vlakna %u | ruznych snimku v obraze %zu / %zu, %dx%d\n",
              znackaA, hotovoA, hotovoB, a.otisky.size(), b.otisky.size(), a.sirka, a.vyska);
  over(znackaA == 0xb332b332u && hotovoA > 300 && hotovoB > 300 && a.otisky.size() > 300 && b.otisky.size() > 300,
       "O1) program kresli a bezi v obou rezimech (snimku " + std::to_string(hotovoA) + " / " + std::to_string(hotovoB) + ", ruznych obrazu " +
       std::to_string(a.otisky.size()) + " / " + std::to_string(b.otisky.size()) + ")");
  over(a.sirka == 640 && a.vyska == 480 && b.sirka == 640 && b.vyska == 480 && !a.spatnaVelikost && !b.spatnaVelikost,
       "O2) 2x (Enhanced Resolution) v obou: obraz " + std::to_string(a.sirka) + "x" + std::to_string(a.vyska) + " / " +
       std::to_string(b.sirka) + "x" + std::to_string(b.vyska));
  over(spolecnych > 300 && jinych == 0,
       "O3) OBRAZ STEJNY: " + std::to_string(spolecnych) + " snimku (podle cisla v obraze) s kreslenim ve vlakne bajt po bajtu stejnych jako bez nej" +
       (jinych ? ", RUZNYCH " + std::to_string(jinych) + " (prvni c. " + std::to_string(prvniJiny) + ")" : std::string("")));
  int ctSpol = 0, ctJinych = 0;
  for (const auto &p : a.cteni) { auto q = b.cteni.find(p.first); if (q == b.cteni.end()) continue; ctSpol++; if (q->second != p.second) ctJinych++; }
  over(ctSpol >= 30 && ctJinych == 0, "O4) cteni VRAM zpet (GPUREAD, kazdy 8. snimek - kreslici vlakno musi dokreslit): " + std::to_string(ctSpol) +
       " cteni, soucty stejne jako bez vlakna (" + std::to_string(ctJinych) + " ruznych)");
  std::printf("   cas na krok: ve vlakne %.2f ms (z toho emulace v GPU %.2f ms, kreslici vlakno %.2f ms procesoru) | bez vlakna %.2f ms (v GPU %.2f ms)\n",
              a.krokMs, a.gpuMsNaKrok, a.vlaknoMs, b.krokMs, b.gpuMsNaKrok);
  over(a.gpuMsNaKrok < b.gpuMsNaKrok * 0.5, "T1) emulace uz nekresli: cas vlakna emulace v GPU " + d2(a.gpuMsNaKrok) + " ms/snimek misto " + d2(b.gpuMsNaKrok) + " ms");
  over(a.vlaknoMs > b.gpuMsNaKrok * 0.4, "T2) kresli kreslici vlakno: " + d2(a.vlaknoMs) + " ms procesoru na snimek (bez vlakna kreslila emulace " + d2(b.gpuMsNaKrok) + " ms)");

  // ---- S: ulozeni a nahrani pozice behem kresleni ve vlakne ----
  boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  Beh s0; kroky(120, s0);
  std::vector<uint8_t> poz(retro_serialize_size());
  bool ulozeno, nahrano;
  { std::lock_guard<std::mutex> l(g_core_mutex); ulozeno = retro_serialize(poz.data(), poz.size()); }
  Beh s1; kroky(90, s1);
  { std::lock_guard<std::mutex> l(g_core_mutex); nahrano = retro_unserialize(poz.data(), poz.size()); }
  Beh s2; kroky(90, s2);
  int sSpol = 0, sJinych = 0;
  for (const auto &p : s1.otisky) { auto q = s2.otisky.find(p.first); if (q == s2.otisky.end()) continue; sSpol++; if (q->second != p.second) sJinych++; }
  over(nap_ps1_kresleni_ve_vlakne() == 1 && ulozeno && nahrano && sSpol > 60 && sJinych == 0,
       "S1) pozice ulozena a nahrana behem kresleni ve vlakne (" + std::to_string(poz.size() / 1024) + " kB): po nahrani " + std::to_string(sSpol) +
       " snimku stejnych jako poprve (" + std::to_string(sJinych) + " ruznych)");
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);

  // ---- P: cesta telefonu - ps1BootDoMonitoru + vlakno emulace, pripnuti, VYKON, znovu pripnuti, konec ----
  falesneCpu(f, {1000000, 2000000}, 1500000, "schedutil");
  g_ps1_cpu_koren = strdup(f.c_str());
  FakeStr jkarta{karta};
  j_denik();
  const std::string rr = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&jsys, (jstring)&jkarta, (jstring)&jhra));
  std::this_thread::sleep_for(std::chrono::milliseconds(6500));
  dn = poStartu(j_denik());
  const int tidP = g_ps1_tid_vlakna[2].load();
  over(rr == "PS1_HRA_OK" && dn.find("VLAKNA: kresleni ve vlakne ANO") != std::string::npos,
       "P1) telefonni cesta: hra bezi (" + rr + "), \"" + radekS(dn, "VLAKNA: kresleni ve vlakne").substr(0, 70) + "...\"");
  over(tidP > 0 && dn.find("VLAKNA: kresleni (tid " + std::to_string(tidP) + ") pri startu na cpu0, emulace na cpu1 - kazde na jinem jadre") != std::string::npos
       && maskaVlakna(tidP) == 1UL,
       "P2) kresleni pripnute na cpu0, emulace na cpu1 (maska kresliciho vlakna " + std::to_string(maskaVlakna(tidP)) + "): \"" +
       radekS(dn, "VLAKNA: kresleni (tid").substr(0, 110) + "...\"");
  std::string vy = radekS(dn, "VYKON ");
  double krMs = -1;
  const size_t kv = vy.find("| kresleni ve vlakne ");
  if (kv != std::string::npos) std::sscanf(vy.c_str() + kv, "| kresleni ve vlakne %lf ms na snimek", &krMs);
  over(krMs > 0 && vy.find("ms na snimek (pripnuto), jadra cpu0 100 %") != std::string::npos && vy.find("+ kresleni (cekani na vlakno) ") != std::string::npos
       && vy.find("| jadro procesoru: cpu1 100 %") != std::string::npos,
       "P3) radek VYKON: emulace jen cpu1, kreslici vlakno " + d2(krMs) + " ms na snimek jen na cpu0 (\"" + (kv == std::string::npos ? std::string("-") : vy.substr(kv, 75)) + "...\")");
  double celk = -1, hra = -1, kr = -1, ob = -1, zv = -1;
  if (!vy.empty())
    std::sscanf(vy.c_str() + vy.find("snimek prumer"), "snimek prumer %lf ms = hra (procesor PS1) %lf + kresleni (cekani na vlakno) %lf + predani obrazu %lf + zvuk %lf",
                &celk, &hra, &kr, &ob, &zv);
  over(celk > 0 && std::fabs(hra + kr + ob + zv - celk) <= 0.25, "P4) VYKON z B328 dal plati: casti daji dohromady cely snimek (" + d2(celk) + " ms)");
  const int zmeneno = vsemVlaknumJadra(3UL);   // jako Android: vsem vlaknum zase obe jadra
  const unsigned long hned = maskaVlakna(tidP);
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));
  dn = poStartu(j_denik());
  over(zmeneno > 0 && hned == 3UL && maskaVlakna(tidP) == 1UL
       && dn.find("VLAKNA: Android kreslicimu vlaknu zmenil jadra - pripinam znovu na cpu0") != std::string::npos
       && dn.find("JADRO PROCESORU: Android vlaknu emulace zmenil jadra - pripinam znovu na cpu1") != std::string::npos,
       "P5) jadra zmenena vsem vlaknum (" + std::to_string(zmeneno) + ", jako Android) -> do 2 s zase kresleni cpu0 a emulace cpu1: \"" +
       radekS(dn, "VLAKNA: Android") + "\"");
  jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(&g_env, nullptr));
  nap_sl_close();
  const std::string dk = j_denik();
  over(g_ps1_tid_vlakna[2].load() == 0 && dk.find("VLAKNO JADRA kresleni: konec (tid " + std::to_string(tidP)) != std::string::npos,
       "P6) konec hry: kreslici vlakno skoncilo (\"" + radekS(dk, "VLAKNO JADRA kresleni: konec") + "\")");
  {
    std::ofstream o(prac + "/UKAZKA_denik_vlakna_b332.txt");
    o << poStartu(dk);
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
