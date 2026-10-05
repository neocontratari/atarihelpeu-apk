// B291: KONEC-KONCU test nativni casti zarizeni NA POCITACI.
// Prelozi se SKUTECNY nap_atari_native.cpp (stejny soubor jako v appce), jen
// misto Androidu jsou pod nim male nahrady (stub/): okno displeje je buffer
// v pameti, OpenSL "neni" (emulace bezi podle hodin), JNIEnv je rucne
// sestaveny. Bezi SKUTECNA vlakna (emulace + kresleni) v realnem case a test
// do pristroje "saha prstem" (devTouchNative) presne jako Java.
//   g++ -std=c++17 -O2 -pthread -Istub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b291_jni_host test_b291_jni_host.cpp
//            (vse na jednom radku)
//   ./test_b291_jni_host            (rychly test, ~20 s)
//   ./test_b291_jni_host - csave    (dlouhy test CSAVE pres vlakna, ~35 s)
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

// ---------- nahrady OpenSL: zvuk "neni k dispozici" ----------
static const SLInterfaceID_ iidE{1}, iidP{2}, iidQ{3};
const SLInterfaceID SL_IID_ENGINE = &iidE;
const SLInterfaceID SL_IID_PLAY = &iidP;
const SLInterfaceID SL_IID_ANDROIDSIMPLEBUFFERQUEUE = &iidQ;
SLresult slCreateEngine(SLObjectItf *, SLuint32, const void *, SLuint32, const SLInterfaceID *, const SLboolean *) { return 1; }

// ---------- nahrada okna displeje ----------
struct ANativeWindow { int w = 0, h = 0, fmt = 0; std::vector<uint32_t> buf, screen; int refs = 1; int posts = 0; long long px = 0; };
static ANativeWindow g_fakeWin;
void ANativeWindow_acquire(ANativeWindow *w) { w->refs++; }
void ANativeWindow_release(ANativeWindow *w) { w->refs--; }
int32_t ANativeWindow_getWidth(ANativeWindow *w) { return w->w; }
int32_t ANativeWindow_getHeight(ANativeWindow *w) { return w->h; }
int32_t ANativeWindow_getFormat(ANativeWindow *w) { return w->fmt; }
int32_t ANativeWindow_setBuffersGeometry(ANativeWindow *w, int32_t width, int32_t height, int32_t format) {
  w->w = width; w->h = height; w->fmt = format; w->buf.assign((size_t)width * height, 0); w->screen = w->buf; return 0;
}
int32_t ANativeWindow_lock(ANativeWindow *w, ANativeWindow_Buffer *b, ARect *r) {
  b->width = w->w; b->height = w->h; b->stride = w->w + 8; b->format = w->fmt;
  w->buf.resize((size_t)b->stride * w->h);
  b->bits = w->buf.data();
  if (r) w->px += (long long)(r->right - r->left) * (r->bottom - r->top);
  return 0;
}
int32_t ANativeWindow_unlockAndPost(ANativeWindow *w) {
  for (int y = 0; y < w->h; y++) for (int x = 0; x < w->w; x++) w->screen[(size_t)y * w->w + x] = w->buf[(size_t)y * (w->w + 8) + x];
  w->posts++; return 0;
}
ANativeWindow *ANativeWindow_fromSurface(JNIEnv *, jobject s) { if (!s) return nullptr; g_fakeWin.refs++; return &g_fakeWin; }

// ---------- rucne sestaveny JNIEnv (jen funkce, ktere native.cpp pouziva) ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<uint8_t> b; };
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->b.size(); }
static void JNICALL fGetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, jbyte *buf) { std::memcpy(buf, ((FakeArr *)a)->b.data() + st, n); }
static jbyteArray JNICALL fNewByteArray(JNIEnv *, jsize n) { FakeArr *a = new FakeArr; a->b.resize(n); return (jbyteArray)a; }
static void JNICALL fSetByteArrayRegion(JNIEnv *, jbyteArray a, jsize st, jsize n, const jbyte *buf) { std::memcpy(((FakeArr *)a)->b.data() + st, buf, n); }
static JNINativeInterface_ g_tbl;
static JNIEnv g_env;

