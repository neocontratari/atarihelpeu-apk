AtariHelp.eu EMU-09 FIX165_REALOS_TEXTBLANK_GTIA_CORE

Navazuje na FIX164_REALOS_GRAPHICS3_MODELINE_CORE.

Co je opravene:
- REAL OS/BASIC READY obrazovka: ANTIC blank radky pred prvnim textovym radkem se kresli modrou editorovou barvou, ne cernou.
- GRAPHICS 3 zustava z FIX164: cerna graficka plocha, dole modry textovy pruh, velke Atari ctverecky.
- Do snapshotu pribyla diagnostika REAL OS BLANK FIX165.
- XEX herni baseline je chraneny, zadne per-game hacky.

Commit summary:
FIX165 realos textblank gtia core

Test plan pro Reneho:
1) Spustit REAL OS BASIC AUTO BOOT.
2) Vyfotit READY: modra plocha ma byt modra i v hornim blank pruhu nad READY, bily READY a bily/inverzni kurzor.
3) Zadat: PRINT 2+2
4) Zadat: 10 PRINT "AHOJ" / RUN
5) GRAPHICS 3 test:
   10 GRAPHICS 3
   20 COLOR 1:PLOT 10,10:DRAWTO 20,20
   30 GOTO 30
   RUN
6) Ocekavani GRAPHICS 3: cca 75 procent cerna graficka plocha, dole modry textovy pruh, cara z velkych hranatych Atari ctverecku.
7) Poslat screenshot READY, screenshot GRAPHICS 3 a snapshot/log.
