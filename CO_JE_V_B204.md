# B204 — každé tlačítko zvlášť, ne dva čtverce (versionCode 250)

Reného zásah, ostrý a přesný:
  "nesmyslne velky ctverec na presun ovladace... Kazde tlacitko musi byt
  umozneno presunuti samostatne... zapominas na presun tlacitek L1 L2 R1
  R2... at to udela dva ctverce a jeste spatne budu presouvat cele bloky."
  (L1/L2/R1/R2 byly ve skutečnosti v LAYOUT_IDS od začátku - přesouvatelné
  byly, ale byl to opravdu jen detail vedle hlavního problému: D-pad a
  akční tlačítka jako dva bloky.)
  "to talcitko setings v landscape neni videt."

===============================================================================
 CO BYL PŮVODNÍ PROBLÉM
===============================================================================

  #padLeft (D-pad) byl v landscape JEDNA neviditelná zóna 42 % × 74 %
  plochy - skoro půl obrazovky. Uvnitř byl malý viditelný kroužek
  (30vh, max 220px) s "knoflíkem", který se posouval podle toho, kam
  prst uvnitř zóny sáhl (virtuální analogová páčka s mrtvou zónou pro
  diagonály). #padRight (akce) byl podobně - zóna 35 % × 66 % se 4 malými
  kolečky (Triangle/Circle/Cross/Square) uvnitř, poznávanými podle toho,
  kterému je dotyk nejblíž.

  V editaci to znamenalo přesouvat CELOU tu obří zónu jako jeden kus -
  přesně to "nesmyslně velký čtverec".

===============================================================================
 CO JE TAM TEĎ
===============================================================================

  #padLeft/#padRight jsou pryč. Místo nich 8 samostatných .hit tlačítek:
      dpadUp, dpadDown, dpadLeft, dpadRight
      btnTriangle, btnCircle, btnCross, btnSquare

  Každé je malé (podobná velikost jako L1/L2), viditelné (šipky/PS1
  symboly), a v editačním režimu se táhne NEZÁVISLE na ostatních - žádné
  bloky, žádné dva čtverce.

  Vazba na vstup appky (AHPS1.ps1Input) se NEMĚNILA - jen se ke stejným
  klíčovým slovům ('UP','DOWN','CROSS'...) teď dostane 8 jednoduchých
  bindHold() volání místo dvou zón s výpočtem "nejbližšího směru uvnitř".
  Přesně stejný vzor, jaký už appka používala pro L1/L2/SELECT/START.

  VEDLEJŠÍ VYLEPŠENÍ, o které nikdo nežádal, ale vyplynulo to samo:
  starý model měl na CELOU zónu jen JEDNO sledované dotykové ID - druhý
  prst na tu samou zónu appka ignorovala. Teď každé tlačítko sleduje svůj
  dotyk zvlášť, takže jde stisknout dvě různá tlačítka SOUČASNĚ dvěma
  prsty (např. D-pad + Cross najednou) - ověřeno testem.

  SETTINGS v landscape: minule jsem dal 9% kolečko s opacity .55 - Rene
  ho prakticky neviděl a musel hledat naslepo. Teď plná neprůhlednost,
  viditelný gear symbol, tmavé kontrastní pozadí, o kousek větší
  (11% × 8,5 %).

===============================================================================
 CO SE SMAZALO (obsolentní kód staré zóny)
===============================================================================

  Funkce: installPadDecor(), bindDpad(), bindActions(), nearestAction(),
  controlRect(). CSS: .dpadVis, .dpadKnob, .psBtn, .psTri/.psCircle/
  .psCross/.psSquare - celý ten systém dával smysl jen pro model "jedna
  zóna, hledej nejbližší bod uvnitř", ne pro 8 samostatných tlačítek.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. `stranka_kontrola.py` na TV cestu (nesouvisí,
  kontroluju při každé změně) - 0 chyb. Žádná duplicitní id v souboru
  (45 id atributů zkontrolováno skriptem).

  Nová jsdom simulace (test_ps1_independent_buttons.js) ověřila:
    - přesun JEN dpadUp nehne dpadDown ani dpadLeft (zůstávají na výchozí
      pozici, žádný inline style)
    - přesun JEN btnCross nehne btnTriangle ani btnSquare
    - uložený layout obsahuje přesunutou pozici pro dpadUp a NEZMĚNĚNOU
      (výchozí) pozici pro dpadDown
    - SOUČASNÝ dvouprstý stisk dpadUp + btnCross pošle OBA vstupy
      (["UP",true] i ["CROSS",true]) - u staré jedna-zóny implementace by
      druhý dotyk appka ignorovala
    - žádná JS chyba v celém průběhu

  Viditelnost/kontrast SETTINGS ikony jsem nemohl ověřit automatizovaně
  (jsdom nekreslí skutečné pixely) - jen porovnáním CSS hodnot proti
  předchozí verzi (opacity .55→1, přidané plné pozadí, větší glyph).
  Tohle je věc, kterou musí potvrdit reálný test na telefonu.
