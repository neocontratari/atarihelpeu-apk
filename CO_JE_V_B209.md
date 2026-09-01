# B209 — zrcadlo jen tam, kde má logický smysl (versionCode 255)

Rene se zeptal přesně na správnou věc: "pro leváky opravdu stačí jen
svislá osa?" Odpověď: ne úplně - B208 zrcadlilo úplně všechno, a to bylo
logicky špatně u dvou skupin tlačítek.

===============================================================================
 ROZBOR - PROČ SE MĚNÍ JEN D-PAD + AKCE
===============================================================================

  D-pad vs. akční tlačítka (Triangle/Circle/Cross/Square)
      Tohle JE skutečná volba stranou - kterým palcem chceš pohyb a
      kterým akce. Zrcadlí se.

  L1/L2/R1/R2
      Jsou pojmenované podle HRANY telefonu, na které prst leží (levá
      hrana = L, pravá hrana = R), když appku držíš na šířku oběma
      rukama. To se s levactvím/pravactvím nemění - levá ruka je pořád
      na levé hraně telefonu, ať appku ovládá kdokoli. Prohodit jejich
      pozici by bylo VĚCNĚ ŠPATNĚ, ne jen jiná volba designu - hráč by
      pak sahal levou rukou přes celou obrazovku na tlačítko jménem
      "R1". PŘESTÁVAJÍ SE ZRCADLIT.

  SELECT/START a spodní menu lišta (Reset/Memory/CD/Load/Save)
      Středová/administrativní tlačítka, na která se sahá jen občas, ne
      reflexivně během hraní - s tím, kterou rukou hraje, nesouvisí.
      ZŮSTÁVAJÍ beze změny.

===============================================================================
 CO SE ZMĚNILO V KÓDU
===============================================================================

  `layoutMirrorHorizontal()` v emu_ps1/index.html teď prochází jen novou
  konstantu `MIRROR_IDS = ['dpad','btnTriangle','btnCircle','btnCross',
  'btnSquare']` místo celého `LAYOUT_IDS`. Zbytek funkce (uložení, log,
  toast) beze změny.

  Text tlačítka upřesněn: "PROHODIT D-PAD A AKCE (LEVÁK)" místo
  obecného "PROHODIT STRANY", ať je z popisku hned jasné, co přesně to
  udělá.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. `stranka_kontrola.py` na TV cestu - 0 chyb.

  Rozšířená jsdom simulace (test_ps1_mirror.js):
    - D-pad a Triangle si prohodí strany přesně jako v B208
    - NOVĚ ověřeno: L1, Select i Reset zůstávají na přesně stejné pozici
      (23,0 / 40,0 / 3,0 %) - žádný z nich se zrcadlením nehne
    - druhé klepnutí vrátí D-pad i Triangle na původní pozice
    - žádná JS chyba
