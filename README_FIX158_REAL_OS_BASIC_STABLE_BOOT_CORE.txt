# AtariHelp.eu EMU-09 FIX158 REAL OS BASIC STABLE BOOT CORE

FIX158 nahrazuje FIX157. FIX157 zachytával CIO/SIO služby příliš brzy a tím zastavil REAL OS BASIC AUTO ještě před vytvořením editorové obrazovky. FIX158 rané CIO/SIO hooky v REALOS režimu vypíná: nejdřív nechá XL OS postavit DLIST/SAVMSC a teprve potom se pokusí přejít do skutečné BASIC ROM.

Commit summary:
FIX158 real OS BASIC stable boot core

Test:
1. Ověřit build tag FIX158_REAL_OS_BASIC_STABLE_BOOT_CORE.
2. Kliknout REAL OS BASIC AUTO BOOT.
3. Pokud se objeví skutečné READY, zkusit PRINT 2+2.
4. Pokud READY nenaběhne, poslat snapshot s REAL OS BASIC AUTO FIX158, DLIST, SAVMSC, PC, earlyNoDeviceSuppressed a ramGuardHits.
5. START+OPTION / NO BASIC je samostatný test. XEX hry zatím nespouštět na jeho zbytku RAM; to přijde až po stabilním OS bootu.
