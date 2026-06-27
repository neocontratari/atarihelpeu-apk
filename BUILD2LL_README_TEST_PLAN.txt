AtariHelp.eu EMU-10 BUILD2LL_PRIORITY01_RIVERRAID_AND_XEX_TURBO_EXIT_SAFE

Zaklad:
- BUILD2LJ, ne BUILD2LK.
- BUILD2LK stabilni HPOS/SIZE se NEPOUZIVA, protoze test neopravil Decathlon telo/nohy a River Raid/P2-P3 PMG se nesmi dal riskovat.
- BUILD2LJ CHBASE 512 zustava, protoze Rene potvrdil zlepseni horni nastenky/textu.

Zmeny v BUILD2LL:
1) GTIA PRIOR=$01 oprava:
   - V LH/LJ byla vetev PRIOR=$01 spatne: P2/P3 byly v praxi trvale schovane.
   - River Raid podle logu kresli protivniky pres P2/P3, PM DMA bezi, ale pDraw padal proti pBlockedPF.
   - BUILD2LL u PRIOR=$01 znovu pusti P/M nad playfield, aby se River Raid protivnici vratili.
   - PRIOR=$04 pro Decathlon zustava LH cesta, aby se nerozbila zpet viditelnost atleta.

2) XEX turbo exit safe:
   - Log ukazal, ze XEX fake-fast zustal aktivni i kdyz uz hra bezela v RAM PC=$99FC/$AE9C.
   - To muze delat startovni graficky/zvukovy kousanec a dojem, ze se hra dlouho chyta po nacteni.
   - BUILD2LL vypne XEX turbo hned, kdyz je SIO fronta prazdna, par snimku je ticho a PC je v RAM/game oblasti.
   - CLOAD/CSAVE se nemeni. ATR disk turbo se nemeni.

Co se NEMENI:
- UI / klavesnice / joystick / kazeta / skin: beze zmen.
- CLOAD / CSAVE: beze zmen.
- Testovaci XEX soubory: KODY JSOU STEJNE.
- Zadny screen-paint, zadny hack podle nazvu hry.

Log markery:
- AtariHelp.eu EMU-10 BUILD2LL_PRIORITY01_RIVERRAID_AND_XEX_TURBO_EXIT_SAFE pripraven
- GTIA PRIOR01 BUILD2LL
- XEX FAST LOAD BUILD2LL
- ANTIC CHBASE 512 BUILD2LJ

Test plan:
1) Decathlon:
   - zacatek hry: je stale cca 4s kousani zvuku/grafiky, nebo lepsi?
   - horni nastenka: zustalo zlepseni z LJ? je jen WELCOME, nebo zacina scroll?
   - atlet telo/nohy: lepsi / stejny / horsi proti LK a LJ?

2) River Raid:
   - jsou protivnici znovu videt? ANO/NE
   - scroll napisy/barvy zustaly lepsi? ANO/NE
   - zvuk a rychlost po nacteni: lepsi / stejny / horsi

3) Regrese:
   - Donkey Junior: OK / rozbite
   - Commando: OK / rozbite
   - Mission: OK / rozbite
   - Night Driver: auto/pruh/zvuk

Co poslat zpet:
- screenshot Decathlon zacatek + atleta
- screenshot River Raid s protivniky nebo bez nich
- LOG/CHYBA cely log po testu
