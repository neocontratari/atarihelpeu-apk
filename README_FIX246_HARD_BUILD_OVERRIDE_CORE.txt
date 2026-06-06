AtariHelp.eu EMU-09 FIX246 HARD BUILD OVERRIDE CORE

Ucel:
- Opravit chybu handoffu, kdy landing screen ukazal FIX245, ale uvnitr EMU-09 stale bezel FIX240.
- Pridat posledni samostatny script block za vsechny starsi FIX bloky, aby BUILD stamp a snapshot byly neprepisovatelne starym FIX240 kodem.
- FAST BASIC: vypnout stare RAW CH pumpy a pouzit servisni screen-editor write + real RETURN az po real READY.
- SELF TEST: pridat ne-fake live RAM DMA view pro DLIST/selftest, pouze podle RAM/registru.

Povinne overeni:
Snapshot musi mit:
BUILD FIX246_HARD_BUILD_OVERRIDE_CORE
FIX246 VERIFY BUILD STAMP CHECK
VERIFY LOADED YES

KODY JSOU STEJNE
