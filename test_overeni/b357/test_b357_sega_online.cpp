// test_overeni/b357/test_b357_sega_online.cpp
// B357: ONLINE NA SEZE - SKUTECNY nap_sega_native_proof.cpp z appky (jadro clownmdemu prelozene vedle, Android =
// stuby z ../b291/stub, infrastruktura jako ../b355/test_b355_sega.cpp) + testovaci ROM (sega_test_rom.py: rucne
// prelozeny program 68000 - kazdy snimek precte OBA ovladace a zamicha je do RAM a barvy pozadi).
//  A) bez site (jadro primo, vstupy pres online vetev nap_real_input_requested):
//     1) stejne vstupy -> stejne otisky stavu (600 snimku), i po predchozi jine hre (studeny start maze vse)
//     2) jiny jediny stisk hrace 1 ve snimku 300 -> otisky pred nim stejne, od nej jine
//     3) totez hrac 2 ve snimku 400 (vstup druheho hrace opravdu dojde do hry)
//     4) kazde z 8 tlacitek (nahoru dolu vlevo vpravo A B C START) obou hracu zmeni stav
//  B) dva "telefony" = dva procesy, kazdy se svym jadrem a vlaknem emulace presne jako v appce; spojeni pres engine
//     (mono NapOnlineTest = trida NapOnline z NapServer.cs v12.4). Jen JNI: realCoreLoadRom-ekvivalent, onlineOtisk,
//     onlineStart, onlineStav, onlineStop, setInput (dotyk), ovladacKlavesa (BT ovladac) v nahodnych chvilich:
//     5) hrac 1 zalozi hru a dostane cislo, hrac 2 se pripoji, oba hraji
//     6) >= 10 overenych otisku stavu na obou telefonech, zadne rozjeti, vstupy obou hracu dorazi do obou jader
//     7) hrac 1 skonci -> hrac 2 "druhy hrac hru ukoncil"
//     8) hrac 2 s jinou ROM (1 bajt) -> oba "jinou hru", hra nezacne
// Spusteni: ./test_b357_sega_online <port enginu>   (engine musi bezet - viz JAK_SPUSTIT.txt)
#include "../../app/src/main/cpp/nap_sega_native_proof.cpp"
#include "sega_test_rom.h"   // vyrobi sega_test_rom.py do pracovni slozky (-I)

#include <sys/wait.h>
#include <unistd.h>

#include <array>
#include <cstdio>
#include <random>
#include <set>
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

static std::vector<uint8_t> testRom(bool jina) {
  std::vector<uint8_t> r(SEGA_TEST_ROM, SEGA_TEST_ROM + sizeof(SEGA_TEST_ROM));
  if (jina) r[0x160] ^= 0x01;   // jediny bajt v nazvu hry (kod stejny) - jiny otisk
  return r;
}

// =====================================================================================================
//  A) bez site
// =====================================================================================================
using Vstupy = std::vector<std::array<uint32_t, 2>>;

static void nahrajBezVlakna(const std::vector<uint8_t> &rom) {
  nap_real_detect_and_set_region(rom.data(), rom.size());
  std::lock_guard<std::mutex> lock(g_real_mutex);
  nap_real_reset_state_locked();
  g_real.cart_words.resize((rom.size() + 1) / 2);
  for (size_t i = 0, j = 0; i < rom.size(); i += 2, ++j)
    g_real.cart_words[j] = (cc_u16l)((rom[i] << 8) | (i + 1 < rom.size() ? rom[i + 1] : 0));
  ClownMDEmu_Constant_Initialise();
}

