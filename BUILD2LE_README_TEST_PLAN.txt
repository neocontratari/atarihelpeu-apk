AtariHelp.eu EMU-10 BUILD2LE_REAL_ANTIC_DMA_STEAL_DECATHLON_CORE

CO JE OPRAVENE:
- LD pevny odhad 5-8 ukradenych cyklu na scanline je nahrazen skutecnejsim ANTIC DMA/HALT budgetem.
- Novy budget pocita:
  * DRAM refresh,
  * playfield / glyph DMA podle aktualniho ANTIC modu,
  * display-list / LMS fetch na zacatku radku,
  * PMG DMA: missiles 1 cyklus + players 4 cykly podle DMACTL/GRACTL.
- GRACTL $D01D maskuje vysoke bity jako realny GTIA, aby zapis typu $9B nezustal jako falesny stav registru.
- Donkey Arcade / Donkey Junior PMG faze z KV/KZ zustava chranena.
- XEX/OBX/ATR fake-fast zustava jen pro nahravani.
- CLOAD / CSAVE / kazeta bez fake zrychleni.
- Bez screen-paintu, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2LE_REAL_ANTIC_DMA_STEAL_DECATHLON_CORE pripraven
ANTIC DMA HALT BUILD2LE
GTIA GRACTL MASK BUILD2LE
VBL/DLI TIMING BUILD2LD

TEST KRATCE:
1) Decathlon:
   - telo/nohy prvniho panacka: OK / lepsi / stejne / horsi
   - napis nahore: OK / lepsi / stejne / horsi
   - rychlost: OK / rychla / pomala
2) Donkey Arcade:
   - clovicek/zebrik OK, zvuk, rychlost
3) Donkey Junior:
   - opicka/zebrik OK, zvuk, rychlost
4) Night Driver:
   - auto, pruh, zvuk
5) Postcard:
   - okna barvy, pruhy

POSLAT ZPET:
Jeden LOG po sade + screenshot jen u Decathlonu, pokud je zmena.
