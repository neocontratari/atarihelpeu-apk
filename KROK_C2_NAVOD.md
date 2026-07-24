# KROK C — PS1 PŘES NATIVNÍ EGL RENDERER (opraveno)

Verze: **EMU10-C2-EGL-PS1** (versionCode 37)

Napojení tvých hotových dílů + oprava tří chyb, které jsem v prvním
napojení sám měl a našel je v kódu, ne po tvém buildu.

## Co dělá

Tlačítko PS1 spustí nativní aktivitu eglrender. Ta si přes `core_ps1.c`
načte jádro (libretro), najde hru, nabootuje a kreslí obraz z jádra
rovnou na displej. **Žádná Java v obrazové cestě.**

## Tři chyby, co jsem opravil (byly moje, ne tvého kódu)

1. **Cesta ke hře.** `core_ps1.c` hledá hru v `ps1/`, ale appka ji měla
   v cache slozce. Ted appka pred spustenim zkopíruje hru (.cue i .bin)
   presne tam, kam renderer kouka.

2. **BIOS.** `core_ps1.c` hledá BIOS v `ps1/bios/`, appka ho měla
   v `ps1_system/`. Ted se pred spustenim zkopíruje na spravne misto -
   takze jede tvuj BIOS, ne náhradní.

3. **Mrtvé předání cesty.** V prvním pokusu jsem cestu ke hře posílal
   intentem, který nikdo nečte. Zahozeno - hra se dava na spravne misto
   primo.

A jednu jsem našel navíc: renderer bere první `.cue` ve složce, takže
se stará hra pred spustenim smaze, aby se nevzala místo nové.

## Co JEŠTĚ nejede (poctivě)

Tvůj `core_ps1.c` je "pouze obraz" - `audio_cb` a `input_state_cb`
jsou v nem prazdne. Takze v teto aktivite:
- **obraz** jede pres novou cistou cestu
- **zvuk** zatim nehraje
- **ovladani** zatim nereaguje

Az obraz potvrdis, doplnim do core_ps1 zvuk a ovladani.

## Pojistka

Kdyby aktivita nesla spustit, appka spadne na starou cestu a napise
proc (`KROKC EGL_PS1_LAUNCH_FAIL`).

## Postup

1. Rozbal → zkopiruj cely obsah pres repozitar → Nahradit vse
2. GitHub Desktop → "krok C2 egl ps1" → Commit → Push
3. Zelena → Artifacts → app-debug → telefon
4. Spust PS1 a hru

## Co uvidis v logu

`KROKC EGL_STAGE_GAME` a `EGL_STAGE_BIOS` - ze se soubory pripravily.
`PS1:` radky z core_ps1 - ze jadro naslo hru a nabootovalo.
