# INTEL: aplikace emu10 (AtariHelp.eu) – vše zjištěné z APK a logů

Zdroj: app-debug.apk uživatele (rozborem manifestu, dex a .so)
+ provozní logy z jeho zařízení. Vše níže je ověřený fakt.

## Aplikace
- package: eu.atarihelp.emu10, název „AtariHelp.eu"
- versionName vzor: „EMU10-BUILD2RY-SEGA-CORE-O3-VDP-LEFTBLANK", minSdk 24, targetSdk 34
- Jedna Java aktivita: MainActivity (WebView hub) + vnitřní třídy:
  AHNet, AHPS1, AHPick, AHSave, AHTvWeb, AHNative, SegaExtract,
  AtariExtract, FetchResult, NativeSegaProofActivity,
  NativePatternView, NativeInPlaceView, NativePs1InPlaceView
- Java ServerSocket + endpoint „/log" → webový log server na 8765
  (stejný port používá i nový renderer; běží ten, kdo dřív obsadí)
- TV zrcadlení: „AHTV WEB CAST" – vlastní web stream z aplikace
  (TV otevře stránku servírovanou telefonem), čte framebuffer jádra
- Oprávnění: INTERNET, READ/WRITE_EXTERNAL_STORAGE, READ_MEDIA_AUDIO
- Assets: index.html + emu10_130xe.html a další (Atari 8bit web emu)

## PS1 (libnapps1core.so – POUZE arm64-v8a, ~3 MB)
- Je to PCSX-ReARMed. Exportuje KOMPLETNÍ libretro API:
  retro_set_environment, retro_set_video_refresh,
  retro_set_audio_sample(+_batch), retro_set_input_poll,
  retro_set_input_state, retro_init, retro_load_game, retro_run,
  retro_get_system_info, retro_get_system_av_info, retro_api_version,
  retro_reset, retro_serialize(_size), retro_unserialize, …
- Navíc vlastní JNI most (třída NativePs1CoreBridge):
  ps1Boot, ps1CoreInfo, ps1GrabFrame, ps1LoadState, ps1PullAudio,
  ps1SaveState, ps1SetInput, ps1Status, ps1Stop
- Uvnitř: dynarec (new_dyna*/ndrc_*), softwarový GPU „unai/neon"
  (žádné NEEDED libEGL/libGLESv2 – GLES si bere přes dlopen),
  HLE BIOS (psxBios_*), podpora cue/bin/chd, SPU, memory karty.
- Z provozních logů emu10:
  - jejich cesta obrazu: EGL 1.4 PBUFFER 1024×768 (offscreen) →
    glReadPixels 3–4,7 ms → software flip ~1 ms → Java View
    (NativePs1InPlaceView) → to je zdroj blikání
  - formát „fmt=1" = XRGB8888 v jejich readbacku
  - tik jádra: avgTotalMs 2,5–5 ms; audio buffer 8192, underruns=0
  - BIOS „adoptován": 4 soubory do files/ps1_system (privátní složka
    emu10); zdroj BIOSu je na webu (PS1-BIOS_.zip), NENÍ nutný (HLE)
  - hry: /storage/emulated/0/Download/AtariHelp/PS1/gdrive_<id>/…
    ověřeno: Crash_Bandicoot.cue + .bin

## Sega (libnapsega_native_proof.so – arm64-v8a, armeabi-v7a, x86, x86_64)
- JNI most (třída NativeSegaCoreBridge), pozorované funkce:
  realCoreLoadRom, realCoreStep, renderPattern, pullAudio /
  pullAudioStereo, setInput, buildString, realCoreStatus
  (úplný přesný seznam: `nm -D --defined-only` na .so)
- Render: nativní kód plní buffer, Java si ho tahá a kreslí
  (NativePatternView / NativeInPlaceView) – opět bez vsyncu
- Řetězec ověřen jejich „proof" logy: Java → JNI → C++ →
  ROM/input/audio/render/log

## Zařízení uživatele
- arm64, Android 9 (API 28), GPU Mali-G71 (EGL 1.4, GLES 3.2)
- displej 720×1384 (na šířku 1384×720), 60 Hz
- IP v domácí síti bývala 192.168.0.34 (může se změnit)

## Distribuce her (web uživatele)
- https://atarihelp.eu – PS1 hry: ?page_id=1048 (Crash Bandicoot,
  Doom, Tekken 3, Metal Gear Solid, Gran Turismo, …), Sega:
  ?page_id=1003, rozcestník her: ?page_id=207
- Odkazy vedou na Google Drive → emu10 je stahuje do
  Download/AtariHelp/<SYSTÉM>/gdrive_<id>/ (odtud názvy složek)

## Repozitář
- github.com/neocontratari/atarihelpeu-apk (soukromý)
- Workflowy: „AtariHelp_eu_EMU10_BUILD1" (historicky 1367 běhů;
  nyní vypnut Pojistkou) a „Build APK" (aktivní, staví renderer)
- Poslední stav emu10 zdrojáků = commit d20e7bd (běh #1076,
  „PREDAVACI_BALICEK_2026-07-22_SK154"); renderer nasazen commitem
  1e2a7af („egl-render") – emu10 je obnovitelné z historie
  (commit → Browse files → Code → Download ZIP)
