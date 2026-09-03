# B218 — jedna trvalá karta, přesně jako na PS1 (versionCode 264)

Rene: "Ne takto memory card nepracuje na ps1 - uz po vyjeti z emu ps1 si
memory card nepamatuje ulozenou pozici. To v te memory cards musi
zustat i kdyz vypnu celou apku - a hned i bios pri prvnim zapnuti musi
videt ze na memory cards neco je. Presne tak ze je furt memory card
zasunuta v ps1."

Měl pravdu a byla to zásadní chyba v přemýšlení, ne detail. B215-B217
řešily **špatný problém**.

===============================================================================
 CO BYLO ŠPATNĚ (B215-B217)
===============================================================================

  Myslel jsem si, že každá hra potřebuje SVOJI VLASTNÍ kartu (aby se
  navzájem "nepřepisovaly"), a stavěl jsem cestu k `.srm` souboru podle
  toho, ve které složce hra leží (`gdrive_hra_A__game.srm` vs.
  `url_hra_B__game.srm` - dva RŮZNÉ soubory pro dvě různé hry).

  Skutečná PS1 to takhle nedělá. Má JEDNU fyzickou kartu (128 kB),
  furt zasunutou - a různé hry na ní PŘIROZENĚ KOEXISTUJÍ, každá si
  zabere jen několik z 15 bloků karty. O to, který blok patří které
  hře, se stará FORMÁT KARTY SAMOTNÉ (hlavička + tabulka bloků) - to
  už dělá jádro (PCSX ReARMed) uvnitř `Mcd1Data` samo, jakmile mu
  appka dá pořád tu STEJNOU kartu.

===============================================================================
 OPRAVA
===============================================================================

  `nap_srm_set_path()` už vůbec nezávisí na tom, jaká hra (nebo jestli
  vůbec nějaká) běží. Cesta je vždycky stejná:

      g_srm_path = g_savedir + "/memory_card_1.srm"

  Parametr `gamePath` zůstává v signatuře (ať nemusím měnit volání na
  třech místech kódu), ale uvnitř funkce se už nepoužívá.

  DALŠÍ MEZERA, KTEROU JSEM PŘI TÉHLE OPRAVĚ NAŠEL: `ps1BootDoMonitoru()`
  - funkce, která bootuje BIOS BEZ DISKU i skutečnou hru (podle toho,
  jestli `jeHra` je pravda) - vůbec nevolala `nap_srm_set_path()`/
  `nap_srm_load()`! Karta se při tomhle způsobu bootu nikdy nenačetla do
  `Mcd1Data` - přesně to, co Rene popsal: BIOS při startu bez disku
  neviděl na kartě nic, i kdyby tam něco bylo uložené z předchozího
  běhu. Přidáno hned po `g_loaded.store(true)`, stejně jako ve zbylých
  dvou boot funkcích.

===============================================================================
 OVĚŘENÍ - 5KROKOVÝ TEST PŘESNĚ PODLE TOHO, CO RENE POPSAL
===============================================================================

  node --check, `stranka_kontrola.py` - 0 chyb. Rovnováha { } v C++
  souboru - 238/238 (změnila se, protože zjednodušení odstranilo
  několik if-bloků, které tam předtím byly).

  Nový test (`test_overeni/test_jedna_karta.cpp`), 5 kroků přesně podle
  Reného popisu:

    1. BIOS bez disku, úplně první zapnutí appky -> karta prázdná
       (správně - nová karta, nic na ní ještě není)
    2. Hra A (Crash Bandicoot) nabootována, uloží si pozici,
       "appka se vypne" (simulace ps1Stop -> nap_srm_save_if_dirty)
    3. Cesta BIOSu a cesta hry A JSOU STEJNÉ (jedna karta, ne dvě
       oddělené) - u B215-B217 by tohle SELHALO
    4. Simulace restartu appky (čerstvý proces, prázdná paměť) - BIOS
       bez disku PO restartu VIDÍ uloženou hru z disku
    5. Jiná hra (Tekken 3) nabootována - vidí TU SAMOU kartu se stejnou
       cestou, a VIDÍ na ní uloženou pozici hry A (koexistence, přesně
       jako na skutečné PS1)

  Všech pět kroků prochází, ověřeno i po rozbalení hotového balíčku.
  Testovací soubor je přiložený v `test_overeni/` spolu s předchozími.
