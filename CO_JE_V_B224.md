# B224 — setVisibility ničil Surface, grafika ve 3D stylu jádra (versionCode 270)

Rene poslal `ps1.txt` z reálného testu B223 plus screenshot černého
obdélníku v LANDSCAPE (nová lokace bugu) a screenshot knihovny s
poznámkou, že grafika je "fádní nehezké."

===============================================================================
 CO UKÁZAL LOG - SETVISIBILITY NIČIL CELÝ SURFACE
===============================================================================

  V `ps1.txt` je u KAŽDÉHO `PLOCHA_JS_POZADAVEK show=false` hned další
  řádek `PLOCHA_ZRUSENA` - a u každého `show=true` zase `PLOCHA_VYTVORENA`.
  `PLOCHA_ZRUSENA`/`PLOCHA_VYTVORENA` znamenají `surfaceDestroyed`/
  `surfaceCreated` - tedy CELÝ podkladový Surface se zničil a znovu
  postavil, ne že by se jen přepnul příznak viditelnosti.

  To je známá vlastnost `SurfaceView.setVisibility(INVISIBLE)` na
  některých Android verzích/zařízeních - vizuálně schová View, ale pod
  kapotou to bere jako "tenhle surface teď nikdo nepotřebuje" a zničí
  ho. Znovu-postavení (nový Surface, nové `setDisplaySurfaceSafe()`
  volání do jádra) je mnohem těžší operace, než jsem čekal - a děla se
  to PŘI KAŽDÉM otevření/zavření panelu. To vysvětluje, proč to "furt
  nebylo stoprocentní" (B221, B222) i proč se objevil ten černý
  obdélník v landscape - znovupostavený povrch chvíli nemá od jádra
  žádný snímek, než dorazí první.

===============================================================================
 OPRAVA
===============================================================================

  Všechna tři místa, která řídí viditelnost plochy kvůli panelu -
  `ps1PlochaVisible()`, obě větve `ps1PlochaUmisti()`, i `plochaZkontroluj()`
  (300ms hlídač, PS1 i Sega větev) - přepnuta z `setVisibility()` na
  ČISTÉ `setAlpha()`. Alpha je jen vykreslovací vlastnost - Surface
  samotný zůstává živý a připojený k jádru po celou dobu, jen se
  nakreslí průhledně. Žádný destroy/create cyklus, žádné opětovné
  `setDisplaySurfaceSafe()` volání jen kvůli otevření menu.

  `setVisibility()` zůstává jen tam, kde patřila původně - jednorázový
  stav "pozice ještě neznámá" při úplně prvním vytvoření plochy
  (`ps1GlEnable()`), ne pro časté přepínání kvůli panelům.

===============================================================================
 GRAFIKA - "3D JAKO JÁDRO PS1", NE PLOCHÉ KARTY
===============================================================================

  B223 udělala barvy poctivě vzorkované ze skinu, ale plochý výsledek
  Rene označil za "fádní nehezké." Recept teď není nový výmysl ani
  návrat k B222 - je to DOSLOVNÉ zopakování `.padBtn` stylu, který už
  appka měla a Rene ho schválil (D-pad/akční tlačítka v landscape):
  tmavý radiální gradient (`rgba(70,78,90,.92)` → `rgba(8,11,16,.96)`),
  zlatý dvojitý prstenec (`rgba(198,165,89,.8)` + vnější `box-shadow`
  prstenec), plně kulaté rohy - `border-radius:999px` (pill tvar u
  širokých tlačítek, plný kruh u PLAY/SMAZAT ikon).

  Panel, tlačítka i řádky her v knihovně teď mají stejný vizuální
  jazyk jako START/R1/R2/Triangle/Circle/Square/Cross - konzistentní
  s tím, co už appka měla a fungovalo.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (76). Žádné duplicitní
  CSS selektory (`.panel .box`, `.panel button`, `.libGame .libPlay`
  každé přesně jednou). `stranka_kontrola.py` (TV cesta) - 0 chyb.
  Rovnováha { } v MainActivity.java - 2355/2355.

  Tři existující jsdom simulace (plocha, odpočet, CD/ISO přehození)
  spuštěné znovu - beze změny prošly, grafický redesign nic funkčně
  nerozbil.

  Jeden starý test (test_ps1_library.js, z B214) teď hlásí chybu - to
  je OČEKÁVANÉ, testuje zastaralé chování (btnMemory otevírající
  knihovnu, což B220 změnilo). Správné, aktuální chování pokrývá
  test_ps1_cd_swap.js, který prochází čistě.

  CO NEJDE OVĚŘIT ODSUD: jestli `setAlpha()` samotné (bez
  `setVisibility`) na tvém konkrétním zařízení skutečně nespouští
  stejný destroy/create cyklus - to uvidíme až v `/8765/log` z dalšího
  testu. Hledej, jestli se `PLOCHA_ZRUSENA`/`PLOCHA_VYTVORENA` ještě
  objevují u běžného otevření/zavření panelu (neměly by - jen u
  otočení obrazovky, kde je zmena vrstvy = prestavet opravdu potřeba).
