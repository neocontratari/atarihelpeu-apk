// B292: KONEC-KONCU test SKUTECNEHO nap_atari_native.cpp s NOVYM jadrem na PC
// (stejny princip jako test_overeni/b291/test_b291_jni_host.cpp: Android je
// nahrazen malymi nahradami v ../b291/stub, bezi skutecna vlakna emulace a
// kresleni a test "saha prstem" na pristroj jako Java).
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include
//       -I$JAVA_HOME/include/linux -I../../app/src/main/cpp/atari
//       -o test_b292_jni_host test_b292_jni_host.cpp        (vse na jednom radku)
//   ./test_b292_jni_host
// Overuje:
//  1) samokontrolu jadra (runSelfTest) - cisla pro NativeAtariCoreBridge.java
//  2) drzeni klavesy prstem: OS ji po ~1 s zacne opakovat, po pusteni prestane
//  3) CSAVE -> WAV (linka SIO DATA OUT) pres vlakna -> devTakeWav
//  4) EJECT vraci Jave akci "vyber kazetu"
//  5) ten WAV vlozeny jako kazeta (devLoadTape, PLAY se zmackne samo) ->
//     CLOAD, RETURN, po pipnuti RETURN -> LIST
//  6) B295: disketa ATR do D1: -> start z diskety, po POWER vyp/zap znovu
//  7) B296: joystick dotykem na obrazovce (leva pulka smer, prava FIRE) a
//     z herniho ovladace (smer, FIRE, START, X = MEZERA)
//  8) B296: VBXE - Popeye (XEX) az do hry, Wolfenstein 3D (ATR) az do hry
//     a chuze joystickem (test_assets/)
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

