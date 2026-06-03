# AtariHelp.eu EMU-09 FIX105 SHARED INPUT + PMG/COBRA CORE

FIX105 je jádrový krok, ne jednorázový Cobra hack.

- Cobra: menu fallback zachován; FIRE/TRIG cadence zjemněna na kratší release a sekundární DOWN okno je svázané s každou druhou trigger hranou.
- GTIA: navazuje na předchozí latch/collision cestu, bez potlačení player/playfield kolizí z FIX102.
- Montezuma: přidán PMG stale-row window filtr pro vertikální duchy postavy po intru; VVBLKD fallback zůstává.
- Donkey: bezpečný baseline beze změny, bez dlouhého FIX99 $1200 holdu.
- Arkanoid: čisté menu zachováno, hra zatím další větev.

Test: Cobra střelba a nárazové kolize; Montezuma duchové po intru; Donkey/Arkanoid jen regresní kontrola.
