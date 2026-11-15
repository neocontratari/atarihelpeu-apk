// B329: PS1 EMULACE NA JEDNOM NEJRYCHLEJSIM JADRE - SE SKUTECNYM JADREM PS1.
//
// Rene: "ano muzes samozrejme" (pripnout emulaci na jedno velke jadro - B328 VYKON ze SpongeBoba: kresleni
// 2x ~20 ms, velka jadra S8 jen 741-962 MHz, emulace preskakovala mezi cpu4-7). Tenhle test dokazuje:
//  C) vyber jadra z /sys (falesne slozky jako S8, Retroid Pocket 6, stejna jadra, zadny takt), takt do logu,
//     skutecne pripnuti vlakna na Linuxu (jen volajici vlakno), chyba pripnuti = emulace bezi jako drive;
//  D) cesta telefonu (ps1BootDoMonitoru + vlakno emulace): pripnuto na nejrychlejsi jadro, radek VYKON
//     s taktem "(pripnuto)", emulace bezi jen na tom jadre; kdyz vlaknu nekdo zmeni jadra (Android pri
//     zmene skupiny procesu), do 2 s se pripne znovu;
//  E) hra na pripnutem vlakne: pamet PS1 po 900 snimcich stejna jako bez pripnuti (pripnuti hru nemeni).
// SKUTECNE: jadro PCSX-ReARMed z appky (B328 obaly), nap_ps1_native.cpp z appky. Nahrazene (jako B317):
// vykreslovac gpu_neon, OpenSL ES, okno Androidu. Kontejner ma 2 jadra (cpu0, cpu1) bez cpufreq.
//
// Preklad (PRAC/jadro = ../b317/stavba_jadra.sh PRAC, mys_test.exe = ../b317/udelej_exe.py PRAC):
//   g++ -std=c++17 -O1 -g -pthread $PS1 -o PRAC/integrace_b329 integrace_b329_jadro.cpp PRAC/jadro/*.o \
//       -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b329 PRAC PRAC/mys_test.exe
#include "../../app/src/main/cpp/nap_ps1_native.cpp"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

// z jadra (frontend/libretro.c, libpcsxcore/pad.c): posun mysi za snimek a cteni portu 1 -
// test jimi zkousi, co hra dostane, kdyz port cte 2x v jednom snimku
extern "C" {
extern int in_mouse[8][2];
unsigned char PAD1_startPoll(void);
unsigned char PAD1_poll(unsigned char, int *);
}

// ---------- vykreslovac GPU (v appce gpu_neon + gpulib, ARM NEON) - grafika se tu nekresli ----------
extern "C" {
int n2_init(void) { return 0; }
void n2_finish(void) {}
void nap_gles_present_frame(void) {}
void nap_gles_sync_display_settings(void) {}
void nap_gles_readback_and_push(void) {}
unsigned nap_gles_grab_texture(int *, int *, int *, int *) { return 0; }
const void *nap_gles_grab_pixels(int *, int *) { return nullptr; }
int nap_gles_vram_w(void) { return 1024; }
int nap_gles_vram_h(void) { return 512; }
unsigned int nap_ps1_b308_tenkych(void) { return 0; }
unsigned int nap_ps1_gpu_otisk(void) { return 1; }
int nap_ps1_gpu_popis(char *buf, int len) { return snprintf(buf, (size_t)len, "obraz (test bez vykreslovace)"); }
// cip GPU: prikazy se zahodi, stav "pripraven" (hra / BIOS nikdy necekaji)
struct rearmed_cbs;
long GPUinit(void) { return 0; }
long GPUshutdown(void) { return 0; }
long GPUopen(unsigned long *, char *, char *) { return 0; }
long GPUclose(void) { return 0; }
uint32_t GPUreadStatus(void) { return 0x14802000u; }
uint32_t GPUreadData(void) { return 0; }
void GPUreadDataMem(uint32_t *p, int n) { if (p && n > 0) std::memset(p, 0, (size_t)n * 4); }
void GPUwriteStatus(uint32_t) {}
void GPUwriteData(uint32_t) {}
void GPUwriteDataMem(uint32_t *, int) {}
long GPUdmaChain(uint32_t *, uint32_t, uint32_t *, int32_t *) { return 0; }
long GPUfreeze(uint32_t, void *, uint16_t **) { return 1; }
void GPUvBlank(int, int) {}
void GPUgetScreenInfo(int *y, int *base_hres) { if (y) *y = 0; if (base_hres) *base_hres = 320; }
void GPUrearmedCallbacks(const struct rearmed_cbs *) {}
}

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

