// app/src/main/cpp/online/nap_online_klient.h
// =====================================================================================================
//  ONLINE HRANI (B357) - KLIENT: spojeni s enginem N&P (WebSocket) + relace (nap_online.h) ve vlastnim vlakne
// =====================================================================================================
//  Pouziva ho kazde jadro (Sega, pozdeji Atari a PS1) stejne:
//    start(nastaveni)      Java da adresu enginu, cestu (/api/v1/online/zaloz?... nebo /pripoj?kod=...) a hlavicky
//                          s podpisem telefonu; vlakno site se pripoji.
//    faze()                PRIPOJUJE -> CEKA_NA_HRACE (hrac 1: engine dal cislo mistnosti) -> HRAJE -> KONEC
//    vezmiStart()          vlakno emulace: true jednou, kdyz ma hra zacit od snimku 0 (oba telefony soubezne)
//    relace()              vstupy / otisky za hry (nap_online.h)
//    stop()                konec (posle KONEC, zavre spojeni)
//  Zpravy enginu (text JSON): {"typ":"mistnost","kod":"1234"}, {"typ":"hrac","jmeno":"..."}, {"typ":"odesel"},
//  {"typ":"chyba","zprava":"..."}. Binarni zpravy = relace (od druheho telefonu, engine je neotevira).
//  Ciste C++ - test test_overeni/b357 (skutecny klient proti NapOnline enginu v mono).
// =====================================================================================================
#pragma once

#include "nap_online.h"
#include "nap_websocket.h"

#include <atomic>
#include <memory>
#include <thread>

namespace nap {
namespace online {

/** Hodnota retezce "klic":"..." z jednoduche JSON zpravy enginu (\" \\ \n \uXXXX -> UTF-8). "" = neni. */
inline std::string jsonText(const std::string &j, const std::string &klic) {
  const std::string k = "\"" + klic + "\"";
  size_t i = j.find(k);
  if (i == std::string::npos) return "";
  i = j.find(':', i + k.size());
  if (i == std::string::npos) return "";
  i++;
  while (i < j.size() && (j[i] == ' ' || j[i] == '\t')) i++;
  if (i >= j.size()) return "";
  if (j[i] != '"') {   // cislo / true / false
    size_t e = i;
    while (e < j.size() && j[e] != ',' && j[e] != '}' && j[e] != ' ') e++;
    return j.substr(i, e - i);
  }
  std::string r;
  for (size_t p = i + 1; p < j.size(); p++) {
    char c = j[p];
    if (c == '"') return r;
    if (c != '\\' || p + 1 >= j.size()) { r += c; continue; }
    c = j[++p];
    if (c == 'n') r += '\n';
    else if (c == 't') r += '\t';
    else if (c == 'r') r += '\r';
    else if (c == 'u' && p + 4 < j.size()) {
      unsigned u = (unsigned)std::strtoul(j.substr(p + 1, 4).c_str(), nullptr, 16);
      p += 4;
      if (u >= 0xD800 && u < 0xDC00 && p + 6 < j.size() && j[p + 1] == '\\' && j[p + 2] == 'u') {
        const unsigned lo = (unsigned)std::strtoul(j.substr(p + 3, 4).c_str(), nullptr, 16);
        if (lo >= 0xDC00 && lo < 0xE000) { u = 0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00); p += 6; }
      }
      if (u < 0x80) r += (char)u;
      else if (u < 0x800) { r += (char)(0xC0 | (u >> 6)); r += (char)(0x80 | (u & 63)); }
      else if (u < 0x10000) { r += (char)(0xE0 | (u >> 12)); r += (char)(0x80 | ((u >> 6) & 63)); r += (char)(0x80 | (u & 63)); }
      else { r += (char)(0xF0 | (u >> 18)); r += (char)(0x80 | ((u >> 12) & 63)); r += (char)(0x80 | ((u >> 6) & 63)); r += (char)(0x80 | (u & 63)); }
    } else r += c;
  }
  return r;
}

class OnlineKlient {
 public:
  enum class Faze { NIC, PRIPOJUJE, CEKA_NA_HRACE, HRAJE, KONEC };

  struct Nastaveni {
    std::string adresa;                    // "http://server:port"
    std::string cesta;                     // "/api/v1/online/zaloz?..." / "/api/v1/online/pripoj?kod=..."
    std::vector<std::string> hlavicky;     // podpis telefonu (z Javy)
    bool hrac1 = true;                     // zaklada = hrac 1
    Ahoj ja;                               // verze, jadro, otisk hry, nastaveni, jmeno
    double snimekMs = 1000.0 / 60.0;
    uint64_t tichoUs = 15000000ULL;
  };

  OnlineKlient() = default;
  ~OnlineKlient() { stop(); }

  bool start(const Nastaveni &n) {
    stop();
    n_ = n;
    {
      std::lock_guard<std::mutex> l(m_);
      faze_ = Faze::PRIPOJUJE;
      chyba_.clear(); kod_.clear(); jmeno_.clear(); infoHry_.clear();
      relace_.reset();
    }
    start_ = false;
    bezi_ = true;
    ws_.obnov();   // stare zrus() (konec minule hry) neplati
    vlakno_ = std::thread([this] { smycka(); });
    return true;
  }

  /** Hrac konci (nebo odchod ze hry): KONEC druhemu, zavrit, pockat na vlakno. */
  void stop() {
    if (!vlakno_.joinable()) return;
    {
      std::lock_guard<std::mutex> l(m_);
      if (relace_) relace_->ukonci(D_HRAC);
    }
    konecPozadovan_ = true;
    ws_.zrus();    // prerusi i spojovani (spoj) - hrac nececka na timeout
    vlakno_.join();
    konecPozadovan_ = false;
    bezi_ = false;
  }

