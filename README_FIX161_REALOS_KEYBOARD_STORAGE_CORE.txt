AtariHelp.eu EMU-09 FIX161_REALOS_KEYBOARD_STORAGE_CORE

Commit summary:
FIX161 realos keyboard storage core

Co je v overlayi:
- Vychází z FIX160, tedy zachovává skutečný REAL OS/BASIC READY pokrok.
- Žádné kreslené READY ani fake BASIC.
- REAL OS/BASIC režim má storage/log guard: aplikace nemá během běhu sama zaplňovat interní úložiště.
- DOM log je omezený na 30 KB, localStorage na 20 KB a přepisuje se nejvýš zhruba po 10 s.
- Soubory do telefonu se ukládají jen po tlačítku SNAPSHOT / ULOŽIT LOG.
- Kompaktnější Atari/QWERTY klávesnice hned pod obrazem.
- Přidané SHIFT, RESET, dvojtečka :, ESC, TAB, DEL, RETURN, SPACE, START/SELECT/OPTION/FIRE.
- RETURN už není mezi písmeny.
- RESET v REAL OS režimu znovu spustí BASIC AUTO BOOT.
- REAL OS režim už nesmí při BRK skákat do staré XEX RAM adresy $5Fxx; místo toho bezpečně zastaví/loguje.
- Donkey / Super Cobra / Montezuma baseline zůstává chráněný jako ve FIX160.

Test plan:
1. Ověřit build tag: FIX161_REALOS_KEYBOARD_STORAGE_CORE.
2. REAL OS BASIC AUTO BOOT.
3. Zkusit: PRINT 2+2 a krátký program:
   10 PRINT "AHOJ"
   RUN
4. Otestovat SHIFT + :, RESET, DEL, RETURN.
5. Nechat READY/BASIC chvíli běžet: nesmí agresivně plnit úložiště; soubor vzniká jen při snapshot/log.
6. Pokud BASIC skočí do grafického režimu po GRAPHICS, vrať se RESET tlačítkem nebo příkazem GRAPHICS 0.
7. Rychle ověřit Donkey / Super Cobra / Montezuma, že zůstaly funkční.
