# B233 — popředí v barvách PS1 (versionCode 279)

Rene: "TO JE ONO!!! Tak pozadí máme a teď si pohraj s tím popředím -
u písma měň barvu, obdélníky taky můžou hrát barvama PS1, neboj se
toho."

===============================================================================
 NADPISY PANELŮ - KAŽDÝ JINÁ SKUTEČNÁ BARVA PS1 TLAČÍTKA
===============================================================================

  Dřív všechny nadpisy stejná bílo-fialová záře. Teď každý panel má
  nadpis v jedné ze 4 skutečných barev PS1 ovladače - na první pohled
  poznáš, kde jsi:

      D-PAD A OVLÁDÁNÍ / OVLÁDÁNÍ — zelená (trojúhelník)
      VYBRAT HRU (CD/ISO) — modrá (křížek)
      SMAZAT HRU — červená (kolečko) - sedí, je to varovná obrazovka
      PAMĚŤOVÁ KARTA — fialová (čtverec)

===============================================================================
 TLAČÍTKA PODLE ROLE, NE UŽ VŠECHNO STEJNĚ
===============================================================================

      potvrdit/pokračovat (výchozí tlačítko) — zelená
      zpět/zavřít (nová třída .zavrit, přidána na všech 5 tlačítek
        ZAVŘÍT/ZAVRIT v appce) — modrá
      vedlejší (.secondary) — fialová
      nebezpečné (mazání) — červená (už existovalo dřív, beze změny)

===============================================================================
 POPISKY V OVLÁDÁNÍ - KAŽDÉ NASTAVENÍ JINÁ BARVA
===============================================================================

  Citlivost D-padu / Průhlednost tlačítek / Velikost tlačítek /
  Velikost D-padu — každý název tučně v jiné barvě z palety, místo
  jednotné azurové pro všechny čtyři.

===============================================================================
 VLASTNÍ CHYBA, KTEROU JSEM CHYTIL HNED PŘI KONTROLE
===============================================================================

  Při vkládání nových `.panel button`/`.panel button.secondary`
  pravidel jsem je vložil PŘED staré, aniž bych staré smazal - vznikly
  DVA řádky pro každý selektor. CSS by v tom případě použilo ten
  POZDĚJŠÍ (starý, bez barev) a nová barevná pravidla by potichu
  vůbec neplatila. Všiml jsem si toho hned při rutinní kontrole počtu
  výskytů (přesně ten typ kontroly, co dělám po každé podobné úpravě)
  a duplicity smazal, než šlo cokoli dál.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (78). Žádné duplicitní
  CSS selektory (`.panel button`, `.panel button.secondary`, `.panel
  button.zavrit`, `.panel h2`, `.panel p`, `.panel b` - každé přesně
  jednou, ověřeno počítáním výskytů). `stranka_kontrola.py` (TV cesta)
  - 0 chyb.

  Všech šest existujících jsdom simulací spuštěno znovu - beze změny
  prošly (čistě vzhledová změna, žádná logika se nedotkla).
