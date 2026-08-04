# B53 — TEXTURA JEDNOU ZA SNÍMEK (versionCode 101)

## Co jsem rozbil a čím

Od B49 se při **každém kreslicím volání** nahrával do textury celý megabajt
videopaměti. Z tvého logu:

    BIOS:  kresleni=47 az 120 na 120 snimku   ->  ~1 kresleni na snimek
    HRA:   kresleni=11647 na 120 snimku       ->  97 kresleni na snimek

V BIOSu to je 1 MB na snímek — nepoznat. **Ve hře to je ~97 MB na snímek.**
To kouše obraz i zvuk. V BIOSu se to nikdy neprojevilo, proto to prošlo
přes B49, B51 i B52. Upozornil jsem na to riziko v prvním vzkazu a pak jsem
na to zapomněl. Moje chyba.

## Oprava

Videopaměť se do textury nahraje, jen když se od posledního nahrání opravdu
změnila. Příznak nastavuje:

- `n2_vram_written()` — když procesor zapíše do videopaměti
- `n2_novy_snimek()` — na začátku každého snímku

Ten druhý je tam schválně: hry nahrávají textury přes DMA a jádro takové
zapisy vůbec nehlásí. Bez něj by textura zamrzla na prvním snímku. Tím je
zaručené aspoň jedno nahrání za snímek, ale už ne devadesát sedm.

## Změřeno, ne odhadnuto

Renderer přeložen a spuštěn na PC: 200 snímků, 50 kreslení v každém.

    kresleni=10000  nahraniVRAM=200   ->  1,00 nahrani na snimek

Před opravou by to bylo 10 000 nahrání. U tebe ve hře 97 MB/snímek -> 1 MB.

## Aby to příště nešlo přehlédnout

Řádek ZÁTĚŽ nově vypisuje i počet nahrání:

    NAPLES2 ZATEZ za 120 snimku: kresleni=N nahraniVRAM=M vrcholy=...
                                 (na snimek: X vrcholu, Y nahrani po 1 MB)

Když `Y` poleze nad 1, jsme zpátky v tomhle průšvihu a hned to uvidíme.

## Změněné soubory

    .../plugins/gpu_naples2/naples2_gl.c   (nahrávat jen při změně + počítadlo)
    .../plugins/gpu_naples2/naples2_gl.h   (dvě nové funkce)
    .../plugins/gpu_naples2/gpulib_if.c    (začátek snímku + rozšířený řádek ZÁTĚŽ)
    app/build.gradle                       (verze 100 -> 101)

Do ničeho jiného jsem nesahal. Zůstává z předchozích buildů:
odstraněný vadný rámec, evropský region, zavírání mechaniky.

## Grafika BIOSu — kde to stojí

Chování v BIOSu se touhle opravou **nemění**: tam bylo ~1 kreslení na snímek,
takže jedno nahrání za snímek tam bylo i předtím. Zelenorůžová změť přes
MEMORY CARD a CD PLAYER tedy zůstává. Příčinu mám z tvého logu:

    NAPLES2 KONTROLA: nenulovych ve videopameti=107/5406

Paměť, ze které renderer bere textury, je z 98 % prázdná — to, co nakreslí
grafika, se do ní nikdy nevrátí, a na těch místech se čtou staré zbytky
z bootu. Řešení je přepsat `tex_vram` na formát RGBA 5551, do kterého
grafika kreslit smí a který zároveň udrží přesnou šestnáctibitovou hodnotu
pro palety. To je další krok — až bude tenhle build potvrzený jako funkční.

## Co testovat

1. Hra — **jede zvuk a obraz plynule jako dřív?**
2. BIOS bez disku — pořád naskočí a jde ovládat?

## Co mi pošli

Očima: kouše se ještě zvuk nebo obraz ve hře?
Z logu: řádek `NAPLES2 ZATEZ` — zajímá mě `nahraniVRAM` a číslo na konci.
