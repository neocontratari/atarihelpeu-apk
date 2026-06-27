AtariHelp.eu EMU-10 BUILD2MB_DOC_FX126_REGMAP_PRIORITY_SAFE
===========================================================

STATUS
------
Zaklad: BUILD2LR + BUILD2MA doc blitter fix.
Nezakladat na LT/LU/LV/LW/LY/LZ.
UI / loader / klavesnice / joystick / kazeta / CLOAD / CSAVE beze zmen.
KODY JSOU STEJNE.

PROC TENTO BUILD EXISTUJE
-------------------------
Po kontrole Lotharek VBXE page + release DOCS/CORES a Pigwa/Atari archive + PART04 VBXE baliku se ukazalo, ze aktualni core je FX 1.26 a ze registr mapa je jina nez stare MSEL komentare v kodu.

Konkretni oprava:
- FX1.26 D644 = CSEL, D645 = PSEL, D646 = CR, D647 = CG, D648 = CB.
- FX1.26 D655-D658 = prime P0-P3 priority registry.
- MSEL/RGB a MSEL/PRIORMAP mechanismus je v FX1.26 odstraneny.
- Proto BUILD2MB rusi chybny legacy MSEL/PRIORMAP trigger na D644.

Duvod: Stary kod mohl pri zapisu CSEL=$80-$BF omylem prepsat P0-P3 priority podle aktualnich palette registru. To muze rozbit atributovou mapu, vrstvy a Popeye/Night grafiku. Tohle neni filtr ani herni hack podle nazvu.

ZACHOVANO Z BUILD2MA
--------------------
- Blitter constant source jen AND maska $00.
- BCB21 control: MODE bits0-2, NEXT bit3, b4-b7 reserved.
- Smer pres signed source_step_x/y a dest_step_x/y.
- Blitter collision podle segmentu SR/HR z fx1.26.

LOG MARKERY
-----------
AtariHelp.eu EMU-10 BUILD2MB_DOC_FX126_REGMAP_PRIORITY_SAFE pripraven
VBXE FX126 REGMAP BUILD2MB
VBXE PRIOMAP DIRECT BUILD2MB
VBXE BLITTER CONST DOC BUILD2MB
VBXE BLITTER CONTROL DOC BUILD2MB
VBXE BLITTER COLLISION DOC BUILD2MB

TEST PLAN
---------
1) Popeye VBXE:
   - intro: horni napis je / neni
   - schody/plosiny: lepsi / stejne / horsi
   - chaos proti LZ/LY: mensi / stejny / horsi

2) Night Driver:
   - pruh: pryc / stejny / horsi
   - auto: cele / urizle / horsi

3) W3D:
   - stejny / lepsi / horsi

4) Decathlon jen regrese:
   - stejny / lepsi / horsi

POZNAMKA
--------
River Raid netreba znovu testovat, pokud se uz nechce. Tenhle build nesaha na plain path.
