# B182 — POČÍTÁNÍ ŘÁDKŮ (versionCode 230)

Ptal ses správně — **to odečítání řádků napojené nebylo.** Čas Segy ano,
řádky v Atari ne. Teď je to hotové.

## Jak to funguje

```
program má 5 a víc řádků   ->  přičte se
má míň                     ->  ignoruje se
dohromady 50 řádků         ->  Sega se odemkne
```

Nemusí to být jeden program. **Deset programů po pěti řádcích vyjde
nastejno** jako jeden padesátiřádkový. Ale `10 PRINT "A"` a `RUN` dokola
nikoho nikam nedostane.

Řádek se počítá, když **začíná číslem a něco za ním je** — tedy jako
řádek programu, ne jako příkaz `RUN` nebo `LIST`. Dávka se odešle při
`RUN` nebo `LIST`.

Počítá se z obou klávesnic — z počítače i z té dotykové v telefonu.

## Ověřeno spuštěním

```
co hráč napsal                    dávka   výsledek
jeden řádek a RUN                   1     ignorováno
dva řádky                           2     ignorováno
pět řádků                           5     přičteno
deset řádků                        10     přičteno

deset programů po pěti řádcích  ->  SEGA OTEVŘENA
```

## A zámek teď říká, kolik zbývá

```
NAPIS JESTE 35 RADKU KODU V ATARI
```

M�sto obecného „napiš kód".

## Stav se dá přečíst

```
BUILD2SA68 ATARI_RADKY +5 -> radku=25/50 sega=ZAMCENA ps1=ZAMCENA
```

A `ODEMKNOUT DALSI` funguje dál — nastaví rovnou 50, aby se to
nepletlo.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i rozcestníku | 0 chyb |
| počítání řádků | 4/4 případy správně |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** OPTIONS → `ZAMKNOUT VSE`, ať začneš od nuly.
**3)** Napiš v Atari program o pěti řádcích a dej `RUN`.
**4)** Zpátky v menu má u Segy stát `NAPIS JESTE 45 RADKU`.
**5)** Zkus napsat jeden řádek a `RUN` — nesmí se počítat.
