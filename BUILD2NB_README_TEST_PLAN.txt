BUILD2NB_SEGA_NO_ART_ROM_PICKER_FORCE_STAGE15

TEST PLAN:
1. Rozbal ZIP pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. Klikni kratce ULOZENE NEBO monitor NEBO HRY/SBIRKA/CARTRIDGE.
5. Musi se otevrit picker ROM.
6. Vyber Sonic .gen nebo Aladdin .gen.
7. Pockej 8-12 sekund.
8. Dlouhym stiskem ULOZENE uloz servisni LOG a posli screenshot.

CEKANY LOG:
- SEGA ROM PICKER BUILD2NB OPEN reason=...
- SEGA ROM PICKER BUILD2NB CHANGE file=...
- SEGA ROM HEADER BUILD2NB file=...
- loadRom prijato title=...
- ENGINE SOURCE SCAN / CLOSURE EVAL OK / embedGenesis...

KDYZ LOG RIKA POSLEDNI ROM: zatim nic nenacteno, ROM picker nebyl potvrzeny nebo soubor nebyl vybran.
