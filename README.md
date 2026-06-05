# AtariHelp.eu EMU-09 FIX206_REALROM_STREAM_LOADER_CORE

KODY JSOU STEJNE. GTIA 9/10/11 VERIFY BASIC kody se nemenily.

Tento overlay prestava honit duhovy test jako hlavni problem a presouva praci na cisty XEX loader pod kapotou:
- real ROM audit z dodanych ROM souboru,
- vychozi DOS-like stream INITAD loader,
- BASIC direct OFF,
- HW/SDLST DLIST first, RAM scan pouze diagnostika.

Pouziti: rozbalit pres repo, commit/push v GitHub Desktop, nechat Actions postavit APK.
