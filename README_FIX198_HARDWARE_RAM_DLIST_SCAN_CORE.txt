AtariHelp.eu EMU-09 FIX198 HARDWARE RAM DLIST SCAN CORE

Cil:
- nerecyklovat Donkey/Cobra DLIST berlicky
- nebrat nahodny 6502 kod jako display list
- kdyz HW/SDLST pointer ukazuje na code-like/stale adresu, najit obecne realny runtime DLIST v RAM

Zmeny:
- build tag FIX198_HARDWARE_RAM_DLIST_SCAN_CORE
- snapshot pise XEX STREAM STATUS FIX198 bez stareho streamed=none/active=no privesku
- snapshot pise NATIVE XEX LOADER FIX198
- snapshot pise HARDWARE PURGE FIX198
- snapshot pise RAM DLIST SCAN FIX198
- rozsiren 6502 code-like detektor pro falešné DLIST kandidáty typu EOR/LDY/LDA/STA/AND/BEQ sekvence
- odstranene profile whitelisty z dlistCandidateScore; kandidát musí projít obecnou ANTIC strukturou
- pridan obecny RAM DLIST scan pro runtime listy mimo puvodni XEX segmenty
- getDlistPtr v XEX hardware režimu zkousi: HW DLIST, SDLST, RAM-SCAN, segment-scan, lastGood
- BASIC direct pro hry zustava OFF
- REALOS/BASIC, G7, GTIA BARS, BCD ADC/SBC a zvuk zustavaji

Test:
1. TEST DONKEY KONG XEX
2. snapshot/log: XEX STREAM STATUS FIX198, NATIVE XEX LOADER FIX198, RAM DLIST SCAN FIX198, DLIST RESOLVE FIX198
3. TEST SUPER COBRA XEX
4. snapshot/log stejné radky

Poznamka:
FIX198 nema slibovat zazrak jednim radkem. Ucel je dostat ciste jadro z nesmyslneho code-like DLISTu $49D7/$8080 do realneho runtime DLISTu bez profilove sablony.