/** Jedna "hra" od studeneho startu: kazdy snimek vstupy obou hracu (jako je da relace) a otisk stavu po snimku. */
static std::vector<uint64_t> beh(const Vstupy &v) {
  {
    std::lock_guard<std::mutex> lock(g_real_mutex);
    nap_real_studeny_start_locked();
  }
  g_online_hraje.store(1);
  std::vector<uint64_t> h;
  for (const auto &x : v) {
    g_online_vstup[0].store(x[0]);
    g_online_vstup[1].store(x[1]);
    ClownMDEmu_Iterate(&g_real.emu);
    h.push_back(nap_sega_otisk_stavu());
  }
  g_online_hraje.store(0);
  return h;
}

/** Nahodne "stisky": kazde tlacitko obou hracu se obcas zmackne a drzi par snimku (jen 8 zakladnich). */
static Vstupy nahodne(uint32_t seed, size_t n) {
  std::mt19937 g(seed);
  Vstupy v(n);
  uint32_t a = 0, b = 0;
  for (size_t i = 0; i < n; i++) {
    if (g() % 4 == 0) a ^= 1u << (g() % 8);
    if (g() % 4 == 0) b ^= 1u << (g() % 8);
    v[i] = {a, b};
  }
  return v;
}

static int prvniRozdil(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y) {
  for (size_t i = 0; i < x.size() && i < y.size(); i++) if (x[i] != y[i]) return (int)i;
  return -1;
}
static int stejnychOd(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, size_t od) {
  int n = 0;
  for (size_t i = od; i < x.size() && i < y.size(); i++) if (x[i] == y[i]) n++;
  return n;
}

static void bezSite() {
  std::printf("A) bez site - jadro clownmdemu primo, vstupy obou hracu online vetvi (g_online_vstup)\n");
  nahrajBezVlakna(testRom(false));
  const Vstupy S = nahodne(1, 600);
  const std::vector<uint64_t> h1 = beh(S);
  std::set<uint64_t> ruzne(h1.begin(), h1.end());
  (void)beh(nahodne(2, 500));   // "jina hra" mezi tim
  const std::vector<uint64_t> h2 = beh(S);
  over(h1 == h2 && ruzne.size() >= 590, "1) stejne vstupy -> stejnych 600 otisku i po jine hre mezi tim (studeny start maze vse); ruznych otisku " +
                                           std::to_string(ruzne.size()) + " z 600 (stav se meni kazdy snimek)");

  Vstupy S1 = S;
  S1[300][0] ^= 1u << 4;   // hrac 1: A jinak jen ve snimku 300
  const std::vector<uint64_t> h3 = beh(S1);
  const int r1 = prvniRozdil(h1, h3);
  const int s1 = r1 < 0 ? -1 : stejnychOd(h1, h3, (size_t)r1);
  over(r1 == 300 && s1 == 0, "2) hrac 1 jinak jen ve snimku 300 -> prvni rozdil otisku ve snimku " + std::to_string(r1) +
                                 ", potom stejnych " + std::to_string(s1) + " (rozjeti je trvale - online by ho poznal)");

  Vstupy S2 = S;
  S2[400][1] ^= 1u << 7;   // hrac 2: START jinak jen ve snimku 400
  const std::vector<uint64_t> h4 = beh(S2);
  const int r2 = prvniRozdil(h1, h4);
  const int s2 = r2 < 0 ? -1 : stejnychOd(h1, h4, (size_t)r2);
  over(r2 == 400 && s2 == 0, "3) hrac 2 jinak jen ve snimku 400 -> prvni rozdil ve snimku " + std::to_string(r2) + ", potom stejnych " + std::to_string(s2));

  const Vstupy nic(64, std::array<uint32_t, 2>{0, 0});
  const uint64_t zaklad = beh(nic).back();
  static const char *J[8] = {"NAHORU", "DOLU", "VLEVO", "VPRAVO", "A", "B", "C", "START"};
  int ok = 0;
  std::string chyby;
  for (int hrac = 0; hrac < 2; hrac++)
    for (int b = 0; b < 8; b++) {
      Vstupy v = nic;
      v[10][(size_t)hrac] = 1u << b;
      if (beh(v).back() != zaklad) ok++;
      else chyby += std::string(" | hrac ") + std::to_string(hrac + 1) + " " + J[b] + " nic";
    }
  over(ok == 16, "4) kazde z 8 tlacitek obou hracu (1 snimek) zmeni stav hry: " + std::to_string(ok) + " z 16" + chyby);
}

