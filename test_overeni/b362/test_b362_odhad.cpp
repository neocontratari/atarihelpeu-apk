// test_overeni/b362/test_b362_odhad.cpp
// B362: ONLINE S ODHADEM STISKU DRUHEHO A OPRAVOU (Rene 24. 11.: "furt je ten online spatny Partaku. A to ho testuju
// na mistni LAN"). V logu S8: oba telefony cekaly na sebe navzajem (spicky site obema smery, odezva 7-123 ms),
// zpozdeni ovladani vyrostlo ze 4 na 11 snimku. Tady (app/src/main/cpp/online/nap_online.h, bez Androidu a bez site):
//   1) zpozdeni na startu s odhadem jen na jednosmernou cestu (LAN 2 snimky)
//   2) LAN se spickami obema smery: STEJNY profil site lockstep (B361) vs odhad (B362) - s odhadem emulator skoro
//      necekal, vysledne obrazy obou telefonu STEJNE a stejne jako prepocet ze zaznamenanych vstupu, otisky overene
//   3) odhad je omezeny: druhy mlci -> nejvys maxOdhad snimku napred, pak se ceka (a pak ticho = konec)
//   4) rozjeti + synchronizace s odhadem: hrac 2 s odhadem dosel ZA snimek G, nez prisel RESYNC -> stav G ho vrati
//      zpet, od G zase stejne; jadro, ktere stav neposle -> konec u G
//   5) STATISTIKA: kazdy telefon zna cisla druheho (cekani, odhady, opravy, vykon, info telefonu), text jen ASCII
//   g++ -std=c++17 -O2 -pthread -Wall -Wextra -I../../app/src/main/cpp -o test_b362_odhad test_b362_odhad.cpp
//   ./test_b362_odhad                                       -> VYSTUP_test_b362_odhad.txt
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
static std::string cs(uint64_t n) { return std::to_string(n); }

