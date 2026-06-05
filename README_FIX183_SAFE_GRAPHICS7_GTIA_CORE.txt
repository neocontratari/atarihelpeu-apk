AtariHelp.eu EMU-09 FIX183 SAFE GRAPHICS7 GTIA CORE

Cil:
- Stabilizovat BASIC/READY po regresi z FIX180-FIX182.
- Nechat klavesnici beze zmeny.
- Vratit GRAPHICS 7 jako referencni funkcni rezim.
- GTIA 9/10/11 testy izolovat tak, aby neprepisovaly REAL OS/BASIC DLIST a editorovou RAM.

Dulezita oprava:
- FIX179-FIX182 direct GTIA test pouzival screen buffer $8150 a plnil 7680 bajtu.
- To sahalo az priblizne k $9F50 a prepisovalo oblast REAL OS/BASIC editoru/DLISTu kolem $9C20/$9C40.
- Proto po RRRR, RESETu nebo po GTIA testech vznikal bordel v hornich radcich a BASIC obrazovka se rozpadla.
- FIX183 presouva GTIA diagnosticky screen buffer na $4000 a DLIST na $3F00.
- Direct GTIA DLIST se uz neuklada jako lastGood REALOS DLIST.
- Po RESET/REAL OS BASIC boot se GTIA direct test ukonci a pointery se vraci na editorovy $9C20/$9C40.

Zmeny:
- build tag: FIX183_SAFE_GRAPHICS7_GTIA_CORE
- safe GTIA direct buffer: DLIST $3F00, screen $4000
- ochrana proti pouziti prosleho GTIA test DLISTu jako BASIC fallbacku
- doplnen chybejici handler pro GRAPHICS 7 EDGE TEST
- GRAPHICS 7 EDGE TEST jede pres BASIC GRAPHICS 7, ne pres prime zasahy do RAM
- klavesnice beze zmeny
- XEX hry bez hernich hacku

Commit summary:
FIX183 safe graphics7 gtia core

Test plan:
1. Spust REAL OS BASIC AUTO BOOT.
2. Napis RRRR a ENTER. Obraz se nesmi rozsypat do nahodneho textu/grafiky.
3. Dej RESET, musi se vratit READY.
4. Otestuj kratce LET A=5, ? "AHOJ", PRINT 2>1.
5. Dej GRAPHICS 7 EDGE TEST.
6. Dej GTIA 9/10/11 TEST jen po jednom a po kazdem zkus RESET/READY.
7. Posli screenshoty a snapshot/log.

