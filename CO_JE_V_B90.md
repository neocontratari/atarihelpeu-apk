# B90 — TV SE VRACÍ ZPĚT (versionCode 138)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Co bylo špatně

Když jsi z PS1 odešel do Atari nebo Segy, TV zůstala viset na posledním
snímku z PlayStation.

Důvod: podmínka, podle které si TV bere snímek z jádra, se ptala **jen na
to, jestli běží jádro**:

```java
if (!ps1SessionActive && !ps1GameWindowOwnsCore && !ps1BiosRunning)
    return false;
```

Jádro ale běží dál i po odchodu z obrazovky PS1 — takže si TV pořád brala
jeho snímek a nikdy se nepřepnula.

Teď se ptá i na to, jestli je na obrazovce **opravdu PS1**. Když ne, vrátí
se ke snímání okna aplikace a ukáže, co má. V logu `TV_ZPET_NA_OKNO`.

## Co zůstává na příště

Tvoje dvě poznámky, obě beru:

- **roztažení na šířku není plné HD** — TV kóduje 1280×720 a ořezává černé
  okraje; podívám se, kde se ztrácí rozlišení
- **na výšku obraz hry přesně nesedí do okénka** konzole — plocha leží přes
  celou obrazovku, místo aby se držela toho okénka

Nesahám na ně teď, ať se to nemíchá s opravou TV.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** Spusť PS1, nech naběhnout obraz
**2)** Vrať se šipkou zpět a jdi do **Atari**
**3)** Pak do **Segy**
**4)** Vrať se zpátky do **PS1**

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz na mobilu i na TV | |
| 2 | **na TV je Atari**, ne poslední snímek z PS1 | visí tam PlayStation |
| 3 | **na TV je Sega** | visí tam PlayStation |
| 4 | na TV zase PS1 | nic / stará obrazovka |

## CO POSLAT ZPĚT

Jednou větou, jestli se TV přepíná. Z logu případně `TV_ZPET_NA_OKNO`.
