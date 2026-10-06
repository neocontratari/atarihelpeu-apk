# Co je v B300

Tvoje zpráva po B299: „Chyba byla v mém mobilu, ne v appce, co se týče
webu. Dále vše funguje, dokonce i ten výpis kódu – ale jediné, co jsem teď
zjistil, že nenajela hra Ghostbusters, a nevím proč, možná je to v logu.“

## 1. Ghostbusters – opraveno (chyba byla v POKEY)

V logu je i paměť Atari z LOG/CHYBA (poprvé k něčemu byla), takže je to
přesně vidět:

- Hra (Homesoft verze) hraje hudbu a řeč „Ghostbusters!“ z **přerušení
  časovače 1 v POKEY**. AUDCTL=$40 (1,79 MHz), **AUDF1=$FF**, asi 6 900
  přerušení za sekundu, každé pošle do zvuku jeden vzorek.
- Titulka čeká, až znělka dohraje (proměnná $000B, bit 0). Teprve pak
  pokračuje k písničce s balónkem a ke START.
- **Chyba u nás:** délka periody kanálu je AUDF+1. Při AUDF=$FF je to 256,
  jenže jádro ji drželo v 8bitové proměnné, kde z 256 byla 0. Čítač pak
  běžel do minusu a kanál **nikdy „netikl“**. Žádné přerušení, hudba
  nepokročila a titulka stála navěky. Viděl jsi jen nápis GHOSTBUSTERS
  nahoře a černo.
- **Oprava:** perioda je teď v plném čísle (1 až 256).
- Totéž se týkalo **zvuku všech her**: kanál s AUDF=$FF (nejhlubší tón
  v daném režimu) byl dosud potichu. Teď hraje.

Ověřeno na PC s tvým Ghostbusters.xex: logo, duch, hudba a řeč, text písně
s balónkem („who you gonna call? GHOSTBUSTERS!“, „I ain't 'fraid of no
ghost!“). Po START jde obrazovka „GHOSTBUSTERS – For Professional
Paranormal Investigations and Eliminations – … please state your name –
Last,First:“, tedy hra jede dál.

## 2. Web – byl to telefon

Appka to v logu i poznala: `B299 TLS: místo atarihelp.eu odpověděl JINÝ
server (certifikát localhost od ospanel)`. Po opravě v telefonu se hry
stahují přímo (`PROVIDER_DIRECT_FIRST_OK`).

## 3. Turbo-BASIC LIST

V tomhle testu fungovalo:

- Tetris jako TXT → Turbo-BASIC: 94 řádků napsaných za 33 s, 0 chyb.
  Potom CSAVE → NEW → CLOAD → LIST v pořádku.
- „WALLS (ZDI) … House Book.wav“: 18 záznamů, 39 řádků. LIST v pořádku.

**Ten dlouhý WALLS, který se rozsypal** („WALLSULTIMATED (N&P Edition)
House Book CSave.wav“, 36 záznamů, 71 řádků), jsi tentokrát nezkoušel.
Zkus prosím ještě jednou: TURBO BASIC, CLOAD a LIST. Když se rozsype,
nemačkej RESET, hned LOG/CHYBA a pošli log.

## Ověřeno na počítači

- Nový test `pokey-casovac`: perioda přerušení časovače 1 pro AUDF 0 až $FF,
  na 1,79 MHz i 64 kHz.
  - B300: všech 10 případů přesně (AUDF=$FF: 259 a 7 168 cyklů).
  - B299: při AUDF=$FF 0 přerušení.
- Acid800: výsledky test po testu stejné jako před opravou.
- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: 61 kontrol, 0 chyb
  (Popeye, W3D, CSAVE/CLOAD, TXT → BASIC/TBXL …).
