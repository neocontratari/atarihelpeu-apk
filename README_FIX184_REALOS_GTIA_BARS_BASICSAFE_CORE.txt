AtariHelp.eu EMU-09 FIX184_REALOS_GTIA_BARS_BASICSAFE_CORE

Commit summary:
FIX184 realos gtia bars basicsafe core

Cil:
- Navazuje na FIX183, ale neplete GTIA diagnostiku s BASIC editorem.
- GRAPHICS 7 zustava realny BASIC test.
- GTIA 9/10/11 tlacitka jsou ted jasne diagnosticke pruhove obrazce, ne BASIC program.
- Klavesnice se nemeni.

Zmeny:
- GTIA 9/10/11 test pattern zmenen z chaotickeho checker/noise vzoru na stabilni svisle pruhy.
- GTIA logika opravena v auditu: GTIA 9/10/11 pres ANTIC F je 80 logickych GTIA pixelu, ne 160.
- GTIA direct buffer zustava izolovany: DLIST $3F00, screen $4000.
- Snapshot doplnen o GTIA BARS FIX184.
- GRAPHICS 7 EDGE TEST je realny BASIC test 160x96 bez mazaci COLOR 0 smycky a bez primeho prepisu RAM.
- BASIC/READY/keyboard zustavaji z predchozich stabilnich oprav.

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. RRRR + ENTER, RESET, READY.
3. LET A=5, ? "AHOJ", PRINT 2>1.
4. GRAPHICS 7 EDGE TEST.
5. Rucni GRAPHICS 7 program.
6. GTIA 9 TEST, GTIA 10 TEST, GTIA 11 TEST.
   Pozor: GTIA tlacitka v FIX184 nepisi BASIC kod, jen rovnou zobrazi diagnosticky pruhovy obraz.
7. Snapshot/log: GTIA BARS FIX184, GTIA RENDER, ANTIC GRID.
