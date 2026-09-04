# B230 — schválená grafika je v appce (versionCode 276)

Rene navrhy schválil ("Koncept C") a v dalších kolech je nechal
rozšířit o plné RGB barvy a živé (animované) pozadí: "souhlas - tak
Partáku teď do appky." Tenhle build je ta implementace.

Návrhy samotné vznikly mimo appku (tři samostatné HTML soubory k
prohlédnutí, ne kód appky) - tady je popis, jak se to skutečně dostalo
do `emu_ps1/index.html`.

===============================================================================
 SDÍLENÉ ANIMOVANÉ POZADÍ - JEDNO MÍSTO, NE KOPIE V KAŽDÉM PANELU
===============================================================================

  Appka má 6 panelů (helpPanel, dpadMenuPanel, libraryPanel, ctrlPanel,
  memCardStatusPanel, libraryDeleteConfirm). Kdybych hvězdné pozadí
  (mlhovina, hvězdy, planetky, prstenec) vložil do KAŽDÉHO zvlášť, je
  to 6x duplikovaný kód a 6x náklad na vykreslení.

  Místo toho jeden sdílený `#panelNebula` (fixed, pod všemi panely),
  co se zobrazí/schová podle třídy `panelNebulaOn` na `<body>`. A tahle
  třída se přepíná přímo uvnitř UŽ EXISTUJÍCÍCH funkcí
  `napPlochaHide()`/`napPlochaShow()` (ty, co řídí schovávání
  nativní plochy při otevření panelu, viz B221-B226) - žádný nový
  spouštěcí bod, jen recyklace toho, co už se volá při každém
  otevření/zavření panelu.

  Co se hýbe:
    - 4 vrstvy mlhoviny (růžová/zelená/jantarová/fialová), každá
      pomalu "dýchá" jinou rychlostí (8-13s) přes transform+opacity
    - 2 vrstvy hvězd driftují opačným tempem (paralaxa)
    - 3 planetky se jemně vznáší nahoru/dolů
    - jemně rotující energetický prstenec za nadpisem (40s na kolo)

  Všechno přes `transform`/`opacity` (GPU vrstva, žádný reflow) -
  vybráno schválně kvůli výkonu na telefonu, ne jen z návrhu.

===============================================================================
 SKLENĚNÉ PANELY MÍSTO PLNÉ BARVY
===============================================================================

  `.panel .box`, tlačítka i řádky knihovny mají teď průhledné pozadí
  se světelným modro-fialovým okrajem - mlhovina za nimi prosvítá,
  místo aby ji plná barva panelu zakrývala (jak to bylo v B227-B229).

===============================================================================
 RGB RÁMEČEK - JEN NA JEDNOM MÍSTĚ
===============================================================================

  Hlavní seznam her v knihovně má jemně animovaný duhový rámeček
  (pomalu mění barvu). Schválně JEN tam, ne na všech tlačítkách -
  jinak by to blikalo jako vánoční stromek a přestalo to působit jako
  promyšlený detail.

===============================================================================
 BAREVNÉ ZNAČKY HER - PODLE SKUTEČNÝCH BAREV PS1 TLAČÍTEK
===============================================================================

  Každá hra v seznamu má barevný proužek vlevo - střídá se po čtyřech
  přesně podle barev PS1 ovladače (zelený trojúhelník, červené
  kolečko, modrý křížek, fialový čtverec). Není to náhodná duha - má
  to rozpoznatelný, smysluplný zdroj.

===============================================================================
 VĚDOMÉ ROZHODNUTÍ: ŽÁDNÝ FONT Z INTERNETU
===============================================================================

  Návrhy používaly Google Fonts (Orbitron, Rajdhani) pro ten
  "sci-fi/technický" dojem. V appce jsem to ZÁMĚRNĚ nepoužil -
  appka dosud nikde nezávisí na síťovém fontu jen kvůli vzhledu, a
  přidávat novou "appka bez internetu vypadá jinak" závislost jen
  pro kosmetiku se mi nezdálo úměrné riziku (WebView na `file://`
  stránce může externí `@import` v některých konfiguracích i
  blokovat). Tučné Arial (už zavedený vzor v týhle appce pro D-pad
  glyfy) dává skoro stejně "chunky" dojem bez rizika. Pokud bude Rene
  chtít i tohle, dá se doplnit s fallbackem.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (78). `stranka_kontrola.py`
  (TV cesta) - 0 chyb.

  Všech šest existujících jsdom simulací (plocha, odpočet, CD/ISO
  přehození, indikátor v monitoru, RESET, dvojitý stisk RESET) spuštěno
  znovu po celém redesignu - beze změny prošly, žádná funkční regrese.

  Nová simulace (test_ps1_nebula.js):
    - `#panelNebula` existuje v DOM, na začátku schovaný
    - otevření panelu zapne `panelNebulaOn` na `<body>`, zavření vypne
    - barevné značky u 5 her se správně střídají n0,n1,n2,n3,n0
    - `#libraryList` má třídu `rgbRam`, obsah je zabalený v `.rgbObsah`

  CO NEJDE OVĚŘIT ODSUD: skutečný vizuální dojem a plynulost animací
  na reálném telefonu - to je na tvém testu. Animace jsou navržené
  přes GPU-přívětivé vlastnosti (transform/opacity), takže by neměly
  znatelně zatížit výkon, ale jistotu dá jen test na zařízení.
