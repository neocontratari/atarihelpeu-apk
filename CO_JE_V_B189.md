# B189 — KLÁVESNICE I PRO SEGU (versionCode 237)

M�l jsi pravdu, že to nestálo za to. **Alt a Ctrl jsou pryč.**

## Atari

```
WASD   pohyb
K      skok      (nebo mezerník)
L      výstřel
F9     přepne psaní / hraní
F1     START     F2  BREAK
F3     SELECT    F4  OPTION
```

## Sega

Sega už všechno uměla — `sendToWrapper()` posílá tlačítko do jádra a to
zná `UP`, `DOWN`, `LEFT`, `RIGHT`, `A`, `B`, `C` a `START`. Chybělo jen
okno ven, aby se na to dalo sáhnout z klávesnice.

```
WASD   pohyb
K      tlačítko A
L      tlačítko B
O      tlačítko C
P      START     (nebo Enter)
```

Žádné přepínání — v Seze se nepíše, takže klávesy jdou rovnou do hry.

## Ověřeno spuštěním proti skutečnému kódu

```
ATARI
  W nahoru   -> páka=1        K skok     -> páka=1
  A vlevo    -> páka=4        L výstřel  -> střelba=true
  S dolů     -> páka=2
  D vpravo   -> páka=8

SEGA
  W -> UP      K -> A       jádro dostalo: A DOWN
  A -> LEFT    L -> B       jádro dostalo: B DOWN
  S -> DOWN    O -> C       jádro dostalo: C DOWN
  D -> RIGHT   P -> START   jádro dostalo: START DOWN
```

Ne proti napodobenině — vzal jsem pravé funkce z obou stránek.

## PS1 zatím ne

Nechal jsem ji, jak jsi říkal. Vstup má (`ps1SetButton`), ale její
tlačítka jsou jinak očíslovaná než u Segy a chci si to nejdřív pořádně
přečíst, ne odhadnout.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari, Segy i všech etap | 0 chyb |
| ovládání proti pravému kódu | 14/14 |
| zvuk Atari proti B185 | shodný |
| cesta PS1 a Segy pro TV proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Atari — `F9`, pak WASD, `K` a `L`.
**3)** **Sega — pusť Sonica a zkus WASD, K, L, O, P.**
**4)** V rohu má stát, co se dá mačkat.
