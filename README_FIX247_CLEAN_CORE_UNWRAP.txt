AtariHelp.eu EMU-09 FIX247_CLEAN_CORE_UNWRAP

CIL:
- Opravny clean unwrap po zmatku, kdy UI ukazovalo FIX246, ale snapshot/log stale tahal stare FIX240/FIX239 vrstvy.
- Snapshot ted obchazi starou compatibility chain a zapisuje cisty FIX247 stav primo.
- FAST BASIC tlacitka jsou znovu navazana na FIX247 screen-editor service po real READY. Stara RAW CH scan-code pumpa se pri kazdem startu fronty vypina.
- SELF TEST render branch kresli z realneho DLIST/DMA/RAM, ne kresleny fake SELF TEST.

POVINNE OVERENI:
Snapshot musi obsahovat:
BUILD FIX247_CLEAN_CORE_UNWRAP
FIX247 CLEAN CORE UNWRAP CHECK
VERIFY LOADED YES
OLD WRAPPERS BYPASSED

KODY JSOU STEJNE.
