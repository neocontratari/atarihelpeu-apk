# B149 — ČAS FILMU (versionCode 197)

## Konečně ta pravá příčina

Tvůj log z B147 ji ukázal přesně:

```
17:34:03.447  BUILD2SA32 INTRO_PS1 konec casti
17:34:12.103  BUILD2SA21 INTRO_HOTOVO      ← o 8,6 s pozdeji
```

Po části s PS1 mělo hrát ještě **26 vteřin** (skok 2 s + finále 20 s +
závěr 4 s). Film to stihl za 8,6 s — **přehnal se do konce během pár
snímků**. Proto se krteček nikdy neukázal.

**Proč:** čas jsem bral jako rozdíl dvou razítek od prohlížeče
(`ts - scenaOd`). Jenže když nad stránkou leží plocha jádra, WebView
přestane být vidět a **prohlížeč kreslení uspí**. Po probuzení je razítko
o desítky vteřin dál, rozdíl je obrovský a scény se prostřídají jedna
za snímek.

**Oprava:** čas se počítá po krocích a **každý krok se omezí na 250 ms**.
Když prohlížeč spal, film jen počká, místo aby přeskočil.

Ověřeno spuštěním — napodobil jsem, že prohlížeč uprostřed PS1 usne na
40 vteřin:

```
razítko na konci: 110,3 s  (v tom 40 s spánku)
film doběhl:      ANO
volání:           spustSegu, zastavSegu, spustPs1, zastavPs1, hotovo
```

## Zůstává z B148

**Pozadí WebView se přepíná jen jednou za intro.** Předtím u každé živé
části — v tvém logu je `INTRO_OKNO` **12×**. Po změně `setBackgroundColor`
s hardwarovou akcelerací WebView často přestane kreslit.

**TV bere během živých částí snímek z jádra**, ne z okna. Plocha jádra
není součástí okna, takže ji `PixelCopy` nezachytí — u PS1 je to v kódu
zadokumentované.

## Zůstává z B147

`stopPs1SessionHard()` kontroluje `ps1BiosRunning`. **V tvém logu to už
funguje** — `PS1_SESSION_STOP ... core=PS1_STOPPED` je tam 24×.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| film doběhne i po 40 s spánku prohlížeče | ověřeno spuštěním |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nech film běžet — **po PS1 se musí objevit krteček**.
**3)** Zahraj hru na PS1, pak pusť intro z OPTIONS — to samé.
**4)** Zapni WEB TV a nech film běžet — obraz i zvuk u všech tří částí.
**5)** Atari, PS1, Sega — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | krteček po PS1 | prázdno |
| 3 | krteček i po hraní | prázdno |
| 4 | obraz i zvuk všude | jen zvuk |
| 5 | jako dosud | jakákoli změna |

## CO POSLAT ZPĚT

Log. Změř si v něm rozestup:

```
BUILD2SA32 INTRO_PS1 konec casti
BUILD2SA21 INTRO_HOTOVO
```

**Musí být kolem 26 vteřin.** Když bude znovu 8 nebo 9, film se pořád
přeskakuje a jdu hledat dál.

A `BUILD2SA37 INTRO_OKNO` musí být **jen jednou**, ne dvanáctkrát.
