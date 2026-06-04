AtariHelp.eu EMU-09 FIX124 ROM QUARANTINE DLIST CORE

Co jsem změnil:
- FIX123 ROM cesta byla správná diagnosticky, ale otrávila chráněné reference.
- Donkey / Cobra / Arkanoid / Montezuma mají ROM vrstvu v karanténě: žádné BASIC/OS ROM CPU běhání, žádné ROM čtení přes jejich runtime.
- Moon Patrol / Pac-Man / generic zůstávají jako ROM-probe profily, aby bylo vidět, jestli ROM/OS směr pomáhá.
- Skutečná ROM se už nekopíruje do RAM obrazu jako charset. Vrací se deterministický syntetický Atari charset.
- Přidán tvrdší filtr falešných DLISTů z 6502 kódu. Cíl: nezobrazovat barevný čaj z programových bajtů jako obraz.
- Build tag je FIX124_ROM_QUARANTINE_DLIST_CORE.

Test:
1. Donkey Kong – hlavně jestli se vrátí hra.
2. Super Cobra – jestli se vrátí menu/hra a nezůstane sek.
3. Arkanoid III – jestli aspoň beze změny.
4. Montezuma PRELIM / ALT / ORIG – jestli je méně rozhozená než FIX123.
5. Moon Patrol + Pac-Man – snapshot log, jestli se DLIST code reject objeví a jestli se obraz posunul.

Poznámka:
Tenhle FIX je stabilizační brzda po FIX123: nevyhazuje ROM směr, ale zavírá ho mimo hry, které nám už fungovaly nebo byly použitelné.
