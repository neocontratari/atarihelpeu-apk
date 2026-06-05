AtariHelp.eu EMU-09 FIX172 REALOS KEYBOARDMAP WARMRESET PASTE GTIA CORE

Navazuje na FIX171/FIX170.

Co je nove:
- REAL OS/BASIC klavesnice jde pres vlastni KEYCODE->ATASCII mapu pres KEYDEF tabulku v page 6.
- Oprava klaves: =, <, >, (, ), uvozovky, apostrof, interpunkce.
- CTRL sipky posilaji primo ATASCII kurzorove kody: UP/DOWN/LEFT/RIGHT.
- BREAK zustava zachovany.
- REAL OS RESET zustava teply reset bez mazani BASIC programu; mazani patri prikazu NEW.
- BASIC TXT / VLOZIT PROGRAM uz posila radky pres KEYCODE/ATASCII frontu, ne pres spatnou fyzickou scan mapu.
- GTIA GRAPHICS 9/10/11 z FIX170 zustava v kodu k okamzitemu testu.
- READY/monitor border, GRAPHICS 3 a GRAPHICS 7 zustavaji z potvrzeneho FIX169/FIX170 stavu.
- XEX hry nejsou hackovane; realos klavesnicova identita se zapina jen v profilu realos.

Commit summary:
FIX172 realos keyboardmap warmreset paste gtia core

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. Otestuj prikazy: PRINT 2+3, A=5, PRINT A.
3. Otestuj: PRINT 1<2, PRINT 2>1, PRINT (2+3)*4.
4. Program: 10 PRINT "AHOJ PARTAKU" / 20 GOTO 10 / RUN.
5. BREAK ma zastavit beh.
6. RESET -> LIST: program ma zustat. NEW -> LIST: program se ma smazat.
7. BASIC TXT / VLOZIT PROGRAM: vloz program s =, <, >, (), uvozovkami.
8. Pak testuj GRAPHICS 9/10/11 z FIX170.

Snapshot ma obsahovat:
- BUILD TAG FIX172_REALOS_KEYBOARDMAP_WARMRESET_PASTE_GTIA_CORE
- REAL OS KEYBOARD MAP FIX172
- GTIA MODE FIX170 nebo navazujici GTIA radky pri GRAPHICS 9/10/11.
