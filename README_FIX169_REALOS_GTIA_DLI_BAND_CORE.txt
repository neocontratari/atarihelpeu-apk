AtariHelp.eu EMU-09
FIX169_REALOS_GTIA_DLI_BAND_CORE

Commit summary:
FIX169 realos gtia dli band core

Co je nove:
- FIX168 je potvrzeny stabilni bod: REAL OS READY monitor, BASIC klavesnice, uvozovky a GRAPHICS 3 zustavaji beze zmeny.
- GTIA jde o krok dal: DLI snimky se pouzivaji nejen pro COLPF/COLBK, ale i pro PCOLR0-3, missile barvy a PRIOR/fifth-player bit po scanline.
- PMG overlay ted vybira barvy podle aktualni DLI bandy, ne jen podle jedne globalni palety z konce frame.
- Kolizni vyhodnoceni PMG proti playfieldu dostava scanline color set, coz je priprava pro Cobru/Donkey/Galaxian bez hernich hacku.
- River/PiTT specialni cesty zustavaji chranene; READY/GRAPHICS 3 nebyly prekopane.

Test plan:
1. REAL OS BASIC AUTO BOOT - READY obraz musi zustat jako FIX167/FIX168.
2. PRINT 2+2, uvozovky, RUN - klavesnice zustava hotova.
3. GRAPHICS 3 - cerna graficka plocha + modry textovy pruh.
4. Snapshot/log - hledej GTIA SYNC FIX169 a GTIA DLI BAND FIX169.
5. Rychly smoke Donkey / Super Cobra / Montezuma. Hlavne sleduj, jestli PMG barvy nebo kolize nejsou horsi.
