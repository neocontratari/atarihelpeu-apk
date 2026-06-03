AtariHelp.eu EMU-09 FIX97 TEXT SCANLINE + MONTEZUMA PMG CORE

FIX97 je opatrný strukturální balík po potvrzení FIX96: Donkey Kong a Super Cobra zůstávají na bezpečném návratu/stabilním základu, Arkanoid menu se zachová bez automatického START/FIRE a opravuje se dvojitý nadpis přes správné ANTIC 5/7 scanline natažení. Montezuma dostává cílenou korekci viewportu a PMG Y offsetu: playfield se už necentruje dolů a single-line PMG se neposouvá o -24 řádků, aby lebka/hráč seděli k cihlám.

Změny:
- ANTIC text/char mode 5 a 7: 16-scanline znakové módy teď natahují každý font řádek 2× místo opakování celého znaku dvakrát.
- Arkanoid III: menu zůstává bez auto-assistu, dvojitý nadpis by se měl změnit na jeden vysoký/stretched nadpis.
- Montezuma: viewport yOff forced 0 a PMG single-line yOff forced 0 pro lepší srovnání lebky/hráče s cihlami.
- Donkey/Cobra: žádný návrat agresivního scan-DLIST promotion z FIX95.
- Snapshot diagnostika přidává MONTEZUMA FIX97 řádek.

Commit Summary:
FIX97 text scanline Montezuma PMG core

Test:
1) Donkey Kong: jen potvrdit, že hra běží jako FIX96.
2) Super Cobra: jen potvrdit návrat/stabilitu jako FIX96.
3) Arkanoid III: menu 3–5 s bez mačkání, screenshot nadpisu; potom FIRE/START a screenshot hry.
4) Montezuma: screenshot stejné obrazovky s lebkou a SNAPSHOT LOG.
