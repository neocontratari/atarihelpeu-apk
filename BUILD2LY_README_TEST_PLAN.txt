
BUILD2LY_VBXE_CORE_REBASE_TILE_UNDERLAY_SAFE

Zaklad: BUILD2LR.
Nepouzito jako zaklad: LT/LU/LV/LW/LX.

Proc:
- LV/LW potvrdily, ze dalsi stripe/transparent odhady nejsou cesta.
- DeepSeek audit spravne ukazal dve podezrele oblasti: SR stripe filtr u Night Driveru a tile-surface underlay u Popeye.
- Finalni implementace ale NENI podle nazvu hry/CRC. Rozhoduje jen realny VBXE/XDL/BCB stav.

Zmeny v jadru:
1) SR low-origin source rebase
   - zadne rezani display radku,
   - zadna retus horizontalnich radku,
   - pokud validni SR record cte z nizke VRAM hlavicky, posune se zdrojova localY mimo $0800.
   Cil: Night Driver pruh pryc bez rezani auta.

2) SR stripe filter OFF pro low-origin SR
   - stare KF/LC heuristiky uz nesmi sezrat realny kus auta/plochy.
   - u ostatnich SR recordu zustava chovani LR.

3) Realny VBXE tile-surface underlay
   - tile candidate z BCB blitu se kresli i pri platnem XDL,
   - jen do COLBK pozadi,
   - neprelepuje ANTIC text/sprity/overlay.
   Cil: Popeye plosiny/schody z realnych BCB tile blitu.

Nemeni se:
- River Raid LL/LR TOP cesta,
- loader/fake-fast LR,
- UI, klavesnice, joystick, kazeta,
- CLOAD/CSAVE,
- test assets: KODY JSOU STEJNE.

LOG markery:
- AtariHelp.eu EMU-10 BUILD2LY_VBXE_CORE_REBASE_TILE_UNDERLAY_SAFE pripraven
- VBXE SR SOURCE REBASE BUILD2LY
- VBXE SR STRIPE FILTER OFF BUILD2LY
- VBXE TILE UNDERLAY BUILD2LY
- VBXE XDL TILE UNDERLAY ACTIVE BUILD2LY

TEST:
1) River Raid: TOP / horsi
2) Night Driver: pruh pryc / stejny / horsi; auto cele / ukrojene
3) Popeye VBXE: intro horni napis; schody/plosiny; hra
4) W3D: jen regrese, protoze uz vypada funkcne: stejny / horsi / lepsi
5) Decathlon: jen regrese; timto buildem Decathlon nehlasim jako opraveny
