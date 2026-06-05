FIX164_REALOS_GRAPHICS3_MODELINE_CORE

Commit summary:
FIX164 realos graphics3 modeline core

Co je opraveno proti FIX163:
- REAL OS/BASIC zustava bez kresleneho/falesneho READY.
- Build tag a snapshoty jsou prepnute na FIX164_REALOS_GRAPHICS3_MODELINE_CORE.
- ANTIC map/bitmap rezimy 8-F uz berou pamet jako bytes-per-mode-line, ne jako bytes-per-scanline.
- BASIC GRAPHICS 3 / ANTIC mode 8 ted opakuje jeden 10bajtovy radek pres 8 scanline: ma vzniknout hruba Atari ctvereckova grafika, ne jemna rozbita cara.
- GRAPHICS 3 test ma mit cca 75 % graficke plochy a dole textovy editorovy pruh z REAL OS.
- REAL OS textove radky zustavaji modre s bilym/svetlym textem a inverzni kurzor ma byt videt jako bily blok.
- OS keyboard rutina pro REAL OS/BASIC vraci ATASCII znak, ne surovy scan code. To pomaha uvozovkam a SHIFT symbolum.
- Donkey / Super Cobra / Montezuma baseline zustava chraneny; zadne per-game hacky kvuli BASIC testu.

Test plan pro Reneho:
1. Spust REAL OS BASIC AUTO BOOT.
2. Vyfot modrou READY obrazovku: ma byt modra plocha, bily READY a bily/inverzni kurzor.
3. Zadej: PRINT 2+2 a ENTER. Ocekavani: 4 a potom READY.
4. Zadej:
   10 PRINT "AHOJ"
   RUN
   Ocekavani: uvozovky jsou videt pri psani a program vypise AHOJ.
5. Zadej GRAPHICS 3 test:
   10 GRAPHICS 3
   20 COLOR 1:PLOT 10,10:DRAWTO 20,20
   30 GOTO 30
   RUN
   Ocekavani: cca 75 % cerna graficka plocha, dole modry textovy pruh, cara z velkych hranatych Atari ctverecku.
6. Udelej snapshot/log a posli screenshot READY + screenshot GRAPHICS 3 + log.
7. Az potom kratce prover Donkey Kong original, Super Cobra a Montezuma PRELIM, jestli baseline nezregresoval.

V logu hledej:
BUILD TAG FIX164_REALOS_GRAPHICS3_MODELINE_CORE
REAL OS GTIA FIX164
REAL OS SCREEN FIX164
ANTIC MODELINE FIX164
