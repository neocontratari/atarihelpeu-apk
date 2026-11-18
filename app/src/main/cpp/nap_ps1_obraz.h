// =============================================================================
//  B346: KONTROLA VELIKOSTI OBRAZU DISKU PS1 (cisty C++, bez Androidu)
//
//  Rene: "nevim proc se nespusti CaC ale to muze byt chyba souboru". Log
//  telefonu 18. 11.: Command & Conquer - Red Alert Retaliation, .cue s jednou
//  stopou MODE2/2352, .BIN ma 557 574 385 B = 237 063 celych sektoru + 2 209 B
//  (do celeho sektoru chybi 143 B). Jadro: "CheckCdrom: missing lic sector?",
//  "unsupported/invalid CD image" -> appka hlasila jen "PS1_HRA_FAIL jadro".
//
//  Surova stopa CD ma vzdy CELE sektory: MODE2/2352, MODE1/2352 a AUDIO
//  2352 B, MODE2/2336 2336 B, MODE1/2048 (a .iso) 2048 B, CDG 2448 B. Soubor,
//  ktery cele sektory nema, je useknuty nebo poskozeny (spatne stazeny, vadny
//  soubor na serveru). Kdyz ho jadro odmitne, appka rekne proc.
// =============================================================================
#pragma once
#include <cstdio>
#include <cstring>     // strcasestr (jako nap_ps1_native.cpp)
#include <string>
#include <strings.h>

namespace nap_obraz {

// velikost sektoru podle radku TRACK z .cue ("TRACK 01 MODE2/2352" -> 2352); 0 = nevim
inline int sektor_stopy(const char *radek) {
  if (!radek) return 0;
  if (strcasestr(radek, "AUDIO")) return 2352;
  if (strcasestr(radek, "CDG")) return 2448;
  if (strcasestr(radek, "/2352")) return 2352;
  if (strcasestr(radek, "/2336")) return 2336;
  if (strcasestr(radek, "/2048")) return 2048;
  return 0;
}

// popis vady ("" = v poradku): velikost souboru neni cely pocet sektoru
inline std::string vada(const std::string &jmeno, long long velikost, int sektor) {
  if (velikost <= 0 || sektor <= 0) return "";
  const long long zbytek = velikost % sektor;
  if (zbytek == 0) return "";
  char t[512];
  snprintf(t, sizeof(t), "%s ma %lld B - neni cely pocet sektoru %d B (%lld celych + %lld B, do celeho chybi %lld B)",
           jmeno.c_str(), velikost, sektor, velikost / sektor, zbytek, (long long)sektor - zbytek);
  return t;
}

// .bin / .img / .iso bez .cue: jadro bere 2048 B (ISO), jinak surove 2352 B - kdyz nesedi nic, vada
inline std::string vada_bez_cue(const std::string &jmeno, long long velikost) {
  if (velikost <= 0) return "";
  if (velikost % 2352 == 0 || velikost % 2048 == 0 || velikost % 2336 == 0) return "";
  return vada(jmeno, velikost, 2352);
}

}  // namespace nap_obraz
