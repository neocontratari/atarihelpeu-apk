AtariHelp.eu EMU-10 BUILD2KN_G2F_GTIA_LINE_LATCH_RASTER_CORE

Navazuje na BUILD2KM, kde byl videt maly posun, ale Postcard/G2F stale neni spravne.

CIL:
- Postcard/G2F: odstranit nebo zmensit fialove/tyrkysove svisle pruhy tim, ze ANTIC F + GTIA9/11 + husty DLI kernel bere COLBK/GPRIOR jako line-latch ze zacatku viditelne radky.
- Neni to hack podle nazvu souboru.
- Neni to screen-paint.
- PMG vrstva zustava pres normalni PRIOR, KK hard-mask zustava vypnuta.
- KD PMBASE zasah zustava pryc.
- KODY JSOU STEJNE.

ZMENENY SOUBOR:
app/src/main/assets/emu_vbxe/index.html

TEST:
1) Rozbal overlay pres GitHub Desktop.
2) Sestav debug APK.
3) Spust appku.
4) Nahraj test_assets/Postcard_Atari_Rocky.xex.
5) Napis jen: posun ano/ne.
6) Kdyz ne, posli screenshot + LOG / CHYBA.

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2KN_G2F_GTIA_LINE_LATCH_RASTER_CORE pripraven
GTIA G2F LINE LATCH BUILD2KN
