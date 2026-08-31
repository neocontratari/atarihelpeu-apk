# B203 — editace rozložení tam, kde je skutečný D-pad (versionCode 249)

Reného oprava: skutečný, hraný D-pad je v landscape (appka otočená na
šířku - tak se PS1 fakticky hraje). Portrait je jen úvodní/výběrová
obrazovka. Editace rozložení v portrait tedy neřešila to hlavní.

===============================================================================
 CO SE ZMĚNILO (vše v emu_ps1/index.html)
===============================================================================

  1. "UPRAVIT ROZLOŽENÍ TLAČÍTEK" v portrait Settings panelu ZMIZELO
     (`#btnEditLayout{display:none}` ve výchozím - portrait - CSS).

  2. V landscape se místo toho SETTINGS tlačítko (dřív úplně schované)
     teď zobrazuje jako malé, průsvitné kolečko vpravo nahoře
     (`opacity:.55`, `right:2%;top:2%;width:9%;height:6%`), aby nerušilo
     výhled na hru, ale šlo se k němu kdykoli dostat. Přes něj je
     "UPRAVIT ROZLOŽENÍ TLAČÍTEK" v landscape teď VIDĚT
     (`#btnEditLayout{display:block}` uvnitř landscape media query).

  3. Editační logika (tažení, uložení, reset, /8765/log) se neměnila -
     byla už od B201 napsaná obecně přes všech 15 prvků a ukládá zvlášť
     pro portrait/landscape, takže landscape teď prostě začal fungovat,
     jakmile byl k němu vůbec přístup.

  4. OPRAVENO PŘI TÉ PŘÍLEŽITOSTI: `layoutOrientation()` v JS se ptal jen
     na `(orientation: landscape)`, ale skutečná CSS podmínka pro landscape
     layout appky je `(orientation:landscape) and (max-height:760px)`
     (kvůli tabletům - velká obrazovka na šířku má pořád portrait CSS).
     Bez opravy by na větší obrazovce appka CSS ukázala portrait, ale JS
     by si myslel, že je landscape, a četl/ukládal by pod špatný klíč.
     Teď je podmínka v JS doslova identická s CSS (ověřeno porovnáním
     řetězců, ne jen okem).

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. `stranka_kontrola.py` na MainActivity.java (TV
  cesta, nesouvisí, ale kontroluju to při každé zmene pro jistotu) - 0 chyb.

  Statická kontrola shody: řetězec `(orientation:landscape) and
  (max-height:760px)` se v souboru objevuje na obou místech (CSS media
  query i JS `layoutOrientation()`) doslova stejně.
