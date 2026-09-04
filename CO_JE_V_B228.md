# B228 — pojistka proti "reset na reset" (versionCode 274)

Rene po důkladnějším otestování B227: "Vsechno beru zpet - po dukladnem
testu - vse funguje. Jediny brouk bug je v tom kdyz das reset na
reset - jinak to jede na sto procent."

===============================================================================
 PŘÍČINA
===============================================================================

  `ps1Reset()` volala `stopPs1SessionHard()` + `ps1MaybeStartBios()`
  BEZ OHLEDU na to, jestli už z PŘEDCHOZÍHO stisku RESETU neběží start
  BIOSu na pozadí (`ps1MaybeStartBios()` běží ve vlastním vlákně, trvá
  několik vteřin).

  Při rychlém druhém stisku: `ps1BiosRunning` ještě nebylo `true`
  (BIOS z prvního resetu ještě nedoběhl) - takže `stopPs1SessionHard()`
  neviděla ŽÁDNOU "aktivní" relaci a hned se vrátila
  (`PS1_ALREADY_STOPPED`) - ale mezitím stihla zasáhnout do stavu
  (zvýšit `ps1LifecycleGen`, zavolat `stopPs1Audio()` atd.), na kterém
  PRVNÍ, ještě běžící start BIOSu pracoval. Odtud "po resetu jen zvuk,
  žádná grafika" - první boot se rozbil uprostřed.

===============================================================================
 OPRAVA - DVĚ VRSTVY
===============================================================================

  1) Java - `ps1Reset()` na začátku zkontroluje `ps1BiosStarting`.
     Pokud už BIOS start běží (z předchozího resetu), NIC nedělá - jen
     no-op vrátí `PS1_RESET_UZ_BEZI`, místo aby zasahovala do
     rozdělaného startu.

  2) JS - tlačítko RESET se samo na dobu resetu (8,5 s) "vypne" -
     `window.__napPs1ResetBusy` flag ignoruje další kliky, než se
     appka vůbec zeptá Javy. Dvojitá pojistka - nespoléhat jen na
     jednu stranu.

  B226 funkčnost (CD/ISO, plocha) a B227 průhlednost D-padu beze
  změny - jak Rene žádal, nic jiného se nedotýkalo.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. `stranka_kontrola.py` (TV cesta) - 0 chyb.
  Rovnováha { } v MainActivity.java - 2365/2365.

  Nová jsdom simulace (test_ps1_reset_double.js):
    - tři rychlé kliky na RESET za sebou -> `AHPS1.ps1Reset()` zavoláno
      jen JEDNOU
    - po uplynutí doby resetu (8,5 s) další klik už normálně projde
      (celkem podruhé)

  Zbylých pět existujících simulací (transparentnost, indikátor v
  monitoru, CD/ISO přehození, plocha) spuštěno znovu - beze změny
  prošly, B226/B227 funkčnost nedotčena.
