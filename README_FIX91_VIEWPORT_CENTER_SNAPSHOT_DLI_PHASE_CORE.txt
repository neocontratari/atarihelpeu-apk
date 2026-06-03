FIX91 VIEWPORT CENTER + SNAPSHOT + DLI PHASE CORE

Cil: nejde o jednou hru. Oprava resi spolecne symptomy po 240px mobilnim viewportu:
- viditelne okno 192-208 radku nalepene nahore,
- PMG/playfield posunuti v hornich castech hry,
- horní text/DLI faze u Super Cobra,
- nefunkcni/nespolehlive tlacitko Kompatibilita/Snapshot,
- zachovani Donkey jako stabilni referencni hry.

Zmeny:
1) FIX91 viewportCenterYOffsetForDl()
   Pro obecne hry, Donkey a Cobru se podle realne vysky DLISTu pocita vizualni Y offset do 240 canvasu.
   PMG overlay se posouva stejne jako playfield, aby se nerozjela kolize/obraz.

2) FIX91 Snapshot robust
   compatibilitySnapshot() ma core + try/catch wrapper.
   Snapshot se uklada primo z kliknuti uzivatele pres saveCurrentLogToMobile('snapshot'), ne jen pres pozdni setTimeout z log().
   Pokud snapshot spadne, chyba se zapise do logu a ulozi jako snapshot-error.

3) FIX91 DLI phase pro horní text
   U Super Cobra renderer pouzije blizky DLI stav o nekolik scanline driv pro horní text/char oblast.
   Cilem je vratit horní listu/text, ktery byl cerny/rozpadly kvuli pozdni aplikaci DLI barev.

4) Stabilni reference
   PiTT, Pitstop, River a specialni renderery nejsou centrovanim vynucene.
   Joystick je ponechan jako ve FIX90/FIX84 smeru.

Test poradi:
1. Donkey Kong - hra, horni patro, sud, celkove centrovani.
2. Super Cobra - horni text/lista, scroll, vrtulnik, restart, strely.
3. Death Race - snapshot a jestli obraz neni jen bila rozhozena plocha.
4. Galaxian - kontrolne, aby se neztratil dosavadni pokrok.
