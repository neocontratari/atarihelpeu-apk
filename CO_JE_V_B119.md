# B119 — ZVUK A TEMPO (versionCode 167)

> **Tento kód je předpoklad. Čeká se na test.**
> Staví na B118, který jsi vzal jako výchozí bod.

## 1. Zvuk Segy

Zvuk **už z jádra taháš** — jde přes `pullAudioStereo` do `AudioTrack`.
Zpoždění nedělá cesta, ale **velikost zásobníku**:

    512 snimku = 11 ms
    1024       = 21 ms   <- tady to bylo
    2048       = 43 ms

Zmenšil jsem ho o stupeň: **21 ms → 11 ms**.

A našel jsem u toho druhou věc: velikost se počítala jako `min * 2`, tedy
dvojnásobek systémového minima. Na některých telefonech to znamená
mnohonásobně větší zásobník, než jsme chtěli. Teď stačí to minimum.

**Kdyby zvuk začal praskat**, je zásobník moc malý — řekni a vrátím původní
hodnoty. Praskání pozná i uživatel hned, není to nic skrytého.

## 2. Zbytek toho zpoždění obrazu

Z tvého logu:

    avgTickGapMs=42   pri   targetDelayMs=12

Smyčka si řekla o 12 ms, ale dostala 42. Důvod: běží na **hlavním vlákně**,
které zároveň kreslí obrazovku a obsluhuje dotyky — takže se ke slovu
dostane, až hlavní vlákno dodělá svoje.

Zkusil jsem ji přehodit na vlastní vlákno, ale **nejde to**: sahá na
`rootFrame` a `web.getUrl()`, což jsou pohledy aplikace, a z jiného vlákna
se na ně sahat nesmí — spadlo by to. Našel jsem to při kontrole, ne po
tvém testu.

Řešení je bezpečnější: když snímek přichází přímo z jádra, další průchod
se **zařadí přednostně**, takže se hlavní vlákno dostane k nám hned, jak
dokreslí. Není to tak rychlé jako vlastní vlákno, ale nespadne to.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Sega C++ | `clang` pro aarch64 | 0 chyb |
| PS1 C++ | `clang` pro aarch64 | 0 chyb |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |

---

## CO TESTOVAT

**1)** **Sega — zvuk.** Je zpoždění menší? A hlavně: **nepraská?**
**2)** **Sega — obraz na TV.** Zmenšilo se zpoždění ještě víc?
**3)** **PS1 — zvuk i obraz.** Kontrola, že jsem nic nerozbil.

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | zvuk blíž obrazu, čistý | **praská** → vrátím zásobník |
| 2 | menší než v B118 | stejné |
| 3 | jako v B118 | zhoršilo se |

**Krok 1 je ten hlavní.** Praskání je jediné riziko téhle změny a poznáš
ho okamžitě.

## CO POSLAT ZPĚT

Praská zvuk? A z logu jeden řádek: `avgTickGapMs=` — má klesnout ze 42.
