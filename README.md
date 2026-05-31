# AtariHelp.eu EMU-09 FIX62 PITSTOP HYBRID TOP + RIVER SAFE COLLISION

FIX62 reaguje na test FIX61: uzivatel spravne hlasil, ze obrazove neni videt posun proti FIX60.

- Cil zustava obecny Atari 130XE/XEX emulator pro vetsinu XEX, ne hack jen pro jednu hru.
- Pitstop II: FIX61 log ukazal, ze horni pulka nema byt road buffer $7100. FIX62 proto kresli horni cast pres obecny ANTIC/DLIST renderer a stabilizuje jen spodni road cast z $7000.
- River Raid: FIX61 log ukazal falesne PMG/PF kolize $D006/$D007=$0F i pri letu ve vode. FIX62 tyto River kolize docasne potlaci, aby slo otestovat, jestli hra prezije start a jestli PMG/HUD/fuel davaji smysl.
- Donkey/Kong obecny smer zustava zachovany: DLIST resolver a obecny ANTIC/PMG core nejsou nahrazeny jednouherni kresbou.

GitHub Desktop Summary:

FIX62 pitstop hybrid top river safe collision

Pri testu uloz log:

- Pitstop II: v zavode po tom, co se obraz ustali nebo rozsype.
- River Raid: po startu, idealne po 10-20 sekundach letu nebo po smrti.
- Donkey Kong: rychly kontrolni test titulka + prvni level.
