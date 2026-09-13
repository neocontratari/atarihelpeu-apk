# B275 — Přenos dat CSAVE trvá reálný čas, ne 0,3 vteřiny

## Poctivé přiznání z minulého kola

V minulém kole jsem nejdřív implementoval opravu (spouštěč přerušení
pro sériový výstup), která se nakonec ukázala jako **zbytečná** —
appka do správné obslužné rutiny dojde sama, přirozeně, jen jí to
trvá přes 20 vteřin (čeká na jiný, nesouvisející časovač). Tuhle
opravu jsem **odstranil** — je lepší žádná změna než zbytečná, co
riskuje novou chybu.

## Skutečná příčina, nalezená přímým srovnáním s referencí

Appka data SPRÁVNĚ posílá — přímo změřeno 701 skutečných zápisů
bajtů. Ale zpoždění mezi jednotlivými bajty bylo **až 140× rychlejší**,
než má být. Reference (JS) používá pro tohle dva nezávislé čítače:

- 10 řádků obrazovky do signálu "výstupní registr prázdný, dej další"
- 30 řádků obrazovky do signálu "celý bajt odeslán"

Já jsem měl jen **2 řádky** na celý cyklus. Proto se celý blok dat
(701 bajtů) odeslal za pouhých 0,3 vteřiny místo realistických ~30
vteřin — zvuk pak zněl jako jeden nerozeznatelný impuls na konci
dlouhého tónu, ne jako postupné "chrčení".

## Oprava

Nahradil jsem svůj předchozí jednoduchý mechanismus přesně tím, co
používá reference — dva nezávislé čítače, každý snížený o jedna
každý řádek obrazovky, přesně podle vzoru z JS kódu.

## Ověření

- Celý přenos (701 bajtů) teď trvá ~31,8 vteřiny (~45ms/bajt — řádově
  správně oproti očekávaným ~18ms/bajt při 600 baudech)
- Všech 9 předchozích testů prochází beze změny
- CLOAD pořád správně končí ERROR 138 (CLOAD se týhle změny vůbec
  nedotýká — používá jiný registr, SERIN, ne SEROUT)

## Co zůstává k ověření

Přesný rytmus a charakter zvuku (zní to jako skutečné "chrčení"?)
musí potvrdit test na skutečném 130XE — CLI test může ověřit časování
a logiku, ale ne jak to doopravdy zní.
