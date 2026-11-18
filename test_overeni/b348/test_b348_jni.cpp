// test_b348_jni.cpp - B348: SKUTECNY nap_atari_native.cpp (vlakna emulace a kresleni jako
// v telefonu, Android nahrazen stuby z ../b291/stub - stejne jako ../b292/test_b292_jni_host.cpp):
//  1) samokontrola jadra: cpuHash = NativeAtariCoreBridge.OCEKAVANY_CPU_HASH (B348 "4A2C6CBA")
//  2) disketa do D1: (devLoadAtrNative) -> OPTION drzene jen ~1 s (BASIC vypnuty, ale zavadec
//     hry uz OPTION nevidi - xBIOS v Prince of Persia jinak ukaze nabidku misto hry)
//  3) vymena diskety bez restartu (devCtlNative "atr_vymena" + devLoadAtrNative - jako
//     "VYMENIT DISKETU BEZ RESTARTU" v appce) a vlastni zavadec s SIO pres POKEY
//     (test_b348_sio.asm) cte, zapisuje a dostava NAK od jednotky D1:
// Preklad (z test_overeni/b348, JAVA_HOME = JDK kvuli jni.h, vse na jednom radku):
//   g++ -std=c++17 -O2 -pthread -I../b291/stub -I$JAVA_HOME/include -I$JAVA_HOME/include/linux
//       -I../../app/src/main/cpp/atari -o test_b348_jni test_b348_jni.cpp
//   ./test_b348_jni                                              -> VYSTUP_test_b348_jni.txt
#include "../../app/src/main/cpp/atari/nap_atari_native.cpp"
#include "test_b348_program.h"
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
static void over(bool ok, const std::string &co) { kontrol++; if (!ok) chyb++; printf("%s  %s\n", ok ? "OK   " : "CHYBA", co.c_str()); fflush(stdout); }
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
static std::string ctl(const char *c) { FakeStr s{c}; return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devCtlNative(&g_env, nullptr, (jstring)&s))->s; }
static int peekS(int a) { std::lock_guard<std::mutex> l(g_mStroj); return (int)g_stroj->mem.ram[a & 0xFFFF]; }

// stejne diskety jako test_b348_sio.cpp
static uint8_t obsah(int s, int i) { return (uint8_t)(s * 7 + i * 13 + (s >> 8) * 5 + 1); }
static FakeArr atr(int sektoru, bool sProgramem) {
  FakeArr a;
  const size_t bajtu = (size_t)sektoru * 128, par = bajtu / 16;
  a.b.assign(16 + bajtu, 0);
  a.b[0] = 0x96; a.b[1] = 0x02; a.b[2] = par & 0xFF; a.b[3] = (par >> 8) & 0xFF; a.b[4] = 128;
  for (int s = 1; s <= sektoru; s++) for (int i = 0; i < 128; i++) a.b[16 + (size_t)(s - 1) * 128 + i] = obsah(s, i);
  if (sProgramem) for (size_t i = 0; i < sizeof(B348_KOD); i++) a.b[16 + i] = B348_KOD[i];
  return a;
}
static std::string nahraj(FakeArr &a, const char *jm) {
  FakeStr n{jm};
  return ((FakeStr *)Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devLoadAtrNative(&g_env, nullptr, (jbyteArray)&a, (jstring)&n))->s;
}

