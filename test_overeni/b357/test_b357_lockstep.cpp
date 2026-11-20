// test_overeni/b357/test_b357_lockstep.cpp
// B357: ONLINE HRANI - synchronizace dvou emulatoru (app/src/main/cpp/online/nap_online.h) a SHA-1 / handshake
// WebSocketu (nap_websocket.h). Dva "telefony" = dve vlakna emulace + dve vlakna site; mezi nimi simulovana
// sit jako TCP (poradi zachovane) se zpozdenim a kolisanim. Emulator = deterministicky stroj (stav + 4 KB RAM),
// vstupy hracu podle skriptu. Overuje se, ze oba telefony maji v kazdem snimku STEJNY obraz (otisk), ze vstupy
// hrace 1 / 2 jdou na spravna mista, volba zpozdeni z odezvy, poznani rozjeti, jine hry, jine verze, ticha a konce.
//   g++ -std=c++17 -O2 -pthread -Wall -Wextra -I../../app/src/main/cpp -o test_b357_lockstep test_b357_lockstep.cpp
//   ./test_b357_lockstep                                    -> VYSTUP_test_b357_lockstep.txt
#include "online/nap_online.h"
#include "online/nap_websocket.h"

#include <atomic>
#include <cstdio>
#include <deque>
#include <random>
#include <thread>

using namespace nap::online;

static int g_ok = 0, g_chyb = 0;
static void over(bool ok, const std::string &co) {
  (ok ? g_ok : g_chyb)++;
  std::printf("%s %s\n", ok ? "OK    " : "CHYBA ", co.c_str());
  std::fflush(stdout);
}

// --------------------------------------------------------------------------- simulovana sit (jako TCP)
struct Linka {
  std::mutex m;
  std::deque<std::pair<uint64_t, std::vector<uint8_t>>> q;   // (cas doruceni us, zprava)
  uint64_t posledniDoruceni = 0;
  int zpozdeniMs = 30, kolisaniMs = 0;
  std::mt19937 rng{7};
  std::atomic<bool> zamrzla{false};                          // nic nedoruci (druhy telefon "zmizel")
  void posli(const std::vector<uint8_t> &z) {
    std::lock_guard<std::mutex> l(m);
    uint64_t t = tedUs() + (uint64_t)zpozdeniMs * 1000 + (kolisaniMs ? (uint64_t)(rng() % (kolisaniMs * 1000 + 1)) : 0);
    if (t < posledniDoruceni) t = posledniDoruceni;          // TCP: poradi zachovane
    posledniDoruceni = t;
    q.push_back({t, z});
  }
  bool vezmi(std::vector<uint8_t> &z) {
    if (zamrzla) return false;
    std::lock_guard<std::mutex> l(m);
    if (q.empty() || q.front().first > tedUs()) return false;
    z.swap(q.front().second);
    q.pop_front();
    return true;
  }
};

// --------------------------------------------------------------------------- deterministicky "emulator"
struct Stroj {
  uint64_t s = 0x1234567;
  uint8_t ram[4096] = {};
  uint64_t krok(uint32_t p1, uint32_t p2) {
    for (int i = 0; i < 64; i++) {
      s = s * 6364136223846793005ULL + 1442695040888963407ULL + p1 * 31 + p2 * 17;
      ram[(s >> 20) % 4096] ^= (uint8_t)(s >> 40);
    }
    return otisk(ram, sizeof(ram), s);     // "obraz snimku"
  }
};

// vstup hrace podle skriptu (hrac 1 / 2 jine vzory, meni se kazdych par snimku)
static uint32_t skript(int hrac, uint32_t f) { return hrac == 1 ? ((f / 7) * 2654435761u) & 0xFFF : (((f / 11) * 40503u) ^ 0x5A5) & 0xFFF; }

