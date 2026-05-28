# AtariHelp.eu EMU-10 - PMG Debug

EMU-09 ukázal, že 2BPP playfield je OK, ale PMG ONLY je skoro prázdné.

## EMU-10 programuje emulátor, ne zkrášlování
- mirror GTIA/ANTIC registrů
- tracking zápisů do PMG paměti
- scan RAM pro PMG kandidáty
- fallback PMG base podle nejlepšího kandidáta
- PMG debug panel
- DUMP PMG DEBUG
- SCAN PMG MEMORY
- RUN PMG FRAME

## Test
1. Hry -> EMU-10 PMG Debug
2. Load XEX into RAM
3. BOOT GAME / HOLD START
4. RUN PMG FRAME
5. SCAN PMG MEMORY
6. DUMP PMG DEBUG
7. RENDER PMG ONLY
8. pošli PMG debug + PMG ONLY + 2BPP
