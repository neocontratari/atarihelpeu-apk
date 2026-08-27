# B181 — ATARI ZPĚT JAKO V B156 (versionCode 229)

M�l jsi pravdu a já to zrušil.

## Proč to nešlo udělat "jako Sega"

```
SEGA a PS1   obraz žije v paměti jádra   ->  Java si ho VYZVEDNE
ATARI        obraz žije v prohlížeči     ->  Java se tam nedostane
```

Sega snímkuje častěji než Atari a nepadá, protože **nic neposílá**.
Atari to udělat nemůže. Moje náhrada — posílat snímky po síti — padala
pořád, ať jsem posílal 16× nebo 8× za vteřinu, ať měl server nová vlákna
nebo zásobu.

V posledním logu snímky **chodily v pořádku** (7× za vteřinu) a stejně
to po minutě spadlo. Takže to nebylo počtem ani vlákny, ale tou cestou
samotnou.

**Byl to můj nápad, ne tvůj požadavek.** Zrušil jsem ho.

## Co je teď

Atari se snímá z okna, **jako v B156, který ti fungoval**.

V souboru Atari zbylo proti B156 **16 řádků aktivního kódu a jsou to jen
klávesy**:

```
ztracené funkce:   žádné
přidané funkce:    žádné
aktivní řádky:     16 (window.napKlavesa)
```

Mrtvý kód po tom posílání jsem vyhodil — 2165 znaků.

## Co ti zůstává

```
klávesnice z počítače, zapne se sama v Atari
intro s pěti etapami a obrazem jader na TV
cesta pro PS1 a Segu shodná s B156
Turbo BASIC, stahování se souhlasem, postup hráče, aktualizace
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| Atari proti B156 | 0 ztracených funkcí, 16 řádků navíc |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Atari + WEB TV** — vydrží? Mělo by se chovat jako B156.
**3)** Klávesnice z počítače.
**4)** PS1, Sega, intro.

---

Kousání Atari s TV, o kterém jsi psal u B156, tímhle nezmizí — to je
starší věc. Ale aspoň to nebude padat a budeš mít klávesnici.

Až budeš mít klid, můžeme se na to kousání podívat s čistou hlavou.
