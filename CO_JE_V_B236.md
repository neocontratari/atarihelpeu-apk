# B236 — ukládání pozice ve hrách Sega (versionCode 282)

Rene: "vim ze to sega jadro neumi - ale dalo by se vymyslet ukladani
her? Jako je treba Sonic? Osobne si myslim ze to asi nejde - ale ptam
se Partaku :-)"

**Ano, jde to.** A šlo to zjistit s jistotou, ne jen odhadem.

===============================================================================
 CO JSEM NAŠEL
===============================================================================

  Jádro appky (ClownMDEmu, `vendor/clownmdemu-core`) má ve své hlavičce
  (`clownmdemu.h`) VESTAVĚNOU dvojici funkcí:

      void ClownMDEmu_SaveState(const ClownMDEmu *clownmdemu, ClownMDEmu_StateBackup *backup);
      void ClownMDEmu_LoadState(ClownMDEmu *clownmdemu, const ClownMDEmu_StateBackup *backup);

  `ClownMDEmu_StateBackup` je kompletní snímek stavu jádra - procesor
  (68000 i Z80), obrazový čip (VDP), zvukové čipy (FM, PSG), i
  Mega CD část. Appka (`nap_sega_native_proof.cpp`) tuhle dvojici
  funkcí ale dosud VŮBEC nepoužívala.

  DŮLEŽITÉ ROZLIŠENÍ: tohle NENÍ oficiální baterií-zálohovaná SRAM
  uložená pozice, jakou má např. Sonic 3 (ta funguje jen v té jedné
  konkrétní hře a jen na místech, kde to hra sama dovolí přes svoje
  vlastní menu). Tohle je univerzální "snímek celého stavu" - přesně
  jako "save state" u emulátorů - funguje na KTEROUKOLI hru, KDEKOLI
  ve hře, kdykoliv se ti zachce. Ve výsledku je to spíš LEPŠÍ řešení
  než čekat, jestli konkrétní hra SRAM podporuje.

===============================================================================
 JAK JSEM TO OVĚŘIL - NE JEN "MĚLO BY FUNGOVAT"
===============================================================================

  Než jsem cokoli napsal do appky, zkopíroval jsem SKUTEČNOU hlavičku
  a všech 24 zdrojových souborů jádra + oba procesorové interpretery
  (68000, Z80) MIMO appku a zkusil je zkompilovat a slinkovat se svým
  navrhovaným voláním. Nejdřív selhalo linkování (chybějící soubory),
  po doplnění všeho potřebného se to POVEDLO ZKOMPILOVAT, SLINKOVAT A
  SPUSTIT:

      sizeof(ClownMDEmu) = 1132832 bajtů
      sizeof(ClownMDEmu_StateBackup) = 1132728 bajtů
      zapsáno 1132728 bajtů do souboru
      přečteno zpět 1132728 bajtů
      VÝSLEDEK: TYPY A VOLÁNÍ SEDÍ, SERIALIZACE FUNGUJE

  Tohle je skutečný, spuštěný test proti reálnému zdrojovému kódu
  jádra - ne odhad, že by to mělo fungovat.

===============================================================================
 KRITICKÁ BEZPEČNOSTNÍ POZNÁMKA - VELIKOST STRUKTURY
===============================================================================

  `ClownMDEmu_StateBackup` je stejně velká jako hlavní `ClownMDEmu`
  struktura (>1 MB) - a appka už měla u `NapRealCoreState`/`ClownMDEmu`
  výslovné varování: "can overflow Android/WebView thread stack".
  Proto je nový buffer (`g_sega_state_backup`) STATICKÁ/GLOBÁLNÍ
  proměnná, NIKDY lokální proměnná uvnitř funkce - stejná opatrnost,
  jakou appka už měla pro `g_real` samotné.

===============================================================================
 CO PŘIBYLO
===============================================================================

  C++ (`nap_sega_native_proof.cpp`): `saveState`/`loadState` JNI
  funkce - zamykají `g_real_mutex` (stejný zámek jako zbytek jádra,
  proti souběhu s emulačním vláknem), kontrolují že běží hra, uloží/
  načtou celou strukturu přes `fwrite`/`fread`.

  Java (`NativeSegaCoreBridge.java` + `MainActivity.java`):
  `segaSaveState()`/`segaLoadState()` - appce-privátní JEDEN společný
  slot (`sega_saves/sega_quicksave.state`) - appka pro Segu zatím
  nemá žádnou knihovnu her jako PS1 (od B214), takže "jeden slot na
  hru najednou" je přirozený první krok. Kdyby chtěl něco jako
  knihovnu s uloženými pozicemi PRO KAŽDOU hru zvlášť, je to navazující
  krok, ne teď.

  JS (`emu_sega/index.html`): dvě nová tlačítka v D-PAD panelu -
  "ULOŽIT POZICI VE HŘE" / "NAČÍST ULOŽENOU POZICI".

===============================================================================
 DROBNÉ ZJIŠTĚNÍ MIMOCHODEM
===============================================================================

  Sega má `#toast` element v HTML, ale funkce `toast()` ho vůbec
  nepoužívá - jen zapisuje do logu (`pushLog`), na rozdíl od PS1, kde
  toast skutečně ukazuje bublinu na obrazovce. To je stav appky ZE
  DŘÍVĚJŠKA, ne něco, co touhle změnou vzniklo - nechal jsem to beze
  změny, ať se to neplete s jinou úpravou. Pokud bys chtěl i v Segu
  vizuální bublinu jako u PS1, řekni, je to malá, samostatná úprava.

===============================================================================
 OVĚŘENÍ
===============================================================================

  Skutečná kompilace+běh proti reálnému zdrojovému kódu jádra (viz
  výše) - nejsilnější možné ověření bez přímo NDK/Android.

  node --check - 0 chyb. Žádná duplicitní id (55). `stranka_kontrola.py`
  (TV cesta) - 0 chyb. Rovnováha { } v MainActivity.java - 2379/2379.
  Rovnováha { }/() v `nap_sega_native_proof.cpp` - 237/237, 1252/1252.

  Obě existující Sega jsdom simulace (layout, nebula) spuštěny znovu -
  beze změny prošly.

  Nová simulace (test_sega_savestate.js): tlačítka ULOŽIT/NAČÍST
  existují v D-PAD panelu, klik na každé správně zavolá odpovídající
  `AHSega` metodu.

  CO NEJDE OVĚŘIT ODSUD: skutečné volání přes JNI/Android (potřebuje
  reálné zařízení + spuštěnou hru) - to je na tvém testu. Doporučený
  postup: spusť libovolnou hru, chvíli hraj, klikni ULOŽIT POZICI VE
  HŘE, udělej v ní kus cesty dál, klikni NAČÍST ULOŽENOU POZICI - měl
  by ses vrátit přesně tam, kde jsi uložil.
