// app/src/main/cpp/online/nap_online.h
// =====================================================================================================
//  ONLINE HRANI (B357) - SYNCHRONIZACE DVOU EMULATORU PRES SIT ("lockstep" se zpozdenim vstupu)
// =====================================================================================================
//  Rene, 19. 11. 2026: "chystej se, chci hrat hry online pomoci meho enginu".
//
//  Princip: oba telefony maji STEJNOU hru a stejne nastaveni a pousti ji od zapnuti (snimek 0). Emulator
//  je deterministicky - stejne vstupy ve stejnem snimku = stejny obraz. Po siti tedy nejde obraz, jen
//  STISKY: kazdy telefon posle svuj vstup pro snimek (ted + D) a snimek N emuluje, az ma vstupy obou hracu
//  pro N. D (zpozdeni vstupu, snimky) se zvoli z doby odezvy (ping) - mala prodleva ovladani misto skubani.
//  Hrac 1 = kdo hru zalozil, hrac 2 = kdo se pripojil (do jadra jako druhy ovladac, B354).
//  Kontrola: kazdou sekundu si telefony porovnaji otisk stavu emulatoru (hash RAM / VRAM). Lisi se
//  = "rozjelo se" (jina hra, jine nastaveni, chyba) - hra se zastavi s vysvetlenim, ne aby kazdy hral neco jineho.
//
//  Zpravy (prenasi je server - engine N&P - beze zmeny; jedna zprava = jedna zprava WebSocketu):
//    AHOJ  verze, jadro, otisk hry (SHA-1 obsahu 20 B), otisk nastaveni (u32), jmeno hrace
//    PING / PONG  cas odesilatele (us) - odezva
//    START (hrac 1) zpozdeni D
//    VSTUP snimek (u32), pocet n (u8), n vstupu (u32) od toho snimku
//    HASH  snimek (u32), otisk stavu (u64)
//    KONEC duvod
//  Vsechna cisla little-endian. Bez Androidu a bez site - test test_overeni/b357 (dva emulatory, simulovana
//  sit se zpozdenim a kolisanim).
// =====================================================================================================
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstring>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace nap {
namespace online {

constexpr uint8_t VERZE = 1;

enum Typ : uint8_t { AHOJ = 1, PING = 2, PONG = 3, START = 4, VSTUP = 5, HASH = 6, KONEC = 7 };
enum Jadro : uint8_t { J_ATARI = 1, J_SEGA = 2, J_PS1 = 3 };
enum Duvod : uint8_t {
  D_NIC = 0,
  D_HRAC = 1,        // druhy hrac hru ukoncil
  D_ROZJETO = 2,     // otisky stavu se lisi
  D_JINA_HRA = 3,    // jina hra / jine nastaveni / jine jadro
  D_VERZE = 4,       // jina verze protokolu (starsi appka)
  D_TICHO = 5,       // od druheho hrace dlouho nic neprislo
  D_CHYBA = 6,       // poskozena zprava
  D_SIT = 7          // spojeni se serverem spadlo
};

enum class Stav { CEKA_AHOJ, MERI, CEKA_START, BEZI, KONEC };

constexpr uint32_t HASH_KAZDYCH = 60;      // otisk stavu kazdych 60 snimku (~1 s)
constexpr uint32_t ZPOZDENI_MIN = 2;
constexpr uint32_t ZPOZDENI_MAX = 15;
constexpr int PINGU = 6;                    // kolik mereni odezvy pred startem
constexpr uint32_t OKNO = 512;              // kruh vstupu (snimky); druhy hrac muze byt napred nejvys o 2 D

inline const char *popisDuvodu(uint8_t d) {
  switch (d) {
    case D_HRAC: return "druhy hrac hru ukoncil";
    case D_ROZJETO: return "hra se rozjela - telefony uz nehraji totez (otisk stavu se lisi)";
    case D_JINA_HRA: return "druhy hrac ma jinou hru nebo jine nastaveni";
    case D_VERZE: return "druhy hrac ma jinou verzi appky - aktualizujte obe";
    case D_TICHO: return "od druheho hrace dlouho nic neprislo";
    case D_CHYBA: return "poskozena zprava ze site";
    case D_SIT: return "spojeni se serverem spadlo";
    default: return "";
  }
}

// --------------------------------------------------------------------------- pomocne
inline void w8(std::vector<uint8_t> &v, uint8_t x) { v.push_back(x); }
inline void w32(std::vector<uint8_t> &v, uint32_t x) { for (int i = 0; i < 4; i++) v.push_back((uint8_t)(x >> (8 * i))); }
inline void w64(std::vector<uint8_t> &v, uint64_t x) { for (int i = 0; i < 8; i++) v.push_back((uint8_t)(x >> (8 * i))); }
inline uint32_t r32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24); }
inline uint64_t r64(const uint8_t *p) { return (uint64_t)r32(p) | ((uint64_t)r32(p + 4) << 32); }

