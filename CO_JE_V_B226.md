# B226 — schování podle vrstvy, indikátor v monitoru, grafika ze skutečného screenshotu (versionCode 272)

Rene: "Na sirku hned, na vysku ne. I hra po nacteni se na vysku
neukaze, az kdyz pretocim... musi byt neco jako ze se hra nahrava
primo v tom okne cd/iso... opravdu chci grafiku jako kdyz se nacte PS1
bez CD, ale prehlednou." Log (ps1.txt) se tentokrát nenačetl - vycházel
jsem ze screenshotu a vlastní analýzy kódu.

===============================================================================
 1) PROČ LANDSCAPE FUNGOVAL A PORTRAIT NE
===============================================================================

  B224 přepnula schovávání plochy na čisté `setAlpha()`. To spravilo
  landscape (plocha POD stránkou, `zOrderOnTop=false`, normální GPU
  vrstva - alpha tam funguje čistě). PORTRAIT ale používá
  `zOrderOnTop=true` (plocha NAD stránkou) - to je zvláštní hardwarová
  překryvná vrstva, kde `setAlpha()` na spoustě zařízení vůbec
  nepůsobí (známý Android limit pro zOrderOnTop SurfaceView). B224 tak
  omylem VYPNULA jediný nástroj (setVisibility), který v portraitu
  doopravdy fungoval - i hra po nabootování proto v portraitu zůstala
  neviditelná, dokud otočení obrazovky nevynutilo úplné znovupostavení
  povrchu (a tím i jeho viditelnost).

===============================================================================
 OPRAVA
===============================================================================

  Nová `plochaAplikujViditelnost(show)` - JEDNO místo, které se podle
  AKTUÁLNÍ vrstvy (`plochaZOrderNahore`) samo rozhodne:
      landscape (pod stránkou) -> setAlpha() - čisté, žádný destroy/create
      portrait (nad stránkou)  -> setVisibility() - jediné spolehlivé
  Všechna tři místa (`ps1PlochaVisible`, `ps1PlochaUmisti`,
  `plochaZkontroluj` hlídač) volají tuhle jednu funkci místo vlastní
  logiky - stejný princip jako u B223 (jedno místo rozhoduje, ne tři
  nezávislé kopie).

===============================================================================
 2) INDIKÁTOR NAHRÁVÁNÍ PŘÍMO V MONITORU
===============================================================================

  Dřív jen bublina (`toast`) - teď navíc `showMonitorLoading()`, malý
  overlay přímo uvnitř `#psMonitor` (přesně tam, kde se hra objeví),
  tikající "NAHRÁVÁM HRU… 10 s" → "9 s" → ... Zůstává viditelný i po
  zavření panelu knihovny, dokud hra doopravdy nenaběhne nebo dokud
  odpočet nedoběhne na nulu.

===============================================================================
 3) GRAFIKA - TENTOKRÁT ZE SKUTEČNÉHO SCREENSHOTU
===============================================================================

  "MAIN MENU / MEMORY CARD / CD PLAYER" obrazovka NENÍ součástí
  statického skinu (ověřeno - v `ps1_final_screen.png` je na tom místě
  jen prázdná tmavá plocha) - je to ŽIVÝ výstup BIOSu. Nedalo se to
  tedy vzorkovat ze skinu jako v B223/B224 - vzorkoval jsem přímo z
  Reného VLASTNÍHO screenshotu té obrazovky:

      výplň tlačítek: rgb(0,1,32) - téměř černá navy
      rám tlačítek: rgb(70,77,183) - sytá modrá (přesně "MAIN MENU" rám)
      záře pozadí panelu: radiální gradient od rgb(72,71,165) do
        rgb(0,1,32) - stejný dojem jako ta modrá koule na pozadí

  Panel, tlačítka i řádky her teď vypadají jako přímé pokračování té
  obrazovky, ne jako cizí vložený prvek.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (76). `stranka_kontrola.py`
  (TV cesta) - 0 chyb. Rovnováha { } v MainActivity.java - 2363/2363.

  Všechny čtyři existující jsdom simulace (plocha, odpočet, CD/ISO
  přehození) spuštěné znovu - beze změny prošly.

  Nová simulace (test_ps1_monitor_loading.js): klik na PLAY vytvoří
  indikátor přímo uvnitř `#psMonitor`, viditelný, s textem "NAHRÁVÁM
  HRU… 10 s", po 1 vteřině "9 s".

  CO NEJDE OVĚŘIT ODSUD: jestli `setVisibility()` v portraitu teď
  doopravdy spolehlivě ukazuje/schovává hru bez nutnosti otáčet - to
  je čistě na tvém telefonu. Pošli prosím `/8765/log` znovu, tentokrát
  se povedlo mi ho nedostat.
