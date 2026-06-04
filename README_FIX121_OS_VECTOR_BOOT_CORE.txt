FIX121 OS VECTOR BOOT CORE

Cíl: posunout interní Atari OS/BASIC kompatibilitu bez toho, aby tester musel ručně nahrávat ROM.

Změny:
- ROM soubory od Reného bereme jako technickou mapu OS/BASIC vstupů; do ZIPu se nevkládají binární ROM data.
- Přidána interní OS vector mapa: CIOV, SIOV, SETVBV, SYSVBV, XITVBV a širší OS $E400-$E4FF trampoline.
- Doplněné OS shadow hodnoty: VDSLST, VVBLKI/VVBLKD, SDMCTL, CHBAS, CH, IOCB0 status.
- CPU si pamatuje poslední smysluplné PC v načteném XEXu; při OS/HW skoku bez návratové adresy se nepokouší nekonečně běžet ROM skeleton.
- Moon Patrol: silnější odmítnutí falešných display-listů, které jsou ve skutečnosti loader kód typu STA/LDA/STA.
- Montezuma PRELIM: vypnut agresivní CHBASE guard z FIX116, který rozbíjel lebku/nástrahy; ALT/ORIG mají jen opatrný fallback.
- Donkey / Cobra / Arkanoid baseline chráněný.

Test:
1) Donkey/Cobra/Arkanoid jen rychle potvrdit baseline.
2) Moon Patrol: obraz, zvuk, jestli se nevrací bílý/code-like čaj.
3) Pac-Man: zda běží jinak/déle, screenshot + snapshot log.
4) Montezuma PRELIM/ALT/ORIG: hlavně PRELIM 3. level, smrt/reload a lebka/nástrahy.
