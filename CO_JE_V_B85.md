# B85 — FORMÁT BODU A SNÍMEK PRO MOBIL (versionCode 133)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Obě vady mají jednu společnou příčinu: změnu vykreslovače

### 1. Dvojitý obraz a zelenofialové barvy

V `nap_video()` stálo:

```cpp
if (g_gles_ready) { g_pixfmt.store(PIXFMT_XRGB8888); }
```

Aplikace si **natvrdo přepisovala formát bodu na čtyři bajty**. Mělo to smysl,
dokud kreslil ručně psaný vykreslovač — ten opravdu posílal čtyři bajty.
Prověřený `gpu_neon` ale posílá **RGB565, dva bajty na bod**.

Četly se tedy dva body místo čtyř → **obraz dvakrát vedle sebe** a rozházené
barvy. Přesně to, co je na tvých screenech z TV.

Formát si hlásí jádro samo a `nap_video` už všechny tři varianty umí. To
přepisování je pryč.

### 2. Na mobilu nic, na TV obraz

Každý bere snímek odjinud:

    nap_video()               -> g_frame_argb   -> odtud bere TV
    nap_ps1_egl_grab_pixels() -> g_frame_buf[]  -> odtud bere MOBIL

`g_frame_buf` plnilo vlákno emulace přes funkci, která patřila ručně psanému
vykreslovači. Po přechodu zůstala prázdná → **na TV obraz byl, na mobilu ne.**

Vlákno emulace teď vezme ten snímek, který jádro opravdu dodá.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | 0 chyb |
| C++ | `clang` pro aarch64, skutečné `jni.h` | 0 chyb |
| jádro + gpu_neon | křížový překlad ARM64 | 0 chyb, ELF ARM aarch64 |
| **symboly** | `nm -u` proti `nm --defined-only` | **všechny sedí** |

---

## CO TESTOVAT

**1)** PS1 bez disku — na výšku i na šířku
**2)** **F1 2000** — ta, co měla kostičky
**3)** **Star Wars**
**4)** Vrať se šipkou do **hlavní nabídky aplikace**

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz **na mobilu i na TV**, jednou, správné barvy | dvojitý obraz / zelenofialové / na mobilu nic |
| 2 | **kostičky pryč**, normální barvy | pořád kostičky |
| 3 | **artefakty pryč** | pořád artefakty |
| 4 | **žádný obraz PS1** v aplikaci | prosvítá boot |

Zvuk už podle tebe výrazně lepší je — hlídej, jestli zůstal.

## CO POSLAT ZPĚT

Očima podle tabulky. Z logu dva řádky:

    VYKRESLOVAC: gpu_neon
    PS1_PIXFMT_CHECK fmt=...     <- ma tam byt fmt=2 (RGB565)
