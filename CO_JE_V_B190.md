# B190 — KLÁVESY V SEZE (versionCode 238)

## Co bylo špatně

Trojice měla fungovat takhle:

```
1. server hlásí     sega=1        ANO, bylo to tam
2. stránka to čte                 NE, tohle chybělo
3. klávesa se pošle               připraveno, ale bez kroku 2 se nespustí
```

**Prostřední kus se do stránky nedostal.** Ta úprava se netrefila do
dlouhého řádku, kde je celý ten skript na jedné řádce — stejná past jako
minule u klávesnice v Atari.

V tvém logu to bylo vidět: odpovědi měly formát `PUSTEN:KeyW`, což je
**Atari**, ne Sega — ta by vrátila `PUSTEN:UP`. Takže se do Segy nic
neposílalo.

Teď to čte obojí. Ověřeno spuštěním:

```
atari=1 sega=0   ->  pošle se
atari=0 sega=1   ->  pošle se
atari=0 sega=0   ->  nepošle
```

## Ovládání

```
ATARI                        SEGA
WASD   pohyb                 WASD   pohyb
K      skok                  K      tlačítko A
L      výstřel               L      tlačítko B
F9     psaní / hraní         O      tlačítko C
F1-F4  START/BREAK/SEL/OPT   P      START
```

V Seze **není potřeba F9** — tam se nepíše, klávesy jdou rovnou do hry.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech stránek | 0 chyb |
| rozhodování podle stránky | 3/3 |
| zvuk Atari proti B185 | shodný |
| cesta PS1 a Segy pro TV proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Sega — pusť Sonica a zkus WASD, K, L, O, P.**
**3)** V rohu má stát `SEGA - WASD pohyb, K = A, L = B, O = C, P = START`.
**4)** Atari má fungovat jako dosud.

Pořád platí: v logu uvidíš `KLAVESA KeyW dolu -> UP`, když to dojde
do Segy. Když tam bude `SMER:KeyW`, jde to pořád do Atari.
