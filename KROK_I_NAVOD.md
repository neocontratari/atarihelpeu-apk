# KROK I — jedno sáhnutí do jádra místo dvou

## Nejdřív uvedení na pravou míru

Napsal jsem "snížená priorita vykreslování" a tys to pochopil jako
zhoršení obrazu. **Není to tak** — priorita nemá s kvalitou obrazu nic
společného. Obraz je pixel po pixelu úplně stejný, priorita jen říká,
kdo jde první ke slovu, když se procesor pere o dvě věci naráz.
Nic se nezahazuje.

**Ale máš pravdu, že to nebyla ta správná oprava.** Vrátil jsem ji.

## Co jsem v logu přehlédl a teď našel

`loops=61080, underrunLoops=28647` — **ve 47 % případů, kdy si zvuk
přišel pro data, žádná nebyla.** Jádro je nestíhalo vyrobit.

Důvod: **sahali jsme si pro obraz dvakrát.** Jednou pro displej telefonu
a podruhé pro televizi — dvě plné kopie celého obrazu, mnohokrát za
vteřinu. Jádro pak nemělo kdy dělat zvuk.

## Oprava — bez jakékoliv ztráty kvality

Teď se **sáhne jednou a použije dvakrát**: obrazovka telefonu si obraz
vytáhne a přenos na televizi si ho jen půjčí. Stejný obraz, stejná
ostrost, jen o polovinu míň dřiny pro jádro.

Tohle je přesně ta "samostatná cesta", o kterou ti šlo — jen z opačné
strany: místo aby zvuk dostal vlastní linku, uvolníme mu místo tím, že
obraz přestane zabírat dvakrát.

## Verze: EMU10-I-SDILENY-SNIMEK

## Co mi pošli

1. **`underruns=...`** — mělo by výrazně klesnout (bylo 757, pak 428)
2. **`underrunLoops=...` a `loops=...`** — poměr by měl spadnout hluboko
   pod těch 47 %
3. Řádek `G TV: ... FPS`
4. Je zvuk čistší a míň rozejetý?
5. Vypadá obraz pořád stejně dobře? (musí — nic jsme na něm neměnili)

Když underruny nespadnou k nule, teprve pak pustíme zvuk vlastní cestou
z jádra. Ale nejdřív ať zvuk vůbec má z čeho brát.
