# AtariHelp.eu EMU-01 - XEX Emulator Lab

Tahle větev vychází ze stabilní v25, ne z rozbité v27.

## Proč

Mobilní port je fajn, ale není to původní Atari hra.
Cíl EMU větve je spustit skutečný XEX.

## Co EMU-01 dělá

- přidá samostatnou stránku emu01_xex_lab.html
- načte PiTT-KiTT XEX z assets
- rozparsuje XEX segmenty
- vypíše RUNAD / INITAD
- vytvoří 64KB RAM obraz
- nahraje segmenty do RAM
- založí první jednoduchý 6502 CPU stav
- umí STEP CPU
- umí RUN 1000 instrukcí
- když narazí na nepodporovaný opcode, vypíše ho

## Co EMU-01 ještě nedělá

- není to plná emulace
- neumí celý 6502
- neumí ANTIC/GTIA obraz
- neumí POKEY
- neumí joystick PORTA/TRIG0
- neumí Atari OS/CIO

## Další kroky

EMU-02: doplnit 6502 instrukce podle toho, kde se PiTT-KiTT zastaví.
EMU-03: OS/vektorové stuby.
EMU-04: obraz ANTIC/GTIA minimum.
EMU-05: joystick.
EMU-06: zvuk.
