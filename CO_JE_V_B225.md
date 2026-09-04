# B225 — paměťová karta přežije odinstalaci appky (versionCode 271)

Rene: "kdyz apku vymazu a znova nainstaluji tak - cd/iso - si hry
pamatuje, najde si je, ale memory card si nepamatuje ulozene hry, je
prazdna."

===============================================================================
 PŘÍČINA
===============================================================================

  `saveDir` (proměnná, kterou appka posílá do jádra jako `g_savedir` -
  tam jádro staví `memory_card_1.srm`, viz B218) šla přes
  `getFilesDir()` - APPCE-PRIVÁTNÍ interní úložiště. Android při
  odinstalaci appky tohle úložiště SMAŽE automaticky, bez výjimky.

  Hry v knihovně (CD/ISO) přežily, protože už od dřívějška leží ve
  VEŘEJNÉM úložišti (`Environment.getExternalStoragePublicDirectory
  (DIRECTORY_DOWNLOADS)/AtariHelp`) - to Android při odinstalaci
  appky NEMAŽE, protože nepatří appce, ale uživateli.

===============================================================================
 OPRAVA
===============================================================================

  Nová `getPublicMemoryCardDir()` - vrací
  `Downloads/AtariHelp/PS1_pametova_karta`, stejné veřejné úložiště
  jako mají hry, jen jiná podsložka. Použita na všech třech místech,
  kde appka bootuje BIOS nebo hru (`bootBiosSafe`, `bootGameSafe` -
  jednou z knihovny/EGL cesty, jednou z ručního výběru souboru).

  Tohle přesně odpovídá tomu, jak se má přemýšlet o skutečné PS1
  paměťové kartě (viz B218): je to FYZICKY ODDĚLENÝ předmět od
  konzole. Když bys konzoli vyhodil a koupil novou, karta v ní pořád
  má tvoje data. Teď se appka chová stejně - smazání/přeinstalování
  appky (= "nová konzole") kartu (= "fyzický předmět") nesmaže.

  JEDNORÁZOVÁ MIGRACE: pokud už existuje karta ve staré, privátní
  cestě (ze všeho dosavadního testování B215-B224) a v nové veřejné
  cestě ještě žádná není, `getPublicMemoryCardDir()` ji tam při prvním
  volání zkopíruje - ať nepřijdeš o rozehrané testování kvůli tomuhle
  přesunu.

  QUICK-SAVE-STATES (`.slot0.state`, tlačítko SAVE GAME/ps1QuickSave)
  ZŮSTÁVAJÍ v appce-privátním úložišti - to je čistě emulátorová
  pohodlnostní funkce (skutečná PS1 nic takového nemá), ne autentická
  součást hardwaru jako memory karta. Netýkalo se tvého hlášení, tak
  jsem to nechal beze změny.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  { } v MainActivity.java - 2360/2360.

  Ověřeno, že VŠECHNA tři místa, kde appka skutečně bootuje BIOS/hru
  (a tedy posílá `saveDir` do jádra), teď volají novou funkci - žádné
  z nich už nejde přes `getFilesDir()`. Jeden další výskyt staré cesty
  (`bootPs1FileOnCurrentThread`) prošetřen zvlášť - je to mrtvý/
  vestigiální kód, který jen vytváří starou privátní složku pro
  quick-save-states, do jádra nikam nejde, netýká se tohohle problému.

  CO NEJDE OVĚŘIT ODSUD: skutečné chování při odinstalaci/přeinstalaci
  appky - to je jen na tvém telefonu. Doporučený test: nahraj kartu
  daty (zahraj něco, ulož přes ve hře memory card menu), appku
  odinstaluj a nainstaluj znovu, zkontroluj přes MEMORY CARD (stav
  karty), jestli `soubor_existuje=ANO` a velikost sedí.
