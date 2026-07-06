BUILD2SA5AR_STAGE192_PS1_ENOSPC_SPACE_GUARD - OPRAVA "write failed ENOSPC"

CO SE DELO (z tveho screenshotu ENOSPC = "No space left on device" = PLNY DISK):
1) Kazda stazena hra = ZIP (~700 MB) + rozbaleny BIN (~700 MB) a cache drzela
   VSECHNY drivejsi hry. Druha hra uz se na S8 proste nevesla.
2) Stazeny ZIP se po rozbaleni NIKDY nemazal - lezel tam navzdy.
3) Tlacitko SMAZAT PS1 CACHE pri BEZICI hre jen tise ohlasilo "BUSY" a nic
   nesmazalo - proto "i po vymazani cache" porad write failed. Nebyl to
   nesmazany binec navic, byla to nefunkcni operace mazani.
4) "Hry z netu se najednou vubec nepousti" + "Tekken z netu se sekne" =
   dusledek plneho disku: stahovani se utne v pulce, hra je ulomena.
   Po teto oprave se to ma srovnat samo - Tekken zkus znovu.

OPRAVA (1 soubor - MainActivity.java, zaklad = tvuj STAGE191):
- PRED kazdym stazenim se automaticky uklidi VSECHNY stare stazene hry
  (verejna Downloads/AtariHelp/PS1 i privatni slozka), krome cache prave
  stahovane hry. Marker: PS1_CACHE_AUTO_CLEAN freed=...
- ZIP se po uspesnem rozbaleni SMAZE. Marker: PS1_ZIP_DELETED_AFTER_EXTRACT.
- Pred stazenim KONTROLA VOLNEHO MISTA (potreba ~2x velikost hry + rezerva):
  kdyz misto neni, dostanes srozumitelnou hlasku "MALO MISTA... uvolni misto",
  ne ENOSPC v pulce. Marker: PS1_SPACE_CHECK need=... usable=...
- SMAZAT PS1 CACHE ted funguje i pri bezici hre: hru napred korektne zastavi
  a pak smaze. Marker: PS1_CACHE_CLEAR_AUTOSTOP.
- Banner: novy pokus o stazeni prepise stary FAIL napis (PS1_REMOTE_START).

DUSLEDEK: v mobilu je vzdy MAX JEDNA stazena hra (ta posledni - cache hit
funguje pri opakovanem spusteni te same hry). Zadne hromadeni.

ZVUK PS1 (chrceni): to je rychlost interpreter jadra, ne zapojeni - realne
reseni je dynarec etapa (jako -O3 u Segy). Bereme hned po potvrzeni teto opravy.
VEREJNA IP: diky! Az dynarec dobehnem, muzeme z ni udelat vlastni herni
server misto Google Drive (zadne confirm stranky). Zatim nic nenastavuj.

TEST (S8): 1) SMAZAT PS1 CACHE (i kdyz hra bezi - musi projit)
2) stahni hru A z Drive -> hraje; v logu AUTO_CLEAN + ZIP_DELETED + SPACE_CHECK
3) stahni hru B -> MUSI projit (drive ENOSPC) 4) Tekken 3 z netu znovu.
POZOR NA WORKFLOW: zaklad je presne tvuj STAGE191 soubor + 4 ohranicene
vlozky vyuzivajici jen existujici funkce (deleteTree/formatMb overeny).
Kdyby cervena - screenshot staci.
