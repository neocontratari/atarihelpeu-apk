# AtariHelp.eu EMU-09 FIX59 PITSTOP SPLIT ROAD + RIVER CLIP CORE

FIX59 navazuje na FIX58 podle mobilních logů:

- Pitstop II: FIX58 už ukázal hru, ale spodní polovina neseděla. Log ukázal, že aktuální DLIST často míří na $A0D7, zatímco virtuální stabilizace pořád opravovala jen $2B00. FIX59 přidává Pitstop split-road renderer, který kreslí horní a spodní závodní buffer odděleně a nečte rozpadlý DLIST uprostřed frame.
- River Raid: obraz už běží, ale PMG letadla je vizuálně mimo vodní koridor. FIX59 přidává vodní koridor pro River PMG a drží kolize po HITCLR do dalšího renderovaného frame, dokud nemáme plné scanline GTIA timing.
- Obecný XEX směr zůstává: Donkey/Pitstop/River/Pac-Man jsou testy ANTIC/DLIST, DLI, PMG, GTIA a OS stubu, ne jediný cíl.

Build tag: `FIX59_PITSTOP_SPLIT_ROAD_RIVER_CLIP_CORE`.
