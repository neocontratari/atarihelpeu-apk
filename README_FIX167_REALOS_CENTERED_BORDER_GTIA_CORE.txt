AtariHelp.eu EMU-09
FIX167_REALOS_CENTERED_BORDER_GTIA_CORE

Navazuje na FIX166.

Co opravuje:
- REAL OS/BASIC monitor border uz neni zbytecne posunuty dolu generic viewport centrovanim.
- Pro profil realos se currentRenderYOffset nastavuje na 0, protoze skutecny OS DLIST uz obsahuje horni ANTIC blank radky.
- Vysledek: cerna TV/monitor oblast nad a pod modrym Atari oknem ma byt vyvazenejsi.
- Modre Atari READY/BASIC okno zustava modre, text a kurzor bile.
- GRAPHICS 3 modeline oprava z FIX164 zustava zachovana.
- BASIC klavesnice, SHIFT symboly a uvozovky zustavaji zachovane.
- XEX hry nejsou cilene hackovane.

Commit summary:
FIX167 realos centered border gtia core

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. READY screen: cerna oblast nahore i dole ma byt podobne velka, uvnitr modre Atari okno.
3. PRINT 2+2.
4. 10 PRINT "AHOJ PARTAKU" / RUN.
5. GRAPHICS 3 test: cerna graficka plocha, dole modry textovy pruh, velke Atari ctverecky.
6. Poslat screenshot READY + GRAPHICS 3 a snapshot/log.
