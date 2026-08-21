# B146 — NÁVRAT Z PS1 (versionCode 194)

Dvě chyby, obě v mém kódu.

## 1) Film se nevrátil na závěrečnou část s krtečkem

Plocha PS1 se na výšku staví **nad stránkou** — v logu:

```
PLOCHA_POSTAVENA_ZNOVU 0,0 720x1384 (na vysku, nad strankou)
```

Dokud tam je, překrývá celý film. Spoléhal jsem na hlídače, že ji
sundá — jenže ten to udělá až při svém dalším tiku.

**Teď se sundá rovnou**, hned po skončení části s PS1.

## 2) Podruhé naskočilo menu BIOSu místo znělky

`ps1MaybeStartBios()` se hned vrátí, když BIOS už běží — a ten po prvním
intru běžel dál. Podruhé se tedy nespustil a bylo vidět, kde BIOS mezitím
došel: **Memory Card a Play CD**.

**Teď jádro před znělkou bootuje načisto** a po skončení části se zastaví,
aby příště nabootovalo zase od začátku. Sám jsi o to požádal —
*„vytáhni si intro z čistého jádra"*.

Obojí je ohraničené na případ, kdy nic jiného neběží:

```java
!ps1SessionActive && !ps1BootActive && !ps1GameWindowOwnsCore
```

Při hraní se nestane nic.

## Ověřeno spuštěním

Pustil jsem film ve dvou případech:

```
přeskočení uprostřed PS1:  spustSegu, zastavSegu, spustPs1, zastavPs1, hotovo
celý film do konce:        spustSegu, zastavSegu, spustPs1, zastavPs1, hotovo
```

Úklid PS1 se zavolá v obou — kdyby ne, plocha by zůstala viset přes
celou aplikaci.

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| úklid při přeskočení i při doběhnutí | ověřeno spuštěním |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nech film běžet celý — po PS1 se **musí vrátit na krtečka**.
**3)** Pusť intro **podruhé** z OPTIONS — musí zaznít znělka Sony,
       ne menu s Memory Card.
**4)** Přeskoč intro **uprostřed části s PS1** — nesmí zůstat viset obraz.
**5)** Atari, PS1, Sega — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | vrátí se na krtečka | zůstane na PS1 |
| 3 | znělka Sony | Memory Card / Play CD |
| 4 | čisté menu | visí obraz PS1 |
| 5 | jako dosud | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA35 INTRO_PS1 stare jadro zastaveno
BUILD2SA35 INTRO_PS1 plocha schovana
BUILD2SA35 INTRO_PS1 jadro zastaveno - priste cisty boot
```