// =====================================================================================================
//  B) jeden "telefon" (proces): ./test hrac <port> <1|2> <kod|-> <seed> <snimku> <dobra|jina>
// =====================================================================================================
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

static int telefon(int port, bool hrac1, const std::string &kod, uint32_t seed, uint32_t snimku, bool jinaRom) {
  pripravEnv();
  const std::vector<uint8_t> rom = testRom(jinaRom);
  const std::string nahrani = nap_real_load_rom_bytes(rom.data(), rom.size());   // = realCoreLoadRom z Javy
  if (nahrani.find("REAL_CORE_THREAD_START_OK") == std::string::npos) { std::printf("VYSLEDEK nahrani=%s\n", nahrani.c_str()); return 1; }
  std::atomic<bool> konec{false};
  std::thread zvuk([&] {   // reproduktor telefonu (AudioTrack bere zvuk) - jinak by fronta zvuku jen rostla
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
    cesta = "/api/v1/online/zaloz?system=sega&hra=B357%20TEST%20ROM&hra_id=sega-0123456789abcdef&otisk=" + otisk.substr(0, bar) +
            "&nastaveni=" + otisk.substr(bar + 1) + "&jmeno=Telefon%201";
  else
    cesta = "/api/v1/online/pripoj?kod=" + kod + "&jmeno=Telefon%202";
  FakeStr a{"http://127.0.0.1:" + std::to_string(port)}, c{cesta}, j{hrac1 ? "Telefon 1" : "Telefon 2"};
  FakeStr h1{"User-Agent: NapTest-B357"}, h2{std::string("X-NaP-Zarizeni: test-telefon-") + (hrac1 ? "1" : "2")};
  FakeArr hl{{&h1, &h2}};
  const std::string st = vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStart(&g_env, nullptr, (jstring)&a, (jstring)&c, (jobjectArray)&hl,
                                                                                       hrac1 ? JNI_TRUE : JNI_FALSE, (jstring)&j));
  std::printf("START %s otisk=%s\n", st.c_str(), otisk.c_str());
  std::fflush(stdout);

  auto ted = [] { return std::chrono::steady_clock::now(); };
  const auto t0 = ted();
  if (hrac1) {   // cislo hry pro druheho hrace (v appce ho vidi v seznamu her)
    std::string k;
    while ((k = pole(stav(), "kod")).empty() && ted() - t0 < std::chrono::seconds(10)) std::this_thread::sleep_for(std::chrono::milliseconds(20));
    std::printf("KOD %s\n", k.empty() ? "-" : k.c_str());
    std::fflush(stdout);
  }
  // ceka na start hry (oba telefony studeny start) nebo konec
  std::string s;
  while (ted() - t0 < std::chrono::seconds(25)) {
    s = stav();
    if (pole(s, "hraje") == "1" || pole(s, "faze") == "KONEC" || pole(s, "aktivni") == "0") break;
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  uint32_t videt[2] = {0, 0};
  if (pole(s, "hraje") == "1") {
    // nahodne stisky v nahodnych chvilich (dotyk i BT ovladac) + co jadro dostava za vstupy obou hracu
    std::mt19937 g(seed);
    static const int KLAVESY[] = {19, 20, 21, 22, 96, 97, 99, 108};   // D-pad, A B X START (Android KEYCODE)
    bool dotyk[8] = {}, klavesa[8] = {};
    auto dalsiZmena = ted();
    const auto tHra = ted();
    while (ted() - tHra < std::chrono::seconds(40)) {
      if (ted() >= dalsiZmena) {
        const int i = (int)(g() % 8);
        if (g() % 2) { dotyk[i] = !dotyk[i]; Java_eu_atarihelp_emu10_NativeSegaCoreBridge_setInput(&g_env, nullptr, i, dotyk[i] ? JNI_TRUE : JNI_FALSE); }
        else { klavesa[i] = !klavesa[i]; Java_eu_atarihelp_emu10_NativeSegaCoreBridge_ovladacKlavesa(&g_env, nullptr, KLAVESY[i], klavesa[i] ? JNI_TRUE : JNI_FALSE); }
        dalsiZmena = ted() + std::chrono::milliseconds(3 + g() % 60);
      }
      videt[0] |= g_online_vstup[0].load();
      videt[1] |= g_online_vstup[1].load();
      std::this_thread::sleep_for(std::chrono::milliseconds(1));
      if ((g() & 63) == 0) {
        s = stav();
        if (pole(s, "hraje") != "1") break;                                         // konec (druhy skoncil, rozjeti ...)
        if (hrac1 && (uint32_t)std::stoul("0" + pole(s, "snimek")) >= snimku) {   // hrac 1 dohral - konci
          Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStop(&g_env, nullptr);
          break;
        }
      }
    }
    // hrac 2: pocka, az se konec projevi (KONEC od hrace 1)
    const auto tk = ted();
    while (!hrac1 && pole(stav(), "hraje") == "1" && ted() - tk < std::chrono::seconds(10)) std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(200));
  s = stav();
  std::printf("VYSLEDEK hrac=%d overeno=%s snimek=%s zpozdeni=%s odezva=%s cekani=%s videt1=%d videt2=%d faze=%s\nKONEC_TEXT %s\nCHYBA_TEXT %s\n",
              hrac1 ? 1 : 2, pole(s, "overeno").c_str(), pole(s, "snimek").c_str(), pole(s, "zpozdeni").c_str(), pole(s, "odezva").c_str(),
              pole(s, "cekani").c_str(), videt[0] ? 1 : 0, videt[1] ? 1 : 0, pole(s, "faze").c_str(), pole(s, "konec").c_str(), pole(s, "chyba").c_str());
  std::fflush(stdout);
  Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStop(&g_env, nullptr);
  konec = true;
  zvuk.join();
  vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_shutdown(&g_env, nullptr));
  return 0;
}

// =====================================================================================================
//  B) dva telefony (procesy) a engine
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

static void dvaTelefony(int port) {
  std::printf("B) dva telefony = dva procesy (kazdy sve jadro a vlakno emulace), spojeni pres engine na portu %d\n", port);
  std::fflush(stdout);
  {
    Dite a = spust({"hrac", std::to_string(port), "1", "-", "11", "720", "dobra"});
    std::string r, kod;
    while (radek(a, r)) {
      if (r.rfind("KOD ", 0) == 0) { kod = r.substr(4); break; }
    }
    Dite b = spust({"hrac", std::to_string(port), "2", kod.empty() ? "0000" : kod, "22", "720", "dobra"});
    dobehni(a);
    dobehni(b);
    for (const auto &x : a.radky) std::printf("   telefon 1 | %s\n", x.c_str());
    for (const auto &x : b.radky) std::printf("   telefon 2 | %s\n", x.c_str());
    const int sa = cislo(hodnota(a, "VYSLEDEK", "snimek")), sb = cislo(hodnota(b, "VYSLEDEK", "snimek"));
    over(kod.size() == 4 && hodnota(a, "START ").rfind("OK", 0) == 0 && hodnota(b, "START ").rfind("OK", 0) == 0 && sa >= 720 && sb >= 600,
         "5) hrac 1 zalozil hru (cislo " + kod + "), hrac 2 se pripojil, hrali: telefon 1 " + std::to_string(sa) + " snimku, telefon 2 " +
             std::to_string(sb) + " (zpozdeni vstupu " + hodnota(a, "VYSLEDEK", "zpozdeni") + " snimky)");
    const int oa = cislo(hodnota(a, "VYSLEDEK", "overeno")), ob = cislo(hodnota(b, "VYSLEDEK", "overeno"));
    const bool rozjeti = hodnota(a, "KONEC_TEXT ").find("rozjela") != std::string::npos || hodnota(b, "KONEC_TEXT ").find("rozjela") != std::string::npos ||
                         hodnota(a, "CHYBA_TEXT ").find("rozjela") != std::string::npos || hodnota(b, "CHYBA_TEXT ").find("rozjela") != std::string::npos;
    const bool videt = hodnota(a, "VYSLEDEK", "videt1") == "1" && hodnota(a, "VYSLEDEK", "videt2") == "1" && hodnota(b, "VYSLEDEK", "videt1") == "1" &&
                       hodnota(b, "VYSLEDEK", "videt2") == "1";
    over(oa >= 10 && ob >= 10 && !rozjeti && videt, "6) overenych otisku stavu: telefon 1 " + std::to_string(oa) + ", telefon 2 " + std::to_string(ob) +
                                                     " (kazdych 60 snimku), rozjeti " + (rozjeti ? "ANO" : "zadne") + ", vstupy obou hracu v obou jadrech " +
                                                     (videt ? "ano" : "NE"));
    over(hodnota(b, "KONEC_TEXT ").find("druhy hrac hru ukoncil") != std::string::npos && hodnota(a, "CHYBA_TEXT ").find("jsi ukoncil") != std::string::npos,
         "7) hrac 1 skoncil -> telefon 2: '" + hodnota(b, "KONEC_TEXT ") + "', telefon 1: '" + hodnota(a, "CHYBA_TEXT ") + "'");
  }
  {
    Dite a = spust({"hrac", std::to_string(port), "1", "-", "33", "720", "dobra"});
    std::string r, kod;
    while (radek(a, r)) {
      if (r.rfind("KOD ", 0) == 0) { kod = r.substr(4); break; }
    }
    Dite b = spust({"hrac", std::to_string(port), "2", kod.empty() ? "0000" : kod, "44", "720", "jina"});
    dobehni(a);
    dobehni(b);
    for (const auto &x : a.radky) std::printf("   telefon 1 | %s\n", x.c_str());
    for (const auto &x : b.radky) std::printf("   telefon 2 | %s\n", x.c_str());
    const std::string ka = hodnota(a, "KONEC_TEXT "), kb = hodnota(b, "KONEC_TEXT ");
    over(ka.find("jinou hru") != std::string::npos && kb.find("jinou hru") != std::string::npos && cislo(hodnota(a, "VYSLEDEK", "snimek")) == 0 &&
             cislo(hodnota(b, "VYSLEDEK", "snimek")) == 0,
         "8) hrac 2 ma jinou ROM (1 bajt) -> telefon 1: '" + ka + "', telefon 2: '" + kb + "', online snimku " + hodnota(a, "VYSLEDEK", "snimek") + " / " +
             hodnota(b, "VYSLEDEK", "snimek") + " (hra nezacala)");
  }
}

int main(int argc, char **argv) {
  if (argc >= 8 && std::string(argv[1]) == "hrac")
    return telefon(std::atoi(argv[2]), std::string(argv[3]) == "1", argv[4], (uint32_t)std::atoi(argv[5]), (uint32_t)std::atoi(argv[6]),
                   std::string(argv[7]) == "jina");
  pripravEnv();
  std::printf("B357 - online hra na Seze: skutecny nap_sega_native_proof.cpp + jadro clownmdemu + testovaci ROM 68000 (sega_test_rom.py)\n");
  bezSite();
  if (argc >= 2) dvaTelefony(std::atoi(argv[1]));
  else std::printf("(B preskoceno - chybi port enginu)\n");
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb ? 1 : 0;
}