// --------------------------------------------------------------------------- simulovana sit (jako TCP)
struct Linka {
  std::mutex m;
  std::deque<std::pair<uint64_t, std::vector<uint8_t>>> q;   // (cas doruceni us, zprava)
  uint64_t posledniDoruceni = 0;
  int zpozdeniMs = 5, kolisaniMs = 2;
  double spickaSance = 0;                 // sance, ze zprava (a TCP vsechny za ni) prijde o spickaMs pozdeji
  int spickaOdMs = 0, spickaDoMs = 0;
  uint8_t zpozdiTyp = 0; int zpozdiTypMs = 0;   // tenhle typ zpravy (a vsechny za nim) o tolik pozdeji - jednou
  std::mt19937 rng{11};
  std::atomic<bool> zamrzla{false};
  std::atomic<uint32_t> spicek{0};
  void posli(const std::vector<uint8_t> &z) {
    std::lock_guard<std::mutex> l(m);
    const uint64_t ted = tedUs();
    uint64_t t = ted + (uint64_t)zpozdeniMs * 1000 + (kolisaniMs ? (uint64_t)(rng() % (kolisaniMs * 1000 + 1)) : 0);
    if (spickaSance > 0 && (rng() % 100000) < (uint32_t)(spickaSance * 100000)) {
      t += (uint64_t)(spickaOdMs + (int)(rng() % (uint32_t)(spickaDoMs - spickaOdMs + 1))) * 1000;
      spicek++;
    }
    if (zpozdiTyp && !z.empty() && z[0] == zpozdiTyp) { t += (uint64_t)zpozdiTypMs * 1000; zpozdiTyp = 0; }
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
  uint64_t casti(uint64_t c[3]) const {
    c[0] = otisk(ram, 2048); c[1] = otisk(ram + 2048, 2048); c[2] = otisk(&s, sizeof(s));
    return otisk(c, 24);
  }
  std::vector<uint8_t> zabal() const {
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

// stisky jako u hrace: drzi tlacitko desitky snimku (odhad "drzi, co drzel" casto sedi, ale ne vzdy)
static uint32_t skript(int hrac, uint32_t f) {
  return hrac == 1 ? (((f / 23) * 2654435761u) >> 7) & 0xFF : ((((f + 9) / 31) * 40503u) >> 3) & 0xFF;
}

struct Telefon {
  int hrac;
  Relace *r = nullptr;
  Linka *ven = nullptr, *dovnitr = nullptr;
  std::atomic<bool> konecSite{false};
  std::vector<uint64_t> obrazy;
  std::vector<std::pair<uint32_t, uint32_t>> vstupy;
  std::atomic<uint32_t> snimku{0};
  std::vector<int> poruchy;
  bool neposilejStav = false;            // jadro "synchronizaci neumi" (hrac 1)
  uint32_t maxOdhad = ODHAD_MAX;         // 0 = lockstep (B361)
  uint64_t tichoUs = 15000000ULL;
  std::vector<uint32_t> synchronizovanoVe;
  uint32_t vracenoZe = 0, vracenoNa = 0; // hrac 2: s odhadem dosel za G, stav ho vratil
  uint32_t nejdalOdhad = 0;              // nejvic snimku napred s odhadem (snimek - jehoDo + 1)
  uint32_t chybUlozeni = 0;
  std::string textVlaknaEmulace;
  // kruh stavu pred snimky spocitanymi s odhadem (jako ClownMDEmu_StateBackup v jadre Segy)
  struct Z { uint32_t snimek = UINT32_MAX; std::vector<uint8_t> v; };
  Z kruh[ODHAD_MAX + 2];
  void uloz(uint32_t f, const Stroj &st) { Z &z = kruh[f % (ODHAD_MAX + 2)]; z.snimek = f; z.v = st.zabal(); }
  bool vrat(uint32_t f, Stroj &st) { Z &z = kruh[f % (ODHAD_MAX + 2)]; return z.snimek == f && st.nahraj(z.v); }

  void sit() {
    r->zacni(tedUs());
    while (!konecSite) {
      r->cekejNaOdchozi(2);
      for (auto &z : r->vyzvedni()) ven->posli(z);
      std::vector<uint8_t> z;
      while (dovnitr->vezmi(z)) r->prijmi(z.data(), z.size(), tedUs());
      r->tik(tedUs(), tichoUs);
      if (r->konec()) { for (auto &z2 : r->vyzvedni()) ven->posli(z2); break; }
    }
  }
  void spocitej(Stroj &st, uint32_t f, uint32_t p1, uint32_t p2) {
    for (int x : poruchy) if ((int)f == x) st.ram[5] ^= 1;
    if (obrazy.size() <= f) { obrazy.resize(f + 1); vstupy.resize(f + 1); }
    obrazy[f] = st.krok(p1, p2);
    vstupy[f] = {p1, p2};
    uint64_t c[3];
    const uint64_t h = st.casti(c);
    r->otiskStavu(f, h, c, 3);
  }
  /** Oprava: stav pred 'od' zpet, snimky od .. ted-1 znovu (jako nap_sega_online_oprav). */
  void oprav(Stroj &st, uint32_t od, uint32_t ted) {
    if (!vrat(od, st)) { chybUlozeni++; return; }
    const uint64_t t0 = tedUs();
    for (uint32_t g = od; g < ted; g++) {
      uint32_t p1 = 0, p2 = 0; bool odhad = false;
      if (r->vstupyOdhad(g, p1, p2, odhad, 0, ODHAD_OPRAVA) != 1) break;
      if (odhad) uloz(g, st);
      spocitej(st, g, p1, p2);
    }
    r->opraveno(od, ted, tedUs() - t0);
  }
  void emulace(uint32_t celkem, double snimekMs) {
    Stroj st;
    while (!r->bezi() && !r->konec()) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    auto dalsi = std::chrono::steady_clock::now();
    uint32_t f = 0;
    while (f < celkem) {
      r->lokalni(f, skript(hrac, f));
      const uint32_t od = r->zkontrolujOdhady(f);
      if (od != UINT32_MAX) oprav(st, od, f);
      if (hrac == 1) {
        if (!neposilejStav && r->chciPoslatStav(f)) { r->posliStav(f, st.zabal()); synchronizovanoVe.push_back(f); }
      } else {
        std::vector<uint8_t> d;
        uint32_t g = 0;
        const int k = r->stavPro(f, d, g, 50);
        if (k < 0) continue;
        if (k > 0) {
          if (st.nahraj(d)) {
            if (f > g) { vracenoZe = f; vracenoNa = g; }
            f = g;
            for (Z &z : kruh) z.snimek = UINT32_MAX;
            r->stavNahran(g);
            synchronizovanoVe.push_back(g);
          } else r->ukonci(D_ROZJETO);
        }
      }
      uint32_t p1 = 0, p2 = 0;
      bool odhad = false;
      const int v = r->vstupyOdhad(f, p1, p2, odhad, 50, maxOdhad);
      if (v == 0) continue;
      if (v < 0) { textVlaknaEmulace = r->textKonce(); break; }
      if (odhad) {
        uloz(f, st);
        const uint32_t napred = f - r->jehoDo() + 1;
        if (napred > nejdalOdhad && f >= r->jehoDo()) nejdalOdhad = napred;
      }
      spocitej(st, f, p1, p2);
      f++;
      snimku = f;
      dalsi += std::chrono::microseconds((long long)(snimekMs * 1000));
      if (dalsi > std::chrono::steady_clock::now()) std::this_thread::sleep_until(dalsi);
      else dalsi = std::chrono::steady_clock::now();
    }
    // dohrat: posledni odhady overit (stisky druheho pro vsechny snimky uz jsou poslane)
    const auto limit = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!r->konec() && r->jistoDo() < celkem && std::chrono::steady_clock::now() < limit) {
      const uint32_t od = r->zkontrolujOdhady(celkem);
      if (od != UINT32_MAX) oprav(st, od, celkem);
      else std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
  }
};

struct Hra {
  Telefon a, b;
  std::unique_ptr<Relace> ra, rb;
  Linka ab, ba;
  Ladeni ladeni;
  std::function<void(Hra &)> priHre;
};

static void hraj(Hra &v, uint32_t snimku, bool cekatNaKonec = false) {
  const double snimekMs = 1000.0 / 60.0;
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

/** Prepocet od nuly ze zaznamenanych vstupu (bez site, bez odhadu) - "pravda" pro porovnani. */
static std::vector<uint64_t> prepocet(const std::vector<std::pair<uint32_t, uint32_t>> &vs, const std::vector<int> &poruchy = {}) {
  Stroj st;
  std::vector<uint64_t> o;
  for (uint32_t f = 0; f < vs.size(); f++) {
    for (int x : poruchy) if ((int)f == x) st.ram[5] ^= 1;
    o.push_back(st.krok(vs[f].first, vs[f].second));
  }
  return o;
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

static void spickyObemaSmery(Hra &v) {
  // LAN jako u Reneho: 5 ms +-2 ms a obcas spicka 60-200 ms (Wi-Fi / TCP opakovani), obema smery
  for (Linka *l : {&v.ab, &v.ba}) { l->zpozdeniMs = 5; l->kolisaniMs = 2; l->spickaSance = 0.012; l->spickaOdMs = 60; l->spickaDoMs = 200; }
  v.ba.rng.seed(23);
}

int main() {
  std::printf("B362 - online s odhadem stisku druheho a opravou (nap_online.h verze %u)\n", (unsigned)VERZE);

  std::printf("== 1) zpozdeni na startu: s odhadem jen jednosmerna cesta (kolisani spolkne odhad)\n");
  {
    const double s = 1000.0 / 60.0;
    over(zpozdeniSOdhadem(9, s) == 2 && zpozdeniSOdhadem(40, s) == 3 && zpozdeniSOdhadem(90, s) == 4 && zpozdeniSOdhadem(400, s) == 14,
         "odezva 9 ms -> " + cs(zpozdeniSOdhadem(9, s)) + " snimky (LAN), 40 ms -> " + cs(zpozdeniSOdhadem(40, s)) + ", 90 ms -> " +
             cs(zpozdeniSOdhadem(90, s)) + ", 400 ms -> " + cs(zpozdeniSOdhadem(400, s)) + " (B361 pro 9 ms +-30 ms: " +
             cs(zpozdeniZOdezvy(9, 30, s)) + ")");
  }

  std::printf("== 2) LAN se spickami obema smery (jako log S8 z 24. 11.): lockstep (B361) proti odhadu (B362), 1500 snimku\n");
  uint32_t lockCekaniMs = 0, lockCekani = 0;
  {
    Hra v;
    spickyObemaSmery(v);
    v.a.maxOdhad = v.b.maxOdhad = 0;     // B361: bez odhadu
    hraj(v, 1500);
    lockCekaniMs = v.ra->cekaniMs() + v.rb->cekaniMs();
    lockCekani = v.ra->cekani() + v.rb->cekani();
    over(v.a.obrazy == v.b.obrazy && v.a.obrazy.size() == 1500,
         "lockstep (B361): obrazy stejne, ale emulatory staly " + cs(lockCekani) + "x, dohromady " + cs(lockCekaniMs) + " ms (nejdel " +
             cs(std::max(v.ra->cekaniMaxMs(), v.rb->cekaniMaxMs())) + " ms), spicek " + cs(v.ab.spicek + v.ba.spicek) + ", zpozdeni a " +
             cs(v.ra->zpozdeniTed()) + " b " + cs(v.rb->zpozdeniTed()));
  }
  {
    Hra v;
    spickyObemaSmery(v);
    v.ladeni.odhad = true;
    v.a.vstupy.reserve(1500); v.b.vstupy.reserve(1500);
    hraj(v, 1500);
    const uint32_t cekMs = v.ra->cekaniMs() + v.rb->cekaniMs();
    over(v.a.obrazy.size() == 1500 && v.a.obrazy == v.b.obrazy && v.a.vstupy == v.b.vstupy,
         "s odhadem: 1500 snimku, po opravach KAZDY obraz i vstupy obou hracu stejne (prvni rozdil " + cs(prvniRozdil(v.a.obrazy, v.b.obrazy)) + ")");
    over(prepocet(v.a.vstupy) == v.a.obrazy,
         "obrazy po opravach = prepocet od nuly ze skutecnych vstupu (oprava nic nerozbila)");
    over(v.ra->oprav() + v.rb->oprav() > 0 && v.ra->odhadnuto() + v.rb->odhadnuto() > 0,
         "odhadnuto snimku a " + cs(v.ra->odhadnuto()) + " (sedelo " + cs(v.ra->odhaduSedelo()) + ") b " + cs(v.rb->odhadnuto()) + " (sedelo " +
             cs(v.rb->odhaduSedelo()) + "), oprav a " + cs(v.ra->oprav()) + " b " + cs(v.rb->oprav()) + " (nejdel " +
             cs(std::max(v.ra->opravaMax(), v.rb->opravaMax())) + " snimku, prepocteno " + cs(v.ra->prepocteno() + v.rb->prepocteno()) + ")");
    over(cekMs * 4 < lockCekaniMs || (lockCekaniMs > 0 && cekMs == 0),
         "emulatory s odhadem staly " + cs(v.ra->cekani() + v.rb->cekani()) + "x, dohromady " + cs(cekMs) + " ms (lockstep " + cs(lockCekaniMs) +
             " ms) - spicky do " + cs(ODHAD_MAX) + " snimku spolkne odhad");
    over(v.ra->synchronizaci() == 0 && v.rb->synchronizaci() == 0 && v.ra->overenychOtisku() >= 20 && !v.ra->konec() && !v.rb->konec() &&
             v.a.chybUlozeni == 0 && v.b.chybUlozeni == 0,
         "otisky jen ze snimku se skutecnymi stisky: overeno " + cs(v.ra->overenychOtisku()) + " / " + cs(v.rb->overenychOtisku()) +
             ", zadne rozjeti, zadna synchronizace, oprava vzdy nasla ulozeny stav");
    over(v.ra->zpozdeni() == 2 && v.ra->zpozdeniTed() <= 4 && v.rb->zpozdeniTed() <= 4,
         "zpozdeni ovladani zustalo male: start " + cs(v.ra->zpozdeni()) + ", na konci a " + cs(v.ra->zpozdeniTed()) + " b " + cs(v.rb->zpozdeniTed()) +
             " snimku (B361 v logu S8: 4 -> 11)");
  }

  std::printf("== 3) odhad je omezeny: druhy telefon mlci -> nejvys %u snimku napred, pak se ceka; ticho = konec\n", (unsigned)ODHAD_MAX);
  {
    Hra v;
    v.ladeni.odhad = true;
    v.a.tichoUs = v.b.tichoUs = 1500000;
    v.priHre = [&](Hra &h) {
      while (h.b.snimku.load() < 200 && !h.rb->konec()) std::this_thread::sleep_for(std::chrono::milliseconds(2));
      h.ba.zamrzla = true;      // od telefonu 2 nic (Wi-Fi pryc)
      h.ab.zamrzla = true;
    };
    hraj(v, 900);
    over(v.a.nejdalOdhad >= 2 && v.a.nejdalOdhad <= ODHAD_MAX && v.a.snimku.load() <= v.b.snimku.load() + ODHAD_MAX + 2,
         "hrac 1 dosel s odhadem nejvys " + cs(v.a.nejdalOdhad) + " snimku za posledni stisk druheho (limit " + cs(ODHAD_MAX) + "), pak stal: snimek " +
             cs(v.a.snimku.load()) + ", druhy " + cs(v.b.snimku.load()));
    over(v.ra->konec() && v.ra->duvod() == D_TICHO && v.a.textVlaknaEmulace == textKonce(D_TICHO, false),
         "po 1,5 s ticha konec: '" + v.a.textVlaknaEmulace + "', posledni zprava od druheho: " + v.ra->posledniZprava());
  }

  std::printf("== 4) rozjeti + synchronizace s odhadem (hrac 2 dosel s odhadem ZA snimek G)\n");
  {
    Hra v;
    v.ladeni.odhad = true;
    spickyObemaSmery(v);
    v.ab.spickaSance = v.ba.spickaSance = 0.004;
    v.b.poruchy = {200};
    v.ab.zpozdiTyp = RESYNC; v.ab.zpozdiTypMs = 120;   // RESYNC (a TCP vse za nim) prijde pozde
    hraj(v, 900);
    const uint32_t G = v.b.synchronizovanoVe.empty() ? 0 : v.b.synchronizovanoVe[0];
    over(v.ra->synchronizaci() == 1 && v.rb->synchronizovano() == 1 && v.a.synchronizovanoVe == v.b.synchronizovanoVe && G > 240 && G < 280,
         "1 synchronizace: hrac 1 poslal stav pred snimkem " + (v.a.synchronizovanoVe.empty() ? std::string("-") : cs(v.a.synchronizovanoVe[0])) +
             " az po overeni snimku pred nim, hrac 2 ho nahral pred " + cs(G));
    over(v.b.vracenoZe > G && v.b.vracenoNa == G,
         "hrac 2 byl s odhadem uz na snimku " + cs(v.b.vracenoZe) + " (za G) - stav G ho vratil zpet na " + cs(v.b.vracenoNa));
    over(v.a.obrazy.size() == 900 && stejneOd(v.a.obrazy, v.b.obrazy, G) && prvniRozdil(v.a.obrazy, v.b.obrazy) == 200 && !v.ra->konec() && !v.rb->konec(),
         "rozdil od snimku 200 (porucha), od G zase KAZDY obraz stejny, hra dobehla do 900");
    over(v.ra->overenychOtisku() >= 8 && v.ra->rozdilCasti() == 1u, "po synchronizaci overeno otisku " + cs(v.ra->overenychOtisku()) + ", lisila se cast 0 (ram[5])");
  }
  {
    Hra v;
    v.ladeni.odhad = true;
    v.b.poruchy = {200};
    v.a.neposilejStav = true;                       // jadro hrace 1 synchronizaci neumi
    hraj(v, 900, true);
    over(v.ra->konec() && v.ra->duvod() == D_ROZJETO && v.rb->konec() && v.a.snimku.load() < 300,
         "jadro hrace 1 stav neposle -> konec u snimku G (" + cs(v.a.snimku.load()) + "), ne jizda dal s rozjetou hrou: '" + v.ra->textKonce() + "'");
  }

  std::printf("== 5) STATISTIKA: kazdy telefon zna cisla druheho (log jednoho telefonu ukaze oba)\n");
  {
    Hra v;
    v.ladeni.odhad = true;
    v.ladeni.statistikaPrvniUs = 1000000;
    v.ladeni.statistikaKazdychUs = 1500000;
    spickyObemaSmery(v);
    v.priHre = [&](Hra &h) {
      h.rb->infoTelefonu("SM-A125F Android 12, Wi-Fi -61 dBm 2.4 GHz, \xc5\xbe\x01");   // diakritika / ridici znak -> '?'
      h.rb->vykon("59.9 snimku/s, jadro 11.8 ms (nejvys 19.0)");
      h.ra->infoTelefonu("SM-G950F Android 9");
    };
    hraj(v, 260);
    const std::string sa = v.ra->statistikaDruheho(), sb = v.rb->statistikaDruheho();
    std::printf("       hrac 1 vi o hraci 2: %s\n       hrac 2 vi o hraci 1: %s\n", sa.c_str(), sb.c_str());
    over(v.ra->statistikDruheho() >= 2 && sa.find("odhad ") != std::string::npos && sa.find("jadro 11.8 ms") != std::string::npos &&
             sa.find("SM-A125F Android 12") != std::string::npos && sb.find("SM-G950F") != std::string::npos,
         "statistik od druheho: hrac 1 " + cs(v.ra->statistikDruheho()) + ", hrac 2 " + cs(v.rb->statistikDruheho()) + " (cekani, odhady, opravy, vykon, telefon)");
    bool ascii = true;
    for (char c : sa) if ((unsigned char)c < 32 || (unsigned char)c > 126) ascii = false;
    over(ascii && sa.find("2.4 GHz, ???") != std::string::npos, "text od druheho jen tisknutelne ASCII (diakritika a ridici znaky -> ?)");
  }

  std::printf("\nVYSLEDEK: %d OK, %d CHYB\n", g_ok, g_chyb);
  return g_chyb == 0 ? 0 : 1;
}
