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
//  B360 (verze 2) - po prvnim skutecnem testu (Rene 23. 11.: "telefony se sekaji ... kazdy jina chyba"):
//    HASH ma navic casti otisku (RAM 68000, RAM Z80, VRAM, CRAM, VSRAM) - v logu je videt, CO se rozjelo
//    ZPOZDENI o_kolik (u8)  "tvoje stisky chodi pozde" - druhy si zvysi SVE zpozdeni (kazdy telefon ma sve, meni se za hry)
//    RESYNC snimek (hrac 1 -> 2): hra se rozjela - pred timto snimkem prijde cely stav hry, do te doby necekej dal
//    STAV snimek + data (hrac 1 -> 2): stav jadra (zabaleny, data jsou veci jadra) - hrac 2 ho nahraje a hraje se dal
//    STAV_OK snimek (hrac 2 -> 1): nahrano
//  Vsechna cisla little-endian. Bez Androidu a bez site - test test_overeni/b357 (dva emulatory, simulovana
//  sit se zpozdenim a kolisanim).
// =====================================================================================================
#pragma once

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <climits>
#include <cstring>
#include <deque>
#include <algorithm>
#include <functional>
#include <iterator>
#include <mutex>
#include <string>
#include <vector>

namespace nap {
namespace online {

constexpr uint8_t VERZE = 2;   // B360: zpozdeni kazdeho telefonu zvlast, synchronizace stavu pri rozjeti

enum Typ : uint8_t { AHOJ = 1, PING = 2, PONG = 3, START = 4, VSTUP = 5, HASH = 6, KONEC = 7, ZPOZDENI = 8, RESYNC = 9, STAV = 10, STAV_OK = 11 };
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
constexpr uint8_t CASTI_MAX = 8;            // B360: casti otisku stavu (RAM 68000, RAM Z80, VRAM, CRAM, VSRAM ...)
constexpr size_t STAV_MAX = 16u << 20;      // B360: zabaleny stav jadra nejvys 16 MB (Sega ~ 100-300 KB)

/** B360: ladeni za hry (vychozi hodnoty pro telefony; test je zkracuje). */
struct Ladeni {
  uint32_t zpozdeniNavic = 8;                    // vlastni D smi vyrust nejvys o tolik nad start (a do ZPOZDENI_MAX)
  uint64_t dlouheCekaniUs = 34000;               // cekani na stisky druheho delsi nez 2 snimky = sit kolisa
  uint32_t zvysitNejvys = 4;                     // jedna zadost zvysi zpozdeni druheho nejvys o 4 snimky
  uint64_t zadostKazdychUs = 2000000;            // o vetsi zpozdeni druheho nejvys jednou za 2 s
  uint64_t zvyseniKazdychUs = 1000000;           // vlastni D nejvys jednou za 1 s
  uint64_t snizitPoUs = 30000000;                // 30 s bez zadosti -> vlastni D o 1 dolu (a pak kazdych 30 s, ne pod start)
  uint32_t maxSynchronizaci = 20;                // za jednu hru
  uint64_t synchronizaceOknoUs = 10000000;       // 3 synchronizace do 10 s = nepomaha, konec
  uint64_t cekatNaSynchronizaciUs = 5000000;     // hrac 2 vidi rozdil, RESYNC od hrace 1 neprisel
  uint64_t cekatNaStavUs = 10000000;             // hrac 2 dostal RESYNC, STAV neprisel (pojistka - jinak by stali oba)
};

inline const char *popisDuvodu(uint8_t d) {
  switch (d) {
    case D_HRAC: return "druhy hrac hru ukoncil";
    case D_ROZJETO: return "hra se rozjela a synchronizace nepomohla (otisk stavu se lisi)";
    case D_JINA_HRA: return "druhy hrac ma jinou hru nebo jine nastaveni";
    case D_VERZE: return "druhy hrac ma jinou verzi appky - aktualizujte obe";
    case D_TICHO: return "od druheho hrace dlouho nic neprislo - spadla mu sit, nebo se telefon zasekl";
    case D_CHYBA: return "poskozena zprava ze site";
    case D_SIT: return "spojeni se serverem spadlo";
    default: return "";
  }
}

/**
 * B360: text konce pro hrace a log - STEJNY z vlakna site (OnlineKlient) i z vlakna emulace. Drive vlakno emulace
 * konec poznalo driv, nez vlakno site text zapsalo, a v logu bylo jen "online skoncilo" (Rene 23. 11., pokus 2:
 * ve skutecnosti 15 s nic od druheho telefonu).
 */
inline std::string textKonce(uint8_t duvod, bool odeMne) {
  if (duvod == D_HRAC && odeMne) return "online hru jsi ukoncil";
  return popisDuvodu(duvod);
}

/** B360: nazvy casti otisku stavu podle bitu (jadro je posila v tomhle poradi). */
inline std::string nazvyCasti(uint32_t bity, const char *const *nazvy, int n) {
  std::string r;
  for (int i = 0; i < n; i++) if (bity & (1u << i)) { if (!r.empty()) r += ", "; r += nazvy[i]; }
  return r;
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
//  Relace - jedna online hra. Vlakno emulace: lokalni(), stav (chciPoslatStav / posliStav, stavPro / stavNahran),
//  vstupy(), otiskStavu(). Vlakno site: prijmi(), vyzvedni() / cekejNaOdchozi(), tik(). Vse pod jednim zamkem.
//
//  B360 - ZPOZDENI KAZDEHO TELEFONU ZVLAST: vstup ma v sobe cislo snimku, pro ktery plati, takze kazdy telefon
//  muze posilat sve stisky s jinym predstihem (Dmuj_) a menit ho za hry. Zadny snimek nesmi zustat bez vstupu:
//  pri zvyseni D se mezera vyplni stejnym stiskem, pri snizeni se snimky, ktere uz maji vstup, jen preskoci.
//  Telefon, ktery na stisky druheho cekal dele nez 2 snimky (sit kolisa), posle ZPOZDENI o tolik snimku, kolik
//  chybelo (nejvys 4) - druhy si D zvysi. Po 30 s bez zadosti ho zase o 1 snizi (ne pod start).
//
//  B360 - SYNCHRONIZACE MISTO KONCE: kdyz se otisky stavu lisi, hrac 1 (je "hlavni") zvoli snimek G = prvni,
//  pro ktery jeste NEPOSLAL svuj vstup, posle RESYNC(G) (dorazi pred jeho vstupem pro G - TCP drzi poradi, takze
//  hrac 2 snimek G bez nej emulovat nemuze), a az sam dojde ke G, posle cely stav jadra (STAV). Hrac 2 pred
//  snimkem G pocka na stav, nahraje ho a oba pokracuji od stejneho stavu se stejnymi vstupy. Otisky pred G se
//  uz neporovnavaji. 3 synchronizace do 10 s (nebo 20 za hru) = nepomaha -> konec D_ROZJETO.
// =====================================================================================================
class Relace {
 public:
  Relace(bool hrac1, const Ahoj &ja, double snimekMs, const Ladeni &ladeni = Ladeni())
      : hrac1_(hrac1), ja_(ja), snimekMs_(snimekMs), ladeni_(ladeni) {
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
   * Pravidelne z vlakna site (kazdych ~20 ms): hrac 1 meri odezvu a pak posle START; oba hlidaji ticho,
   * B360: snizuji sve zpozdeni po klidu, hrac 2 hlida, ze po rozdilu otisku prijde synchronizace.
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
        posliLocked(z);
        pingu_++;
        dalsiPingUs_ = ted + 60000;          // 60 ms mezi mericimi pingy
      }
      if (pongu_ >= PINGU || (pingu_ >= PINGU && ted > dalsiPingUs_ + 1500000)) {
        const int rtt = pongu_ ? (int)(nejmensiRttUs_ / 1000) : 300;
        // rezerva = cele rozpeti namerenych odezev (6 pingu nezachyti nejhorsi pripad; mobilni sit kolisa)
        const int jitter = pongu_ ? (int)((nejvetsiRttUs_ - nejmensiRttUs_) / 1000) : 50;
        D_ = zpozdeniZOdezvy(rtt, jitter, snimekMs_);
        std::vector<uint8_t> z;
        w8(z, START); w32(z, D_);
        posliLocked(z);
        // Hrac 1 zacne az za pul odezvy - to je chvile, kdy START dorazi k hraci 2 (ten zacne hned). Oba tak
        // jedou soubezne a staci D na JEDNOSMERNOU cestu. Bez toho hrac 1 cekal skoro v kazdem snimku
        // (test 4: 415 x z 420) - jeho snimky by se trhaly podle kolisani site.
        startPoUs_ = ted + (uint64_t)rtt * 500;
        stav_ = Stav::CEKA_START;
      }
    }
    if (stav_ == Stav::CEKA_START && hrac1_ && startPoUs_ && ted >= startPoUs_) {
      startPoUs_ = 0;
      startLocked(ted);
      return true;
    }
    if (stav_ == Stav::BEZI) {
      if (ted >= dalsiPingUs_) {   // odezva i za hry (zobrazeni)
        std::vector<uint8_t> z;
        w8(z, PING); w64(z, ted);
        posliLocked(z);
        dalsiPingUs_ = ted + 2000000;
      }
      // B360: dlouho nikdo nezadal o vetsi zpozdeni -> o 1 dolu (mensi prodleva ovladani), ne pod start
      if (Dmuj_ > D_ && ted >= std::max(zvyseniUs_, snizeniUs_) + ladeni_.snizitPoUs) {
        Dmuj_--;
        snizeni_++;
        snizeniUs_ = ted;
      }
      // B360: hrac 2 videl rozdil otisku a synchronizace od hrace 1 neprisla / RESYNC prisel, ale stav ne
      if (!hrac1_ && rozjetoVidimUs_ && ted > rozjetoVidimUs_ + ladeni_.cekatNaSynchronizaciUs) ukonciLocked(D_ROZJETO, true);
      else if (!hrac1_ && resyncAktivni_ && !maStav_ && resyncOdUs_ && ted > resyncOdUs_ + ladeni_.cekatNaStavUs) ukonciLocked(D_ROZJETO, true);
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
        posliLocked(z);
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
        startLocked(posledniPrijemUs_);
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
        uint64_t casti[CASTI_MAX] = {};
        uint8_t nc = 0;
        if (n >= 14) {   // B360: casti otisku (verze 2)
          nc = d[13];
          if (nc > CASTI_MAX || n < 14 + 8 * (size_t)nc) return chybaLocked();
          for (uint8_t i = 0; i < nc; i++) casti[i] = r64(d + 14 + 8 * i);
        }
        porovnejLocked(s, h, casti, nc, false);
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
      case ZPOZDENI: {   // B360: druhy na me stisky cekal - vlastni D nahoru o kolik chce (ne casteji nez 1x za s, strop)
        if (stav_ != Stav::BEZI) return true;
        const uint32_t strop = std::min<uint32_t>(ZPOZDENI_MAX, D_ + ladeni_.zpozdeniNavic);
        uint32_t o = n >= 2 ? d[1] : 1;
        if (o < 1) o = 1;
        if (o > ladeni_.zvysitNejvys) o = ladeni_.zvysitNejvys;
        if (Dmuj_ < strop && posledniPrijemUs_ >= zvyseniUs_ + ladeni_.zvyseniKazdychUs) {
          Dmuj_ = std::min(strop, Dmuj_ + o);
          zvyseni_++;
          zvyseniUs_ = posledniPrijemUs_;
        }
        return true;
      }
      case RESYNC: {     // B360: hrac 1 -> 2: pred snimkem G prijde stav hry
        if (n < 5) return chybaLocked();
        if (hrac1_ || stav_ != Stav::BEZI) return true;
        const uint32_t g = r32(d + 1);
        if (g < spotrebovano_) return chybaLocked();   // G uz je emulovany - nemozne (RESYNC jde pred vstupem pro G)
        resyncG_ = g;
        resyncAktivni_ = true;
        maStav_ = false;
        stavData_.clear();
        if (g > ignorujDo_) ignorujDo_ = g;
        rozjetoVidimUs_ = 0;
        resyncOdUs_ = posledniPrijemUs_;
        synchronizaci_++;
        return true;
      }
      case STAV: {       // B360: hrac 1 -> 2: stav jadra pro snimek G
        if (n < 5 || n - 5 > STAV_MAX) return chybaLocked();
        if (hrac1_ || stav_ != Stav::BEZI) return true;
        const uint32_t g = r32(d + 1);
        if (!resyncAktivni_ || g != resyncG_) return true;   // stary / necekany - nic
        stavData_.assign(d + 5, d + n);
        maStav_ = true;
        stavKB_ = (uint32_t)((n - 5 + 1023) / 1024);
        cv_.notify_all();
        return true;
      }
      case STAV_OK: {    // B360: hrac 2 -> 1: nahrano
        if (n < 5) return chybaLocked();
        if (hrac1_ && resyncAktivni_ && stavOdeslan_ && r32(d + 1) == resyncG_) { resyncAktivni_ = false; synchronizovano_++; }
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
    std::vector<std::vector<uint8_t>> r(std::make_move_iterator(ven_.begin()), std::make_move_iterator(ven_.end()));
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
   * Vstup tohoto telefonu nasbirany PRED emulaci snimku 'snimek' - plati pro snimek + Dmuj_. Odesle se hned.
   * (Snimky 0 .. D-1 maji vstup 0 u obou - stejne na obou stranach.) B360: po zvyseni D se mezera vyplni
   * stejnym stiskem; snimky, ktere uz vstup maji (opakovany pokus po cekani, snizene D), se neposilaji znovu.
   */
  void lokalni(uint32_t snimek, uint32_t bity) {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::BEZI) return;
    const uint32_t cil = snimek + Dmuj_;
    if (cil <= posledniPoslany_) return;
    const uint32_t od = posledniPoslany_ + 1;
    uint32_t pocet = cil - od + 1;
    if (pocet > 255) pocet = 255;   // nemozne (D roste po 1), jen pojistka velikosti zpravy
    std::vector<uint8_t> z;
    w8(z, VSTUP); w32(z, od); w8(z, (uint8_t)pocet);
    for (uint32_t i = 0; i < pocet; i++) {
      const uint32_t s = od + i;
      Polozka &p = muj_[s % OKNO];
      p.snimek = s; p.bity = bity; p.plne = true;
      w32(z, bity);
    }
    posledniPoslany_ = od + pocet - 1;
    posliLocked(z);
  }

  /**
   * Vstupy obou hracu pro snimek (hrac 1, hrac 2). Ceka nejvys cekejMs na druheho hrace.
   * @return 1 = mam, 0 = jeste ne (cekat dal / emulator stoji), -1 = relace skoncila
   */
  int vstupy(uint32_t snimek, uint32_t &p1, uint32_t &p2, int cekejMs) {
    std::unique_lock<std::mutex> l(m_);
    // B360: hrac 1 je za snimkem G a stav neposlal (jadro synchronizaci neumi) - hra se rozjela a nespravi se
    if (hrac1_ && stav_ == Stav::BEZI && resyncAktivni_ && !stavOdeslan_ && snimek > resyncG_) ukonciLocked(D_ROZJETO, true);
    auto mam = [&]() {
      if (stav_ == Stav::KONEC) return true;
      if (!hrac1_ && resyncAktivni_ && snimek >= resyncG_) return false;   // B360: nejdriv stav od hrace 1
      if (snimek < D_) return true;
      const Polozka &a = muj_[snimek % OKNO], &b = jeho_[snimek % OKNO];
      return a.plne && a.snimek == snimek && b.plne && b.snimek == snimek;
    };
    if (!mam()) {
      if (cekaSnimek_ != snimek) { cekaSnimek_ = snimek; cekaOdUs_ = tedUs(); cekani_++; }
      if (cekejMs > 0) cv_.wait_for(l, std::chrono::milliseconds(cekejMs), mam);
    }
    if (stav_ == Stav::KONEC) return -1;
    if (!mam()) return 0;
    if (cekaSnimek_ == snimek && cekaOdUs_) {   // B360: jak dlouho se na tento snimek cekalo
      const uint64_t t = tedUs() - cekaOdUs_;
      cekaniUs_ += t;
      if (t > cekaniMaxUs_) cekaniMaxUs_ = t;
      if (t >= ladeni_.dlouheCekaniUs) { dlouhychCekani_++; pozadejOZpozdeniLocked(t); }
      cekaOdUs_ = 0;
    }
    uint32_t moje = 0, jeho = 0;
    if (snimek >= D_) { moje = muj_[snimek % OKNO].bity; jeho = jeho_[snimek % OKNO].bity; }
    p1 = hrac1_ ? moje : jeho;
    p2 = hrac1_ ? jeho : moje;
    if (snimek + 1 > spotrebovano_) spotrebovano_ = snimek + 1;
    return 1;
  }

  /**
   * Otisk stavu PO emulaci snimku (jen kazdych HASH_KAZDYCH snimku se posila a porovnava). B360: casti (RAM 68000,
   * RAM Z80 ...) jdou s nim - pri rozjeti je v logu videt, co se lisi.
   */
  void otiskStavu(uint32_t snimek, uint64_t h, const uint64_t *casti = nullptr, uint8_t nCasti = 0) {
    if (snimek % HASH_KAZDYCH != 0) return;
    if (!casti) nCasti = 0;
    if (nCasti > CASTI_MAX) nCasti = CASTI_MAX;
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::BEZI) return;
    std::vector<uint8_t> z;
    w8(z, HASH); w32(z, snimek); w64(z, h);
    w8(z, nCasti);
    for (uint8_t i = 0; i < nCasti; i++) w64(z, casti[i]);
    posliLocked(z);
    uint64_t c[CASTI_MAX] = {};
    for (uint8_t i = 0; i < nCasti; i++) c[i] = casti[i];
    porovnejLocked(snimek, h, c, nCasti, true);
  }

  // ------------------------------------------------------------------ B360: synchronizace stavu (vlakno emulace)
  /** Hrac 1: ma pred timto snimkem poslat cely stav jadra? (zavolat pred vstupy() kazdeho snimku) */
  bool chciPoslatStav(uint32_t snimek) const {
    std::lock_guard<std::mutex> l(m_);
    return hrac1_ && stav_ == Stav::BEZI && resyncAktivni_ && !stavOdeslan_ && snimek == resyncG_;
  }
  /** Hrac 1: stav jadra pred snimkem (data zabalilo jadro). */
  void posliStav(uint32_t snimek, const std::vector<uint8_t> &data) {
    std::lock_guard<std::mutex> l(m_);
    if (!hrac1_ || stav_ != Stav::BEZI || !resyncAktivni_ || stavOdeslan_ || snimek != resyncG_) return;
    std::vector<uint8_t> z;
    z.reserve(5 + data.size());
    w8(z, STAV); w32(z, snimek);
    z.insert(z.end(), data.begin(), data.end());
    stavOdeslan_ = true;
    stavKB_ = (uint32_t)((data.size() + 1023) / 1024);
    posliLocked(z);
  }
  /**
   * Hrac 2 pred snimkem (zavolat pred vstupy()): 0 = nic (zadna synchronizace, nebo relace skoncila - vstupy() to
   * reknou), 1 = tady je stav pro tento snimek (nahrat do jadra a zavolat stavNahran), -1 = stav jeste neprisel
   * (cekal nejvys cekejMs) - emulator stoji.
   */
  int stavPro(uint32_t snimek, std::vector<uint8_t> &data, int cekejMs) {
    std::unique_lock<std::mutex> l(m_);
    if (hrac1_ || !resyncAktivni_ || snimek != resyncG_ || stav_ == Stav::KONEC) return 0;
    if (!maStav_) {
      if (cekaSnimek_ != snimek) { cekaSnimek_ = snimek; cekaOdUs_ = tedUs(); }
      if (cekejMs > 0) cv_.wait_for(l, std::chrono::milliseconds(cekejMs), [&] { return maStav_ || stav_ == Stav::KONEC; });
    }
    if (stav_ == Stav::KONEC) return 0;
    if (!maStav_) return -1;
    data.swap(stavData_);
    stavData_.clear();
    maStav_ = false;
    return 1;
  }
  /** Hrac 2: stav nahrany do jadra - hraje se dal (posle STAV_OK). */
  void stavNahran(uint32_t snimek) {
    std::lock_guard<std::mutex> l(m_);
    if (hrac1_ || !resyncAktivni_ || snimek != resyncG_) return;
    resyncAktivni_ = false;
    synchronizovano_++;
    if (cekaSnimek_ == snimek) cekaOdUs_ = 0;   // cekani na stav se nepocita jako cekani na stisky
    std::vector<uint8_t> z;
    w8(z, STAV_OK); w32(z, snimek);
    posliLocked(z);
    cv_.notify_all();
  }

  // ------------------------------------------------------------------ stav
  Stav stav() const { std::lock_guard<std::mutex> l(m_); return stav_; }
  bool bezi() const { std::lock_guard<std::mutex> l(m_); return stav_ == Stav::BEZI; }
  bool konec() const { std::lock_guard<std::mutex> l(m_); return stav_ == Stav::KONEC; }
  uint8_t duvod() const { std::lock_guard<std::mutex> l(m_); return duvod_; }
  bool duvodOdeMne() const { std::lock_guard<std::mutex> l(m_); return duvodOdeMne_; }
  /** B360: text konce (stejny jako zapise vlakno site) - "" dokud hra bezi. */
  std::string textKonce() const { std::lock_guard<std::mutex> l(m_); return stav_ == Stav::KONEC ? nap::online::textKonce(duvod_, duvodOdeMne_) : std::string(); }
  uint32_t zpozdeni() const { std::lock_guard<std::mutex> l(m_); return D_; }
  uint32_t zpozdeniTed() const { std::lock_guard<std::mutex> l(m_); return Dmuj_; }
  int odezvaMs() const { std::lock_guard<std::mutex> l(m_); return (int)(posledniRttUs_ / 1000); }
  uint32_t cekani() const { std::lock_guard<std::mutex> l(m_); return cekani_; }
  uint32_t cekaniMs() const { std::lock_guard<std::mutex> l(m_); return (uint32_t)(cekaniUs_ / 1000); }
  uint32_t cekaniMaxMs() const { std::lock_guard<std::mutex> l(m_); return (uint32_t)(cekaniMaxUs_ / 1000); }
  uint32_t dlouhychCekani() const { std::lock_guard<std::mutex> l(m_); return dlouhychCekani_; }
  uint32_t zadosti() const { std::lock_guard<std::mutex> l(m_); return zadosti_; }
  uint32_t zvyseni() const { std::lock_guard<std::mutex> l(m_); return zvyseni_; }
  uint32_t snizeni() const { std::lock_guard<std::mutex> l(m_); return snizeni_; }
  uint32_t overenychOtisku() const { std::lock_guard<std::mutex> l(m_); return overeno_; }
  uint32_t rozjetoVeSnimku() const { std::lock_guard<std::mutex> l(m_); return rozjetoSnimek_; }
  uint32_t rozjeti() const { std::lock_guard<std::mutex> l(m_); return rozjeti_; }
  uint32_t rozdilCasti() const { std::lock_guard<std::mutex> l(m_); return rozdilCasti_; }
  uint32_t synchronizaci() const { std::lock_guard<std::mutex> l(m_); return synchronizaci_; }
  uint32_t synchronizovano() const { std::lock_guard<std::mutex> l(m_); return synchronizovano_; }
  bool synchronizuje() const { std::lock_guard<std::mutex> l(m_); return resyncAktivni_; }
  uint32_t stavKB() const { std::lock_guard<std::mutex> l(m_); return stavKB_; }
  /** Jak dlouho emulator ted stoji (ceka na stisky druheho nebo na stav) - 0 = nestoji. */
  uint32_t stojiMs() const { std::lock_guard<std::mutex> l(m_); return cekaOdUs_ ? (uint32_t)((tedUs() - cekaOdUs_) / 1000) : 0; }
  /** Jak dlouho od druheho nic neprislo. */
  uint32_t tichoMs() const { std::lock_guard<std::mutex> l(m_); return posledniPrijemUs_ ? (uint32_t)((tedUs() - posledniPrijemUs_) / 1000) : 0; }
  std::string jmenoDruheho() const { std::lock_guard<std::mutex> l(m_); return jeho_ahoj_.jmeno; }
  bool mamAhojDruheho() const { std::lock_guard<std::mutex> l(m_); return mamAhoj_; }

 private:
  struct Polozka { uint32_t snimek = 0; uint32_t bity = 0; bool plne = false; };
  struct Otisk { uint32_t snimek = 0; uint64_t h = 0; bool je = false; uint8_t n = 0; uint64_t casti[CASTI_MAX] = {}; };

  void posliLocked(const std::vector<uint8_t> &z) {
    ven_.push_back(z);
    cvVen_.notify_all();
    if (budik_) budik_();
  }

  void posliAhojLocked() {
    std::vector<uint8_t> z;
    w8(z, AHOJ); w8(z, ja_.verze); w8(z, ja_.jadro);
    for (int i = 0; i < 20; i++) w8(z, ja_.hra[i]);
    w32(z, ja_.nastaveni);
    std::string j = ja_.jmeno.substr(0, 60);
    w8(z, (uint8_t)j.size());
    z.insert(z.end(), j.begin(), j.end());
    posliLocked(z);
  }

  void startLocked(uint64_t ted) {
    stav_ = Stav::BEZI;
    spotrebovano_ = 0;
    for (uint32_t i = 0; i < OKNO; i++) { muj_[i] = Polozka(); }
    // vstupy druheho, ktere prisly pred startem (hrac 2 zacina o pul odezvy driv nez hrac 1), nemazat
    dalsiPingUs_ = 0;
    Dmuj_ = D_;
    posledniPoslany_ = D_ - 1;    // snimky 0 .. D-1 maji vstup 0 u obou
    zvyseniUs_ = snizeniUs_ = ted;
  }

  void porovnejLocked(uint32_t s, uint64_t h, const uint64_t *casti, uint8_t nc, bool muj) {
    Otisk *a = muj ? mojeOtisky_ : jehoOtisky_;
    Otisk *b = muj ? jehoOtisky_ : mojeOtisky_;
    const uint32_t i = (s / HASH_KAZDYCH) % 8;
    a[i].snimek = s; a[i].h = h; a[i].je = true; a[i].n = nc;
    for (uint8_t k = 0; k < CASTI_MAX; k++) a[i].casti[k] = k < nc ? casti[k] : 0;
    if (!b[i].je || b[i].snimek != s) return;
    if (b[i].h == h) {
      overeno_++;
      // B360: stejny otisk po snimku G = synchronizace se povedla (STAV_OK prijde taky, tohle je pojistka)
      if (hrac1_ && resyncAktivni_ && stavOdeslan_ && s >= resyncG_) { resyncAktivni_ = false; synchronizovano_++; }
      return;
    }
    if (stav_ == Stav::KONEC || s < ignorujDo_) return;   // pred synchronizaci - stara data, uz se resi
    rozjetoSnimek_ = s;
    rozjeti_++;
    rozdilCasti_ = 0;
    const uint8_t n = std::min(a[i].n, b[i].n);
    for (uint8_t k = 0; k < n; k++) if (a[i].casti[k] != b[i].casti[k]) rozdilCasti_ |= 1u << k;
    if (hrac1_) zacniSynchronizaciLocked();
    else if (!rozjetoVidimUs_) rozjetoVidimUs_ = tedUs();
  }

  /** Hrac 1: hra se rozjela - RESYNC(G), stav pujde pred snimkem G (prvni, pro ktery jeste neodesel muj vstup). */
  void zacniSynchronizaciLocked() {
    if (resyncAktivni_) return;   // uz bezi - rozdily pred G se nepocitaji (ignorujDo_), po G prijde dalsi
    const uint64_t ted = tedUs();
    while (!synchronizaceCasy_.empty() && ted > synchronizaceCasy_.front() + ladeni_.synchronizaceOknoUs) synchronizaceCasy_.pop_front();
    if (synchronizaci_ >= ladeni_.maxSynchronizaci || synchronizaceCasy_.size() >= 2) { ukonciLocked(D_ROZJETO, true); return; }
    synchronizaceCasy_.push_back(ted);
    synchronizaci_++;
    resyncG_ = posledniPoslany_ + 1;
    resyncAktivni_ = true;
    stavOdeslan_ = false;
    ignorujDo_ = resyncG_;
    std::vector<uint8_t> z;
    w8(z, RESYNC); w32(z, resyncG_);
    posliLocked(z);
  }

  /**
   * Cekal jsem na stisky druheho cekaniUs (dele nez 2 snimky) - at si zvysi zpozdeni o tolik snimku, kolik chybelo
   * (o 1 min nez cele cekani - cast spolkne beh obou telefonu; nejvys zvysitNejvys), nejvys jednou za 2 s.
   */
  void pozadejOZpozdeniLocked(uint64_t cekaniUs) {
    const uint64_t ted = tedUs();
    if (zadostUs_ && ted < zadostUs_ + ladeni_.zadostKazdychUs) return;
    zadostUs_ = ted;
    zadosti_++;
    const double snimekUs = (snimekMs_ > 0 ? snimekMs_ : 16.7) * 1000.0;
    uint32_t o = (uint32_t)((double)cekaniUs / snimekUs);
    o = o > 1 ? o - 1 : 1;
    if (o > ladeni_.zvysitNejvys) o = ladeni_.zvysitNejvys;
    std::vector<uint8_t> z;
    w8(z, ZPOZDENI); w8(z, (uint8_t)o);
    posliLocked(z);
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
  const Ladeni ladeni_;
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
  // B360: zpozdeni tohoto telefonu za hry
  uint32_t Dmuj_ = 3, posledniPoslany_ = 2;
  uint64_t zadostUs_ = 0, zvyseniUs_ = 0, snizeniUs_ = 0;
  uint32_t zadosti_ = 0, zvyseni_ = 0, snizeni_ = 0;
  // B360: cekani na stisky druheho (a hrac 2 na stav)
  uint32_t cekaSnimek_ = UINT32_MAX;
  uint64_t cekaOdUs_ = 0, cekaniUs_ = 0, cekaniMaxUs_ = 0;
  uint32_t dlouhychCekani_ = 0;
  // B360: synchronizace stavu
  uint32_t resyncG_ = 0, ignorujDo_ = 0, synchronizaci_ = 0, synchronizovano_ = 0, rozjeti_ = 0, rozdilCasti_ = 0, stavKB_ = 0;
  bool resyncAktivni_ = false, stavOdeslan_ = false, maStav_ = false;
  std::vector<uint8_t> stavData_;
  std::deque<uint64_t> synchronizaceCasy_;
  uint64_t rozjetoVidimUs_ = 0, resyncOdUs_ = 0;
};

}  // namespace online
}  // namespace nap
