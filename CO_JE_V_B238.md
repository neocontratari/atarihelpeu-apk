# B238 — Sega: knihovna, uložené pozice, SBÍRKA rozcestník, bottom-row (versionCode 284)

Rene si všiml, že jsem po TV cast vyšetřování zapomněl dodělat práci,
kterou po mně chtěl pro Segu: "tu Segu sis nedělal Partáku? SBÍRKA
jede furt stejně - bez možnosti výběru hrát či uložit. Tlačítko
ULOŽENÉ vůbec nefunguje a tlačítko HRY skáče na přímo do mobilu.
Projdi si to, co jsem po tobě chtěl, a oprav to."

Tohle je ta práce, dokončená ve dvou navazujících zprávách (Java
infrastruktura + JS/HTML dokončení).

===============================================================================
 1) HRY → SKUTEČNÁ KNIHOVNA (jako PS1)
===============================================================================

  Dřív: `romPicker` - rovnou otevřel systémový výběr souboru.
  Teď: otevře panel "MOJE HRY" se seznamem stažených her (SPUSTIT/
  SMAZAT), stejný vzor jako PS1 knihovna (B213-B225). "VYBRAT SOUBOR
  RUČNĚ V TELEFONU" zůstává uvnitř jako druhá volba - stará funkčnost
  nezmizela.

  Veřejné úložiště (`Downloads/AtariHelp/Sega_hry`) rovnou od začátku -
  na rozdíl od PS1 (kde memory karta musela projít migrací v B225,
  protože šla nejdřív přes appce-privátní cestu), tady jsem se z toho
  poučil a udělal to správně napoprvé.

===============================================================================
 2) ULOŽENÉ → SEZNAM ULOŽENÝCH POZIC (per hra)
===============================================================================

  Dřív: export diagnostického logu (`saveLog()`).
  Teď: otevře panel "ULOŽENÉ POZICE" se seznamem her, které MAJÍ
  uloženou pozici. Klepnutí na hru pozici rovnou načte - pokud ta hra
  zrovna neběží, appka ji nejdřív sama najde v knihovně a nahraje.

  DŮLEŽITÁ ZMĚNA OPROTI B236: uložená pozice teď patří KE KONKRÉTNÍ
  hře (`sega_saves/<název_hry>.state`), ne jeden sdílený slot pro
  všechny hry najednou - jinak by "načíst pozici" u jiné hry vůbec
  nedávalo smysl.

  Export logu jsem nezahodil - přesunul jsem ho do D-PAD panelu jako
  "EXPORTOVAT LOG (DIAGNOSTIKA)", pro případ, že by se ještě hodil.

===============================================================================
 3) SBÍRKA → ROZCESTNÍK PO NAČTENÍ
===============================================================================

  SBÍRKA se chová přesně jako doposud - klepnutí naviguje na web,
  vybraná hra se hned přehraje, nic se nezdržuje. NOVĚ: jakmile hra
  naběhne, appka nabídne malý plovoucí pruh "Uložit „Název“ do
  telefonu?" s tlačítky ULOŽIT / NE DÍKY, který sám zmizí za 8 vteřin,
  pokud si ho nikdo nevšimne.

  Technicky: appka sleduje AKTUÁLNÍ hru (jméno i bajty) na JEDNOM
  společném místě (`loadRomBase64` v Javě) - tudy prochází úplně
  všechny způsoby nahrání hry (ruční výběr, knihovna, SBÍRKA), takže
  stačilo přidat sledování na tohle jedno místo, ne duplikovat ho
  všude zvlášť.

===============================================================================
 4) BOTTOM-ROW: SAVE — START — LOAD GAME
===============================================================================

  V landscape teď vedle START (uprostřed dole, od B236) přibyly SAVE
  (vlevo) a LOAD GAME (vpravo) - přímý přístup k ukládání/načítání
  BĚHEM hraní, ne až přes D-PAD panel.

  Pozice spočítané s bezpečnou mezerou od D-padu (končí kolem
  1.8vw+43vh) i sloupce A/B/C (začíná kolem 2.2vw+17.5vh od prava) -
  žádný překryv.

  DŮLEŽITÝ DETAIL: appka má jeden globální dotykový obslužník, který
  bere VŠECHNO na herní ploše jako herní vstup (D-pad/A/B/C/Start),
  s výjimkou pár jmenovitě vyloučených tlačítek (SBÍRKA, HRY,
  ULOŽENÉ...). Nová tlačítka SAVE/LOAD GAME jsem musel přidat do
  TÉHLE výjimkové listiny - jinak by dotyk na ně mohl být špatně
  vyhodnocen jako herní vstup.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  { } v MainActivity.java - beze změny v počtu párů, jen posunuté.

  Obě existující jsdom simulace (layout, nebula) spuštěny znovu na
  novém kódu - beze změny prošly, log appky sám ukazuje přepojení
  (`id=btnGames key=library`, `id=btnSaves key=savedPositions`).

  Nová simulace (test_sega_library_full.js), sedm bodů:
    - HRY otevře knihovnu se 2 hrami (ne systémový picker)
    - klik na SPUSTIT zavolá launch se správným klíčem, panel se zavře
    - klik na SMAZAT otevře potvrzení, HRU I POZICI smaže obojí
    - ULOŽENÉ otevře seznam pozic (1 hra), klik NAČTE správnou pozici
    - po `napInjectRomBase64` (simulace SBÍRKY) se objeví nabídka
      uložení, klik ULOŽIT zavolá uložení do knihovny a nabídka zmizí
    - příznak "ze sbírky" se sám nevznáší mezi voláními
    - bottom-row SAVE/LOAD existují v DOM

  CO NEJDE OVĚŘIT ODSUD: přesné pozice SAVE/LOAD/START v landscape na
  reálném zařízení (jsdom neumí věrně vyhodnotit @media dotazy vázané
  na skutečné rozměry - stejné, už dříve zdokumentované omezení).
  Textově ověřeno, že CSS pravidla jsou správně uvnitř landscape
  media query s bezpečně spočítanými odstupy.
