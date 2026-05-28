# AtariHelp.eu EMU-08 - ANTIC / Display Lab

Reakce na EMU-07:
Uživatel už vidí první hory a řeku z původní PiTT-KiTT XEX hry.

## Důležité
Nejde o emulátor pro jednu XEX zvlášť.
PiTT-KiTT je testovací hra. Cíl je obecný AtariHelp.eu mini emulátor pro naše XEX hry.

## Co EMU-08 přidává

- nový renderer obrazu
- auto režim:
  - intro přes Display List
  - hra přes 2bit grafiku ze SAVMSC
- převod Atari screen code na text pro intro
- první hrubé playfield barvy
- vykreslení 96 řádků herní grafiky místo původních 24
- diagnostika DLIST / SDLST / SAVMSC
- tlačítka:
  - REFRESH VIDEO AUTO
  - RENDER INTRO DISPLAY LIST
  - RENDER GAME 2BIT RAM

## Test

1. Nainstaluj EMU-08.
2. Hry -> EMU-08 ANTIC Display.
3. Load XEX into RAM.
4. BOOT GAME / HOLD START.
5. REFRESH VIDEO AUTO.
6. Zkus RENDER GAME 2BIT RAM.
7. Pošli screenshot Video preview + PC/log.

Cíl EMU-08:
lepší obraz, ne další port hry.
