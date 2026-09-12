# B263 — PS1 Doom: interlace nepomohl, cílená diagnostika místo dalšího hádání (versionCode 309)

Rene: "Co ten Doom a grafika v PS1?"

===============================================================================
 INTERLACE OPRAVA NEPOMOHLA
===============================================================================

Potvrdil jsi: "žádná změna Partáku v Doom furt špatná grafika." Moje
hypotéza z minula (appka nikdy nepovolí prokládaný režim) tedy **není
potvrzená** - Doom pravděpodobně skutečný prokládaný režim pro hlavní
hraní vůbec nepoužívá.

===============================================================================
 DALŠÍ DVĚ STOPY, KTERÉ JSEM PROVĚŘIL
===============================================================================

**Enhanced Resolution** (zdvojnásobení 3D geometrie) - má v kódu
rozsáhlou historii, kterou jsi sám už dřív potvrzoval jako funkční a
důležitou ("nedotknutelné"). Nesahal jsem na to bez tvrdých důkazů -
riskoval bych rozbití něčeho, co už prokazatelně funguje.

**Nesoulad šířky řádku (pitch)** - když se obrázek čte z paměti po
řádcích, "pitch" říká, kolik bajtů skutečně tvoří jeden řádek (může
být víc než šířka×barevná_hloubka kvůli zarovnání). Nesoulad by
způsobil přesně tenhle druh "posunutého" vzoru. Při čtení kódu to
vypadá správně (používá se skutečná hodnota pitch, ne odhad) - ale
bez skutečného běhu si tím nemůžu být jistý.

===============================================================================
 CO JSEM UDĚLAL MÍSTO DALŠÍHO HÁDÁNÍ
===============================================================================

Přidal jsem cílenou diagnostiku přesně na místo, kam snímek od
skutečně aktivního vykreslovače (gpu_neon) doopravdy přichází.
Zaznamenává se při KAŽDÉ ZMĚNĚ šířky/výšky/pitch/formátu obrazu - a
rovnou i OČEKÁVANÁ hodnota pitch (šířka × velikost pixelu), pro přímé
srovnání. Pokud se od sebe liší, hned to ukáže padding/nesoulad. Loguje
se jen při změně, ne každý snímek, ať log zůstane čitelný.

Žádná změna chování vykreslování - čistě přidání diagnostiky, nízké
riziko i bez možnosti to tady zkompilovat.

===============================================================================
 OVĚŘENÍ
===============================================================================

Ověřena syntaxe (existující makro `NAPDIAG`, existující konstanty
`PIXFMT_*`) a rovnováha závorek v celém souboru. Nejde zkompilovat
ani spustit OpenGL/Android řetězec v tomto sandboxu - stejné omezení
jako u všech PS1 změn dosud.

Další krok: tvůj test s tímhle logem ukáže, jestli se pitch/šířka/
výška během hraní Doomu někdy neshoduje s očekávanou hodnotou - a to
přesně ukáže, kam se dívat dál.
