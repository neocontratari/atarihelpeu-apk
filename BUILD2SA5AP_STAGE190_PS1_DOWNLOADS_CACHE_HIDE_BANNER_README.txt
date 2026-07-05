AtariHelp EMU10 BUILD2SA5AP / Stage190

Zaklad:
- navazuje na Stage189 PS1 Google Drive download banner
- Sega ani 130XE cesta nejsou menene
- resi dve veci od Reneho testu:
  1) banner po nahrani hry musi okamzite zmizet
  2) PS1 stazene hry musi byt dohledatelne a smazatelne v telefonu

Banner:
- behem downloadu zustava videt:
  PS1_REMOTE_CONNECT
  PS1_REMOTE_DOWNLOAD
  PS1_REMOTE_ZIP_READY
  PS1_REMOTE_ZIP_EXTRACT
  PS1_REMOTE_CACHE_HIT
- jakmile prijde PS1_REMOTE_BOOT_OK, banner se hned schova
- chyby zustavaji videt kratce, aby slo precist duvod

Kam se PS1 hry ukladaji:
- primarne do viditelne slozky:
  Downloads/AtariHelp/PS1/
- kazdy Google Drive soubor ma vlastni podslozku:
  gdrive_<drive-file-id>/
- uvnitr je rozbaleny .cue/.bin nebo primo .iso/.img/.pbp/.chd a marker _ps1_boot.name
- pokud Android primy zapis do Downloads zakaze, appka pouzije fallback app-private cache a zapise to do logu

PS1 settings:
- PS1 CACHE CESTA ukaze presnou slozku
- SMAZAT PS1 CACHE smaze novou viditelnou cache i starou app-private PS1 cache z predchozich buildu

Poznamka k prvnimu testu:
- protoze se cache presunula z app-private slozky do Downloads/AtariHelp/PS1, prvni spusteni muze hru stahnout znovu
- dalsi spusteni stejneho Drive odkazu uz ma jit z cache

Bezpecnost:
- zadny BIOS/bin/rom/game payload neni soucast overlaye
