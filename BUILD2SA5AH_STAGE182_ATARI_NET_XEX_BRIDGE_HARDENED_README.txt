AtariHelp EMU10 BUILD2SA5AH / Stage182

Ucel:
- opravuje Atari XEX/ZIP z NET HRY, kdy se po stazeni hra nedostala do 130XE
- zachovava protected web relay z predchozich stage
- Sega download cesta zustava oddelena
- lokalni XEX z mobilu zustava bez zmeny

Co je zmeneno:
- lokalni Atari NET HRY seznam pouziva vnitrni ahgame:// odkaz
- Android zachyti ahgame:// primo ve WebView navigaci, ne jen pres onclick/JavascriptInterface
- po stazeni Atari payloadu se 130XE otevre pres cisty file:///android_asset/emu/index.html
- XEX injekce ceka na AHRECV bridge a pote jeste kratce pocka na ustaleni 130XE stranky
- fallback injekce zustava, ale je posunuta pozdeji
- ulozeny log .txt/.log doplni i ANDROID MAINACTIVITY LOG, aby bylo videt:
  BUILD2SA5AH AHGAME_BRIDGE
  BUILD2SA5AH ROUTE_GAME
  BUILD2SA5AF WEB_GAME_DOWNLOADED
  BUILD2SA5AF ZIP_CONTAINS_ATARI
  BUILD2SA5AH EMU130_INJECT_COMMIT

Co neni zmeneno:
- zadny BIOS
- zadne 130XE jadro/assets
- zadne Sega jadro/assets
- zadna zmena lokalniho vyberu XEX z telefonu
