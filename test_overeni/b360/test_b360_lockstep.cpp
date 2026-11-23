// test_overeni/b360/test_b360_lockstep.cpp
// B360: ONLINE PO PRVNIM SKUTECNEM TESTU (Rene 23. 11.: "mame chyby asi v prenosu - telefony se sekaji, posilam dva
// logy, kazdy jina chyba"). Pokus 1 skoncil po 13 s "hra se rozjela", pokus 2 po 2,5 min: druhy telefon 15 s mlcel
// a v logu bylo jen "online skoncilo". Tady (app/src/main/cpp/online/nap_online.h, bez Androidu a bez site):
//   1) text konce: z relace stejny jako z vlakna site (zadne "online skoncilo"), i kdyz konec pozna vlakno emulace
//   2) bez poruchy: kazdy obraz stejny na obou, vstupy stejne, zadna synchronizace (LAN / internet / mobil)
//   3) rozjeti -> SYNCHRONIZACE: hrac 1 posle cely stav, hrac 2 ho pred snimkem G nahraje, od G zase stejne,
//      hra pokracuje; casti otisku rikaji, co se lisilo
//   4) dve rozjeti v ruznou dobu -> dve synchronizace, hra bezi; trvale rozjeti -> konec "synchronizace nepomohla"
//   5) sit se spickami: kdo ceka na stisky druheho, pozada o vetsi zpozdeni, druhy si ho zvysi -> cekani zmizi;
//      po klidu zpozdeni zase klesne; zadny snimek bez vstupu (oba stejne obrazy i pri zmenach zpozdeni)
//   6) pojistky: RESYNC / STAV se ztrati, poskozeny stav -> konec s duvodem (nikdo nestoji navzdy)
//   g++ -std=c++17 -O2 -pthread -Wall -Wextra -I../../app/src/main/cpp -o test_b360_lockstep test_b360_lockstep.cpp
//   ./test_b360_lockstep                                    -> VYSTUP_test_b360_lockstep.txt
#include "online/nap_online.h"

#include <atomic>
#include <cstdio>
#include <deque>
#include <functional>
#include <memory>
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
  int spickaKazdychMs = 0, spickaMs = 0, spickyDoMs = 0;     // B360: obcas jedna zprava o spickaMs pozdeji (Wi-Fi spi)
  uint64_t dalsiSpicka = 0, konecSpicek = 0;
  uint8_t zahodTyp = 0, poskodTyp = 0;                       // B360: ztracena / poskozena zprava daneho typu
  std::mt19937 rng{7};
  std::atomic<bool> zamrzla{false};
  void posli(const std::vector<uint8_t> &z0) {
    std::lock_guard<std::mutex> l(m);
    if (zahodTyp && !z0.empty() && z0[0] == zahodTyp) return;
    std::vector<uint8_t> z = z0;
    if (poskodTyp && !z.empty() && z[0] == poskodTyp && z.size() > 20) z[z.size() / 2] ^= 0x5A;
    const uint64_t ted = tedUs();
    uint64_t t = ted + (uint64_t)zpozdeniMs * 1000 + (kolisaniMs ? (uint64_t)(rng() % (kolisaniMs * 1000 + 1)) : 0);
    if (spickaKazdychMs) {
      if (!dalsiSpicka) { dalsiSpicka = ted + (uint64_t)spickaKazdychMs * 1000; konecSpicek = spickyDoMs ? ted + (uint64_t)spickyDoMs * 1000 : 0; }
      if (ted >= dalsiSpicka && (!konecSpicek || ted < konecSpicek)) { t += (uint64_t)spickaMs * 1000; dalsiSpicka = ted + (uint64_t)spickaKazdychMs * 1000; }
    }
    if (t < posledniDoruceni) t = posledniDoruceni;          // TCP: poradi zachovane
    posledniDoruceni = t;
    q.push_back({t, std::move(z)});
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

// --------------------------------------------------------------------------- deterministicky "emulator" se stavem
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
  // casti otisku jako u jadra: 0 = ram 0..2047, 1 = ram 2048..4095, 2 = vnitrni stav
  uint64_t casti(uint64_t c[3]) const {
    c[0] = otisk(ram, 2048); c[1] = otisk(ram + 2048, 2048); c[2] = otisk(&s, sizeof(s));
    return otisk(c, 24);
  }
  std::vector<uint8_t> zabal() const {   // stav + kontrolni soucet (jako zlib u jadra - poskozeni se pozna)
    std::vector<uint8_t> v(8 + sizeof(ram) + 8);
    std::memcpy(v.data(), &s, 8);
    std::memcpy(v.data() + 8, ram, sizeof(ram));
    const uint64_t k = otisk(v.data(), 8 + sizeof(ram));
    std::memcpy(v.data() + 8 + sizeof(ram), &k, 8);
    return v;
  }
  bool nahraj(const std::vector<uint8_t> &v) {
    if (v.size() != 8 + sizeof(ram) + 8) return false;
    uint64_t k;
    std::memcpy(&k, v.data() + 8 + sizeof(ram), 8);
    if (k != otisk(v.data(), 8 + sizeof(ram))) return false;
    std::memcpy(&s, v.data(), 8);
    std::memcpy(ram, v.data() + 8, sizeof(ram));
    return true;
  }
};

