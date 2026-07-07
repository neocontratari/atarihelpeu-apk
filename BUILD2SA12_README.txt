BUILD2SA12B_STAGE200_BIOS_ZERO_CLICK_DIRECT_URL (obsahuje SA7-SA10; aplikuj + SA11 memcard)

BIOS NA NULA KLIKU - TVUJ SEN, BEZPECNOU CESTOU:
Pri prvnim spusteni PS1 hry (kdyz BIOS jeste neni):
1) appka si PRECTE tvoji stranku page_id=1048 primo z telefonu (jako
   normalni navstevnik) a najde na ni BIOS ZIP odkaz - i kdyz soubor
   nekdy presunes, najde ho znovu
2) stahne, overi (scph*.bin, 524288 B), NAINSTALUJE vsechny + kopii ulozi
   do Download/AtariHelp (preziti preinstalace)
3) uzivatel NEUDELA NIC - zapne hru a jede SONY znelka
Zalozni cesty zustavaji: klik na webu v appce, LOAD GAME, rucni kopie.
Markery: PS1_BIOS_AUTO_DL_TRY / PS1_BIOS_AUTO_DOWNLOADED / banner
PS1_BIOS_AUTO_INSTALLED. BIOS NENI v APK - repo/Actions v bezpeci.

SA12B: primy odkaz od Reneho zadratovan jako PRVNI volba:
https://atarihelp.eu/wp-content/uploads/2026/07/PS1-BIOS_.zip
Poradi pokusu: 1) primy odkaz 2) sken stranky page_id=1048 (samolecba,
kdyz soubor presunes) 3) zalozni cesty (web klik/LOAD GAME/rucni kopie).
Muj server dostava od webu 403 (ochrana proti datacentrum) - z telefonu
jako bezny navstevnik to projde; kdyby ne, v logu bude AUTO_DL_FAIL 403
a vyresime whitelisting.

TEST: cisty stav (SMAZAT PS1 CACHE + smaz Download/AtariHelp/_PS1_-_BIOS_.ZIP
a scph* v Download) -> spust hru z webu -> v logu AUTO_DOWNLOADED ->
SONY logo BEZ jedineho kroku navic.
