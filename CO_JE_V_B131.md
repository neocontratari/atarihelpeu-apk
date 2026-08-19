# B131 — ÚVODNÍ FILM PRO SRAZ KLANU (versionCode 179)

> **Tento kód je předpoklad. Čeká se na test.**

## Tři okna, jak jsi je popsal

```
0,0 –  2,5 s   televize se zapíná
2,5 – 12,0 s   1. OKNO: ATARI 130XE ... OK / SEGA MEGA DRIVE ... OK
               PLAYSTATION ... OK / GOLD BETA - KKT ONLY / READY.
               kurzor spadne pod READY
12,0 – 30,0 s  2. OKNO: program v BASICu se píše řádek po řádku,
               klávesy klapou, na konci RUN
30,0 – 32,5 s  skok
32,5 – 56,5 s  3. OKNO: JEDNO logo nahoře, dole vsazená obrazovka,
               v ní běží program — duha a vypsaný text
56,5 – 60,5 s  zatmívačka
```

Celkem **60,5 vteřiny**, pak samo do rozcestníku. **PŘESKOČIT** kdykoli.

## Co jsem opravil podle tebe

**Dvě loga jsou pryč.** Neopravoval jsem odraz — **smazal jsem ho**.
A s ním i celou starou scénu, ve které byl, ať se nemůže vrátit.
Ověřeno: `odrazLoga(` se v souboru vyskytuje **nulakrát**.

**GOLD BETA - JEN PRO KKT** smazáno. Nulakrát.

**Plovoucí text smazán.** Nulakrát.

**Slunce smazáno.** Nulakrát.

Vyhodil jsem rovnou i všechny funkce, které se už nepouštěly —
`hlavni`, `logoPrilet`, `beh`, `scroller`, `odrazLoga`, `slunce`.
Mrtvý kód je past: za měsíc to někdo zapojí zpátky a diví se.

## Vsazená obrazovka s duhou

Ve třetím okně je dole **rámeček jako monitor** a v něm běží program:
duha přes celou plochu a přes ni vypsaný text. Duha se posouvá.

Na Atari se to dělá tak, že se mění barva pozadí na každém řádku
(`POKE 710` v přerušení display listu). Tady je to nakreslené —
je to fake, jak jsi sám řekl — ale vypadá to stejně a chová se to
stejně: text se dopisuje s kurzorem, jako by program běžel.

## Klávesy klapou jako Atari

Předtím jsem tam měl **pípání** a to bylo špatně. Atari pro klávesu
žádný tón nedělá — OS jen **klapne reproduktorem** (bit 3 registru
CONSOL). Je to krátký cvak, ne pípnutí.

Předělal jsem to na velmi krátký šum s ostrým náběhem. RETURN klape
jinak než písmeno.

## Ověřeno u mě

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 42 tříd |
| C++ Atari, PS1, Sega (aarch64) | 4/4 |
| linkování PS1+Sega | 31/31 |
| JS rozcestníku, intra i HELP | 0 chyb |
| film spuštěný proti skutečnému plátnu | doběhne za 60,5 s, přesměruje |
| původní Atari | bajt po bajtu netknuté |

**Výkon:** nejtěžší scéna 53 ms na snímek v **softwarovém** vykreslování
mého testu. Telefon má GPU, kde je roztažení plazmy prakticky zadarmo.
Kdyby se přesto sekalo, je v `intro/index.html`:

```js
var PLAZMA_PLYNULE = true;   // na false = kostičkovaná plazma, 11x rychlejší
```

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci a **nech film doběhnout celý**.
**3)** Spusť znovu (ukončit a otevřít) — má naskočit zas.
**4)** Během filmu zmáčkni **PŘESKOČIT**.
**5)** Vrať se z hry do nabídky — film **nemá** naskočit.
**6)** **ATARI**, **PS1**, **SEGA** — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | tři okna za sebou, **jedno jediné logo**, dole běžící program s duhou | dvě loga, chybí okno, seká se, ticho |
| 3 | naskočí znovu | nenaskočí |
| 4 | okamžitě do nabídky | nic |
| 5 | nenaskočí | naskočí |
| 6 | jako dosud | jakákoli změna |

## CO MI ŘEKNI

1. **Je tam pořád někde druhé logo?** Mělo by být jedno.
2. **Klapou klávesy jako Atari?** Dělal jsem to po paměti.
3. **Neseká se to?**
4. **Sedí ta duha?** Je to fake, ale má vypadat jako běžící program.

## CO POSLAT ZPĚT

Log přes **ULOZIT LOG A ODESLAT** na obrazovce HELP.

---

## Atari v C++ — kde se skončilo

Práce **pozastavená** na tvůj pokyn, celý stav je v
`PREDAVACI_BALICEK_PS1.txt`.

**Běží:** procesor (1 024 000 instrukcí, 0 rozdílů), paměť (16 252 928
čtení, 0 rozdílů), WSYNC, zavaděč XEX, display list, DLI, OS nabootuje,
BASIC naskočí, **self-test z ROM se vykreslí celý**, Decathlon se nahraje.

**Neběží:** vykreslení Decathlonu, kolize, zvuk, VBXE.

**Poučení v protokolu:** vrstvy 1 a 2 jsem překládal z JavaScriptu a
vyšly napoprvé. Vrstvu 3 jsem začal psát znovu a za den z toho bylo
**osm vlastních chyb**, které v původním JS nejsou. Další krok je
vrstvu 3 **přeložit**, ne psát.
