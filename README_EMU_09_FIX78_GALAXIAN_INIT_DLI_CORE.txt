EMU-09 FIX78 GALAXIAN INIT + DLI CORE

Cil FIX78:
- navazat na FIX77_RELEASE_VIEWPORT_COBRA_UI,
- zamerit se na Galaxian (Title Version 2).xex,
- nebourat obecny XEX emulator ani chráněné reference.

Co ukazal log FIX77:
- Galaxian ma INITAD $9B86, ale nema RUNAD.
- Po inicializaci hra vraci CPU do nizke pameti kolem $050F, kde v mini-OS bylo $00/BRK.
- Soucasne uz je nastaven DLIST $3D00, NMIEN $80 a DLI $B1CE.
- DLI/VBI wrapper ve FIX77 po RTI jeste pokracoval do $0000 a logoval BRK at $0000.

Zmeny FIX78:
1) INIT-only idle core:
   Pokud XEX nema RUNAD, ale ma INITAD, platny DLIST a povolene NMI/DLI, BRK v prazdne nizke pameti se nebere jako smrt hry.
   CPU zustane v idle a emulátor dal obsluhuje frame interrupt/DLI.

2) Bezpecnejsi DLI/VBI wrapper:
   runAsyncAtariRoutine ted po RTI/RTS okamzite konci.
   Stack page $0100-$01FF se po async DLI/VBI vraci, aby DLI rutina nerozbila hlavni stack hry.
   HW/RAM zapisy DLI/VBI rutiny zustavaji zachovane.

3) Logovani:
   Status a snapshot vypisuji INITIDLE/INIT-ONLY IDLE FIX78 recoveries.

Testovat hlavne:
- Galaxian (Title Version 2).xex: jestli uz nezastavi na BRK $050F a jestli titul/obraz bezi dal.
- Donkey Kong/Pitstop II/KiTT Garage/Super Cobra: rychla kontrola, ze se FIX78 nerozbil.

GitHub Desktop summary:
FIX78 galaxian init dli core
