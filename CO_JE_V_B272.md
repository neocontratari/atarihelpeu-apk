# B272 — Obraz a zvuk teď postupují spolu, žádné dohánění

## Skutečná příčina zpoždění 5–15 vteřin

Moje předchozí oprava (B271) vyřešila "zvuk se zahazuje" tak, že
začala zvuk průběžně zachytávat během psaní příkazů, vstupu do
self-testu a bootování. Jenže appka pořád dělala **celý blok** (stovky
snímků) bleskurychle v jednom volání a hned vrátila **hotový obraz**.
Zachycený zvuk (klidně několik simulovaných vteřin) se ale musel
přehrát **reálnou** rychlostí — zvuk nejde zrychlit. Vznikla fronta
"zvuku z minulosti", která doháněla obraz, co už byl dávno hotový. Čím
víc takových akcí za sebou, tím větší zpoždění.

## Skutečná oprava — architektura, ne záplata

Appka teď zpracovává **každý stisk klávesy** (psaní příkazů, RETURN,
BYE pro self-test, i celé bootování) přesně stejným způsobem jako
běžný chod — jeden snímek najednou, přirozeným tempem hlavní smyčky.
Obraz i zvuk vznikají **současně**, snímek po snímku — žádná fronta k
dohánění.

## Kritická pojistka (ověřeno testem, ne předpokladem)

Jádro při každém "stisknutí" klávesy znovu vyvolá přerušení
klávesnice. Kdybych poslal stejnou klávesu 8× (jednou za každý
snímek držení), appka by to viděla jako **osm samostatných
stisknutí** místo jednoho drženého — to by mohlo appku rozbít.

Oprava: klávesa se pošle jen na **prvním** snímku držení, zbylých
sedm snímků appka jen běží dál beze změny — klávesa zůstane "držená"
sama od sebe, přesně jako držený prst. Přímo testem ověřeno, že
opravený způsob vyvolá přerušení přesně jednou, stejně jako předtím.

## Boot a RESET

Taky už neběží stovky snímků najednou — jen bleskově resetují stroj a
hned pokračují normální smyčkou, která zbytek (test paměti při startu
atd.) odehraje přirozeným tempem. "Dlouhý zvuk při startu" by teď měl
hrát **současně** s tím, jak se mění obraz, ne až později.

## Výjimka

Ukládání do WAV souboru zůstává jako "velký blok" schválně — jde o
export na pozadí, ne o živě sledovanou akci, takže zpoždění tam
nikoho neruší (nic se v tu chvíli nepřehrává naživo).

## Ověření

Všech 12 předchozích testů prochází beze změny + 3 nové:
- Přesné porovnání počtu vyvolání přerušení mezi starým a novým
  způsobem — shodné, 1×
- Kompletní simulace nové architektury pro CSAVE — zvuk přítomný
  průběžně, motor správně zapnutý po druhém RETURN
