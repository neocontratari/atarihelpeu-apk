AtariHelp.eu EMU-09 FIX240_SELFTEST_START_EDGE_FAST_BASIC_QUEUE_CORE

Commit summary:
FIX240 selftest start edge fast basic queue core

Zmeny:
1) PURE HARDWARE OPTION SELF TEST:
   - FIX239 log ukazal PC smycku $5059/$505C/$505E.
   - Self-test ROM cte CONSOL $D01F a ceka na START low.
   - FIX240 poda jeden START-low edge pouze pres realny CONSOL read $D01F, pouze kdyz CPU opravdu stoji v $5059-$5060.
   - Zadny skok PC, zadne kresleni SELF TEST, zadny fake screen.

2) Rychly BASIC text automat:
   - Pridana servisni UI sekce: BASIC TXT FAST, VLOZIT FAST, VLOZIT FAST + RUN, SUBMARINE FAST.
   - Fronta se nespusti pri bootu ani v selftestu.
   - Ceka na realne BASIC READY: DLIST $9C20, SAVMSC $9C40, BASIC window, VBI.
   - Text jde pres samostatnou ATASCII/KGETCH frontu, EOL je $9B.
   - Nepouziva fyzicky keyboard mapping pro kazdy znak.
   - Ma inputEpoch cancel pri reset/power/load XEX.
   - Po dokonceni cisti CH $02FC, KBCODE/SKSTAT idle a RETURN latch.

3) Snapshot/log:
   - Pridava FIX240 SELFTEST/FAST BASIC CHECK.
   - Loguje SELFTEST edge, FAST BASIC QUEUE START, WAIT READY/READY OK, QUEUE COMPLETE, LATCH CLEAR, QUEUE CANCELLED epoch changed.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou zmenene.
