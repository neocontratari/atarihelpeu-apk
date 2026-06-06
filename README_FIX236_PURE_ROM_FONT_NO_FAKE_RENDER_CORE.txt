AtariHelp.eu EMU-09 FIX236 PURE ROM FONT NO FAKE RENDER CORE

Proc FIX236:
- Rene upozornil, ze READY ve FIX235 muze podle tvaru pisma vypadat umele.
- Audit kodu potvrdil, ze v REALOS/PURE HW rendereru stale existovaly rizikove cesty: FONT5X7/synthetic charset fallback, forced READY blue/white paleta a stare kreslene READY/ROM AUDIT probe obrazovky.
- FIX236 tyhle cesty v REALOS/PURE HW zavira. Kdyz ROM/CHBASE nedoda znakova data, renderer radeji kresli prazdno nez hezkou malovanku.

Zmeny:
1) REALOS/PURE textovy renderer:
   - znaky bere jen z ATARIXL.ROM charsetu ($E000) nebo z realne RAM charset oblasti podle CHBASE.
   - synthetic FONT5X7 fallback je v REALOS/PURE zablokovany.

2) Barvy:
   - odstranene nucene READY blue/white v REALOS/PURE render ceste.
   - barvy se berou z GTIA registru / OS shadow hodnot, bez citelnostniho prikraslovani.

3) Stare probe obrazovky:
   - kreslene FIX181 READY/MEMO PAD a kresleny ROM AUDIT screen jsou presmerovane na PURE HW BASIC/AUDIT.
   - nahodne kliknuti uz nema ukazat malovanou obrazovku.

4) Snapshot:
   - pridan FIX236 NO-FAKE RENDER CHECK.
   - vypise zdroj charsetu, paletu a SCREEN PROOF: DLIST/SAVMSC/CHBASE + prvnich 6 radku obrazove RAM v textu i hexa bajtech.

Test plan:
1. Rozbal overlay do lokalniho repo pres GitHub Desktop workflow.
2. Commit summary: FIX236 pure ROM font no fake render core.
3. GitHub Actions postavi APK.
4. V APK spust:
   a) PURE HW AUDIT -> uloz snapshot/log.
   b) POWER XL/XE BASIC -> pockej na obraz, udelej screenshot a SNAPSHOT.
   c) POWER OPTION SELF TEST -> pockej na obraz/zvuk, udelej screenshot a SNAPSHOT.
5. Posli hlavne snapshot TXT, kde musi byt:
   - BUILD FIX236_PURE_ROM_FONT_NO_FAKE_RENDER_CORE
   - FIX236 NO-FAKE RENDER CHECK
   - RENDER ... no FONT5X7
   - PALETTE ... no forced READY blue/white
   - SCREEN PROOF s radky SAVMSC

KODY JSOU STEJNE
- BASIC/Altirra porovnavaci kody se v tomto overlayi nemenily.
- Menil se pouze emulator/render/core audit kolem PURE HW REALOS.
