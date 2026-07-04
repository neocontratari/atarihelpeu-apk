BUILD2SA5S / STAGE167 - provider relay tested for AtariHelp/WEDOS block

Co je zmeneno:
- atarihelp.eu a vedos.cz zustavaji v appce pod puvodni URL.
- Appka ale jejich WebView pozadavky zachyti a nestahuje je primo pres blokovanou cestu.
- Prvni relay je nove otestovany:
  1. https://proxy.cors.sh/<puvodni_url>
  2. https://api.allorigins.win/raw?url=...
  3. https://corsproxy.io/?url=...
  4. nouzove pro hlavni stranku https://r.jina.ai/<puvodni_url>
- proxy.cors.sh byl realne otestovan na HTML i binarni ZIP:
  - https://atarihelp.eu/
  - https://atarihelp.eu/?page_id=207
  - https://atarihelp.eu/?page_id=21
  - https://atarihelp.eu/?page_id=1003
  - https://vedos.cz/
  - galactic_chase.zip vracel ZIP magic PK
  - Sonic-The-Hedgehog-USA-Europe.zip vracel ZIP magic PK
- Klik na ZIP/XEX/ATR/GEN/MD ze SBIRKY se stahuje pres relay vrstvu a routuje do spravneho emu.
- Sega ZIP s ROM uvnitr jde do Sega emu, Atari ZIP/XEX/ATR jde do 130XE loaderu.
- Reader fallback je jen posledni nouzova cesta pro zobrazeni stranky, ne pro binarni download.

Zustava beze zmeny:
- PS1 Stage164 audio rollback: chunk=384, minPull=128, async CD.
- Zadny BIOS ZIP import neni pridan.
- Sega/130XE assety nejsou soucasti tohoto overlay baliku.

Log markery:
- BUILD2SA5S ATARIHELP_RELAY_LOAD
- BUILD2SA5S PROVIDER_RELAY_OK mode=0
- BUILD2SA5S PROVIDER_RELAY_FAIL
- BUILD2SA5S WEB_GAME_DOWNLOADED
- BUILD2SA5S SEGA_WEB_ROM_DOWNLOADED

Poznamka:
- Z testu z teto site: prime atarihelp.eu/vedos.cz selhalo nebo vracelo 429/connection closed.
- proxy.cors.sh proslo jako funkcni cesta pro stranky i ZIPy.
- Pokud nekdy spadne i verejny relay, dalsi poctivy krok je vlastni maly relay/mirror mimo blokovanou trasu.