static uint32_t skript(int hrac, uint32_t f) { return hrac == 1 ? ((f / 7) * 2654435761u) & 0xFFF : (((f / 11) * 40503u) ^ 0x5A5) & 0xFFF; }

struct Telefon {
  int hrac;
  Relace *r = nullptr;
  Linka *ven = nullptr, *dovnitr = nullptr;
  std::atomic<bool> konecSite{false};
  std::atomic<bool> siteZamrzla{false};
  std::vector<uint64_t> obrazy;
  std::vector<std::pair<uint32_t, uint32_t>> vstupy;
  std::atomic<uint32_t> snimku{0};
  std::vector<int> poruchy;          // v techto snimcich se emulator "splete" (ram[5] ^= 1)
  int trvalaOd = -1;                 // od tohoto snimku se splete v kazdem snimku
  uint64_t tichoUs = 15000000ULL;
  std::vector<uint32_t> synchronizovanoVe;   // snimky, pred kterymi se nahral / poslal stav
  std::atomic<bool> konecTextHned{false};
  std::string textVlaknaEmulace;     // text konce, jak ho videlo vlakno emulace hned po vstupy() = -1
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
      if (r->konec()) {
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
      if (hrac == 1) {
        if (r->chciPoslatStav(f)) { r->posliStav(f, st.zabal()); synchronizovanoVe.push_back(f); }
      } else {
        std::vector<uint8_t> d;
        int k;
        while ((k = r->stavPro(f, d, 50)) < 0) {}
        if (k > 0) {
          if (st.nahraj(d)) { r->stavNahran(f); synchronizovanoVe.push_back(f); }
          else r->ukonci(D_ROZJETO);
        }
      }
      uint32_t p1 = 0, p2 = 0;
      int v;
      while ((v = r->vstupy(f, p1, p2, 50)) == 0) {}
      if (v < 0) { textVlaknaEmulace = r->textKonce(); break; }
      for (int x : poruchy) if ((int)f == x) st.ram[5] ^= 1;
      if (trvalaOd >= 0 && (int)f >= trvalaOd) st.ram[(f * 13) % 2048] ^= 1;
      obrazy.push_back(st.krok(p1, p2));
      vstupy.push_back({p1, p2});
      uint64_t c[3];
      const uint64_t h = st.casti(c);
      r->otiskStavu(f, h, c, 3);
      snimku = f + 1;
      dalsi += std::chrono::microseconds((long long)(snimekMs * 1000));
      if (dalsi > std::chrono::steady_clock::now()) std::this_thread::sleep_until(dalsi);
      else dalsi = std::chrono::steady_clock::now();
    }
  }
};

