AtariHelp.eu EMU-10 BUILD2KV_PMG_PHASE_ROLLBACK_XEX_TURBO_GUARD_CORE

CIL:
- navazuje na BUILD2KU, ale bere vazne tvoje testy: KU pomohl vice hram, ale posunul PMG objekty vertikalne.
- Donkey Arcade: clovicek pod zebrikem.
- Donkey Junior: opicka pod zebrikem + rychlost RYCHLA.
- Decathlon: telo/nohy porad spatne.
- Donkey Title: obraz neni cely, rychlost velka.
- Night Driver: auto ano, pruh ano, zvuk ano.

ZMENA V KODU:
1) PMG vertical phase rollback
   - KU bral PMG DMA index jako visibleY=M.line-8.
   - KV vraci fyzickou scanline fazi jako KT: li=M.line, pri double line li=M.line>>1.
   - Cil: vratit postavy na spravne Y proti zebrikum/zakladnam a nezrusit GTIA color-clock zlepseni.

2) XEX turbo idle guard
   - Nektere XEX hry po poslednim payloadu neprectou EOF header, ale hra uz bezi.
   - Stary M.xexLoading mohl zustat true a drzet UI tick v rychlem rezimu.
   - KV ukonci turbo po 90 framech bez pending SIO/rx aktivity.
   - Pri kazdem novem XEX se resetuje _hvTurboLogged/_hvTurboDone, aby log jasne ukazal start i konec loaderu.

3) Zachovano z KU/KT
   - GTIA color-clock pro bitmap/hires zustava.
   - GTIA9/10/11 bitmap pixel generator zustava.
   - PRIOR tabulka z KT/KU zustava.
   - VBXE overlay / UI / kazeta / joystick bez zmen.

BEZPECNOST:
- bez screen-paintu.
- bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

LOG MARKERY:
- BUILD2KV_PMG_PHASE_ROLLBACK_XEX_TURBO_GUARD_CORE
- GTIA PMG VERTICAL PHASE BUILD2KV
- GTIA COLOR CLOCK BUILD2KV
- GTIA BITMAP MODES BUILD2KV
- XEX FAST LOAD BUILD2KV: idle-guard ukoncil turbo loader

TEST:
1) Donkey Kong Arcade.xex
   - zvuk intro/hra, rychlost OK, clovicek uz neni pod zebrikem?
2) Donkey Kong Junior.xex
   - nabeh/hra/zvuk, rychlost OK nebo RYCHLA, opicka proti zebrikum.
3) Donkey Kong Junior (Title Version).xex
   - obrazek celistvy/lepsi/stejny/horsi, zvuk, rychlost.
4) Decathlon
   - telo/nohy prvniho panacka, napis nahore, rychlost.
5) Night Driver
   - auto, pruh u auta, zvuk.
6) Postcard
   - okna barvy, pruhy.

Pokud se zlepsi rychlost Junior/Title a vrati se Y poloha postav, KV je spravny stabilizacni krok.
