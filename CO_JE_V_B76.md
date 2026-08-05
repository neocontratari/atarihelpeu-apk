# B76 — JÁDRO KRESLÍ PŘÍMO NA PLOCHU (versionCode 124)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> Ověřeno je jen to, co jde bez telefonu: překlad Javy proti `android.jar`
> a překlad C++ přes clang pro aarch64. Chování na Mali ověřené není.

## Měl jsi pravdu — a našel jsem i proč to tehdy nešlo

V `eglrender/egl_main.c` je ta přímá cesta přes **sdílenou texturu** a je
**vypnutá**, s tímhle u toho:

```c
//  POZOR: PRIMA CESTA (sdilena textura) je VYPNUTA.
//  Duvod: v B2 byl obraz VIDET. Od B3, kdy jsem prepnul na sdilenou
//  texturu, je obraz cerny nebo roztrhany.
static const int POUZIT_PRIMOU_TEXTURU = 0;
```

Takže sdílená textura tu **nikdy nefungovala** — někdo přede mnou narazil
přesně na to, na co jsem se dvanáctkrát rozbil já. To bylo dobré vědět.

**Co tam fungovalo, byla cesta přes pixely** — a v ní žádný JPEG není:

```c
const void* px = core_get_pixels(&sw, &sh);
glTexImage2D(..., GL_RGBA, sw, sh, ..., px);
glDrawArrays(...);  eglSwapBuffers(...);
```

Chyběla jí jen plocha, na kterou kreslit — brala si ji ze samostatného okna,
které jsme zrušili.

## Co je v B76

Přesně tahle ověřená smyčka, jen dostane plochu z aplikace:

```
jadro -> nap_ps1_egl_grab_pixels() -> GL textura -> obrazovka
```

- `SurfaceView` v aplikaci, leží **pod** webovou stránkou → ovladač zůstává
  nad obrazem a je vidět jen na mobilu
- plocha se předá do C přes `ANativeWindow_fromSurface()`
- kreslí vlastní vlákno, **vlastní EGL kontext, nic se s jádrem nesdílí**
- `nap_ps1_egl_grab_pixels()` nedělá žádné GL — jen vrátí poslední hotový
  snímek, takže odpadá celá třída chyb s přepínáním kontextu
- **JPEG do stránky je odpojený** — vrací prázdný řetězec, dokud plocha běží
- zapíná se pro **BIOS i pro hru**

## Co uvidíš v logu

    PS1_OBRAZ_PRIMO_ZAPNUT (bez JPEG)
    PLOCHA_VYTVORENA - predavam ji jadru
    PLOCHA PRIPRAVENA: obraz jde z jadra rovnou na obrazovku, bez JPEG
    PLOCHA: snimku=300 prazdnych=0 zdroj=512x240 okno=1080x2220

Kdyby plocha nenaběhla, bude tam `PLOCHA: kontext se nepodarilo pripravit`
i s číslem chyby — poznám to hned a nebudu hádat.

## Zůstává z minula

Oprava, že TV bere snímek z jádra i u BIOSu (`ps1BiosRunning`), takže se na
TV nemá objevit ovladač.

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |
| 12 nativních funkcí vs. Java | 1:1 | sedí |

## Co testovat

1. **BIOS bez disku** — obraz na mobilu, na výšku i na šířku.
2. **Hra** — totéž, a plynulost obrazu i zvuku.
3. Na TV nemá být ovladač.
