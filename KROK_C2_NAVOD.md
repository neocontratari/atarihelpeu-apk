# KROK C2 — opraveno podle tvého logu

Parťáku, log byl zlatý. Našel jsem v něm přesnou příčinu obou věcí,
co jsi hlásil. Obojí je opravené.

## 1) FPS — náš obraz byl v pohodě, vinu měl někdo jiný

V logu: přesně 300 snímků každých 5,0 vteřiny = **60,0 FPS**. Náš obraz
tedy jel naplno. Ale zároveň tam bylo `PS1_NATIVE_TEXTURE_SLOW_AVG
avgCostMs=31–57` — **stará zobrazovací cesta běžela dál pod naším
obrazem**, neviditelně, a na jeden snímek si brala 31 až 57 ms
(rozpočet je 16,6). Rvala se s námi o procesor a o jádro.

Oprava: když zapneš náš obraz, stará cesta se teď vypne (použil jsem
tvoji vlastní funkci `ps1DeactivateNativeView`). Když ho vypneš, zase
se zapne. Uvolní se tím výkon.

## 2) TV cast — a tady jsi to ty měl původně správně

V logu bylo po zapnutí našeho obrazu 8282× `TV_WEB_DARK_FRAME_SPIKE
brightAvg=0` — na TV šla úplná čerň. Důvod: použil jsem `GLSurfaceView`,
což je samostatná hardwarová vrstva, kterou snímání obrazovky
(PixelCopy) neumí zachytit — vidí tam díru.

**Tvoje původní volba `TextureView` byla pro cast správná.** Ten se
kreslí do okna aplikace, takže ho cast zachytí. Tak jsem to udělal po
tvém: obraz teď jede přes TextureView, ale s naším vlastním EGL/OpenGL
kontextem — takže si držíme double buffering, vsync a roztažení na GPU,
a cast to zase uvidí.

## 3) Zvuk

Podle tvého pokynu neřeším. Až řekneš, podíváme se na to.

## Jak to vyzkoušet

1. Rozbal ZIP → zkopíruj CELÝ obsah přes repozitář → **Nahradit vše**.
   Commit „krok C2" → Push → zelená → APK do mobilu.
2. Verze: **EMU10-C2-TEXTUREVIEW-GL**
3. Spusť PS1 hru → klepni na logo NaP vlevo nahoře → obraz se přepne.
   Dalším klepnutím zpět.
4. **Zkus i cast na TV** — teď by měl obraz vidět.

## Co mi pošli

- Byl build zelený?
- Jede obraz plynuleji než minule (bez toho půlkování)?
- Vidí TV cast náš obraz?
- Log z 8765/log — hledám řádky „C2".
