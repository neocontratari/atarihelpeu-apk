AtariHelp.eu EMU-09 FIX198 HARDWARE RAM DLIST SCAN CORE

Overlay pro GitHub Desktop.

Commit summary:
FIX198 hardware ram dlist scan core

Hlavni zmena:
- XEX loader zustava cisty a v logu hlasi STREAMED=OK ACTIVE=YES.
- Stary matoucí snapshot privesek streamed=none active=no je odstraneny.
- DLIST resolver uz nesmi vzit code-like 6502 adresu jako display list.
- Kdyz HW/SDLST ukazuje na code-like/stale adresu, pouzije obecny RAM scan runtime DLISTu.
- Zadny Donkey/Cobra hard-lock.

Test:
1. TEST DONKEY KONG XEX
2. TEST SUPER COBRA XEX
3. Poslat snapshoty s radky: XEX STREAM STATUS FIX198, NATIVE XEX LOADER FIX198, HARDWARE PURGE FIX198, RAM DLIST SCAN FIX198, DLIST RESOLVE FIX198.
