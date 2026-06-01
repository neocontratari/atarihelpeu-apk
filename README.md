# AtariHelp.eu EMU-09 FIX70 OSCOLOR + TEXT + COLLISION

FIX70 reaguje na testy z 2026-06-01:

- Hlavni vestavene testy jsou jen PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chraneny referencni titul.
- Nova KiTT Garage reference se uz nedetekuje jako PiTT-KiTT jen kvuli slovu KITT v nazvu.
- KiTT Garage: obecny core obnovuje Atari OS shadow barvy do GTIA pred DLI snimkem, textove ANTIC 2/3 mody kresli normalni znaky pres COLPF2 a KiTT bezi pomalejsim obecnym taktem.
- Super Cobra: obecna validace ANTIC display listu uz neodmita korektni DLIST s hustymi LMS radky. To ma opravit cernou/fallback obrazovku bez krajiny.
- Super Cobra / obecne PMG hry: HITCLR uz nemaze vypoctene GTIA kolize uprostred emulovaneho frame driv, nez si je hra stihne precist.
- Donkey Kong: obecna PMG vrstva doplnuje player-player a missile-player GTIA kolize, aby hry videly i bonusy/objekty kreslene pres PMG, nejen playfield.
- CORE TEST TXT se uklada do mobilu automaticky a po testu vraci puvodni hru.

Build:

FIX70_OSCOLOR_TEXT_COLLISION

