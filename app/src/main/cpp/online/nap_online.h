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
//  B362 (verze 3) - po tretim testu (Rene 24. 11.: "furt je ten online spatny ... testuju na mistni LAN"): oba
//  telefony cekaly na sebe navzajem (spicky site obema smery, odezva 7-123 ms), zpozdeni vyrostlo na 11 snimku.
//    ODHAD + OPRAVA (jako netplay bojovek, "rollback"): kdyz stisk druheho pro snimek jeste neprisel, jadro snimek
//      spocita s ODHADEM (druhy drzi, co drzel naposledy) a pred nim si ulozi stav; prijde-li skutecny stisk jiny,
//      jadro se vrati k ulozenemu stavu a snimky spocita znovu (zkontrolujOdhady / vstupyOdhad / opraveno).
//      Nejvys ODHAD_MAX snimku napred (jadro si limit snizi, kdyz je telefon pomaly), pak se ceka jako dosud.
//      Otisky stavu jdou jen ze snimku spocitanych se skutecnymi stisky (odhadnute se odeslou, az se potvrdi).
//    STATISTIKA text  kazdych 10 s druhemu telefonu (cekani, odhady, opravy, vykon, Wi-Fi) - log jednoho telefonu
//      ukaze oba.
//  Vsechna cisla little-endian. Bez Androidu a bez site - test test_overeni/b357 (dva emulatory, simulovana
//  sit se zpozdenim a kolisanim), B362: test_overeni/b362 (spicky site, odhad a oprava).
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

constexpr uint8_t VERZE = 3;   // B362: odhad stisku druheho + oprava, STATISTIKA (B360: zpozdeni kazdeho telefonu, synchronizace)

enum Typ : uint8_t { AHOJ = 1, PING = 2, PONG = 3, START = 4, VSTUP = 5, HASH = 6, KONEC = 7, ZPOZDENI = 8, RESYNC = 9, STAV = 10, STAV_OK = 11,
                     STATISTIKA = 12 };
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
constexpr uint32_t ODHAD_MAX = 8;           // B362: nejvys tolik snimku napred s odhadem stisku druheho (133 ms pri 60 Hz)
constexpr uint32_t ODHAD_OPRAVA = 0xFFFFFFFFu;   // B362: vstupyOdhad pri oprave - snimky uz jednou spocitane, odhad bez limitu
constexpr size_t STATISTIKA_MAX = 480;      // B362: text statistiky nejvys tolik znaku

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
  uint64_t pingKazdychUs = 1000000;              // B362: odezva za hry kazdou sekundu (drive 2 s)
  uint64_t statistikaPrvniUs = 3000000;          // B362: prvni STATISTIKA 3 s po startu
  uint64_t statistikaKazdychUs = 10000000;       // B362: pak kazdych 10 s
  bool odhad = false;                            // B362: jadro umi odhad + opravu -> START zpozdeni bez rezervy na kolisani
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

/**
 * B362: zpozdeni vstupu pro jadro s odhadem a opravou: jen jednosmerna cesta + 4 ms, v celych snimcich, + 1. Kolisani
 * site (spicky) pokryje odhad - zpozdeni ovladani zustane male (LAN 2 snimky, internet 3-4).
 */
inline uint32_t zpozdeniSOdhadem(int rttMs, double snimekMs) {
  if (snimekMs <= 0) snimekMs = 16.7;
  const double jednosmerne = rttMs / 2.0 + 4.0;
  uint32_t d = (uint32_t)(jednosmerne / snimekMs + 0.999) + 1;
  if (d < ZPOZDENI_MIN) d = ZPOZDENI_MIN;
  if (d > ZPOZDENI_MAX) d = ZPOZDENI_MAX;
  return d;
}

/** B362: text od druheho telefonu do logu - jen tisknutelne ASCII, nejvys max znaku. */
inline std::string cistyText(const uint8_t *d, size_t n, size_t max) {
  std::string r;
  for (size_t i = 0; i < n && r.size() < max; i++) r += (d[i] >= 32 && d[i] < 127) ? (char)d[i] : '?';
  return r;
}

