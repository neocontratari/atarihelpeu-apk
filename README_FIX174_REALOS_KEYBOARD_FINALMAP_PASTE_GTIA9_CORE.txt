AtariHelp.eu EMU-09 FIX174 REALOS KEYBOARD FINALMAP PASTE GTIA9 CORE

Navazuje na FIX173. Cíl: neopouštět stabilní REAL OS/BASIC stav, ale opravit poslední klávesnicové chyby z testu Reného.

Změny:
- build tag FIX174_REALOS_KEYBOARD_FINALMAP_PASTE_GTIA9_CORE
- zachován teplý RESET: LIST/NEW fungují jako ve FIX173
- mikrofix REAL OS/BASIC klávesnice: = a > mají fallback podle Reného testu
- ? drží přímý ATASCII one-shot déle, aby ho REAL OS nepřeložil na mezeru
- BASIC TXT / VLOZIT PROGRAM pumpuje text pomaleji, aby se neztrácely znaky
- GRAPHICS 9/10/11 zůstává z FIX170/FIX173, přidána diagnostika FIX174 nibble renderer
- žádné herní per-game hacky

Commit summary:
FIX174 realos keyboard finalmap paste gtia9 core

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. Otestovat: LIST, NEW.
3. Otestovat ručně: PRINT 2+3, A=5, PRINT A, PRINT 2>1, PRINT ?, PRINT "AHOJ".
4. Otestovat BASIC TXT / VLOZIT PROGRAM s krátkým programem.
5. Pak GRAPHICS 9/10/11.
6. Poslat screenshot + snapshot/log.
