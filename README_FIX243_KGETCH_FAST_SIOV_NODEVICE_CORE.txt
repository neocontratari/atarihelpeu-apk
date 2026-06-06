AtariHelp.eu EMU-09 FIX243_KGETCH_FAST_SIOV_NODEVICE_CORE

CIL:
- Opravit FAST BASIC helper, ktery ve FIX241/FIX242 porad rozbijel BASIC radky pres RAW CH $02FC mapu.
- Dodat strict self-test no-device guard pro CIOV/SIOV vektory $E456/$E459, jen ve SELF TEST rezimu.

ZMENY:
1) FAST BASIC uz pri paste nepouziva fyzicky CH scan-code mapping.
   - Po real READY ceka na ROM KGETCH smycku $F2FD-$F314.
   - Dalsi ATASCII bajt vrati pres ROM KGETCH navrat $F3DA.
   - EOL je ATASCII $9B a ma delsi pauzu.
   - Nepise do obrazove pameti, nepodstrkuje READY, nepouziva fake boot.

2) SELF TEST strict no-device guard:
   - Jen v PURE HW SELF TEST.
   - Kdyz CPU vejde do $E456 nebo $E459, vrati status $8A no-device/timeout a vrati se ze zasobniku.
   - Bez kresleni SELF TEST obrazovky, bez PC skoku do hotove obrazovky.

3) UI:
   - Tlacitka BASIC TXT FAST / VLOZIT FAST / VLOZIT FAST + RUN / SUBMARINE FAST jsou reboundovana na FIX243 KGETCH queue.

LOG HLEDEJ:
BUILD FIX243_KGETCH_FAST_SIOV_NODEVICE_CORE
FIX243 KGETCH FAST/SIOV CHECK
FAST ... KGETCH FEED / QUEUE COMPLETE
SELFTEST ... STRICT NODEV CIOV/E456 nebo SIOV/E459 status $8A
RULE FAST_BASIC_KGETCH_ATASCII_RETURN / NO_PHYSICAL_CH_MAP_FOR_PASTE / EOL_9B_DELAY / READY_GATED / EPOCH_CANCEL / LATCH_CLEAR / STRICT_E456_E459_NODEV_SELFTEST_ONLY / NO_FAKE_READY_SELFTEST

KODY JSOU STEJNE
