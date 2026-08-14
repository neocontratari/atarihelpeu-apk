# B94 — TV BEZ JAVY (versionCode 142)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> B92 je pořád ten funkční základ — tenhle build na něm mění **jen cestu
> obrazu na TV**. Jádra, zvuku ani obrazu na mobilu se nedotýká.

## Měl jsi pravdu, B93 byla půlka práce

Tam jsem vyhodil jednu kopii ze tří. Tady je Java z té cesty **pryč celá**.

**Dřív:**

    jadro -> Java pole -> druhe Java pole -> Bitmap -> Canvas -> enkoder

Tři kopie snímku v Javě. U 640×480 při 30 snímcích za vteřinu **105 MB
za vteřinu jen na přesýpání** — a procesor přitom sdílí s emulací.

**Teď:**

    jadro -> GL textura -> enkoder

Vstupem enkodéru je `Surface` a na Surface umíme z C kreslit — je to ta
samá cesta, jakou jde obraz na displej telefonu. Nativní vlákno na něj
kreslí přes OpenGL ES.

**Java na té cestě snímek nedrží ani nekopíruje.** Odpadlo:

- tři kopie snímku (105 MB/s)
- doostřování bod po bodu (už vypnuté v B92)
- `Canvas.drawBitmap` a ořez černých okrajů

## Pojistka

Kdyby prohlížeč nezvládl H.264 a spadl na JPEG zálohu, enkodér skončí —
nativní kreslení se odpojí a javová cesta se vrátí, aby TV nezůstala černá.
V logu `TV_PRIMO_VYPNUTO`.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |
| názvy nativních funkcí vs. Java | 1:1 | všechny sedí |

---

## CO TESTOVAT

**1)** **Hra na mobilu bez TV** — kontrola, že je to pořád jako B92
**2)** Zapni **TV** a nech hru běžet
**3)** Zkus něco náročného (**NFS**, **F1**)
**4)** Vrať se z PS1 do **Atari** nebo **Segy**

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | jako B92 — obraz a zvuk bomba | zhoršilo se |
| 2 | obraz na TV je, správné barvy a otočení | černo / obráceně / divné barvy |
| 3 | **se zapnutou TV skoro žádný rozdíl** proti kroku 1 | pořád se kouše |
| 4 | na TV se přepne Atari/Sega | visí tam PS1 |

Krok 3 je ten hlavní. Krok 2 hlídej i na barvy a otočení — je to nová cesta
a přesně tam se to dvakrát rozešlo.

## CO POSLAT ZPĚT

Očima podle tabulky. Z logu tři řádky:

    TV_PRIMO_ZAPNUTO
    TV_PRIMO PRIPRAVENO
    TV_PRIMO: snimku=...

Kdyby byla TV černá, hledej `TV_PRIMO: kontext se nepodarilo pripravit`
— bude tam i číslo chyby.
