AtariHelp.eu EMU-09
FIX157_REAL_OS_BASIC_AUTO_CORE

Commit summary:
FIX157 real OS BASIC auto core

Co je nové:
- Žádné kreslené READY ani falešný BASIC.
- XEX hry zůstávají na bezpečném baseline z FIX156/FIX148/FIX145.
- Přidána dvě servisní tlačítka:
  1) REAL OS BASIC AUTO BOOT — bez START/OPTION, BASIC má být zapnutý.
  2) REAL OS START+OPTION / NO BASIC — test bootu s vypnutým BASICem.
- REAL OS boot pořád startuje přes skutečný XL reset vector $C2AA.
- BASIC ROM zůstává mapovaná na $A000-$BFFF.
- Pokud OS postaví editorovou obrazovku (DLIST/SAVMSC) a pak uvízne v SIO/boot RAM, FIX157 zkusí vstoupit do skutečné BASIC ROM na $A000. Není to ručně kreslený READY; dál běží BASIC ROM kód.
- Přidána diagnostika REAL OS BASIC AUTO FIX157.

Test plan:
1. Ověř BUILD TAG FIX157_REAL_OS_BASIC_AUTO_CORE.
2. Klikni REAL OS BASIC AUTO BOOT.
3. Počkej několik sekund. Pokud se objeví READY, zkus PRINT 2+2.
4. Pokud ne, ulož snapshot a pošli řádky: PC, DLIST, SAVMSC, REAL OS BASIC AUTO FIX157, REAL OS SIO FIX157.
5. Volitelně zkus REAL OS START+OPTION / NO BASIC.
6. Krátce ověř Donkey Kong, Super Cobra, Montezuma PRELIM.
