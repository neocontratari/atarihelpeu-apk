# AtariHelp.eu EMU-09 FIX63 PITSTOP CLEAN ROAD + RIVER BANK COLLISION

FIX63 reaguje na test FIX62:

- Pitstop II: FIX62 hybrid top zhorsil horni pulku. FIX63 se vraci k citelnejsimu split road renderu top $7100 / bottom $7000 a agresivneji cisti bilozelene sumove radky.
- River Raid: FIX62 potlacil falesne kolize, ale letadlo pak prezilo i mimo reku. FIX63 vraci bank collision jen pro hlavni letadlo P2, kdyz je mimo vodni koridor. Ostatni PMG/fuel/enemy kolize zatim nezabijeji start.
- Cilem zustava obecny Atari 130XE/XEX emulator, ne jednouherni hack.

GitHub Desktop Summary:

FIX63 pitstop clean road river bank collision

Pri testu uloz log:

- Pitstop II: zavod po 5-10 sekundach.
- River Raid: po startu a po pokusu letet mimo reku.
- Donkey Kong: kratce titulka + prvni level.
