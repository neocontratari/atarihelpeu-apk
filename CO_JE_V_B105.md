# B105 — DOLADĚNÍ OBRAZU NA TV (versionCode 153)

> **Tento kód je předpoklad. Čeká se na test.**
> Jádra, emulace ani obrazu na mobilu jsem se **nedotkl**.

## Odpověď na tvou otázku: ano, jde to — a není to fake

Ptal ses na „pořádnou berlu", něco jako u zvuku, co si doladíš sám.
Udělal jsem to a je v tom jeden podstatný rozdíl proti tomu, co jsi
odmítal dřív:

**Počítá to grafika tvého PC v prohlížeči, ne telefon.**

Telefon o tom vůbec neví a nedělá nic navíc — emulace se tím nezpomalí
ani o milisekundu. A není to zásah do jádra: je to úprava **hotového
obrazu až na cíli**, přesně jako když si doladíš ostrost na televizi.
Emulace zůstává čistá.

## Co přibylo v panelu

    jas
    kontrast
    sytost
    ostrost      <- nove
    vyhlazeni    <- nove
    PUVODNI NASTAVENI
    CELA OBRAZOVKA: vyplnit / oriznout

**Ostrost** vytáhne detaily zpátky — obraz z PlayStation má ostré pixely
a komprese je změkčí, tímhle se vrátí.

**Vyhlazení** naopak schová hrany kostek z komprese. Zní to protichůdně,
ale funguje to spolu: trochu vyhladit a pak doostřit dá čistší obraz než
kterékoli z toho samo.

Doporučuju začít na **ostrost 30, vyhlazení 20** a doladit podle sebe.

Nastavení se pamatuje, stejně jako jas a kontrast.

## K tomu kousnutí zvuku

Píšeš, že se to kousne na jednom místě a **na skutečné PS1 taky**. To je
dobrá zpráva — znamená to, že se emulace chová stejně jako originál.
Nesahám na to.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Zapni TV a zkus **ostrost** — táhni pomalu nahoru
**2)** Přidej **vyhlazení** — sleduj kostky v tmavých plochách
**3)** Zkus **PUVODNI NASTAVENI**, jestli vrátí všech pět
**4)** Zkontroluj **mobil** — nesmí se nic změnit
**5)** Sleduj plynulost — telefon by neměl poznat rozdíl

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz **ostřejší**, detaily vystoupí | nic se neděje |
| 2 | kostky se **schovají** | nic |
| 3 | všech pět skočí na výchozí | jen tři |
| 4 | mobil **beze změny** | zhoršilo se |
| 5 | plynulost stejná jako v B104 | kouše se |

Krok 5 je důležitý: kdyby se to kouslo, znamená to, že filtr počítá procesor
místo grafiky — řekni a udělám to jinak.

## CO POSLAT ZPĚT

Jde obraz doladit do čista? A nezhoršila se plynulost?
