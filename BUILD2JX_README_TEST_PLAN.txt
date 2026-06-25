AtariHelp.eu EMU-10 BUILD2JX_XDL_RENDER_WITHOUT_BLITTER_SAFE

NAVAZUJE NA:
- BUILD2JW_XDL_TILE_UNDERLAY_AFTER_ZERO_GRAPH_SAFE
- posledni potvrzeny stav: Commando Arnold NE, Night Driver auto NE, Decathlon NE

PROC TENTO BUILD:
Log z Commando v BUILD2JW ukazal:
- XDL se opravdu zapne: base=$20000
- ale neni zadny VBXE BLITTER start v logu
- stary M.renderVbxeProbe mel tvrdou podminku bltStarts<1 return
=> XDL renderer se vubec nemusel spustit, pokud obraz do VBXE VRAM prisel pres MEMAC/CPU a ne pres BCB blitter.

CO JE ZMENENE:
- XDL renderer se zkusi spustit pred podminkou bltStarts>0.
- Kdyz je XDL enabled/sawValid a bltStarts=0, log vypise:
  VBXE XDL RENDER WITHOUT BLITTER BUILD2JX
- Nejde o hack podle hry.
- Nepridava se zadny Arnold obrazek.
- Neni screen-paint.
- Jen se odblokuje obecne VBXE XDL kresleni z realne VRAM/MEMAC dat.

CO NENI MENENE:
- UI / kazeta / joystick / loader: beze zmeny
- BASIC / Turbo BASIC / Altirra porovnavaci kody: KODY JSOU STEJNE
- Decathlon PM priority zatim bez nasilneho hacku

TEST PLAN:
1) Nahraj overlay pres GitHub Desktop.
2) Spust appku a over v logu:
   AtariHelp.eu EMU-10 BUILD2JX_XDL_RENDER_WITHOUT_BLITTER_SAFE pripraven
3) Commando:
   - Ocekavani: pokus o Arnold obraz pred / kolem PRESS ANY KEY TO START.
   - Posli: Arnold ANO/NE + LOG / CHYBA.
   - V logu hledat:
     VBXE XDL RENDER WITHOUT BLITTER BUILD2JX
     pripadne VBXE SR/HR XDL RENDER / TEXT XDL / XDL BLOCKED
4) Night Driver:
   - Ocekavani: overit, jestli se zlepsi VBXE obraz / auto.
   - Posli: auto ANO/NE + LOG.
5) Decathlon:
   - Ocekavani: nejspis beze zmeny, ale hlidat regresi.
   - Posli: zmena ANO/NE + LOG.
6) Mission / Tetris VBXE / HL:
   - jen rychla regresni kontrola, pokud bude cas.

KDYZ TO SELZE:
- Staci poslat jeden TXT log z kazde hry pres LOG / CHYBA.
- Rene nemusi v logu nic hledat rucne.
