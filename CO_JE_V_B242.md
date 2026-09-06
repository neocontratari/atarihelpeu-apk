# B242 — Sega dělá to samé co PS1, sbírka rychlejší, konec zbytečného stahování (versionCode 288)

Rene: "co se tyce nacitani her pres web... trva to dlouho nez se to
otevre... nechapu proc se to furt stahuje po reinstalaci appky...
a furt tam mas chybu v te sega, furt se tam mota ten obraz, proc to
neudelas stejne jak v ps1 - tam to bylo uz osetreny."

===============================================================================
 1) SEGA - KONEČNĚ STEJNÝ MECHANISMUS JAKO PS1
===============================================================================

  Měl pravdu. Čtyři předchozí pokusy pro Segu (B235 základ, B239
  nesouvisející ale důležitá oprava, B240 zpoždění, B241 neprůhledné
  panely) NESTAČILY. To silně naznačuje, že `segaPlocha` se chová
  jako `zOrderOnTop=true` (renderuje NAD celým WebView, vlastní
  hardwarová vrstva) - i když se `setZOrderOnTop()` v kódu nikde
  nevolá. Proti tomuhle žádná průhlednost HTML panelu nemá šanci -
  je to úplně jiná vrstva.

  PS1 přesně tohle už řešil (B226) - pro svůj portrait režim (kde je
  taky "nad stránkou") používá `setVisibility()`, ne `setAlpha()`.
  Sega teď dělá PŘESNĚ TOTÉŽ, na všech třech místech, která o
  viditelnosti rozhodují (přímé volání z JS, 300ms hlídač, i
  okamžik vytvoření nové plochy). Sega nemá žádnou "landscape panel"
  scénu jako PS1 (všechny Sega panely jsou vždy na výšku), takže
  stačí jednoduché "vždy setVisibility" bez toho složitého
  orientation-větvení, co PS1 potřebuje.

===============================================================================
 2) SBÍRKA - RYCHLEJŠÍ, PŘÍMO JAKO PRVNÍ
===============================================================================

  B241 přidala přímé připojení jako ZÁLOHU, až po vyčerpání 3 (teď
  nefunkčních) proxy služeb - to znamenalo zbytečně čekat na 3
  selhání, než se dostane k tomu, co skutečně funguje. Pořadí je teď
  obrácené: přímé připojení JAKO PRVNÍ (rychlý, běžný případ je hned
  rychlý), proxy služby jen jako záloha pro scénář, kvůli kterému
  vůbec vznikly (kdyby WEDOS zase začal blokovat přímý přístup).

===============================================================================
 3) "PROČ SE TO FURT STAHUJE PO REINSTALACI?"
===============================================================================

  Nalezena skutečná příčina - a je to STEJNÁ poučka jako u PS1
  paměťové karty (B225): rozhodování "mám se zeptat na stažení BIOS a
  Sonic?" záviselo částečně na příznaku "už jsem se ptal", uloženém v
  appce-privátním úložišti (SharedPreferences) - které Android PŘI
  ODINSTALACI SMAZE. I když SOUBORY (na veřejném úložišti) pořád
  existovaly, appka se PO KAŽDÉ reinstalaci znovu ptala, protože si
  "nepamatovala", že se už ptala.

  Appka už MĚLA správnou funkci na kontrolu skutečné přítomnosti
  souborů na disku (`neceMChybi`) - jen se výsledek kombinoval s tím
  nespolehlivým příznakem. Teď se appka ptá VÝHRADNĚ podle toho, jestli
  soubory skutečně chybí - žádná závislost na vzpomínce, co se dá
  odinstalací smazat.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check (obě emu stránky - beze změny, tahle oprava byla čistě
  Java), `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha `{ }` v
  MainActivity.java - 2446/2446.

  Všech dvanáct existujících jsdom simulací (osm PS1, čtyři Sega)
  spuštěno znovu - beze změny prošly (JS logika se vůbec nedotkla).

  Aktualizovaná izolovaná simulace kontrolní logiky sbírky
  (`test_provider_relay_logic.js`) - tři scénáře s NOVÝM pořadím
  (přímo první, proxy záloha) potvrzeny.

  CO NEJDE OVĚŘIT ODSUD: jestli `setVisibility()` u Sega plochy
  doopravdy vyřeší prolínání obrazu na reálném zařízení - to je čistě
  na tvém testu. Dávám tomu vysokou důvěru, protože je to DOSLOVNĚ
  stejný mechanismus, který u PS1 v portraitu prokazatelně funguje.
