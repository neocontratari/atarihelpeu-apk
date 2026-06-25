AtariHelp.eu EMU-10 BUILD2JY_JX_SUCCESS_STABILITY_GUARD

NAVAZUJE NA:
- BUILD2JX_XDL_RENDER_WITHOUT_BLITTER_SAFE
- prvni potvrzeny uspech: Commando Arnold ANO + hra ANO
- zpetna vazba z testu: Night Driver ma viditelne zlepseni/VBXE spodek, ale auto porad ne a jsou male duchove; Decathlon NE; W3D nabehne, ale trva dele; Mission ma nove drobne chyby.

PROC TENTO BUILD:
JX je dulezity, protoze poprve ukazal Commando Arnolda bez vlozeneho obrazku a bez screen-paintu.
Soucasne ale nechceme, aby nova no-blitter XDL cesta sahala na slabou/non-VBXE hru nebo na zbytky stareho XDL/VRAM stavu.

CO JE ZMENENE:
- Commando oprava z JX zustava zachovana.
- Pridana bezpecnostni brana BUILD2JY:
  - no-blitter XDL render se smi pouzit jen pokud aktualni program je potvrzeny VBXE scanem/loaderem,
  - nebo pokud existuje realny BCB/surface kandidat.
- Pro slabe/non-VBXE hry se zapise log:
  VBXE XDL NO-BLITTER BLOCK BUILD2JY
  a emulator nebude kreslit stary VRAM/XDL obraz pres normalni Atari hru.
- Pridan log aktualniho programu:
  VBXE CURRENT PROGRAM BUILD2JY

CO NENI MENENE:
- Zadny Arnold obrazek vlozeny natvrdo.
- Zadny screen-paint.
- Zadny hack podle nazvu hry.
- UI / kazeta / joystick / loader beze zmeny.
- Night Driver/Decathlon priority zatim bez nasilne zmeny.
- BASIC / Turbo BASIC / Altirra porovnavaci kody: KODY JSOU STEJNE.

TEST PLAN:
1) Nahraj overlay pres GitHub Desktop.
2) Spust appku a over v logu:
   AtariHelp.eu EMU-10 BUILD2JY_JX_SUCCESS_STABILITY_GUARD pripraven

3) Commando:
   - Ocekavani: Arnold ANO, hra ANO jako v JX.
   - Posli: Arnold ANO/NE + LOG / CHYBA.

4) Mission:
   - Ocekavani: pokud chyby z JX byly zpusobene prilis sirokym XDL no-blitter renderem, mely by zmizet nebo byt mensi.
   - Posli: Mission zlepseni/stejne/horsi + LOG.

5) W3D:
   - Ocekavani: musi stale nabehnout; hlidat, jestli neni pomalejsi/horsi nez JX.
   - Posli: W3D nabehne ANO/NE + LOG, pokud bude viditelna regrese.

6) Night Driver:
   - Ocekavani: spodni VBXE obraz z JX zachovat, auto porad muze chybet; hlidat duchy.
   - Posli: auto ANO/NE, duchove stejne/mensi/vetsi + LOG.

7) Decathlon:
   - Ocekavani: pravdepodobne zatim beze zmeny; hlidat regresi.
   - Posli: zmena ANO/NE + LOG.

KDYZ TO SELZE:
- Rene nemusi v logu nic hledat rucne.
- Staci ulozit LOG / CHYBA po testu konkretni hry a poslat TXT.
- Nejcennejsi ted bude LOG z Mission a W3D, protoze tam jde o stabilitu po uspechu Commando.
