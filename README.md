AtariHelp.eu EMU-09 FIX204_GTIA_REGMAP_COLOR_CORE

Hotovy overlay pro GitHub Desktop. Rozbalit presne do lokalniho repo pres existujici soubory, commit/push, GitHub Actions postavi APK.

Commit summary:
FIX204 gtia regmap color core

Proc tento fix:
FIX203 proveril, ze problem neni jen XEX loader ani DLI. GTIA 9/10/11 VERIFY proti Altirre a realnemu Atari 130XE ukazal spatny zaklad v GTIA mapovani barev.

Hlavni zmeny:
- GRAPHICS 10 / GTIA $80 ma opravene mapovani: 0-3 PCOLR0-3, 4-7 COLPF0-3, 8 COLBK.
- GRAPHICS 11 / GTIA $C0 ma opravene pozadi: pixel 0 = COLBK hue s luminanci 0, ne seda plocha z COLBK luminance.
- GRAPHICS 9 / GTIA $40 nula/clear uz nepada do forced black.
- Snapshot pridava GTIA REGMAP FIX204, kde je videt tabulka hodnot pro nibbles 0-F.
- Bez hernich hacku; obecny GTIA renderer zaklad.

Test plan:
1. GTIA 9 VERIFY + screenshot + snapshot.
2. GTIA 10 VERIFY + screenshot + snapshot.
3. GTIA 11 VERIFY + screenshot + snapshot.
4. Kratky smoke test Donkey Kong a Super Cobra.