// ======================================================= B328 =======================================================
static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s  %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string t(unsigned long long v) { return std::to_string(v); }
static std::string ms(unsigned long long ns) { char b[32]; snprintf(b, sizeof(b), "%.1f ms", ns / 1e6); return b; }

// otisk cele pameti PS1 (2 MB RAM) - FNV-1a 64
static uint64_t otiskRam() {
  const uint8_t *ram = (const uint8_t *)retro_get_memory_data(2 /* RETRO_MEMORY_SYSTEM_RAM */);
  const size_t n = retro_get_memory_size(2);
  uint64_t h = 1469598103934665603ull;
  for (size_t i = 0; ram && i < n; i++) { h ^= ram[i]; h *= 1099511628211ull; }
  return h;
}
struct Mereni { unsigned long long gpu = 0, vout = 0, spu = 0, vol = 0; };
static Mereni mereni() { Mereni m; nap_mereni_stav(&m.gpu, &m.vout, &m.spu, &m.vol); return m; }
// presne N kroku jadra (bez casovani zvuku - stejny pocet snimku pokazde)
static void kroky(int n) { for (int i = 0; i < n; i++) { std::lock_guard<std::mutex> l(g_core_mutex); retro_run(); } }

#include <dirent.h>

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
// denik od posledniho startu hry (denik je spolecny pro cely test - starsi radky by se pletly)
static std::string poStartu(const std::string &dn) {
  const size_t p = dn.rfind("==================== PS1 START");
  return p == std::string::npos ? dn : dn.substr(p);
}
static std::string radekS(const std::string &dn, const std::string &co) {
  size_t v = dn.rfind(co);
  return v == std::string::npos ? std::string() : dn.substr(v, dn.find('\n', v) - v);
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b329 PRAC mys_test.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B329 - PS1 emulace na jednom nejrychlejsim jadre, SKUTECNE jadro PS1 (PCSX-ReARMed z appky, BIOS HLE, mys_test.exe)\n");
  const std::string sys = prac + "/system", karta = prac + "/karta_b329", f = prac + "/falesne_cpu";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  std::remove((karta + "/PS1_mys_pro_hry.txt").c_str());
  FakeStr diag{prac + "/nap_tv_session_log_b329.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;

  // ---- C1-C4: vyber jadra ----
  falesneCpu(f, {1690000, 1690000, 1690000, 1690000, 2314000, 2314000, 2314000, 2314000});
  Ps1Jadro j = ps1_nejrychlejsi_jadro(f.c_str());
  over(j.cpu == 7 && j.maxKhz == 2314000 && j.minKhz == 1690000 && j.jader == 8,
       "C1) jako Galaxy S8 (4 x 1690 + 4 x 2314 MHz): nejrychlejsi cpu" + std::to_string(j.cpu) + " " + std::to_string(j.maxKhz / 1000) + " MHz, jader " + std::to_string(j.jader));
  falesneCpu(f, {2016000, 2016000, 2016000, 2803200, 2803200, 2803200, 2803200, 3187200});
  j = ps1_nejrychlejsi_jadro(f.c_str());
  over(j.cpu == 7 && j.maxKhz == 3187200, "C2) jako Retroid Pocket 6 / Snapdragon 8 Gen 2 (3 + 4 + 1 hlavni 3187 MHz): cpu" + std::to_string(j.cpu));
  falesneCpu(f, {2400000, 2400000, 1800000, 1800000});
  j = ps1_nejrychlejsi_jadro(f.c_str());
  over(j.cpu == 1 && j.maxKhz == 2400000, "C3) rychla jadra na zacatku (2 x 2400 + 2 x 1800 MHz): vybrano cpu" + std::to_string(j.cpu) + " (ne posledni jadro)");
  falesneCpu(f, {0, 1500000, 0, 2500000});
  j = ps1_nejrychlejsi_jadro(f.c_str());
  over(j.cpu == 3 && j.jader == 2, "C4) jadra bez cpufreq (vypnuta) se preskoci: cpu" + std::to_string(j.cpu) + ", jader s taktem " + std::to_string(j.jader));

  // ---- C5-C7: kdy se NEpripina (emulace bezi jako drive) a co jde do deniku ----
  falesneCpu(f, {2000000, 2000000, 2000000, 2000000});
  j_denik();
  int r = -2;
  std::thread([&] { r = ps1_pripni_emulaci(f.c_str()); }).join();
  std::string dn = j_denik();
  over(r == -1 && dn.find("vsechna jadra stejne rychla (4 x 2000 MHz) - emulaci nepripinam") != std::string::npos,
       "C5) stejna jadra: nepripina se, denik \"" + radekS(dn, "JADRO PROCESORU") + "\"");
  falesneCpu(f, {0, 0});
  std::thread([&] { r = ps1_pripni_emulaci(f.c_str()); }).join();
  dn = j_denik();
  over(r == -1 && dn.find("takt jader nejde zjistit - emulaci nepripinam") != std::string::npos,
       "C6) takt nejde zjistit (jako tenhle kontejner): nepripina se, denik \"" + radekS(dn, "JADRO PROCESORU") + "\"");
  falesneCpu(f, {1690000, 1690000, 1690000, 1690000, 2314000, 2314000, 2314000, 2314000});
  std::thread([&] { r = ps1_pripni_emulaci(f.c_str()); }).join();
  dn = j_denik();
  over(r == -1 && dn.find("pripnout na cpu7 nejde (chyba 22) - emulace bezi jako drive") != std::string::npos,
       "C7) jadro, ktere system nepovoli (cpu7 tu neni): chyba se zapise a emulace bezi jako drive (\"" + radekS(dn, "JADRO PROCESORU") + "\")");

  // ---- C8-C9: takt do logu, skutecne pripnuti jen volajiciho vlakna ----
  falesneCpu(f, {1000000, 2000000}, 962000, "interactive");
  const std::string tk = ps1_takt_text(f.c_str(), 1);
  over(tk == "cpu1 962 MHz (smi 2000, umi 2000 MHz, interactive)", "C8) takt do logu: \"" + tk + "\"");
  unsigned long hlavniPred[4] = {0}, hlavniPo[4] = {0};
  syscall(__NR_sched_getaffinity, 0, sizeof(hlavniPred), hlavniPred);
  bool pripnuto = false, porad = false, jen0 = true;
  int bezel = -1;
  std::thread([&] {
    r = ps1_pripni_emulaci(f.c_str());
    pripnuto = ps1_vlakno_pripnute(1);
    jen0 = ps1_vlakno_pripnute(0);
    for (int i = 0; i < 2000; i++) { volatile double x = 0; for (int k = 0; k < 20000; k++) x += k; (void)x; if ((i & 63) == 0) sched_yield(); }
    bezel = ps1_jadro_ted();
    porad = ps1_vlakno_pripnute(1);
  }).join();
  syscall(__NR_sched_getaffinity, 0, sizeof(hlavniPo), hlavniPo);
  dn = j_denik();
  over(r == 1 && pripnuto && !jen0 && porad && bezel == 1,
       "C9) skutecne pripnuti (Linux, sched_setaffinity): vlakno bezi jen na cpu" + std::to_string(bezel) + ", maska jen cpu1");
  over(hlavniPred[0] == hlavniPo[0] && hlavniPo[0] == 3, "C10) ostatni vlakna se nemeni (hlavni vlakno porad cpu0 + cpu1, maska " + std::to_string(hlavniPo[0]) + ")");
  over(dn.find("emulace pripnuta na cpu1 (nejrychlejsi, umi 2000 MHz, nejpomalejsi jadro 1000 MHz, jader 2) - hra se nemeni, 2x zustava | takt ted: cpu1 962 MHz") != std::string::npos,
       "C11) denik: \"" + radekS(dn, "JADRO PROCESORU") + "\"");

  // ---- E: hra na pripnutem vlakne = stejna pamet PS1 jako bez pripnuti ----
  FakeStr jsys{sys}, jhra{exe};
  const int N = 900;
  int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  kroky(N);
  const uint64_t bez = otiskRam();
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  uint64_t s = 0;
  int boot2 = -1;
  std::thread([&] {
    ps1_pripni_vlakno(1);
    boot2 = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
    kroky(N);
    s = otiskRam();
    Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  }).join();
  over(boot >= 0 && boot2 >= 0 && s == bez && bez != 0,
       "E) hra " + std::to_string(N) + " snimku na vlaknu pripnutem na cpu1: pamet PS1 (2 MB) bajt po bajtu stejna jako bez pripnuti - pripnuti hru nemeni");

  // ---- D: cesta telefonu - ps1BootDoMonitoru + vlakno emulace, pripnuti, VYKON, znovu pripnuti ----
  falesneCpu(f, {1000000, 2000000}, 1500000, "schedutil");
  g_ps1_cpu_koren = strdup(f.c_str());
  FakeStr jkarta{karta};
  j_denik();
  const std::string rr = jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1BootDoMonitoru(&g_env, nullptr, (jstring)&jsys, (jstring)&jkarta, (jstring)&jhra));
  std::this_thread::sleep_for(std::chrono::milliseconds(6000));
  dn = poStartu(j_denik());
  over(rr == "PS1_HRA_OK" && dn.find("JADRO PROCESORU: emulace pripnuta na cpu1") != std::string::npos,
       "D1) telefonni cesta: hra bezi (" + rr + "), vlakno emulace pripnute: \"" + radekS(dn, "JADRO PROCESORU").substr(0, 90) + "...\"");
  std::string vy = radekS(dn, "VYKON ");
  over(vy.find("| jadro procesoru: cpu1 100 %") != std::string::npos && vy.find("| takt: cpu1 1500 MHz (smi 2000, umi 2000 MHz, schedutil) (pripnuto)") != std::string::npos,
       "D2) radek VYKON: emulace jen na cpu1 a takt (" + (vy.find("| jadro") == std::string::npos ? std::string("-") : vy.substr(vy.find("| jadro"))) + ")");
  double celk = -1, hra = -1, kr = -1, ob = -1, zv = -1;
  if (!vy.empty())
    std::sscanf(vy.c_str() + vy.find("snimek prumer"), "snimek prumer %lf ms = hra (procesor PS1) %lf + kresleni %lf + predani obrazu %lf + zvuk %lf",
                &celk, &hra, &kr, &ob, &zv);
  over(celk > 0 && std::fabs(hra + kr + ob + zv - celk) <= 0.25, "D3) radek VYKON z B328 dal plati: casti daji dohromady cely snimek");
  const int zmeneno = vsemVlaknumJadra(3UL);   // jako Android: vlaknum zase obe jadra
  std::this_thread::sleep_for(std::chrono::milliseconds(3500));
  dn = poStartu(j_denik());
  over(zmeneno > 0 && dn.find("JADRO PROCESORU: Android vlaknu emulace zmenil jadra - pripinam znovu na cpu1") != std::string::npos,
       "D4) vlaknum zmenena jadra (" + std::to_string(zmeneno) + " vlaken, jako Android) -> do 2 s znovu pripnuto: \"" + radekS(dn, "JADRO PROCESORU") + "\"");
  std::this_thread::sleep_for(std::chrono::milliseconds(6000));
  dn = poStartu(j_denik());
  vy = radekS(dn, "VYKON ");
  over(vy.find("| jadro procesoru: cpu1 100 %") != std::string::npos, "D5) po znovu pripnuti zase jen cpu1 (" + (vy.find("| jadro") == std::string::npos ? std::string("-") : vy.substr(vy.find("| jadro"), 40)) + ")");
  jstr(Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1Stop(&g_env, nullptr));
  nap_sl_close();
  {
    std::ofstream o(prac + "/UKAZKA_denik_jadro_b329.txt");
    o << dn;
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
