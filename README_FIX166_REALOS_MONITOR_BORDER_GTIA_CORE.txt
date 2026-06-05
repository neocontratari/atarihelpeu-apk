AtariHelp.eu EMU-09 - FIX166_REALOS_MONITOR_BORDER_GTIA_CORE

Pro Reneho / GitHub Desktop overlay.

Co FIX166 meni:
- FIX165 textblank smer se NEpouziva: realny Atari monitor ma kolem hlavniho okna cernou oblast.
- REAL OS/BASIC renderer ted explicitne oddeluje cerny monitor/border od modreho editor/playfield okna.
- ANTIC blank radky v REAL OS testu zustavaji cerne jako monitor/border zona.
- Textove BASIC radky zustavaji modre s bilym READY/kurzorem.
- GRAPHICS 3 oprava z FIX164 zustava: mode 8, 10 bajtu na radek, 8 scanline, velke ctverecky, dole modry textovy pruh.
- BASIC klavesnice a uvozovky zustavaji z FIX164/FIX166 zachovane.
- XEX herni baseline zustava chraneny; zadne per-game hacky.

Commit summary:
FIX166 realos monitor border gtia core

Test plan:
1) REAL OS BASIC AUTO BOOT.
2) READY screen: zkontroluj cerny monitor/border nahore/dole a modre Atari okno s bilym READY a kurzorem.
3) Zkus PRINT 2+2.
4) Zkus 10 PRINT "AHOJ" / RUN.
5) GRAPHICS 3 test:
   10 GRAPHICS 3
   20 COLOR 1:PLOT 10,10:DRAWTO 20,20
   30 GOTO 30
   RUN
   Ocekavani: cerna graficka plocha, dole modry textovy pruh, cara z velkych Atari ctverecku.
6) Uloz snapshot/log: hledej REAL OS MONITOR FIX166 a build tag FIX166_REALOS_MONITOR_BORDER_GTIA_CORE.
