// B320: PS1 NA TV / PC SE SKUTECNYM VYKRESLOVACEM - posun obrazu a polovicni rozliseni na obrazech,
// ktere opravdu kresli gpu_neon z appky (2x Enhanced Resolution, dithering).
//
// Log B319 z telefonu (Dune 2000 po F5): "posun obrazu" zkousen 322x, nalezen 0x; polovicni rozliseni
// jen 304 casti z 1137. Na umele mape v testu B319 (PC) to fungovalo. Domnenka B320: PS1 kresli textury
// s ditheringem a mrizka 4x4 je ve 2x obrazu pevne na OBRAZOVCE. Tady se to overuje na SKUTECNEM
// vykreslovaci, ne na mem modelu:
//  - jadro PCSX-ReARMed z appky (../b318/stavba_jadra.sh), BIOS HLE,
//  - vykreslovac gpulib + gpu_neon z appky (stavba_vykreslovace.sh - SIMD varianta, na PC SSE),
//  - nap_ps1_native.cpp z appky: volby jadra jako v telefonu (Enhanced Resolution zapnuto, dithering
//    podle jadra = zapnuto), obraz pres retro_video_refresh -> nap_video -> g_frame_argb, presne ten
//    snimek, ktery vlakno emulace v telefonu dava proudu na TV (g_ps1Proud.snimek),
//  - "hra" mapa_dune.exe (mapa_dune.c): Dune jako 2D strategie, kresli 20 obrazu za sekundu; mapa jako
//    polygony s texturou + dithering (rezim 0) nebo jako sprity (rezim 1, PS1 je neditheruje).
// Stejne snimky jdou do tri kodeku: drive (bez voleb, B318), B319 (posun + polovicni PRESNE) a B320
// (nastavKodekPs1 - tolerance ditheringu, jako proudOtevri2Native v appce). Prohlizec v C++ = stranka
// WEB TV B319 (beze zmeny), obraz se porovnava se snimkem z jadra.
//
// Preklad (PRAC/jadro = ../b318/stavba_jadra.sh, PRAC/vykreslovac = stavba_vykreslovace.sh):
//   g++ -std=c++17 -O2 -g -pthread $PS1 -I../../app/src/main/cpp/vendor/stb -o PRAC/integrace_b320_gpu \
//       integrace_b320_gpu.cpp PRAC/jadro/*.o PRAC/vykreslovac/*.o -l:libEGL.so.1 -l:libGLESv2.so.2 -lz -ldl
//   EGL_PLATFORM=surfaceless ./integrace_b320_gpu PRAC PRAC/mapa_dune.exe
#include "../../app/src/main/cpp/nap_ps1_native.cpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <sys/stat.h>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#include "stb_image.h"
#include "../b319/prohlizec_b319.h"

extern "C" {
#include "../../app/src/main/cpp/vendor/pcsx_rearmed/frontend/plugin_lib.h"   // pl_rearmed_cbs (dithering jadra)
}

using namespace nap::proud;

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

// ---------- OpenSL ES: zvukova karta jako v testu B318 (zvuk se tu nehodnoti) ----------
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

// ---------- rucne sestaveny JNIEnv (jako B318) ----------
struct FakeStr { std::string s; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;
static void pripravEnv() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF;
  g_env.functions = &g_tbl;
}

// ---------- vymena s hrou mapa_dune.exe (pamet PS1 0x80100000) ----------
enum { S_ZNACKA = 0, S_OBRAZU = 1, S_CAMX = 2, S_CAMY = 3, S_VBLANKU = 4, S_REZIM_HRA = 5, S_REZIM = 8, S_DX = 9, S_DY = 10, S_ZASTAV = 11 };
static volatile uint32_t *stav() { return (volatile uint32_t *)((uint8_t *)retro_get_memory_data(2 /* RETRO_MEMORY_SYSTEM_RAM */) + 0x100000); }

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}
static std::string t(long long v) { return std::to_string(v); }
static std::string f1(double v) { char b[32]; snprintf(b, sizeof(b), "%.1f", v); return b; }
static std::string f2(double v) { char b[32]; snprintf(b, sizeof(b), "%.2f", v); return b; }

