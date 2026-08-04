# B59 — STARÉ SOFTOVÉ JÁDRO PRYČ (versionCode 107)

## Nejdřív odpověď na tvoji otázku o GPU

**Nic jsem nepřepsal do softu.** Ověřeno v kódu:

    CMakeLists prekladá:   plugins/gpu_naples2/  (jediny GPU plugin)
    linkuje se proti:      EGL, GLESv2
    gpu_neon / gpu_unai:   NEPREKLADAJI SE

Renderer kreslí přes OpenGL ES, obraz vzniká v GPU. Cesta GPU + OpenGL ES +
egl_render je nedotčená.

## Proč byl obraz černý — a proč jsem to tentokrát nemusel hádat

V kódu byla **dvě vlákna emulace** a lišila se jedním krokem:

    nap_core_thread_fn   retro_run()  ->  nap_gles_grab_pixels()   <- vezme obraz z GPU
    nap_worker           retro_run()  ->  nic                      <- obraz z GPU nikdo nesebere

`nap_worker` je z doby, kdy se kreslilo **v softu** a obraz chodil z jádra jako
pixely. S GPU rendererem se do monitoru nedostane nic — proto `gfw=0 gfh=0`.

**To bylo to staré softové jádro.** Tlačítko ISO CD šlo přesně tudy.

## Co je smazané

- `nap_worker` — staré vlákno emulace bez odběru obrazu z GPU
- `g_worker` — jeho vlákno
- `ps1Boot` (JNI) a `bootSafe` (Java) — start, který jádro rozběhl bez grafiky
- z dřívějška: `NativeActivity`, `NativePs1InPlaceView`, `Ps1GlTextureView`
  jako pohled

Zůstává **jediná** cesta: `ps1BootDoMonitoru(systemDir, saveDir, gamePath)`.
Prázdná cesta = start bez disku (BIOS), vyplněná = hra. Stejná grafika,
stejný zvuk, stejné vlákno, stejný monitor.

## Ověřeno překladem, ne odhadem

Nativní soubor jsem přeložil u sebe (`g++ -fsyntax-only` proti náhradním
hlavičkám). **Test našel skutečnou chybu, kterou moje mazání způsobilo** —
zdvojenou deklaraci u `ps1Status`. Opravena. Po opravě se soubor přeloží
bez chyby.

Dál ověřeno:
- všech 11 nativních funkcí, které Java deklaruje, v C++ existuje (1:1)
- na smazané věci nezůstal v Javě ani jeden odkaz
- složené závorky sedí ve všech změněných souborech

Co ověřit nedokážu: překlad přes NDK a chování na telefonu.

## Co testovat

1. PS1 bez disku — BIOS jako dosud.
2. **ISO CD z mobilu** a **LOAD GAME z netu** — hra v monitoru.
3. V logu: `PS1_HRA_DO_MONITORU ... vysledek=PS1_HRA_OK` a hned za tím
   `PS1_GRABFRAME_HEARTBEAT` s **nenulovým** gfw/gfh.
