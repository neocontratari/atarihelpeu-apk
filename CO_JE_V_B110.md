# B110 — BEZ TRHÁNÍ (versionCode 158)

> **Tento kód je předpoklad. Čeká se na test.**

## Našel jsem to a je to moje chyba z B108

Z tvého logu:

    avgTickGapMs=96      <- smycka bezi kazdych 96 ms misto 16
    avgDrawMs=3          <- enkoder pritom stiha bez problemu

96 ms je **devět snímků za vteřinu** místo šedesáti. Odtud to trhání.

**Příčina:** v B108 jsem přidal barevný filtr (kontrast a sytost) na plátno,
které je `new Canvas(bitmapa)` — tedy **plátno v paměti, ne na grafice.**
Všechno, co se na něm dělá, počítá **procesor**. Ten filtr znamenal devět
násobení na každý bod, u 640×480 přes **osm milionů operací na snímek**.

Přesně to, čemu jsme se celou dobu vyhýbali. Odstraněno.

## Co zůstalo

**Vyhlazení při zvětšení** — to Canvas zvládne levně a je to ta část, která
dělala obraz čistší. Zůstává a jde vypnout tlačítkem.

**Kontrast a sytost** si nastav v panelu v prohlížeči — tam to počítá
grafika tvého PC a telefon to nestojí nic. Výsledek je stejný.

## A přidal jsem měření

Do logu se zapisuje, kolik kreslení opravdu stojí:

    TV_KRESLENI prumer=3 ms  vyhlazeni=ZAP

Když to poleze nad 8 ms, je vyhlazení příliš drahé a vypneme ho. **Teď to
uvidíme, místo abychom hádali.**

## Z B109 zůstává

Tlačítko VYLADENI OBRAZU (tentokrát opravdu v HTML) a ořez na okénko
konzole při snímání na TV.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| JavaScript stránky | **spuštěn** v mock prohlížeči | projde |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64 | 0 chyb |

---

## CO TESTOVAT

**1)** Zapni TV, pusť hru — **hlavně jestli se to netrhá**
**2)** Přepni **VYLADENI OBRAZU** a porovnej
**3)** Na výšku: je na TV jen obraz hry?

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | plynulé jako v B104 | pořád se trhá |
| 2 | rozdíl v čistotě, plynulost stejná | s vyhlazením se trhá |
| 3 | jen obraz hry, roztažený | celá aplikace |

## CO POSLAT ZPĚT

Trhá se to ještě? A z logu jeden řádek:

    TV_KRESLENI prumer=... ms

To mi řekne, jestli je vyhlazení únosné, nebo ho mám vyhodit taky.
