# B165 — ATARI DÁVÁ SNÍMEK SAMO (versionCode 213)

Použil jsem to svolení. Atari teď funguje **přesně jako Sega a PS1**.

## Co bylo špatně

```
SEGA a PS1   snímek dává JÁDRO        ->  okno se nesnímá vůbec
ATARI        PixelCopy CELÉHO OKNA    ->  čtení zpátky z grafické karty
```

To čtení **zastaví celý vykreslovací řetěz** — a v tom řetězu kreslí
i plátno, na kterém emulátor běží. Proto se to kouše hned po zapnutí TV
a proto nepomohlo nic z toho, co jsem zkoušel: zpomalení, menší
rozlišení, rychlejší most ani kreslení procesorem.

Problém nebyl v tom, KOLIK to stojí, ale v tom, **ŽE to zastaví grafiku**.

## Co je teď

Atari má hotový snímek v paměti (`M.fb`, 384×240). Posílá ho **binárně
na vlastní server aplikace**:

```
DŘÍVE   PixelCopy okna 720x1336 = 961 920 bodů + čtení z grafické karty
TEĎ     Atari pošle 384x240 = 92 160 bodů, tedy 10 % - a bez čtení
```

Žádné `PixelCopy`, žádné base64, a `fetch` je asynchronní, takže
**emulátor nikde nečeká**.

Na straně Javy jde snímek **tou samou cestou jako ze Segy a PS1** —
jen ho místo jádra dodá prohlížeč.

Když Atari snímky posílá, snímání okna se vypne. Jinak by běželo obojí
a `PixelCopy` by grafiku dál zastavovalo.

## Co jsem v Atari změnil

Se svolením, a je to přírůstek — nic se nepřepsalo:

```
přidáno   36 řádků  (funkce posliSnimekNaTv + jedno volání)
smazáno   0 funkcí
```

Ověřeno porovnáním seznamu funkcí proti B123.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| čtení portu ze `status()` | 4/4 případy správně |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV, jdi do **ATARI 130XE**, zahraj si.
**3)** **Kouše se ještě?**
**4)** Jde obraz na TV?
**5)** Vypni TV — Atari má jet jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | **jede plynule i se zapnutou TV** | pořád se kouše |
| 4 | obraz Atari na TV | černá |
| 5 | jako dosud | horší |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA51 ATARI_SNIMEK 384x240 prijato=.. (bez PixelCopy)
```

Když tam ten řádek **je** a přesto je TV černá, snímky chodí a chyba je
až za tím. Když tam **není**, Atari je neposílá a půjdu tam.
