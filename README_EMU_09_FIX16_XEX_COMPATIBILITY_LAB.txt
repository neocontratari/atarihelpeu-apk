EMU-09 FIX16 XEX COMPATIBILITY LAB

Základ: FIX15, PiTT-KiTT ponechán byte-for-byte jako referenční hra.

Novinky:
- Vestavěný test River Raid.xex.
- Tlačítko KOMPATIBILITA / SNAPSHOT vypíše PC, ENTRY, DLIST, SAVMSC, NMIEN, VBI/DLI, DMA/PMG/barvy a poslední instrukce.
- River Raid profil: entry $4000, INIT/depacker styl, opatrnější CPU dávky.
- Generic/River profil už nespadají do PiTT-KiTT rendereru.
- Přidán servisní běh VBI/DLI rutin pro cizí XEXy bez rozbití PiTT-KiTT profilu.
- Přidán RTI a bezpečné undocumented NOP opkódy často používané jako timing/padding.
- Opraveny délky ANTIC řádků: mode $E = 40 bytů, mode $6 = 20 bytů.
- Obecný ANTIC renderer umí aspoň základní 20sloupcový text pro scoreboard.

Poznámka:
River Raid ještě není garantovaně hotový. FIX16 je laboratorní krok, který má zabránit zamrznutí a dát použitelný obraz/snapshot pro další ladění.
