# B208 — levák jedním klepnutím (versionCode 254)

Rene: "pamatujes i na to ze clovek neni jen pravak ale i levak?"

Volné přesouvání (B204-B206) technicky levákům umožňovalo přerovnat si
appku ručně, tlačítko po tlačítku - ale to není řešení, to je jen
obcházení chybějící funkce. Profi ovladač má na tohle jedno tlačítko.

===============================================================================
 CO PŘIBYLO
===============================================================================

  Nové tlačítko "PROHODIT STRANY (LEVÁK)" přímo v Settings, vedle
  "UPRAVIT ROZLOŽENÍ TLAČÍTEK" a "OVLÁDÁNÍ: CITLIVOST A VZHLED" (stejná
  viditelnost - jen landscape).

  Funguje OKAMŽITĚ, bez nutnosti nejdřív jít do editace: jedno klepnutí
  zrcadlově převrátí pozici VŠECH herních tlačítek podle svislé osy
  uprostřed obrazovky - D-pad skončí tam, kde byla akční tlačítka, a
  naopak. Rovnou se to i uloží (žádné čekání na HOTOVO).

  Druhé klepnutí vrátí přesně původní (pravácké) rozložení zpět -
  zrcadlení zrcadla je matematicky identita, ověřeno testem do desetiny
  procenta.

  Po prohození jde ještě doladit obyčejnou editací (přesun jednotlivých
  tlačítek nebo po skupinách, jak už appka umí) - zrcadlo dá hrubý
  levácký layout, jemné doladění zůstává na uživateli.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (56 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.

  Nová jsdom simulace (test_ps1_mirror.js) ověřila:
    - před zrcadlením: D-pad vlevo (~2 %), Triangle vpravo (~76 %)
    - po klepnutí PROHODIT STRANY: D-pad je teď VÍC VPRAVO než Triangle
      (skutečně se prohodily strany, ne jen kosmeticky posunuly)
    - uloženo do localStorage automaticky, bez potřeby HOTOVO
    - druhé klepnutí vrátí obě tlačítka na přesně původní pozice
    - žádná JS chyba v celém průběhu
