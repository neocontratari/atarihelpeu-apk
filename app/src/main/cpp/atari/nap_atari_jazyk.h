// nap_atari_jazyk.h
// ====================================================================
//  B314: HLASKY A NAPISY PRISTROJE ATARI 130XE CESKY / ANGLICKY
//
//  Rene: "slo by dat vyber jazyka - volbu Cze a Ang? ... po volbe na ang by
//  byla apka v anglictine? a naopak?"
//
//  Jazyk posila Java (OPTIONS -> JAZYK · LANGUAGE) prikazem devCtl
//  "jazyk:en" / "jazyk:cs" pri zapnuti pristroje. Napisy na Atari samotnem
//  (klavesy, START / SELECT / OPTION, HELP, READY) jsou anglicky vzdy -
//  tak vypada skutecne Atari. Prekladaji se hlasky appky: lista pod
//  kazetou, tlacitka servisni listy, D-pad na sirku, uprava rozlozeni.
//
//  Bez Androidu - testuje se na PC (test_overeni/b314).
// ====================================================================
#pragma once
#include <atomic>
#include <cstring>
#include <string>

namespace nap {

inline std::atomic<bool> &atariJazykEn() {
  static std::atomic<bool> en{false};
  return en;
}
inline bool atariEn() { return atariJazykEn().load(std::memory_order_relaxed); }

// text podle jazyka: tx("CESKY", "ENGLISH")
inline const char *tx(const char *cz, const char *en) { return atariEn() ? en : cz; }
inline std::string tx(const std::string &cz, const std::string &en) { return atariEn() ? en : cz; }

// stitky tlacitek servisni listy a D-padu na sirku - v tabulkach zustavaji cesky
// (tabulky jsou i v testech), prelozi se az pri kresleni
inline const char *stitek(const char *s) {
  if (!s || !atariEn()) return s;
  static const char *const P[][2] = {
      {"HRY", "GAMES"}, {"MOBIL", "PHONE"}, {"CHYBA", "ERROR"}, {"MEZERA", "SPACE"},
  };
  for (const auto &p : P)
    if (std::strcmp(s, p[0]) == 0) return p[1];
  return s;
}

// chyba nacteni kazety (nap_atari_tape.h) -> anglicky (do listy pod kazetou)
inline std::string chybaKazety(const std::string &e) {
  if (!atariEn()) return e;
  struct P { const char *cz, *en; };
  static const P PP[] = {
      {"neni WAV (chybi RIFF/WAVE)", "not a WAV (no RIFF/WAVE)"},
      {"WAV bez fmt/data", "WAV without fmt/data"},
      {"WAV neni PCM (format ", "WAV is not PCM (format "},
      {"WAV ma divne parametry", "WAV has odd parameters"},
      {"WAV ma nepodporovanou bitovou hloubku ", "WAV has an unsupported bit depth "},
      {"WAV je prilis kratky", "WAV is too short"},
      {"WAV je ticho - zadny signal kazety", "WAV is silent - no tape signal"},
  };
  for (const P &p : PP) {
    const size_t n = std::strlen(p.cz);
    if (e.compare(0, n, p.cz) == 0) return std::string(p.en) + e.substr(n);
  }
  return e;
}

// "N ŘÁDKŮ" / "N LINES" (ceske tvary 1 / 2-4 / 5+)
inline std::string radkuText(long n, bool velka) {
  if (atariEn()) return std::to_string(n) + (n == 1 ? " LINE" : " LINES");
  const char *s = n == 1 ? (velka ? u8"ŘÁDEK" : u8"řádek") : (n >= 2 && n <= 4) ? (velka ? u8"ŘÁDKY" : u8"řádky") : (velka ? u8"ŘÁDKŮ" : u8"řádků");
  return std::to_string(n) + " " + s;
}

}  // namespace nap
