AtariHelp.eu EMU-09 FIX244_SELFTEST_DMA_SCREENFAST_CORE

KODY JSOU STEJNE

Cil:
- FAST BASIC vkladani nesmi pouzivat fyzicky CH scan-code mapping, protoze predchozi logy ukazaly PPRINT/RRINT/POSTTION/SSUBMARINE.
- SELF TEST ROM bezi v $532A, DLIST $51D1 a SDMCTL $21, ale renderer nekreslil self-test DMA data z DLISTu.

Zmeny:
1. FAST BASIC screen-editor service
   - Po real READY zapisuje kazdy radek do skutecne BASIC editor screen RAM podle SAVMSC/ROWCRS/COLCRS.
   - Potom posle real RETURN latch pro tokenizaci radku ROM BASICem.
   - Nepouziva fyzicky CH scan map pro znaky programu.
   - Nepise READY, netokenizuje BASIC bokem, neni to fake boot.

2. SELF TEST DMA renderer
   - V PURE SELF TEST rezimu kresli specialni ANTIC DLIST branch pro DLIST $51D1.
   - Cte SDMCTL $022F / DMACTL $D400, CHBASE $D409/$02F4, PRIOR $D01B/$026F a GTIA barvy live.
   - Kresli ANTIC mode 6/7 text20 a mode 8+ bitmap data z realne pameti ($3000 podle DLISTu).
   - Zadne fake menu, zadna kreslena self-test obrazovka.

Log hledej:
BUILD FIX244_SELFTEST_DMA_SCREENFAST_CORE
FIX244 SELFTEST DMA/SCREENFAST CHECK
FAST ... SCREEN LINE WRITE / QUEUE COMPLETE
SELFTEST ... SELFTEST DMA RENDER dl $51D1 ... SDMCTL ... CHBASE ... PRIOR ...
RULE FAST_BASIC_SCREEN_EDITOR_SERVICE / REAL_READY_ONLY / REAL_RETURN_EACH_LINE / NO_PHYSICAL_CH_MAP_FOR_PASTE / SELFTEST_DLIST_51D1_DMA_RENDER / SDMCTL_CHBASE_PRIOR_LIVE / RAW_BYTES_PRESERVED / NO_FAKE_READY_SELFTEST
