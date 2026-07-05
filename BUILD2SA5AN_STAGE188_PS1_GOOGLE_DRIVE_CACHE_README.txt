AtariHelp EMU10 BUILD2SA5AN / Stage188

Zaklad:
- navazuje na Stage184 Atari/Sega web loader a Stage187 PS1 Google Drive loader
- Sega ani 130XE cesta nejsou menene
- port forwarding / Tenda server neni potreba

PS1 Google Drive cesta:
- PS1 LOAD GAME otevre stranku:
  https://atarihelp.eu/?page_id=1048
- klik na Google Drive odkaz na teto strance se zachyti uvnitr aplikace
- drive.google.com/file/d/.../view se prepise na Google download endpoint
- prvni spusteni hru stahne do app-private PS1 cache
- ZIP se hned rozbali do samostatne cache slozky podle Drive file id
- dalsi spusteni stejneho odkazu uz nesaha na sit a bootuje lokalni cache

Proc to neni uplne bez stazeni:
- PS1 core potrebuje rychle nahodne cteni sektoru
- Google Drive ZIP neni vhodny jako primo streamovane CD
- proto je nejbezpecnejsi prvni stazeni + lokalni cache; pak uz se nestahuje znovu

Podporovane PS1 soubory:
- .zip, .bin, .iso, .img, .pbp, .chd
- ZIP muze obsahovat .cue + .bin; CUE se prepise na lokalni bezpecne nazvy
- prime HTTP/HTTPS .cue je podporovane jen pokud jsou vedle nej dostupne i navazane .bin/.img/.iso/.wav soubory
- Google Drive .cue sam o sobe neni dobra cesta, protoze jeden Drive odkaz neumi dodat navazane BIN soubory vedle CUE
- .7z neni podporovane

Nutne nastaveni Google Drive:
- soubor musi byt sdileny jako "Kdokoli s odkazem" / Viewer
- kdyz Google vrati login HTML misto souboru, log ukaze jasnou chybu

UI:
- CD ISO zustava lokalni picker z telefonu
- LOAD GAME otevre PS1 Google Drive stranku
- SETTINGS obsahuje PS1 GOOGLE DRIVE a PS1 PRIMA URL jako zalozni volby

Log znacky:
- BUILD2SA5AN_STAGE188_PS1_GOOGLE_DRIVE_CACHE
- PS1_REMOTE_CONNECT via=google_drive
- PS1_REMOTE_ZIP_READY
- PS1_REMOTE_CACHE_STORE
- PS1_REMOTE_CACHE_HIT
- PS1_REMOTE_BOOT_OK

Test:
- prvni klik na Crash/Dune muze trvat dlouho, protoze se stahuje a rozbaluje asi 500 MB ZIP
- druhy klik na stejny odkaz ma v logu ukazat PS1_REMOTE_CACHE_HIT a jit rovnou z telefonu

Bezpecnost:
- zadny BIOS/bin/rom/game payload neni soucast overlaye
