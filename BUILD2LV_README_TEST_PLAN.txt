AtariHelp.eu EMU-10 BUILD2LV_CORE_SR_SEGMENT_FILTER_NO_GAME_PATCH

STAV / ROZHODNUTI:
- Zaklad zustava potvrzeny LR/LU: River Raid TOP a loader speed chranen.
- BUILD2LS se porad nepouziva.
- BUILD2LT/LU ukazaly, ze pevne orezani SR radku umi odstranit Night Driver pruh, ale reze i legitimni grafiku auta.
- BUILD2LV proto neni dalsi per-game lepeni. Rusi pevne "skip N radku" a zavadi obecnejsi VBXE SR segmentovy filtr.

ZMENY:
1) VBXE SR LOW-ORIGIN pevny skip vypnut
   - nevyhazuji se cele radky z nizke VRAM.
   - cil: auto nesmi byt ukrojene.

2) VBXE SR SEGMENT FILTER BUILD2LV
   - filtruje jen dlouhe jednobarevne horizontalni behy, ktere sahaji k okraji nebo berou vetsinu sirky.
   - kompaktni objekt uprostred radky se ma kreslit.
   - neni to podle nazvu hry, funguje podle SR/XDL obsahu.

NEMENI SE:
- Loader / fake-fast: beze zmen proti LR/LU.
- River Raid cesta: beze zmen.
- UI / klavesnice / joystick / kazeta: beze zmen.
- CLOAD / CSAVE: beze zmen.
- KODY JSOU STEJNE.

LOG MARKERY:
AtariHelp.eu EMU-10 BUILD2LV_CORE_SR_SEGMENT_FILTER_NO_GAME_PATCH pripraven
VBXE SR LOW-ORIGIN PIXEL FILTER BUILD2LV
VBXE SR SEGMENT FILTER BUILD2LV

NESMI BYT JAKO AKTIVNI NOVY SMER:
ANTIC HSCROL PHASE BUILD2LN
GTIA PMG EARLY HBLANK BUILD2LN
ANTIC CH6 HSCROL SCALE BUILD2LS
GTIA PRIOR04 P2P3 LINE GUARD BUILD2LS

TEST:
1) River Raid
- TOP / horsi
- nahravani rychle / horsi

2) Night Driver
- auto cele / porad ukrojene / horsi
- pruh pryc / vratil se / horsi
- zvuk OK / horsi

3) Decathlon
- ocekavam spis stejny stav, ale sleduj: WELCOME/text, telo/nohy, dolni scroll

4) Popeye VBXE/W3D
- jen rychla regrese: nahravani OK / horsi, grafika stejna / horsi

DULEZITE:
Pokud BUILD2LV nevrati auto a nepridrzi pruh pryc, dalsi krok uz nebude dalsi filtr podle Night Driveru.
Pujde se do sirsiho VBXE XDL/SR/HR pipeline auditu a Decathlon PMG/ANTIC auditu bez per-game hacku.
