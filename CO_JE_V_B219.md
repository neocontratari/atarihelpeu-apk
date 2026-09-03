# B219 — stejný D-pad systém v Seze (versionCode 265)

Rene: "Ted by to chtelo udelat v emu sega s d-pad uplne to same jako v
emu ps1. Samostatne nastaveni atd proste na uplnem principu jako v
ps1... nezapomen pridat tlacitko start na sirku, to tam nikdy nebylo."

===============================================================================
 CO BYLO V SEZE JINAK NEŽ V PS1 (zjištěno PŘED zásahem, ne po)
===============================================================================

  - D-pad UŽ MĚL pohyblivý knoflík sledující prst (`#dpadNub`) - psáno s
    přesně tou samou poučkou, na kterou jsem u PS1 přišel až po chybě:
    "zadny kriz, jen puntik co klouze pod palcem." Tohle jsem nepřepisoval,
    jen na to napojil možnost přesunu.
  - Vstup nejde přes samostatné posluchače na tlačítkách (jako PS1), ale
    přes JEDEN globální dotykový handler (`update(ev)`) na `#stage`, který
    se ptá "co je pod každým prstem" při každém doteku. Přesun tlačítek
    tomu nevadí - `getBoundingClientRect()` se čte vždy čerstvě, takže
    handler automaticky najde tlačítko na nové pozici. Jen jsem musel
    tenhle handler v editačním režimu ÚPLNĚ VYPNOUT (`if(layoutEditActive)
    return;`), protože per-element `capture:true` (vzor z PS1) by tu
    nezabral - handler na `#stage` by dotyk dostal dřív (capture fáze jde
    od kořene dolů, `#stage` je nadřazený element).
  - Žádný Settings panel neexistoval vůbec - musel jsem postavit celou
    infrastrukturu (`.panel`, D-PAD A OVLÁDÁNÍ, OVLÁDÁNÍ CITLIVOST) od
    nuly, jen s modrým Sega nádechem místo zlatého PS1.
  - Sega nemá ramena ani Select - jen D-pad, A/B/C, Start. Skupina pro
    "po skupinách" je proto jen `[btnA, btnB, btnC]` - D-pad a Start se
    táhnou vždy samostatně.
  - **START BYLO V LANDSCAPE ÚPLNĚ VYPNUTÉ** (`display:none!important;
    pointer-events:none!important;`) - v landscape (kde se skutečně
    hraje) nešlo vůbec zmáčknout. Přidáno: viditelné, funkční, součástí
    `update()` dotykové detekce i celého přesouvacího systému.

===============================================================================
 CO SE PŘIDALO (stejný seznam jako u PS1, přes AHSega místo AHPS1)
===============================================================================

  Java: `napVibrator()` přesunut na úroveň `MainActivity` (sdílený s
  PS1), nová třída `AHSega` (`segaLog`, `segaVibrate`), zaregistrovaná
  do WebView jako `AHSega`.

  emu_sega/index.html:
    - Nové ozubené kolečko (`#btnSegaSettings`, landscape) → přímo na
      D-PAD A OVLÁDÁNÍ panel (rozložení / citlivost+vzhled / levák)
    - UPRAVIT ROZLOŽENÍ TLAČÍTEK - PO JEDNOM / PO SKUPINÁCH přepínač,
      stejná logika ořezávání skupiny jako tuhé těleso (B217 lekce)
    - OVLÁDÁNÍ: citlivost D-padu (napojeno na existující `dp.r*0.07`
      práh, teď `dp.r*(ctrlSettings.sensitivity/100)`), průhlednost,
      velikost tlačítek zvlášť od velikosti D-padu, haptika
    - PROHODIT D-PAD A AKCE (LEVÁK) - jen D-pad↔A/B/C, Start zůstává
      (stejné zdůvodnění jako u PS1 ramen - nejde o volbu rukou)
    - Haptika zapojená přímo do `sendToWrapper()` (obdoba PS1 `setPs1`)

===============================================================================
 VLASTNÍ CHYBA, KTEROU JSEM UDĚLAL A HNED CHYTIL
===============================================================================

  Při vkládání velkého bloku kódu jsem omylem smazal začátek funkce
  `isLandscapeNow()` a deklaraci `holdTimers` (str_replace smazal víc,
  než měl). Všiml jsem si toho HNED při první kontrole syntaxe
  (`node --check`) a opravil, než šlo cokoli dál.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (52 zkontrolováno).
  `stranka_kontrola.py` (TV cesta) - 0 chyb.

  Nová jsdom simulace (test_sega_layout.js):
    - všechny nové prvky existují v DOM
    - klik na ozubené kolečko otevře D-PAD panel
    - vstup do editace přidá .layoutEdit VŠEM prvkům včetně btnStart
    - PO JEDNOM: přesun btnA nehne btnB
    - PO SKUPINÁCH: přesun btnB hne i btnC (stejná skupina), D-pad a
      Start zůstávají beze změny
    - HOTOVO uloží do localStorage a zavře editaci
    - citlivost skutečně řídí chování D-padu (50% výchylka při výchozí
      7% citlivosti vyvolá RIGHT)
    - zrcadlení prohodí D-pad a akce, Start zůstává na místě
    - žádná JS chyba nesouvisející s prostředím (jsdom "getContext"
      chyby existovaly už v PŮVODNÍM souboru před mými úpravami -
      ověřeno porovnáním, nesouvisí s touhle změnou)
