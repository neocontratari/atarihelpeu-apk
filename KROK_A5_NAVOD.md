# KROK A5 — ODLOŽENÝ BOOT + PŘEPÍNÁNÍ KONTEXTU MIMO ZVUK

Verze: **EMU10-A5-ODLOZENY-BOOT** (versionCode 37)

Log z A4 řekl přesně dvě věci a obě jsou tady opravené.

## Co log ukázal (A4)

```
CESTA_A GLES POZOR: eglrender kontext neni current, sdileni nefunguje
MUJLOG cestaA: vydano=0 prazdno=vse
```

gpu-gles kreslil (canvasTex měl obsah), ale eglrender ho neviděl -
kontexty se nesdílely. A zvuk skřípal.

## Příčina č.1 — boot běžel moc brzy

`core_init` se v eglrenderu volá PŘED tím, než vznikne EGL kontext
(ten se dělá až v APP_CMD_INIT_WINDOW). Takže když gpu-gles bootoval,
`eglGetCurrentContext()` vrátil NIC a sdílení textury selhalo.

**Oprava:** boot cesty A se ODLOŽÍ na první snímek (core_step), kdy
už eglrender kontext existuje a je current. Teď se sdílení povede.

## Příčina č.2 — zvuk skřípal kvůli přepínání v ticku

Přepínání kontextu jsem měl v ticku = na vlákně, kde běží retro_run.
Tím se retro_run zdržel, zvuková fronta se plnila nepravidelně a zvuk
skřípal.

**Oprava:** přepínání kontextu teď dělá RENDER vlákno kolem kreslení,
ne tick. retro_run se nezdrží, zvuk má být plynulý jako v A3.

## Jak to teď teče

```
eglrender vytvori EGL kontext (INIT_WINDOW)
prvni snimek: core_step -> ODLOŽENÝ boot gpu-gles
              (ted uz kontext existuje -> sdileni OK)
kazdy snimek: bind gpu-gles -> retro_run (kresli do canvasu)
              bind eglrender -> nakresli sdilenou texturu na okno
```

## Co čekám v logu

Místo `POZOR: kontext neni current` má být:
- `CESTA_A GLES sdili kontext s eglrenderem (share=...)` - sdileni OK
- `PS1: CESTA A boot OK (textura=ANO) - kontext uz existoval`
- `MUJLOG cestaA: vydano>0` - koencne obraz!

## Postup

1. Rozbal → zkopiruj cely obsah pres repozitar → Nahradit vse
2. GitHub Desktop → Commit → Push
3. Zelena → Artifacts → app-debug → telefon
4. Spust PS1 a hru

## Poctivě

Tohle je oprava toho, co log jednoznačně ukázal - boot moc brzy a
přepínání ve špatném vlákně. Syntaxe C prošla (RC=0), symboly napojené,
závorky sedí. Sdílení GLES1<->GLES2 na Mali ale pořád může mít
záludnost, kterou z kódu nevidím. Log s řádky CESTA_A a MUJLOG cestaA
mi řekne, jestli to teď sedí.

## Co posli
Ocima: je obraz? Je zvuk plynuly (ne skripe)?
Log: radky CESTA_A, MUJLOG cestaA, PS1:.
