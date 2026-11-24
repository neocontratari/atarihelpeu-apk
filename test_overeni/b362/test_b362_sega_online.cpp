// test_overeni/b362/test_b362_sega_online.cpp
// B362: ONLINE SEGA S ODHADEM A OPRAVOU - SKUTECNY nap_sega_native_proof.cpp + jadro clownmdemu + testovaci ROM
// (../b357/sega_test_rom.py), dva "telefony" = dva procesy, kazdy se svym jadrem a vlaknem emulace presne jako v appce,
// spojeni pres engine (mono NapOnlineTest = trida NapOnline z NapServer.cs). Jen JNI jako z Javy.
// Spicky site: vlakno site telefonu obcas na 60-220 ms "usne" (jako Wi-Fi telefonu v logu S8 z 24. 11.) - nic neprijme
// ani neodesle (hacek NAP_ONLINE_TEST_SIT v nap_online_klient.h, v appce prazdny).
//   1) spicky na obou telefonech, stisky se meni kazdych 5-85 ms (odhad casto nesedi = hodne oprav): hra jede,
//      jadro se vraci k ulozenemu stavu a pocita znovu - otisky stavu po celou hru STEJNE (zadne rozjeti, zadna
//      synchronizace), otisk jadra ve snimku 660 u obou stejny = oprava skutecneho jadra je presna
//   2) STATISTIKA: telefon 1 vi o telefonu 2 (info telefonu z Javy - onlineTelefon, vykon jadra, odhady)
//   3) jadro hrace 2 se ve snimku 300 splete + spicky -> synchronizace stavu i s odhadem, hra pokracuje
//   4) telefon 2 zamrzne (SIGSTOP) -> telefon 1 nejvys ODHAD_MAX snimku s odhadem, pak stoji, po 15 s konec s duvodem
// Spusteni: ./test_b362_sega_online <port enginu>
#include <cstdint>
static void test_po_kroku(uint32_t snimek);
static uint64_t test_otisk_660();
static void test_sit();
#define NAP_SEGA_ONLINE_TEST_PO_KROKU(snimek) test_po_kroku(snimek)
#define NAP_ONLINE_TEST_SIT() test_sit()
#include "../../app/src/main/cpp/nap_sega_native_proof.cpp"
#include "sega_test_rom.h"   // ../b357/sega_test_rom.py do pracovni slozky (-I)

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <random>
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

// ---------- porucha jadra (jen tenhle test) ----------
static int g_test_porucha = -1;                 // snimek online hry, ve kterem se jadro "splete" (-1 = nikdy)
static std::atomic<uint64_t> g_test_otisk660{0};
static void test_po_kroku(uint32_t snimek) {   // vlakno emulace, po ClownMDEmu_Iterate, pred otiskem stavu
  if ((int)snimek == g_test_porucha) g_real.emu.state.m68k.ram[0x123] ^= 0x10;
  if (snimek == 660) g_test_otisk660.store(nap_sega_otisk_stavu());
}
static uint64_t test_otisk_660() { return g_test_otisk660.load(); }

// ---------- spicky site (jen tenhle test): vlakno site obcas "usne" ----------
static bool g_test_spicky = false;
static std::mt19937 g_test_sit_rng(5);
static std::chrono::steady_clock::time_point g_test_dalsi_spicka;
static std::atomic<uint32_t> g_test_spicek{0}, g_test_spicky_ms{0};
static void test_sit() {   // vlakno site, pred prijmem
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

// ---------- rucne sestaveny JNIEnv (retezce a pole retezcu) ----------
struct FakeStr { std::string s; };
struct FakeArr { std::vector<FakeStr *> p; };
static jstring JNICALL fNewStringUTF(JNIEnv *, const char *c) { return (jstring)(new FakeStr{c ? c : ""}); }
static const char *JNICALL fGetStringUTFChars(JNIEnv *, jstring s, jboolean *) { return ((FakeStr *)s)->s.c_str(); }
static void JNICALL fReleaseStringUTFChars(JNIEnv *, jstring, const char *) {}
static jsize JNICALL fGetArrayLength(JNIEnv *, jarray a) { return (jsize)((FakeArr *)a)->p.size(); }
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

static std::string stav() { return vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStav(&g_env, nullptr)); }
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

