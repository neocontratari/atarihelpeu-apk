BUILD2LG_PMBASE_ZERO_DMA_GUARD_PRIORITY_CORE

Zaklad: BUILD2LF rollback + Deep audit Decathlon.

Co je zmenene:
- LF zustava jako stabilizacni rollback: LE DMA/HALT kradez je porad vypnuta.
- Novy obecny PMBASE zero DMA guard: kdyz je PMBASE maskovane $00 a pritom je aktivni PMG DMA,
  emulator nebude cist sprity z nulte stranky/stacku jako nahodny sum. Pouzije existujici software
  GRAFP/GRAFM/beam segmenty.
- Opravena GTIA PRIOR tabulka low nibble po skupinach P0/P1 a P2/P3 proti PF0/PF1/PF2/PF3.
- Donkey Arcade / Donkey Junior PMG faze zustava chranena.
- XEX/OBX/ATR fake-fast zustava jen pro nahravani.
- CLOAD / CSAVE / kazeta bez fake zrychleni.
- Bez screen-paintu, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

V LOGu ma byt:
AtariHelp.eu EMU-10 BUILD2LG_PMBASE_ZERO_DMA_GUARD_PRIORITY_CORE pripraven
GTIA PMBASE ZERO DMA GUARD BUILD2LG
GTIA PRIOR TABLE BUILD2LG

Test nejdriv jen:
1) Decathlon: telo/nohy OK/lepsi/stejne/horsi, napis OK/lepsi/stejne/horsi, rychlost OK/pomala/rychla.
2) Kdyz Decathlon neni horsi: Donkey Arcade a Donkey Junior jen rychla kontrola, ze postavy/zebrik zustaly OK.

Kdyz Decathlon horsi nebo stejny: stop a poslat jeden LOG.
