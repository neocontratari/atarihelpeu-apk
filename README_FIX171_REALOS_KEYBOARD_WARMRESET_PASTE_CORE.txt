AtariHelp.eu EMU-09 FIX171_REALOS_KEYBOARD_WARMRESET_PASTE_CORE

Overlay navazuje na FIX170 a opravuje BASIC praci pred dalsim GTIA testovanim.

Obsah:
- app/src/main/assets/emu09_pmg_gtia_overlay.html
- app/src/main/assets/index.html
- ROM soubory zustavaji soucasti overlaye

Commit summary:
FIX171 realos keyboard warmreset paste core

Zmeny:
- Klavesa = ma posilat spravne BASIC rovnítko, ne sipku/graficky znak.
- REAL OS keyboard ma navic ATASCII shadow pro symboly a TXT import.
- RESET v REAL OS/BASIC je teply reset pres WARMSV $E474, RAM a BASIC program se nema mazat.
- NEW zustava BASIC prikaz pro smazani programu.
- Pridano BREAK tlacitko.
- Pridany CTRL sipky na mobilni klavesnici a podpora Ctrl+Arrow z HW klavesnice.
- Pridan BASIC TXT import/paste: BASIC TXT a VLOZIT PROGRAM.
- GTIA 9/10/11 z FIX170 zustava, ale hlavni test tohoto fixu je klavesnice/reset/import.

Test plan:
1. Spust REAL OS BASIC AUTO BOOT.
2. Zkus PRINT 2+3 a potom A=5:PRINT A, hlavne znak =.
3. Napis maly program, treba:
   10 PRINT "AHOJ PARTAKU"
   20 GOTO 10
   RUN
4. Stiskni RESET. Po READY dej LIST. Program by mel zustat v pameti.
5. Dej NEW a potom LIST. Program se ma smazat.
6. Zkus BASIC TXT nebo VLOZIT PROGRAM a pockej, az se radky dopisou.
7. Volitelne zkus BREAK pri bezicim programu.
8. Pak teprve pokracuj na GRAPHICS 9/10/11 testy z FIX170.
