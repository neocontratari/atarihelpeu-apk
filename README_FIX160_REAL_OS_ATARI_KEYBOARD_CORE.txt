AtariHelp.eu EMU-09
FIX160_REAL_OS_ATARI_KEYBOARD_CORE

Commit summary:
FIX160 real OS Atari keyboard core

Co je nové:
- pokračuje bezpečný rychlý baseline z FIX159,
- REAL OS/BASIC už zůstává bez kresleného READY a běží přes skutečný DLIST/SAVMSC,
- mobilní Atari klávesnice je vysunutá hned pod obrazem, ne schovaná nízko pod joystickem,
- klávesnice má rozložení blíž Atari/QWERTY:
  ESC, čísla, písmena, TAB, +, -, *, /, =, čárka, tečka, středník, DEL, RETURN, SPACE,
- přidaná konzolová tlačítka START / SELECT / OPTION / FIRE přímo do klávesnice,
- REAL OS obraz má o něco světlejší modrou paletu,
- hry Donkey / Super Cobra / Montezuma zůstávají na chráněném XEX baseline.

Test plan:
1. REAL OS BASIC AUTO BOOT.
2. Ověřit, že READY zůstává a klávesnice je hned pod obrazem.
3. Zkusit: PRINT 2+2 a RETURN.
4. Zkusit + - * / = ESC TAB DEL RETURN.
5. Krátce ověřit Donkey Kong, Super Cobra a Montezuma PRELIM, že zůstaly funkční.

Poznámka:
FIX160 nelepí XEX hry na REAL OS RAM. To přijde až po stabilizaci BASIC/READY a vstupu.
