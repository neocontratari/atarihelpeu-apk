# AtariHelp.eu EMU-09 FIX64 RIVER DLI SEQ HUD + PITSTOP ROAD CLEAN

FIX64 reaguje na test FIX63:

- River Raid: PMG/HPOS se uz nebere podle puleneho VCOUNT. Pouziva relativni sekvenci zachycenych DLI stavu, aby souperi/fuel/letadlo nebyli rozhazeni proti playfieldu.
- River Raid: viewport je posunuty na yoff -32 a renderer projde vice DLIST scanlines, aby byl lip videt spodni HUD/panel.
- River Raid: bank collision se vraci pro P2 i P3 cast letadla, protoze hra cte $D006 i $D007.
- Pitstop II: zustava citelnejsi clean split road top $7100 / bottom $7000 s cistenim bilozelenych sumovych radku.
- Cil zustava obecny Atari 130XE/XEX emulator pro vetsinu XEX.

GitHub Desktop Summary:

FIX64 river dli seq hud pitstop road clean

Pri testu:
- River Raid: zkontrolovat spodni panel, fuel/soupere a reakci letadla na breh/skaly.
- Pitstop II: jen rychle overit, ze se nezhoršil proti FIX63.
- Donkey Kong: rychla kontrola titulka + prvni level.