// =====================================================================================================
//  jeden "telefon" (proces): ./test hrac <port> <1|2> <kod|-> <seed> <snimku> <porucha|->
// =====================================================================================================
static int telefon(int port, bool hrac1, const std::string &kod, uint32_t seed, uint32_t snimku, int porucha, bool spicky) {
  pripravEnv();
  g_test_porucha = porucha;
  g_test_spicky = spicky;
  g_test_sit_rng.seed(seed * 7 + 1);
  const std::vector<uint8_t> rom(SEGA_TEST_ROM, SEGA_TEST_ROM + sizeof(SEGA_TEST_ROM));
  const std::string nahrani = nap_real_load_rom_bytes(rom.data(), rom.size());   // = realCoreLoadRom z Javy
  if (nahrani.find("REAL_CORE_THREAD_START_OK") == std::string::npos) { std::printf("VYSLEDEK nahrani=%s\n", nahrani.c_str()); return 1; }
  std::atomic<bool> konec{false};
  std::thread zvuk([&] {   // reproduktor telefonu (AudioTrack bere zvuk)
    std::vector<jshort> buf(2 * 480);
    while (!konec.load()) {
      nap_audio_pull_stereo(buf.data(), 480);
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  });
  const std::string otisk = vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineOtisk(&g_env, nullptr));
  const size_t bar = otisk.find('|');
  std::string cesta;
  if (hrac1)
    cesta = "/api/v1/online/zaloz?system=sega&hra=B362%20TEST%20ROM&hra_id=sega-0123456789abcdef&otisk=" + otisk.substr(0, bar) +
            "&nastaveni=" + otisk.substr(bar + 1) + "&jmeno=Telefon%201";
  else
    cesta = "/api/v1/online/pripoj?kod=" + kod + "&jmeno=Telefon%202";
  FakeStr a{"http://127.0.0.1:" + std::to_string(port)}, c{cesta}, j{hrac1 ? "Telefon 1" : "Telefon 2"};
  FakeStr h1{"User-Agent: NapTest-B362"}, h2{std::string("X-NaP-Zarizeni: test-telefon-") + (hrac1 ? "1" : "2")};
  FakeArr hl{{&h1, &h2}};
  {   // B362: Java da info o telefonu (MainActivity.napOnlineInfoTelefonu) - jde druhemu ve STATISTIKA
    FakeStr info{hrac1 ? "TEST-S8 Android 9, Wi-Fi HIGH_PERF" : "TEST-A12 Android 12, Wi-Fi -61 dBm 2.4 GHz LOW_LATENCY"};
    Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineTelefon(&g_env, nullptr, (jstring)&info);
  }
  const std::string st = vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStart(&g_env, nullptr, (jstring)&a, (jstring)&c, (jobjectArray)&hl,
                                                                                       hrac1 ? JNI_TRUE : JNI_FALSE, (jstring)&j));
  std::printf("START %s\n", st.c_str());
  std::fflush(stdout);
  auto ted = [] { return std::chrono::steady_clock::now(); };
  const auto t0 = ted();
  if (hrac1) {
    std::string k;
    while ((k = pole(stav(), "kod")).empty() && ted() - t0 < std::chrono::seconds(10)) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::printf("KOD %s\n", k.empty() ? "-" : k.c_str());
    std::fflush(stdout);
  }
  std::string s;
  while (ted() - t0 < std::chrono::seconds(25)) {
    s = stav();
    if (pole(s, "hraje") == "1" || pole(s, "faze") == "KONEC" || pole(s, "aktivni") == "0") break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::string videnoSync, videnoStoji;
  if (pole(s, "hraje") == "1") {
    std::printf("HRAJE\n");
    std::fflush(stdout);
    std::mt19937 g(seed);
    bool dotyk[8] = {};
    auto dalsiZmena = ted();
    const auto tHra = ted();
    while (ted() - tHra < std::chrono::seconds(60)) {
      if (ted() >= dalsiZmena) {
        const int i = (int)(g() % 8);
        dotyk[i] = !dotyk[i];
        Java_eu_atarihelp_emu10_NativeSegaCoreBridge_setInput(&g_env, nullptr, i, dotyk[i] ? JNI_TRUE : JNI_FALSE);
        dalsiZmena = ted() + std::chrono::milliseconds(5 + g() % 80);
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
      s = stav();
      if (pole(s, "sync") == "1") videnoSync = "1";
      const int stoji = std::atoi(pole(s, "stoji_ms").c_str());
      if (stoji > 0 && videnoStoji.empty()) videnoStoji = std::to_string(stoji);
      if (pole(s, "hraje") != "1") break;
      if (hrac1 && (uint32_t)std::stoul("0" + pole(s, "snimek")) >= snimku) {
        Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStop(&g_env, nullptr);
        break;
      }
    }
    const auto tk = ted();
    while (!hrac1 && pole(stav(), "hraje") == "1" && ted() - tk < std::chrono::seconds(10)) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  s = stav();
  std::printf("VYSLEDEK hrac=%d overeno=%s snimek=%s zpozdeni=%s zpozdeni_ted=%s cekani=%s cekani_ms=%s synchronizaci=%s synchronizovano=%s "
              "rozjeti=%s rozjeti_snimek=%s stav_kb=%s otisk660=%016llx sync_videno=%s faze=%s\n",
              hrac1 ? 1 : 2, pole(s, "overeno").c_str(), pole(s, "snimek").c_str(), pole(s, "zpozdeni").c_str(), pole(s, "zpozdeni_ted").c_str(),
              pole(s, "cekani").c_str(), pole(s, "cekani_ms").c_str(), pole(s, "synchronizaci").c_str(), pole(s, "synchronizovano").c_str(),
              pole(s, "rozjeti").c_str(), pole(s, "rozjeti_snimek").c_str(), pole(s, "stav_kb").c_str(), (unsigned long long)test_otisk_660(),
              videnoSync.empty() ? "0" : "1", pole(s, "faze").c_str());
  std::printf("ODHAD odhad=%s sedelo=%s oprav=%s oprava_max=%s oprava_max_ms=%s prepocteno=%s odhad_max=%s chyb_opravy=%s druhy_info_c=%s "
              "posledni_zprava=%s spicek=%u spicky_ms=%u navrat_ze=%s navrat_na=%s\n",
              pole(s, "odhad").c_str(), pole(s, "odhad_sedelo").c_str(), pole(s, "oprav").c_str(), pole(s, "oprava_max").c_str(),
              pole(s, "oprava_max_ms").c_str(), pole(s, "prepocteno").c_str(), pole(s, "odhad_max").c_str(), pole(s, "chyb_opravy").c_str(),
              pole(s, "druhy_info_c").c_str(), pole(s, "posledni_zprava").c_str(), g_test_spicek.load(), g_test_spicky_ms.load(),
              pole(s, "navrat_ze").c_str(), pole(s, "navrat_na").c_str());
  std::printf("DRUHY %s\n", pole(s, "druhy_info").c_str());
  std::printf("CASTI %s\nKONEC_TEXT %s\nCHYBA_TEXT %s\nPOLE %s\n", pole(s, "rozjeti_casti").c_str(), pole(s, "konec").c_str(), pole(s, "chyba").c_str(),
              (pole(s, "ticho_ms").empty() || pole(s, "dlouhych").empty() || pole(s, "zadosti").empty()) ? "chybi" : "vsechna");
  std::fflush(stdout);
  Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStop(&g_env, nullptr);
  konec = true;
  zvuk.join();
  vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_shutdown(&g_env, nullptr));
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
  char b[4096];
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
                    Dite &a, Dite &b, bool zmrazB = false, bool spicky = false) {
  const std::string sp = spicky ? "1" : "0";
  a = spust({"hrac", std::to_string(port), "1", "-", seedA, snimku, "-", sp});
  std::string r, kod;
  while (radek(a, r)) if (r.rfind("KOD ", 0) == 0) { kod = r.substr(4); break; }
  b = spust({"hrac", std::to_string(port), "2", kod.empty() ? "0000" : kod, seedB, snimku, poruchaB, sp});
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
  if (argc >= 9 && std::string(argv[1]) == "hrac")
    return telefon(std::atoi(argv[2]), std::string(argv[3]) == "1", argv[4], (uint32_t)std::atoi(argv[5]), (uint32_t)std::atoi(argv[6]),
                   std::string(argv[7]) == "-" ? -1 : std::atoi(argv[7]), std::string(argv[8]) == "1");
  pripravEnv();
  std::printf("B362 - online Sega s odhadem stisku druheho a opravou (skutecny nap_sega_native_proof.cpp + clownmdemu, protokol verze %u)\n",
              (unsigned)nap::online::VERZE);
  if (argc < 2) { std::printf("chybi port enginu\nVYSLEDEK: 0 OK, 1 CHYBA\n"); return 1; }
  const int port = std::atoi(argv[1]);

  std::printf("== 1) spicky site na obou telefonech, stisky se meni porad -> odhady, opravy, otisky stejne\n");
  {
    Dite a, b;
    dvojice(port, "71", "72", "1260", "-", a, b, false, true);
    const int sa = cislo(hodnota(a, "VYSLEDEK", "snimek")), sb = cislo(hodnota(b, "VYSLEDEK", "snimek"));
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
         "oprava skutecneho jadra je presna: overeno " + hodnota(a, "VYSLEDEK", "overeno") + " otisku, rozjeti 0, synchronizaci 0, otisk jadra ve snimku 660 u obou " +
             oa + (oa == ob ? " (STEJNY)" : " / " + ob));
    over(cislo(hodnota(a, "VYSLEDEK", "zpozdeni")) <= 3,
         "zpozdeni ovladani " + hodnota(a, "VYSLEDEK", "zpozdeni") + " -> " + hodnota(a, "VYSLEDEK", "zpozdeni_ted") + " / " +
             hodnota(b, "VYSLEDEK", "zpozdeni_ted") + " snimku, emulator stal " + hodnota(a, "VYSLEDEK", "cekani") + "x (" + hodnota(a, "VYSLEDEK", "cekani_ms") +
             " ms) / " + hodnota(b, "VYSLEDEK", "cekani") + "x (" + hodnota(b, "VYSLEDEK", "cekani_ms") + " ms), limit odhadu " +
             hodnota(a, "ODHAD", "odhad_max") + " / " + hodnota(b, "ODHAD", "odhad_max") + " snimku");
    const std::string da = hodnota(a, "DRUHY ");
    std::printf("       telefon 1 vi o telefonu 2: %s\n", da.c_str());
    over(cislo(hodnota(a, "ODHAD", "druhy_info_c")) >= 2 && da.find("TEST-A12 Android 12") != std::string::npos && da.find("snimku/s, jadro") != std::string::npos &&
             da.find("odhad ") != std::string::npos && hodnota(b, "DRUHY ").find("TEST-S8") != std::string::npos,
         "STATISTIKA: telefon 1 dostal od telefonu 2 " + hodnota(a, "ODHAD", "druhy_info_c") + "x jeho cisla (vykon jadra, odhady, opravy, info z Javy)");
  }

  std::printf("== 2) jadro hrace 2 se ve snimku 300 splete + spicky site -> synchronizace i s odhadem, hra pokracuje\n");
  {
    Dite a, b;
    dvojice(port, "11", "22", "780", "300", a, b, false, true);
    const int sa = cislo(hodnota(a, "VYSLEDEK", "snimek")), sb = cislo(hodnota(b, "VYSLEDEK", "snimek"));
    over(cislo(hodnota(a, "VYSLEDEK", "synchronizaci")) == 1 && cislo(hodnota(b, "VYSLEDEK", "synchronizovano")) == 1 &&
             hodnota(a, "VYSLEDEK", "rozjeti_snimek") == "300" && hodnota(a, "CASTI ") == "RAM 68000",
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
    dvojice(port, "55", "66", "100000", "-", a, b, true);
    const double s = std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count();
    const std::string k = hodnota(a, "KONEC_TEXT ");
    over(k.rfind("od druheho hrace dlouho nic neprislo", 0) == 0 && hodnota(a, "VYSLEDEK", "faze") == "KONEC" &&
             cislo(hodnota(a, "ODHAD", "odhad")) <= 60,
         "telefon 1: '" + k + "' (cele trvani " + std::to_string((int)s) + " s), s odhadem jen " + hodnota(a, "ODHAD", "odhad") +
             " snimku, posledni zprava od druheho: " + hodnota(a, "ODHAD", "posledni_zprava"));
  }
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
