# AtariHelp.eu EMU-09 FIX64 RIVER DLI SEQ HUD + PITSTOP ROAD CLEAN

FIX64 reaguje na test FIX63:

- River Raid: PMG/HPOS se uz nebere podle puleneho VCOUNT ani podle natazeneho celeho viewportu. Sekvence zachycenych DLI stavu je ukotvena k prvni DLI scanline display listu, aby souperi/fuel/letadlo nebyli rozhazeni proti playfieldu.
- River Raid: viewport je posunuty na yoff -32 a renderer projde vice DLIST scanlines, aby byl lip videt spodni HUD/panel.
- River Raid: kolize P2/P3 se pocitaji z opravdu prekrytych PF pixelu. Voda je nekolizni, zeleny breh/skala/fuel/most vraci PF bity, protoze hra cte $D006 i $D007.
- Pitstop II: zustava citelnejsi clean split road top $7100 / bottom $7000 s cistenim bilozelenych sumovych radku.
- Cil zustava obecny Atari 130XE/XEX emulator pro vetsinu XEX.

GitHub Desktop Summary:

FIX64 river dli seq hud pitstop road clean

Pri testu:
- River Raid: zkontrolovat spodni panel, fuel/soupere a reakci letadla na breh/skaly.
- Pitstop II: jen rychle overit, ze se nezhoršil proti FIX63.
- Donkey Kong: rychla kontrola titulka + prvni level.
