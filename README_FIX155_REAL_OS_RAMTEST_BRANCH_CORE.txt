# AtariHelp.eu EMU-09 — FIX155_REAL_OS_RAMTEST_BRANCH_CORE

Overlay ZIP pro Android WebView Atari 800/XL/XE/130XE XEX emulátor.

## Smysl buildu

FIX154 byl poctivý pokus o skutečný OS/BASIC reset, ale log ukázal, že XL OS startuje z reset vectoru `$C2AA`, pak kvůli nulovému boot/RAM flagu `$01` spadne do pevného skoku `$C3C1 -> $5003`, kde v našem jádru leží prázdná RAM (`BRK at $5003`).

FIX155 nedělá falešné READY a nekreslí BASIC ručně. Jen v režimu `EXPERIMENT REAL OS RESET / BASIC BOOT` přidává cílený RAM-test/boot guard:

- pokud OS při resetu dojde na `$C3AB` a `$01` je nulové, nastaví `$01=1`,
- pokud by přesto došel na `$C3C1`, přeskočí prázdný skok do `$5003` na běžnou OS pokračovací větev `$C3C4`,
- loguje to jako `RAMTEST guard`, aby bylo jasné, že nejde o kreslený READY.

## Co zůstává

- Donkey / Super Cobra / Montezuma XEX baseline zůstává chráněný.
- ROM jsou vestavěné jako v předchozích buildech.
- Nepotřebná tlačítka zůstávají odstraněná.

## Commit summary

`FIX155 real OS ramtest branch core`

## Test plan

1. Spusť build a ověř tag `FIX155_REAL_OS_RAMTEST_BRANCH_CORE`.
2. Klikni `EXPERIMENT REAL OS RESET / BASIC BOOT`.
3. Pokud naběhne skutečný READY/BASIC, zkus `PRINT 2+2`.
4. Pokud nenaběhne, ulož snapshot. Hlavně řádky `REAL OS BOOT FIX155`, `RAMTEST guard`, `PC`, `DLIST`, `SAVMSC`, `PORTB`.
5. Krátce ověř Donkey / Super Cobra / Montezuma, že baseline zůstal funkční.