// ---------- tri kodeky na stejnych snimcich ----------
enum { DRIVE = 0, B319 = 1, B320 = 2 };
static const char *JMENO[3] = {"drive", "B319", "B320"};
struct Proud {
  Hub<KodekJpeg> hub;
  int id = 0;
  ProhlizecB319 pr;
  std::vector<uint8_t> pk;
  std::string log;
  explicit Proud(int druh) : hub("PS1", 44100, 4, 3) {
    if (druh == B319) hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); k.tolerance = 0; }, "B319 presne");
    else hub.nastavVolby([](KodekJpeg &k, int v) { nastavKodekPs1(k, v); }, "B320 jako appka");
    id = hub.otevri(druh == DRIVE ? 0 : (VOLBA_POSUN | VOLBA_POLOVICNI));
  }
  long long posli(const std::vector<uint32_t> &F, int W, int H) {
    hub.snimek(F.data(), W, H);
    const int r = hub.dalsi(id, 200, 0, pk);
    if (r != 1) { pr.chyb++; return 0; }
    pr.prijmi(pk);
    return (long long)pk.size();
  }
  std::string zavri() { hub.zavri(id); log = hub.vezmiLog(); return log; }
};

// vysledky jedne faze (posun mapy jednim smerem) pro jeden kodek
struct Faze {
  const char *jmeno;
  int dx, dy, obrazu;
  bool zastav;
};
struct Mereni {
  long long bajtu = 0;
  int obrazu = 0, sKopii = 0;
  double psnrSoucet = 0, psnrMin = 99, nejCtverec = 0;
  long long polovicnich0 = 0, casti0 = 0;
};

// ---------- rozbor skutecnych snimku gpu_neon ----------
// podil bloku 2x2 ve vyhledu na mapu, ktere maji 4 STEJNE body / lisi se nejvys o 1 odstin;
// mrizka 2x2 zacina na (o, o) - 0 = zarovnana s obrazem, 1 = posunuta o bod
static void rozbor2x2(const std::vector<uint32_t> &F, int W, int X1, int Y1, double &stejnych, double &do1, int o = 0) {
  long long n = 0, s = 0, d = 0;
  for (int y = o; y + 1 < Y1; y += 2)
    for (int x = o; x + 1 < X1; x += 2) {
      const uint32_t a = F[(size_t)y * W + x], b = F[(size_t)y * W + x + 1], c = F[(size_t)(y + 1) * W + x], e = F[(size_t)(y + 1) * W + x + 1];
      n++;
      if (a == b && a == c && a == e) { s++; d++; continue; }
      auto blz = [](uint32_t p, uint32_t q) {
        for (int k = 0; k < 24; k += 8) if (std::abs((int)((p >> k) & 255) - (int)((q >> k) & 255)) > 10) return false;
        return true;
      };
      if (blz(a, b) && blz(a, c) && blz(a, e)) d++;
    }
  stejnych = n ? 100.0 * s / n : 0;
  do1 = n ? 100.0 * d / n : 0;
}
// novy snimek = minuly posunuty o (vx, vy) bodu? Podil bodu vyhledu (bez okraju) presne stejnych / do 1 odstinu
static void rozborPosunu(const std::vector<uint32_t> &F, const std::vector<uint32_t> &P, int W, int X1, int Y1, int vx, int vy,
                         double &presne, double &do1) {
  long long n = 0, s = 0, d = 0;
  for (int y = 40; y < Y1 - 40; y++)
    for (int x = 40; x < X1 - 40; x++) {
      const int sx = x - vx, sy = y - vy;
      if (sx < 0 || sy < 0 || sx >= X1 || sy >= Y1) continue;
      const uint32_t a = F[(size_t)y * W + x], b = P[(size_t)sy * W + sx];
      n++;
      if (a == b) { s++; d++; continue; }
      bool ok = true;
      for (int k = 0; k < 24; k += 8) if (std::abs((int)((a >> k) & 255) - (int)((b >> k) & 255)) > 10) ok = false;
      if (ok) d++;
    }
  presne = n ? 100.0 * s / n : 0;
  do1 = n ? 100.0 * d / n : 0;
}

