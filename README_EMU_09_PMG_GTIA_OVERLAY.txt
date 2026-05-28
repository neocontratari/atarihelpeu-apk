# AtariHelp.eu EMU-09 - PMG / GTIA Overlay

Reakce na EMU-08:
Uživatel vidí hory, řeku a silnici z původní XEX hry, ale bez aut.

## Proč
To je očekávané. Atari hry často kreslí auta, oko, střely a scanner přes PMG:
Player/Missile Graphics jsou mimo hlavní playfield/SAVMSC.

## Co EMU-09 přidává

- první PMG overlay renderer
- čte PMBASE $D407
- čte HPOSP0-3 $D000-$D003
- čte HPOSM0-3 $D004-$D007
- čte SIZEP0-3 $D008-$D00B
- čte SIZEM $D00C
- používá PCOLR0-3 shadow barvy $02C0-$02C3
- overlay nad herním 2BPP playfieldem
- tlačítko RENDER PMG ONLY pro diagnostiku

## Test

1. Nainstaluj EMU-09.
2. Hry -> EMU-09 PMG Overlay.
3. Load XEX into RAM.
4. BOOT GAME / HOLD START.
5. RENDER GAME 2BIT RAM.
6. RENDER PMG ONLY.
7. Pošli screenshot obou obrazů a log.
