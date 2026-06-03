AtariHelp.eu EMU-09 FIX104 SHARED INPUT + GTIA LATCH CORE

Cíl: vrátit směr zpět k obecnému emulátoru, ne k opravě jedné hry.

Změny:
- sjednocená TRIG0 repeat/latch cesta pro hry, které na mobilním držení FIRE potřebují hrany,
- D010 i OS STRIG0 teď používají stejný stav u Cobra/River-like kontinuální střelby,
- Cobra dostává jen krátký sekundární DOWN pulse pro jednu-tlačítkový bomb cadence, ne trvalý joystick hack,
- Cobra/River si nechávají GTIA HITCLR latch do dalšího renderu, protože kolize počítáme z vykresleného snímku,
- bounded PF fallback z renderovaných pixelů: kolize terénu/tanků se nemají ztratit kvůli DLI barevnému posunu,
- Cobra title overlay zůstává, Donkey safe baseline, Arkanoid čisté menu a Montezuma VVBLKD/PMG zůstávají zachované.

Test hlavně:
1) Cobra: menu, gameplay, držení FIRE, pomalé/rychlé střelby, náraz do nádrže/terénu.
2) Donkey: jestli se nevrátil dlouhý šum FIX99.
3) Montezuma: po intru, duchy.
4) Arkanoid: menu.
