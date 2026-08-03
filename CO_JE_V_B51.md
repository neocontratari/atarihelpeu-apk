# B51 — RÁMEC PRYČ + EVROPSKÝ BIOS ZPĚT (versionCode 99)

## Dvě změny. Píšu rovnou proč dvě, ne jedna.

### 1) Odstraněn vadný rámec v rendereru

K textuře `tex_vram` (formát `GL_LUMINANCE_ALPHA`) visel rámec (FBO). Do takové
textury grafika kreslit nesmí — rámec byl neúplný, `n2_init()` na tom skončil
a nenakreslilo se nic. V logu `NAPLES2 FBO_VRAM_FAIL stav=0x8cd6`.

### 2) Region u startu bez disku vrácen na EVROPSKÝ (PAL)

B49 to přehodil na americký (US/NTSC). **To bylo zbytečné a škodlivé.**
Region v tomhle jádře rozhoduje, KTERÝ SOUBOR BIOSU se nahraje
(`psxMemReset()` indexuje `Config.Bios[]` právě regionem):

    EU -> scph7502.bin      US -> scph5501.bin / scph1001.bin

Takže B49 ti pod rukama vyměnil BIOS, jehož menu zrovna ladíme, a přehodil
50 Hz na 60 Hz. Přeměřeno na jádře spuštěném mimo telefon, 1200 snímků,
start bez disku, s tvými BIOSy:

| nastavení | snímků s obrazem |
|---|---|
| EU + PAL + `SetCdOpenCaseTime(-1)` | **946 z 1200** |
| US + NTSC + `SetCdOpenCaseTime(-1)` | 890 z 1200 |
| EU + PAL **bez** `SetCdOpenCaseTime` | **0 z 1200** |

Rozhoduje `SetCdOpenCaseTime(-1)` — ten zůstává. Region s tím nemá co dělat.
Číslo 946 v protokolu bylo naměřené na **evropském** BIOSu, ale do aplikace
se pak dal americký. To nesedělo.

Týká se to **jen startu bez disku**, hry to nechává být (je to uvnitř větve
`if (info == NULL)`).

## Změněné soubory

    .../plugins/gpu_naples2/naples2_gl.c    (rámec pryč)
    .../plugins/gpu_naples2/naples2_gl.h    (deklarace smazané funkce)
    .../frontend/libretro.c                 (region zpět na EU/PAL)
    app/build.gradle                        (verze 97 -> 99)
    PREDAVACI_BALICEK.md, CO_JE_V_B51.md    (dokumentace)

Atari, HTML, CSS, workflow — nedotčené.

## Jak jsem to ověřil, než jsem ti to poslal

- Renderer přeložen a **spuštěn** proti napodobenině grafiky, která dodržuje
  stejné pravidlo jako Mali. Starý kód v ní vypsal přesně
  `NAPLES2 FBO_VRAM_FAIL stav=0x8cd6`, nový projde.
- Jádro PS1 **přeloženo a nabootováno** z tohoto balíčku na počítači,
  s tvými BIOSy, start bez disku. Menu BIOSu naběhlo a nakreslilo se.
- Čísla v tabulce výše jsou z toho běhu, ne odhad.

Co ověřit nedokážu: NDK překlad a chování Mali. To pozná až telefon.

## Co testovat

Rozbal, zkopíruj celý obsah přes repozitář, Nahradit vše, Commit + Push,
zelená, `app-debug` do telefonu. Pak **BIOS bez disku** a potom **hru**.

## Co čekat

- V logu **nesmí** být `FBO_VRAM_FAIL`, má tam být `NAPLES2 PRIPRAVEN`.
- Obraz zpátky ve hře.
- V menu BIOSu: **jsou bubliny kulaté, nebo plné bloky?** Posílám ti
  zvlášť obrázek, jak to má vypadat — porovnej tvary, ne barvy.

## Co mi pošli

Očima: obraz ano/ne, bubliny kulaté/bloky, zvuk plynulý/kouše.
Z logu: `VERZE APKY`, `NAPLES2 PRIPRAVEN`, `NAPLES2 KONTROLA`,
`NAPLES2 ZATEZ`, `NAPLES2 TEXTURY`.

Řádek `ZATEZ` potřebuju zvlášť: `kresleni=N` děleno 120 mi řekne, kolikrát
za snímek se nahrává celá videopaměť (1 MB pokaždé). Jednotky = v pořádku,
desítky = další věc k opravě.
