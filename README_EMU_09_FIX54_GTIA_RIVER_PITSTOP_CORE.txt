# AtariHelp.eu EMU-09 FIX54 GTIA + RIVER + PITSTOP CORE

FIX54 navazuje na první skutečně hratelný Donkey/Kong výsledek z FIX52/FIX53. Cíl pořád není hack jedné hry, ale obecnější XEX emulátor.

Hlavní změny:

- opravený River Raid renderer: ve FIX53 byl překlep `dliCaps`, který mohl skončit černou obrazovkou,
- River má fallback: když speciální River renderer vyrobí skoro černý snímek, pustí se obecný ANTIC/DLIST renderer,
- GTIA collision diagnostika už nemaže statistiky při každém HITCLR; log ukáže `readMap`, `clears` a které registry hra čte,
- PMG/playfield collision je přesnější: vzorkuje víc bodů sprite obdélníku a nastavuje PF bity podle barvy playfieldu,
- missile/playfield collision se také zapisuje do GTIA registrů,
- bitmapové ANTIC módy používají DLI/DMA stride dynamicky po scanline, což je příprava na Pitstop rozbitou jízdu,
- Donkey/Kong zůstává hlavní test generic PMG + kolizí, Pitstop a River jsou tvrdé testy DLI/bitmap rendereru.

Upload na GitHub: nahraj obsah složky, ne ZIP. V GitHub Desktop napiš Summary, commitni a dej Push origin. APK pak stáhneš z GitHub Actions / Artifacts.
