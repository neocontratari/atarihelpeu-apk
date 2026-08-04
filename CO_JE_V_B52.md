# B52 — ZAVŘÍT MECHANIKU (versionCode 100)

## Co bylo rozbité a čí to byla chyba

Po spuštění hry naskočil BIOS bez CD místo hry. **Způsobil to build, který
jsem ti poslal** — konkrétně řádek, který přišel z B49 a já ho v B51 nechal být.

`SetCdOpenCaseTime(-1)` znamená „víko mechaniky je otevřené, disk žádný".
Nastavuje ho větev „start bez disku". Jenže `cdOpenCaseTime`
(`libpcsxcore/plugins.c`) je **statická proměnná živá po celou dobu běhu
procesu** a nikdo ji nikdy nevrací zpátky — neresetuje ji `retro_deinit()`,
`retro_init()` ani `SysReset()`.

Takže: appka při startu nabootuje BIOS bez disku → víko zůstane natrvalo
otevřené → pak spustíš hru → jádro disk načte, ale emulovaná mechanika pořád
hlásí „víko otevřené" (`CDR__getStatus` vrací 0x10) → BIOS disk nikdy neuvidí
a skončí ve svém menu. **Místo hry.**

To taky vysvětluje, proč ty dva BIOSy vypadají jinak: první je evropský
(50 Hz, 640×512), druhý spadne do jiné větve a je americký (60 Hz, 640×480).

## Oprava

Na začátku `retro_load_game()` se mechanika vždycky zavře:

    SetCdOpenCaseTime(0);

Větev bez disku si ji hned níže zase otevře. Každé načtení tak začíná ze
známého stavu.

## Jak jsem to dokázal, ne tvrdil

Postavil jsem jádro se sondou, která stav mechaniky vypíše při vstupu do
`retro_load_game`, a projel tvoji posloupnost — nabootovat, jádro úplně
vypnout (`retro_unload_game` + `retro_deinit`), zapnout a načíst znovu:

    1. nacteni: mechanika ZAVRENA (0x00)      <- cerstvy proces
    2. nacteni: mechanika OTEVRENA (0x10)     <- priznak prezil vypnuti jadra
       po oprave:  ZAVRENA (0x00)

Ten příznak přežije i kompletní vypnutí jádra. To je přesně tvůj případ.

## Změněné soubory

    .../frontend/libretro.c    (zavřít mechaniku na začátku načtení)
    app/build.gradle           (verze 99 -> 100)

Renderer jsem tentokrát nechal být — z B51 zůstává odstraněný vadný rámec
(ten funguje, v logu už není `FBO_VRAM_FAIL`) a evropský region.

## Co JEŠTĚ NENÍ opravené

Ta zelenorůžová změť přes MEMORY CARD a CD PLAYER. Příčinu mám z tvého logu:

    NAPLES2 KONTROLA: nenulovych ve videopameti=107/5406

Videopaměť, ze které renderer bere textury, je z 98 % prázdná. Renderer
texturuje z paměti, kam zapisuje procesor — ale to, co nakreslí grafika, se
tam nikdy nevrátí. Na těch místech pak čte staré zbytky z bootu (nápis SONY
COMPUTER ENTERTAINMENT). Řešení je přepsat `tex_vram` na formát RGBA 5551.
To je větší zásah a nechávám ho na samostatný build.

## Co testovat

1. Spusť PS1 — naskočí BIOS bez disku (to je správně, je to úmysl).
2. **Vyber hru.** Musí naskočit HRA, ne druhý BIOS.
3. Kdyby přesto naskočil BIOS, pošli log — bude v něm vidět, kde to vázne.

## Co mi pošli

Očima: naskočila hra, nebo zase BIOS? Jde obraz? Jde zvuk?
Z logu: `VERZE APKY`, `CESTA_A BOOT_OK`, `NAPLES2 KONTROLA`, `NAPLES2 OBRAZ`.
