# B99 — TV ZPĚT NA FUNKČNÍ CESTU (versionCode 147)

## Co jsem udělal

**Vypnul jsem tu novou cestu na TV.** Čtyři pokusy, čtyři pády — a já ti
každý z nich poslal s tím, že to snad bude ono.

TV jede zase javovou cestou, kde v B92 **prokazatelně fungovala**.

## Přiznání, ať je to jasně napsané

V B94 až B98 jsem našel a opravil čtyři skutečné chyby: okno enkodéru se
předávalo dřív než se enkodér spustil, chyběl formát okna, hrozilo zaseknutí
na zámku, staré vlákno mohlo kreslit souběžně s novým.

**Všechny čtyři byly opravdové. A přesto to padalo dál.** To znamená, že tam
je něco pátého, co nevidím — a chování `MediaCodec` se dvěma vlákny si u sebe
ověřit nedokážu. Neměl jsem tě do toho tahat počtvrté.

Nativní kód zůstává v projektu, jen se nespustí (`POUZIT_PRIMOU_CESTU_NA_TV
= false`). Kdyby se k tomu někdy vracelo, musí to být s telefonem po ruce,
ne přes testy na dálku.

## Co v buildu zůstává — všechno, co funguje

| co | odkud | stav |
|---|---|---|
| obraz na mobilu přes OpenGL ES | B89 | netknuté |
| prověřený vykreslovač `gpu_neon` | B84 | netknuté |
| zvuk | B92 | netknuté |
| TV: doostřování v Javě vypnuté | B92 | zůstává |
| TV: tempo 16 ms místo 5 | B82 | zůstává |
| TV: o jednu kopii snímku míň | B93 | zůstává |
| TV se vrací na okno při odchodu z PS1 | B90 | zůstává |
| průběžný zápis logu vypnutý | B98 | zůstává |

Takže proti B92 je TV cesta pořád **lehčí**, jen ne přepsaná do C.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni **TV web**
**2)** Nech hru chvíli běžet

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | **aplikace nespadne**, na TV obraz | spadne |
| 2 | jako v B92, možná o kus plynulejší | horší než B92 |

Když tohle projde, jsme zpátky na jisté zemi a tu cestu na TV už znovu
otevírat nebudu, dokud si to nebudu umět ověřit sám.
