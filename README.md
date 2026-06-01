# AtariHelp.eu EMU-09 FIX69 DLIST CORE + REFERENCES

FIX69 reaguje na testy z 2026-06-01:

- Hlavni vestavene testy jsou jen PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chraneny referencni titul.
- Nova KiTT Garage reference se uz nedetekuje jako PiTT-KiTT jen kvuli slovu KITT v nazvu.
- Super Cobra: obecna validace ANTIC display listu uz neodmita korektni DLIST s hustymi LMS radky. To ma opravit cernou/fallback obrazovku bez krajiny.
- Donkey Kong: obecna PMG vrstva doplnuje player-player a missile-player GTIA kolize, aby hry videly i bonusy/objekty kreslene pres PMG, nejen playfield.
- CORE TEST TXT se uklada do mobilu automaticky a po testu vraci puvodni hru.

Build:

FIX69 dlist core references
