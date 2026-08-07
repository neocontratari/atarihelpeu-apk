# B89 — NA MOBILU SPRÁVNĚ (versionCode 137)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**

## Co jsi popsal

Problikávání zmizelo, zvuk čistý, **na TV je obraz správně** — ale **na
mobilu je vzhůru nohama a s červenou místo modré.**

To, že jedno je správně a druhé ne, byla ta nejcennější informace. TV a
mobil totiž berou obraz jinudy, takže chyba musela být v té části, kterou
používá jen mobil.

## Dvě věci, obě v kreslicí smyčce plochy

**1. Otočení.** Plocha čekala snímek **zdola nahoru** — tak ho dodával
ručně psaný vykreslovač (šel z `glReadPixels`). Z jádra přes `nap_video()`
chodí **shora dolů**. Souřadnice textury jsou proto obrácené.

**2. Barvy.** Snímek je v pořadí **ARGB** (v paměti B,G,R,A), ale nahrával
se jako **RGBA** → prohozená červená a modrá. Stínovač je teď prohodí zpět:

```glsl
gl_FragColor = vec4(texture2D(uTex, vUV).bgr, 1.0);
```

Zapsal jsem to do předávacího balíčku jako věc, kterou je nutné zkontrolovat
pokaždé, když se změní zdroj snímku — protože přesně tohle nás teď zdrželo
dvakrát.

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
| 1 | na mobilu obraz **správně otočený a v modré**, stejně jako na TV | vzhůru nohama / červený |
| 2 | totéž, kostičky pryč | |
| 3 | totéž, artefakty pryč | |

Problikávání a zvuk už podle tebe v pořádku jsou — hlídej, jestli zůstaly.

## CO POSLAT ZPĚT

Jednou větou: sedí obraz na mobilu s tím na TV?
