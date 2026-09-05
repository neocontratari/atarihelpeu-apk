# B235 — Sega ve stylu Sonic (versionCode 281)

Rene: "ano PS1 mame uzavrenou Partáku - teď Sega - totéž udělej i tam,
ale barevně jinak, ať to ladí se Sonicem. A dej si pozor na tlačítko
START - dej ho dolu doprostřed, ať se ti nepřekrývají pozice."

===============================================================================
 1) SCHOVÁVÁNÍ NATIVNÍ PLOCHY - STEJNÝ PRINCIP, ALE JEDNODUŠŠÍ
===============================================================================

  Sega má svoje `segaPlocha` (SurfaceView), stejně jako PS1 - a stejný
  problém: HTML panel ji sám o sobě nemůže schovat, protože je to
  nativní vrstva mimo WebView.

  Na rozdíl od PS1 ale Sega NIKDY nepoužívá `setZOrderOnTop()` -
  `segaPlocha` je vždycky POD stránkou (jako PS1 v landscape). Díky
  tomu stačí čisté `setAlpha()` vždycky - žádná ta složitost s
  `plochaAplikujViditelnost()`, kterou PS1 potřeboval kvůli dvěma
  různým vrstvám (portrait/landscape).

  Nová `AHSega.segaPlochaVisible(show)` + `segaPlochaSchovanaKvuliPanelu`
  příznak, respektovaný i v 300ms hlídači (`plochaZkontroluj`, Sega
  větev) - stejná poučka jako v B223 (hlídač musí o příznaku vědět,
  jinak ho během pár set milisekund přepíše zpátky).

===============================================================================
 2) "GREEN HILL ZONE" MÍSTO HLUBOKÉHO VESMÍRU
===============================================================================

  Sdílené `#segaNebula` pozadí (stejný princip jako `#panelNebula` u
  PS1 - jedno pozadí za všemi panely, ne kopie v každém):

      obloha - modrý gradient (tmavší nahoře, světlejší dole)
      zelené kopce - dvě vrstvy, pomalu se komíhají do stran
      mráčky - tři, pomalu plují napříč obrazovkou
      zlaté prsteny (Sonicovy kroužky!) - lítají místo planet z PS1,
        kruh s průhledným středem místo plné barvy
      rychlý bílý pruh - misto meteoru z PS1, sedí lépe k Sonicově
        rychlosti - proletí jednou za pár vteřin

  ROVNOU ZABUDOVANÁ POUČKA Z PS1 (B234): `layoutEditEnter()` hned po
  schování plochy vypíná `segaNebulaOn` - jinak by pozadí (vyšší vrstva
  než D-pad/tlačítka) při editaci rozložení D-pad zakrylo, přesně jak
  se to stalo u PS1.

===============================================================================
 3) BARVY PODLE ROLE - SONICOVA PALETA MÍSTO PS1 TLAČÍTEK
===============================================================================

  Nadpisy: D-PAD A OVLÁDÁNÍ / OVLÁDÁNÍ - modrá/zelená (obloha/tráva).
  Tlačítka podle role: potvrdit = zelená (tráva), zavřít (nová třída
  `.zavrit`, na obou ZAVŘÍT tlačítkách) = modrá (obloha), vedlejší =
  zlatá (prsteny).

===============================================================================
 4) START - DOLŮ DOPROSTŘED
===============================================================================

  Dřív: vpravo nahoře nad sloupcem A/B/C (`right:3vw; bottom:56vh`).
  Teď: `left:50%; transform:translateX(-50%); bottom:1.8vh` - úplně
  dole, uprostřed, na STEJNÉ spodní hraně jako D-pad (vlevo) a spodek
  sloupce A/B/C (vpravo). Sedí přesně v mezeře mezi nimi, nic
  nepřekrývá.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (53). Žádné duplicitní
  CSS selektory (`.panel`, `.panel button`, `.panel button.secondary`,
  `.panel button.zavrit`, `.panel h2` - každé přesně jednou).
  `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha { } v
  MainActivity.java - 2372/2372.

  Stará jsdom simulace (test_sega_layout.js) spuštěná znovu na novém
  kódu - beze změny prošla (drag po jednom/po skupinách, citlivost,
  zrcadlení - nic z toho se nedotklo).

  Nová simulace (test_sega_nebula.js):
    - `#segaNebula` existuje, na začátku schované
    - D-PAD panel otevřený → pozadí smí být
    - UPRAVIT ROZLOŽENÍ → pozadí se vypne (D-pad zůstává editovatelný)
    - HOTOVO → pozadí zůstává vypnuté

  CO NEJDE OVĚŘIT ODSUD: přesnou pozici START v landscape přes jsdom -
  jsdom neumí věrně vyhodnotit CSS `@media` dotazy vázané na skutečné
  rozměry obrazovky (stejné omezení jako u PS1 testů). Ověřeno textově
  - CSS pravidlo v souboru obsahuje přesně `left:50%;
  transform:translateX(-50%); bottom:1.8vh`. Skutečné neprekrývání s
  D-padem/sloupcem A/B/C na konkrétním zařízení je na tvém testu.
