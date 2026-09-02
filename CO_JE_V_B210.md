# B210 — samostatný D-pad panel + oprava z reálného logu (versionCode 256)

Rene poslal skutečný `/8765/log` ze zařízení z testu B209 a napsal: "to
ozubené kolečko musí být jen na klávesnici [D-padu] - oddel to nastavení
tak, aby tam nebyly další věci - čistě jen settings na D-pad."

===============================================================================
 1) NOVÝ SAMOSTATNÝ PANEL "D-PAD A OVLÁDÁNÍ"
===============================================================================

  Předtím: ozubené kolečko (v obou orientacích) vedlo do `helpPanel` -
  jednoho velkého panelu se vším: CD/ISO, Load Game, Memory Card, stav
  jádra, testovací log, PS1 Google Drive, cache, TV Web Cast, a mezi tím
  zahrabané tři tlačítka pro D-pad (rozložení/citlivost/levák).

  Teď: v LANDSCAPE (kde se skutečně hraje) vede ozubené kolečko rovnou na
  nový, zaostřený panel `dpadMenuPanel` - jen čtyři tlačítka:
      UPRAVIT ROZLOŽENÍ TLAČÍTEK
      OVLÁDÁNÍ: CITLIVOST A VZHLED
      PROHODIT D-PAD A AKCE (LEVÁK)
      DALŠÍ (BIOS, CD, LOG...) - jedno tlačítko navíc, kdyby uživatel
      přece jen potřeboval BIOS/CD/log, otevře to původní helpPanel

  PORTRAIT (úvodní/výběrová obrazovka) zůstává beze změny - tam dává
  smysl mít BIOS/CD/log rovnou, není to zbytečné klikání navíc, a stejně
  se tam D-pad nastavení dřív ani nezobrazovalo (bylo portrait-hidden).

  Z `helpPanel` (v portrait) jde taky dostat do D-pad panelu - přibylo
  tlačítko "D-PAD A OVLÁDÁNÍ".

  VLASTNÍ CHYBA, KTEROU JSEM CHYTIL AŽ NA KONTROLE: při psaní HTML
  komentáře jsem ho omylem zavřel `*/` (JS styl) místo `-->` (HTML styl).
  Nezavřený HTML komentář by spolkl VŠECHNO za ním - celý nový panel i
  všechno pod ním by v prohlížeči přestalo existovat. Objevilo se to
  hned při první jsdom simulaci (`$('dpadMenuPanel')` vracelo null),
  opraveno před balením.

===============================================================================
 2) OPRAVA NALEZENÁ V RENEHO SKUTEČNÉM LOGU (ne v mém testu)
===============================================================================

  V logu jsem si všiml řádku:
      LAYOUT_DRAG_END btnL1@0%,65.5663% | btnL2@0%,77.4129% | ...
  L1 a L2 měly PŘED tažením stejné top, po skupinovém tažení k okraji
  obrazovky ale KAŽDÝ jiné - skupina se "rozjela".

  Příčina: `moveDrag()` volal `layoutClamp()` pro KAŽDÉHO člena skupiny
  ZVLÁŠŤ. Když jeden člen (jiná šířka/pozice) narazil na okraj dřív než
  ostatní, ořízl se jen on - relativní mezery mezi tlačítky se rozbily.

  Oprava: `moveDrag()` teď nejdřív spočítá JEDEN společný povolený rozsah
  posunu (průnik limitů přes všechny členy skupiny) a teprve ten aplikuje
  na všechny stejně. Skupina se u okraje jen jako celek zastaví, ale
  nerozpadne se. Pro jednotlivý přesun (1 člen) vychází naprosto stejný
  výsledek jako předtím - žádná regrese.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (60 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.

  Nová jsdom simulace (test_ps1_dpadmenu.js):
    - v landscape klik na ozubené kolečko otevře dpadMenuPanel, ne helpPanel
    - v portrait klik otevře helpPanel, ne dpadMenuPanel (beze změny)
    - DALŠÍ z D-pad panelu správně přepne na helpPanel
    - D-PAD A OVLÁDÁNÍ z helpPanel správně přepne zpátky
    - UPRAVIT ROZLOŽENÍ z nového panelu pořád funguje a panel správně zavře

  Nová jsdom simulace (test_ps1_group_clamp.js), schválně se čtyřmi
  tlačítky RŮZNÝCH šířek a pozic (aby test vůbec mohl odhalit rozjetí):
    - tažení skupiny L1/L2/R1/R2 daleko za pravý okraj
    - VŠECHNY mezery mezi tlačítky zůstaly přesně zachované (8, 35, 43 %)
    - VŠECHNY top hodnoty zůstaly identické (žádné svislé rozjetí)
    - žádné tlačítko nevylezlo mimo plochu
