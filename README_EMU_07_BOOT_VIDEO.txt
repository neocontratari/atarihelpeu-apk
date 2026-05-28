# AtariHelp.eu EMU-07 - Boot / Video Lab

Reakce na test EMU-06:

Screenshot ukazuje PC $22C7 a stav RUN.
To není pád. To je intro smyčka.
Důležité: PiTT-KiTT se podle zdrojáku v intru spouští START přes CONSOL, ne FIRE.

## Co EMU-07 přidává

- BOOT GAME / HOLD START
- drží START, dokud CPU neopustí intro
- SAVMSC stub na $6000
- RTCLOK tick
- RANDOM $D20A stub
- první hrubý Video Preview canvas z obrazové RAM
- RUN VIDEO FRAME
- REFRESH VIDEO

## Test

1. Nainstaluj EMU-07.
2. Hry -> EMU-07 Boot / Video.
3. Load XEX into RAM.
4. BOOT GAME / HOLD START.
5. Pošli PC / Steps / OS / log.
6. Vyfoť i Video preview.
7. Pokud narazí na unsupported opcode, pošli opcode/adresu + TRACE.