int main() {
  std::memset(&g_tbl, 0, sizeof(g_tbl));
  g_tbl.GetStringUTFChars = fGetStringUTFChars; g_tbl.ReleaseStringUTFChars = fReleaseStringUTFChars;
  g_tbl.NewStringUTF = fNewStringUTF; g_tbl.GetArrayLength = fGetArrayLength;
  g_tbl.GetByteArrayRegion = fGetByteArrayRegion; g_tbl.NewByteArray = fNewByteArray; g_tbl.SetByteArrayRegion = fSetByteArrayRegion;
  g_env.functions = &g_tbl;
  FakeStr surface{"plocha"};
  printf("B348 - skutecny nap_atari_native.cpp: samokontrola, OPTION pri bootu, vymena diskety a SIO pres POKEY\n\n");

  // 1) samokontrola
  {
    jstring r = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_runSelfTest(&g_env, nullptr);
    const std::string js = ((FakeStr *)r)->s;
    const size_t p = js.find("\"cpuHash\":\"");
    const std::string h = p == std::string::npos ? "?" : js.substr(p + 11, 8);
    printf("SELFTEST cpuHash %s\n", h.c_str());
    over(h == "4A2C6CBA", "samokontrola: cpuHash 4A2C6CBA = OCEKAVANY_CPU_HASH v NativeAtariCoreBridge.java (B348 LXA)");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStartNative(&g_env, nullptr, JNI_TRUE);
  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devSurfaceNative(&g_env, nullptr, (jobject)&surface, 1080, 2160);
  for (int i = 0; i < 160 && obrazovka().find("READY") == std::string::npos; i++) spi(100);
  over(obrazovka().find("READY") != std::string::npos, "Atari nastartovalo do READY");

  // 2) bootovaci disketa s programem -> D1: ; jak dlouho je OPTION drzene
  {
    FakeArr boot = atr(720, true);
    const std::string r = nahraj(boot, "b348_boot.atr");
    printf("devLoadAtr: %s\n", r.c_str());
    over(r.rfind("OK ", 0) == 0, "bootovaci disketa prijata do D1: (studeny start)");
    const long long f0 = g_snimkuCelkem.load();
    long long drzeno = -1, maxSnimek = -1;
    bool videnoDrzene = false;
    for (int i = 0; i < 600; i++) {
      int con; long long f;
      { std::lock_guard<std::mutex> l(g_mStroj); con = g_stroj->consol; f = g_snimkuCelkem.load(); }
      if (!(con & 4)) { videnoDrzene = true; maxSnimek = f - f0; }
      else if (videnoDrzene && drzeno < 0) drzeno = f - f0;
      if (drzeno >= 0 && peekS(0x6FE) == 0xA5) break;
      spi(5);
    }
    printf("OPTION drzene po vlozeni diskety: posledni snimek s OPTION %lld, uvolneno ve snimku %lld\n", maxSnimek, drzeno);
    over(videnoDrzene && drzeno >= 40 && drzeno <= 60, "OPTION drzene pri bootu ~1 s (B348: 50 snimku misto 150)");
    int portb; { std::lock_guard<std::mutex> l(g_mStroj); portb = g_stroj->mem.portB(); }
    over((portb & 2) != 0, "BASIC vypnuty (PORTB bit 1 = 1) - OS OPTION precetl vcas");
    for (int i = 0; i < 300 && peekS(0x6FE) != 0xA5; i++) spi(10);
    over(peekS(0x6FE) == 0xA5, "program z diskety nabootoval (sektory 1-5 pres OS) a ceka na vymenu diskety");
  }

  // 3) vymena diskety bez restartu + vlastni SIO programu
  {
    over(ctl("d1") == "b348_boot.atr", "v D1: je bootovaci disketa (devCtlNative \"d1\")");
    ctl("atr_vymena");
    FakeArr test = atr(720, false);
    const std::string r = nahraj(test, "b348_test.atr");
    printf("devLoadAtr (vymena): %s\n", r.c_str());
    over(r.find("VYMENENA bez restartu") != std::string::npos && peekS(0x6FE) == 0xA5, "VYMENIT DISKETU BEZ RESTARTU: program bezi dal, v D1: je testovaci disketa");
    const uint8_t tab[][8] = {
      {0x31, 'R', 4, 0, 128, 2, 0x30, 0},      // cteni sektoru 4
      {0x31, 'P', 10, 0, 128, 1, 0x34, 0},     // zapis sektoru 10 ze VZOR
      {0x31, 'R', 10, 0, 128, 2, 0x35, 0},     // a zpet
      {0x31, 'R', 0xD1, 2, 128, 2, 0x32, 0},   // 721 -> NAK
      {0, 0, 0, 0, 0, 0, 0, 0},
    };
    {
      std::lock_guard<std::mutex> l(g_mStroj);
      for (int k = 0; k < 5; k++) for (int i = 0; i < 8; i++) g_stroj->mem.ram[B348_TAB + k * 8 + i] = tab[k][i];
      g_stroj->mem.ram[0x6FF] = 1;
    }
    for (int i = 0; i < 1000 && peekS(0x6F0) != 0x5A; i++) spi(10);
    over(peekS(0x6F0) == 0x5A, "program dobehl (vsechny prikazy SIO pres POKEY)");
    std::lock_guard<std::mutex> l(g_mStroj);
    const uint8_t *res = &g_stroj->mem.ram[0x600];
    printf("vysledky: %c%c%c%d | %c%c%c%d | %c%c%c%d | %c%d%d%d  casovac %02X %02X chyby %02X\n",
           res[0], res[1] ? res[1] : '-', res[2], res[3], res[4], res[5], res[6], res[7], res[8], res[9] ? res[9] : '-', res[10], res[11],
           res[12], res[13], res[14], res[15], g_stroj->mem.ram[0x6F1], g_stroj->mem.ram[0x6F2], g_stroj->mem.ram[0x6F3]);
    bool data = true;
    for (int i = 0; i < 128; i++) if (g_stroj->mem.ram[0x3000 + i] != obsah(4, i)) data = false;
    over(res[0] == 'A' && res[2] == 'C' && res[3] == 1 && data, "cteni sektoru 4: 'A' 'C' + 128 B, soucet a data sedi");
    over(res[4] == 'A' && res[5] == 'A' && res[6] == 'C', "zapis sektoru 10: 'A' + datovy ramec + 'A' 'C'");
    bool zpet = true, naDisku = true;
    size_t off; int len;
    g_stroj->disk.sec(10, off, len);
    for (int i = 0; i < 128; i++) {
      const uint8_t v = (uint8_t)(i * 3 + 7);
      if (g_stroj->mem.ram[0x3500 + i] != v) zpet = false;
      if (g_stroj->disk.data[off + i] != v) naDisku = false;
    }
    over(res[8] == 'A' && res[10] == 'C' && res[11] == 1 && zpet && naDisku, "sektor 10 precten zpet = zapsana data (i v ATR v pameti appky)");
    over(res[12] == 'N', "neexistujici sektor 721 -> 'N' (NAK)");
    over(g_stroj->mem.ram[0x6F1] == 0x04 && g_stroj->mem.ram[0x6F2] == 0x00, "casovac 4: v asynchronnim prijmu stoji, bez nej bezi");
    over(g_stroj->mem.ram[0x6F3] == 0, "zadna chyba ramce / preteceni");
  }
  // log appky (to, co Rene posila): radek o vlastnim zavadeci
  {
    spi(100);
    jstring s = Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devPollLogNative(&g_env, nullptr);
    const std::string log = s ? ((FakeStr *)s)->s : std::string();
    over(log.find("B348 SIO: program mluvi s D1: primo pres POKEY") != std::string::npos, "v logu appky je radek B348 SIO (hra cte disketu vlastnim zavadecem)");
  }

  Java_eu_atarihelp_emu10_NativeAtariCoreBridge_devStopNative(&g_env, nullptr);
  printf("\nVYSLEDEK: %d kontrol, %d chyb\n", kontrol, chyb);
  return chyb ? 1 : 0;
}
