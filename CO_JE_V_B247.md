# B247 — PS1 umí načíst rychlou pozici (versionCode 293)

Rene: "kdyby se náš save game připsal na memory card a hra by po
načtení naběhla do tohoto místa, jak to teď dělá Sega, to by byl bonus
pro všechny, co hrají o pauze v práci. Jde o to, jestli to dokážeš
udělat tak, aby sis neporušil funkční PS1 - tohle rozhodnutí nechávám
na tobě."

===============================================================================
 PROČ TO BYLO NÍZKORIZIKOVÉ
===============================================================================

  Než jsem cokoli napsal, zjistil jsem, že `ps1QuickLoad()` **už celý
  existoval** - na obou stranách:

  - Java (`MainActivity.java`): kompletní, včetně per-hra ukládání
    (`ps1StateFile()` používá `ps1CurrentGameLabel` - přesně stejný
    princip jako u Segy)
  - JS (`emu_ps1/index.html`): kompletní, včetně hlášky "SAVE STATE
    načten" a zápisu do logu

  Jediné, co chybělo, bylo tlačítko, které tuhle už hotovou funkci
  zavolá.

===============================================================================
 CO PŘIBYLO
===============================================================================

  Jedno nové tlačítko "NAČÍST RYCHLOU POZICI" - **do už existujícího**
  panelu D-PAD A OVLÁDÁNÍ (vedle UPRAVIT ROZLOŽENÍ / OVLÁDÁNÍ /
  PROHODIT D-PAD), ne do hlavního řádku pěti fungujících tlačítek dole
  na úvodní obrazovce (RESET / MEMORY CARD / CD-ISO / LOAD GAME / SAVE
  GAME). Tenhle řádek zůstává úplně beze změny.

  Žádná změna ukládacího mechanismu, cesty k souboru ani formátu -
  jen připojení už hotové, existující funkce k novému tlačítku.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, žádná duplicitní id (79), `stranka_kontrola.py` (TV
  cesta) - 0 chyb.

  Všech osm existujících PS1 jsdom simulací spuštěno znovu - beze
  změny prošly (ověřuje, že hlavní obrazovka, knihovna, monitor při
  načítání, průhlednost, dvojitý RESET, vesmírné pozadí i pauza
  animací při TV castu fungují přesně jako předtím).

  Nová simulace (`test_ps1_quickload_button.js`), čtyři kontroly:
    - existující SAVE GAME na hlavní obrazovce funguje beze změny
    - nové tlačítko existuje přesně v D-PAD panelu
    - klik na něj správně zavolá `ps1QuickLoad`
    - ostatní tlačítka téhož panelu zůstala nedotčená

  CO NEJDE OVĚŘIT ODSUD: skutečné načtení uložené pozice na reálném
  zařízení - to je na tvém testu. Vzhledem k tomu, že se použila
  100% existující, už dřív fungující funkce (jen se k ní přidalo
  tlačítko), je riziko regrese minimální.
