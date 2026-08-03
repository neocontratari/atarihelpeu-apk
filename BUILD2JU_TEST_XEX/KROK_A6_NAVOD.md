# KROK A6 — BOD 2: gpu-gles OBRAZ PŘES PIXELY

Verze: **EMU10-A6-PIXEL-CESTA** (versionCode 37)

To, na čem jsme se shodli. Log z A5 ukázal tvrdý fakt: Mali odmítá
sdílet GLES1 (gpu-gles) s GLES2 (eglrender) kontextem. Takže obraz
jde přes pixely, ne přes sdílenou texturu.

## Co log z A5 ukázal

```
CESTA_A GLES sdili kontext s eglrenderem (share=0x...)   ← timing OK
BUILD2SK98 GLES_INIT_FAIL step=eglCreateContext          ← ale Mali odmitl
```

Odložený boot byl správně (kontext existoval). Ale sdílení GLES1<->GLES2
Mali nedovolí - to je omezení grafiky, ne chyba kódu.

## Bod 2 - jak to obchází

Místo sdílené textury: gpu-gles nakreslí do canvasu, přečteme pixely
(glReadPixels) do bufferu, ten předáme eglrenderu, on z něj udělá
texturu ve svém kontextu a nakreslí. **Pixely nejsou vázané na GL
kontext, takže projdou napříč.**

```
tick: prepni na gpu-gles -> retro_run (kresli do canvasu)
grab: glReadPixels z canvasu -> buffer -> prepni na eglrender
eglrender: nahraj pixely jako texturu -> nakresli na okno
```

## Poctivě: co to je a co není

- **Obraz je ostrý gpu-gles** (s tvým rozlišením) - ANO
- **Bez Javy** - ANO, celé v C
- **Bez procesoru** - NE, jde přes jeden krok v procesoru (glReadPixels)

To je ten kompromis, na kterém jsme se shodli: berlička jen v tom, že
obraz projde jednou procesorem - ne přes Javu. Bod 1 (sdílení bez
procesoru = přepis gpu-gles na GLES2) se udělá potom, až ověříme, že
tohle funguje.

## Zvuk

Přepínání kontextu je v render vlákně, ne v ticku samostatně - takže
retro_run se nezdrží a zvuk nemá skřípat jako v A4.

## Co čekám v logu

- `CESTA_A PIXELS: cteno WxH z canvasFbo` - pixely se čtou
- `MUJLOG cestaA: vydano>0 ... (ostry gpu-gles pres pixely)` - obraz!
- Žádný `GLES_INIT_FAIL` (sdílení už nezkoušíme)

## Postup

1. Rozbal → zkopiruj cely obsah pres repozitar → Nahradit vse
2. GitHub Desktop → Commit → Push
3. Zelena → Artifacts → app-debug → telefon
4. Spust PS1 a hru

## Co posli
Ocima: JE OBRAZ? Je ostry? Je zvuk plynuly?
Log: radky CESTA_A PIXELS, MUJLOG cestaA, PS1:.

## Overeno
Syntaxe C (RC=0), zavorky, napojeni symbolu (grab_pixels), stdlib.h
pro realloc, canvas je RGBA (sedi s glReadPixels), shader uMode=0 bere
RGBA. NDK build ale neprozenu - kdyby spadl, posli cerveny vypis.
