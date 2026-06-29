BUILD2OH_SEGA_SAFE_REGION_ROLLBACK_STAGE46

CIL:
- okamzite vratit funkcni nacitani her po rozbite vetvi 2OG
- udrzet zvuk cisty jako v 2OD
- neskakat dal do region patchu, dokud neni potvrzeno, ze vsechny normalni hry znovu startuji

TEST PLAN:
1) Uplne zavri aplikaci.
2) Spust appku.
3) Sega -> Sonic nebo Chase nebo Chess.
   Ocekavani: hra znovu normalne najede, zadne "Failed to load the cartridge file".
4) Sega -> druha USA/World hra.
   Ocekavani: hra najede, zvuk zustava chvalitebny/cisty.
5) Sega -> Asterix Europe nebo Aladdin Europe.
   Ocekavani: muze porad zustat PAL/SECAM hlaska. To neni v tomto buildu opraveno.

V LOGU CHCI VIDET:
- BUILD2OH_SEGA_SAFE_REGION_ROLLBACK_STAGE46
- REGION_SAFE_ROLLBACK_2OH using Module.arguments=[blobURL], no --user, no --cartridge
- BOOTING_ROM ... blob=blob:file:///
- SCRIPT_APPEND https://sonicresearch.org/clownacy/clownmdemu.js
- MODULE_POSTRUN
- zadne ERR WARNING: Could not load the cartridge file u normalni USA/World hry

CO POSLAT ZPET:
- Sonic/Chase/Chess: jede / nejede
- zvuk: cisty / chrci
- Asterix/Aladdin EU: porad region hlaska / jina zmena
- TXT log
