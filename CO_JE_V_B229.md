# B229 — skutečná příčina "reset na už načtený BIOS" (versionCode 275)

Rene upřesnil B228: "tam nebyl dvojitý stisk - tam nebyla chyba. Pokud
byl bios bez cd a ten se nacetl - tak druhy udelal chybu. tj nacteny
bios bez cd, dalsi udelal chybu."

Takže: BIOS bez CD se v pořádku načte. Zmáčkneš RESET PODRUHÉ (ne
rychle za sebou - klidně s odstupem, BIOS už běží) - a TEN konkrétní
reset selže. B228 řešilo špatný scénář (rychlý dvojklik během startu).

===============================================================================
 SKUTEČNÁ PŘÍČINA
===============================================================================

  `stopPs1SessionHard()` (funkce, kterou `ps1Reset()` volá jako první
  krok) v sobě už měla starší opravu (komentář `BUILD2SA36`) - dřív se
  při běžícím BIOSu (bez hry) rovnou vrátila beze změny
  (`PS1_ALREADY_STOPPED`), protože její podmínka "běží něco?" na BIOS
  nemyslela. To bylo opraveno přidáním `ps1BiosRunning` do té
  podmínky - funkce teď SPRÁVNĚ POKRAČUJE a skutečně zastaví jádro
  (`NativePs1CoreBridge.stopSafe()`).

  ALE: přidání do PODMÍNKY nestačí - funkce nikde dál v těle
  NIKDY nenastavila `ps1BiosRunning` zpátky na `false`! Jádro se tedy
  doopravdy zastavilo (proto ten zvuk/blik při zastavení, co Rene
  slyšel), ale příznak "BIOS běží" zůstal ve stavu appky nesprávně
  `true`.

  Když `ps1Reset()` hned po tomhle zavolala `ps1MaybeStartBios()`, její
  vlastní ochranná podmínka `if (ps1BiosRunning || ps1BiosStarting)
  return;` viděla ten STARÝ, už neplatný příznak - a nový start se
  vůbec nezačal. Výsledek: jádro zastavené, nic nového nenaběhlo,
  žádná grafika.

===============================================================================
 OPRAVA
===============================================================================

  `ps1BiosRunning = false;` přidáno přímo do `stopPs1SessionHard()`,
  vedle už existujících `ps1BootActive = false; ps1SessionActive =
  false;`. Pro úplnost přidán i `ps1GameWindowOwnsCore = false;` -
  stejná logika (funkce, co "zastaví úplně všechno", by měla vynulovat
  VŠECHNY příznaky vlastnictví/běhu jádra, ne jen některé).

  B228 pojistka proti rychlému dvojitému stisku ZŮSTÁVÁ - řeší JINÝ,
  taky reálný scénář (dvojklik během ještě neskončeného startu), obě
  opravy se doplňují, ne nahrazují.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check, `stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha
  { } v MainActivity.java - 2365/2365 (beze změny počtu - přidané
  řádky jsou uvnitř už existujícího bloku).

  CO NEJDE OVĚŘIT ODSUD: tohle je čistě Java stavová logika (dva
  booleany), bez JS dopadu - nejde postavit jsdom simulaci, protože
  celý native boot běh (retro_load_game, EGL, vlákna) tady neběží.
  Ověřeno ruční kontrolou kódu - `ps1BiosRunning` se teď nastavuje na
  `false` na STEJNÉM místě, kde uz to delaji `ps1BootActive`/
  `ps1SessionActive` (stejny vzor, ne novy). Skutecne overeni je jen
  na tvem telefonu - presny test: nech nabootovat BIOS bez CD, pockej
  az se ustali, pak zmackni RESET (bez spechu, ne dvojklik) - mel by
  se BIOS znovu cistě nabootovat.
