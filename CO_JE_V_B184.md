# B184 — STICK I PEEK (versionCode 232)

Ta tvoje čísla byla důležitá. Ověřil jsem je proti tabulce v Atari
a sedí přesně: **W 46, A 63, S 62, D 58**.

## Proč to bylo potřeba doplnit

V Atari se hry ovládají dvěma způsoby:

```
STICK(0)     čte páku (joystick)
PEEK(764)    čte klávesu       W=46, A=63, S=62, D=58
```

Já v B183 přidal jen tu páku. Kdo by si napsal hru na `PEEK(764)` —
tedy tak, jak to sám děláš — tomu by po zmáčknutí `F9` přestalo ovládání
fungovat.

Teď jde směr **do obojího zároveň**.

## Ověřeno spuštěním

```
klávesa   PÁKA (STICK)    KLÁVESA (PEEK 764)    tys čekal
   W      1  nahoru       46                    46   OK
   A      4  vlevo        63                    63   OK
   S      2  dolů         62                    62   OK
   D      8  vpravo       58                    58   OK
```

Proti skutečné tabulce z Atari, ne proti odhadu.

Skok posílá mezerník do klávesnice i nahoru do páky. Výstřel jde jen
do páky — na klávesnici pro něj Atari nic nemá.

## Zůstává z B183

```
F9   přepne PSANÍ / HRANÍ        F1  START
F2   BREAK                       F3  SELECT
                                 F4  OPTION
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| páka i klávesa proti tabulce Atari | 4/4 |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Napiš si v Atari kousek na `PEEK(764)`, zmáčkni `F9` a zkus WASD.
**3)** To samé s hrou na `STICK(0)`.
**4)** `F1`, `F3`, `F4` — START, SELECT, OPTION.
