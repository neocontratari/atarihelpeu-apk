AtariHelp.eu EMU-09 FIX170 REALOS GTIA 9 10 11 CORE
FIX170_REALOS_GTIA_9_10_11_CORE

Commit summary:
FIX170 realos gtia 9 10 11 core

Co je v tomto overlayi:
- navazuje na potvrzeny FIX169
- READY / monitor border / BASIC klavesnice / uvozovky beze zmeny
- GRAPHICS 3 a GRAPHICS 7 nechane funkcni
- pridana obecna GTIA interpretace pro GRAPHICS 9/10/11
- PRIOR $40 = GTIA 9: 16 odstinu jedne barvy
- PRIOR $80 = GTIA 10: 9 barev z GTIA registru
- PRIOR $C0 = GTIA 11: 16 hue barev se spolecnou luminanci
- DLI band a PMG/PRIOR sync z FIX169 zustavaji zachovane
- zadne per-game hacky

Test plan pro Reneho:
1. REAL OS BASIC AUTO BOOT - READY musi zustat jako FIX167/FIX169.
2. GRAPHICS 3 test - musi zustat spravne ctvereckovy.
3. GRAPHICS 7 test - potvrdit, ze stale kresli barevne cary.
4. GRAPHICS 9 / 10 / 11 jednoduche PLOT/DRAWTO testy.
5. Snapshot/log - hledej GTIA MODE FIX170, GTIA SYNC FIX170 a GTIA DLI BAND FIX170.
6. Rychly smoke Donkey / Cobra / Montezuma, jestli se nerozbily barvy nebo PMG.
