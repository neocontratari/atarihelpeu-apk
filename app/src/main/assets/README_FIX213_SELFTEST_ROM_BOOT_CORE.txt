AtariHelp.eu EMU-09 FIX213_SELFTEST_ROM_BOOT_CORE

KODY JSOU STEJNE.
- SUBMARINE BASIC kod zustava stejny jako FIX207-FIX212.
- GTIA 9/10/11 verify kody zustavaji stejne.

Proc FIX213:
- Rene chce poctivy test, ze je v emulatoru opravdu ATARIXL.ROM a ze asistent nekeca.
- Pridano tlacitko ROM SELF TEST: studeny REAL OS boot s OPTION drzenym od resetu.
- Pridano mapovani XL/XE self-test ROM okna $5000-$57FF z ATARIXL.ROM offsetu $1000.
- Pridano tlacitko BYE SELFTEST: z BASICu posle prikaz BYE rychlou CH/KBCODE cestou.

Nove logy:
- BUILD TAG FIX213_SELFTEST_ROM_BOOT_CORE
- FIX213 SELFTEST
- FIX213 SELFTEST ROM MAP
- FIX213 SELFTEST OPTION BOOT
- FIX213 BYE SELFTEST CLICK

Test plan:
1. Nejdriv klikni ROM SELF TEST.
2. Cekej na skutecne Atari SELF TEST menu.
3. Kdyz nenabehne, zkus BASIC READY a tlacitko BYE SELFTEST.
4. Posli screenshot + snapshot TXT.

Poznamka:
Toto neni herni hack ani dalsi duha. Je to test ROM cesty a mapovani self-test ROM regionu.
