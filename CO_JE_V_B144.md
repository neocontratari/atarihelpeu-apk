# B144 — GRAFIKA PS1 (versionCode 192)

## Chyba byla v pořadí a byla moje

```java
// uvnitr ps1MaybeStartBios():
ui.post(() -> ps1GlEnable());     ← kresleni se zapina TADY
String r = bootBiosSafe(...);
```

`ps1GlEnable()` si bere rozměry z `plochaL..H`. Já v intru stavěl plochu
**až po** `ps1MaybeStartBios()` — jenže ten boot běží na vlastním vlákně
a `ps1GlEnable()` se spustil dřív, než plocha existovala. **Jádro kreslilo
do ničeho.** Odtud černá obrazovka a jen zvuk.

U Segy je pořadí správně — plocha, pak jádro. U PS1 jsem ho měl obráceně.

```
předtím:   okno → boot → plocha
teď:       okno → PLOCHA → boot → zapnout kreslení
```

K tomu jedna pojistka: když BIOS už běžel z dřívějška, `ps1MaybeStartBios()`
se hned vrátí a `ps1GlEnable()` se nezavolá vůbec. Proto ho zapínám i sám —
je to **zapnutí kreslení, ne zásah do běhu jádra**.

## Na cyklus PS1 se nesahá

Ověřeno v balíčku: `stopPs1SessionHard("intro...")` **0×**.
Jádro PS1 v C++ netknuté.

## Zůstává

Původní grafika Segy (funguje), zvuk PS1 na TV, BIOS v `PS1_BIOS`,
stahování se souhlasem, kontrola souborů.

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nech film běžet — po Seze má naskočit **obrazovka PS1 i s grafikou**,
       ne jen zvuk.
**3)** Pusť intro znovu z OPTIONS — to samé i podruhé.
**4)** Atari, PS1, Sega — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | grafika PS1 i zvuk | černo a jen zvuk |
| 3 | to samé podruhé | jen napoprvé |
| 4 | jako dosud | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA33 INTRO_PS1 plocha 720x1336
PS1_BIOS_START vysledek=PS1_BIOS_OK
```

První řádek musí být **před** druhým. Když bude po něm, pořadí je pořád
špatně.
