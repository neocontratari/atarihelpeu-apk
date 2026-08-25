# B157 — HUDBA USTOUPÍ JÁDRU (versionCode 205)

Zhoršil jsem to a z tvého logu je vidět přesně jak.

## Co bylo špatně

```
BUILD2SA41 INTRO_ZACATEK zvuk v Jave     5×
BUILD2SA41 INTRO_ZVUK konec              0×
```

Hudbu jsem pustil na začátku každé etapy a **nikdy jsem ji nezastavil.**
Z toho plyne všechno, cos viděl:

| co jsi viděl | proč |
|---|---|
| zvuk Atari přes celé video | hudba hrála i během Segy a PS1 |
| na TV jen Atari, kousavě | pořád posílala vzorky, TV nic jiného nepustila |
| Sega a PS1 na TV vůbec | TV ustoupí, když jiný zdroj poslal mladší než 300 ms |

## Oprava

Hudba se **vypne, když má hrát jádro**, a vrátí se po něm. Ověřeno
projitím celé sekvence:

```
etapa 1 - zapnutí a test    hraje moje hudba
etapa 2 - píše se kód       hraje moje hudba
SEGA                        ticho, jádro má cestu volnou
PS1                         ticho, jádro má cestu volnou
etapa 5 - krteček           hraje moje hudba
rozcestník                  ticho
Atari po intru              ticho
```

K tomu **pojistka**: kdyby uživatel z intra odešel jinam, hudba se
zastaví i tak — sleduje se změna obrazovky.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech etap | 0 chyb |
| sekvence zvuku | 7/7 stavů správně |
| řetěz etap | 1 → 2 → jádra → 5 → konec |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nech intro běžet na mobilu — **během Segy a PS1 nesmí hrát moje hudba**.
**3)** V etapě 5 s krtečkem hudba zase být má.
**4)** Po intru v Atari, PS1 a Seze **nesmí hrát nic z intra**.
**5)** Zapni WEB TV a projdi to znovu — jde zvuk i obraz u všech částí?

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | slyšíš Segu a PS1 | přes ně hraje hudba |
| 3 | hudba hraje | ticho |
| 4 | ticho | hudba hraje dál |
| 5 | všechno | jen Atari |

## CO POSLAT ZPĚT

Log. Teď tam **musí být**:

```
BUILD2SA43 INTRO_ZVUK stop (bezi Sega)
BUILD2SA43 INTRO_ZVUK stop (bezi PS1)
BUILD2SA43 INTRO_ZVUK stop (intro dobehlo)
```

Když některý chybí, hudba se v tom místě nezastavila.

---

**Na webu nic nepřepisuj** — jak jsi řekl, verzi vyřešíme až s hlídačem.
Aktualizace se proto zeptá, nic nenajde a bude mlčet. Tak to má být.
