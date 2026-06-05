FIX192_XEX_STREAM_INIT_LOADER_CORE

- Navazuje na potvrzeny FIX187/FIX188/FIX189/FIX191 baseline.
- FIX190 REAL OS DIRECT pro hotove XEX hry zustava vypnuty.
- CISTY XEX DONKEY/COBRA zustava jako stabilni baseline.
- Pridan volitelny XEX LOADER CODE rezim:
  - segmenty se nacitaji jako normalni XEX,
  - mezilehle INITAD ($02E2/$02E3) se spusti okamzite pri nacitani, jeste pred dalsimi segmenty,
  - finalni INIT-only vstup zustava jako ENTRY, aby se nerozbila Cobra-style cesta,
  - BASIC ROM direct je vypnuty.
- Pridana tlacitka XEX LOADER CODE DONKEY / COBRA.
- Snapshot pise XEX LOADER CODE FIX192 a seznam streamed INITAD adres.
- G7 tunnel, GTIA 9/10/11 BARS, zvuk a pomaly BASIC paste zustavaji.
