AtariHelp EMU-10 BUILD2SA5L STAGE161

Stav zjisteny testem:
- Nox browser sam hlasi pro https://atarihelp.eu/?page_id=207 chybu net::ERR_CONNECTION_CLOSED.
- HTTP bez TLS vraci z hostingu WEDOS.protection 429 Too Many Requests.
- To znamena, ze hlavni blokace je na hostingu / WEDOS ochrane / TLS vrstve pro Android/Nox/WebView, ne v ZIP loaderu aplikace.

Co tahle etapa dela:
1) HTTPS -> HTTP fallback
   - Kdyz WebView nedostane https://atarihelp.eu/?page_id=207, appka jednou automaticky zkusi http://atarihelp.eu/?page_id=207.
   - Log marker: BUILD2SA5L ATARIHELP_HTTPS_FAILED_TRY_HTTP.

2) HTTP 429 je zachycen a pojmenovan
   - Pokud hosting vrati 429, appka ukaze diagnostiku misto cerne obrazovky.
   - Log marker: BUILD2SA5L ATARIHELP_LOAD_ERROR.

3) Fallback obrazovka ma akce
   - Zkusit HTTPS znovu.
   - Zkusit HTTP fallback.
   - Vybrat ZIP z telefonu.
   - Otevrit v externim browseru.

4) ZIP bridge zustava
   - Pokud se stranka nebo soubor dostane do appky, klik na .zip/.xex/.atr/.com/.exe/.gen/.md/.smd/.sms/.68k/.sgd porad vola AHNET.runGameUrl().
   - ZIP s XEX jde do Atari/130XE cesty.
   - ZIP s GEN/MD/SMD/SMS jde do Sega cesty.

Co appka nemuze opravit sama:
- Pokud WEDOS/hosting zavre spojeni uz na TLS handshake nebo vrati 429, aplikace nema odkud HTML/ZIP stahnout.

Trvale reseni na webu:
- Ve WEDOS/hostingu vypnout nebo zmirnit WEDOS.protection pro atarihelp.eu.
- Nebo udelat vyjimku/whitelist pro Android WebView/Nox a cesty:
  - /?page_id=207
  - /?page_id=21
  - /?page_id=1003
  - /wp-content/uploads/*.zip
- Nejlepsi technicke reseni: dat herni ZIPy na subdomenu bez WAF/bot ochrany, napr. files.atarihelp.eu, a na webu odkazovat tam.

Zamerne NEzmeneno:
- emu_sega/index.html neni v baliku.
- Sega native core neni v baliku.
- 130XE core/assets nejsou v baliku.
