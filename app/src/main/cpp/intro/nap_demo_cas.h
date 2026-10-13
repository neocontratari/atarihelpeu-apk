// nap_demo_cas.h
// ====================================================================
//  B307: CASY SCENY 4 (obraz i zvuk se ridi stejnymi cisly)
//
//  27,00  klavesy Atari se rozzari (vlna od kontrolky)
//  27,55  z klaves vystreli neonove pruty (vlevo azurove, vpravo ruzove)
//  28,75  pruty dorazi ke konzolim - zablesk, kresli se obrys (drat)
//  29,45  konzole N&P 16 a N&P 32 se zhmotni zdola nahoru
//  30,60  zaber na trojici (Atari 130XE, N&P 16, N&P 32)
//  31,95  kamera leti k CRT monitoru, 32,35 CRT se zapne
//  32,80  defile 50 snimku/s: Pitt-Kitt Platinum (Atari 130XE),
//  34,25  N&P 16 (16 bit), 35,45 N&P 32 (32 bit)
//  36,65  zablesk do loga (scena 5 od 37,00)
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
constexpr float ATARI_KAP = 33.12f;     // podstrih: kapitola (Las Vegas 1982)
constexpr float ATARI_JIZDA = 33.40f;   // podstrih: jizda
constexpr float STRIH_16 = 34.25f;
constexpr float STRIH_32 = 35.45f;
constexpr float STRIHY_KONEC = 36.65f;
constexpr float SCENA5 = 37.0f;
}  // namespace cas
}  // namespace napdemo