static int chyb = 0, kontrol = 0;
static void over(bool ok, const char *co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co); fflush(stdout); }
static void spi(int ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }
static std::string obrazovka() {
  std::lock_guard<std::mutex> l(g_mStroj);
  Machine &m = *g_stroj;
  int sav = m.mem.ram[0x58] | (m.mem.ram[0x59] << 8);
  std::string out;
  for (int r = 0; r < 24; r++) { std::string line;
    for (int c = 0; c < 40; c++) { int v = m.mem.ram[(sav + r * 40 + c) & 0xFFFF] & 0x7F; int a; if (v < 64) a = v + 32; else if (v < 96) a = v - 64; else a = v; if (a < 32 || a > 126) a = '.'; line.push_back((char)a); }
    while (!line.empty() && line.back() == ' ') line.pop_back();
    out += line + "\n";
  }
  return out;
}
static void vypisLog() {
  jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
  if (s) { printf("%s", ((FakeStr *)s)->s.c_str()); }
}
// klepnuti prstem na prvek pristroje (souradnice v du navrhu)
static int klep(float ux, float uy, int pid = 0, int drzetMs = 70) {
  float x, y;
  { std::lock_guard<std::mutex> dl(g_mDev); x = devGet().X(ux); y = devGet().Y(uy); }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 0, x, y);
  spi(drzetMs);
  int r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 1, x, y);
  spi(90);
  return r;
}
static void klavesa(const char *l2) {
  for (int i = 0; i < 57; i++) if (std::string(nap::dev::KEYS[i].l2) == l2) {
    const auto &k = nap::dev::KEYS[i]; klep(k.x + k.w / 2, k.y + k.h / 2); return; }
  printf("klavesa %s nenalezena\n", l2);
}
static void ulozObraz(const char *jm) {
  FILE *f = fopen(jm, "wb"); fprintf(f, "P6 %d %d 255\n", g_fakeWin.w, g_fakeWin.h);
  for (size_t i = 0; i < g_fakeWin.screen.size(); i++) { uint32_t c = g_fakeWin.screen[i]; fputc(c & 255, f); fputc((c >> 8) & 255, f); fputc((c >> 16) & 255, f); }
  fclose(f);
}

