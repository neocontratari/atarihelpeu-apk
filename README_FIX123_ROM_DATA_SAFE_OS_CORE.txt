FIX123 ROM DATA SAFE OS CORE

- Oprava FIX122: BASIC/OS ROM už není slepě zkopírovaná do RAM jako vykonatelný kód.
- ROM je vestavěná a používá se pro čtení dat, znakové sady a vektory.
- CPU vstup do BASIC/OS prostoru se chová jako bezpečný OS/CIO trampoline, ne jako nekontrolované provádění celé ROM.
- Cíl: vrátit Donkey/Cobra baseline a zachovat směr pro Moon Patrol, Pac-Man a Montezumu.
- Tester nic nenahrává, žádný externí ROM soubor.
