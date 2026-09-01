# B206 — po skupinách i po jednom, obojí zároveň (versionCode 252)

Reného zadání: "je dobre ze si muzu kazde tlacitko vzit a dat kam chci...
ale pokud chci presunout jen pravy kriz jinam celej tak nemusim po
tlacitku... ja chci presun po skupinach ale i po jednotlivem tlacitku -
a na tobe je aby si to vymyslel jak to udelat."

Individuální přesun (B204/B205) zůstává přesně jak byl - navíc přibyl
přepínač na skupinové tažení, obojí funguje ve stejné editaci vedle sebe.

===============================================================================
 CO PŘIBYLO
===============================================================================

  V editační liště nové tlačítko: PO SKUPINÁCH / PO JEDNOM (přepínač,
  vždy ukazuje, na co se dá přepnout). Vstup do editace vždy začíná
  v režimu PO JEDNOM (výchozí, bezpečnější).

  Skupiny (fyzické/logické shluky na skutečném PS1 padu):
      akce:         Triangle, Circle, Cross, Square
      ramena:       L1, L2, R1, R2
      menu:         Select, Start
      spodní lišta: Reset, Memory Card, CD ISO, Load Game, Save Game

  V režimu PO SKUPINÁCH: chytíš libovolné tlačítko ve skupině, táhneš -
  CELÁ skupina jede se stejným posunem, ostatní skupiny se nehnou.
  V režimu PO JEDNOM (jako dřív): táhneš jen to jedno tlačítko.

  D-pad není v žádné skupině (je jen jeden kus, přesouvá se vždy sám -
  nezávisle na režimu) a je teď i větší (30vh místo 22vh, min 110px,
  max 210px), jak Rene chtěl.

  Ukládání do localStorage se nezměnilo - pořád je to pozice každého
  tlačítka zvlášť. Skupinové tažení jen posune víc tlačítek najednou
  o stejnou vzdálenost, výsledek se ukládá stejně jako dřív.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (44 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.

  Nová jsdom simulace (test_ps1_groups.js) ověřila:
    - PO JEDNOM: tažení Triangle nehne Square/Circle/Cross (regrese z
      B204/B205 - individuální přesun pořád funguje beze změny)
    - přepnutí na PO SKUPINÁCH mění text tlačítka i přidává vizuální
      zvýraznění (třída .active)
    - PO SKUPINÁCH: tažení za Square pohne CELOU akční skupinou (Circle
      i Cross se posunou o STEJNOU vzdálenost), ale D-pad a spodní lišta
      (jiné/žádné skupiny) zůstávají beze změny
    - PO SKUPINÁCH: tažení za L1 pohne L2 (stejná skupina "ramena"), ale
      NE Select (jiná skupina "menu")
    - PO SKUPINÁCH: tažení za Reset pohne Memory Card o stejný posun
      (skupina "spodní lišta")
    - žádná JS chyba v celém průběhu
