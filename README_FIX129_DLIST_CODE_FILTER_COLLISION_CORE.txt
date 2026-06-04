AtariHelp.eu EMU-09 FIX129_DLIST_CODE_FILTER_COLLISION_CORE

Co je cílem:
- Nelepit jednu hru, ale posunout společné jádro: DLIST filtr, fallback renderer a GTIA kolize.
- Zachovat Donkey/Cobra/Arkanoid reference co nejvíc chráněné.

Změny:
1) Moon Patrol / obecné XEX
- $7481 už není brán jako display list, protože log z FIX128 ukázal 6502 kód: STA/LDA/JMP CIOV.
- Falešné code-like DLISTy se blokují a obraz padá do bezpečnějšího framebuffer scan fallbacku.
- Snapshot přidává řádek FRAMEBUFFER SCAN FIX129, aby bylo vidět, jakou RAM oblast renderer vybral.

2) Super Cobra kolize
- HITCLR u Cobry už nesmaže hned MxPF/MxPL latch v tom samém frame.
- Missile-player kolize jsou znovu povolené pro PMG cíle P2/P3, ale P0/P1 se potlačí jako vlastní vrtulník/self-hit.
- Cíl: kulomet a bomby mají větší šanci hlásit zásah bez návratu náhodných self-hitů.

3) Ochrany
- Cobra drží $2000 a nemá framehold flicker.
- Donkey chráněný.
- Arkanoid není agresivně měněný; teď jen těží z obecného DLIST/renderer filtru.
- Montezuma zůstává na bezpečnějším režimu z předchozí větve.

Co prosím otestuj:
1) Nahoře/log musí být FIX129_DLIST_CODE_FILTER_COLLISION_CORE.
2) Cobra: hlavně střelba/kolize kulometu a jestli po smrti jen malý/no flicker.
3) Donkey: jestli pořád běží.
4) Moon Patrol: pošli snapshot log, hlavně řádky DLIST RESOLVE a FRAMEBUFFER SCAN FIX129.
5) Arkanoid III: menu a první obraz hry stejné/lepší/horší.
6) Montezuma PRELIM: jen rychlá kontrola START a po smrti.
