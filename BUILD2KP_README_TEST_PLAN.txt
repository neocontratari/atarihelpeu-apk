AtariHelp.eu EMU-10 BUILD2KP_G2F_PMG_LINE_STAGING_CORE

Ucel:
- Navazuje na KM/KN/KO, kde se Postcard/G2F lehce pohnul, ale obraz je porad hodne mimo.
- KO lathoval barvy, ale porad nechaval PMG objektove registry HPOS/SIZE/GRAF/GRACTL paprskove menit obraz uvnitr stejne radky.
- KP u ANTIC F + GTIA9/11 + husty G2F/DLI kernel bere PMG objektove registry jako line-staged stav cele scanline.
- Cíl: odstranit spatne svisle PMG/GTIA barevne sloupce u Postcard/G2F bez screen-paintu a bez hacku podle nazvu souboru.
- KD PMBASE zasah zustava pryc.
- KODY JSOU STEJNE.

Test:
1) Nahraj overlay pres GitHub Desktop.
2) Spust APK.
3) Nahraj test_assets/Postcard_Atari_Rocky.xex.
4) Napis jen: posun ano/ne, nebo pruhy mensi/stejne/horsi.
5) Kdyz ne, posli screenshot + LOG / CHYBA.

Log marker:
AtariHelp.eu EMU-10 BUILD2KP_G2F_PMG_LINE_STAGING_CORE pripraven
GTIA G2F PMG LINE STAGING BUILD2KP
