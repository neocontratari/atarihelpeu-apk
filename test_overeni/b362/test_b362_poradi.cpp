// test_overeni/b362/test_b362_poradi.cpp
// B362: ONLINE HRA NEZACALA, KDYZ ENGINE PREDAL "AHOJ" HRACE 1 DRIV NEZ ZPRAVU "hrac" HRACI 2.
// Nalezeno pri regresi test_b357_online (visel obcas, i s B361: 1x z 25, pod zatezi casteji): engine po pripojeni
// posle {"typ":"hrac"} nejdriv hraci 1 a az pak hraci 2. Hrac 1 hned posle AHOJ - kdyz ho engine preda hraci 2
// drive nez jeho zpravu "hrac" (vlakno enginu mezi dvema zpravami "usne" - na Windows klidne 15 ms), klient hrace 2
// binarni zpravu bez relace zahodil -> AHOJ hrace 1 nikdy nedostal, START ignoroval, hra nezacala: hrac 1 stal na
// snimku D, pingy chodily dal, takze ani 15 s ticha hru neukoncilo. B362: klient zpravy pred "hrac" podrzi
// (nap_online_klient.h, predRelaci) a po vzniku relace je preda.
//   Engine: test/NapOnlineTest.cs + NapServer.cs v12.4.5 (balicek enginu u Reneho / Clauda, NE v repozitari) s
//   jednim radkem navic JEN PRO TEST - mezi dvema PosliText(... "hrac" ...) v Pripoj():
//     Thread.Sleep(NAP_TEST_ZDRZ_HRAC2_MS z prostredi)   (vlakno enginu "usne" mezi zpravami)
//   g++ -std=c++17 -O2 -pthread -Wall -Wextra -I../../app/src/main/cpp -o test_b362_poradi test_b362_poradi.cpp
//   NAP_TEST_ZDRZ_HRAC2_MS=150 mono NapOnlineTest_zdrz.exe --port N &
//   ./test_b362_poradi http://127.0.0.1:N          -> VYSTUP_test_b362_poradi.txt
//   Proti klientovi B361 (stare hlavicky online/*.h z gitu): hra nezacne -> CHYBA (VYSTUP_test_b362_poradi_proti_b361.txt)
#include "online/nap_online_klient.h"

#include <cstdio>
#include <functional>
#include <thread>

using namespace nap::online;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

static const char *OTISK = "0123456789abcdef0123456789abcdef01234567";
static Ahoj ahoj(const char *jmeno) {
  Ahoj a;
  a.jadro = J_SEGA; a.nastaveni = 7; a.jmeno = jmeno;
  for (int i = 0; i < 20; i++) a.hra[i] = (uint8_t)(0x10 + i);
  return a;
}
static std::vector<std::string> podpis(const char *zarizeni) {
  return {"User-Agent: NapVision/B362 (Android test; PC)", std::string("X-NaP-Zarizeni: ") + zarizeni, "X-NaP-Cas: 1763600000", "X-NaP-Podpis: 00"};
}
static bool cekej(std::function<bool()> f, int ms) {
  const auto konec = std::chrono::steady_clock::now() + std::chrono::milliseconds(ms);
  while (!f()) { if (std::chrono::steady_clock::now() > konec) return false; std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
  return true;
}

// vlakno emulace (lockstep jako b357): kolik snimku dohral, nejvys 'limitS' s
static uint32_t emulace(OnlineKlient &k, int hrac, uint32_t celkem, int limitS) {
  const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(limitS);
  while (!k.vezmiStart()) {
    if (k.faze() == OnlineKlient::Faze::KONEC || std::chrono::steady_clock::now() > limit) return 0;
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
  std::shared_ptr<Relace> r = k.relace();
  auto dalsi = std::chrono::steady_clock::now();
  uint32_t f = 0;
  for (; f < celkem; f++) {
    r->lokalni(f, hrac == 1 ? (f / 7) & 0xFF : (f / 11) & 0xFF);
    uint32_t p1 = 0, p2 = 0;
    int v;
    while ((v = r->vstupy(f, p1, p2, 50)) == 0) {
      if (std::chrono::steady_clock::now() > limit) return f;
    }
    if (v < 0) return f;
    dalsi += std::chrono::microseconds(16667);
    if (dalsi > std::chrono::steady_clock::now()) std::this_thread::sleep_until(dalsi);
    else dalsi = std::chrono::steady_clock::now();
  }
  return f;
}

int main(int argc, char **argv) {
  const std::string adresa = argc > 1 ? argv[1] : "http://127.0.0.1:18791";
  std::printf("B362 - engine preda AHOJ hrace 1 drive nez zpravu \"hrac\" hraci 2 (vlakno enginu mezi zpravami usne)\n");
  for (int pokus = 1; pokus <= 3; pokus++) {
    OnlineKlient ka, kb;
    OnlineKlient::Nastaveni na;
    na.adresa = adresa;
    na.cesta = std::string("/api/v1/online/zaloz?system=sega&hra=Sonic&otisk=") + OTISK + "&nastaveni=7&jmeno=Rene";
    na.hlavicky = podpis("tel-a"); na.hrac1 = true; na.ja = ahoj("Rene");
    ka.start(na);
    cekej([&] { return ka.faze() == OnlineKlient::Faze::CEKA_NA_HRACE && !ka.kodMistnosti().empty(); }, 3000);
    OnlineKlient::Nastaveni nb = na;
    nb.cesta = "/api/v1/online/pripoj?kod=" + ka.kodMistnosti() + "&jmeno=bracha"; nb.hlavicky = podpis("tel-b"); nb.hrac1 = false; nb.ja = ahoj("bracha");
    kb.start(nb);
    uint32_t sa = 0, sb = 0;
    std::thread ea([&] { sa = emulace(ka, 1, 240, 8); }), eb([&] { sb = emulace(kb, 2, 240, 8); });
    ea.join(); eb.join();
    std::shared_ptr<Relace> rb = kb.relace();
    over(sa == 240 && sb == 240 && rb && rb->mamAhojDruheho(),
         "pokus " + std::to_string(pokus) + ": hra zacala a dobehla - hrac 1 " + std::to_string(sa) + " snimku, hrac 2 " + std::to_string(sb) +
             " (hrac 2 ma AHOJ hrace 1: " + (rb && rb->mamAhojDruheho() ? "ano" : "NE") + ", stav hrace 2: " + kb.popis() + ")");
    ka.stop(); kb.stop();
  }
  std::printf("VYSLEDEK: %d OK, %d CHYB\n", g_ok, g_chyb);
  return g_chyb == 0 ? 0 : 1;
}
