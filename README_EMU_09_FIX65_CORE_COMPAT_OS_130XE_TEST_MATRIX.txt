EMU-09 FIX65 CORE COMPAT OS + 130XE + TEST MATRIX

Summary pro GitHub Desktop:
FIX65 core compat os 130xe test matrix

Co se ma testovat:
- CORE TEST MATRIX: vestavene testy PiTT-KiTT, Super Cobra, Pitstop II, Arkanoid III, River Raid bez okamziteho trapu.
- Super Cobra: overit start/menu a prvni beh.
- Pitstop II: overit menu/jmeno/zavod jako tezsi DLIST/VBI test.
- Arkanoid III: overit, jestli se dostane dal nez jen load/trap.
- Vlastni XEX: Pac-Man a Donkey Kong pres NACIST VLASTNI XEX.

Co je zmenene:
- Centralni OS ROM stub pro skoky/volani do $E000-$FFFF.
- Bezpecnejsi prazdny BASIC/cart prostor $A000-$BFFF, pokud tam XEX nenahral vlastni segment.
- 130XE PORTB $D301 bank switching pro $4000-$7FFF.
- XEX loader znaci nahrane rozsahy, aby emulator nepletl XEX kod v ROM prostoru s prazdnou ROM.
- CPU opcode $EB a RTCLOK $14/$13/$12.
- Nove tlacitko CORE TEST MATRIX.

Poznamka:
Tohle neni River-only fix. River je jen jeden test vedle Super Cobra, Pitstop II, Arkanoid III a vlastnich XEX.