struct Telefon {
  int hrac;
  Relace *r = nullptr;
  Linka *ven = nullptr, *dovnitr = nullptr;
  std::atomic<bool> konecSite{false};
  std::atomic<bool> siteZamrzla{false};
  std::vector<uint64_t> obrazy;
  std::vector<std::pair<uint32_t, uint32_t>> vstupy;
  uint32_t snimku = 0;
  int poruchaVeSnimku = -1;      // test rozjeti: v tomto snimku se emulator "splete"
  uint64_t tichoUs = 15000000ULL;
  void sit() {
    r->zacni(tedUs());
    while (!konecSite) {
      r->cekejNaOdchozi(2);
      if (!siteZamrzla) {
        for (auto &z : r->vyzvedni()) ven->posli(z);
        std::vector<uint8_t> z;
        while (dovnitr->vezmi(z)) r->prijmi(z.data(), z.size(), tedUs());
      }
      r->tik(tedUs(), tichoUs);
      if (r->konec()) {   // dorucit posledni zpravy (KONEC) a skoncit - zmizely telefon uz nic neposle
        if (!siteZamrzla) for (auto &z : r->vyzvedni()) ven->posli(z);
        break;
      }
    }
  }
  void emulace(uint32_t celkem, double snimekMs) {
    Stroj st;
    while (!r->bezi() && !r->konec()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto dalsi = std::chrono::steady_clock::now();
    for (uint32_t f = 0; f < celkem; f++) {
      r->lokalni(f, skript(hrac, f));
      uint32_t p1 = 0, p2 = 0;
      int v;
      while ((v = r->vstupy(f, p1, p2, 50)) == 0) {}
      if (v < 0) break;
      if ((int)f == poruchaVeSnimku) st.ram[5] ^= 1;
      obrazy.push_back(st.krok(p1, p2));
      vstupy.push_back({p1, p2});
      r->otiskStavu(f, otisk(st.ram, sizeof(st.ram), st.s));
      snimku = f + 1;
      dalsi += std::chrono::microseconds((long long)(snimekMs * 1000));
      if (dalsi > std::chrono::steady_clock::now()) std::this_thread::sleep_until(dalsi);
      else dalsi = std::chrono::steady_clock::now();
    }
  }
};

struct Vysledek { Telefon a, b; std::unique_ptr<Relace> ra, rb; Linka ab, ba; };

static void hraj(Vysledek &v, int zpozdeniMs, int kolisaniMs, uint32_t snimku, Ahoj ja, Ahoj on,
                 int poruchaB = -1, bool zamrzniB = false, uint64_t tichoUs = 15000000ULL, int bKonciVeSnimku = -1) {
  const double snimekMs = 1000.0 / 60.0;
  v.ab.zpozdeniMs = v.ba.zpozdeniMs = zpozdeniMs;
  v.ab.kolisaniMs = v.ba.kolisaniMs = kolisaniMs;
  v.ra.reset(new Relace(true, ja, snimekMs));
  v.rb.reset(new Relace(false, on, snimekMs));
  v.a.hrac = 1; v.a.r = v.ra.get(); v.a.ven = &v.ab; v.a.dovnitr = &v.ba; v.a.tichoUs = tichoUs;
  v.b.hrac = 2; v.b.r = v.rb.get(); v.b.ven = &v.ba; v.b.dovnitr = &v.ab; v.b.tichoUs = tichoUs;
  v.b.poruchaVeSnimku = poruchaB;
  std::thread sa([&] { v.a.sit(); }), sb([&] { v.b.sit(); });
  std::thread ea([&] { v.a.emulace(snimku, snimekMs); });
  std::thread eb([&] {
    if (bKonciVeSnimku >= 0) {
      v.b.emulace((uint32_t)bKonciVeSnimku, snimekMs);
      v.rb->ukonci();
    } else if (zamrzniB) {
      v.b.emulace(120, snimekMs);
      v.b.siteZamrzla = true;          // telefon B zmizel (bez KONEC)
    } else {
      v.b.emulace(snimku, snimekMs);
    }
  });
  ea.join(); eb.join();
  // obe strany dohraly (nebo skoncily) - site ukoncit
  const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while ((!v.ra->konec() || !v.rb->konec()) && std::chrono::steady_clock::now() < limit && (poruchaB >= 0 || zamrzniB || bKonciVeSnimku >= 0))
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  v.a.konecSite = v.b.konecSite = true;
  sa.join(); sb.join();
}

static Ahoj ahoj(const char *jmeno, uint8_t jadro = J_SEGA, uint8_t hraBajt = 0x42, uint32_t nast = 7, uint8_t verze = VERZE) {
  Ahoj a;
  a.jadro = jadro; a.nastaveni = nast; a.jmeno = jmeno; a.verze = verze;
  for (int i = 0; i < 20; i++) a.hra[i] = (uint8_t)(hraBajt + i);
  return a;
}

int main() {
  std::printf("== 1) SHA-1 a handshake WebSocketu (RFC 6455)\n");
  {
    Sha1 h; h.pridej("abc", 3); uint8_t o[20]; h.konec(o);
    char hex[41]; for (int i = 0; i < 20; i++) std::snprintf(hex + 2 * i, 3, "%02x", o[i]);
    over(std::string(hex) == "a9993e364706816aba3e25717850c26c9cd0d89d", std::string("SHA-1(\"abc\") = ") + hex);
    std::string milion(1000000, 'a');
    Sha1 h2; h2.pridej(milion.data(), milion.size()); h2.konec(o);
    for (int i = 0; i < 20; i++) std::snprintf(hex + 2 * i, 3, "%02x", o[i]);
    over(std::string(hex) == "34aa973cd4c4daa4f61eeb2bdbad27316534016f", "SHA-1(milion x 'a') (FIPS 180 vektor)");
    over(wsPrijeti("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=", "Sec-WebSocket-Accept = priklad z RFC 6455");
    std::string host; int port = 0;
    over(WebSocket::rozloz("http://192.0.2.10:8080", host, port) && host == "192.0.2.10" && port == 8080, "adresa enginu http://192.0.2.10:8080 -> host + port");
    over(!WebSocket::rozloz("https://example.com", host, port), "https (TLS) neumim -> false (engine je http)");
  }

  std::printf("== 2) zpozdeni vstupu z odezvy (60 snimku/s)\n");
  over(zpozdeniZOdezvy(20, 2, 16.67) == 2, "LAN 20 ms -> D " + std::to_string(zpozdeniZOdezvy(20, 2, 16.67)));
  over(zpozdeniZOdezvy(80, 10, 16.67) == 5, "internet CZ 80 ms -> D " + std::to_string(zpozdeniZOdezvy(80, 10, 16.67)));
  over(zpozdeniZOdezvy(2000, 300, 16.67) == ZPOZDENI_MAX, "2 s -> strop " + std::to_string(ZPOZDENI_MAX));

  struct Sc { const char *co; int ms, kol; };
  const Sc sc[] = {{"LAN 5 ms", 5, 2}, {"internet 40 ms +-15 ms", 40, 15}, {"mobilni sit 90 ms +-40 ms", 90, 40}};
  int i = 3;
  for (const Sc &s : sc) {
    std::printf("== %d) %s, 420 snimku (7 s)\n", i++, s.co);
    Vysledek v;
    hraj(v, s.ms, s.kol, 420, ahoj("Rene"), ahoj("bracha"));
    bool stejne = v.a.obrazy.size() == 420 && v.a.obrazy == v.b.obrazy;
    size_t prvniRozdil = 0;
    while (prvniRozdil < std::min(v.a.obrazy.size(), v.b.obrazy.size()) && v.a.obrazy[prvniRozdil] == v.b.obrazy[prvniRozdil]) prvniRozdil++;
    over(stejne, "oba telefony 420 snimku, KAZDY obraz stejny (a " + std::to_string(v.a.obrazy.size()) + ", b " + std::to_string(v.b.obrazy.size()) + ", prvni rozdil " + std::to_string(prvniRozdil) + ")");
    bool spravnaMista = v.a.vstupy == v.b.vstupy;
    const uint32_t D = v.ra->zpozdeni();
    for (uint32_t f = D; f < 420 && spravnaMista; f++) spravnaMista = v.a.vstupy[f].first == skript(1, f - D) && v.a.vstupy[f].second == skript(2, f - D);
    over(spravnaMista, "hrac 1 (zalozil) = 1. ovladac, hrac 2 (pripojil se) = 2. ovladac, na obou stejne, vstup plati o D snimku pozdeji");
    over(v.rb->zpozdeni() == D && D >= ZPOZDENI_MIN && D <= ZPOZDENI_MAX, "zpozdeni vstupu D = " + std::to_string(D) + " snimku (" + std::to_string((int)(D * 16.7)) + " ms) na obou, odezva " + std::to_string(v.ra->odezvaMs()) + " ms");
    over(v.ra->overenychOtisku() >= 6 && v.rb->overenychOtisku() >= 6 && !v.ra->konec() && !v.rb->konec(),
         "otisk stavu overen " + std::to_string(v.ra->overenychOtisku()) + "x / " + std::to_string(v.rb->overenychOtisku()) + "x, nerozjelo se");
    std::printf("      (cekani na druheho: a %u x, b %u x)\n", v.ra->cekani(), v.rb->cekani());
  }

  std::printf("== 6) rozjeti: emulator B se ve snimku 200 splete -> poznaji to oba (otisk v 240)\n");
  {
    Vysledek v;
    hraj(v, 20, 5, 420, ahoj("Rene"), ahoj("bracha"), 200);
    over(v.ra->konec() && v.rb->konec() && v.ra->duvod() == D_ROZJETO && v.rb->duvod() == D_ROZJETO,
         std::string("oba konci: ") + popisDuvodu(v.ra->duvod()) + " (a ve snimku " + std::to_string(v.ra->rozjetoVeSnimku()) + ", b " + std::to_string(v.rb->rozjetoVeSnimku()) + ")");
    over(v.ra->rozjetoVeSnimku() == 240 || v.rb->rozjetoVeSnimku() == 240, "prvni porovnani po chybe = snimek 240 (otisky kazdych 60)");
  }

  std::printf("== 7) jina hra / jine nastaveni / jina verze -> nezacne, oba vedi proc\n");
  {
    Vysledek v;
    hraj(v, 10, 0, 60, ahoj("Rene", J_SEGA, 0x42), ahoj("bracha", J_SEGA, 0x43));
    over(v.ra->duvod() == D_JINA_HRA && v.rb->duvod() == D_JINA_HRA && v.a.obrazy.empty() && v.b.obrazy.empty(), std::string("jina hra: ") + popisDuvodu(v.rb->duvod()));
    Vysledek v2;
    hraj(v2, 10, 0, 60, ahoj("Rene", J_SEGA, 0x42, 7), ahoj("bracha", J_SEGA, 0x42, 8));
    over(v2.ra->duvod() == D_JINA_HRA && v2.rb->duvod() == D_JINA_HRA, "jine nastaveni jadra (region ...) -> jina hra");
    Vysledek v3;
    hraj(v3, 10, 0, 60, ahoj("Rene"), ahoj("bracha", J_SEGA, 0x42, 7, VERZE + 1));
    over(v3.ra->duvod() == D_VERZE && v3.rb->duvod() == D_VERZE, std::string("jina verze: ") + popisDuvodu(v3.ra->duvod()));
    over(v.ra->jmenoDruheho() == "bracha" && v.rb->jmenoDruheho() == "Rene", "jmena hracu dorazila (Rene / bracha)");
  }

  std::printf("== 8) druhy telefon zmizi (zadna zprava) -> ticho -> konec, emulator nevisi\n");
  {
    Vysledek v;
    hraj(v, 10, 0, 600, ahoj("Rene"), ahoj("bracha"), -1, true, 1500000ULL);
    over(v.ra->konec() && v.ra->duvod() == D_TICHO && v.a.snimku < 600, std::string("a: ") + popisDuvodu(v.ra->duvod()) + " po " + std::to_string(v.a.snimku) + " snimcich");
  }

  std::printf("== 9) druhy hrac ukonci hru -> prvni se to dozvi\n");
  {
    Vysledek v;
    hraj(v, 10, 0, 600, ahoj("Rene"), ahoj("bracha"), -1, false, 15000000ULL, 180);
    over(v.ra->konec() && v.ra->duvod() == D_HRAC && v.rb->duvod() == D_HRAC && v.rb->duvodOdeMne(), std::string("a: ") + popisDuvodu(v.ra->duvod()) + " po " + std::to_string(v.a.snimku) + " snimcich");
  }

  std::printf("== 10) poskozena zprava -> konec s chybou (ne pad)\n");
  {
    Relace r(true, ahoj("Rene"), 16.7);
    r.zacni(tedUs());
    const uint8_t spatna[] = {VSTUP, 1, 0};
    over(!r.prijmi(spatna, sizeof(spatna)) && r.duvod() == D_CHYBA, "kratka VSTUP -> D_CHYBA");
    Relace r2(false, ahoj("bracha"), 16.7);
    r2.zacni(tedUs());
    const uint8_t neznama[] = {99, 1, 2, 3};
    over(r2.prijmi(neznama, sizeof(neznama)) && !r2.konec(), "neznamy typ zpravy (novejsi verze) -> ignorovat");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_chyb);
  return g_chyb == 0 ? 0 : 1;
}
