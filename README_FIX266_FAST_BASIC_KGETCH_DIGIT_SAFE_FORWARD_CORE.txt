AtariHelp.eu EMU-09 FIX266 FAST BASIC KGETCH DIGIT SAFE FORWARD CORE

Smer: dopredu z funkcni FIX265/FIX261. Nejde o rollback.

Cil:
- Opravit chyby rychlozapisu BASIC kodu, hlavne riziko zameny 0/O, zavorek a uvozovek.
- Vypnout stare FAST vrstvy FIX244/FIX247 screen-editor service pro tlacitka BASIC TXT FAST / VLOZIT FAST / VLOZIT FAST + RUN / SUBMARINE FAST.
- Pouzit servisni helper az po skutecnem BASIC READY: ATASCII fronta primo pres ROM KGETCH wait/return.

Pravidla:
- Zadny fake READY.
- Zadny fake LOAD.
- Zadny program RAM inject.
- Zadny screen RAM write pro rychlozapis.
- Zadny herni hack.

Snapshot audit:
- build=FIX266_FAST_BASIC_KGETCH_DIGIT_SAFE_FORWARD_CORE
- FAST_BASIC_FIX=KGETCH_ATASCII_DIRECT / SCREEN_EDITOR_DISABLED / CH_SCAN_MAP_DISABLED_FOR_FAST
- queuePreviewHex ukazuje skutecne ATASCII bajty. Pro ? FRE(0) ma byt: 3F 20 46 52 45 28 30 29 9B.

KODY JSOU STEJNE.
