# B223 — skutečná příčina konečně nalezena, grafika bez výmyslu (versionCode 269)

Rene: "tam mas chyby v prepinani na mobilu vyska sirka - abych to
probudil na mobilu musim bud minimalizovat nebo pretocit display vyska
sirka... projdi si opravdu dukladne kod ps1." Plus tvrdé, oprávněné
odmítnutí grafiky z B222: "vem grafiku z ps1, ne tvuj vymysl AI."

===============================================================================
 SKUTEČNÁ PŘÍČINA - NE DALŠÍ HÁDANKA
===============================================================================

  B221 a B222 opravovaly správnou VĚC (ps1Plocha nad WebView), ale
  přehlédly, že appka má JEŠTĚ JEDEN, úplně samostatný mechanismus,
  který o mém novém příznaku vůbec nevěděl:

  `plochaHlidac` - `java.util.Timer` běžící `plochaZkontroluj()` **KAŽDÝCH
  300 ms**, po celou dobu běhu appky (ne jen občas). Tahle funkce dělá
  jednoduché rozhodnutí: "jsme na stránce emu_ps1? Pak plocha MUSÍ být
  VIDITELNÁ" - a to zcela BEZ OHLEDU na to, jestli je otevřený nějaký
  HTML panel. Otevření panelu totiž stránku neopouští (žádná navigace,
  jen DOM navrch) - takže `jePs1` zůstává `true` po celou dobu, co je
  panel otevřený.

  Výsledek: i když moje B221/B222 oprava JS správně řekla "schovej", tenhle
  hlídač to **do 300 ms zase vrátil zpět** - úplně nezávisle, žádná
  souvislost s tím, co si přál panel. Proto to nebylo "skoro
  stoprocentní" (B221) ani "pořád ne stoprocentní i s alpha" (B222) -
  bojoval jsem s příznakem u jedné cesty (`ps1PlochaUmisti`), zatímco
  druhá, ČASTĚJŠÍ cesta (tenhle hlídač) o boji vůbec nevěděla.

===============================================================================
 OPRAVA
===============================================================================

  `plochaZkontroluj()` teď počítá viditelnost takhle:

      chci = (jsme na PS1 stránce) A ZÁROVEŇ (panel si NEPŘEJE schovat)

  Jeden společný příznak (`plochaSchovanaKvuliPanelu`, nastavovaný z JS
  přes `ps1PlochaVisible()`) teď respektují VŠECHNA tři místa, která o
  viditelnosti rozhodují: přímé volání z JS, `ps1PlochaUmisti()`
  (hlášení pozice) i tenhle 300ms hlídač. Přidán i `setAlpha()` do
  hlídače pro shodu s ostatními místy.

  Přidáno logování `BUILD2SB24 PLOCHA_JS_POZADAVEK show=...` při každém
  volání z JS a rozšířené `appendNativeLog` v hlídači (obsahuje teď i
  stav `panelSchovej=`), ať je v `/8765/log` příště vidět přesně, který
  mechanismus co udělal a kdy.

===============================================================================
 GRAFIKA - ZPĚT KE SKUTEČNÝM BARVÁM ZE SKINU, ŽÁDNÝ VÝMYSL
===============================================================================

  B222 zavedla vlastní gradienty/záře ("moje umělecké dílo"), což bylo
  přesně to, co Rene odmítl. Napravdu jsem si teď VZOREK barev
  ze skutečného `ps1_final_screen.png` (ne odhad):

      pozadí: #03060c (tmavá, skoro černá - vzorkováno ze středu skinu)
      text/akcent: #d9a84e (skutečný odstín "RESET"/"MEMORY CARD" popisků)
      okraje: tenký 1px kovový/šedý (#6b7686, #4a5361) - žádný "zářící"
        gradient rám

  Panely, karty her i tlačítka jsou teď PLOCHÉ - žádné radial-gradient
  glow, žádné letter-spacing efekty, žádné vymyšlené "dvojité rámy".

===============================================================================
 PŘESNÝ POPIS TESTU (jak Rene žádal - "nachystej si presny popis testu")
===============================================================================

  1. Otevři PS1 v portrait (na výšku), počkej až se něco vykreslí
     (BIOS/hra).
  2. Otevři libovolný panel - CD/ISO, MEMORY CARD, ozubené kolečko.
  3. Sleduj, jestli se PŘES panel po CELOU dobu, co je otevřený, NIC
     nezobrazuje (ne jen na chvíli).
  4. Zavři panel, zkus hru/BIOS ovládat - reaguje HNED, bez nutnosti
     minimalizovat nebo otočit?
  5. Pokud NE - pošli mi `/8765/log` z tohohle konkrétního testu. Hledej
     v něm řádky `PLOCHA_JS_POZADAVEK`, `PLOCHA_ZOBRAZENA`,
     `PLOCHA_SCHOVANA` (s `panelSchovej=`) - podle jejich pořadí a
     času přesně uvidím, který mechanismus vyhrál a kdy.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (76). `stranka_kontrola.py`
  (TV cesta) - 0 chyb. Rovnováha { } v MainActivity.java - 2356/2356.
  Žádné duplicitní CSS selektory po předělání grafiky (zkontrolováno
  ručně - `.panel .box`, `.libGame .libMeta` atd. každé přesně jednou).

  Všechny tři existující jsdom simulace (plocha, odpočet, CD/ISO
  přehození) spuštěné znovu proti tomuhle souboru - beze změny prošly.

  CO NEJDE OTESTOVAT ODSUD: samotný `plochaHlidac` běží jen v reálné
  appce (Java Timer, ne JS) - jeho opravu nejde ověřit jinak než na
  tvém telefonu. Proto ten přesný popis testu výše.