int main(int argc, char **argv) {
  if (argc < 3) { std::fprintf(stderr, "pouziti: integrace_b320_gpu PRAC mapa_dune.exe\n"); return 2; }
  const std::string prac = argv[1], exe = argv[2];
  pripravEnv();
  std::printf("B320 - PS1 na TV / PC se SKUTECNYM vykreslovacem: jadro PCSX-ReARMed + gpu_neon z appky (2x Enhanced Resolution),\n"
              "       nap_ps1_native.cpp (volby jadra jako telefon), hra mapa_dune.exe (Dune jako 2D strategie, 20 obrazu / s)\n\n");
  const std::string sys = prac + "/system_b320", karta = prac + "/karta_b320";
  mkdir(sys.c_str(), 0755);
  mkdir(karta.c_str(), 0755);
  FakeStr diag{prac + "/nap_tv_session_log_b320.txt"};
  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1SetDiagLogPath(&g_env, nullptr, (jstring)&diag);
  g_savedir = karta;
  FakeStr jsys{sys}, jhra{exe};
  const int boot = Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglBoot(&g_env, nullptr, (jstring)&jsys, (jstring)&jhra);
  over(boot >= 0 && g_loaded.load(), "jadro nastartovalo hru mapa_dune.exe (ps1EglBoot = " + t(boot) + ", vykreslovac: " +
                                          std::string(boot == 1 ? "gpu_neon pres nap_most.c jako v telefonu" : "?") + ")");
  auto tick = [] { Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglTick(&g_env, nullptr); };
  int n = 0;
  while (stav()[S_ZNACKA] != 0x454E5544u && n < 600) { tick(); n++; }
  for (int i = 0; i < 30; i++) tick();
  const uint32_t ob0 = stav()[S_OBRAZU];
  over(stav()[S_ZNACKA] == 0x454E5544u && ob0 >= 8, "hra bezi (po " + t(n) + " snimcich jadra), kresli: " + t(ob0) + " obrazu za 30 snimku (20 / s)");

  std::vector<uint32_t> F, P;     // F = snimek z jadra (jako g_ps1Proud.snimek v telefonu), P = minuly nakresleny obraz hry
  int W = 0, H = 0;
  auto snimek = [&]() {
    std::lock_guard<std::mutex> fl(g_frame_mutex);
    W = g_fw.load(); H = g_fh.load();
    if (W > 0 && H > 0 && g_frame_argb.size() >= (size_t)W * H) F.assign(g_frame_argb.begin(), g_frame_argb.begin() + (size_t)W * H);
  };
  tick();
  snimek();
  std::string pop;
  {
    char b[256] = "";
    nap_ps1_gpu_popis(b, sizeof(b));
    pop = b;
  }
  over(W == 736 && H == 480, "obraz z jadra " + t(W) + "x" + t(H) + " = 368x240 hry ve 2x Enhanced Resolution (jako Dune 2000 v logu B318 / B319)");
  std::printf("   (vykreslovac: %s; dithering jadra: %s)\n", pop.c_str(),
              pl_rearmed_cbs.dithering == 0 ? "VYPNUTO" : pl_rearmed_cbs.dithering == 2 ? "VYNUCENO" : "ZAPNUTO");

  const Faze FAZE[] = {
      {"mapa stoji", 0, 0, 10, false},
      {"doprava 1 bod", 1, 0, 20, false},
      {"dolu 1", 0, 1, 20, false},
      {"sikmo 1,1", 1, 1, 15, false},
      {"doleva 3", -3, 0, 15, false},
      {"nahoru 2", 0, -2, 15, false},
      {"sikmo 3,-1", 3, -1, 15, false},
      {"doprava 4", 4, 0, 15, false},
      {"vse stoji", 0, 0, 20, true},
  };
  const int NF = (int)(sizeof(FAZE) / sizeof(FAZE[0]));
  double rozbStejnych[2] = {0, 0}, rozbDo1[2] = {0, 0}, rozbPosDo1[2] = {0, 0}, posPresne[2][2] = {{0, 0}, {0, 0}}, posDo1[2][2] = {{0, 0}, {0, 0}};
  int posN[2][2] = {{0, 0}, {0, 0}};
  Mereni celkem[2][3];
  Mereni posun[2][3], lich[2][3];
  double klid[2][3];
  int nejhorsiFaze320[2] = {100, 100};
  std::string logy[2][3];

  for (int rezim = 0; rezim < 2; rezim++) {
    std::printf("\n   == rezim %d: mapa jako %s ==\n", rezim,
                rezim == 0 ? "POLYGONY s texturou, modulaci a DITHERINGEM (E1 bit 9)" : "SPRITY (obdelniky s texturou - PS1 je neditheruje)");
    stav()[S_REZIM] = (uint32_t)rezim;
    stav()[S_DX] = 0; stav()[S_DY] = 0; stav()[S_ZASTAV] = 0;
    for (int i = 0; i < 9; i++) tick();          // hra prejde na novy rezim (3 obrazy)
    snimek();
    P = F;
    Proud *p[3] = {new Proud(DRIVE), new Proud(B319), new Proud(B320)};
    for (int k = 0; k < 3; k++) for (int i = 0; i < 3; i++) p[k]->posli(F, W, H);   // klicovy snimek a usazeni
    int rozbN = 0;
    for (int fi = 0; fi < NF; fi++) {
      const Faze &f = FAZE[fi];
      stav()[S_DX] = (uint32_t)f.dx; stav()[S_DY] = (uint32_t)f.dy; stav()[S_ZASTAV] = f.zastav ? 1u : 0u;
      const bool jeP = f.dx || f.dy;
      const bool jinaFaze = ((f.dx & 1) != (f.dy & 1));
      int fKopii[3] = {0, 0, 0}, fObrazu = 0;
      uint32_t obr = stav()[S_OBRAZU];
      int obrFaze = 0;
      while (obrFaze < f.obrazu) {
        tick();
        snimek();
        const uint32_t o2 = stav()[S_OBRAZU];
        const bool novy = o2 != obr;
        long long b[3];
        for (int k = 0; k < 3; k++) b[k] = p[k]->posli(F, W, H);
        if (!novy) {
          for (int k = 0; k < 3; k++) { celkem[rezim][k].bajtu += b[k]; if (jeP) posun[rezim][k].bajtu += b[k]; if (jeP && jinaFaze) lich[rezim][k].bajtu += b[k]; }
          continue;
        }
        obr = o2;
        obrFaze++;
        // prvni obraz faze ma jeste posun minule faze (hra cte dx, dy pri kresleni) - nehodnoti se
        const bool hodnotit = obrFaze > 1;
        for (int k = 0; k < 3; k++) {
          Mereni *m[3] = {&celkem[rezim][k], jeP ? &posun[rezim][k] : nullptr, (jeP && jinaFaze) ? &lich[rezim][k] : nullptr};
          const double ps = psnrB(F, p[k]->pr.canvas, W, 0, 0, W, H);
          const double ct = nejhorsiCtverec(F, p[k]->pr.canvas, W, H);
          for (Mereni *mm : m) {
            if (!mm) continue;
            mm->bajtu += b[k];
            mm->obrazu++;
            mm->psnrSoucet += ps; mm->psnrMin = std::min(mm->psnrMin, ps); mm->nejCtverec = std::max(mm->nejCtverec, ct);
            if (hodnotit && jeP && p[k]->pr.kopiiPoslednihoSnimku > 0) mm->sKopii++;
          }
          if (hodnotit && jeP && p[k]->pr.kopiiPoslednihoSnimku > 0) fKopii[k]++;
        }
        if (hodnotit && jeP) fObrazu++;
        // rozbor skutecneho obrazu: 2x2 bloky a posun proti minulemu obrazu (vyhled 592x480)
        double s1, d1, s2, d2;
        rozbor2x2(F, W, 592, 480, s1, d1);
        rozbor2x2(F, W, 592, 480, s2, d2, 1);
        rozbStejnych[rezim] += s1; rozbDo1[rezim] += d1; rozbPosDo1[rezim] += d2; rozbN++;
        if (hodnotit && jeP && std::abs(f.dx) <= 4 && std::abs(f.dy) <= 4) {
          double pp, pd;
          rozborPosunu(F, P, W, 592, 480, -2 * f.dx, -2 * f.dy, pp, pd);
          const int j = jinaFaze ? 1 : 0;
          posPresne[rezim][j] += pp; posDo1[rezim][j] += pd; posN[rezim][j]++;
        }
        P = F;
      }
      if (fi == NF - 1) {
        // "vse stoji": po zastaveni zjemneni - vernost posledniho obrazu
        for (int k = 0; k < 3; k++) klid[rezim][k] = psnrB(F, p[k]->pr.canvas, W, 0, 0, W, H);
      }
      if (jeP) {
        std::printf("      %-14s kopie: B319 %2d z %2d, B320 %2d z %2d%s\n", f.jmeno, fKopii[B319], fObrazu, fKopii[B320], fObrazu,
                    jinaFaze ? "   (jina faze ditheringu)" : "");
        nejhorsiFaze320[rezim] = std::min(nejhorsiFaze320[rezim], 100 * fKopii[B320] / std::max(1, fObrazu));
      }
    }
    for (int k = 0; k < 3; k++) logy[rezim][k] = p[k]->zavri();
    rozbStejnych[rezim] /= std::max(1, rozbN); rozbDo1[rezim] /= std::max(1, rozbN); rozbPosDo1[rezim] /= std::max(1, rozbN);
    for (int j = 0; j < 2; j++) { posPresne[rezim][j] /= std::max(1, posN[rezim][j]); posDo1[rezim][j] /= std::max(1, posN[rezim][j]); }
    std::printf("      skutecny obraz (vyhled na mapu 592x480): bloku 2x2 se 4 stejnymi body %.1f %%, lisicich se nejvys o 1 odstin %.1f %%, s mrizkou posunutou o bod %.1f %%\n",
                rozbStejnych[rezim], rozbDo1[rezim], rozbPosDo1[rezim]);
    std::printf("      obraz po posunu = minuly obraz posunuty: pri stejne fazi ditheringu presne %.1f %% bodu (do 1 odstinu %.1f %%), pri jine fazi presne %.1f %% (do 1 odstinu %.1f %%)\n",
                posPresne[rezim][0], posDo1[rezim][0], posPresne[rezim][1], posDo1[rezim][1]);
    for (int k = 0; k < 3; k++) {
      const Mereni &m = posun[rezim][k];
      std::printf("      %-5s pri posunu mapy: %5.1f kB / obraz hry (%.2f Mbit/s pri 20 obrazech / s), kopie v %d obrazech, PSNR prumer %.2f nejhur %.2f dB, nejhorsi ctverec %.1f\n",
                  JMENO[k], m.bajtu / 1024.0 / std::max(1, m.obrazu), m.bajtu * 8.0 / 1e6 / std::max(1, m.obrazu) * 20, m.sKopii,
                  m.psnrSoucet / std::max(1, m.obrazu), m.psnrMin, m.nejCtverec);
    }
    std::printf("      polovicni rozliseni: B319 %lld z %lld casti, B320 %lld z %lld; po zastaveni PSNR: drive %.2f, B319 %.2f, B320 %.2f dB\n",
                p[B319]->pr.polovicnich, p[B319]->pr.casti, p[B320]->pr.polovicnich, p[B320]->pr.casti, klid[rezim][0], klid[rezim][1], klid[rezim][2]);
    std::string l = logy[rezim][B320];
    for (size_t q; (q = l.find('\n')) != std::string::npos; l.erase(0, q + 1))
      if (l.find("odpojen") != std::string::npos) std::printf("      log B320: %s\n", l.substr(0, q).c_str());
    celkem[rezim][0].polovicnich0 = p[B319]->pr.polovicnich; celkem[rezim][0].casti0 = p[B319]->pr.casti;
    celkem[rezim][1].polovicnich0 = p[B320]->pr.polovicnich; celkem[rezim][1].casti0 = p[B320]->pr.casti;
    int chyb = 0;
    for (int k = 0; k < 3; k++) chyb += (int)p[k]->pr.chyb + (int)p[k]->pr.spatnyPriznak;
    celkem[rezim][2].casti0 = chyb;
    for (int k = 0; k < 3; k++) delete p[k];
  }

  std::printf("\n");
  // ---- co dela skutecny vykreslovac ----
  over(rozbStejnych[1] >= 99.0,
       "R1) sprity (bez ditheringu) ve 2x: " + f1(rozbStejnych[1]) + " % bloku 2x2 ma 4 stejne body - presne 2x zvetseny obraz (B319 polovicni rozliseni plati)");
  over(rozbStejnych[0] <= 10.0 && rozbDo1[0] <= 30.0 && rozbPosDo1[0] >= 80.0,
       "R2) polygony s texturou ve 2x: jen " + f1(rozbStejnych[0]) + " % bloku 2x2 ma 4 stejne body (do 1 odstinu " + f1(rozbDo1[0]) +
           " %) - body textury jsou ve 2x obrazu posunute o bod (mrizka 2x2 od (1,1): " + f1(rozbPosDo1[0]) +
           " % do 1 odstinu) a lisi se ditheringem; polovicni rozliseni tu (B319 i B320) nejde - proto v logu B319 jen 304 z 1137 casti");
  over(posPresne[0][1] <= 90.0 && posDo1[0][1] >= 95.0 && posPresne[0][0] >= 95.0,
       "R3) dithering je pevne na obrazovce: po posunu mapy o lichy pocet bodu hry je presne stejnych jen " + f1(posPresne[0][1]) +
           " % bodu (kopie B319 potrebuje cele ctverce 16x16 presne), do 1 odstinu " + f1(posDo1[0][1]) + " %; pri stejne fazi " + f1(posPresne[0][0]) + " % presne");
  over(posPresne[1][1] >= 90.0 && posPresne[1][0] >= 90.0,
       "R4) sprity: obraz po posunu = minuly posunuty presne (" + f1(posPresne[1][1]) + " / " + f1(posPresne[1][0]) + " % bodu)");
  // ---- kodek B320 na skutecnych obrazech ----
  over(lich[0][B319].obrazu > 0 && lich[0][B319].sKopii * 10 <= lich[0][B319].obrazu,
       "K1) chyba z logu B319 na skutecnem vykreslovaci: presne porovnani pri jine fazi ditheringu najde kopii jen v " +
           t(lich[0][B319].sKopii) + " z " + t(lich[0][B319].obrazu) + " obrazu");
  over(posun[0][B320].sKopii * 10 >= (posun[0][B320].obrazu - 7) * 9 && nejhorsiFaze320[0] >= 85,
       "K2) B320 na polygonech s ditheringem najde posun v " + t(posun[0][B320].sKopii) + " obrazech posunu (z " + t(posun[0][B320].obrazu - 7) +
           " hodnocenych), v kazde fazi aspon " + t(nejhorsiFaze320[0]) + " %");
  over(posun[0][B320].bajtu * 10 <= posun[0][DRIVE].bajtu * 4,
       "K3) dat pri posunu mapy (dithering): B320 " + t(100 * posun[0][B320].bajtu / std::max(1LL, posun[0][DRIVE].bajtu)) + " % drivejska, B319 " +
           t(100 * posun[0][B319].bajtu / std::max(1LL, posun[0][DRIVE].bajtu)) + " %");
  over(posun[0][B320].nejCtverec <= posun[0][DRIVE].nejCtverec * 1.25 + 1.0 && posun[0][B320].psnrMin >= posun[0][DRIVE].psnrMin - 1.0,
       "K4) obraz v prohlizeci jako drive: nejhorsi ctverec " + f1(posun[0][B320].nejCtverec) + " (drive " + f1(posun[0][DRIVE].nejCtverec) +
           "), PSNR nejhur " + f2(posun[0][B320].psnrMin) + " dB (drive " + f2(posun[0][DRIVE].psnrMin) + ")");
  over(celkem[0][1].polovicnich0 * 10 <= celkem[0][1].casti0 && celkem[1][1].polovicnich0 * 10 >= celkem[1][1].casti0 * 9,
       "K5) polovicni rozliseni: sprity " + t(celkem[1][1].polovicnich0) + " z " + t(celkem[1][1].casti0) + " casti; polygony jen " +
           t(celkem[0][1].polovicnich0) + " z " + t(celkem[0][1].casti0) + " (tam, kde to opravdu sedi - obraz spravne, K4)");
  over(klid[0][B320] >= klid[0][DRIVE] - 1.0 && klid[1][B320] >= klid[1][DRIVE] - 1.0,
       "K6) po zastaveni se obraz zjemni jako drive: dithering " + f2(klid[0][B320]) + " dB (drive " + f2(klid[0][DRIVE]) + "), sprity " +
           f2(klid[1][B320]) + " dB (drive " + f2(klid[1][DRIVE]) + ")");
  over(posun[1][B320].sKopii >= posun[1][B319].sKopii && posun[1][B319].sKopii * 10 >= (posun[1][B319].obrazu - 7) * 9 &&
           posun[1][B320].bajtu <= posun[1][B319].bajtu * 1.05,
       "K7) sprity bez ditheringu: B319 i B320 posun najdou (" + t(posun[1][B319].sKopii) + " / " + t(posun[1][B320].sKopii) + "), B320 dat " +
           t(100 * posun[1][B320].bajtu / std::max(1LL, posun[1][B319].bajtu)) + " % B319");
  over(celkem[0][2].casti0 == 0 && celkem[1][2].casti0 == 0, "K8) vsechny pakety vsech tri kodeku jdou rozebrat jako ve strance B319");

  Java_eu_atarihelp_emu10_NativePs1CoreBridge_ps1EglStop(&g_env, nullptr);
  std::printf("\nVYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  std::fflush(stdout);
  std::_Exit(g_chyb ? 1 : 0);
}
