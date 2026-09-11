# B260 — Další moje regrese, tentokrát chycená srovnávacím testem (versionCode 306)

Rene: "Atari test - atari nejde nabootovat. Modrá obrazovka se
čtverečkem a neustále se opakující zvuk."

===============================================================================
 CO SE STALO
===============================================================================

B258 přidal dvě věci najednou:
1. POKEY časovače 1/2/4 (bezpečné, potřebné pro odblokování CSAVE)
2. "Okamžitá připravenost" - když program povolí přerušení "sériový
   výstup potřebuje bajt" zatímco je port v klidu, appka hned
   signalizovala "připraveno" (RIZIKOVÉ - a jak se ukázalo, špatně)

Zreprodukoval jsem přesně tvou situaci - čistý boot, bez jakéhokoli
CSAVE - a potvrdil jsem: ta druhá věc (2) způsobuje, že i během
ÚPLNĚ NORMÁLNÍHO bootu (ne jen CSAVE) dostane OS neočekávaný signál
přerušení, odkloní se z normální klidové smyčky (kde má být ticho) do
jiné oblasti kódu, kde zůstane hrát pořád stejný tón. Přesně to, co
jsi viděl.

===============================================================================
 JAK JSEM TO TENTOKRÁT OVĚŘIL DŮKLADNĚJI
===============================================================================

Udělal jsem srovnávací test ve třech variantách:
- **Před B258** (žádná ze dvou změn)
- **Jen s časovači** (změna 1, bez změny 2)
- **S oběma změnami** (přesně to, co jsi testoval a co selhalo)

Výsledek byl jednoznačný:
- Bez jakékoli B258 změny: PC se pohybuje jen v malém rozmezí
  ($F302-$F310), naprosté ticho - to je ten SPRÁVNÝ, fungující stav
- Se samotnými časovači (bez rizikové části): **naprosto identické**
  chování jako "před B258" - a CSAVE se přesto do ~3500 snímků
  spolehlivě vrátí do stejné normální klidové smyčky, se stejným
  tichem
- S oběma změnami (co jsi dostal): PC uteče do úplně jiné oblasti
  kódu, zvuk hraje pořád - přesně tvůj popis

To znamená: **samotné časovače stačí** na to, aby se CSAVE odblokoval
- ta riziková druhá část vůbec nebyla potřeba, a navíc kazila normální
boot.

===============================================================================
 OPRAVA
===============================================================================

Riziková část (2) odstraněna. Zůstávají jen bezpečně ověřené
časovače (1).

===============================================================================
 OVĚŘENÍ
===============================================================================

Celý konsolidovaný test (6 kontrol) - jedna z nich dřív testovala
přesně tu teď odstraněnou vlastnost, aktualizována tak, aby ověřovala
její NEPŘÍTOMNOST (žádný vedlejší efekt při zápisu IRQEN). Boot test
dává bit-identický výsledek s tím, co fungovalo předtím. CSAVE
reprodukce potvrzuje, že se stále spolehlivě odblokuje.

Celý `nap_atari_native.cpp` znovu zkompilován čistě.

===============================================================================
 PS1 - MEZITÍM
===============================================================================

V logu, který jsi poslal z Doomu, se moje nová diagnostika
(BUILD2SB66, změna prokládaného bitu) **vůbec neobjevila** - to
znamená, že hra prokládaný režim v tomhle testu vůbec nezapnula. Moje
hypotéza "appka prokládaný režim ignoruje" tedy touhle zkouškou není
potvrzená - potřebuje to další vyšetřování. Všiml jsem si ale, že se
zdrojové rozlišení obrazu během hraní mění (640×480, 320×240,
512×480) - možná souvisí s obecným přepínáním rozlišení, ne
konkrétně s prokládaným bitem. Budu v tom pokračovat.
