# B222 — odpočet, silnější oprava prolínání, grafika ve stylu PS1 BIOS (versionCode 268)

Rene po B221: "mas to tam furt, ale jakakoli dalsi akce to okno vymaze,
ale furt to neni stoprocentni... dej si zalezet na te grafice vyberu
her, at to ma nejaky styl, tohle vypada jako pracovni verze, inspiruj
se grafikou z bios ps1 bez cd... zadny odpocet neni videt kdyz kliknu
play v cd/iso."

===============================================================================
 1) SILNĚJŠÍ OPRAVA PROLÍNÁNÍ - setAlpha VEDLE setVisibility
===============================================================================

  B221 řešila správnou věc (ps1Plocha je nativní SurfaceView nad
  WebView, žádné CSS ji neschová), ale `View.setVisibility()` u
  SurfaceView je známý problém - má VLASTNÍ hardwarovou vrstvu mimo
  normální kreslení Androidu, a změna viditelnosti se občas neprojeví
  hned, až po dalším překreslení (přesně to, co popsal - "jakákoli
  další akce to vymaže").

  `ps1PlochaVisible()` teď dělá OBOJÍ: `setAlpha(0f/1f)` PŘED
  `setVisibility()`. Alpha působí přímo na GPU vrstvu a je spolehlivější
  než jen visibility flag. Pořád žádný zásah do EGL, vlákna
  renderování, ani bootování jádra - jen silnější verze té samé, už
  bezpečné věci z B221.

===============================================================================
 2) GRAFIKA VE STYLU PS1 BIOS
===============================================================================

  Všechny panely (`.panel`, `.panel .box`, `.libGame`) předělány:
    - Pozadí panelu: modrá zář uprostřed, tmavá k okrajům - stejný
      dojem jako "MAIN MENU" BIOS obrazovka na skinu
    - Box: dvojitý zlato-stříbrný rám (vnější zlatý okraj + jemný
      vnitřní modrý rámeček), stín/lesk jako kovová konzole
    - Nadpisy: širší rozestup písmen, jemná zlatá záře
    - Hry v knihovně: teď vypadají jako CD sloty - jemný přechod
      pozadí, zlatý okraj, kulaté prosvětlené tlačítko PLAY (stejný
      vizuální jazyk jako D-pad/akční tlačítka - `radial-gradient` +
      zlatá záře)

===============================================================================
 3) SKUTEČNÝ TIKAJÍCÍ ODPOČET
===============================================================================

  Místo statické hlášky "může to trvat ~10s" teď `startBootCountdown()`
  - text v `#toast` se každou vteřinu mění: "Spouštím hru… 10 s" →
  "9 s" → "8 s"... až na 0, pak se sám schová. Jasně vidět, že se něco
  děje, ne že appka zamrzla.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (76 zkontrolováno).
  `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha { } v
  MainActivity.java - 2356/2356.

  Nová jsdom simulace (test_ps1_countdown.js) - skutečný klik na PLAY
  v knihovně:
    - text ihned po kliku obsahuje "10 s"
    - po 1 vteřině textu je "9 s"
    - po 2 vteřinách "8 s"
  (tiká doopravdy, ne jen jedna statická hláška)

  Vzhled panelů nejde ověřit automatizovaně (CSS/vizuál) - na to bude
  potřeba tvůj test na telefonu, ale syntaxe i chování jsou ověřené.
