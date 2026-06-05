# AtariHelp.eu EMU-09 FIX156_REAL_OS_SIO_TIMEOUT_CORE

Overlay pro GitHub Desktop.

## Co je změněno
- Pokračuje poctivý REAL OS/BASIC boot bez ručně kresleného READY.
- FIX155 prokázal, že ROM už vytvořila DLIST $9C20 a SAVMSC $9C40, ale zůstala ve smyčce OS/SIO na PC $EA9E.
- FIX156 přidává jen SIO timeout guard: když XL OS visí na $EA9E a čeká na serial-ready flag $3A, nastaví $3A=1 a nechá ROM pokračovat.
- V REALOS testu je vypnuté pískání/audio, aby stisk klávesy nespustil WebAudio bzučení.
- XEX baseline Donkey/Cobra/Montezuma zůstává chráněný.

## Commit summary
FIX156 real OS SIO timeout core

## Test
1. Klikni `EXPERIMENT REAL OS RESET / BASIC BOOT`.
2. Sleduj, jestli se z černé obrazovky posune k BASIC/READY.
3. Pokud ne, pošli snapshot s řádky `REAL OS BOOT FIX156`, `REAL OS SIO FIX156`, `REAL OS SCREEN FIX156`, `PC`, `DLIST`, `SAVMSC`.
4. Krátce ověř Donkey, Super Cobra, Montezuma.
