# B201 — PS1: PŘESOUVATELNÉ ROZLOŽENÍ TLAČÍTEK (versionCode 247)

Zadání od Reného: "na kolečku nastavení v pravém horním rohu uděláš další
odbočku na D-pad - jakékoli tlačítko si bude moct uživatel dát na
obrazovku, kam chce, včetně hlavního ovladače, a dotykové to bude fungovat
přesně tam, kde si to uživatel přednastaví."

Týká se `emu_ps1/index.html`. Nic jinde v appce se nezměnilo.

===============================================================================
 CO PŘIBYLO
===============================================================================

  1. V SETTINGS panelu (tlačítko v pravém horním rohu) nové tlačítko
     "UPRAVIT ROZLOŽENÍ TLAČÍTEK".

  2. Po kliknutí se panel zavře a appka přejde do editačního režimu:
       - všech 15 dotykových prvků (BACK, SETTINGS, D-pad, akční tlačítka,
         L1/L2/R1/R2, SELECT, START, RESET, MEMORY CARD, CD ISO, LOAD GAME,
         SAVE GAME) dostane žlutý přerušovaný rámeček a jde je chytit prstem
       - dole se objeví lišta se dvěma tlačítky: VÝCHOZÍ a HOTOVO

  3. Tažením se tlačítko přesune kamkoli po ploše (procentuálně vůči
     obrazovce, takže to sedí na libovolné velikosti telefonu). Nejde ho
     přetáhnout úplně mimo plochu - vždycky zůstane aspoň kousek uvnitř,
     aby na něj šlo znovu sáhnout.

  4. HOTOVO uloží aktuální pozice do `localStorage` a vrátí normální hru
     (appka to tam už dělá jinde - `napPs1LastGame`, `napAvd`, `napFillV3` -
     takže nejde o nový mechanismus, jen o další klíč).

  5. VÝCHOZÍ vrátí všechna tlačítka na původní místa z CSS a zůstane
     v editačním režimu, aby šlo hned zkusit rozestavět znovu.

  6. Při dalším spuštění appky (i po zavření a znovu otevření PS1
     obrazovky) se uložené rozložení samo natáhne.

  7. Portrait a landscape mají v appce úplně jiné výchozí pozice tlačítek
     (jiný CSS blok), takže se ukládají pod dva různé klíče
     (`napPs1Layout_portrait` / `napPs1Layout_landscape`) a otočení
     telefonu samo přepne na tu druhou sadu.

===============================================================================
 DŮLEŽITÉ OMEZENÍ - ZATÍM JEN PORTRAIT
===============================================================================

  V landscape (appka otočená na šířku, tak, jak se PS1 skutečně hraje) je
  tlačítko SETTINGS schované (`#btnSettings{display:none}` - bylo to tak
  už předtím, kvůli přehlednosti při hraní). Bez SETTINGS se nedá spustit
  editační režim. Takže: rozložení pro portrait si uživatel upraví normálně;
  rozložení pro landscape zatím upravit nejde, protože se k tomu nedostane.

  Řešení (přístupný vstup do editace i v landscape) jsem nedělal bez
  zeptání - je to zásah do rozhraní při hraní, ne jen přidání funkce.

===============================================================================
 JAK JSEM TO OVĚŘIL
===============================================================================

  `node --check` na výsledný soubor - 0 chyb.

  Navíc jsem si appku skutečně "rozjel" v jsdom (Node.js knihovna, co umí
  přehrát HTML/JS mimo prohlížeč) a proklikal celý scénář jako uživatel:
    - SETTINGS -> UPRAVIT ROZLOŽENÍ TLAČÍTEK -> panel se zavře, lišta se
      zobrazí, tlačítka dostanou zvýraznění
    - tažení D-padu o kus doprava a dolů -> pozice (v %) se změnila přesně
      o odpovídající kus
    - PROVĚŘENO, že během tažení v editačním režimu appka NEPOŠLE do PS1
      jádra ani jeden stisk (0 volání `AHPS1.ps1Input`) - jinak by se při
      přerovnávání tlačítek klidně mohla rozjet postava ve hře
    - VÝCHOZÍ vrátilo tlačítko přesně na CSS pozici
    - nové tažení + HOTOVO uložilo do localStorage přesně tu pozici, kde
      tlačítko doopravdy bylo (ne nějakou starou/výchozí)
    - NOVÉ načtení stránky (simulace zavření a znovuotevření PS1 obrazovky)
      pozici ze storage samo natáhlo a tlačítko bylo tam, kam ho uživatel
      dal

  Test skript held v `/home/claude/work/test_ps1_layout.js` v tomhle
  prostředí, do balíčku pro appku nejde (Node.js nástroj, ne assets) - je
  jen pro záznam, jak byla funkce ověřená.
