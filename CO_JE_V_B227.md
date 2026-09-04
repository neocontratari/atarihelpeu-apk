# B227 — RESET jako vytažení CD, rychlý přepínač průhlednosti (versionCode 273)

Rene: "takto bych to nechal az na tu grafiku... tlacitko reset v emu
ps1 musi fungovat tak, ze po zmacknuti se vyresetuje hra /jakoby bych
vytahnul cd/ a nabehne bios jadro bez cd... v nastaveni d-padu udelej
jeste to aby byl bud ovladac pruhledny a nebo nepruhledny."

Grafiku CD/ISO/nastavení Rene sám navrhne a pošle jako předlohu -
zatím se jí nedotýkám.

===============================================================================
 1) RESET = VYTÁHNOUT CD, NABĚHNE BIOS BEZ DISKU
===============================================================================

  RESET dřív jen napsal hlášku ("PS1 RESET připraven") a nic dalšího
  neudělal. Teď nová `AHPS1.ps1Reset()` (JS-volatelná) spojuje DVĚ UŽ
  EXISTUJÍCÍ, bezpečné věci přesně v tomhle pořadí - žádná nová
  nízkoúrovňová logika:

      1. `stopPs1SessionHard("jsPs1Reset")` - stejná cesta, jakou appka
         už používá při odchodu ze stránky/pauze appky - zastaví zvuk,
         jádro, uzavře soubor hry
      2. `ps1MaybeStartBios()` - appka si SAMA hlídá, že nespustí nic,
         dokud jádro není úplně dole (kontroluje `ps1BiosRunning`/
         `ps1SessionActive`/`ps1BootActive`) - po kroku 1 už tahle
         podmínka správně pustí start BIOSu bez disku

  RESET teď taky ukazuje stejný tikající odpočet a indikátor přímo v
  monitoru jako spuštění hry z knihovny (B226), jen s vlastním
  popiskem "Resetuji (BIOS bez CD)" místo "Spouštím hru".

===============================================================================
 2) RYCHLÝ PŘEPÍNAČ "PRŮHLEDNÝ OVLADAČ"
===============================================================================

  V OVLÁDÁNÍ panelu byl už jemný posuvník průhlednosti (25-100 %), ale
  Rene chtěl jednoduchou volbu "buď/nebo" pro hráče, co nechtějí
  ladit posuvník. Nový checkbox "Průhledný ovladač" vedle posuvníku:

      zaškrtnuto  -> opacity = 30 % (vidíš skrz na hru)
      odškrtnuto  -> opacity = 100 % (výchozí, neprůhledné - to chce Rene)

  Obousměrná synchronizace s posuvníkem - přepínač nastaví posuvník na
  odpovídající hodnotu, a ruční posun posuvníku pod/nad 40 % zase
  přepne checkbox. Uloženo ve stejném `ctrlSettings.opacity`, žádná
  nová proměnná ani konflikt s existujícím posuvníkem.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (77). `stranka_kontrola.py`
  (TV cesta) - 0 chyb. Rovnováha { } v MainActivity.java - 2364/2364.

  Nová jsdom simulace (test_ps1_reset_transparency.js):
    - klik na RESET zavolá `AHPS1.ps1Reset()` a ukáže odpočet i
      indikátor v monitoru
    - výchozí stav přepínače odpovídá 100% opacity (odškrtnutý)
    - zaškrtnutí nastaví opacity na 30 % a posune posuvník na 30
    - odškrtnutí vrátí na 100 % a posuvník na 100
    - ruční posun posuvníku na 25 zaškrtne přepínač, posun na 80 ho
      odškrtne

  Zbylé čtyři existující simulace (plocha, odpočet, CD/ISO přehození,
  indikátor v monitoru) spuštěné znovu - beze změny prošly.

  CO NEJDE OVĚŘIT ODSUD: jestli `ps1MaybeStartBios()` po
  `stopPs1SessionHard()` doopravdy nastartuje BIOS spolehlivě na
  reálném zařízení (časování dvou po sobě jdoucích asynchronních
  operací) - to je na tvém testu.