/** B362: nazev zpravy do logu (co od druheho prislo naposledy). */
inline const char *nazevZpravy(uint8_t t) {
  switch (t) {
    case AHOJ: return "AHOJ";
    case PING: return "PING";
    case PONG: return "PONG";
    case START: return "START";
    case VSTUP: return "VSTUP";
    case HASH: return "OTISK";
    case KONEC: return "KONEC";
    case ZPOZDENI: return "ZPOZDENI";
    case RESYNC: return "RESYNC";
    case STAV: return "STAV";
    case STAV_OK: return "STAV_OK";
    case STATISTIKA: return "STATISTIKA";
    default: return "-";
  }
}

struct Ahoj {
  uint8_t verze = VERZE;
  uint8_t jadro = 0;
  uint8_t hra[20] = {};      // SHA-1 obsahu hry (ROM, XEX, CUE+BIN ...)
  uint32_t nastaveni = 0;    // otisk nastaveni jadra, ktere meni chovani (region, stroj ...)
  std::string jmeno;         // jmeno hrace (jen pro zobrazeni)
};

// =====================================================================================================
//  Relace - jedna online hra. Vlakno emulace: lokalni(), zkontrolujOdhady() + opraveno(), stav (chciPoslatStav /
//  posliStav, stavPro / stavNahran), vstupyOdhad() / vstupy(), otiskStavu(). Vlakno site: prijmi(), vyzvedni() /
//  cekejNaOdchozi(), tik(). Vse pod jednim zamkem.
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
//
//  B362 - ODHAD + OPRAVA: jehoDo_ = prvni snimek, pro ktery stisk druheho jeste neprisel (TCP drzi poradi, takze
//  vsechny pred nim uz jsou). Snimek >= jehoDo_ (nejvys maxOdhad napred) se smi spocitat s odhadem = posledni znamy
//  stisk druheho (odhad_ si ho pamatuje). jistoDo_ = snimky pred nim jsou spocitane se skutecnymi stisky obou.
//  Pred kazdym snimkem jadro zavola zkontrolujOdhady(): odhady, pro ktere uz prisel skutecny stisk, se porovnaji -
//  prvni, ktery nesedi, vrati (jadro nahraje stav ulozeny pred nim a snimky do ted spocita znovu: vstupyOdhad s
//  ODHAD_OPRAVA, otiskStavu). Otisk snimku spocitaneho s odhadem se drzi (cekajici_) a odesle se, az jistoDo_ za
//  nim projde bez opravy; oprava ho zahodi (pocita se znovu). Synchronizace: hrac 1 posle stav pred G, az jsou
//  vsechny snimky pred G overene; hrac 2 mohl s odhadem dojit za G - stav G ho vrati zpet (stavPro vrati G).
// =====================================================================================================
class Relace {
 public:
  Relace(bool hrac1, const Ahoj &ja, double snimekMs, const Ladeni &ladeni = Ladeni())
      : hrac1_(hrac1), ja_(ja), snimekMs_(snimekMs), ladeni_(ladeni) {
    for (uint32_t i = 0; i < OKNO; i++) { muj_[i] = jeho_[i] = odhad_[i] = Polozka(); }
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
   * B362: STATISTIKA druhemu telefonu (3 s po startu, pak kazdych 10 s).
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
        // B362: jadro s odhadem a opravou kolisani spolkne samo - zpozdeni jen na jednosmernou cestu
        D_ = ladeni_.odhad ? zpozdeniSOdhadem(rtt, snimekMs_) : zpozdeniZOdezvy(rtt, jitter, snimekMs_);
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
      if (ted >= dalsiPingUs_) {   // odezva i za hry (zobrazeni, log)
        std::vector<uint8_t> z;
        w8(z, PING); w64(z, ted);
        posliLocked(z);
        dalsiPingUs_ = ted + ladeni_.pingKazdychUs;
      }
      // B362: statistika druhemu telefonu (jeho log ukaze i tento telefon)
      if (!dalsiStatistikaUs_) dalsiStatistikaUs_ = ted + ladeni_.statistikaPrvniUs;
      else if (ted >= dalsiStatistikaUs_) {
        posliStatistikuLocked();
        dalsiStatistikaUs_ = ted + ladeni_.statistikaKazdychUs;
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
    posledniTyp_ = d[0];
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
          if (s < jehoDo_) continue;                                        // uz ho mam (B362: drive "uz emulovany")
          if (s >= jistoDo_ + OKNO - 1) return chybaLocked();               // moc napred - nemozne (D + odhad)
          Polozka &p = jeho_[s % OKNO];
          p.snimek = s; p.bity = r32(d + 6 + 4 * i); p.plne = true;
        }
        if (stav_ == Stav::BEZI) posunJehoDoLocked();                       // pred startem: posune startLocked()
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
        // G uz je spocitany se skutecnymi stisky - nemozne (RESYNC jde pred vstupem pro G). B362: s odhadem smel
        // hrac 2 G uz spocitat (odhadem) - stav G ho vrati zpet
        if (g < jistoDo_) return chybaLocked();
        resyncG_ = g;
        resyncAktivni_ = true;
        maStav_ = false;
        stavData_.clear();
        if (g > ignorujDo_) ignorujDo_ = g;
        rozjetoVidimUs_ = 0;
        resyncOdUs_ = posledniPrijemUs_;
        synchronizaci_++;
        cv_.notify_all();
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
      case STATISTIKA: { // B362: druhy telefon o sobe (cekani, odhady, vykon, Wi-Fi) - jen do logu
        jehoStatistika_ = cistyText(d + 1, n - 1, STATISTIKA_MAX);
        jehoStatistik_++;
        jehoStatistikaMujSnimek_ = spotrebovano_;
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
   * B362: pred kazdym snimkem (vlakno emulace, PRED vstupyOdhad pro 'snimek'): snimky spocitane s odhadem, pro
   * ktere uz prisel skutecny stisk druheho, porovna s odhadem. Vrati prvni, kde odhad NESEDEL - jadro nahraje stav
   * ulozeny pred nim a snimky od nej do 'snimek' spocita znovu (vstupyOdhad s ODHAD_OPRAVA, otiskStavu, pak
   * opraveno). UINT32_MAX = vse sedi (nebo neni co overit). Odhady, ktere sedely, se potvrdi i s cekajicimi otisky.
   */
  uint32_t zkontrolujOdhady(uint32_t snimek) {
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::BEZI) return UINT32_MAX;
    if (!hrac1_ && resyncAktivni_) return UINT32_MAX;   // stav od hrace 1 stejne prepise vse od G
    const uint32_t konec = std::min(jehoDo_, snimek);
    for (uint32_t f = jistoDo_; f < konec; f++) {
      Polozka &o = odhad_[f % OKNO];
      if (!(o.plne && o.snimek == f)) continue;        // spocitany se skutecnym stiskem
      o.plne = false;
      if (o.bity != jeho_[f % OKNO].bity) {
        jistoDo_ = f;
        zahodCekajiciOtiskyLocked(f);                   // otisky od f se spocitaji znovu
        return f;
      }
      odhaduSedelo_++;
    }
    if (konec > jistoDo_) jistoDo_ = konec;
    potvrdCekajiciOtiskyLocked();
    return UINT32_MAX;
  }

  /**
   * Vstupy obou hracu pro snimek (hrac 1, hrac 2). Ceka nejvys cekejMs na druheho hrace. B362: kdyz jeho stisk
   * jeste neprisel a snimek je nejvys maxOdhad snimku za poslednim znamym, vrati ODHAD (posledni znamy stisk
   * druheho) a odhad = true - jadro si PRED snimkem ulozi stav (pro opravu). maxOdhad 0 = bez odhadu (lockstep),
   * ODHAD_OPRAVA = oprava (snimky uz jednou spocitane - bez limitu, bez cekani).
   * @return 1 = mam, 0 = jeste ne (cekat dal / emulator stoji), -1 = relace skoncila
   */
  int vstupyOdhad(uint32_t snimek, uint32_t &p1, uint32_t &p2, bool &odhad, int cekejMs, uint32_t maxOdhad) {
    std::unique_lock<std::mutex> l(m_);
    odhad = false;
    // B360: hrac 1 je za snimkem G a stav neposlal (jadro synchronizaci neumi) - hra se rozjela a nespravi se.
    // B362: snimek G se bez odeslaneho stavu nespocita (blokovano) - jadro, ktere stav neposlalo, i kdyz uz mohlo
    // (snimky pred G overene, chciPoslatStav vratil true), synchronizaci neumi -> konec hned u G
    if (hrac1_ && stav_ == Stav::BEZI && resyncAktivni_ && !stavOdeslan_ && maxOdhad != ODHAD_OPRAVA &&
        (snimek > resyncG_ || (snimek == resyncG_ && jistoDo_ >= resyncG_)))
      ukonciLocked(D_ROZJETO, true);
    // synchronizace: snimek G a dal se ted nesmi spocitat (hrac 2: nejdriv stav od hrace 1; B362 hrac 1: nejdriv
    // stav odeslat - az budou snimky pred G overene)
    auto blokovano = [&]() { return resyncAktivni_ && snimek >= resyncG_ && (hrac1_ ? !stavOdeslan_ : true); };
    auto mam = [&]() {
      if (stav_ == Stav::KONEC) return true;
      if (blokovano()) return false;
      if (snimek < D_) return true;
      const Polozka &a = muj_[snimek % OKNO], &b = jeho_[snimek % OKNO];
      return a.plne && a.snimek == snimek && b.plne && b.snimek == snimek;
    };
    auto mohuOdhadnout = [&]() {
      if (stav_ != Stav::BEZI || maxOdhad == 0 || blokovano() || snimek < D_ || snimek < jehoDo_) return false;
      const Polozka &a = muj_[snimek % OKNO];
      if (!(a.plne && a.snimek == snimek)) return false;   // vlastni stisk musi byt (lokalni() ho poslal)
      return maxOdhad == ODHAD_OPRAVA || snimek - jehoDo_ < maxOdhad;
    };
    if (!mam() && !mohuOdhadnout()) {
      if (cekaSnimek_ != snimek) { cekaSnimek_ = snimek; cekaOdUs_ = tedUs(); cekani_++; }
      if (cekejMs > 0) cv_.wait_for(l, std::chrono::milliseconds(cekejMs), [&] { return mam() || mohuOdhadnout(); });
    }
    if (stav_ == Stav::KONEC) return -1;
    const bool skutecne = mam();
    if (!skutecne && !mohuOdhadnout()) return 0;
    if (cekaSnimek_ == snimek && cekaOdUs_) {   // B360: jak dlouho se na tento snimek cekalo
      const uint64_t t = tedUs() - cekaOdUs_;
      cekaniUs_ += t;
      if (t > cekaniMaxUs_) cekaniMaxUs_ = t;
      if (t >= ladeni_.dlouheCekaniUs) { dlouhychCekani_++; pozadejOZpozdeniLocked(t); }
      cekaOdUs_ = 0;
    }
    uint32_t moje = 0, jeho = 0;
    if (snimek >= D_) {
      moje = muj_[snimek % OKNO].bity;
      Polozka &o = odhad_[snimek % OKNO];
      if (skutecne) {
        jeho = jeho_[snimek % OKNO].bity;
        if (o.snimek == snimek) o.plne = false;      // B362: snimek se ted pocita se skutecnym stiskem (oprava)
      } else {
        jeho = jehoDo_ > D_ ? jeho_[(jehoDo_ - 1) % OKNO].bity : 0;   // B362: odhad - druhy drzi, co drzel naposledy
        o.snimek = snimek; o.bity = jeho; o.plne = true;
        odhad = true;
        if (maxOdhad != ODHAD_OPRAVA) odhadnuto_++;
      }
    }
    if (skutecne && jistoDo_ == snimek) jistoDo_ = snimek + 1;   // B362: vse do snimku vcetne se skutecnymi stisky
    p1 = hrac1_ ? moje : jeho;
    p2 = hrac1_ ? jeho : moje;
    if (snimek + 1 > spotrebovano_) spotrebovano_ = snimek + 1;
    return 1;
  }

  /** Lockstep (bez odhadu): vstupy obou hracu pro snimek, ceka nejvys cekejMs. 1 / 0 / -1 jako vstupyOdhad. */
  int vstupy(uint32_t snimek, uint32_t &p1, uint32_t &p2, int cekejMs) {
    bool odhad = false;
    return vstupyOdhad(snimek, p1, p2, odhad, cekejMs, 0);
  }

  /** B362: jadro opravilo snimky od .. po-1 (nahralo stav a spocitalo je znovu, trvalo us) - pocty do logu. */
  void opraveno(uint32_t od, uint32_t po, uint64_t us) {
    std::lock_guard<std::mutex> l(m_);
    if (po <= od) return;
    oprav_++;
    prepocteno_ += po - od;
    if (po - od > opravaMax_) opravaMax_ = po - od;
    opravyUs_ += us;
    if (us > opravaMaxUs_) opravaMaxUs_ = us;
  }

  /**
   * Otisk stavu PO emulaci snimku (jen kazdych HASH_KAZDYCH snimku se posila a porovnava). B360: casti (RAM 68000,
   * RAM Z80 ...) jdou s nim - pri rozjeti je v logu videt, co se lisi. B362: snimek spocitany s odhadem (jistoDo_
   * za nim jeste neni) se odesle, az se odhad potvrdi; oprava ho zahodi a jadro zavola otiskStavu znovu.
   */
  void otiskStavu(uint32_t snimek, uint64_t h, const uint64_t *casti = nullptr, uint8_t nCasti = 0) {
    if (snimek % HASH_KAZDYCH != 0) return;
    if (!casti) nCasti = 0;
    if (nCasti > CASTI_MAX) nCasti = CASTI_MAX;
    std::lock_guard<std::mutex> l(m_);
    if (stav_ != Stav::BEZI) return;
    if (snimek < jistoDo_) { posliOtiskLocked(snimek, h, casti, nCasti); return; }
    Otisk &o = cekajici_[(snimek / HASH_KAZDYCH) % 8];
    o.snimek = snimek; o.h = h; o.je = true; o.n = nCasti;
    for (uint8_t k = 0; k < CASTI_MAX; k++) o.casti[k] = k < nCasti ? casti[k] : 0;
  }

  // ------------------------------------------------------------------ B360: synchronizace stavu (vlakno emulace)
  /** Hrac 1: ma pred timto snimkem poslat cely stav jadra? (zavolat pred vstupy() kazdeho snimku) B362: az jsou
   *  snimky pred G overene (stav G nesmi byt spocitany z odhadu). */
  bool chciPoslatStav(uint32_t snimek) const {
    std::lock_guard<std::mutex> l(m_);
    return hrac1_ && stav_ == Stav::BEZI && resyncAktivni_ && !stavOdeslan_ && snimek == resyncG_ && jistoDo_ >= snimek;
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
    cv_.notify_all();
  }
  /**
   * Hrac 2 pred snimkem (zavolat pred vstupy()): 0 = nic (zadna synchronizace, nebo relace skoncila - vstupy() to
   * reknou), 1 = tady je stav pro snimek g (nahrat do jadra, pokracovat OD g a zavolat stavNahran(g)), -1 = stav
   * jeste neprisel (cekal nejvys cekejMs) - emulator stoji. B362: snimek muze byt i za G (hrac 2 s odhadem dosel
   * dal, nez prisel RESYNC) - jadro se vrati na g.
   */
  int stavPro(uint32_t snimek, std::vector<uint8_t> &data, uint32_t &g, int cekejMs) {
    std::unique_lock<std::mutex> l(m_);
    if (hrac1_ || !resyncAktivni_ || snimek < resyncG_ || stav_ == Stav::KONEC) return 0;
    g = resyncG_;
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
  int stavPro(uint32_t snimek, std::vector<uint8_t> &data, int cekejMs) {
    uint32_t g = 0;
    return stavPro(snimek, data, g, cekejMs);
  }
  /** Hrac 2: stav nahrany do jadra pred snimkem g - hraje se dal od g (posle STAV_OK). */
  void stavNahran(uint32_t snimek) {
    std::lock_guard<std::mutex> l(m_);
    if (hrac1_ || !resyncAktivni_ || snimek != resyncG_) return;
    resyncAktivni_ = false;
    synchronizovano_++;
    if (cekaSnimek_ == snimek) cekaOdUs_ = 0;   // cekani na stav se nepocita jako cekani na stisky
    cekaSnimek_ = UINT32_MAX;                  // B362 (z B360): dalsi cekani na snimek G se zase meri
    // B362: od G se pocita znovu z nahraneho stavu - odhady a cekajici otisky pred tim neplati
    jistoDo_ = snimek;
    if (spotrebovano_ > snimek) spotrebovano_ = snimek;
    for (uint32_t i = 0; i < OKNO; i++) odhad_[i].plne = false;
    zahodCekajiciOtiskyLocked(0);
    std::vector<uint8_t> z;
    w8(z, STAV_OK); w32(z, snimek);
    posliLocked(z);
    cv_.notify_all();
  }

  // ------------------------------------------------------------------ B362: info do statistiky (jakekoli vlakno)
  /** Telefon o sobe (model, Android, Wi-Fi, baterie - pise Java) - jde druhemu ve STATISTIKA. */
  void infoTelefonu(const std::string &s) { std::lock_guard<std::mutex> l(m_); infoTelefonu_ = s.substr(0, 200); }
  /** Vykon jadra ("59.9 snimku/s, jadro 7.6 ms ...") - jde druhemu ve STATISTIKA. */
  void vykon(const std::string &s) { std::lock_guard<std::mutex> l(m_); vykon_ = s.substr(0, 160); }

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
  // B362: odhad a oprava
  uint32_t odhadnuto() const { std::lock_guard<std::mutex> l(m_); return odhadnuto_; }
  uint32_t odhaduSedelo() const { std::lock_guard<std::mutex> l(m_); return odhaduSedelo_; }
  uint32_t oprav() const { std::lock_guard<std::mutex> l(m_); return oprav_; }
  uint32_t prepocteno() const { std::lock_guard<std::mutex> l(m_); return prepocteno_; }
  uint32_t opravaMax() const { std::lock_guard<std::mutex> l(m_); return opravaMax_; }
  uint32_t opravyMs() const { std::lock_guard<std::mutex> l(m_); return (uint32_t)(opravyUs_ / 1000); }
  uint32_t opravaMaxMs() const { std::lock_guard<std::mutex> l(m_); return (uint32_t)(opravaMaxUs_ / 1000); }
  uint32_t jistoDo() const { std::lock_guard<std::mutex> l(m_); return jistoDo_; }
  uint32_t jehoDo() const { std::lock_guard<std::mutex> l(m_); return jehoDo_; }
  // B362: statistika druheho telefonu a posledni zprava od nej (log)
  std::string statistikaDruheho() const { std::lock_guard<std::mutex> l(m_); return jehoStatistika_; }
  uint32_t statistikDruheho() const { std::lock_guard<std::mutex> l(m_); return jehoStatistik_; }
  uint32_t mujSnimekPriStatistice() const { std::lock_guard<std::mutex> l(m_); return jehoStatistikaMujSnimek_; }
  const char *posledniZprava() const { std::lock_guard<std::mutex> l(m_); return nazevZpravy(posledniTyp_); }
  /** Text, ktery by ted sel druhemu ve STATISTIKA (test, log). */
  std::string statistika() const { std::lock_guard<std::mutex> l(m_); return statistikaTextLocked(); }

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
    for (uint32_t i = 0; i < OKNO; i++) { muj_[i] = Polozka(); odhad_[i] = Polozka(); }
    // vstupy druheho, ktere prisly pred startem (hrac 2 zacina o pul odezvy driv nez hrac 1), nemazat
    dalsiPingUs_ = 0;
    Dmuj_ = D_;
    posledniPoslany_ = D_ - 1;    // snimky 0 .. D-1 maji vstup 0 u obou
    zvyseniUs_ = snizeniUs_ = ted;
    // B362: snimky 0 .. D-1 maji stisk druheho (0) - prvni, ktery chybi, je D (nebo dal, kdyz uz prisly)
    jistoDo_ = 0;
    jehoDo_ = D_;
    posunJehoDoLocked();
  }

  /** B362: jehoDo_ = prvni snimek bez stisku druheho (stisky chodi poporade - TCP). */
  void posunJehoDoLocked() {
    for (uint32_t i = 0; i < OKNO; i++) {
      const Polozka &p = jeho_[jehoDo_ % OKNO];
      if (!(p.plne && p.snimek == jehoDo_)) break;
      jehoDo_++;
    }
  }

  /** Otisk ze snimku spocitaneho se skutecnymi stisky: druhemu + porovnat s jeho. */
  void posliOtiskLocked(uint32_t snimek, uint64_t h, const uint64_t *casti, uint8_t nCasti) {
    std::vector<uint8_t> z;
    w8(z, HASH); w32(z, snimek); w64(z, h);
    w8(z, nCasti);
    for (uint8_t i = 0; i < nCasti; i++) w64(z, casti[i]);
    posliLocked(z);
    uint64_t c[CASTI_MAX] = {};
    for (uint8_t i = 0; i < nCasti; i++) c[i] = casti[i];
    porovnejLocked(snimek, h, c, nCasti, true);
  }

  /** B362: cekajici otisky snimku pred jistoDo_ (odhady sedely) odeslat. */
  void potvrdCekajiciOtiskyLocked() {
    for (Otisk &o : cekajici_) {
      if (!o.je || o.snimek >= jistoDo_) continue;
      o.je = false;
      posliOtiskLocked(o.snimek, o.h, o.casti, o.n);
    }
  }
  /** B362: cekajici otisky od snimku dal zahodit (oprava je spocita znovu). */
  void zahodCekajiciOtiskyLocked(uint32_t od) {
    for (Otisk &o : cekajici_) if (o.je && o.snimek >= od) o.je = false;
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

  /** B362: text pro druhy telefon (jeho log): cekani, odhady, opravy, zpozdeni, odezva, vykon jadra, telefon. */
  std::string statistikaTextLocked() const {
    char b[360];
    std::snprintf(b, sizeof(b),
                  "snimek %u, cekal %ux (%u ms, nejdel %u ms), odhad %u snimku (sedelo %u), oprav %u (nejdel %u snimku / %u ms, "
                  "celkem %u ms), zpozdeni %u, odezva %u ms",
                  spotrebovano_, cekani_, (unsigned)(cekaniUs_ / 1000), (unsigned)(cekaniMaxUs_ / 1000), odhadnuto_, odhaduSedelo_,
                  oprav_, opravaMax_, (unsigned)(opravaMaxUs_ / 1000), (unsigned)(opravyUs_ / 1000), Dmuj_, (unsigned)(posledniRttUs_ / 1000));
    std::string s = b;
    if (!vykon_.empty()) s += ", " + vykon_;
    if (!infoTelefonu_.empty()) s += " | " + infoTelefonu_;
    if (s.size() > STATISTIKA_MAX) s.resize(STATISTIKA_MAX);
    return s;
  }
  void posliStatistikuLocked() {
    const std::string t = statistikaTextLocked();
    std::vector<uint8_t> z;
    w8(z, STATISTIKA);
    z.insert(z.end(), t.begin(), t.end());
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
      if (stav_ == Stav::BEZI) posliStatistikuLocked();   // B362: posledni cisla do logu druheho (kdyz jeste prijdou)
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
  // B362: odhad stisku druheho a oprava
  Polozka odhad_[OKNO];
  uint32_t jehoDo_ = 0, jistoDo_ = 0;
  uint32_t odhadnuto_ = 0, odhaduSedelo_ = 0, oprav_ = 0, prepocteno_ = 0, opravaMax_ = 0;
  uint64_t opravyUs_ = 0, opravaMaxUs_ = 0;
  Otisk cekajici_[8];
  // B362: statistika (text od druheho telefonu, posledni zprava)
  std::string infoTelefonu_, vykon_, jehoStatistika_;
  uint32_t jehoStatistik_ = 0, jehoStatistikaMujSnimek_ = 0;
  uint64_t dalsiStatistikaUs_ = 0;
  uint8_t posledniTyp_ = 0;
};

}  // namespace online
}  // namespace nap
