# AtariHelp.eu EMU-09 FIX58 PITSTOP VIRTUAL DLIST + RIVER COLLISION CORE

FIX58 navazuje na FIX57 podle mobilních logů:

- Pitstop II: v závodě se obě poloviny obrazovky na chvíli ukážou, ale potom se DLIST $2B00 během samomodifikace rozpadne. FIX58 už nepřepisuje trvale RAM; při renderu používá virtuální stabilní závodní DLIST, zatímco obrazová RAM hry zůstává živá.
- River Raid: mapování DLI snapshotu už nejde prostým indexem y, ale podle VCOUNT/scanline, aby PMG a playfield seděly ve stejném okamžiku.
- Pac-Man: log stále jasně ukáže XEXy, které skočí do prázdné cartridge/BASIC oblasti a nejsou čistý samostatný XEX.

Cíl zůstává obecný XEX emulátor. Pitstop/River/Donkey/Pac-Man jsou testovací případy pro ANTIC DLIST, DLI timing, PMG, GTIA kolize a OS stub.

Build tag: `FIX58_PITSTOP_VIRTUAL_DLIST_RIVER_COLLISION_CORE`.
