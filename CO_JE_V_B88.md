# B88 — PROBLIKÁVÁNÍ PRYČ (versionCode 136)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Co jsi viděl

Střídal se správný snímek s jedním, který byl **otočený o 180 stupňů
a červený místo modrého**. Na tvých dvou screenech je to vedle sebe —
na druhém je MAIN MENU vpravo dole a zrcadlově, CD PLAYER nad MEMORY CARD
a celý obraz do červena.

## Proč

V kódu byli **dva zapisovatelé téhož snímku**:

```
nap_video()                 -> zapise snimek SPRAVNE (RGB565 -> ARGB, shora dolu)
nap_publish_frame_for_app() -> zapise ho ZNOVU podle stareho zvyku
```

Ta druhá funkce dělá tohle:

```c
const uint8_t* src = rgba + (h - 1 - y) * w * 4;   // OTOCENI RADKU
dst[x] = src[0]<<16 | src[1]<<8 | src[2];          // PROHOZENI R a B
```

Bylo to správně, dokud snímek chodil od ručně psaného vykreslovače — ten ho
dodával zdola nahoru a v jiném pořadí barev. Prověřený `gpu_neon` ho dodává
už hotový, takže ho ta funkce jen **znovu otočila a prohodila barvy**.

Podle toho, který zápis stihl přijít dřív, se ukázal správný nebo obrácený
snímek — odtud to rychlé problikávání.

Teď se volá jen tehdy, když snímek nepřišel z jádra.

## Co potvrdil tvůj log

    PS1_PIXFMT_CHECK fmt=2 (RGB565)

Formát bodu je správně — ta oprava z minula sedla. Proto je obraz i na výšku
na mobilu a grafika čistší.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| symboly | `nm -u` proti `nm --defined-only` | všechny sedí |

---

## CO TESTOVAT

**1)** PS1 bez disku — na výšku i na šířku
**2)** **F1 2000**
**3)** **Star Wars**

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz **klidný, neproblikává**, správné barvy | střídá se otočený/červený |
| 2 | klidný obraz, kostičky pryč | problikává |
| 3 | klidný obraz, artefakty pryč | problikává |

Zvuk už čistý je — hlídej, jestli zůstal.

## CO POSLAT ZPĚT

Očima podle tabulky. Hlavně jestli **problikávání zmizelo**.