int main(int argc, char **argv) {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  const char *tbxlPath = (argc > 1 && std::string(argv[1]) != "-") ? argv[1]
                          : "../../app/src/main/assets/emu_atari_cpp/turbo_basic_xl.xex";
  FakeStr surface{"plocha"};

  // Volitelny dlouhy test CSAVE pres vlakna: ./test_b291_jni_host tbxl.xex csave
  if (argc > 2 && std::string(argv[2]) == "csave") {
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
    for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
    for (const char *k : {"C", "S", "A", "V", "E", "Return"}) klavesa(k);
    spi(3000);                                         // dve pipnuti, OS ceka na klavesu
    klavesa("Return");                                 // "stisk klavesy" -> motor + data
    jbyteArray wav = nullptr;
    bool motorVidel = false;
    for (int i = 0; i < 450 && !wav; i++) {            // max 45 s
      spi(100);
      if (g_motorOn.load()) motorVidel = true;
      wav = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTakeWavNative(&g_env, nullptr);
    }
    vypisLog();
    over(motorVidel, "CSAVE zapnul motor kazety (videt i na civkach pristroje)");
    over(wav != nullptr, "CSAVE: hotovy WAV prisel do Javy (devTakeWav)");
    if (wav) {
      double sek = ((FakeArr *)wav)->b.size() / 2 / 44100.0;
      printf("WAV %.1f s (%zu bajtu)\n", sek, ((FakeArr *)wav)->b.size());
      over(sek > 10 && sek < 60, "delka WAV odpovida CSAVE");
    }
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
    printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
    return chyb ? 1 : 0;
  }

  // 1) HELP otevren: start + plocha 1080x2160
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  spi(1500);
  vypisLog();
  over(g_fakeWin.posts > 0, "kresleni bezi - na displej doslo aspon jedno predani");
  over(g_fakeWin.fmt == WINDOW_FORMAT_RGBA_8888 && g_fakeWin.w == 1080 && g_fakeWin.h == 2160, "okno nastaveno na 1080x2160 RGBA");
  over(g_snimkuCelkem.load() >= 40, "emulace bezi (aspon 40 snimku za 1,5 s)");
  ulozObraz("jni_start.ppm");

  // 2) cekat na READY (bez SIO zkratky trva start ~10 s, jako dosud)
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY (obraz z jadra)");
  long long px0 = g_fakeWin.px; int posts0 = g_fakeWin.posts;
  spi(1000);
  printf("klidovy stav: %d predani/s, %.1f %% plochy za snimek\n", g_fakeWin.posts - posts0,
         g_fakeWin.posts > posts0 ? 100.0 * (g_fakeWin.px - px0) / (g_fakeWin.posts - posts0) / (1080.0 * 2160) : 0.0);

  // 3) psani PRSTEM na klavesnici pristroje: PRINT 2+2 RETURN
  for (const char *k : {"P", "R", "I", "N", "T"}) klavesa(k);
  klep(216 + 242, 1175 + 26);                         // mezernik
  klavesa("2");
  // '+' je na klavese "+" (bez shiftu)
  klavesa("+");
  klavesa("2");
  klavesa("Return");
  spi(600);
  std::string scr = obrazovka();
  printf("--- obrazovka ---\n%s---\n", scr.c_str());
  over(scr.find("PRINT 2+2") != std::string::npos, "dotyky na klavesy napsaly PRINT 2+2");
  over(scr.find("\n  4\n") != std::string::npos || scr.find(" 4\n") != std::string::npos, "Atari spocitalo 4");

  // 4) SHIFT zamek: SHIFT, 1 -> '!'; SHIFT znovu -> odemceno
  klep(41 + 60, 1101 + 34);                          // levy SHIFT
  { std::lock_guard<std::mutex> dl(g_mDev); over(devGet().shiftLatched, "SHIFT po klepnuti zustava zamceny"); }
  klavesa("1");
  klep(41 + 60, 1101 + 34);
  { std::lock_guard<std::mutex> dl(g_mDev); over(!devGet().shiftLatched, "druhe klepnuti SHIFT odemkne"); }
  spi(300);
  scr = obrazovka();
  over(scr.find("!") != std::string::npos, "SHIFT+1 napsalo '!'");

  // 4b) START: i hodne kratke tuknuti (5 ms) musi program videt (PEEK(53279)=6)
  {
    FakeStr prg{"NEW\n10 IF PEEK(53279)<>6 THEN 10\n20 PRINT \"START OK\"\nRUN\n"};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&prg, JNI_FALSE);
    spi(2500);
    klep(436.4f + 70, 740 + 36, 3, 5);              // START (viditelna cast mezi HELP a SELECT)
    spi(500);
    std::string sc2 = obrazovka();
    over(sc2.find("START OK") != std::string::npos, "kratke tuknuti na START program zachytil (min. 5 snimku drzeni)");
  }

  // 5) kazeta: PLAY drzi, pocitadlo bezi; STOP pusti; EJECT otevre dvirka, klepnuti na okenko zavre
  klep(383 + 40, 1440 + 35);
  spi(2300);
  int cit; bool hraje;
  { std::lock_guard<std::mutex> dl(g_mDev); cit = devGet().counter; hraje = devGet().play; }
  over(hraje && cit >= 2, "PLAY zamcene a pocitadlo pocita (+1 za vterinu)");
  klep(547 + 40, 1440 + 35);                          // STOP
  { std::lock_guard<std::mutex> dl(g_mDev); over(!devGet().play && !devGet().rec, "STOP pustil PLAY/REC"); }
  klep(855 + 18, 1328 + 24);                          // nulovani
  { std::lock_guard<std::mutex> dl(g_mDev); over(devGet().counter == 0, "nulovaci spinac vynuloval pocitadlo"); }
  klep(627 + 41, 1440 + 35);                          // EJECT
  { std::lock_guard<std::mutex> dl(g_mDev); over(devGet().doorOpen, "EJECT otevrel dvirka kazetaku"); }
  spi(400);
  ulozObraz("jni_dvirka.ppm");
  klep(466, 1340);                                    // klepnuti na okenko
  { std::lock_guard<std::mutex> dl(g_mDev); over(!devGet().doorOpen, "klepnuti na okenko dvirka zavrelo"); }

  // 6) servisni tlacitko MENU vraci akci 8 (az pri pusteni)
  int akce = klep(42 + 7 * 111 + 50, 1537 + 39);
  over(akce == nap::dev::SVC_MENU, "MENU vraci Jave akci MENU");
  akce = klep(42 + 4 * 111 + 50, 1537 + 39);
  over(akce == nap::dev::SVC_TXT, "BASIC/TBXL TXT vraci akci TXT");

  // 7) POWER: vypnout -> stroj stoji; zapnout -> studeny start
  long long s0 = g_snimkuCelkem.load();
  klep(8 + 17, 393 + 25);
  spi(400);
  long long s1 = g_snimkuCelkem.load();
  spi(400);
  over(!g_strojBezi.load() && g_snimkuCelkem.load() == s1 && s1 >= s0, "POWER vypnul stroj (zadne dalsi snimky)");
  ulozObraz("jni_vypnuto.ppm");
  klep(30, 380);                                      // klepnuti jinam pri vypnuti: nic
  klep(8 + 17, 393 + 25);
  spi(500);
  over(g_strojBezi.load() && g_snimkuCelkem.load() > s1, "POWER zapnul stroj znovu (snimky bezi)");

  // 8) XEX: Turbo-BASIC XL pres stejnou cestu jako tlacitko TURBO/BASIC
  FILE *f = fopen(tbxlPath, "rb");
  FakeArr arr;
  int c;
  while (f && (c = fgetc(f)) != EOF) arr.b.push_back((uint8_t)c);
  if (f) fclose(f);
  FakeStr nm{"turbo_basic_xl.xex"};
  jboolean ok = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(&g_env, nullptr, (jbyteArray)&arr, (jstring)&nm);
  over(ok == JNI_TRUE, "XEX prijat k zavedeni");
  for (int i = 0; i < 150 && obrazovka().find("READY") == std::string::npos; i++) spi(100);   // TBXL ukaze uvodni text, pak READY
  FakeStr txt{"? $FF*2\n"};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&txt, JNI_FALSE);
  spi(2000);
  vypisLog();
  scr = obrazovka();
  printf("--- obrazovka (TBXL) ---\n%s---\n", scr.c_str());
  over(scr.find("510") != std::string::npos, "Turbo-BASIC XL bezi ($FF*2 = 510 umi jen TBXL)");
  ulozObraz("jni_tbxl.ppm");

  // 9) plocha zanikne a vznikne (LOG/CHYBA a zpet) - bez padu, kresli dal
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, nullptr, 0, 0);
  int p0 = g_fakeWin.posts; spi(300);
  over(g_fakeWin.posts == p0, "bez plochy se nic nekresli");
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  spi(300);
  over(g_fakeWin.posts > p0, "po navratu plochy se kresli znovu");

  // 10) stop - obe vlakna skonci
  auto t0 = std::chrono::steady_clock::now();
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
  over(!g_run.load() && ms < 500, "stop: obe vlakna skoncila (bez zaseknuti)");
  vypisLog();
  jstring info = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devInfoNative(&g_env, nullptr);
  printf("%s\n", ((FakeStr *)info)->s.c_str());
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
