BUILD2LF_WSYNC_REAL_LINEEND_DMA_ROLLBACK_CORE

Co je opraveno:
- LE je zahozene jako horsi vetev.
- ANTIC DMA/HALT kradez cyklu z LE je vypnuta na 0, protoze v testu zhorsila Decathlon a zpomalila nahravani.
- WSYNC je vracen na konec scanline, ne do HBLANK mezikroku z KL.
- Zapisy po WSYNC se maji projevit az na dalsi radce, ne jako bordel v aktualni radce.
- Donkey Arcade / Donkey Junior PMG faze z KV zustava chranena.
- XEX/OBX/ATR fake-fast zustava jen pro nahravani.
- CLOAD/CSAVE/kazeta bez fake zrychleni.
- Bez screen-paintu, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

Test:
1) Decathlon: telo/nohy, napis nahore, rychlost, nahravani.
2) Donkey Arcade: musi zustat OK.
3) Donkey Junior: musi zustat OK a rychlost hry OK.
4) Night Driver: auto/pruh/zvuk.
5) Postcard: okna/pruhy.

V LOGu hledej:
AtariHelp.eu EMU-10 BUILD2LF_WSYNC_REAL_LINEEND_DMA_ROLLBACK_CORE pripraven
ANTIC DMA HALT BUILD2LF: LE kradez cyklu rollback na 0
WSYNC REAL LINEEND BUILD2LF
