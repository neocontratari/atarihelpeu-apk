# KROK A — gpu-gles OSTRÝ OBRAZ ČISTOU CESTOU

Verze: **EMU10-A1-GPUGLES-CISTA-CESTA** (versionCode 37)

Sto procent, cos chtěl: gpu-gles s dvojnásobným rozlišením, protažené
čistou cestou přes eglrender, bez Javy v obrazové cestě.

## Cesta A - jak to teď teče

```
eglrender (jeho vlakno, jeho GL kontext sdileny s gpu-gles)
  → nap_ps1_egl_boot_c: gpu-gles init + retro_init + load_game
  → kazdy snimek: nap_ps1_egl_tick_c = retro_run + gpu-gles kresleni
  → nap_gles_grab_texture: vezme HOTOVOU gpu-gles texturu (id+vyrez)
  → egl_main nakresli tu texturu primo na displej
```

**Žádný glReadPixels. Žádný prenos do procesoru. Žádna Java. Žádna
ctecka.** Obraz z gpu-gles jde rovnou na obrazovku - ostry, s tvym
zdvojnasobenym 3D rozlisenim.

## Co jsem sahnul do tveho jadra (s tvym svolenim)

**gpulib_if.c** - pridal jsem `nap_gles_grab_texture`: udela levnou
GPU->GPU kopii canvasu do snapshot textury (jako present_frame, ale
BEZ ctecky) a vrati id+vyrez. Nezmenil jsem, jak gpu-gles kresli -
jen jsem pridal dvirka, kterymi se ta hotova textura da vzit zvenku.

**nap_ps1_native.cpp** - pridal jsem `nap_ps1_egl_boot_c/tick_c/grab`:
boot a tick gpu-gles rizeny z eglrender vlakna (varianta 1 - stary
worker se pro tuhle cestu nespousti, jedno vlakno vola retro_run,
zadna kolize).

## Varianta 1 - stary worker se obchazi

Jak jsi rozhodl. eglrender prebira cely boot. Stara Java cesta
(grabFrameSafe, ps1AudioThread) i stary worker se pro PS1 nespousti.
Jedno vlakno, zadne hadani obraz/zvuk.

## Pojistka

Kdyby gpu-gles cesta nenabehla, core_ps1 spadne na softwarovou cestu
a napise proc. A kdyby ani ta ne, na demo. Zadna cerna obrazovka.

## MUJ LOG - abych videl co ty

- `MUJLOG cestaA: vydano=X prazdno=Y vyrez=WxH (ostry gpu-gles)` -
  kolik snimku gpu-gles vydalo a jaky vyrez. Kdyz "prazdno" roste,
  obraz stoji.
- `CESTA_A BOOT_OK gpuGles=ANO` - gpu-gles cesta nabehla
- `PS1: CESTA A boot OK (textura=ANO ostry gpu-gles)` - jede ostry obraz

## Postup

1. Rozbal → zkopiruj cely obsah pres repozitar → Nahradit vse
2. GitHub Desktop → "krok A1 gpu-gles cista cesta" → Commit → Push
3. Zelena → Artifacts → app-debug → telefon
4. Spust PS1 a hru

## Co uvidis / co mi posli

Ocima: je obraz ostry (dvojnasobne 3D)? Bez blikani? Bez artefaktu?
Log: radky `CESTA_A`, `MUJLOG cestaA`, `PS1:`.

## Overeno pred odeslanim

Syntaxe C prosla (gcc -fsyntax-only, RC=0). Vsech pet symbolu ma
shodu dlsym<->definice. Zadne -fvisibility=hidden (symboly viditelne).
Extern "C" (dlsym je najde). Zavorky sedi. NDK build ale neprozenu -
kdyby spadl na NDK-specificke drobnosti, posli cerveny vypis.
