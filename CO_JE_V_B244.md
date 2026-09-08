# B244 — kontrola souborů při startu si teď dá druhou šanci (versionCode 290)

Rene si všiml přesného a klíčového detailu: "intro najede přesně jak
má, i core Segy a core PS1, i když sem dal NESTAHNOUT - takže jedna
část appky soubory najde bez problémů, ale ta PRVNÍ kontrola je
nenajde." A poslal screenshot správce souborů, který potvrdil, že
soubory na disku fyzicky BYLY (`emu/sega`, `PS1_BIOS` obě existovaly).

===============================================================================
 CO TO ZNAMENÁ
===============================================================================

  B242 a B243 opravily ROZHODOVACÍ logiku (spoléhat na skutečnou
  přítomnost souborů, ne na appce-privátní vzpomínku) - to bylo
  správně. Ale René svým postřehem odhalil, že i SAMOTNÁ kontrola
  ("jsou tam soubory?") občas selhává - ne proto, že by soubory
  chyběly, ale kvůli ČASOVÁNÍ.

  Appka volá tuhle kontrolu SYNCHRONNĚ v `onCreate()` - tedy hned na
  samém začátku, za studena. Intro naproti tomu spouští `spustSegu()`
  (stejnou funkci, na stejný adresář) až o pár vteřin POZDĚJI, poté,
  co už proběhlo pár kroků intra.

  Tohle přesně odpovídá známému chování Androidu: hned po studeném
  startu appky (obzvlášť po čerstvé instalaci) může být veřejné
  úložiště ještě "dobíhající" a `File.listFiles()` chvíli vrací
  prázdno, i když soubory na disku fyzicky existují - do doby, kdy
  intro dorazí ke svému dalšímu kroku, už to stihne být v pořádku.

===============================================================================
 OPRAVA
===============================================================================

  Když první pokus najde "chybí", appka to teď NEVZDÁ hned. Krátce
  počká (700ms, pak 1400ms, maximálně dvě opakování) a zkusí to znovu
  - přesně tak dlouho, aby to mělo stejnou šanci jako ta pozdější
  kontrola v intru. Pokud soubory OPRAVDU chybí (tři pokusy po sobě
  je nenajdou), appka se normálně zeptá - žádné nekonečné čekání.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check (obě emu stránky - nedotčeny), `stranka_kontrola.py`
  (TV cesta) - 0 chyb. Rovnováha `{ }` v MainActivity.java - 2460/2460
  (při přestavbě na samostatnou metodu jsem si na chvíli spletl počet
  závorek - dvakrát zkontrolováno a opraveno, než šlo dál).

  Izolovaná simulace kontrolní logiky (`test_startup_retry_logic.js`)
  - tři scénáře:
    - soubory jsou hned napoprvé → žádné zpoždění, rychlá cesta
    - první pokus řekne "chybí" (úložiště ještě neběží), druhý pokus
      je najde → NEPTÁ se (přesně Reneho případ)
    - soubory opravdu chybí (všechny 3 pokusy) → zeptá se, žádné
      zacyklení

  Všech dvanáct existujících Sega/PS1 jsdom simulací spuštěno znovu -
  beze změny prošly (čistě Java strana, JS logika nedotčena).

  CO NEJDE OVĚŘIT ODSUD: jestli tohle doopravdy vyřeší situaci na
  tvém konkrétním zařízení - to je na tvém testu. Dávám tomu vysokou
  důvěru, protože mechanismus přesně kopíruje to, co už empiricky
  funguje u intra (stejná kontrola, jen s trochou času navíc).
