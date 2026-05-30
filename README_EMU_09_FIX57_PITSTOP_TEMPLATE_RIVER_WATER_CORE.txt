# AtariHelp.eu EMU-09 FIX57 PITSTOP TEMPLATE + RIVER WATER CORE

FIX57 navazuje na FIX56 a řeší dvě chyby z mobilních logů:

- Pitstop II: v závodě se DLIST $2B00 někdy zachytí uprostřed samomodifikace a vypadá jako krátký DLIST s JVB na $0099. FIX57 opravuje jen instrukční kostru závodního DLISTu, aby zůstalo dělení obrazovky horní char část + spodní bitmap/road část.
- River Raid: voda už se nebere jako solid kolize. FIX56 hlásil kolizi prakticky na všech nečerných pixelech, takže letadlo nemělo použitelný rozdíl mezi vodou a břehem/skálou.

Cíl pořád zůstává obecný XEX emulátor, ne jen vestavěné testy. Pitstop/River/Donkey/Pac-Man jsou testovací případy pro ANTIC, DLI, PMG, GTIA kolize a OS stub.

Build tag: `FIX57_PITSTOP_TEMPLATE_RIVER_WATER_CORE`.
