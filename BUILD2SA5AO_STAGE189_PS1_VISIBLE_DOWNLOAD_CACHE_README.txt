AtariHelp EMU10 BUILD2SA5AO / Stage189

Zaklad:
- navazuje na Stage184 Atari/Sega web loader a Stage188 PS1 Google Drive cache
- Sega ani 130XE cesta nejsou menene
- tahle etapa resi hlavne viditelnou indikaci PS1 downloadu/cache

PS1 Google Drive cesta:
- PS1 LOAD GAME otevre stranku:
  https://atarihelp.eu/?page_id=1048
- klik na Google Drive odkaz se zachyti uvnitr aplikace
- prvni spusteni hru stahne a rozbali do app-private PS1 cache
- dalsi spusteni stejneho odkazu uz bootuje lokalni cache

Nova indikace:
- PS1 UI zapina status polling automaticky hned po otevreni obrazovky
- pres obrazovku se zobrazi banner se stavy:
  PS1_REMOTE_CONNECT
  PS1_REMOTE_DOWNLOAD
  PS1_REMOTE_ZIP_READY
  PS1_REMOTE_ZIP_EXTRACT
  PS1_REMOTE_CACHE_HIT
  PS1_REMOTE_BOOT_OK
  PS1_REMOTE_FAIL
- pri downloadu se ukazuje procento, pokud server vraci Content-Length
- pri extrakci ZIPu se ukazuje stav rozbalovani
- banner si pamatuje cache cestu a ukazuje ji i behem downloadu

Kam se to uklada:
- do app-private slozky:
  Android/data/eu.atarihelp.emu10/files/ps1_games/
- Google Drive hra ma vlastni podslozku:
  gdrive_<drive-file-id>/
- uvnitr je rozbaleny .cue/.bin nebo primo .iso/.img/.pbp/.chd a marker _ps1_boot.name

Proc to neni uplne bez stazeni:
- PS1 core potrebuje rychle nahodne cteni sektoru
- Google Drive ZIP neni vhodny jako primo streamovane CD
- proto je nejbezpecnejsi prvni stazeni + lokalni cache; pak uz se nestahuje znovu

Podporovane PS1 soubory:
- .zip, .bin, .iso, .img, .pbp, .chd
- ZIP muze obsahovat .cue + .bin; CUE se prepise na lokalni bezpecne nazvy
- .7z neni podporovane

Test:
- klikni PS1 -> LOAD GAME -> hra na Google Drive strance
- po navratu do PS1 se musi nahore ukazat banner CONNECT/DOWNLOAD
- prvni klik muze trvat dlouho, protoze Crash/Dune jsou kolem 500 MB
- druhy klik na stejnou hru ma ukazat PS1_REMOTE_CACHE_HIT

Bezpecnost:
- zadny BIOS/bin/rom/game payload neni soucast overlaye
