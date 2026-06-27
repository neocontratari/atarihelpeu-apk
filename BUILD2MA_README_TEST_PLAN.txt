BUILD2MA_DOC_BLITTER_CONSTANT_COLLISION_SAFE

ZAKLAD:
- BUILD2LR, ne LT/LU/LV/LW/LY/LZ.
- UI, keyboard, joystick, cassette, loader, CLOAD/CSAVE beze zmen.
- KODY JSOU STEJNE.

PROC TENTO BUILD EXISTUJE:
PART04 obsahuje VBXE fx1.26/Altirra/VBXE package dokumentaci. Po kontrole docs je nejsilnejsi skutecny bug v blitteru:
- fx1.26 rika: konstantni zdroj v blitteru jen kdyz blt_and_mask == 0.
- BUILD2LR bral jako konstantu i AND == XOR, napr. FF/FF.
- To je spatne: FF/FF ma invertovat zdroj, ne vyplnit cil $FF.
- Tohle muze primo rozbit Popeye/tile/font/surface blity a nasledne rozhodit VBXE obraz.

ZMENY:
1) VBXE blitter constant source:
   - bylo: constant = (AND==0) OR (AND==XOR)
   - ted: constant = (AND==0)

2) VBXE BCB control byte:
   - podle fx1.26: bit0-2 MODE, bit3 NEXT, bit4-7 rezervovane
   - BUILD2LR pouzival bit4-7 jako sx/sy/dx/dy smerove vlajky
   - BUILD2MA je ignoruje a respektuje signed source/dest step_x/step_y z BCB

3) VBXE blitter collision:
   - SR: segmenty 1..31, 32..63, ... 224..255 podle mask bitu
   - HR: nibble segmenty podle fx1.26

CO TESTOVAT:
1) Popeye VBXE:
   - schody/plosiny: lepsi / stejny chaos / horsi
   - intro horni napis: videt / chybi
   - jestli se chaos uklidnil proti LZ

2) Night Driver:
   - pruh: pryc / stejny / horsi
   - auto: cele / urizle
   Poznamka: tento build nelakuje Night jako hotovy; primarni core rez je blitter.

3) W3D:
   - stejny / horsi / lepsi

4) River Raid jen pokud chces regresi, jinak neni nutne.

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2MA_DOC_BLITTER_CONSTANT_COLLISION_SAFE pripraven
VBXE BLITTER CONST DOC BUILD2MA
VBXE BLITTER CONTROL DOC BUILD2MA
VBXE BLITTER COLLISION DOC BUILD2MA
