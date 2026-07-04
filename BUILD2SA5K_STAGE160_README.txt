AtariHelp EMU-10 BUILD2SA5K STAGE160

Problem:
- NET HRY / SBIRKA uz zustavaji uvnitr app WebView, ale v Noxu se AtariHelp stranka zobrazi jako cerna/prázdna.
- Logy neukazuji bezici Sega core pres web; spis jde o WebView render/sit/TLS.

Co je opraveno:
1) Normalni webovy rezim pro atarihelp.eu
   - Pri nacteni atarihelp.eu se WebView prepne z pruhledneho Sega-native rezimu na normalni bily nepruhledny web.
   - Layer type se pro normalni web vraci na NONE, aby pruhledny hardware overlay nedelal cernou obrazovku.
   - Log marker: BUILD2SA5K WEBVIEW_NORMAL_WEB.

2) Diagnostika misto cerne diry
   - Kdyz Android WebView nedokaze nacist hlavni atarihelp.eu stranku, zobrazi bilou diagnostickou stranku s chybou.
   - Log marker: BUILD2SA5K ATARIHELP_LOAD_ERROR.

3) Browser-like UA a hlavicky
   - WebView pouziva desktop Chrome User-Agent.
   - ZIP downloader pro atarihelp.eu posila User-Agent, Accept, Accept-Language a Referer.

4) Herni bridge zustava vnitrni
   - AHNet.openGames() porad nacita https://atarihelp.eu/?page_id=207 uvnitr app WebView.
   - Klik na .zip/.xex/.atr/.com/.exe/.gen/.md/.smd/.sms/.68k/.sgd porad vola AHNET.runGameUrl().
   - ZIP s XEX jde do Atari/130XE cesty.
   - ZIP s GEN/MD/SMD/SMS jde do Sega cesty.

Zamerne NEzmeneno:
- emu_sega/index.html neni v baliku.
- Sega native core neni v baliku.
- 130XE core/assets nejsou v baliku.

Poznamka:
- Stranky https://atarihelp.eu/ a https://atarihelp.eu/?page_id=207 odpovidaji z normalniho weboveho prohlizece.
- Pokud Stage160 ukaze bilou diagnostiku s chybou typu connection closed / timeout, je problem v hostingu/TLS/WAF pro Android/Nox WebView, ne v ZIP loaderu.
