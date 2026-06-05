AtariHelp.eu EMU-09 FIX205_GTIA_PALETTE_LUMA_CORE

KODY JSOU STEJNE.
GTIA 9 VERIFY, GTIA 10 VERIFY a GTIA 11 VERIFY pouzivaji stejny BASIC kod jako FIX203/FIX204.
Menil se pouze emulator pod kapotou.

Navazuje na FIX204_GTIA_REGMAP_COLOR_CORE.
Cil: cisty obecny GTIA/ANTIC zaklad pro Real OS/BASIC i XEX loader, bez hernich berlicek.
Hry se muzou docasne rozbit, ted se ladi spravne jadro.

Zmeny:
- GRAPHICS 9 / GTIA mode $40:
  * opraven realny vzorec luminance: (COLBK_hue | ((COLBK_lum & $0E) | pixelValue))
  * FIX204 delal hodnoty $88,$81,$82..., coz nesedelo na Altirru/real Atari pri POKE 712,$88.
- GRAPHICS 10 / GTIA mode $80:
  * doplnena cela realna tabulka 0-F:
    0-3 PCOLR0-3, 4-7 COLPF0-3, 8-B COLBK, C-F COLPF0-3.
  * test pouziva hlavne 0-8, ale hry a budouci testy muzou sahnout na 9-F.
- GRAPHICS 11 / GTIA mode $C0:
  * ponechan realny zaklad: pixel 0 = COLBK hue s luminanci 0, pixely 1-F = hue OR pixel s COLBK luminanci.
  * zlepsena obecna Atari hue/palette tabulka, hlavne hue F uz neni seda, ale oranzova podle Atari poradi barev.
- Snapshot nove obsahuje:
  * GTIA REGMAP FIX205
  * GTIA PALETTE FIX205

Test plan:
1. GTIA 9 VERIFY proti Altirre a real Atari 130XE.
2. GTIA 10 VERIFY proti Altirre a real Atari 130XE.
3. GTIA 11 VERIFY proti Altirre a real Atari 130XE.
4. Poslat screenshoty + snapshot TXT.
5. Hry netestovat jako prioritu; jen kdyz bude chut, mohou byt rozbite.

Commit summary:
FIX205 gtia palette luma core
