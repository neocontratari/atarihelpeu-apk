# AtariHelp.eu EMU-09 FIX205_GTIA_PALETTE_LUMA_CORE

KODY JSOU STEJNE.

Tento overlay navazuje na FIX204. Nejsou pridane zadne herni berlicky. Cilem je ciste obecne jadro: Real OS/BASIC, GTIA 9/10/11, ANTIC mode F a pozdeji znovu cisty XEX loader + DLI/WSYNC/PMG timing.

Hlavni opravy:
- GRAPHICS 9 pouziva realny GTIA luminance vzorec `COLBK_hue | ((COLBK_lum & $0E) | pixelValue)`.
- GRAPHICS 10 ma celou tabulku pixelu 0-F: PCOLR0-3, COLPF0-3, COLBK, COLBK, COLBK, COLBK, COLPF0-3.
- Atari hue/paleta je prepsana podle realneho poradi barev Atari: rust, red-orange, dark orange, red, lavender, cobalt, ultramarine, medium blue, dark blue, blue-grey, olive, green, dark green, orange-green, orange.
- Snapshot ukazuje `GTIA REGMAP FIX205` a `GTIA PALETTE FIX205`.

Commit summary: `FIX205 gtia palette luma core`
