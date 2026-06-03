FIX90 MULTI-GAME COLLISION + VIEWPORT CORE

Cil: nejit jen po jedne hre, ale opravit obecne veci, ktere se opakuji u Donkey/Cobra/Death Race/Galaxian a dalsich XEX.

Zmeny:
- PMG player-player self-collision guard: jeden player uz nesmi vyrobit kolizi sam se sebou.
  To miri na Donkey horni patro / sud / stiny a obecne na hry cteci $D00C-$D00F.
- Player-player a missile-player kolize respektuji 240r viewport collision window.
  Horni/spodni overscan/stiny nemaji zabijet nebo zastavovat objekty.
- Generic DLIST auto promotion: pro obecne XEXy se muze povysit naskenovany runtime DLIST,
  pokud je vyrazne lepsi nez aktualni SDLST/HW pointer. Chranene reference Cobra/Donkey/PiTT/KiTT/Pitstop/River/Galaxian se tim neprepisuji.
- Snapshot log pridal VIEWPORT FIX90 radek: DLIST, visible/blank scanlines, collision window, PMG y offset.
- Timing/joystick zustava ze stabilni FIX89/FIX84 vetve.

Test poradi:
1) Donkey - horni patro, skok, sud.
2) Cobra - scroll, vrtulnik, horni lista, restart, kulomet.
3) Death Race - intro/menu i vstup do hry.
4) Galaxian - kontrolne, jestli se nezhoršil.
