# B118 — SEGA BEZ ZPOŽDĚNÍ (versionCode 166)

> **Tento kód je předpoklad. Čeká se na test.**

## Log ukázal, kde to zbylé zpoždění bylo

    avgTickGapMs=46      <- snimek se bere kazdych 46 ms (22 za vterinu)
    avgDrawMs=3          <- enkoder pritom stiha na 3 ms

Enkodér má rezervu, ale **snímek se mu podával jen dvaadvacetkrát za
vteřinu**. Sega jede na 60, takže se dvě třetiny zahodily.

Moje oprava z B116 byla polovičatá: rozdělil jsem snímání na rychlou
a pomalou větev podle toho, jestli obraz jde z jádra — ale ptal jsem se
**jen na PS1**. Sega tehdy do jádra ještě nepatřila, jenže od B117 už ano
a já tu podmínku zapomněl rozšířit.

Teď je Sega v rychlé větvi, kam patří.

## A zkrátil jsem krok na 12 ms

Enkodér má podle logu rezervu (3 ms při stropu 16), takže snímek může jít
na TV dřív. Menší krok = menší zpoždění.

Pod 12 nejdu — při 5 ms se v B82 kousal zvuk, protože smyčka brala procesor
emulaci. To je zapsané jako past.

## Ke zvuku Segy

Píšeš, že mírné zpoždění zvuku bylo vždycky i na mobilu. To odpovídá tomu,
že zvuk pořád jde starou cestou — jádro už ho umí dodat nativně
(`pullAudio`), ale aplikace na to nepřepnula. To je další krok, ale nechci
ho míchat do buildu, který řeší obraz.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Sega C++ | `clang` pro aarch64 | 0 chyb |
| PS1 C++ | `clang` pro aarch64 | 0 chyb |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |

---

## CO TESTOVAT

**1)** **Sega na TV** — je zpoždění pryč, nebo aspoň menší?
**2)** **Sega na mobilu** — nezhoršilo se?
**3)** **PS1** — kontrola, že jsem nic nerozbil
**4)** Zvuk u obou — nezačal se kousat?

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | zpoždění pryč nebo znatelně menší | stejné |
| 2 | jako v B117 | horší |
| 3 | jako v B114 | zhoršilo se |
| 4 | zvuk čistý | kouše se |

**Krok 4 je pojistka** — zkrátil jsem krok smyčky, takže chci vědět, jestli
to nezačalo brát procesor emulaci. Kdyby ano, vrátím 16 ms.

## CO POSLAT ZPĚT

Je zpoždění menší? A nekouše se zvuk?
Z logu jeden řádek: `avgTickGapMs=` — má klesnout ze 46 blíž ke dvanácti.
