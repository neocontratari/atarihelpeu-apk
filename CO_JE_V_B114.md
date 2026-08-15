# B114 — VYPLNIT OBRAZOVKU JAKO VÝCHOZÍ (versionCode 162)

> **Tento kód je předpoklad. Čeká se na test.**
> Stojí na B112, který jsi označil za správný. Sáhl jsem **jen na to
> tlačítko** — na nic jiného.

## Nemáš se za co omlouvat

Ten přepínač byl matoucí, protože jsem mu dal špatné popisky. „CELA
OBRAZOVKA: vyplnit / oriznout" neříkalo, **co se stane po zmáčknutí** —
jestli to je stav současný, nebo ten, do kterého se přepne. To je chyba
v návrhu, ne v používání.

## Co je změněné

**Výchozí je teď „vyplnit obrazovku"** — obraz vyplní celou plochu bez
černých pruhů, přesně jak jsi chtěl. Okraje se ořežou.

**Přepnout na celý obraz** jde tlačítkem, kdykoli.

A popisky teď říkají, co tlačítko udělá:

    OBRAZ: vyplnit obrazovku    <- ted vyplnuje
    OBRAZ: cely, bez orezu      <- ted ukazuje cely

Ověřeno spuštěním, i s tím, že v prohlížeči zůstaly staré uložené hodnoty:

    1) po startu:    cover   | OBRAZ: vyplnit obrazovku
    2) po prepnuti:  contain | OBRAZ: cely, bez orezu
    3) zpatky:       cover   | OBRAZ: vyplnit obrazovku

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| JavaScript | **spuštěn** i se starými hodnotami | výchozí je vyplnit |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64 | 0 chyb |

---

## CO TESTOVAT

**1)** Zapni web — obraz má **vyplnit obrazovku** hned od začátku
**2)** Zmáčkni tlačítko — má ukázat **celý obraz**
**3)** Zmáčkni znovu — zpátky na vyplněno

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | vyplněno, bez pruhů | pruhy |
| 2 | celý obraz, poměr zachovaný | nic |
| 3 | zase vyplněno | zasekne se |

## Co je na řadě potom

Ovladač s minimální odezvou a napojení klávesnice a joysticku — to, cos
zadal před testem na plátně. Mám k tomu zmapované cesty, čeká to na tebe.
