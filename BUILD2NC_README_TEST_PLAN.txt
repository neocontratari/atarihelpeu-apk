BUILD2NC_SEGA_RESTORE_SKIN_DIRECT_ROM_STAGE16

TEST PLAN:
1. Rozbal ZIP pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. OCEKAVANI: musi se vratit puvodni Sega skin, zadna cerna rozbita obrazovka a zadny rozbity img symbol.
5. Klikni HRY / SBIRKA / CARTRIDGE SLOT a vyber Sonic .gen.
6. Pockej 8-12 sekund.
7. Klikni ULOZENE a posli log.

CO HLEDAT V LOGU:
- POSLEDNI ROM uz nesmi byt 'zatim nic nenacteno'.
- musi byt SEGA ROM HEADER BUILD2NC file=...
- musi byt loadRom prijato title=...
- pokud core spadne, poslat presnou chybu po vyberu ROM.

KODY JSOU ZMENENE.
