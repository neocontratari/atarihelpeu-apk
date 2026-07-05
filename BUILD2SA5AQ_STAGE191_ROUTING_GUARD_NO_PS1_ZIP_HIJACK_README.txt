AtariHelp EMU10 BUILD2SA5AQ / Stage191

Hlavni oprava:
- PS1 remote loader uz nesmi sezrat kazdy .zip z WebView.
- .zip je spolecna pripona pro Atari 130XE, Segu i PS1, proto se PS1 pouzije jen v PS1 kontextu:
  - PS1 obrazovka
  - PS1 Google Drive stranka page_id=1048
  - explicitni PS1 remote URL akce

Proc:
- po PS1 testu zustal webovy router moc hladovy
- Sonic/Donkey Kong z webu se tvarily jako "nejaky ZIP" a route je poslala do PS1
- po restartu aplikace to vypadalo, jako by se porad chtelo stahovat z Google Drive

Co zustava:
- Sega ZIP/GEN/MD/SMD/SMS/68K/SGD ma prednost pred PS1
- Atari XEX/ATR/COM/EXE/ZIP zustava v 130XE loaderu
- PS1 Google Drive download a cache z Stage190 zustavaji zachovane
- PS1 cache je primarne v Downloads/AtariHelp/PS1/
- PS1 banner se po PS1_REMOTE_BOOT_OK hned schova

Test:
1. Otevri Segu a z webu spust Sonic ZIP/GEN -> musi zustat v Sega emu.
2. Otevri Atari NET HRY a spust Donkey Kong / Galactic Chase ZIP -> musi zustat ve 130XE.
3. Otevri PS1 LOAD GAME / page_id=1048 a spust Drive odkaz -> musi jit do PS1.

Bezpecnost:
- zadny BIOS/bin/rom/game payload neni soucast overlaye