struct Hra {
  Telefon a, b;
  std::unique_ptr<Relace> ra, rb;
  Linka ab, ba;
  Ladeni ladeni;
  std::function<void(Hra &)> priHre;   // pozorovatel (bezi soubezne s hrou)
};

static void hraj(Hra &v, int zpozdeniMs, int kolisaniMs, uint32_t snimku, bool cekatNaKonec = false) {
  const double snimekMs = 1000.0 / 60.0;
  v.ab.zpozdeniMs = v.ba.zpozdeniMs = zpozdeniMs;
  v.ab.kolisaniMs = v.ba.kolisaniMs = kolisaniMs;
  Ahoj ja, on;
  ja.jadro = on.jadro = J_SEGA; ja.nastaveni = on.nastaveni = 7; ja.jmeno = "Rene"; on.jmeno = "tester";
  for (int i = 0; i < 20; i++) ja.hra[i] = on.hra[i] = (uint8_t)(0x42 + i);
  v.ra.reset(new Relace(true, ja, snimekMs, v.ladeni));
  v.rb.reset(new Relace(false, on, snimekMs, v.ladeni));
  v.a.hrac = 1; v.a.r = v.ra.get(); v.a.ven = &v.ab; v.a.dovnitr = &v.ba;
  v.b.hrac = 2; v.b.r = v.rb.get(); v.b.ven = &v.ba; v.b.dovnitr = &v.ab;
  std::thread sa([&] { v.a.sit(); }), sb([&] { v.b.sit(); });
  std::thread ea([&] { v.a.emulace(snimku, snimekMs); }), eb([&] { v.b.emulace(snimku, snimekMs); });
  std::thread poz;
  if (v.priHre) poz = std::thread([&] { v.priHre(v); });
  ea.join(); eb.join();
  if (poz.joinable()) poz.join();
  const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(4);
  while (cekatNaKonec && (!v.ra->konec() || !v.rb->konec()) && std::chrono::steady_clock::now() < limit)
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  v.a.konecSite = v.b.konecSite = true;
  sa.join(); sb.join();
}

static size_t prvniRozdil(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, size_t od = 0) {
  size_t i = od;
  while (i < std::min(x.size(), y.size()) && x[i] == y[i]) i++;
  return i;
}
static bool stejneOd(const std::vector<uint64_t> &x, const std::vector<uint64_t> &y, size_t od) {
  if (x.size() != y.size()) return false;
  for (size_t i = od; i < x.size(); i++) if (x[i] != y[i]) return false;
  return true;
}
static std::string cs(uint32_t n) { return std::to_string(n); }

