AtariHelp.eu EMU-10 BUILD2KX_FAST_XEX_AND_HBLANK_RASTER_GUARD_CORE

Co je zmenene:
- Navazuje na BUILD2KW/KV.
- Donkey Arcade / Donkey Junior PMG fyzicka faze z KV zustava: postavy u zebriky se nesmi rozbit.
- XEX turbo guard prepsany na PC-aware logiku: turbo drzi behem OS/loaderu (PC v ROM/stage), ale vypne hned po skoku do hry. Cil: vratit rychle nahravani, ale nevratit rychlou hru.
- GTIA/PMG/CHBASE zapisy v HBLANKU po WSYNC se uz neberou jako segment aktualni radky. Zustanou normalne v registrech pro dalsi scanline. Cil: Postcard pruhy / G2F raster bordel / Night Driver caj.
- GTIA color-clock z KU/KV/KW zustava.
- Bez screen-paintu, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

Test:
1) Donkey Arcade: clovicek/zebrik, zvuk, rychlost, nahravani rychlejsi/pomale
2) Donkey Junior: opicka/zebrik, zvuk, rychlost OK/RYCHLA, nahravani
3) Donkey Title: obrazek, zvuk, rychlost OK/RYCHLA
4) Decathlon: telo/nohy, napis nahore, rychlost
5) Postcard: okna barvy, pruhy
6) Night Driver: auto, pruh, zvuk

V LOGu hledej:
BUILD2KX_FAST_XEX_AND_HBLANK_RASTER_GUARD_CORE
XEX FAST LOAD BUILD2KX: pc-aware guard ukoncil turbo loader
GTIA/ANTIC HBLANK NEXTLINE BUILD2KX
GTIA PMG VERTICAL PHASE BUILD2KX
GTIA COLOR CLOCK BUILD2KX
