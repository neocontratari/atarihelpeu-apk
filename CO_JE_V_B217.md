# B217 — 2x měř: druhá kontrola našla skutečnou chybu (versionCode 263)

Rene: "kodu neverim... 8000 testu mam za sebou... udelej si kontrolu at
to mame super... 2x mer jednou rez." Měl pravdu mít pochybnosti - druhé,
hlubší kolo testování (všechny opravy najednou, ne izolovaně) skutečnou
chybu našlo.

===============================================================================
 CO JSEM UDĚLAL NAVÍC (nad rámec B216)
===============================================================================

  1) Ověřil jsem, že BIOS soubory, co jsi poslal, mají správnou velikost
     (524 288 bajtů = přesně to, co appka kontroluje) - všechny čtyři
     (scph1001/5500/5501/7502.bin) sedí.

  2) Ověřil jsem POŘADÍ volání, ne jen že oprava z B216 funguje
     izolovaně: appka volá `retro_load_game()` (uvnitř kterého jádro
     samo volá `load_memcards()`) VŽDY před `nap_srm_set_path()`/
     `nap_srm_load()` - na obou místech v kódu, kde se hra bootuje.
     Takže až appka zkusí kartu číst, jádro už bude mít
     `memcard_type[0]` nastavené správně.

  3) Napsal jsem DRUHÝ, hlubší test - ne izolovaně "zapne se karta?",
     ale CELÝ řetězec najednou: dvě RŮZNÉ hry se STEJNÝM názvem
     souboru (běžný případ u PS1 dumpů - přesně scénář, který řešila
     B215), obě nabootované, obě si "uloží" vlastní odlišná data,
     a kontrola, že se nikde nezkříží.

===============================================================================
 CHYBA, KTEROU TENHLE DRUHÝ TEST NAŠEL
===============================================================================

  `nap_srm_load()` - když .srm soubor JEŠTĚ NEEXISTUJE (hra se hraje
  poprvé), kód jen napsal do logu "MEMCARD_NEW, poprvé" a nic dalšího
  neudělal. Neuvědomil jsem si (a ani B216 by to samo o sobě
  neodhalilo, protože testovalo jen JEDNU hru): **karta (`Mcd1Data`) je
  globální 128kB buffer sdílený mezi VŠEMI hrami po celou dobu běhu
  appky.** Když si zahraješ hru A, appka jí naplní kartu jejími daty.
  Když pak přepneš na NOVOU hru B (co ještě nikdy nehrálas), appka si
  sice správně všimne, že B nemá svůj .srm soubor - ale NIKDY
  nevymazala to, co v kartě zůstalo po hře A. Hra B by tak uviděla
  cizí, nesouvisející uloženou pozici, i když by na disku všechno
  vypadalo v pořádku (její vlastní .srm soubor správně neexistoval).

  Test se dvěma hrami stejného jména (`test_overeni/test_end_to_end.cpp`,
  KROK 3) tohle přesně odhalil - PŘED opravou padal, PO opravě prochází.

===============================================================================
 OPRAVA
===============================================================================

  V `nap_srm_load()` přidán `memset(mem, 0, sz);` PŘED pokusem o
  otevření souboru. Nová hra teď vždycky začíná s vymazanou (naprosto
  prázdnou, jako z výroby) kartou, ne s cizí uloženou pozicí. Existuje-li
  soubor, přepíše se nad tím normálně jako dřív.

===============================================================================
 OVĚŘENÍ - OBA TESTY SPOLU, NE IZOLOVANĚ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  { } v C++ souboru - 241/241 (beze změny, oprava je uvnitř už
  existujícího bloku).

  `test_overeni/test_memcard_wiring.cpp` (z B216, zapnutí karty) -
  spuštěno znovu se ZDROJÁKEM PO téhle opravě - pořád prochází
  (oprava jednoho problému nerozbila ten druhý).

  `test_overeni/test_end_to_end.cpp` (nový, celý řetězec):
    krok 1: dvě hry stejného jména nabootovány
    krok 2: jejich .srm cesty na disku jsou různé (B215 stále funguje)
    krok 3: hra B po nabootování NEVIDÍ data hry A (NOVÁ oprava)
    krok 4: opětovné nabootování hry A vrátí JEJÍ data, ne data hry B
    krok 5: na disku existují dva samostatné .srm soubory
    → všech pět kroků prochází, ověřeno i po rozbalení hotového balíčku

  Oba testovací soubory jsou přiložené v `test_overeni/` spolu s tím
  starým, ať jde všechno znovu zopakovat.
