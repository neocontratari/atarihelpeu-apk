AtariHelp.eu EMU-10 BUILD2JZ_ROLLBACK_TO_JX_SUCCESS_SAFE

Duvod buildu:
- BUILD2JX byl prvni potvrzeny uspech: Commando Arnold ANO + hra ANO.
- BUILD2JY safety guard mel chranit Mission/W3D, ale uz na prvnim testu rozbil Commando: Arnold NE, hra ANO.
- BUILD2JZ proto vraci vykonny index.html na JX logiku a meni jen identifikaci/log/README.

Co je menene:
- JY safety guard je pryc.
- XDL render bez blitteru je zpet jako v BUILD2JX.
- Zadny screen-paint, zadny vlozeny Arnold, zadny hack podle nazvu hry.
- UI, kazeta, joystick, loader beze zmen proti JX.

KODY JSOU STEJNE
- BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

Test plan:
1) Commando jako prvni
   Ocekavani: Arnold ANO, hra ANO.
   Kdyz Arnold neni: hned STOP test a poslat LOG / CHYBA.

2) W3D
   Sleduj rychlost nabehu: rychlejsi / stejna / pomalejsi proti JX/JY.
   Sleduj obraz: lepsi / stejny / horsi.

3) Mission
   Sleduj chyby v obrazu: lepsi / stejna / horsi proti JX.

4) Night Driver
   Auto ano/ne.
   Duse/duchove: mensi / stejne / vetsi.

5) Decathlon
   Zmena ano/ne.

Co poslat zpet:
- Staci kratce: Commando Arnold ANO/NE + hra ANO/NE.
- Potom W3D rychlost, Mission stav, Night Driver auto/duchove, Decathlon zmena.
- LOG poslat hlavne pri NE/regresi, nebo pro Night Driver/Decathlon kdyz se bude rezat dalsi oprava.
