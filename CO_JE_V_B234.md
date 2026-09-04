# B234 — vesmír nezakrývá D-pad při editaci (versionCode 280)

Rene: "Partáku, když dám rozložení tlačítek, tak zmizí D-pad - to
nechceme."

===============================================================================
 PŘÍČINA
===============================================================================

  B230 přidala sdílené animované pozadí `#panelNebula` (z-index 79),
  zapínané/vypínané uvnitř `napPlochaHide()`/`napPlochaShow()` - funkcí,
  co appka volá při KAŽDÉM otevření/zavření panelu i při vstupu do
  editace rozložení (ta plochu taky schovává, aby živá hra "neprosvítala"
  přes editaci).

  D-pad, akční tlačítka a editační rámeček/lišta mají z-index 12/70/75
  - VŠECHNY nižší než 79. Když `layoutEditEnter()` zavolala
  `napPlochaHide()` (aby schovala živou hru), navíc se tím **omylem**
  zapnul i vesmír - a ten svým vyšším z-indexem D-pad i akční tlačítka
  přebil. Přesně to je "D-pad zmizí".

  V normálních panelech (knihovna, OVLÁDÁNÍ...) tohle nevadí - tam
  vesmír SMÍ být, nic se pod ním netahá. V editačním režimu ale ANO -
  tam se s D-padem a tlačítky přímo pracuje.

===============================================================================
 OPRAVA
===============================================================================

  `layoutEditEnter()` teď po `napPlochaHide()` hned vesmír zase vypne
  (`document.body.classList.remove('panelNebulaOn')`). Živá plocha
  zůstává schovaná (to je nezávislé, nativní volání přes
  `AHPS1.ps1PlochaVisible`) - jen se přes D-pad a tlačítka už nekreslí
  nic navíc.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb.

  Nová jsdom simulace (test_ps1_layout_nebula.js):
    - D-PAD panel otevřený → vesmír SMÍ být (skutečný panel, nic se
      netahá)
    - UPRAVIT ROZLOŽENÍ TLAČÍTEK → vesmír se vypne (jinak by D-pad
      zmizel), editační lišta i `.layoutEdit` na D-padu fungují dál
    - HOTOVO → vesmír zůstává vypnutý (vrací se živá plocha, ne vesmír)

  Všech šest existujících jsdom simulací spuštěno znovu - beze změny
  prošly.
