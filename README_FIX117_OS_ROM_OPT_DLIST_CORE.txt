FIX117 OS ROM OPT DLIST CORE

Cíl:
- Nejet dál jen po jednotlivých hrách. Přidat obecnější cestu pro hry, které volají OS/BASIC ROM nebo nechávají v DLIST pointerech kód.
- Udržet Donkey Kong / Super Cobra / Arkanoid baseline.
- Montezumu chránit: žádný agresivní CHBASE override do $0C, pouze jemná korekce když je skutečně dostupný charset a registr je prázdný/OS.

Změny:
- Volitelný loader Atari OS ROM a BASIC ROM z telefonu/PC. ROM není součástí ZIPu; app ji umí načíst, pokud ji tester vlastní.
- CPU čtení z $C000-$CFFF a $D800-$FFFF umí použít načtený OS ROM; $A000-$BFFF umí použít BASIC ROM.
- Mini OS/CIO shim zůstává jako fallback pro ROM-less režim.
- DLIST resolver nově odmítá falešné display-listy, které jsou očividně 6502 kód (typicky Moon Patrol loader $7481: STA/LDA/STA/JMP CIOV).
- Montezuma CHBASE guard je zjemněný: nepřepisuje každý Montezuma DLIST na $0C.
- Snapshot log uvádí ROM stav a romReads.
- Viditelný build tag: FIX117_OS_ROM_OPT_DLIST_CORE.

Test:
1. Montezuma PRELIM, ALT, ORIG – zda je grafika lepší/ne horší a jestli se po smrti nepřepisuje.
2. Moon Patrol – bez ROM i případně s OS ROM, screenshot + snapshot.
3. Pac-Man – bez ROM i případně s OS+BASIC ROM, screenshot + snapshot.
4. Donkey / Cobra / Arkanoid – jen kontrola baseline.
