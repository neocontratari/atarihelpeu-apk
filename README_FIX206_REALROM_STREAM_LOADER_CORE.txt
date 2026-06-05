AtariHelp.eu EMU-09 FIX206_REALROM_STREAM_LOADER_CORE

Ucel:
- Zastavit slepe ladeni GTIA duhy a vratit se k tomu, co Rene pozaduje: cisty loader pod kapotou.
- ROM soubory znovu vlozeny z dodanych souboru a zapsany do assets:
  - ATARIXL.ROM md5 06daac977823773a3eea3422fd26a703 / 16384 bytes
  - ATARIOSB.ROM md5 a3e8d617c95d08031fe1b20d541434b2 / 10240 bytes
  - ATARIBAS.ROM md5 0bac0c6a50104045d902df4503a4c30b / 8192 bytes
  - REVA.ROM md5 a4dc52536d526ecc51ea857b9fa2b90f / 8192 bytes, REVB.ROM md5 04ea6a4e386601445ca5bfc8e37fb620 / 8192 bytes, REVC.ROM md5 0bac0c6a50104045d902df4503a4c30b / 8192 bytes

KODY JSOU STEJNE:
- GTIA 9 VERIFY BASIC kod je stejny jako predtim.
- GTIA 10 VERIFY BASIC kod je stejny jako predtim.
- GTIA 11 VERIFY BASIC kod je stejny jako predtim.
- Tenhle overlay neni dalsi duhovy/paletovy pokus.

Hlavni zmena jadra:
- XEX loader ma ted stream INITAD rezim zapnuty jako vychozi cestu pro vsechny XEX.
- Segmenty se kopiruji v poradi souboru.
- Mezilehle INITAD se spousti behem loadu, jako DOS-like XEX loader.
- Zbyvajici INITAD se spusti pred RUNAD/ENTRY v startPlaying().
- BASIC direct zustava OFF.
- OS ROM probe zustava sluzba pro ROM calls, ne fake obrazova cesta.
- HW/SDLST DLIST ma porad prednost pred RAM scanem.

Snapshot nove hlasi:
- REAL ROM AUDIT FIX206
- XEX STREAM LOADER FIX206
- NATIVE XEX LOADER FIX206
- XEX OS KERNEL FIX206
- XEX STREAM STATUS FIX206

Test plan:
1. Nepokracuj v duhovych GTIA testech jako hlavnim smeru.
2. Otestuj vlastni XEX pres normalni loader: Super Cobra, Donkey, Pitstop II, Arkanoid, Moon Patrol/Pac-Man pokud mas.
3. Po nacteni hry dej snapshot/log.
4. V logu hledej hlavne REAL ROM AUDIT FIX206 a XEX STREAM LOADER FIX206.
5. Pokud hra ukaze rozsypany obraz, posli screenshot + log; dal se jde po loader/DLIST/ANTIC, ne po palete duhy.

Commit summary:
FIX206 realrom stream loader core
