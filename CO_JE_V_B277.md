# B277 — Datový tok CSAVE teď trvá reálný čas (~4-5s)

## Poctivé přiznání

Omlouvám se za opakované zmiňování testu na skutečném Atari — bylo to
zbytečné a matoucí. Testuješ appku přímo, na mobilu, a to je přesně
správný způsob. Do tohohle přístupu se už nebudu vracet.

## Co jsem zjistil o dlouhém čekání (~18s)

To dlouhé čekání před samotnými daty **není chyba** — je to autentická
hodnota přímo z originálního OS ROM (appka si ji vyčte z tabulky podle
čísla kanálu). 17,84 vteřiny sedí s tvým popisem "~20s pískání".

## Skutečná chyba — datový tok

Tady byl problém: appka dostávala signál "připraveno na další bajt"
mnohem dřív, než starý bajt stihl doopravdy dohrát svých 10 bitů
zvuku (10 bitů při 600 baudech = 16,67ms, ale appka čekala jen
~2ms). Každý nový bajt tak přepsal rozehraný předchozí — celých 264
bajtů dat se namačkalo do necelé vteřiny místo správných ~4-5 vteřin.

## Oprava

Přepočítal jsem časování na skutečnou dobu potřebnou pro 10 bitů při
600 baudech (~260 řádků obrazovky místo předchozích 30). Cestou jsem
narazil na regresi (appka se na chvíli zasekla) — způsobenou tím, že
oba signály "připraveno na další" a "celý přenos hotov" vystřelily ve
STEJNÝ okamžik. Opravil jsem to tak, aby byly mírně odlišné, přesně
jak varoval starší komentář v kódu z dřívějška.

## Ověření

- Datový tok (264 bajtů) teď trvá **4,86 vteřiny** — přesně sedí s
  tvým popisem "~4-5s"
- Průměr ~18,4ms na bajt, blízko teoretickým 16,67ms
- Obrazovka správně končí na READY (ne rozbitá)
- Dvoutónové kódování (poměr frekvencí 1,91) beze změny
- CLOAD pořád správně končí ERROR 138 — beze změny
- Všech 9 předchozích testů prochází
