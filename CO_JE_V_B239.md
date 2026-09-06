# B239 — segaPlocha se korektně odpojí při opuštění Segy (versionCode 285)

Rene: "logiku máš správné - nicméně - hra se po loadu kousne - máš
překrývání canvasu do všech výběrů - udělej si pořádnou kontrolu - a
nachystej si log, ať ti mám co poslat."

Tentokrát jsem to našel DŘÍVE, než přišel log - důkladnou ruční
kontrolou existujícího kódu, ne čekáním na data. Log jsem si i tak
připravil pro případ, že tohle nestačí úplně (viz níže).

===============================================================================
 SKUTEČNÁ PŘÍČINA - "CANVAS DO VŠECH VÝBĚRŮ"
===============================================================================

  `stopNativeInPlaceHard()` - funkce, kterou appka volá při KAŽDÉM
  opuštění Segy (návrat do hlavní nabídky, přechod na PS1...) -
  zastavila jádro (`NativeSegaCoreBridge.shutdown()`) a uklidila
  STAROU, dnes už nepoužívanou cestu (`nativeInPlaceView`, TextureView
  mechanismus z dřívějška). Ale SKUTEČNOU plochu (`segaPlocha`,
  SurfaceView, na kterou jádro kreslí přímo od B117) se NIKDY
  nedotkla.

  Výsledek: `segaPlocha` zůstávala připojená k `rootFrame`, s plnou
  viditelností (alpha=1), a ukazovala POSLEDNÍ ZAMRZLÝ SNÍMEK ze hry -
  úplně nezávisle na tom, kam appka dál navigovala. Přesně to je
  "canvas přes VŠECHNY výběry" - ať jsi otevřel cokoli po opuštění
  Segy (hlavní nabídku, PS1, jakýkoli panel), ten zamrzlý snímek tam
  mohl prosvítat.

===============================================================================
 DALŠÍ NALEZENÁ MEZERA - HLÍDAČ VŮBEC NEBĚŽEL PRO SEGU
===============================================================================

  Appka má 300ms hlídač (`plochaHlidac`), co pravidelně kontroluje a
  opravuje přesně tenhle druh problému - je to bezpečnostní síť,
  postavená kvůli podobným chybám u PS1 (B223). Jenže tenhle hlídač
  se STARTOVAL JEN Z PS1 KÓDU (`ps1GlEnable()`) - když hráč hrál JEN
  Segu (aniž by v tý relaci vůbec sáhl na PS1), hlídač se nikdy
  nespustil. Žádná periodická pojistka pro Segu vůbec neexistovala.

===============================================================================
 OPRAVA
===============================================================================

  1. `stopNativeInPlaceHard()` teď `segaPlochu` schová (alpha=0) A
     odpojí z `rootFrame`, stejný vzor, jaký `ps1GlDisable()` má pro
     PS1 od samého začátku.

  2. `plochaHlidacStart()` se teď volá i ze `segaPlochaZapni()` (ne
     jen z PS1 kódu) - hlídač teď běží i během hraní jen Segy.

  3. Rozšířené logování (`stranka=` v hlídači, nová hláška
     `SEGA_PLOCHA_ODPOJENA_PRI_ODCHODU`) - kdyby tohle náhodou
     nestačilo úplně, příští log ukáže přesně, co se dělo.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  `{ }` v MainActivity.java - 2435/2435 (kulaté závorky v souboru
  nesedí kvůli textu v komentářích/řetězcích - to je stav souboru už
  odpradávna, ne důsledek téhle úpravy).

  Všechny čtyři existující jsdom simulace (layout, nebula, savestate,
  knihovna) spuštěny znovu na novém kódu - beze změny prošly. Tahle
  oprava je čistě Java (životní cyklus nativní plochy) a JS strana se
  vůbec nezměnila, takže beze změny je přesně to, co se čekalo.

  CO NEJDE OVĚŘIT ODSUD: jestli tohle skutečně odstraní "kousnutí po
  loadu" a "canvas do všech výběrů" na reálném zařízení - to je na
  tvém testu. Log teď má dost detailů (`SEGA_PLOCHA_JS_POZADAVEK`,
  `SEGA_PLOCHA_ZOBRAZENA/SCHOVANA` s `panelSchovej=` a `stranka=`,
  `SEGA_PLOCHA_ODPOJENA_PRI_ODCHODU`), aby bylo z jednoho poslaného
  logu jasně vidět, jestli se plocha odpojuje správně, nebo jestli je
  problém jinde.
