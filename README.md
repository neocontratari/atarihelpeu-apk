# AtariHelp.eu EMU-09 FIX66 PITT SAFE CORE MATRIX TXT + 130XE

FIX66 opravuje dve zasadni veci z FIX65:

- PiTT-KiTT Remaster je chraneny referencni titul. Core matrix ho nespousti jako pokusny test a po dobehu vraci puvodni stav emulatoru.
- Klik na CORE TEST TXT automaticky ulozi vysledek do TXT souboru pres Android bridge. Na mobilu se nema nic kopirovat rucne.

Core smer:

- CPU/OS: centralni Atari OS ROM stub pro vstupy do $E000-$FFFF, vcetne SETVBV, CIOV/SIOV a bezpecneho navratu z prazdnych OS rutin.
- BASIC/cart prostor: prazdne $A000-$BFFF se pri JSR/JMP nebere jako okamzita smrt hry, pokud tam XEX nic nenahral.
- 130XE: zakladni PORTB $D301 bank switching pro 16KB okno $4000-$7FFF.
- XEX loader: oznacuje skutecne nahrane XEX segmenty, aby emulator rozlisil program v $A000-$BFFF od prazdne ROM oblasti.
- CPU kompatibilita: undocumented SBC immediate opcode $EB a RTCLOK carry pres $14/$13/$12.

Co testovat jako prvni:

1. RESET + VESTAVENY PiTT-KiTT - ověřit, ze referencni hra neni rozbita.
2. CORE TEST TXT - musi ulozit TXT report automaticky a potom vratit puvodni hru.
3. Super Cobra - obecny XEX/INITAD/loader.
4. Pitstop II - VBI/SETVBV/DLIST/CPU.
5. Arkanoid III - dlouhe segmenty a tvrdsi loader.
6. Vlastni XEX: Pac-Man a Donkey Kong pres NACIST VLASTNI XEX.

Altirra smer:

Nesmi to byt seznam hernich zaplat. Spravny smer je vrstva po vrstve: 6502 CPU, pametova mapa XL/XE, PORTB banky, ANTIC display list a DMA, GTIA barvy/PMG/kolize, POKEY/PIA vstupy, OS vektory a interrupt timing.

GitHub Desktop summary:

FIX66 pitt safe core matrix txt 130xe
