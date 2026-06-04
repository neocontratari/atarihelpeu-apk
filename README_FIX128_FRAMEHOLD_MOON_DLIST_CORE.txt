AtariHelp.eu EMU-09 FIX128 FRAMEHOLD MOON DLIST CORE

Co jsem změnil:
- Cobra: vypnul jsem framehold/restore pro Cobru. Log FIX127 ukázal správný DLIST $2000, ale zároveň framehold přechod $B39E->$2000; to odpovídá blikání do čaje. Teď Cobra kreslí živě $2000 bez vracení staré frame.
- Moon Patrol: opravil jsem pořadí DLIST výjimek. Ve FIX127 byla $7481 sice nalezená scannerem, ale code-like filtr ji shodil a výsledkem bylo DLIST $0000 / šedá obrazovka. Teď je $7481 pro Moon důvěryhodná karanténní listina, $854C zůstává zakázaná.
- Donkey: ponechán chráněný stabilní režim.
- Arkanoid III: ponechán chráněný DLIST $3503, ale bez dalších agresivních změn.
- Montezuma PRELIM/ALT/ORIG: bez nové agresivní změny; nechávám stranou, abych nerozbil základ.
- ROMky zůstávají vestavěné, tester nic nenahrává.

Commit Summary:
FIX128 FRAMEHOLD MOON DLIST CORE

Co prosím otestuj:
1) Nahoře/log musí být FIX128_FRAMEHOLD_MOON_DLIST_CORE.
2) Cobra: jestli po START už nebliká do čaje.
3) Donkey: jen potvrdit, že běží.
4) Arkanoid III: menu + po START stručně stejné/lepší/horší.
5) Moon Patrol: hlavně screenshot + snapshot log, potřebuji vidět DLIST RESOLVE, jestli je used $7481 a už ne $0000.
6) Montezuma PRELIM: jen rychle, jestli jsem ji nerozbil.

Poznámka:
Souhlasím s Reného připomínkou: žádná hra zatím nejede jako v hotovém emulátoru. Cíl dalších fixů musí být jádro: ANTIC/DLIST/DLI/PMG/OS timing, ne jednotlivé herní záplaty.
