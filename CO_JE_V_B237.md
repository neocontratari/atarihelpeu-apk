# B237 — TV cast a animace v panelech se nesmí prát o GPU (versionCode 283)

Rene: "po super settings grafice se zacal sekat prenos - tezko ti
poslu log, protoze ho na tuhle chybu nemas nastaveny."

Klíčová informace byla ČASOVÁ SOUVISLOST - choppy přenos začal AŽ PO
grafickém přepracování nastavení (B230-B236). To mění celé
vyšetřování - předchozí zpráva zkoumala, jestli PS1/Sega jádra
nezůstávají běžet souběžně (nezůstávají, to je v pořádku), ale
nezvažovala GPU zátěž ze samotné grafiky nastavení.

===============================================================================
 HYPOTÉZA (a proč dává smysl)
===============================================================================

  TV cast bere obraz PŘÍMO Z JÁDRA (`Ps1GlTextureView.borrowFrame` /
  `NativeSegaCoreBridge.grabFrameSafe`) - obchází WebView úplně, takže
  by se zdálo, že grafika v HTML panelech nemůže mít na kvalitu
  přenosu žádný vliv.

  ALE: i když TV cast nebere OBSAH z WebView, WebView pořád MUSÍ
  neustále překreslovat cokoli, co je v něm animované - a to je
  SKUTEČNÁ práce pro GPU. Nedávné grafické přepracování přidalo do
  panelů docela dost souběžně běžících animací (u PS1: 10 létajících
  planet, dýchající mlhovina, driftující hvězdy, rotující prstenec,
  RGB rámeček; u Segy: obdobně kopce/mráčky/prsteny/rychlý pruh).
  GPU je sdílený hardware - když WebView kompozituje spoustu
  animovaných vrstev najednou, přímo to soutěží o tu samou GPU s
  H.264 hardwarovým enkodérem, který TV cast potřebuje pro plynulé
  kódování.

===============================================================================
 OPRAVA
===============================================================================

  Nové `AHPS1.tvCastActive()` / `AHSega.tvCastActive()` - čtou už
  existující příznak `napTvWebRunning` (appka ho už měla, jen
  nebyl dostupný pro JS).

  JS v obou stránkách (`emu_ps1`, `emu_sega`) kontroluje tenhle
  příznak jednou za vteřinu a podle toho přepíná třídu `tvCastActive`
  na `<body>`.

  CSS: když je `tvCastActive` nastavená, VŠECHNY animace v panelech
  (mlhovina, hvězdy, planety, prstenec, RGB rámeček u PS1; kopce,
  mráčky, prsteny, rychlý pruh u Segy) dostanou
  `animation-play-state:paused` - obraz zůstane vidět, jen "zamrzne"
  na místě, žádná práce pro GPU navíc. Jakmile TV cast skončí, animace
  se do vteřiny zase rozjedou.

  Filozofie: kvalita PŘENOSU je důležitější než plynulost dekorativního
  pozadí v menu, které navíc člověk na TV stejně nevidí (bere se
  snímek z jádra, ne z panelu).

===============================================================================
 DIAGNOSTIKA - PRO PŘÍŠTĚ, KDYBY TOHLE NESTAČILO
===============================================================================

  Appka už měla docela slušnou diagnostiku pro TV cast výkon
  (`TV_WEB_H264_FRAME_AVG`, `TV_WEB_H264_FRAME_SLOW` - průměrný a
  jednotlivý čas kreslení/vypouštění snímku), jen se NIKDY nezapisoval
  s informací, NA JAKÉ STRÁNCE appky se to stalo. Přidáno `stranka=`
  do obou hlášek - příště, i kdyby tahle oprava nestačila, bude z
  logu jasně vidět, jestli se to kouslo zrovna na emu_ps1/emu_sega
  (kde animace běžely) nebo úplně jinde v appce.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check (obě stránky) - 0 chyb. `stranka_kontrola.py` (TV cesta)
  - 0 chyb. Rovnováha { } v MainActivity.java - 2385/2385.

  Všech deset existujících jsdom simulací (PS1 i Sega) spuštěno znovu
  - beze změny prošly.

  Nová simulace (test_ps1_tvcast_pause.js):
    - na začátku (TV cast neběží) žádná třída, animace běží
    - TV cast se rozběhne → do 1 vteřiny se objeví třída `tvCastActive`
    - animace mají `animation-play-state:paused`
    - TV cast skončí → třída zmizí, animace se vrátí na `running`

  CO NEJDE OVĚŘIT ODSUD: jestli tohle skutečně vyřeší kousavost na
  reálném zařízení - to je čistě na tvém testu. Pokud to nepomůže
  úplně, teď už bude z logu (`stranka=` u FRAME_AVG/FRAME_SLOW)
  jasně vidět, jestli problém souvisí s tímhle, nebo je jinde.
