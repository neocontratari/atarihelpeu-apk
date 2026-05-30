EMU-09 FIX44 LIVE DLI CHAIN CORE

Základ: návrat na vizuálně lepší linii FIX39/FIX36.
- PiTT-KiTT zůstává referenční XEX.
- Pitstop II: generic PMG overlay zůstává vypnutý, aby se nevrátil modrý sloup.
- Obecný renderer už nespouští DLI rutiny sám kvůli barvám.
- DLI běží jen v runFrameInterrupts jako živý VDSLST řetěz; renderer používá uložené snapshoty.
- Cíl: pevnější základ bez rozbíjení grafiky rendererem.
