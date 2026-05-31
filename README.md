# AtariHelp.eu EMU-09 FIX65 CORE COMPAT OS + 130XE + TEST MATRIX

FIX65 meni smer z lepeni jedne hry na jadro emulatoru.

Co je v tomhle buildu:

- CPU/OS: centralni Atari OS ROM stub pro vstupy do $E000-$FFFF, vcetne SETVBV, CIOV/SIOV a bezpecneho navratu z prazdnych OS rutin.
- BASIC/cart prostor: prazdne $A000-$BFFF se pri JSR/JMP nebere jako okamzita smrt hry, pokud tam XEX nic nenahral.
- 130XE: zakladni PORTB $D301 bank switching pro 16KB okno $4000-$7FFF.
- XEX loader: oznacuje skutecne nahrane XEX segmenty, aby emulator rozlisil program v $A000-$BFFF od prazdne ROM oblasti.
- CPU kompatibilita: doplnen undocumented SBC immediate opcode $EB a RTCLOK carry pres $14/$13/$12.
- Diagnostika: nove tlacitko CORE TEST MATRIX spusti vestavene smoke testy PiTT-KiTT, Super Cobra, Pitstop II, Arkanoid III a River Raid.

Co testovat jako prvni:

1. CORE TEST MATRIX - musi ukazat RUN bez trapu pro vestavene testy.
2. Super Cobra - test obecneho XEX/INITAD/loaderu.
3. Pitstop II - test VBI/SETVBV/DLIST/CPU.
4. Arkanoid III - test dlouhych segmentu a tvrdejsiho loaderu.
5. Vlastni XEX pres NACIST VLASTNI XEX: Pac-Man, Donkey Kong, Super Cobra jiny dump.

Poznamka:

River Raid zustava test, ale neni hlavni smer. Cilem je obecny Atari 130XE/XEX emulator pro co nejvetsi mnozstvi her.

GitHub Desktop summary:

FIX65 core compat os 130xe test matrix
