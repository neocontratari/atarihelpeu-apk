# B172 — PÁDY A KLÁVESNICE (versionCode 220)

## 1) Proč aplikace padala právě v Atari

Alokoval jsem nové pole na **každý snímek**:

```
360 kB na snímek
16 snímků za vteřinu = 5,6 MB/s
za minutu a půl     = 411 MB odpadu
```

Uklízeč paměti to nestíhal a aplikace spadla. **A padalo to jen v Atari,
protože jinde tahle cesta neběží** — Sega a PS1 mají snímek z jádra.

Pole se teď drží a jen přepisuje. **Odpad: nula.**

## 2) Shift psal S

Bral jsem z názvu klávesy **první písmeno**. Takže:

```
klávesa      dřív -> teď
Shift        'S'  -> nic
Enter        'E'  -> nic
Control      'C'  -> nic
Alt          'A'  -> nic
ArrowUp      'A'  -> nic
a            'a'  -> 'a'
"            '"'  -> '"'
```

Teď musí být znak **přesně jeden**. A přeřaďovače se ze stránky vůbec
neposílají — samy o sobě nic nepíšou.

## 3) Ten projektor

Zámek jsem nikde neudělal — prošel jsem to. Ale na druhé obrazovce
nebo na celé ploše **okno snadno ztratí zaměření**, a prohlížeč pak
klávesy nikam neposílá. To vypadá přesně jako „nefunguje to".

Stránka si teď zaměření **bere zpátky** — při doteku, při přepnutí zpět
a každé dvě vteřiny sama. A hlavně: **když ho přesto nemá, napíše to**:

```
KLAVESNICE ZAPNUTA - F2 = BREAK        vpravo dole modře = píše
KLIKNI DO OBRAZU - okno nema zamereni  červeně = klávesy nikam nejdou
```

Takže na plátně hned uvidíš, jestli je problém v zaměření, nebo jinde.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| klávesy proti pravé tabulce Atari | 11/11 |
| kontrola stránky pro TV | 6/6 |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Atari nech běžet **aspoň pět minut** — spadne to?
**3)** Napiš `10 PRINT "AHOJ"` — projdou uvozovky a nepíše Shift písmena?
**4)** Na projektoru koukni vpravo dole: **modře, nebo červeně?**

Když bude červeně, klikni do obrazu a zkus znovu.
