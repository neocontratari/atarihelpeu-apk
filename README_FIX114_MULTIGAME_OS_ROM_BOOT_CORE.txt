EMU-09 FIX114 MULTIGAME OS-ROM/BOOT CORE

Důvod:
FIX113 ukázal tři různé typy problémů:
1) Montezuma pořád umí po přechodu/reloadu přepsat grafiku, i když framehold je vypnutý.
2) Moon Patrol se posunul z PC $0001, ale loader handoff skočil do datového segmentu ($9D55) a opakovaně narazil na unsupported opcode $02.
3) Pac-Man nejde jen přes DLIST; log ukazuje velkou závislost na BASIC/OS ROM.

Změny:
- Přidán vestavěný TEST MONTEZUMA ORIG vedle TEST MONTEZUMA PRELIM.
- Moon Patrol/generic loader handoff je povolený jen tehdy, když cílový segment vypadá jako kód.
- Handoff loguje kandidáta, skóre a první opcode.
- Pac-Man BASIC/OS diagnostika je zřetelnější ve snapshotu.
- Donkey Kong, Super Cobra a Arkanoid zůstávají bez nových zásahů.
- Viditelný build tag a HTML/JS snapshoty jsou přepsané na FIX114.

Test:
1. Montezuma PRELIM: projít do game over / reload a snapshot.
2. Montezuma ORIG: stejný postup, hlavně porovnat jestli se grafika přepisuje.
3. Moon Patrol: načíst, 5-10 sekund počkat, snapshot.
4. Pac-Man: načíst a snapshot; čekáme diagnostiku BASIC/OS ROM.
5. Donkey/Cobra/Arkanoid jen rychlá regrese.
