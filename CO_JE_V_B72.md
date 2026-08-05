# B72 — GRAFIKA BIOSU OPRAVENÁ (versionCode 120)

**Ověřeno obrázkem, ne domněnkou.** Přiložený `BIOS_pred_a_po.png` je vlevo
předtím, vpravo potom — obojí z rendereru v tomhle projektu, na skutečném
OpenGL ES.

## Co to dělalo

BIOS si tu bublinu **nekreslí** stranou. On si ji **přečte z obrazovky**
a uloží si ji vedle (na stránku 704), aby ji pak použil jako texturu.

Jenže když jádro čte z videopaměti, čte z paměti procesoru — a obraz kreslí
grafika, takže se tam nikdy nedostane. BIOS si tedy přečetl prázdno a na
stránce 704 zůstalo to, co tam leželo předtím: **logo SONY**. To se pak
natáhlo přes MEMORY CARD a CD PLAYER jako ta zelená kaše.

Změřeno: v paměti referenčního rendereru je na stránce 704 modrá koule,
MAIN MENU a barevný přeliv. V paměti tohohle rendereru tam bylo logo.

## Oprava

V `plugins/gpulib/gpu.c`, ve funkci `do_vram_io`, před čtením:

```c
if (is_read)
    n2_readback_to_vram(x, y, w, h);
```

Nová funkce `n2_readback_to_vram()` v `naples2_gl.c` přečte danou oblast
z obrazu v GPU a zapíše ji do paměti jádra ve správném formátu (5 bitů na
barvu). Bere se **jen ta oblast, o kterou si jádro řeklo**, ne celá paměť.

## A druhá věc, bez které by to bylo taky špatně

Při vracení obrazu se **bit masky musí nechat na nule**. V obraze je totiž
v průhlednosti uložená míra průhlednosti kreslení, ne bit masky konzole.
Když se tam zapsala jednička, přestal být černý bod nulový — a černé okolí
bubliny se kreslilo jako **černý čtverec** místo aby bylo průhledné.
(Nulový texel = průhledný, to je pravidlo PlayStation.)

## Výsledek

Odchylka od referenčního rendereru klesla z 15,0 na **12,1** ze 255,
a hlavně: bubliny jsou kulaté, přeliv za položkami menu je barevný a
zelená kaše je pryč.

## Dál je v balíčku

- oprava lomených čar z B70 (proměnná délka příkazu)
- hra jde do stejného monitoru jako BIOS (ISO CD i LOAD GAME), samostatné
  okno na šířku a stará softová větev `nap_worker` smazané

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |
| jádro PS1 pro ARM64 vč. assembleru | `aarch64-linux-gnu` | **0** |
| renderer | přeložen **a spuštěn** na skutečném OpenGL ES | viz obrázek |
