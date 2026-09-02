# B212 — portrait bez obrázku ovladače, D-pad graficky lepší (versionCode 258)

Rene poslal screenshot portrait obrazovky a napsal: "obrázek dalších
tlačítek na výšku nechci... vypadá to hnusně a nemá to tam co dělat!
Nastavení a ovládání D-padu bude jen na mobilu na šířku... Nastavení
D-padu udělej graficky lepší, ať je to hodno PS1, třeba ve stylu BIOS
PS1... ať se dá nastavit i velikost jen samostatného kolečka ovladače."
Log potvrdil: LAYOUT_MIRROR_TAP z B211 teď funguje (viz níže).

===============================================================================
 1) LEVÁK - POTVRZENO OPRAVENO
===============================================================================

  V logu z tohoto testu je "LAYOUT_MIRROR_TAP" hned následované úspěšným
  "LAYOUT_MIRROR" - čtyřikrát za sebou. Tlačítko funguje. Diagnostický
  řádek jsem nechal - nezavazí a příště hned uvidíme, jestli se to
  náhodou nevrátí.

===============================================================================
 2) OBRÁZEK OVLADAČE V PORTRAIT - ODSTRANĚN Z GRAFIKY
===============================================================================

  `ps1_final_screen.png` byl jeden plochý obrázek 941×1672 px se vším -
  rámem monitoru, MAIN MENU/MEMORY CARD/CD PLAYER dekorací, FOTOGRAFICKÝM
  obrázkem PS1 ovladače (D-pad, Sony logo, Select/Start, akční tlačítka)
  a řadou RESET/MEMORY/CD/LOAD/SAVE ikon úplně dole.

  Vyříznul jsem pásmo 895-1320 px (ovladač + plošina pod ním) a zbytek
  slepil dohromady - nový obrázek má 941×1247 px. Přechod mezi rámem
  monitoru a řadou ikon je čistý, bez viditelného švu (obě strany řezu
  jsou tmavé hvězdné pozadí).

  Původní obrázek je zálohovaný (ne v balíčku pro appku, jen pro mě).

  Navazující přepočet CSS (jinak by appka natahovala nesprávný poměr
  stran a všechna tlačítka by seděla jinde, než kde teď opravdu je
  odpovídající grafika):
    `.stage{aspect-ratio:941/1247}` (bylo 941/1672)
    btnBack/btnSettings/psMonitor - nová pozice přepočtená škálovacím
      faktorem 1672/1247 (byly PŘED řezem, jejich pixel pozice se
      nezměnila, jen se mění % vůči nové kratší celkové výšce)
    btnReset/Memory/Cd/Load/Save - nová pozice = (stará pozice v px -
      425 px odříznutého pásma) / 1247 * 100

===============================================================================
 3) D-PAD/AKČNÍ TLAČÍTKA/RAMENA/SELECT/START - TEĎ I FUNKČNĚ MIMO PORTRAIT
===============================================================================

  Nejen graficky pryč - "Nastavení a ovládání D-padu bude jen na mobilu
  na šířku" - přidáno do portrait `display:none`: #dpad, čtyři akční
  tlačítka, L1/L2/R1/R2 (ty už tam byly), Select, Start. Portrait je teď
  čistě menu obrazovka (Back, Settings, Monitor, Reset/Memory/CD/Load/
  Save) - žádný herní ovladač.

===============================================================================
 4) D-PAD A AKČNÍ TLAČÍTKA - PS1 BIOS STYL
===============================================================================

  `.padBtn` (landscape) přepracován na tmavý střed + bronzový prstenec +
  stříbrný lem - stejný jazyk jako mají RESET/MEMORY CARD/CD ikony ve
  skinu, místo ploché poloprůhledné bubliny.

  D-pad: místo znaku "✚" (kříž) teď "●" (malý plný knoflík), menší
  písmo, ať vypadá jako střed ovladače, ne jako popisek.

===============================================================================
 5) SAMOSTATNÁ VELIKOST D-PADU
===============================================================================

  Nový posuvník "Velikost D-padu" v panelu OVLÁDÁNÍ, vedle stávající
  "Velikost tlačítek" (ta teď popsaná jako Triangle/Circle/Cross/Square +
  ramena + Select/Start - beze změny funkce, jen jasnější popisek).

  Nová CSS proměnná `--napDpadScale`, nezávislá na `--napBtnScale`.
  `#dpad.ctrlAdjustable` má vyšší specificitu než holé `.ctrlAdjustable`,
  takže se D-pad řídí svou vlastní velikostí, zatímco ostatní tlačítka tou
  sdílenou. Průhlednost zůstává společná pro všechny (nebyl důvod ji dělit).

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (62 zkontrolováno).
  `stranka_kontrola.py` na TV cestu - 0 chyb.

  Obrázek: vizuálně zkontrolován před i po úpravě (nástroj pro zobrazení
  obrázku) - přechod čistý, žádný viditelný šev, žádný ořezaný text.

  Nová jsdom simulace (test_ps1_dpad_size.js):
    - výchozí --napBtnScale i --napDpadScale = 1.00
    - změna JEN velikosti D-padu na 150 % nechá --napBtnScale beze změny
    - změna JEN velikosti ostatních tlačítek na 80 % nechá --napDpadScale
      beze změny (obě jsou opravdu nezávislé, ne jen graficky oddělené
      popisky nad jedním sdíleným posuvníkem)
    - uložení a nové načtení stránky obě hodnoty věrně obnoví
    - portrait `display:none` pravidlo pro D-pad/akce/ramena/Select/Start
      přítomné v souboru
