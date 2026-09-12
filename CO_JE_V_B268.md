# B268 — Poctivě: dvě věci opravené, čtyři zůstávají (versionCode 314)

Nahlásil jsi najednou hodně věcí. Musím být na rovinu: **v tomhle kole
jsem vyřešil jen dvě z nich.** Zbytek zůstává - nechci předstírat víc,
než je hotovo.

===============================================================================
 CO ZŮSTÁVÁ NEVYŘEŠENO (upřímně, hned na začátku)
===============================================================================

- **Attract mode** (změna barev po nečinnosti) - ověřil jsem, že se
  **opravdu nikdy nespustí** (test 10 simulovaných minut bez jediné
  klávesy, barva pozadí se ani jednou nezměnila). Skutečný mechanismus
  v ROM zatím nemám dohledaný - potřebuje další práci.
- **CSAVE dvojité pípnutí** (místo jednoho) - nevyřešeno.
- **Závěrečný "chroustavý" WAV zvuk programu** po tónu - nevyřešeno.
- **CLOAD ERROR 138** po timeoutu - nevyřešeno.

===============================================================================
 CO JE OPRAVENO - A JE TO JEDNA SPOLEČNÁ PŘÍČINA DVOU HLÁŠENÍ
===============================================================================

Boot/reset zvuk obráceně a nesmyslné kliknutí po self-testu měly
**stejnou kořenovou příčinu**.

Kdykoli appka posune spoustu snímků najednou (boot=600, self-test=400,
napsání textu=60, reset=60), aniž by mezitím generovala zvuk (ten
začne generovat až POTOM, v hlavní smyčce), tak se za tu dobu
nahromadí spousta zaznamenaných přepnutí reproduktoru. Jakmile pak
generování zvuku konečně naběhne, snaží se to všechno narvat na
úplný začátek - buď to zní jako krátký "zmáčknutý" zvuk (místo
rozloženého dlouhého), nebo naopak jako nesmyslně vysoký počet
kliknutí najednou.

**Ověřil jsem to přímo:** zreprodukoval jsem přesně tu samou
posloupnost, jakou dělá appka při vstupu do self-testu - před opravou
se nahromadilo 64 přepnutí (přesně odpovídá tvému hlášení "67
kliknutí"), po opravě přesně 0.

Oprava: po každém takovém velkém bloku snímků appka teď srovná
sledování zvuku - zahodí, co se nahromadilo, a srovná časovou základnu
s aktuálním stavem procesoru. Normálního běhu (jeden snímek po druhém,
jak appka běží většinu času) se to netýká vůbec.

===============================================================================
 CO TO NEZNAMENÁ
===============================================================================

Tahle oprava **neznamená**, že boot teď zní přesně jako na skutečném
Atari (dlouhý zvuk s náznakem obrazovky, pak READY). To by vyžadovalo
přehrávat zvuk průběžně i během samotného bootování, ne až po něm -
to je větší přestavba. Tahle oprava jen odstraňuje ty vyloženě
špatné, "zmáčknuté" nebo uměle vysoké artefakty, co vznikaly popsanou
chybou.

===============================================================================
 OVĚŘENÍ
===============================================================================

Všech 10 předchozích testů prošlo beze změny. Nový cílený test
potvrzuje opravu (64 → 0 nahromaděných přechodů při vstupu do
self-testu). Celý `nap_atari_native.cpp` znovu zkompilován čistě.
