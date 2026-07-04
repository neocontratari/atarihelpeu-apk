BUILD2SA5T / STAGE168 - Sega ZIP route stays in Sega, no 130XE fallback

Co opravuje:
- Stage167 provider relay fungoval, ale pri selhani posledniho download fallbacku catch blok otevrel EMU_URL.
- EMU_URL je 130XE, proto se po Sega pokusu mohlo skocit do VBXE/130XE obrazovky.
- Ten skok je odstranen: download chyba uz nesmi mimo 130XE nacist 130XE.

Sega routovani:
- Klik na ZIP v kontextu Sega sbirky page_id=1003 jde pres downloadAndRunSegaArchive().
- ZIP se po stazeni rozbali a hleda se .gen/.md/.smd/.sms/.68k/.sgd.
- Nalezena ROM jde rovnou do Sega emu pres jednu spolecnou cestu openSegaRomBytes().
- Raw Sega ROM odkazy zustavaji podporovane.
- Obecny Atari ZIP z NET HRY stale zustava 130XE cesta, pokud neni Sega ROM uvnitr.

Zachovano ze Stage167:
- atarihelp.eu a vedos.cz se tahaji pres provider relay.
- Prvni otestovana cesta zustava https://proxy.cors.sh/<puvodni_url>.
- AllOrigins/CorsProxy/Jina zustavaji jako nouzove fallbacky.
- PS1 Stage164 audio rollback zustava beze zmen.
- Zadny BIOS ZIP import neni pridan.

Log markery:
- BUILD2SA5T SEGA_ARCHIVE_DOWNLOADED
- BUILD2SA5T SEGA_ROUTE_OPEN
- BUILD2SA5T ZIP_CONTAINS_SEGA
- BUILD2SA5T WEB_GAME_DOWNLOAD_FAIL
- BUILD2SA5S PROVIDER_RELAY_OK mode=0

Test cil:
- Z menu Sega -> SBIRKA -> Sega Games -> Sonic ZIP se musi vratit/zustat v Sega emu.
- Pokud nejaky relay download spadne, nesmi to samo otevrit 130XE READY obrazovku.
