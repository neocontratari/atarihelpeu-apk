FIX162_REALOS_GTIA_KEYBOARD_CORE

Commit summary:
FIX162 realos gtia keyboard core

Co je opraveno:
- REAL OS/BASIC zůstává bez falešného READY.
- Klávesnice píše pro BASIC trvale velkými písmeny.
- Doplněné uvozovky, SHIFT symboly (! # $ % & ' ( ) ?), dvojtečka, ESC/TAB/DEL/RETURN/RESET.
- Klávesnice je kompaktnější, RETURN už není mezi písmeny.
- ANTIC mode 8 / BASIC GRAPHICS 3 je opravený jako 2bpp čtyřbarevný režim s 10 bajty na řádek a 8x šířkou pixelu.
- REALOS textová obrazovka si drží bezpečnou modrou paletu, ale bitmapové BASIC GRAPHICS režimy už používají skutečné COLBK/COLPF0-3 a logují PRIOR.
- Storage guard z FIX161 zůstává: žádné automatické soubory při běhu, jen snapshot / uložit log.
- Donkey / Super Cobra / Montezuma zůstávají chráněné baseline.

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. Zkus: PRINT 2+2
3. Zkus program po řádcích:
   10 GRAPHICS 3
   20 COLOR 1:PLOT 10,10:DRAWTO 20,20
   30 GOTO 30
   RUN
4. Otestuj uvozovky: 10 PRINT "AHOJ" / RUN.
5. Otestuj SHIFT, :, DEL, RETURN, RESET.
6. Krátce Donkey / Cobra / Montezuma, jestli baseline zůstal.

V logu hledej:
BUILD TAG FIX162_REALOS_GTIA_KEYBOARD_CORE
REAL OS GTIA FIX162
REAL OS SCREEN FIX162
