# B216 — skutečná příčina "memory card nefunguje" (versionCode 262)

Rene: "chci primou emulaci a bez fake. tak jdeme do toho. memory card u
ps1 je dulezita bez toho to nema smysl."

Šel jsem do jádra samotného (PCSX ReARMed, `vendor/pcsx_rearmed`) a
zjistil, že B213-B215 (diagnostika + oprava kolizí názvů) opravovaly
skutečné věci - ale narážely na mnohem hlubší problém, který žádná z
nich nemohla odhalit: **jádro mělo memory kartu úplně vypnutou, od
samého začátku.**

===============================================================================
 CO SE SKUTEČNĚ DĚLO
===============================================================================

  PCSX ReARMed se při startu ptá frontendu (naší appky) na tzv. core
  option `pcsx_rearmed_memcard1` (`load_memcards()` v
  `vendor/pcsx_rearmed/frontend/libretro.c`). Podle odpovědi buď:
      "libretro"  -> jádro drží kartu samo v paměti (Mcd1Data, 128 kB -
                     PŘESNĚ velikost skutečné PS1 karty), frontend si
                     ji čte/zapisuje přes standardní SAVE_RAM rozhraní
      "serial"/"shared" -> jádro si kartu spravuje samo, soubor na disku
      cokoli jiného / appka neodpoví -> memcard_type zůstane NONE

  Appka (`nap_core_option_value()` v `nap_ps1_native.cpp`) měla
  odpovědi připravené pro BIOS, region, DRC, async CD, enhanced
  rozlišení - ale na `pcsx_rearmed_memcard1` odpověď NEBYLA. Když jádro
  na dotaz nedostane platnou odpověď, `load_memcards()` rovnou
  přeskočí (`continue`) a `memcard_type[0]` zůstane na výchozí `0` =
  `MEMCARDTYPE_NONE`.

  Důsledek: `retro_get_memory_data(RETRO_MEMORY_SAVE_RAM)` vracelo VŽDY
  `NULL` a `retro_get_memory_size` VŽDY `0` - přesně to, co diagnostika
  z B213 hlásila jako `MEMCARD_NONE core nedava SAVE_RAM`. Celý systém
  ukládání (i po opravě kolizí v B215) neměl vůbec co ukládat - jádro
  mu nikdy nedalo žádnou paměť k dispozici.

===============================================================================
 OPRAVA
===============================================================================

  V `nap_core_option_value()` přidány dva řádky:

      if (strcmp(key, "pcsx_rearmed_memcard1") == 0) return "libretro";
      if (strcmp(key, "pcsx_rearmed_memcard2") == 0) return "libretro";

  Stejný, už zavedený a fungující vzor jako u BIOS/region/DRC výše v
  té samé tabulce - ne nový mechanismus, jen chybějící řádek v už
  existující tabulce.

  ŽÁDNÝ VLASTNÍ/ZJEDNODUŠENÝ FORMÁT: `MCD_SIZE` v jádru samotném je
  `1024*8*16 = 131072 bajtů = 128 kB` - přesná velikost skutečné PS1
  paměťové karty. Data (`Mcd1Data`) spravuje jádro samo, ve stejném
  binárním formátu, jaký používá opravdový PS1 - appka jen ukládá a
  načítá přesně tenhle blok, přes standardní libretro SAVE_RAM
  rozhraní. To je ta "přímá emulace, bez fake", o kterou jsi žádal.

===============================================================================
 VZTAH K B215 (kolize názvů) - obě opravy se doplňují
===============================================================================

  B215 zajistila, že KDYŽ jádro data dá, uloží se pod jedinečným klíčem
  (žádná kolize mezi hrami se stejným názvem souboru).
  B216 zajišťuje, že jádro data VŮBEC DÁVÁ.
  Bez B216 by B215 ukládala pořád jen prázdno - obě opravy jsou nutné
  společně, ne alternativy.

===============================================================================
 OVĚŘENÍ
===============================================================================

  Zdroj problému dohledán přímo v kódu jádra (`libretro.c`,
  `load_memcards()`) a porovnán s appkou vlastní tabulkou core options
  (`nap_core_option_value()`) - není to odhad, je to konkrétní chybějící
  řádek v konkrétní tabulce, se stejným vzorem, jaký appka už úspěšně
  používá pro jiné options.

  `MCD_SIZE` ověřen přímo v `vendor/pcsx_rearmed/libpcsxcore/sio.h` -
  128 kB, autentická velikost.

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  { } v C++ souboru nezměněna (241/241 - přidané řádky jsou uvnitř už
  existujícího bloku, žádné nové závorky).

  CO NEJDE OVĚŘIT V TOMHLE PROSTŘEDÍ: appka je ARM64 Android binárka,
  spustit a reálně otestovat uložení/načtení jde jen na tvém telefonu.
  Diagnostika z B213 (`ps1MemCardInfo`, posílaná do /8765/log po každém
  SAVE GAME/MEMORY CARD stisku) teď ukáže `core_da_sram=ANO` místo `NE`,
  pokud oprava sedí - to je první věc, kterou zkontroluj v logu.

===============================================================================
 DODATEK: SKUTEČNÝ TEST, NE JEN "MĚLO BY" (na Reného žádost)
===============================================================================

  Rene: "rozhodne me nestaci melo by - ty si to otestuj nez to posles."

  Appku jako celek (ARM64 Android APK) v tomhle prostředí spustit
  nejde. Ale konkrétně TUHLE opravu jde otestovat jinak: vytáhl jsem
  SKUTEČNOU funkci `load_memcards()` přímo ze zdrojáku jádra
  (`vendor/pcsx_rearmed/frontend/libretro.c`, doslovně, ne přepsanou) a
  SKUTEČNOU `nap_core_option_value()` z appky (taky doslovně), spojil
  je přesně tak, jak se spojují za běhu (přes `environ_cb` volání), a
  zkompiloval jako samostatný program pro tenhle stroj (x86_64 Linux -
  jen pro TENHLE test, ne pro appku samotnou).

  Spustil jsem to DVAKRÁT - jednou se STAROU verzí (B215, bez opravy) a
  jednou s NOVOU (B216) - ať je vidět rozdíl, ne jen jedno číslo:

      STARÁ (B215):  memcard_type[0] = 0 (NONE)
                     retro_get_memory_data(SAVE_RAM) = NULL
                     retro_get_memory_size(SAVE_RAM) = 0 bajtů

      NOVÁ (B216):   memcard_type[0] = 3 (LIBRETRO)
                     retro_get_memory_data(SAVE_RAM) = ukazuje na Mcd1Data
                     retro_get_memory_size(SAVE_RAM) = 131072 bajtů (128 kB)

  Soubory testu (`test_memcard_wiring.cpp` + tři vytažené `_REAL.c`
  soubory se skutečným kódem jádra a appky) jsou přiložené v
  `test_overeni/` v tomhle balíčku, ať to jde komukoli zopakovat.

  CO TENHLE TEST DOKAZUJE: že cesta appka->jádro pro zapnutí memory
  karty je teď propojená správně - jádro dostane odpověď, kterou
  potřebuje, a nabídne appce skutečnou 128kB paměť.

  CO TENHLE TEST NEDOKAZUJE: jak se appka chová na tvém telefonu jako
  celek (grafika, dotyk, celá hra od začátku do konce). To je pořád na
  tobě - ale otázka "je cesta k zapnutí karty propojená správně" už
  není "mělo by být", je to změřené.

