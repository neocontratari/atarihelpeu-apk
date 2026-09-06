# B240 — tři opravy podle skutečného logu (versionCode 286)

Rene poslal `/8765/log` + screenshot ("beze změny, chyby zůstaly") a
dvě další hlášení: appka se pořád ptá na uložení hry, i když ji nahraje
z telefonu, a uložené pozice musí přežít odinstalaci appky jako u PS1.

===============================================================================
 1) PROLÍNÁNÍ OBRAZU DO PANELŮ - PRAVDĚPODOBNÁ PŘÍČINA A OPRAVA
===============================================================================

  Log ukázal řadu `SEGA_PLOCHA_JS_POZADAVEK show=false/true` volání,
  ale ANI JEDNOU se v logu neobjevilo `PLOCHA_ZRUSENA`/`PLOCHA_VYTVORENA`
  kolem těchhle přepínačů (na rozdíl od PS1 v B224, kde `setVisibility`
  prokazatelně ničil a znovu stavěl celý Surface). To znamená, že
  samotný mechanismus (`setAlpha`) pravděpodobně funguje správně -
  problém je nejspíš v ČASOVÁNÍ.

  `napSegaPlochaHide()` pošle žádost přes JS→Java most, Java to
  zpracuje na UI vlákně - tohle NENÍ okamžité. Když se panel v HTML
  hned na dalším řádku synchronně otevře, může se objevit na
  obrazovce DŘÍV, než nativní `alpha=0` stihne dorazit.

  OPRAVA: všechny čtyři Sega panely (D-PAD, OVLÁDÁNÍ, MOJE HRY,
  ULOŽENÉ POZICE) teď před skutečným zobrazením počkají 90 milisekund
  - sotva postřehnutelné pro člověka, ale dost času na to, aby žádost
  o schování doopravdy dorazila dřív, než je co prosvítat.

  POCTIVÁ POZNÁMKA: tohle je moje nejlepší analýza z dostupných dat,
  ne stoprocentní jistota - proto zůstává v logu i dost detailů
  (`SEGA_PLOCHA_JS_POZADAVEK`, `panelSchovej=`, `stranka=`), aby šlo
  příště přesně vidět, jestli tohle stačilo, nebo je příčina jinde.

===============================================================================
 2) "PROČ SE MĚ TO POŘÁD PTÁ, JESTLI CHCI ULOŽIT HRU?"
===============================================================================

  Log to potvrdil na první pohled - "Aladdin" byl spuštěný TŘIKRÁT
  přes `SEGA_LIBRARY_LAUNCH` (tedy z KNIHOVNY, hra už dávno uložená),
  a pokaždé appka nabídla "uložit do telefonu"?!

  PŘÍČINA: `segaLibraryLaunch()` (spuštění hry, co UŽ v knihovně je)
  posílala hru do JS STEJNOU cestou (`napInjectRomBase64`) jako
  SBÍRKA (kde uložení dává smysl, protože jde o NOVOU hru) - a ta
  cesta VŽDY nastavila příznak "nabídni uložení", bez ohledu na to,
  odkud hra přišla.

  OPRAVA: `napInjectRomBase64` má nový třetí parametr - knihovna ho
  posílá jako `false` (nikdy nenabízet, hra už uložená je), SBÍRKA ho
  vůbec neposílá (zůstává původní chování - vždy nabídnout).

===============================================================================
 3) ULOŽENÉ POZICE PŘEŽIJÍ ODINSTALACI - PŘESNĚ JAKO PS1
===============================================================================

  Přesunuto z appce-privátního úložiště do veřejného
  (`Downloads/AtariHelp/Sega_ulozene_pozice`) - stejná poučka jako u
  PS1 paměťové karty v B225: uložená pozice je "fyzicky oddělená od
  konzole", měla by přežít i smazání appky.

  Jednorázová migrace existujících pozic ze staré cesty, ať nepřijdeš
  o rozehrané testování kvůli téhle změně.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  `{ }` v MainActivity.java - 2439/2439.

  Všechny čtyři existující jsdom simulace spuštěny znovu - tři beze
  změny prošly (layout, nebula, savestate), čtvrtá (knihovna) musela
  být upravena kvůli novému 90ms zpoždění (očekávaná změna, ne
  regrese) a teď navíc přímo ověřuje:
    - panel se NEotevře okamžitě po kliku (dočasně, kvůli zpoždění)
    - panel SE otevře po 120ms
    - spuštění hry Z KNIHOVNY už NEUKÁŽE nabídku "uložit do telefonu"
    - spuštění ze SBÍRKY (`napInjectRomBase64` bez třetího parametru)
      nabídku pořád správně ukáže

  CO NEJDE OVĚŘIT ODSUD: jestli 90ms zpoždění skutečně vyřeší
  prolínání obrazu na reálném zařízení - to je moje nejlepší analýza
  z dostupného logu, ne jistota. Prosím otestuj a v případě
  přetrvávání pošli nový log - teď bude mít dost detailů na přesnou
  diagnózu.
