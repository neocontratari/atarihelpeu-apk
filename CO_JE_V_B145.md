# B145 — HLÍDAČ PLOCHY (versionCode 193)

## Co bylo v logu

```
54.740  PLOCHA_POSTAVENA_ZNOVU 720x1384
54.791  PLOCHA_VYTVORENA - predavam ji jadru
54.887  PS1_BIOS_START
55.033  PS1_BIOS_OK
55.038  PLOCHA_ZRUSENA
55.047  PLOCHA_SCHOVANA (obrazovka .../intro/index.html)
```

Pořadí z B144 sedělo. Plocha se postavila správně — a **hned nato ji
někdo schoval**.

V aplikaci je hlídač, který plochu Segy i PS1 schová, když nejsi na
jejich stránce. Chrání před tím, aby obraz prosvítal do zbytku aplikace.
Intro má vlastní adresu, tak ji sundal.

Jádro tedy běželo — proto byl slyšet zvuk — ale kreslilo do schované
plochy. Odtud černá obrazovka a nic na TV.

## Oprava

Hlídač se **nevypíná**. Jen ví, že během živých částí intra má plochu
nechat být.

Ověřeno spuštěním na šesti situacích:

```
                       ukaž PS1   ukaž Segu
hra PS1                  ANO         ne
hra Sega                  ne        ANO
INTRO, živá část         ANO        ANO
INTRO, ostatní scény      ne         ne
rozcestník                ne         ne
Atari                     ne         ne
```

Příznak spadne po skončení každé části a ještě jednou při dokončení intra.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| rozhodnutí hlídače v 6 situacích | 6/6 správně |
| cyklus PS1 | nesaháno |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Nech film běžet — **vidíš obrazovku Segy a po ní obrazovku PS1?**
**3)** Zapni WEB TV a pusť intro — **jde obojí i na TV?**
**4)** Po intru jdi do Atari, PS1 a Segy — musí fungovat jako dosud
       a **nikde nesmí prosvítat cizí obraz**.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | obě obrazovky | černo, jen zvuk |
| 3 | obojí i na TV | černo |
| 4 | čisté obrazovky | prosvítá Sega nebo PS1 |

**Krok 4 je ten, kde buď přísný** — sáhl jsem na hlídač, který přesně
tomuhle bránil.

## CO POSLAT ZPĚT

Log. Nesmí v něm být:

```
PLOCHA_SCHOVANA (obrazovka .../intro/index.html)
```

Když tam bude, hlídač plochu pořád sundává.
