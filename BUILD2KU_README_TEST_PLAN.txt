AtariHelp.eu EMU-10 BUILD2KU_PMG_VERTICAL_PHASE_AND_GTIA_COLOR_CLOCK_CORE

CIL:
- komplexni graficky krok po KT, ne jednoradkovy pokus.
- Postcard/G2F, Decathlon a Donkey ukazuji stejnou tridu chyb: barvy + PMG masky + playfield nejsou slozene stejne jako real Atari.
- BUILD2KU proto nesaha do VBXE overlaye, ale do spolecne ANTIC/GTIA/PMG pipeline.

ZMENA V KODU:
1) PMG DMA vertical phase
   - playfield renderer kresli viditelne Y jako M.line-8.
   - PMG DMA ted pouziva stejnou viditelnou Y fazi pro line index.
   - cil: Decathlon telo/nohy a G2F barevne masky nesmi byt o radky mimo playfield.

2) GTIA color-clock rendering
   - GTIA 9/10/11 barva a priorita se u hires/m1bpp/m2bpp bitmapy uz nebere jen z leve hrany sirsiho pixelu.
   - pro kazdy vystupni X pixel se znovu bere COLBK/COLPF/COLPM/PRIOR z realneho raster segmentu.
   - cil: Postcard okna/barvy/pruhy a Donkey title obrazek.

3) COLBK normalizace
   - background fill pouziva stejnou GTIA normalizovanou barvu jako ostatni barevne registry.

BEZPECNOST:
- zadny screen-paint.
- zadny hack podle nazvu hry.
- zadna vlozena bitmapa.
- KD PMBASE hack zustava pryc.
- KODY JSOU STEJNE.

LOG MARKERY:
- BUILD2KU_PMG_VERTICAL_PHASE_AND_GTIA_COLOR_CLOCK_CORE
- GTIA PMG VERTICAL PHASE BUILD2KU
- GTIA COLOR CLOCK BUILD2KU
- GTIA BITMAP MODES BUILD2KU

TEST:
1) Postcard_Atari_Rocky.xex
   Ocekavani: barvy v oknech a pruhy posun lepsi / stejne / horsi.
2) The Activision Decathlon.xex
   Ocekavani: prvni panacek - telo/nohy lepsi / stejne / horsi; napis nahore.
3) Donkey title verze
   Ocekavani: celistvost uvodniho obrazku + zvuk ano/ne.
4) Donkey arcade
   Ocekavani: nesmi se rozbit, zvuk a rychlost jako ted.
