# B112 — SNÍMÁNÍ OKNA ZPĚT JAKO V B104 (versionCode 160)

> **Tento kód je předpoklad. Čeká se na test.**

## Měl jsi pravdu, měl jsem to prohozené

Ten ořez na okénko, který jsem přidal v B109, **rozbil snímání okna
aplikace**: mobil na výšku posílal na web pohled na šířku a naopak,
Atari i Sega se ořezávaly špatně.

Důvod: ta funkce má **vlastní logiku pro otočení a měřítko**, do které
můj ořez zasahoval. Bylo to v pořádku v B104 a já to zbytečně přepsal.

**Snímání okna je teď přesně jako v B104** — porovnal jsem to řádek po
řádku proti tvému funkčnímu základu.

Obraz PS1 jde na TV úplně jinou cestou, která bere snímek přímo z jádra
a okna aplikace se vůbec netýká — proto se to nedotklo toho, co u PS1
funguje.

## Co zůstává z posledních buildů

Všechno, co se osvědčilo:

- **vyladění obrazu před kompresí** (vyhlazení) — B108, píšeš že je míň
  kostiček
- **odstraněný barevný filtr na procesoru** — B110, ten způsoboval trhání
- **výchozí kontrast 112 % a sytost 108 %** — B111, dorovnává změkčení
- **tlačítko VYLADENI OBRAZU** — můžeš porovnat oba stavy
- **měření TV_KRESLENI** v logu

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| snímání okna | porovnáno řádek po řádku s B104 | shodné |
| JavaScript stránky | **spuštěn** v mock prohlížeči | projde |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64 | 0 chyb |

---

## CO TESTOVAT

**1)** **Úvodní obrazovka** na výšku → na TV má být na výšku
**2)** **Atari** → celá obrazovka Atari
**3)** **Sega** → celá obrazovka Sega
**4)** **PS1** → obraz hry (tam už to podle tebe funguje)

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | na TV totéž co na mobilu, správně otočené | prohozené |
| 2 | celé Atari, ne výřez | ořízlé |
| 3 | celá Sega | ořízlé |
| 4 | jako v B111 — super | zhoršilo se |

## CO POSLAT ZPĚT

Sedí otočení u všech čtyř? A nezhoršilo se PS1?