// ---------- rucne sestaveny JNIEnv ----------
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
static void prst(float ux, float uy, int pid, int akce) {
  float x, y;
  { std::lock_guard<std::mutex> dl(g_mDev); x = devGet().X(ux); y = devGet().Y(uy); }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, akce, x, y);
}
static int klep(float ux, float uy, int pid = 0, int drzetMs = 70) {
  float x, y;
  { std::lock_guard<std::mutex> dl(g_mDev); x = devGet().X(ux); y = devGet().Y(uy); }
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 0, x, y);
  spi(drzetMs);
  int r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, 1, x, y);
  spi(90);
  return r;
}
static const nap::dev::KeyDef *klavesaDef(const char *l2) {
  for (int i = 0; i < 57; i++) if (std::string(nap::dev::KEYS[i].l2) == l2) return &nap::dev::KEYS[i];
  printf("klavesa %s nenalezena\n", l2); return nullptr;
}
static void klavesa(const char *l2) { const auto *k = klavesaDef(l2); if (k) klep(k->x + k->w / 2, k->y + k->h / 2); }
static void napis(const char *t) {
  FakeStr s{t};
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTypeTextNative(&g_env, nullptr, (jstring)&s, JNI_FALSE);
}
static int pocet(const std::string &s, char c) { int n = 0; for (char x : s) if (x == c) n++; return n; }

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};

  // 1) samokontrola
  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    std::string js = ((FakeStr *)r)->s;
    printf("SELFTEST %s\n", js.c_str());
    over(js.find("\"cpuInstr\":51200") != std::string::npos && js.find("\"memReads\":16252928") != std::string::npos,
         "samokontrola probehla (51200 instrukci, 16252928 cteni)");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");

  const bool jenSirka = getenv("JEN_SIRKA") != nullptr;   // B297: rychly beh jen testu na sirku
  if (!jenSirka) {
  // 2) drzeni klavesy: "A" drzene 2,5 s -> OS opakuje (KRPDEL ~1 s, pak KEYREP)
  {
    const auto *k = klavesaDef("A");
    prst(k->x + k->w / 2, k->y + k->h / 2, 1, 0);
    spi(2500);
    prst(k->x + k->w / 2, k->y + k->h / 2, 1, 1);
    spi(400);
    std::string s1 = obrazovka();
    spi(800);
    std::string s2 = obrazovka();
    int a = pocet(s1, 'A') - 1;                        // "READY" ma jedno A
    printf("drzeni A 2,5 s -> %d znaku A\n", a);
    over(a >= 5, "drzena klavesa se opakuje jako na skutecnem Atari");
    over(pocet(s2, 'A') == pocet(s1, 'A'), "po pusteni klavesy opakovani prestalo");
    klavesa("Return");
  }

  // 3) CSAVE pres vlakna
  napis("NEW\n10 PRINT \"KAZETA B292\"\n20 GOTO 10\nCSAVE\n");
  spi(6000);                                           // napsani + dve pipnuti
  klavesa("Return");                                   // "stisk klavesy" -> motor + data
  jbyteArray wav = nullptr;
  for (int i = 0; i < 450 && !wav; i++) { spi(100); wav = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTakeWavNative(&g_env, nullptr); }
  vypisLog();
  over(wav != nullptr, "CSAVE: hotovy WAV prisel do Javy (devTakeWav)");
  if (!wav) { printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb + 1); return 1; }
  // WAV soubor z PCM (presne jako Java ulozitAtariPcmWav)
  FakeArr soubor;
  {
    const std::vector<uint8_t> &pcm = ((FakeArr *)wav)->b;
    const uint32_t nb = (uint32_t)pcm.size();
    uint8_t h[44] = {'R','I','F','F',0,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x44,0xAC,0,0,0x88,0x58,1,0,2,0,16,0,'d','a','t','a',0,0,0,0};
    uint32_t r = 36 + nb; std::memcpy(h + 4, &r, 4); std::memcpy(h + 40, &nb, 4);
    soubor.b.assign(h, h + 44); soubor.b.insert(soubor.b.end(), pcm.begin(), pcm.end());
    printf("WAV %.1f s\n", pcm.size() / 2 / 44100.0);
  }

  // 4) EJECT -> akce pro Javu
  int akce = klep(627 + 41, 1440 + 35);
  over(akce == nap::dev::SVC_EJECT, "EJECT vraci Jave akci VYBER KAZETY");
  { std::lock_guard<std::mutex> dl(g_mDev); over(devGet().doorOpen, "EJECT otevrel dvirka"); }

  // 5) kazeta -> CLOAD (po studenem startu: POWER vyp/zap - kazeta zustava v magnetofonu)
  FakeStr nm{"kazeta_b292.wav"};
  jstring vr = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadTapeNative(&g_env, nullptr, (jbyteArray)&soubor, (jstring)&nm);
  printf("devLoadTape: %s\n", ((FakeStr *)vr)->s.c_str());
  over(((FakeStr *)vr)->s.rfind("OK ", 0) == 0, "kazeta vlozena (WAV demodulovan)");
  { std::lock_guard<std::mutex> dl(g_mDev); over(!devGet().doorOpen, "po vlozeni kazety se dvirka zavrela"); }
  { std::lock_guard<std::mutex> dl(g_mDev); over(devGet().play, "B295: po vlozeni kazety je PLAY zmacknute samo"); }
  klep(8 + 17, 393 + 25); spi(400); klep(8 + 17, 393 + 25);   // POWER vyp / zap
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  napis("CLOAD\n");
  spi(3000);                                           // pipnuti, OS ceka na klavesu
  klavesa("Return");                                   // B295: PLAY uz je zmacknute - jen RETURN
  bool motor = false;
  for (int i = 0; i < 400; i++) {
    spi(100);
    if (g_motorOn.load()) motor = true;
    if (motor && !g_motorOn.load()) break;
  }
  spi(500);
  napis("LIST\n");
  spi(2500);
  vypisLog();
  std::string scr = obrazovka();
  printf("--- obrazovka po CLOAD + LIST ---\n%s---\n", scr.c_str());
  over(motor, "CLOAD zapnul motor (pasek bezel)");
  over(scr.find("10 PRINT \"KAZETA B292\"") != std::string::npos && scr.find("20 GOTO 10") != std::string::npos,
       "CLOAD nahral program z kazety (LIST ho ukazuje)");

  // 6) B295: disketa ATR (Acid800 - bootovaci disketa) -> D1: a start z ni
  {
    const char *atrPath = getenv("ATR") ? getenv("ATR") : "/home/claude/ref/atari800/test/acid800.atr";
    FILE *f = fopen(atrPath, "rb");
    if (!f) printf("(ATR %s neni - test diskety preskocen)\n", atrPath);
    else {
      FakeArr atr; int c; while ((c = fgetc(f)) != EOF) atr.b.push_back((uint8_t)c); fclose(f);
      FakeStr an{"acid800.atr"};
      jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&atr, (jstring)&an);
      printf("devLoadAtr: %s\n", ((FakeStr *)r)->s.c_str());
      over(((FakeStr *)r)->s.rfind("OK ", 0) == 0, "ATR prijat do mechaniky D1:");
      bool nabootovano = false;
      for (int i = 0; i < 150 && !nabootovano; i++) { spi(100); nabootovano = obrazovka().find("Acid800") != std::string::npos; }
      std::string sc = obrazovka();
      printf("--- obrazovka po startu z diskety ---\n%s---\n", sc.c_str());
      over(nabootovano, "Atari nabootovalo z diskety v D1: (Acid800)");
      // POWER vyp/zap: disketa zustava v mechanice a bootuje znovu
      klep(8 + 17, 393 + 25); spi(400); klep(8 + 17, 393 + 25);
      nabootovano = false;
      for (int i = 0; i < 150 && !nabootovano; i++) { spi(100); nabootovano = obrazovka().find("Acid800") != std::string::npos; }
      over(nabootovano, "po POWER vyp/zap disketa v mechanice zustala a bootuje znovu");
    }
  }

  // 7) B296: joystick - dotyk na obrazovce (leva pulka smer, prava FIRE) a herni ovladac
  {
    auto stav = [](int &pa, int &tr, int &co) { std::lock_guard<std::mutex> l(g_mStroj); pa = g_stroj->porta & 15; tr = g_stroj->trig[0]; co = g_stroj->consol; };
    int pa, tr, co;
    // leva pulka: prst dolu, posun nahoru o 60 du -> joystick NAHORU
    prst(300, 400, 2, 0); spi(60);
    prst(300, 340, 2, 3); spi(120);
    stav(pa, tr, co);
    printf("joystick dotykem nahoru: PORTA dolni bity $%X, TRIG0 %d\n", pa, tr);
    over(pa == 0x0E, "B296: dotyk na leve pulce obrazovky + posun nahoru = joystick NAHORU (PORTA $E)");
    prst(360, 340, 2, 3); spi(120);                         // doprava-nahoru (stred jde za prstem)
    stav(pa, tr, co);
    over(pa == 0x06, "B296: posun prstu doprava = joystick NAHORU+VPRAVO (PORTA $6)");
    // prava pulka: FIRE (druhy prst)
    prst(700, 400, 3, 0); spi(120);
    stav(pa, tr, co);
    over(tr == 0, "B296: dotyk na prave pulce obrazovky = FIRE (TRIG0 = 0)");
    prst(700, 400, 3, 1); prst(360, 340, 2, 1); spi(120);
    stav(pa, tr, co);
    over(pa == 0x0F && tr == 1, "B296: po zvednuti prstu je joystick v klidu (PORTA $F, TRIG0 1)");
    // herni ovladac: vlevo + FIRE + START
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 4 | 16 | 32); spi(150);
    stav(pa, tr, co);
    printf("ovladac vlevo+FIRE+START: PORTA $%X TRIG0 %d CONSOL %d\n", pa, tr, co);
    over(pa == 0x0B && tr == 0 && (co & 1) == 0, "B296: herni ovladac = joystick VLEVO + FIRE + START");
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0); spi(250);
    stav(pa, tr, co);
    over(pa == 0x0F && tr == 1 && (co & 1) == 1, "B296: ovladac pusteny -> joystick v klidu, START pusteny");
    // tlacitko X = klavesa MEZERA (drzena), pusteni
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 256); spi(150);
    int kb, sk;
    { std::lock_guard<std::mutex> l(g_mStroj); kb = g_stroj->kbcode; sk = g_stroj->skstat; }
    over(kb == 33 && !(sk & 4), "B296: tlacitko X ovladace = klavesa MEZERA drzena (Wolfenstein: dvere)");
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0); spi(150);
    { std::lock_guard<std::mutex> l(g_mStroj); sk = g_stroj->skstat; }
    over((sk & 4) != 0, "B296: tlacitko X pusteno = klavesa pustena");
  }

  // 8) B296: VBXE - Popeye (XEX) a Wolfenstein 3D (ATR) pres skutecne vlakna
  {
    auto nactiSoubor = [](const char *p, FakeArr &a) { FILE *f = fopen(p, "rb"); if (!f) return false; int c; while ((c = fgetc(f)) != EOF) a.b.push_back((uint8_t)c); fclose(f); return true; };
    const char *popPath = "/home/claude/atarihelpeu-apk/test_assets/Popeye (VBXE, PAL Version)(2).xex";
    FakeArr pop;
    if (!nactiSoubor(popPath, pop)) printf("(Popeye neni - preskoceno)\n");
    else {
      FakeStr pn{"Popeye (VBXE, PAL Version)(2).xex"};
      jboolean ok = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadXexNative(&g_env, nullptr, (jbyteArray)&pop, (jstring)&pn);
      over(ok == JNI_TRUE, "B296: Popeye (VBXE) prijat jako XEX");
      spi(9000);                                              // start OS + nahrani, "Press any key"
      klavesa("Return"); spi(26000);                          // "Loading..." (rozbaleni grafiky do VRAM) -> titulka
      long long xdl, zap; int radku = 0;
      { std::lock_guard<std::mutex> l(g_mStroj); xdl = g_stroj->vbx.nXdlFrames; zap = g_stroj->vbx.nWrites; for (int y = 0; y < AnticView::H; y++) radku += g_view->vbxeRadek[y]; }
      printf("Popeye: zapisu do VBXE %lld, snimku s XDL %lld, radku z VBXE %d\n", zap, xdl, radku);
      over(xdl > 50 && radku > 200, "B296: Popeye zapnul VBXE (XDL + overlay) a obraz kresli VBXE");
      // START = hra (pres herni ovladac), pak FIRE = zacit
      Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 32); spi(300);
      Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0); spi(3000);
      Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 16); spi(300);
      Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPadNative(&g_env, nullptr, 0); spi(6000);
      long long blitu;
      { std::lock_guard<std::mutex> l(g_mStroj); blitu = g_stroj->vbx.nBlits; }
      printf("Popeye po START/FIRE: blitu %lld\n", blitu);
      over(blitu > 2000, "B296: Popeye hraje (blitter VBXE kresli postavy - tisice blitu)");
      vypisLog();
    }
    const char *w3dPath = "/home/claude/atarihelpeu-apk/test_assets/wolf3d(1).atr";
    FakeArr w3d;
    if (!nactiSoubor(w3dPath, w3d)) printf("(wolf3d.atr neni - preskoceno)\n");
    else {
      FakeStr wn{"wolf3d(1).atr"};
      jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&w3d, (jstring)&wn);
      printf("devLoadAtr: %s\n", ((FakeStr *)r)->s.c_str());
      spi(9000);
      long long xdl; int ov;
      { std::lock_guard<std::mutex> l(g_mStroj); xdl = g_stroj->vbx.nXdlFrames; ov = g_stroj->vbx.lastOvMode; }
      printf("W3D titulka: snimku s XDL %lld, overlay %d\n", xdl, ov);
      over(xdl > 50 && ov == 2, "B296: Wolfenstein 3D nabootoval a titulku kresli VBXE (overlay SR 320 bodu)");
      // klavesa -> menu, RETURN x3 -> hra (overlay LR)
      klavesa("Return"); spi(4000);
      klavesa("Return"); spi(3000);
      klavesa("Return"); spi(3000);
      klavesa("Return"); spi(9000);
      { std::lock_guard<std::mutex> l(g_mStroj); ov = g_stroj->vbx.lastOvMode; }
      printf("W3D po menu: overlay %d\n", ov);
      over(ov == 1, "B296: Wolfenstein 3D ve hre (3D pohled = overlay LR 160 bodu)");
      // joystick nahoru 1,5 s (dotykem) -> hrac jde dopredu (meni se obraz)
      std::vector<uint32_t> pred, po;
      { std::lock_guard<std::mutex> f(g_mFrame); pred = g_sdilenySnimek; }
      prst(300, 400, 4, 0); spi(50); prst(300, 330, 4, 3); spi(1500); prst(300, 330, 4, 1); spi(300);
      { std::lock_guard<std::mutex> f(g_mFrame); po = g_sdilenySnimek; }
      size_t ruzne = 0; for (size_t i = 0; i < pred.size() && i < po.size(); i++) ruzne += pred[i] != po[i];
      printf("W3D: joystick nahoru -> zmenenych bodu %zu z %zu\n", ruzne, po.size());
      over(ruzne > 10000, "B296: joystick dotykem ve Wolfensteinu - hrac jde dopredu (obraz se zmenil)");
      vypisLog();
    }
  }

  }   // !jenSirka

  // 9) B297: na sirku - D-pad a tlacitka jako u Segy, uprava rozlozeni, nastaveni
  {
    using D = nap::dev::Device;
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 2400, 1080);
    spi(1500);
    bool land = false; float dcx = 0, dcy = 0, rd = 0, fx = 0, fy = 0, sx = 0, sy = 0, stx = 0, sty = 0, gx = 0, gy = 0;
    {
      std::lock_guard<std::mutex> dl(g_mDev); D &d = devGet();
      land = d.land; dcx = d.cpx(D::C_DPAD); dcy = d.cpy(D::C_DPAD); rd = d.rDpad;
      fx = d.cpx(D::C_FIRE); fy = d.cpy(D::C_FIRE); sx = d.cpx(D::C_SPACE); sy = d.cpy(D::C_SPACE);
      stx = d.cpx(D::C_START); sty = d.cpy(D::C_START); gx = d.gearCx; gy = d.gearCy;
    }
    printf("na sirku: land=%d dpad (%.0f,%.0f) r=%.0f FIRE (%.0f,%.0f)\n", land, dcx, dcy, rd, fx, fy);
    over(land, "B297: displej 2400x1080 = na sirku (obraz pres celou vysku, ovladani jako Sega)");
    auto dotek = [&](int pid, int akce, float x, float y) { return Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devTouchNative(&g_env, nullptr, pid, akce, x, y); };
    auto stav = [](int &pa, int &tr, int &co, int &kb, int &sk) { std::lock_guard<std::mutex> l(g_mStroj); pa = g_stroj->porta & 15; tr = g_stroj->trig[0]; co = g_stroj->consol; kb = g_stroj->kbcode; sk = g_stroj->skstat; };
    int pa, tr, co, kb, sk;
    // D-pad: prst na stred, posun nahoru (o polovinu polomeru)
    int r1 = dotek(5, 0, dcx, dcy);
    dotek(5, 3, dcx, dcy - rd * .5f); spi(150);
    stav(pa, tr, co, kb, sk);
    over(pa == 0x0E, "B297: D-pad posun nahoru = joystick NAHORU (PORTA $E)");
    dotek(5, 3, dcx + rd * .45f, dcy + rd * .45f); spi(150);
    stav(pa, tr, co, kb, sk);
    over(pa == 0x05, "B297: D-pad sikmo vpravo dolu = diagonala (PORTA $5)");
    // FIRE druhym prstem (s vibraci)
    int r2 = dotek(6, 0, fx, fy); spi(150);
    stav(pa, tr, co, kb, sk);
    over(tr == 0 && pa == 0x05, "B297: FIRE soucasne s D-padem (TRIG0 = 0, smer drzi)");
    over((r2 & 0x1000) != 0, "B297: stisk tlacitka vraci Jave vibraci");
    (void)r1;
    dotek(6, 1, fx, fy); dotek(5, 1, dcx, dcy); spi(150);
    stav(pa, tr, co, kb, sk);
    over(pa == 0x0F && tr == 1, "B297: po zvednuti prstu je joystick v klidu");
    // MEZERA = klavesa drzena
    dotek(7, 0, sx, sy); spi(150);
    stav(pa, tr, co, kb, sk);
    over(kb == 33 && !(sk & 4), "B297: tlacitko MEZERA = klavesa mezera drzena");
    dotek(7, 1, sx, sy); spi(150);
    stav(pa, tr, co, kb, sk);
    over((sk & 4) != 0, "B297: MEZERA pustena");
    // START
    dotek(8, 0, stx, sty); spi(150);
    stav(pa, tr, co, kb, sk);
    over((co & 1) == 0, "B297: tlacitko START drzi konzolovy START");
    dotek(8, 1, stx, sty); spi(250);
    // ozubene kolecko -> menu (akce 10)
    dotek(9, 0, gx, gy); int ak = dotek(9, 1, gx, gy);
    over((ak & 0xFFF) == nap::dev::SVC_CTRL, "B297: ozubene kolecko vraci Jave menu D-PAD A OVLADANI");
    // uprava rozlozeni: FIRE posunout o 150 px doleva, HOTOVO
    FakeStr ce{"edit"};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&ce);
    dotek(10, 0, fx, fy); dotek(10, 3, fx - 75, fy); dotek(10, 3, fx - 150, fy); dotek(10, 1, fx - 150, fy);
    float fx2;
    nap::dev::RRect hot[3];
    { std::lock_guard<std::mutex> dl(g_mDev); D &d = devGet(); fx2 = d.cpx(D::C_FIRE); d.editBarButtons(hot); }
    over(std::fabs(fx2 - (fx - 150)) < 2.f, "B297: v uprave rozlozeni jde FIRE pretahnout prstem");
    stav(pa, tr, co, kb, sk);
    over(tr == 1, "B297: pri uprave rozlozeni se nehraje (tahani FIRE nestrili)");
    const float hx = (hot[2].x0 + hot[2].x1) * .5f, hy = (hot[2].y0 + hot[2].y1) * .5f;
    dotek(11, 0, hx, hy); ak = dotek(11, 1, hx, hy);
    over((ak & 0xFFF) == nap::dev::SVC_CTRL_SAVE, "B297: HOTOVO vraci Jave ulozeni rozlozeni");
    FakeStr cg{"get"};
    std::string cfg = ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&cg))->s;
    printf("nastaveni: %s\n", cfg.c_str());
    over(cfg.find("pos=0.") != std::string::npos, "B297: rozlozeni je v nastaveni pro Javu (pos=...)");
    // levak (mirror): D-pad na pravou stranu
    FakeStr cm{"mirror"};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&cm);
    float dcx2; { std::lock_guard<std::mutex> dl(g_mDev); dcx2 = devGet().cpx(D::C_DPAD); }
    over(dcx2 > 1200, "B297: PROHODIT (levak) - D-pad je vpravo");
    // nastaveni z Javy (velikost, citlivost) a navrat na vychozi
    FakeStr cs{"set:size=130;sens=15"};
    std::string c2 = ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&cs))->s;
    over(c2.find("size=130") != std::string::npos && c2.find("sens=15") != std::string::npos, "B297: velikost a citlivost z Javy se pouziji");
    FakeStr cr{"reset"}, cr2{"set:sens=7;op=100;size=100;dsize=100;hap=1"};
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&cr);
    std::string c3 = ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&cr2))->s;
    over(c3 == "v=1;sens=7;op=100;size=100;dsize=100;hap=1;pos=", "B297: VYCHOZI rozlozeni i nastaveni");
    // zpet na vysku: pristroj jako driv, joystick na obrazovce
    Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
    spi(1500);
    { std::lock_guard<std::mutex> dl(g_mDev); land = devGet().land; }
    over(!land, "B297: otoceni zpet na vysku = pristroj Atari jako driv");
    prst(300, 400, 12, 0); spi(60); prst(300, 340, 12, 3); spi(150);
    stav(pa, tr, co, kb, sk);
    over(pa == 0x0E, "B297: na vysku zase funguje joystick na obrazovce");
    prst(300, 340, 12, 1); spi(100);
    vypisLog();
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  vypisLog();
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