  Faze faze() const { std::lock_guard<std::mutex> l(m_); return faze_; }
  std::string kodMistnosti() const { std::lock_guard<std::mutex> l(m_); return kod_; }
  std::string jmenoDruheho() const { std::lock_guard<std::mutex> l(m_); return jmeno_; }
  std::string chyba() const { std::lock_guard<std::mutex> l(m_); return chyba_; }
  /** Zprava enginu pro hrace 2: {"typ":"hrac",...,"system","hra","hra_id","otisk"} */
  std::string infoHry() const { std::lock_guard<std::mutex> l(m_); return infoHry_; }

  /** Relace (jen ve fazi HRAJE a KONEC). shared_ptr: vlakno emulace ji drzi, i kdyz Java mezitim zavola start(). */
  std::shared_ptr<Relace> relace() { std::lock_guard<std::mutex> l(m_); return relace_; }

  /** Vlakno emulace: true jednou - hra ma zacit od snimku 0 (reset jadra). */
  bool vezmiStart() { return start_.exchange(false); }

  /** Text pro hrace (stav online hry). */
  std::string popis() const {
    std::lock_guard<std::mutex> l(m_);
    switch (faze_) {
      case Faze::PRIPOJUJE: return "PRIPOJUJI SE K ENGINU...";
      case Faze::CEKA_NA_HRACE: return "CEKAM NA DRUHEHO HRACE - CISLO HRY " + kod_;
      case Faze::HRAJE: {
        if (!relace_) return "ONLINE";
        if (!relace_->bezi()) return "ONLINE: DOMLOUVAM SE S " + jmeno_ + "...";
        return "ONLINE S " + jmeno_ + " - ODEZVA " + std::to_string(relace_->odezvaMs()) + " MS";
      }
      case Faze::KONEC: return "ONLINE KONEC: " + chyba_;
      default: return "";
    }
  }

 private:
  void nastavChybu(const std::string &c) { std::lock_guard<std::mutex> l(m_); if (chyba_.empty()) chyba_ = c; }

  void smycka() {
    std::string chyba;
    int kod = 0;
    if (!ws_.spoj(n_.adresa, n_.cesta, n_.hlavicky, 10000, chyba, &kod)) {
      std::lock_guard<std::mutex> l(m_);
      chyba_ = ws_.zruseno() ? "hra zrusena" : chyba;
      faze_ = Faze::KONEC;
      return;
    }
    {
      std::lock_guard<std::mutex> l(m_);
      faze_ = n_.hrac1 ? Faze::CEKA_NA_HRACE : Faze::PRIPOJUJE;
    }
    std::vector<uint8_t> z;
    uint8_t op = 0;
    Relace *r = nullptr;
    while (true) {
      if (r) for (auto &v : r->vyzvedni()) ws_.posli(v);
      if (r && r->konec()) {
        for (auto &v : r->vyzvedni()) ws_.posli(v);
        // hrac tohoto telefonu hru ukoncil sam (stop) - ne "druhy hrac hru ukoncil"; B360: stejny text jako
        // Relace::textKonce, ktery cte vlakno emulace, kdyz konec pozna driv nez tohle vlakno
        nastavChybu(textKonce(r->duvod(), r->duvodOdeMne()));
        break;
      }
      if (konecPozadovan_ && !r) { nastavChybu("hra zrusena"); break; }
      const int p = ws_.prijmi(z, r ? 20 : 100, &op);
      if (p < 0) {
        if (r) r->ztrataSpojeni();
        nastavChybu(r ? popisDuvodu(D_SIT) : "engine spojeni zavrel");
        break;
      }
      if (p == 1 && op == 1) {   // text = zprava enginu
        const std::string j(z.begin(), z.end());
        const std::string typ = jsonText(j, "typ");
        if (typ == "mistnost") {
          std::lock_guard<std::mutex> l(m_);
          kod_ = jsonText(j, "kod");
        } else if (typ == "hrac" && !r) {
          {
            std::lock_guard<std::mutex> l(m_);
            jmeno_ = jsonText(j, "jmeno");
            infoHry_ = j;
            relace_ = std::make_shared<Relace>(n_.hrac1, n_.ja, n_.snimekMs);
            r = relace_.get();
            faze_ = Faze::HRAJE;
          }
          r->budik([this] { ws_.probud(); });
          r->zacni(tedUs());
        } else if (typ == "odesel") {
          if (r) r->druhyOdesel();
          else { nastavChybu("druhy hrac odesel"); break; }
        } else if (typ == "chyba") {
          nastavChybu(jsonText(j, "zprava"));
          if (r) r->ukonci(D_CHYBA);
          else break;
        }
      } else if (p == 1 && op == 2 && r) {
        r->prijmi(z.data(), z.size(), tedUs());
      }
      if (r && r->tik(tedUs(), n_.tichoUs)) start_ = true;
    }
    ws_.zavri();
    std::lock_guard<std::mutex> l(m_);
    faze_ = Faze::KONEC;
  }

  Nastaveni n_;
  mutable std::mutex m_;
  Faze faze_ = Faze::NIC;
  std::string chyba_, kod_, jmeno_, infoHry_;
  std::shared_ptr<Relace> relace_;
  WebSocket ws_;
  std::thread vlakno_;
  std::atomic<bool> start_{false}, bezi_{false}, konecPozadovan_{false};
};

}  // namespace online
}  // namespace nap
