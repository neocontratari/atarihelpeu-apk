// B358: KAZDY POKUS O ONLINE MA SVE CISLO (start=N v onlineStav) - skutecny nap_sega_native_proof.cpp.
// Reneho log z 23. 11.: telefon 1 zalozil hru 4x, kazdy pokus po par vterinach skoncil a duvod v logu nebyl. B358 zapisuje
// stav online do logu (Java NapOnlineHlidac) - pokus, ktery skonci hned a se STEJNYM duvodem jako ten minuly (napr. engine
// nedostupny), se ale pozna jen podle cisla pokusu. Tady: 3 pokusy proti zavrenemu portu (engine "nebezi") ->
// start=1, 2, 3, pokazde faze KONEC se stejnym duvodem z C++ ("spojeni se serverem nejde: ..."), a hra se pri tom nezastavi.
// Preklad jako test_overeni/b357 (JAK_SPUSTIT.txt bod 3) + -I../b357:
//   g++ -std=c++17 -O2 -pthread -Wno-unused-function $SEGA -IPRAC -I../b357 -o PRAC/test_b358_start test_b358_start.cpp PRAC/clown/*.o \
//       -l:libEGL.so.1 -l:libGLESv2.so.2   ->   PRAC/test_b358_start > VYSTUP_test_b358_start.txt
#define main b357_hlavni
#include "test_b357_sega_online.cpp"
#undef main

int main() {
  pripravEnv();
  const std::vector<uint8_t> rom = testRom(false);
  const std::string nahrani = nap_real_load_rom_bytes(rom.data(), rom.size());
  over(nahrani.find("REAL_CORE_THREAD_START_OK") != std::string::npos, "hra nahrana (testovaci ROM 68000)");
  const std::string otisk = vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineOtisk(&g_env, nullptr));
  const size_t bar = otisk.find('|');
  std::vector<std::string> starty, faze, duvody;
  for (int i = 0; i < 3; i++) {
    FakeStr a{"http://127.0.0.1:9"}, j{"Telefon 1"};
    FakeStr c{"/api/v1/online/zaloz?system=sega&hra=B358&hra_id=&otisk=" + otisk.substr(0, bar) + "&nastaveni=" + otisk.substr(bar + 1) + "&jmeno=T"};
    FakeStr h1{"User-Agent: NapTest-B358"};
    FakeArr hl{{&h1}};
    const std::string st = vezmi(Java_eu_atarihelp_emu10_NativeSegaCoreBridge_onlineStart(&g_env, nullptr, (jstring)&a, (jstring)&c,
                                                                                         (jobjectArray)&hl, JNI_TRUE, (jstring)&j));
    const auto t0 = std::chrono::steady_clock::now();
    std::string s = stav();
    while (pole(s, "faze") != "KONEC" && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(5)) {
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
      s = stav();
    }
    starty.push_back(pole(s, "start"));
    faze.push_back(pole(s, "faze"));
    duvody.push_back(pole(s, "chyba"));
    std::printf("pokus %d: start %s -> faze=%s start=%s chyba=%s\n", i + 1, st.c_str(), pole(s, "faze").c_str(), pole(s, "start").c_str(), pole(s, "chyba").c_str());
  }
  over(starty == std::vector<std::string>{"1", "2", "3"}, "3 pokusy -> start=" + starty[0] + ", " + starty[1] + ", " + starty[2] + " (kazdy pokus sve cislo)");
  over(faze == std::vector<std::string>{"KONEC", "KONEC", "KONEC"} && duvody[0].rfind("spojeni se serverem nejde", 0) == 0 && duvody[0] == duvody[1]
           && duvody[1] == duvody[2],
       "pokazde KONEC se STEJNYM duvodem (\"" + duvody[0] + "\") - bez cisla pokusu by log nove pokusy nerozlisil");
  // hra pri tom bezi dal (pokusy o spojeni emulaci nezastavi)
  const std::string s1 = pole(stav(), "snimek");
  const uint64_t a1 = nap_sega_otisk_stavu();
  std::this_thread::sleep_for(std::chrono::milliseconds(300));
  over(nap_sega_otisk_stavu() != a1, "hra po neuspesnych pokusech bezi dal (stav jadra se meni)");
  Java_eu_atarihelp_emu10_NativeSegaCoreBridge_shutdown(&g_env, nullptr);
  std::printf("VYSLEDEK: %d OK, %d CHYBA\n", g_ok, g_chyb);
  return g_chyb == 0 ? 0 : 1;
}
