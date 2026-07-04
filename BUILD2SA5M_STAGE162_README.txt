AtariHelp EMU-10 BUILD2SA5M STAGE162

Proc:
- Rene ma podezreni na IP ban / rate limit u WEDOS.
- I mobilni Chrome a Nox browser mimo aplikaci hlasi chyby na WEDOS/atarihelp.eu.
- Je potreba mit jistotu, ze aplikace nema smycku, ktera by web porad trefovala.

Kontrola kodu:
- Appka nema periodicky timer, ktery by sam opakovane nacital atarihelp.eu.
- Cesty na atarihelp.eu jsou jen:
  1) uzivatel zmackne NET HRY / SBIRKA,
  2) uzivatel klikne odkaz ve WebView,
  3) uzivatel klikne ZIP/XEX/GEN a appka stahuje soubor.

Oprava BUILD2SA5M:
1) Vypnut automaticky HTTPS -> HTTP retry.
   - Stage161 zkousela po HTTPS chybe jednou HTTP fallback.
   - Stage162 uz nic automaticky neopakuje.

2) Pridana ochranna brzda.
   - Minimalni mezera pro dalsi nacitani stranky atarihelp.eu: 30 sekund.
   - Po chybe nacitani atarihelp.eu: 15 minut cooldown.
   - Behem cooldownu appka zobrazi lokalni stranku "AtariHelp ochranna brzda" a neposle zadny dalsi request.
   - Logy:
     BUILD2SA5M ATARIHELP_REQUEST_ALLOWED
     BUILD2SA5M ATARIHELP_REQUEST_BLOCKED_RATE
     BUILD2SA5M ATARIHELP_REQUEST_BLOCKED_COOLDOWN
     BUILD2SA5M ATARIHELP_LOAD_ERROR_COOLDOWN

3) ZIP klik zustava pouzitelny.
   - Uzivatelsky klik na ZIP/XEX/GEN neni blokovan jen proto, ze se pred chvili nacetla stranka.
   - Pokud je ale aktivni 15min cooldown po chybe, i ZIP stazeni se zastavi, aby appka nezhorsovala ban.

Zamerne NEzmeneno:
- emu_sega/index.html neni v baliku.
- Sega native core neni v baliku.
- 130XE core/assets nejsou v baliku.

Doporuceni ted:
- Netestovat web opakovane z APK/Nox/mobilu.
- Pockat na odezneni IP limitu nebo resit WEDOS Protection / WAF / rate-limit.