/** FNV-1a 64 - otisk stavu (RAM, VRAM ...). Stejny na vsech telefonech (zadna zavislost na CPU). */
inline uint64_t otisk(const void *d, size_t n, uint64_t h = 1469598103934665603ULL) {
  const uint8_t *p = (const uint8_t *)d;
  for (size_t i = 0; i < n; i++) { h ^= p[i]; h *= 1099511628211ULL; }
  return h;
}

inline uint64_t tedUs() {
  return (uint64_t)std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
}

/** Zpozdeni vstupu ze zmerene odezvy: pul odezvy + rezerva na kolisani, v celych snimcich, + 1. */
inline uint32_t zpozdeniZOdezvy(int rttMs, int jitterMs, double snimekMs) {
  if (snimekMs <= 0) snimekMs = 16.7;
  const double jednosmerne = rttMs / 2.0 + jitterMs + 4.0;   // +4 ms zpracovani na telefonech
  uint32_t d = (uint32_t)(jednosmerne / snimekMs + 0.999) + 1;
  if (d < ZPOZDENI_MIN) d = ZPOZDENI_MIN;
  if (d > ZPOZDENI_MAX) d = ZPOZDENI_MAX;
  return d;
}

struct Ahoj {
  uint8_t verze = VERZE;
  uint8_t jadro = 0;
  uint8_t hra[20] = {};      // SHA-1 obsahu hry (ROM, XEX, CUE+BIN ...)
  uint32_t nastaveni = 0;    // otisk nastaveni jadra, ktere meni chovani (region, stroj ...)
  std::string jmeno;         // jmeno hrace (jen pro zobrazeni)
};

// =====================================================================================================
//  Relace - jedna online hra. Vlakno emulace: lokalni(), vstupy(), otiskStavu(). Vlakno site: prijmi(),
//  vyzvedni() / cekejNaOdchozi(), tik(). Vse pod jednim zamkem.
// =====================================================================================================
class Relace {
 public:
  Relace(bool hrac1, const Ahoj &ja, double snimekMs) : hrac1_(hrac1), ja_(ja), snimekMs_(snimekMs) {
    for (uint32_t i = 0; i < OKNO; i++) { muj_[i] = jeho_[i] = Polozka(); }
  }

  bool hrac1() const { return hrac1_; }

  /** Zavola se, kdykoli je co poslat (vlakno site ceka v poll() na socketu - probudi ho). Nastavit pred zacni(). */
  void budik(std::function<void()> b) { std::lock_guard<std::mutex> l(m_); budik_ = std::move(b); }

