AtariHelp.eu EMU-10 / N&P VISION
BUILD2MW_SEGA_ORIGIN_GUARD_PATCH_STAGE10

CIL:
Opravit dalsi konkretni chybu z BUILD2MV:
CLOSURE_EVAL_RUN_FAILED online:lrusso/Genesis.min.js Error. This emulator cannot be used from a different origin.

CO TESTOVAT:
1. Rozbal ZIP overlay pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. Pres HRY / SBIRKA / CARTRIDGE vyber Sonic .gen.
5. Necekej iframe kolecko. Cekej graficky boot/status monitor.
6. Pokud core nabehne, zkus START / A / B / C / D-PAD.
7. Klikni kratce ULOZENE a posli TXT log z Downloads/AtariHelp.

CO HLEDAT V LOGU:
- BUILD2MW_SEGA_ORIGIN_GUARD_PATCH_STAGE10
- ENGINE SOURCE SCAN online:lrusso/Genesis.min.js
- ORIGIN GUARD PATCH ... phrase=YES ... patches=...
- CURRENT_SCRIPT SRC SHIM online original=... safe=...

DOBRY VYSLEDEK:
- CLOSURE EVAL OK embedGenesis exported
- CORE ENGINE READY
- embedGenesis CALLBACK STARTED
- popripade realny obraz v monitoru

POKUD TO SPADNE:
Posli LOG. Dulezite je, jestli uz zmizela chyba "different origin" a jaka je dalsi presna chyba.

POCTIVY STAV:
Tohle porad neni fake emulace. BUILD2MW jen patchuje loader-origin ochranu, ktera brani spustit realny engine uvnitr Android WebView/appassets prostredi. Emulacni kod/video/audio/logika se nemaluje ani nenahrazuje.
