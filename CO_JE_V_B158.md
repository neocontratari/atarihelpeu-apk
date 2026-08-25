# B158 — OBRAZ I ZVUK NA TV (versionCode 206)

Motal jsem se, protože jsem hádal. Tenhle build stojí na dvou tvrdých
nálezech z tvého logu.

## 1) Sega a PS1 neměly na TV obraz

V logu je u **všech** záznamů během intra:

```
url=file:///android_asset/intro/etapa2.html
```

Adresa se během živých jader nemění — WebView zůstává na etapě 2.
A hned na začátku té funkce je podmínka:

```java
if (!naSege && !ps1SessionActive && !ps1GameWindowOwnsCore && !ps1BiosRunning) {
    return false;     // ← vyskočilo se TADY
}
```

Moje přepnutí na jádro bylo **až za ní** a nikdy se nespustilo. TV proto
dál snímala okno.

Teď se během intra pokračuje dál a zdroj se pozná podle toho, které
jádro běží. Ověřeno spuštěním, 7/7 případů:

```
hra Sega              -> Sega z jádra
hra PS1               -> PS1 z jádra
INTRO - běží Sega     -> Sega z jádra
INTRO - běží PS1      -> PS1 z jádra
INTRO - etapa 1 a 5   -> snímek okna
rozcestník            -> snímek okna
```

## 2) Zvuk Atari na TV byl kousavý

Ten parametr je **počet shortů, ne rámců**. Pole je stereo, takže na
1024 rámců připadá 2048 shortů — a já posílal 1024.

```
posílal jsem 1024  ->  50 % vzorků
teď          2048  ->  100 %
```

A druhá věc, kterou má Sega v kódu dávno napsanou: **posílat se musí
dřív, než se přehraje.** `at.write()` čeká, až reproduktor dávku dohraje,
takže když se na TV posílá až potom, přijde zvuk pozdě a televize ho
zahodí.

Obojí opravené.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech etap | 0 chyb |
| zdroj obrazu pro TV | 7/7 správně |
| řetěz etap | 1 → 2 → jádra → 5 → konec |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV a nech intro běžet.
**3)** Etapa 1 a 2 — obraz i **plynulý** zvuk?
**4)** Sega — **obraz i zvuk**?
**5)** PS1 — **obraz i zvuk**?
**6)** Etapa 5 s krtečkem — obraz i zvuk?
**7)** Na mobilu má být všechno jako v B157.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | plynulý zvuk | kousavý |
| 4 | obraz i zvuk | jen zvuk |
| 5 | obraz i zvuk | jen zvuk |
| 6 | obraz i zvuk | něco chybí |
| 7 | jako v B157 | horší |

## CO POSLAT ZPĚT

Log. Během Segy a PS1 už tam **nesmí** být:

```
TV_WEB_PERIODIC ... url=.../etapa2.html
```

Když tam pořád bude, TV se na jádro nepřepnula.