  /** Server hlasi, ze druhy hrac odesel (spojeni mu spadlo, nebo hru zavrel) - pokud uz neni duvod konce. */
  void druhyOdesel() {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::KONEC) ukonciLocked(D_HRAC, false);
  }

  // ------------------------------------------------------------------ zacatek (vlakno site)
  /** Odesle AHOJ - zavolat hned po pripojeni (oba). */
  void zacni(uint64_t ted) {
    std::lock_guard<std::mutex> l(m_);
    posliAhojLocked();
    stav_ = Stav::CEKA_AHOJ;
    posledniPrijemUs_ = ted;
  }

  /**
   * Pravidelne z vlakna site (kazdych ~20 ms): hrac 1 meri odezvu a pak posle START; oba hlidaji ticho.
   * @return true, kdyz se prave rozhodlo o startu (obe strany: emulator muze zacit od snimku 0)
   */
  bool tik(uint64_t ted, uint64_t tichoUs = 15000000ULL) {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ == Stav::KONEC) return false;
    if (posledniPrijemUs_ && ted > posledniPrijemUs_ + tichoUs) { ukonciLocked(D_TICHO, true); return false; }
    if (stav_ == Stav::MERI && hrac1_) {
      if (pingu_ < PINGU && ted >= dalsiPingUs_) {
        std::vector<uint8_t> z;
        w8(z, PING); w64(z, ted);
        ven_.push_back(z);
        pingu_++;
        dalsiPingUs_ = ted + 60000;          // 60 ms mezi mericimi pingy
        cvVen_.notify_all(); if (budik_) budik_();
      }
      if (pongu_ >= PINGU || (pingu_ >= PINGU && ted > dalsiPingUs_ + 1500000)) {
        const int rtt = pongu_ ? (int)(nejmensiRttUs_ / 1000) : 300;
        // rezerva = cele rozpeti namerenych odezev (6 pingu nezachyti nejhorsi pripad; mobilni sit kolisa)
        const int jitter = pongu_ ? (int)((nejvetsiRttUs_ - nejmensiRttUs_) / 1000) : 50;
        D_ = zpozdeniZOdezvy(rtt, jitter, snimekMs_);
        std::vector<uint8_t> z;
        w8(z, START); w32(z, D_);
        ven_.push_back(z);
        cvVen_.notify_all(); if (budik_) budik_();
        // Hrac 1 zacne az za pul odezvy - to je chvile, kdy START dorazi k hraci 2 (ten zacne hned). Oba tak
        // jedou soubezne a staci D na JEDNOSMERNOU cestu. Bez toho hrac 1 cekal skoro v kazdem snimku
        // (test 4: 415 x z 420) - jeho snimky by se trhaly podle kolisani site.
        startPoUs_ = ted + (uint64_t)rtt * 500;
        stav_ = Stav::CEKA_START;
      }
    }
    if (stav_ == Stav::CEKA_START && hrac1_ && startPoUs_ && ted >= startPoUs_) {
      startPoUs_ = 0;
      startLocked();
      return true;
    }
    if (stav_ == Stav::BEZI && ted >= dalsiPingUs_) {   // odezva i za hry (zobrazeni)
      std::vector<uint8_t> z;
      w8(z, PING); w64(z, ted);
      ven_.push_back(z);
      dalsiPingUs_ = ted + 2000000;
      cvVen_.notify_all(); if (budik_) budik_();
    }
    const bool s = startPrave_;
    startPrave_ = false;
    return s;
  }

  // ------------------------------------------------------------------ prijem (vlakno site)
  /** Jedna cela zprava ze site. false = zprava poskozena (relace skoncila D_CHYBA). */
  bool prijmi(const uint8_t *d, size_t n, uint64_t ted = 0) {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ == Stav::KONEC) return true;
    if (n < 1) return chybaLocked();
    posledniPrijemUs_ = ted ? ted : tedUs();
    switch (d[0]) {
      case AHOJ: {
        if (n < 1 + 1 + 1 + 20 + 4 + 1) return chybaLocked();
        jeho_ahoj_.verze = d[1];
        jeho_ahoj_.jadro = d[2];
        std::memcpy(jeho_ahoj_.hra, d + 3, 20);
        jeho_ahoj_.nastaveni = r32(d + 23);
        const size_t dj = d[27];
        if (n < 28 + dj) return chybaLocked();
        jeho_ahoj_.jmeno.assign((const char *)d + 28, dj);
        mamAhoj_ = true;
        if (jeho_ahoj_.verze != ja_.verze) { ukonciLocked(D_VERZE, true); return true; }
        if (jeho_ahoj_.jadro != ja_.jadro || std::memcmp(jeho_ahoj_.hra, ja_.hra, 20) != 0 || jeho_ahoj_.nastaveni != ja_.nastaveni) {
          ukonciLocked(D_JINA_HRA, true);
          return true;
        }
        if (stav_ == Stav::CEKA_AHOJ) stav_ = hrac1_ ? Stav::MERI : Stav::CEKA_START;
        return true;
      }
      case PING: {
        if (n < 9) return chybaLocked();
        std::vector<uint8_t> z;
        w8(z, PONG); w64(z, r64(d + 1));
        ven_.push_back(z);
        cvVen_.notify_all(); if (budik_) budik_();
        return true;
      }
      case PONG: {
        if (n < 9) return chybaLocked();
        const uint64_t t = r64(d + 1), ted2 = posledniPrijemUs_;
        if (ted2 >= t) {
          const uint64_t rtt = ted2 - t;
          if (!pongu_ || rtt < nejmensiRttUs_) nejmensiRttUs_ = rtt;
          if (!pongu_ || rtt > nejvetsiRttUs_) nejvetsiRttUs_ = rtt;
          pongu_++;
          posledniRttUs_ = rtt;
        }
        return true;
      }
      case START: {
        if (n < 5) return chybaLocked();
        if (hrac1_ || stav_ != Stav::CEKA_START) return true;   // START posila jen hrac 1
        uint32_t d2 = r32(d + 1);
        if (d2 < ZPOZDENI_MIN || d2 > ZPOZDENI_MAX) return chybaLocked();
        D_ = d2;
        startLocked();
        startPrave_ = true;
        return true;
      }
      case VSTUP: {
        if (n < 6) return chybaLocked();
        const uint32_t od = r32(d + 1);
        const uint32_t pocet = d[5];
        if (n < 6 + 4 * (size_t)pocet) return chybaLocked();
        for (uint32_t i = 0; i < pocet; i++) {
          const uint32_t s = od + i;
          if (s < spotrebovano_) continue;                                  // stary (uz emulovany) snimek
          if (s >= spotrebovano_ + OKNO - 1) return chybaLocked();          // moc napred - nemozne (max 2 D)
          Polozka &p = jeho_[s % OKNO];
          p.snimek = s; p.bity = r32(d + 6 + 4 * i); p.plne = true;
        }
        cv_.notify_all();
        return true;
      }
      case HASH: {
        if (n < 13) return chybaLocked();
        const uint32_t s = r32(d + 1);
        const uint64_t h = r64(d + 5);
        porovnejLocked(s, h, false);
        return true;
      }
      case KONEC: {
        // duvod druheho z jeho pohledu: rozjeti / jina hra / verze plati pro oba, jeho ticho / sit / chyba = spojeni
        // mezi nami se prerusilo, jinak hru ukoncil on
        const uint8_t duvod = n >= 2 ? d[1] : (uint8_t)D_HRAC;
        uint8_t muj = D_HRAC;
        if (duvod == D_ROZJETO || duvod == D_JINA_HRA || duvod == D_VERZE) muj = duvod;
        else if (duvod == D_TICHO || duvod == D_SIT || duvod == D_CHYBA) muj = D_TICHO;
        ukonciLocked(muj, false);
        return true;
      }
      default:
        return true;   // neznamy typ (novejsi verze) - ignorovat
    }
  }

  /** Spojeni se serverem spadlo (vlakno site). */
  void ztrataSpojeni() {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::KONEC) ukonciLocked(D_SIT, false);
  }

  /** Hrac sam konci (posle KONEC). */
  void ukonci(uint8_t duvod = D_HRAC) {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::KONEC) ukonciLocked(duvod, true);
  }

  // ------------------------------------------------------------------ odchozi (vlakno site)
  std::vector<std::vector<uint8_t>> vyzvedni() {
    std::lock_guard<std::mutex> l(m_);
    std::vector<std::vector<uint8_t>> r(ven_.begin(), ven_.end());
    ven_.clear();
    return r;
  }
  /** Ceka na odchozi zpravu nebo konec (max ms). */
  void cekejNaOdchozi(int ms) {
    std::unique_lock<std::mutex> l(m_);
    if (!ven_.empty() || stav_ == Stav::KONEC) return;
    cvVen_.wait_for(l, std::chrono::milliseconds(ms), [&] { return !ven_.empty() || stav_ == Stav::KONEC; });
  }

  // ------------------------------------------------------------------ za hry (vlakno emulace)
  /**
   * Vstup tohoto telefonu nasbirany PRED emulaci snimku 'snimek' - plati pro snimek + D. Odesle se hned.
   * (Snimky 0 .. D-1 maji vstup 0 u obou - stejne na obou stranach.)
   */
  void lokalni(uint32_t snimek, uint32_t bity) {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::BEZI) return;
    const uint32_t s = snimek + D_;
    Polozka &p = muj_[s % OKNO];
    if (p.plne && p.snimek == s) return;   // uz poslano (opakovany pokus o snimek po cekani)
    p.snimek = s; p.bity = bity; p.plne = true;
    std::vector<uint8_t> z;
    w8(z, VSTUP); w32(z, s); w8(z, 1); w32(z, bity);
    ven_.push_back(z);
    cvVen_.notify_all(); if (budik_) budik_();
  }

  /**
   * Vstupy obou hracu pro snimek (hrac 1, hrac 2). Ceka nejvys cekejMs na druheho hrace.
   * @return 1 = mam, 0 = jeste ne (cekat dal / emulator stoji), -1 = relace skoncila
   */
  int vstupy(uint32_t snimek, uint32_t &p1, uint32_t &p2, int cekejMs) {
    std::unique_lock<std::mutex> l(m_);
    auto mam = [&]() {
      if (stav_ == Stav::KONEC) return true;
      if (snimek < D_) return true;
      const Polozka &a = muj_[snimek % OKNO], &b = jeho_[snimek % OKNO];
      return a.plne && a.snimek == snimek && b.plne && b.snimek == snimek;
    };
    if (!mam()) {
      cekani_++;
      if (cekejMs > 0) cv_.wait_for(l, std::chrono::milliseconds(cekejMs), mam);
    }
    if (stav_ == Stav::KONEC) return -1;
    if (!mam()) return 0;
    uint32_t moje = 0, jeho = 0;
    if (snimek >= D_) { moje = muj_[snimek % OKNO].bity; jeho = jeho_[snimek % OKNO].bity; }
    p1 = hrac1_ ? moje : jeho;
    p2 = hrac1_ ? jeho : moje;
    if (snimek + 1 > spotrebovano_) spotrebovano_ = snimek + 1;
    return 1;
  }

  /** Otisk stavu PO emulaci snimku (jen kazdych HASH_KAZDYCH snimku se posila a porovnava). */
  void otiskStavu(uint32_t snimek, uint64_t h) {
    if (snimek % HASH_KAZDYCH != 0) return;
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::BEZI) return;
    std::vector<uint8_t> z;
    w8(z, HASH); w32(z, snimek); w64(z, h);
    ven_.push_back(z);
    cvVen_.notify_all(); if (budik_) budik_();
    porovnejLocked(snimek, h, true);
  }

  // ------------------------------------------------------------------ stav
  Stav stav() const { std::lock_guard<std::mutex> l(m_); return stav_; }
  bool bezi() const { std::lock_guard<std::mutex> l(m_); return stav_ == Stav::BEZI; }
  bool konec() const { std::lock_guard<std::mutex> l(m_); return stav_ == Stav::KONEC; }
  uint8_t duvod() const { std::lock_guard<std::mutex> l(m_); return duvod_; }
  bool duvodOdeMne() const { std::lock_guard<std::mutex> l(m_); return duvodOdeMne_; }
  uint32_t zpozdeni() const { std::lock_guard<std::mutex> l(m_); return D_; }
  int odezvaMs() const { std::lock_guard<std::mutex> l(m_); return (int)(posledniRttUs_ / 1000); }
  uint32_t cekani() const { std::lock_guard<std::mutex> l(m_); return cekani_; }
  uint32_t overenychOtisku() const { std::lock_guard<std::mutex> l(m_); return overeno_; }
  uint32_t rozjetoVeSnimku() const { std::lock_guard<std::mutex> l(m_); return rozjetoSnimek_; }
  std::string jmenoDruheho() const { std::lock_guard<std::mutex> l(m_); return jeho_ahoj_.jmeno; }
  bool mamAhojDruheho() const { std::lock_guard<std::mutex> l(m_); return mamAhoj_; }

 private:
  struct Polozka { uint32_t snimek = 0; uint32_t bity = 0; bool plne = false; };
  struct Otisk { uint32_t snimek = 0; uint64_t h = 0; bool je = false; };

  void posliAhojLocked() {
    std::vector<uint8_t> z;
    w8(z, AHOJ); w8(z, ja_.verze); w8(z, ja_.jadro);
    for (int i = 0; i < 20; i++) w8(z, ja_.hra[i]);
    w32(z, ja_.nastaveni);
    std::string j = ja_.jmeno.substr(0, 60);
    w8(z, (uint8_t)j.size());
    z.insert(z.end(), j.begin(), j.end());
    ven_.push_back(z);
    cvVen_.notify_all(); if (budik_) budik_();
  }

  void startLocked() {
    stav_ = Stav::BEZI;
    spotrebovano_ = 0;
    for (uint32_t i = 0; i < OKNO; i++) { muj_[i] = Polozka(); }
    // vstupy druheho, ktere prisly pred startem (nemohou - posila je az po startu), nemazat
    dalsiPingUs_ = 0;
  }

  void porovnejLocked(uint32_t s, uint64_t h, bool muj) {
    Otisk *a = muj ? mojeOtisky_ : jehoOtisky_;
    Otisk *b = muj ? jehoOtisky_ : mojeOtisky_;
    const uint32_t i = (s / HASH_KAZDYCH) % 8;
    a[i].snimek = s; a[i].h = h; a[i].je = true;
    if (b[i].je && b[i].snimek == s) {
      if (b[i].h == h) overeno_++;
      else if (stav_ != Stav::KONEC) { rozjetoSnimek_ = s; ukonciLocked(D_ROZJETO, true); }
    }
  }

  bool chybaLocked() {
    ukonciLocked(D_CHYBA, true);
    return false;
  }

  void ukonciLocked(uint8_t duvod, bool poslat) {
    if (stav_ == Stav::KONEC) return;
    duvod_ = duvod;
    duvodOdeMne_ = poslat;
    if (poslat) {
      std::vector<uint8_t> z;
      w8(z, KONEC); w8(z, duvod);
      ven_.push_back(z);
    }
    stav_ = Stav::KONEC;
    cv_.notify_all();
    cvVen_.notify_all(); if (budik_) budik_();
  }

  mutable std::mutex m_;
  std::condition_variable cv_, cvVen_;
  std::function<void()> budik_;
  const bool hrac1_;
  Ahoj ja_, jeho_ahoj_;
  const double snimekMs_;
  Stav stav_ = Stav::CEKA_AHOJ;
  bool mamAhoj_ = false;
  bool startPrave_ = false;
  uint32_t D_ = 3;
  Polozka muj_[OKNO], jeho_[OKNO];
  uint32_t spotrebovano_ = 0;
  std::deque<std::vector<uint8_t>> ven_;
  int pingu_ = 0, pongu_ = 0;
  uint64_t startPoUs_ = 0;
  uint64_t dalsiPingUs_ = 0, nejmensiRttUs_ = 0, nejvetsiRttUs_ = 0, posledniRttUs_ = 0, posledniPrijemUs_ = 0;
  Otisk mojeOtisky_[8], jehoOtisky_[8];
  uint32_t overeno_ = 0, rozjetoSnimek_ = 0, cekani_ = 0;
  uint8_t duvod_ = D_NIC;
  bool duvodOdeMne_ = false;
};

}  // namespace online
}  // namespace nap
