# B232 — víc planet, víc barev (versionCode 278)

Rene: "Paráda, už se to blíží - můžeš ještě víc - přidej planety a
barvy, Partáku, super!"

===============================================================================
 CO SE PŘIDALO
===============================================================================

  Pět dalších planetek (10 celkem), každá jiný odstín - schválně žádné
  dvě stejné barvy, ať je scéna pestrá, ne opakovaná:

      f - červeno-oranžová (Mars styl)
      g - azurová/tyrkysová
      h - fialová
      i - ledově bílo-modrá, nejmenší, nejrychlejší let
      j - jantarová PLYNOVÝ OBR se Saturn-stylovým prstencem (protáhlý
          ovál pod úhlem přes planetu - na plochém pozadí funguje
          dobře, i když to není skutečná 3D geometrie)

  Žádné nové animační dráhy - všechny nové planetky recyklují už
  existující `napLetA`/`napLetB`/`napLetC` (jen jinak časované/otočené),
  takže žádný nový druh výpočtu, jen víc kopií toho samého, co appka
  už dělala.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (78). `stranka_kontrola.py`
  (TV cesta) - 0 chyb.

  Všech šest existujících jsdom simulací spuštěno znovu - beze změny
  prošly (čistě CSS/vzhledová změna, žádná logika se nedotkla).
