AtariHelp.eu EMU-10 BUILD2JW_XDL_TILE_UNDERLAY_AFTER_ZERO_GRAPH_SAFE
===================================================================

STAV / NAVAZANI
- Navazuje na BUILD2JV / BUILD2JU / BUILD2JT / BUILD2JS.
- Posledni test uz potvrdil: Commando stale nema Arnolda, ale log porad ukazuje realny VBXE blitter:
  VBXE BLITTER FAST DONE blocks=862 bytes=66864 copied=66864 renderBase=$24000.
- Night Driver: asi drobne zlepseni grafiky, ale auto stale neni.
- Decathlon: bez viditelne zmeny.

CO JE ZMENENE
- BUILD2JW opravuje jednu obecnou blokaci ve VBXE rendereru:
  kdyz existuje XDL, ale graficky SR/LR/HR record nevykresli zadny pixel, nesmi to automaticky zablokovat
  realny tile-surface poskladany BCB blitterem ve VBXE VRAM.
- Proto JW povoli realny BCB tile-surface underlay i po zero-graph XDL, ale pouze do COLBK pozadi.
- Text a bezne ANTIC/VBXE pixely zustavaji nahore.
- Neni to screen-paint, neni to vlozeny Arnold, neni to podle nazvu hry.

CO NENI ZMENENE
- Night Driver / Decathlon PM priority zatim nemenim naslepo.
- UI, kazeta, joystick, loader, ROM, BASIC, Turbo BASIC zustavaji beze zmen.
- BUILD2JT PM audit zustava v logu.

KODY JSOU STEJNE
- BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

TEST PLAN
1) Nahraj overlay pres GitHub Desktop.
2) Spust aplikaci.
3) V LOGu musi byt:
   AtariHelp.eu EMU-10 BUILD2JW_XDL_TILE_UNDERLAY_AFTER_ZERO_GRAPH_SAFE pripraven
4) Spust Commando.
   Ocekavany vysledek: v intru se ma aspon pokusit objevit Arnold / VBXE obraz za textem.
   Kdyz ne, posli LOG / CHYBA.
   V logu hledej jen informacne, nemusis rucne:
   VBXE XDL TILE UNDERLAY BUILD2JW
   nebo
   VBXE TILE UNDERLAY BUILD2JU
5) Potom test Night Driver.
   Ocekavany vysledek: kontrola, jestli se nezhoršil obraz a jestli se neobjevilo auto.
   Kdyz auto porad neni, posli LOG.
6) Potom test Decathlon.
   Ocekavany vysledek: kontrola regrese / zmeny spritu.
   Kdyz zadna zmena, posli LOG.
7) Volitelne Mission / Tetris VBXE / Heartlight jen jako regresni kontrola.

CO POSLAT ZPET
- Kratce: Commando Arnold ano/ne, Night Driver auto ano/ne, Decathlon zmena ano/ne.
- K tomu jeden LOG / CHYBA po kazde hre, idealne zvlast.
