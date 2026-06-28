AtariHelp.eu EMU-10 / N&P VISION
BUILD2MV_SEGA_FETCHTEXT_EVAL_REPAIR_NO_PIP_STAGE9

CIL:
Opravit BUILD2MU chybu "fetchText is not defined" a odstranit matouci obraz-v-obraze efekt pri tlacitku ULOZENE.

CO TESTOVAT:
1. Rozbal ZIP overlay pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. Pres HRY / SBIRKA / CARTRIDGE vyber Sonic .gen nebo Aladdin .gen.
5. Cekej na graficky boot/status monitor.
6. Klikni ULOZENE jen kratce: log se ma ulozit do Downloads/AtariHelp a monitor se nema prekryt velkym logem.
7. Dlouhy stisk ULOZENE cca 1s muze zobrazit servisni nahled logu v monitoru.

CO MA BYT V LOGU:
- nesmi byt: fetchText is not defined
- hledej: ENGINE SOURCE SCAN
- hledej: CLOSURE EVAL OK ... nebo presna nova chyba CLOSURE_EVAL_COMPILE_FAILED / CLOSURE_EVAL_RUN_FAILED / ENGINE_SOURCE_HAS_NO_EMBEDGENESIS_TOKEN

POCTIVY STAV:
Tohle porad neni fake emulace. Je to oprava loaderu pro realny lrusso Genesis/PicoDrive engine. Pokud eval cesta selze, log ma konecne rict konkretni duvod. Atari/VBXE/EMU2 se nemeni.
