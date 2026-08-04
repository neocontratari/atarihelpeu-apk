# B61 — PŘÍMÁ CESTA, ŽÁDNÝ JPEG (versionCode 109)

## JPEG je pryč. Smazaný, ne vypnutý.

Metoda `ps1FramePreviewB64()` každý snímek zvětšila obraz třikrát, zabalila
ho do JPEGu a poslala do webové stránky jako text:

    PS1_PREVIEW_AVG avgMs=90 srcW=512 srcH=240 outBytes=236396

Devadesát milisekund a čtvrt megabajtu na snímek. Smazané je celé tělo
metody i všech osm pomocných proměnných (bitmapy, plátno, štětec, počítadla).

## Přímá cesta — jak funguje a proč nešla

Je v kódu připravená celá. Klíč je v `nap_gles_egl_init()`:

    g_egl_render_ctx  = eglGetCurrentContext();
    eglCreateContext(display, config, g_egl_render_ctx, ...);   // SDÍLENÝ

Jádro si vezme kontext, který je **na daném vlákně už aktivní**, a udělá si
svůj jako sdílený. Zobrazovací plocha pak vidí jeho texturu přímo.

Jenže jádro se startovalo **na vlastním vlákně, kde žádný kontext nebyl** —
`eglGetCurrentContext()` vrátil prázdno, sdílení nevzniklo a obraz se musel
tahat přes procesor. Přesně to jsi říkal: ta cesta běžela a pak se vypnula.

## Co je v B61

1. Nová nativní funkce `ps1AttachDisplayContext()` — zobrazovací plocha ji
   volá **ze svého vlákna, kde už má svůj GL kontext**. Jádro se tím napojí
   jako sdílené.
2. Nová funkce `ps1GrabTexture(crop)` — vrátí **číslo sdílené textury**
   a výřez obrazovky. Žádné pixely, žádné kopírování.
3. Plocha kreslí tuhle texturu přímo: `glBindTexture` + `glDrawArrays`.
4. **Pořadí startu obrácené**: nejdřív plocha (a její kontext), teprve potom
   jádro. Jinak by sdílení nevzniklo.

Cesta obrazu je tedy: jádro kreslí v GPU → sdílená textura → plocha ji nakreslí
na obrazovku. Procesor se toho nedotkne.

## Zvuk

Prošel jsem log. Zvuk **vlastní chybu nemá** — běží nativně přes OpenSL ES,
Java zvuk je vypnutý a v logu není jediné podtečení fronty. Kousal se proto,
že mu procesor sežral těch 90 ms na obrázek. Tímhle to má odpadnout.

## Ověřeno

- `nap_ps1_native.cpp` se **přeloží** (g++ proti náhradním hlavičkám, 0 chyb)
- závorky sedí v `MainActivity.java`, `Ps1GlTextureView.java`,
  `NativePs1CoreBridge.java`
- po JPEG cestě nezůstal v kódu ani jeden odkaz

Co ověřit nedokážu: NDK překlad a chování Mali.

## Co uvidíš v logu

    PS1_OBRAZ_PRIMA_CESTA priprava=OK sdileno=ANO
    prvni snimek PRIMOU cestou 640x480

Kdyby tam bylo `sdileno=NE` nebo `ZALOZNI_CESTA`, znamená to, že Mali sdílení
odmítlo — pak to poznáme hned z prvního řádku a nebudeme hádat.

## Co testovat

1. Hra na šířku — obraz na displeji telefonu, ovladač nad ním.
2. Plynulost obrazu i zvuku.
3. Na TV pořád obraz bez ovladače.
