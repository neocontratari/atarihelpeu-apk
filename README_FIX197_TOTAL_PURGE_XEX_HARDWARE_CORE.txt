FIX197_TOTAL_PURGE_XEX_HARDWARE_CORE

Proc:
- STOP pro profilove render sablony. FIX196 sice mapoval OS kernel probe, ale log porad ukazoval stare FIX181 DLIST/Donkey/Cobra cesty a XEX LOADER CODE ACTIVE=NO.
- FIX197 je cisty purge build: neslibuje zazrak, ale vyhazuje skryte Donkey/Cobra/Pitstop/Arkanoid DLIST locky z hlavni XEX render cesty, aby se konecne testovalo obecne jadro.

Co je v overlayi:
- build tag FIX197_TOTAL_PURGE_XEX_HARDWARE_CORE
- XEX loader loguje STREAMED=OK ACTIVE=YES po realnem nasypani segmentu do RAM
- NATIVE XEX LOADER FIX197 ukazuje bloky, bajty, RUNAD/INITAD a entry
- TOTAL PURGE FIX197 ukazuje, zda je purge aktivni, kolik bloku/bajtu bylo nahrano
- getDlistPtr pro XEX hry preskakuje stare profilove vetve a bere HW DLIST / SDLST / obecny scanner
- Donkey/Cobra hard-locky a Donkey $466F/$49D7 priority jsou pro FIX197 XEX cestu vypnute
- BASIC direct zustava OFF, OS ROM zustava jen guarded probe/service vrstva
- REALOS/BASIC, G7/GTIA/BCD testy zustavaji

Dulezite:
- Tento build muze rozbit obraz u her, ktere predtim drzely jen diky historicke sablone. To je zamer: bud jadro obstalo, nebo log ukaze skutecnou chybu v ANTIC/DLIST/PMG.

Test:
1. TEST DONKEY KONG XEX, snapshot: hledej STREAMED=OK ACTIVE=YES a TOTAL PURGE FIX197.
2. TEST SUPER COBRA XEX, snapshot.
3. Pokud obraz spadne, nevracet hacky; poslat snapshot a opravuje se obecny ANTIC/DLIST/PMG resolver.
