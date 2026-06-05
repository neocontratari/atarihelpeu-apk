FIX193_DONKEY_PMG_TRANSITION_CORE

- Navazuje na potvrzeny FIX187/FIX188/FIX191/FIX192 baseline.
- Donkey/Cobra cisty XEX loader zustava beze zmen.
- FIX192 stream INIT loader je ponechany jako diagnostika, ale Donkey nema mezilehle INITAD, proto u nej nemohl zmenit duchy.
- Pridan Donkey PMG transition guard:
  - pri nestabilnim DLIST/DLI prechodu se dočasně nekresli stara PMG vrstva,
  - po stabilnich runtime framech se PMG zase povoli,
  - RAM hry se nemeni, je to jen render/PMG guard proti duchum mezi intro obrazovkami.
- Pridano tlacitko DONKEY PMG CLEAN TEST.
- Snapshot pise DONKEY PMG CLEAN FIX193.
- G7 tunnel, GTIA 9/10/11 BARS, POKEY zvuk, cisty XEX a pomaly BASIC paste zustavaji.
