# B161 — ATARI DÝCHÁ (versionCode 209)

## Spočítal jsem to z tvého logu

`PixelCopy` stojí ~29 ms a kreslení H.264 ~9 ms na jeden snímek.
Kolik z toho spolkne hlavní vlákno, na kterém běží emulátor:

```
B160   27 ms, plné rozlišení      141 % vlákna   ← víc, než vůbec je
jen zpomalení na 66 ms             58 % vlákna   ← proto to minule nestačilo
B161   66 ms + poloviční strana    14 % vlákna   ← emulátoru zbývá 86 %
```

Při 141 % nemá emulátor **žádný** čas. Proto se kouše i na mobilu, ne
jen na TV.

## Dvě příčiny

**1) Atari dostávalo RYCHLEJŠÍ tempo, ne pomalejší.**

```java
: Math.max(8, napTvWebFrameDelayMs / 2); // Sega/Atari: rychleji
```

PS1, Sega i živá jádra v intru dávají snímek **přímo** — snímání okna se
jich netýká a rychlá větev je u nich správně. Atari se ale emuluje
v JavaScriptu na hlavním vlákně a snímek se z něj bere `PixelCopy` celého
okna — na tom samém vlákně.

Ten popisek psal někdo v době, kdy se **Sega ještě snímala z okna**. Od
B117 chodí z jádra a v té větvi zůstalo jen Atari — tedy přesně to, co
tam patří nejméně.

**2) Samotné zpomalení nestačí, jeden snímek je moc drahý.**

```
720x1336 = 961 920 bodů, bitmapa 3,7 MB
360x668  = 240 480 bodů, bitmapa 0,9 MB   = 25 %
```

Poloviční strana je čtvrtina bodů a čtvrtinová kopie. Na TV bude obraz
měkčí, ale emulátor bude mít čas počítat.

## A dopsal jsem měření, které se dělalo a nikam nepsalo

V kódu se dávno počítá, kolik stojí ta kopie bitmapy na hlavním vlákně —
ale **nikdy se to nezalogovalo**. Teď ano:

```
BUILD2SA47 TV_CENA_HLAVNIHO_VLAKNA kopieBitmapy=..ms pixely=..ms yuv=..ms
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech etap | 0 chyb |
| jádra Segy i PS1 (C++) | nesaháno |
| intro a živá jádra | beze změny (mají rychlou větev dál) |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV a jdi do **ATARI 130XE**. Zahraj si.
**3)** **Kouše se obraz nebo zvuk — na mobilu nebo na TV?**
**4)** Vypni TV, Atari znovu — má být jako dřív.
**5)** Intro, PS1 a Sega — beze změny.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | **Atari běží plynule**, TV měkčí a asi 15 snímků/s | pořád se kouše |
| 4 | jako dřív | horší |
| 5 | beze změny | jakákoli změna |

**Je v tom vědomý kompromis:** TV bude méně plynulá, aby emulátor běžel.
Řekni, jestli je ten poměr správně — to číslo 66 ms se dá měnit.

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA47 TV_CENA_HLAVNIHO_VLAKNA kopieBitmapy=...
```

Ten řádek řekne, kolik to opravdu stojí — už nebudu počítat z odhadu.
