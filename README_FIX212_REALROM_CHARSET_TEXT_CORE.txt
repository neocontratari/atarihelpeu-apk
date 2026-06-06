AtariHelp.eu EMU-09 FIX212_REALROM_CHARSET_TEXT_CORE

KODY JSOU STEJNE.
SUBMARINE BASIC kod beze zmeny. GTIA 9/10/11 kody beze zmeny.

Oprava:
- FIX211 rychly CH loader zustava.
- REAL OS ROM je opravdu vlozena; FIX212 ji pouziva i pro znakovy font.
- ANTIC 6/7 / GRAPHICS 17 / BASIC text20 uz pri CHBASE=$00 nebo $E0 nebere synteticky 5x7 font.
- Renderer bere znakove bajty z ATARIXL.ROM charsetu na $E000 (offset $2000 v 16KB OS ROM).
- Bez prepisu RAM, bez hernich hacku. Jen korektnejsi textovy renderer pod kapotou.

Snapshot hledej:
- BUILD TAG FIX212_REALROM_CHARSET_TEXT_CORE
- FIX212 ROM CHARSET
- FIX211 CH LATCH CLEAR
- REAL ROM AUDIT FIX209/FIX212

Test:
1) SUBMARINE FAST
2) BASIC TXT FAST s kratkym GRAPHICS 17 testem
3) screenshot + snapshot TXT
