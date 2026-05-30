EMU-09 FIX43 PITSTOP FRAME SYNC CORE

Základ:
- vychází z FIX39/FIX36/FIX42 linie, ne z rozbitých odboček FIX37/FIX38/FIX40/FIX41.
- PiTT-KiTT zůstává chráněný referenční XEX.

Hlavní změna:
- Pitstop II wait loop $4F78 už se neprolamuje opakovaným čištěním latchů uvnitř jedné CPU dávky.
- Emuluje se frame-sync: CPU se na waitu zastaví, vykreslí se snímek, a latch $50AE/$50AF se uvolní nejvýš jednou za video frame.
- Generic DLI capture pro renderer běží v RAM sandboxu, aby samotné snímání barev neměnilo herní RAM.

Test:
1) TEST PITSTOP II XEX
2) ZADAT JMÉNO -> REN
3) sledovat, jestli se hra po startu nepřesype jako FIX40/FIX41
4) Snapshot musí hlásit FIX43_PITSTOP_FRAME_SYNC_CORE
