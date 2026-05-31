# AtariHelp.eu EMU-09 FIX61 PITSTOP SPLIT DIAGNOSTIC + RIVER PMG DEBUG

FIX61 navazuje na FIX60, ale nebere FIX60 jako viditelne potvrzenou opravu.

- Cil zustava obecny Atari 130XE/XEX emulator pro vetsinu XEX, ne hack jen pro jednu hru.
- Pitstop II: split renderer ted loguje zdroj hornich/spodnich bufferu a kontrolni soucty kandidatu $7000/$7100/$7200/$9800/$9900. FIX60 radkovy filtr je v race split renderu vypnuty, aby test ukazal skutecna data.
- River Raid: aircraft PMG se uz vizualne neclampuje do vody. Log ukazuje PMBASE, P0-P3 HPOS/SIZE/rawY/visY/nonzero a aircraftGuess.
- GTIA collision log uklada poslednich 20 cteni $D000-$D00F vcetne PC/frame a pocet HITCLR.
- Donkey/Kong obecny smer zustava zachovany: DLIST resolver a obecny ANTIC/PMG core nejsou nahrazeny jednouherni kresbou.

GitHub Desktop Summary:

FIX61 pitstop split diagnostic river pmg debug

Pri testu uloz log:

- Pitstop II: v zavode po tom, co se obraz rozsype.
- River Raid: po startu, idealne po prvni smrti nebo kdyz letadlo vypada mimo reku.
- Donkey Kong: po titulce/prvnim levelu, aby bylo videt, ze jsme ho nerozbili.