int main() {
  std::printf("B360 - online: text konce, synchronizace pri rozjeti, zpozdeni kazdeho telefonu za hry (nap_online.h)\n");

  std::printf("== 1) text konce - stejny z relace (vlakno emulace) i z vlakna site\n");
  {
    over(textKonce(D_HRAC, true) == "online hru jsi ukoncil" && textKonce(D_HRAC, false) == "druhy hrac hru ukoncil",
         "D_HRAC: sam = 'online hru jsi ukoncil', druhy = 'druhy hrac hru ukoncil'");
    over(textKonce(D_TICHO, false).rfind("od druheho hrace dlouho nic neprislo", 0) == 0 && textKonce(D_ROZJETO, true).rfind("hra se rozjela", 0) == 0,
         "D_TICHO: '" + textKonce(D_TICHO, false) + "' (stranka ho pozna podle zacatku jako dosud)");
    Relace r(true, Ahoj(), 16.7);
    r.zacni(tedUs());
    over(r.textKonce().empty(), "dokud hra bezi, relace zadny text konce nema");
    const uint8_t konec[] = {KONEC, D_TICHO};
    r.prijmi(konec, sizeof(konec));
    over(r.textKonce() == textKonce(D_TICHO, false), "po konci ho ma relace hned (vlakno emulace uz nemusi cekat na vlakno site)");
  }

  struct Sc { const char *co; int ms, kol; };
  const Sc sc[] = {{"LAN 5 ms", 5, 2}, {"internet 40 ms +-15 ms", 40, 15}, {"mobilni sit 90 ms +-40 ms", 90, 40}};
  int i = 2;
  for (const Sc &s : sc) {
    std::printf("== %d) bez poruchy: %s, 420 snimku\n", i++, s.co);
    Hra v;
    hraj(v, s.ms, s.kol, 420);
    over(v.a.obrazy.size() == 420 && v.a.obrazy == v.b.obrazy && v.a.vstupy == v.b.vstupy,
         "420 snimku, KAZDY obraz i vstupy obou hracu stejne (prvni rozdil " + std::to_string(prvniRozdil(v.a.obrazy, v.b.obrazy)) + ")");
    const uint32_t D = v.ra->zpozdeni();
    const bool menilo = v.rb->zvyseni() || v.ra->zvyseni() || v.rb->snizeni() || v.ra->snizeni();
    bool mista = v.a.vstupy.size() == 420;
    for (uint32_t f = D; f < 420 && mista && !menilo; f++)
      mista = v.a.vstupy[f].first == skript(1, f - D) && v.a.vstupy[f].second == skript(2, f - D);
    over(mista, menilo ? "zpozdeni se za hry menilo (sit kolisa) - vstupy obou hracu na obou telefonech stejne (viz vyse)"
                       : "hrac 1 = 1. ovladac, hrac 2 = 2. ovladac, vstup plati o D = " + cs(D) + " snimku pozdeji");
    over(v.ra->synchronizaci() == 0 && v.rb->synchronizaci() == 0 && v.ra->overenychOtisku() >= 6 && !v.ra->konec() && !v.rb->konec(),
         "zadna synchronizace, overeno otisku " + cs(v.ra->overenychOtisku()) + " / " + cs(v.rb->overenychOtisku()) + ", cekalo se a " +
             cs(v.ra->cekani()) + "x (" + cs(v.ra->cekaniMs()) + " ms), b " + cs(v.rb->cekani()) + "x (" + cs(v.rb->cekaniMs()) + " ms), zpozdeni a " +
             cs(v.ra->zpozdeniTed()) + " b " + cs(v.rb->zpozdeniTed()));
  }

  std::printf("== 5) rozjeti (emulator hrace 2 se ve snimku 200 splete) -> synchronizace, hra pokracuje do 600\n");
  {
    Hra v;
    v.b.poruchy = {200};
    hraj(v, 20, 5, 600);
    const uint32_t G = v.b.synchronizovanoVe.empty() ? 0 : v.b.synchronizovanoVe[0];
    over(v.ra->synchronizaci() == 1 && v.rb->synchronizaci() == 1 && v.ra->synchronizovano() == 1 && v.rb->synchronizovano() == 1 &&
             v.a.synchronizovanoVe.size() == 1 && v.a.synchronizovanoVe == v.b.synchronizovanoVe,
         "1 synchronizace: hrac 1 poslal stav pred snimkem " + (v.a.synchronizovanoVe.empty() ? std::string("-") : cs(v.a.synchronizovanoVe[0])) +
             ", hrac 2 ho pred tymz snimkem nahral (" + cs(G) + "), velikost " + cs(v.ra->stavKB()) + " KB");
    over(v.ra->rozjetoVeSnimku() == 240 && G > 240 && G < 260 && prvniRozdil(v.a.obrazy, v.b.obrazy) == 200 && stejneOd(v.a.obrazy, v.b.obrazy, G),
         "rozdil od snimku " + std::to_string(prvniRozdil(v.a.obrazy, v.b.obrazy)) + ", poznano v otisku " + cs(v.ra->rozjetoVeSnimku()) +
             ", od snimku G = " + cs(G) + " zase KAZDY obraz stejny");
    over(v.a.obrazy.size() == 600 && v.b.obrazy.size() == 600 && !v.ra->konec() && !v.rb->konec() && v.a.vstupy == v.b.vstupy,
         "hra nekoncila: oba 600 snimku, vstupy obou hracu stejne po celou dobu");
    over(v.ra->rozdilCasti() == 1u, "casti otisku: lisila se jen cast 0 (bit " + cs(v.ra->rozdilCasti()) + ") - porucha byla v ram[5]");
    over(v.ra->overenychOtisku() >= 6 && v.rb->overenychOtisku() >= 6, "po synchronizaci overeno otisku " + cs(v.ra->overenychOtisku()) + " / " + cs(v.rb->overenychOtisku()));
  }

  std::printf("== 6) dve rozjeti v ruznou dobu -> dve synchronizace; trvale rozjeti -> konec s duvodem\n");
  {
    Hra v;
    v.ladeni.synchronizaceOknoUs = 3000000;   // 3 s (na telefonu 10 s)
    v.b.poruchy = {200, 560};
    hraj(v, 15, 5, 720);
    over(v.ra->synchronizaci() == 2 && v.rb->synchronizovano() == 2 && stejneOd(v.a.obrazy, v.b.obrazy, v.b.synchronizovanoVe.size() == 2 ? v.b.synchronizovanoVe[1] : 0) &&
             !v.ra->konec() && v.a.obrazy.size() == 720,
         "2 synchronizace (pred snimky " + (v.b.synchronizovanoVe.size() == 2 ? cs(v.b.synchronizovanoVe[0]) + " a " + cs(v.b.synchronizovanoVe[1]) : std::string("?")) +
             "), hra dobehla do 720, na konci stejne");
    Hra w;
    w.ladeni.synchronizaceOknoUs = 5000000;
    w.b.trvalaOd = 200;
    hraj(w, 15, 5, 900, true);
    over(w.ra->konec() && w.rb->konec() && w.ra->duvod() == D_ROZJETO && w.rb->duvod() == D_ROZJETO && w.ra->synchronizaci() == 2,
         "emulator hrace 2 se plete porad -> po " + cs(w.ra->synchronizaci()) + " synchronizacich konec u obou: '" + w.ra->textKonce() + "' / '" + w.rb->textKonce() + "'");
  }

  std::printf("== 7) sit se spickami (Wi-Fi telefonu obcas spi): zpozdeni hrace 2 se samo zvysi, cekani zmizi\n");
  {
    Hra v;
    v.ladeni.zadostKazdychUs = 700000;    // na telefonu 2 s / 1 s / 30 s
    v.ladeni.zvyseniKazdychUs = 300000;
    v.ladeni.snizitPoUs = 3000000;
    // stisky hrace 2 k hraci 1 (linka ba): kazdych 500 ms jedna zprava o 120 ms pozdeji (TCP - ostatni za ni), prvnich 6 s
    v.ba.spickaKazdychMs = 500; v.ba.spickaMs = 120; v.ba.spickyDoMs = 6000;
    uint32_t dlouhaPo3s = 0, dlouhaPo6s = 0, maxD = 0;
    v.priHre = [&](Hra &h) {
      const auto t0 = std::chrono::steady_clock::now();
      while (!h.ra->bezi() && std::chrono::steady_clock::now() - t0 < std::chrono::seconds(5)) std::this_thread::sleep_for(std::chrono::milliseconds(5));
      const auto t1 = std::chrono::steady_clock::now();
      while (std::chrono::steady_clock::now() - t1 < std::chrono::seconds(16)) {
        const auto t = std::chrono::steady_clock::now() - t1;
        maxD = std::max(maxD, h.rb->zpozdeniTed());
        if (t < std::chrono::seconds(3)) dlouhaPo3s = h.ra->dlouhychCekani();
        if (t < std::chrono::seconds(6)) dlouhaPo6s = h.ra->dlouhychCekani();
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        if (h.ra->konec()) break;
      }
    };
    hraj(v, 8, 2, 900);
    const uint32_t D0 = v.rb->zpozdeni();
    over(v.ra->zadosti() >= 1 && v.rb->zvyseni() >= 1 && maxD > D0,
         "hrac 1 cekal na stisky hrace 2 (dlouhych cekani " + cs(v.ra->dlouhychCekani()) + ", nejdel " + cs(v.ra->cekaniMaxMs()) + " ms) -> pozadal " +
             cs(v.ra->zadosti()) + "x, hrac 2 zvysil zpozdeni " + cs(v.rb->zvyseni()) + "x: " + cs(D0) + " -> nejvys " + cs(maxD) + " snimku");
    over(dlouhaPo6s - dlouhaPo3s < dlouhaPo3s || dlouhaPo3s == 0,
         "dlouhych cekani v prvnich 3 s " + cs(dlouhaPo3s) + ", v dalsich 3 s (spicky porad) " + cs(dlouhaPo6s - dlouhaPo3s) + " - vetsi zpozdeni je spolklo");
    over(v.rb->snizeni() >= 1 && v.rb->zpozdeniTed() < maxD,
         "po spickach klid -> zpozdeni hrace 2 zase dolu (" + cs(v.rb->snizeni()) + "x, ted " + cs(v.rb->zpozdeniTed()) + ")");
    over(v.a.obrazy.size() == 900 && v.a.obrazy == v.b.obrazy && v.a.vstupy == v.b.vstupy && v.ra->synchronizaci() == 0 && !v.ra->konec(),
         "pri vsech zmenach zpozdeni zadny snimek bez vstupu: 900 snimku, KAZDY obraz a vstup stejny, bez synchronizace");
  }

  std::printf("== 8) pojistky: synchronizace se nepovede -> konec s duvodem, nikdo nestoji navzdy\n");
  {
    Hra v;
    v.ladeni.cekatNaSynchronizaciUs = 1500000;
    v.b.poruchy = {200};
    v.ab.zahodTyp = RESYNC;   // RESYNC od hrace 1 se ztrati
    hraj(v, 10, 0, 900, true);
    over(v.rb->konec() && v.rb->duvod() == D_ROZJETO && v.ra->konec() && v.ra->duvod() == D_ROZJETO,
         "RESYNC se ztratil -> hrac 2 ceka 1,5 s, pak konec u obou: '" + v.rb->textKonce() + "'");
    Hra w;
    w.ladeni.cekatNaStavUs = 1500000;
    w.b.poruchy = {200};
    w.ab.zahodTyp = STAV;     // stav od hrace 1 se ztrati
    hraj(w, 10, 0, 900, true);
    over(w.rb->konec() && w.rb->duvod() == D_ROZJETO && w.ra->konec() && w.b.snimku.load() < 300,
         "STAV se ztratil -> hrac 2 stoji pred G nejvys 1,5 s, pak konec u obou (hrac 2 snimek " + cs(w.b.snimku.load()) + ")");
    Hra x;
    x.b.poruchy = {200};
    x.ab.poskodTyp = STAV;    // stav prijde poskozeny (jadro ho nenahraje)
    hraj(x, 10, 0, 900, true);
    over(x.rb->konec() && x.rb->duvod() == D_ROZJETO && x.ra->duvod() == D_ROZJETO && x.b.synchronizovanoVe.empty(),
         "poskozeny stav se nenahraje -> konec u obou: '" + x.ra->textKonce() + "'");
  }

  std::printf("== 9) druhy telefon zmizi -> ticho; text konce vidi hned i vlakno emulace (ne 'online skoncilo')\n");
  {
    Hra v;
    v.a.tichoUs = v.b.tichoUs = 1500000;
    v.priHre = [&](Hra &h) {
      while (h.b.snimku.load() < 150 && !h.rb->konec()) std::this_thread::sleep_for(std::chrono::milliseconds(2));
      h.b.siteZamrzla = true;   // telefon 2 prestal posilat i prijimat (Wi-Fi pryc), bez KONEC
      h.ba.zamrzla = true;
    };
    hraj(v, 10, 0, 600, false);
    over(v.ra->konec() && v.ra->duvod() == D_TICHO && v.a.textVlaknaEmulace == textKonce(D_TICHO, false),
         "hrac 1: vlakno emulace po vstupy() = -1 cte z relace: '" + v.a.textVlaknaEmulace + "'");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_chyb);
  return g_chyb == 0 ? 0 : 1;
}
