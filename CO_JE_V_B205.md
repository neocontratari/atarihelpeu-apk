# B205 — D-pad je zpátky jeden kus, oprava podle screenshotu (versionCode 251)

Rene poslal screenshot živé editace v landscape. Z něj tři konkrétní věci:

1. D-pad rozdělený na 4 tlačítka je špatně - "musí být základní ovladač
   kruhový... ten kruhový ovladač si musím posunout samostatně." Skutečný
   PS1 pad má D-pad jako JEDEN fyzický kus. Akční tlačítka (Triangle/
   Circle/Cross/Square) čtyři samostatná zůstávají - to bylo správně.
2. L1/L2/R1/R2/SELECT/START nebyly v editaci vidět - zakrývala je
   editační lišta dole.
3. SETTINGS ikona: na screenshotu doslova přetéká text "SETTING..." mimo
   svůj rámeček - "musí být taky průhledný a bez nápisu, malé kolečko
   ozubené bude stačit."

===============================================================================
 CO SE ZMĚNILO
===============================================================================

  1) D-pad: 4 tlačítka (dpadUp/Down/Left/Right) SMAZÁNA, nahrazena JEDNÍM
     `#dpad`. Uvnitř funguje stejná logika jako v B197-B203 (poloha dotyku
     uvnitř kruhu určuje UP/DOWN/LEFT/RIGHT, včetně diagonál) - jen bez
     animovaného knoflíku, který tam byl navíc jen jako ozdoba. V editaci
     se `#dpad` táhne jako jeden kus, přesně jak chtěl.
     Akční tlačítka (btnTriangle/btnCircle/btnCross/btnSquare) beze změny
     - 4 samostatná, jak už bylo v B204.

  2) `.layoutEditBar` (spodní lišta "Přetáhni tlačítko...") se v landscape
     přesunula nahoru (`bottom:auto;top:0`) a zmenšila - dole, kde žije
     L1-R2/SELECT/START (úzký pruh bottom:4%-11,6%), teď žádná lišta není.

  3) `#btnSettings`: textový obsah tlačítka změněn z doslovného "SETTINGS"
     na znak ozubeného kola (&#9881;). To byla ta chyba - dal jsem
     tlačítku viditelné písmo (26px), ale nikdy jsem nezměnil, ŽE to
     tlačítko pořád říká "SETTINGS" jako text - proto to přetékalo.
     Navíc menší (9%×7% místo 11%×8,5%) a průhlednější (poloprůhledné
     pozadí místo plného).

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (42 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb (nesouvisí, kontroluju
  při každé změně pro jistotu).

  Nová jsdom simulace (test_ps1_dpad_single.js):
    - #dpad existuje jako jeden prvek, #dpadUp/Down/Left/Right už neexistují
    - dotyk v pravém horním kvadrantu kruhu pošle SOUČASNĚ RIGHT:true
      i UP:true (diagonála funguje stejně jako v původní verzi)
    - puštění správně pošle oba na false
    - přesun #dpad v editaci nehne btnCross (odděleny zůstávají)
    - CSS pravidlo přesouvající editační lištu nahoru v landscape je
      v souboru přítomné

  Vizuální věci (jak to doopravdy VYPADÁ - velikost kruhu, kontrast
  ikony) jsem znovu nemohl ověřit automatizovaně - jsdom nekreslí
  pixely. Prosím o další screenshot nebo test na telefonu.
