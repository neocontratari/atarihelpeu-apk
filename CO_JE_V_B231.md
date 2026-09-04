# B231 — planetky lítají, ne jen kymácí (versionCode 277)

Rene: "je to super - ale neboj se, S8 Samsung zvládne víc, klidně i ty
velké balónky ať lítají, dej si záležet." A potvrdil, že tučné Arial
místo Google Fontů "sedí" - takže tenhle build se týká JEN pohybu.

===============================================================================
 CO SE ZMĚNILO
===============================================================================

  Planetky (`panelPlaneta`) dřív jen jemně kymácely nahoru/dolů (10px
  rozsah). Teď opravdu LÉTAJÍ napříč velkou částí obrazovky v plynulé
  smyčce (60-100px rozsah, 15-26 vteřin na jeden cyklus, různé dráhy
  pro každou), a mírně mění velikost cestou (jemné `scale`), ať to
  vypadá jako let ve třech rozměrech, ne jen posun stranou.

  Přidány dvě nové planetky (5 celkem) a příležitostný meteor - proletí
  obrazovkou jednou za 9 vteřin s krátkou zářící stopou.

===============================================================================
 VÝKONNOSTNÍ POZNÁMKA
===============================================================================

  Pořád výhradně `transform`/`opacity` (GPU vrstva, žádný reflow) -
  jen VĚTŠÍ rozsah pohybu, ne nový druh nákladu na výkon. Samsung
  Galaxy S8 (Snapdragon 835/Exynos 8895, Adreno 540/Mali-G71) je na
  tohle výrazně předimenzovaný - těch pár desítek malých, GPU-
  akcelerovaných vrstev by nemělo být vůbec znát, ani na starším
  telefonu, natožpak na S8.

===============================================================================
 OVĚŘENÍ
===============================================================================

  node --check - 0 chyb. Žádná duplicitní id (78). `stranka_kontrola.py`
  (TV cesta) - 0 chyb.

  Všech sedm existujících jsdom simulací (plocha, odpočet, CD/ISO
  přehození, indikátor v monitoru, RESET, dvojitý stisk RESET, pozadí/
  značky) spuštěno znovu po týhle úpravě - beze změny prošly, žádná
  funkční regrese - tahle změna se týkala jen CSS animací, ne žádné
  logiky.

  CO NEJDE OVĚŘIT ODSUD: skutečnou plynulost na tvém konkrétním
  telefonu - to je čistě na tobě.
