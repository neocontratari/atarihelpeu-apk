# B215 — kolize uložených pozic opravena, volba při mazání (versionCode 261)

Rene: "memory card musi fungovat presne tak jak ma? Mas osetrene
ukladani - a hlasku o tom jestli chci premazat celou hru a nebo celou
hru vcetne ulozenych save game... Partaku - nelakuj me."

Poctivá odpověď na první otázku je v hlavní zprávě (shrnutí: NE, není to
integrace s opravdovým BIOSem appky, je to appková knihovna vedle něj).
Tenhle build řeší tu druhou, konkrétní část - ošetřené ukládání a volbu
při mazání.

===============================================================================
 KRITICKÁ OPRAVA: KOLIZE ULOŽENÝCH POZIC MEZI RŮZNÝMI HRAMI
===============================================================================

  Save-state (`ps1StateFile`/`ps1CurrentGameLabel`) i nativní memory
  card (`nap_srm_set_path` v C++) používaly jako klíč k uložené pozici
  JEN NÁZEV SOUBORU hry. PS1 dumpy mají často generické názvy
  ("game.bin", "disc1.bin", "track01.bin"...) - DVĚ RŮZNÉ hry se stejným
  názvem by si tiše přepsaly uloženou pozici, BEZ JAKÉHOKOLI varování.
  S B214 (knihovna více her najednou) je tohle riziko mnohem reálnější
  než dřív, kdy appka držela jen jednu aktivní hru.

  Každá hra má už ale svou VLASTNÍ, jedinečnou složku (`ps1RemoteCacheDir`
  podle URL/Google Drive ID). Klíč k uložené pozici se teď skládá ZE
  SLOŽKY I NÁZVU SOUBORU, ne jen z názvu:

      C++ (nap_srm_set_path): z plné cesty ke hře se vytáhne i název
        rodičovské složky, ne jen název souboru - "gdrive_abc123__game"
        místo jen "game"
      Java (bootPs1FileOnCurrentThread → ps1CurrentGameLabel): stejný
        princip, `parentName + "__" + rawLabel`
      Java (ps1LibrarySaveStateFor, pro zobrazení "uložená hra" v
        knihovně): musí počítat úplně stejný klíč, jinak by knihovna
        ukazovala "žádná uložená hra" i tam, kde reálně je

  Kořenové názvy složek ("ps1_games", "PS1") se vynechávají (nejsou
  jedinečné samy o sobě), takže staré uložené pozice z jednoznačných
  případů zůstávají čitelné.

===============================================================================
 VOLBA PŘI MAZÁNÍ - VLASTNÍ DIALOG, NE OBYČEJNÝ confirm()
===============================================================================

  Dřív: klik na koš u hry v knihovně → `confirm()` (jen ANO/NE) → smazat.

  Teď: klik na koš otevře dialog SMAZAT HRU se třemi tlačítky:
      JEN HRU (uložená pozice zůstane)
      HRU I ULOŽENOU POZICI
      ZRUŠIT

  `ps1LibraryDeleteByKey(key, wipeSaveToo)` — když `wipeSaveToo` je
  pravda, přečte si název souboru hry PŘED smazáním složky (jinak by
  se to už nedalo zjistit), spočítá stejný klíč jako `ps1LibrarySaveStateFor`
  a smaže i uloženou pozici, s hlášením kolik místa se uvolnilo
  celkem (hra + případná uložená pozice).

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (71 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.
  Rovnováha { } - Java 2341/2341, C++ 241/241 (obě ověřeny samostatně).

  Rozšířená jsdom simulace (test_ps1_library.js):
    - klik na koš OTEVŘE dialog, nesmaže nic hned
    - před volbou je log mazání prázdný
    - volba "HRU I ULOŽENOU POZICI" zavolá ps1LibraryDelete se správným
      klíčem A druhým parametrem true
    - dialog se po volbě zavře
    - ZRUŠIT nezavolá žádné mazání (log zůstává beze změny)
    - žádná JS chyba v celém průběhu
