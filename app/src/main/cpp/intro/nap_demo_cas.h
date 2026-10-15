// nap_demo_cas.h
// ====================================================================
//  B307: CASY SCENY 4 (obraz i zvuk se ridi stejnymi cisly)
//  B309: Rene - "na intro ti pridam 14 s - intro muze mit 59 vterin - ty
//  ukazky her jsou hekticke a rychle". Vsech 14 s dostalo defile na CRT
//  (drive 3,85 s, ted 17,85 s): kazda ukazka ~6 s misto ~1,3 s. Sceny 1-3
//  a zacatek sceny 4 (pruty, trojice, prulet ke CRT) jsou beze zmeny,
//  scena 5 (logo, titulky) je cela posunuta o POSUN5 = 14 s.
//
//  27,00  klavesy Atari se rozzari (vlna od kontrolky)
//  27,55  z klaves vystreli neonove pruty (vlevo azurove, vpravo ruzove)
//  28,75  pruty dorazi ke konzolim - zablesk, kresli se obrys (drat)
//  29,45  konzole N&P 16 a N&P 32 se zhmotni zdola nahoru
//  30,60  zaber na trojici (Atari 130XE, N&P 16, N&P 32)
//  31,95  kamera leti k CRT monitoru, 32,35 CRT se zapne
//  32,80  defile 50 snimku/s: Pitt-Kitt Platinum (Atari 130XE) - titulek N&P,
//  33,90  kapitola (Las Vegas 1982), 35,00 jizda se zvukem hry
//  38,40  N&P 16 (16 bit, 6 s)
//  44,40  N&P 32 (32 bit, 6,25 s)
//  50,65  zablesk do loga (scena 5 od 51,00 do 59,00)
// ====================================================================
#pragma once

namespace napdemo {
namespace cas {
constexpr float KLAVESY = 27.0f;
constexpr float PRUTY = 27.55f;
constexpr float PRILET = 28.75f;
constexpr float DRAT = 28.85f;
constexpr float ODHAL = 29.45f;
constexpr float ODHAL_KONEC = 30.45f;
constexpr float TROJICE = 30.6f;
constexpr float PRULET = 31.95f;
constexpr float CRT_ZAP = 32.35f;
constexpr float STRIH_ATARI = 32.8f;
constexpr float ATARI_KAP = 33.9f;      // podstrih: kapitola (Las Vegas 1982)
constexpr float ATARI_JIZDA = 35.0f;    // podstrih: jizda (se zvukem hry)
constexpr float STRIH_16 = 38.4f;
constexpr float STRIH_32 = 44.4f;
constexpr float STRIHY_KONEC = 50.65f;
constexpr float SCENA5 = 51.0f;
constexpr float POSUN5 = SCENA5 - 37.0f;  // scena 5 (B304-B308 od 37 s) je ted o tolik pozdeji
constexpr float KONEC = 59.0f;            // konec dema
// B309: hlasitost zvuku hry (Pitt-Kitt, jizda 3,4 s) ve stope - hudba hry je tichsi
// (RMS -30 dBFS) a drive znela jen 0,85 s; ted je stejne hlasita jako zbytek defile
constexpr float ZVUK_HRY = 4.5f;
}  // namespace cas
}  // namespace napdemo
