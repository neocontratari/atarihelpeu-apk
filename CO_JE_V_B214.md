# B214 — paměťová karta je knihovna her, D-pad knoflík zpátky (versionCode 260)

Rene: "zapomen na to co bylo - memory card musi fungovat presne tak jako
jadro ps1 - tudiz otevru ps1 bez cd ukaze se memory card a prehravac
cd - v memory card musi byt ulozene hry - a to presne podle adresaru co
se vytvori v mobilu... nactu hru a pres memory card musim otevrit hru z
kazde pozice a z kazde hry." Plus dodělávka z minula: D-pad knoflík
sledující prst, a průběžně žádaná aktualizace hlavního protokolu.

===============================================================================
 1) KRITICKÝ NÁLEZ, KTERÝ VYSVĚTLUJE, PROČ KNIHOVNA NEFUNGOVALA
===============================================================================

  Appka měla funkci `ps1PurgeOtherRemoteGames()`, která se volala PŘED
  KAŽDÝM stažením nové hry a smazala VŠECHNY předchozí stažené hry
  (jak z veřejné Downloads/AtariHelp/PS1 složky, tak z appce-privátní).
  Byl to záměrný návrh (šetřit místo - PS1 hry mají stovky MB), ale je
  to přesný opak toho, co Rene chce: knihovnu více her, ne jednu
  aktivní hru, kterou při dalším stažení ztratíš.

  Řádek `ps1PurgeOtherRemoteGames(dir);` v `startPs1RemoteDownloadAndBoot`
  odstraněn z volací cesty. Kontrola volného místa (`ps1EnsureFreeSpace`)
  zůstává - pokud dojde místo, appka to řekne a uživatel si sám smaže,
  co nechce, přes novou knihovnu (viz níže).

===============================================================================
 2) PAMĚŤOVÁ KARTA = SKUTEČNÁ KNIHOVNA HER
===============================================================================

  Tlačítko "MEMORY CARD" (portrait, `btnMemory`) dřív dělalo jen
  `ps1QuickLoad()` - matoucí, protože to je jiná věc (save state, ne
  memory card). Teď otevírá nový panel PAMĚŤOVÁ KARTA se seznamem VŠECH
  stažených her.

  Adresářová struktura pro to už existovala (`ps1RemoteCacheDir` dává
  každé hře vlastní podsložku podle URL/Google Drive ID) - jen se
  nikde neuměla vypsat. Nové Java metody (C++ by tu nešlo použít -
  procházení adresářů a mazání souborů je na Androidu přes Javu):

      ps1LibraryListJson() - projde obě možná uložiště (veřejné
        Downloads/AtariHelp/PS1 i appce-privátní), pro každou složku
        s kompletně staženou hrou (má boot marker) vrátí klíč, název,
        velikost a jestli k ní existuje uložená hra (.slot0.state)
      ps1LibraryDirByKey(key) - najde adresář podle klíče, bezpečně
        (viz oprava níže)
      ps1LibraryDeleteByKey(key) - smaže celou složku hry
      ps1LibraryLaunchByKey(key) - spustí přesně tu hru, bez stahování

  V JS: klepnutí na hru v seznamu ji spustí (▶) nebo smaže (🗑, s
  potvrzovacím dialogem). Prázdná knihovna ukáže srozumitelnou hlášku
  místo prázdného boxu.

  BEZPEČNOSTNÍ OPRAVA PŘI PSANÍ: `ps1LibraryDirByKey` nejdřív porovnávala
  cesty jen jako TEXT (`getParentFile().getAbsolutePath().equals(...)`).
  Klíč `".."` projde čištěním názvu (tečky jsou povolené znaky) a
  textové porovnání by ho nechalo projít, i když `File.delete()` na
  systémové úrovni `..` skutečně rozřeší - teoreticky by šlo smazat
  rodičovský adresář. Opraveno na `getCanonicalFile()` porovnání, které
  `..` doopravdy rozřeší, ne jen jako text.

===============================================================================
 3) DIAGNOSTIKA MEMORY KARTY (.srm) V /8765/log
===============================================================================

  Appka už měla docela důkladný systém pro in-game memory card save
  (`nap_srm_save_if_dirty`, volaný při onPause/onDestroy/odchodu ze
  stránky/hardware BACK) - ale jeho diagnostické hlášky
  ("MEMCARD_SAVED"/"MEMCARD_NONE"/...) šly jen do Android logcatu,
  který Rene nemá jak přečíst.

  Nová nativní funkce `ps1MemCardInfo()` (C++, stejný vzor jako už
  existující `ps1Status()`) vrací aktuální stav na požádání - má jádro
  SRAM? Existuje soubor? Kolik bajtů? Povedlo se poslední uložení?
  Volá se po každém stisku SAVE GAME i MEMORY CARD tlačítka a posílá
  se přes už existující `jsLog` most do `/8765/log`.

===============================================================================
 4) D-PAD KNOFLÍK SLEDUJE PRST
===============================================================================

  Dodělávka z minula: `#dpad` má teď uvnitř `<span class="dpadKnob">`,
  který se v `bindDpad()`/`movePoint()` živě posouvá podle polohy
  doteku (omezeno na poloměr kruhu, ať nevyleze ven), a po puštění
  skočí zpátky na střed. Viditelný jen v landscape (v portrait je celý
  D-pad neviditelný, viz B213).

===============================================================================
 5) AKTUALIZOVÁN HLAVNÍ PŘEDÁVACÍ PROTOKOL
===============================================================================

  `PREDAVACI_PROTOKOL_PRO_NASTUPCE.txt` má nový bod 7 - shrnutí toho,
  co se stalo od B197 dál, a připomínky, které Rene opakovaně zdůrazňoval
  (zapomenout na Javu, všechno musí jít do /8765/log, ověřovat před
  mazáním, číst screenshoty/logy důkladně). Od teď se tenhle bod
  aktualizuje u každého buildu, jak Rene žádal - "kdyby se něco stalo,
  ať mám stoprocentní zálohu i v předávání dál."

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (66 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.
  Rovnováha { } v MainActivity.java - 2334/2334 (kontrolováno po každém
  zásahu zvlášť, ne jen na konci).

  Nová jsdom simulace (test_ps1_library.js) ověřila:
    - klik na MEMORY CARD otevře knihovnu a vypíše obě zadané hry se
      správnými jmény, velikostmi a značkou "uložená hra" jen tam, kde
      má být
    - klik na ▶ u druhé hry zavolá ps1LibraryLaunch se SPRÁVNÝM klíčem
      (ne první hry, ne obou) a zavře panel
    - klik na 🗑 (s potvrzením) zavolá ps1LibraryDelete se správným
      klíčem a seznam se sám obnoví
    - prázdná knihovna ukáže srozumitelnou hlášku
    - žádná JS chyba v celém průběhu

  Plný `javac`/spuštění appky jsem v tomhle prostředí znovu nemohl
  ověřit (chybí Android SDK) - syntaxi C++ i Java bloků jsem ověřil
  ručně po každé úpravě (rovnováha závorek + `stranka_kontrola.py`).
