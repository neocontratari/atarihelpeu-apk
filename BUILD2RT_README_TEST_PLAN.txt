BUILD2RT_SEGA_NATIVE_CPP_ONLY_REGION_MEMORY_AUDIT_STAGE136

UCEL:
- Tvrdy audit Sega C++ cesty pred dalsim ladenim zvuku.
- Overit, jestli se do C++ nemicha stary Java/WebView wrapper/audio.
- Overit PAL/NTSC auto timing pro ROM header.
- Overit STEREO/MONO audio cestu pro stare mobily.
- Overit, kde vzniká levy graficky pruh: raw framebuffer vs TextureView.

KODY JSOU ZMENENE:
- app/src/main/cpp/nap_sega_native_proof.cpp
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java
- app/src/main/java/eu/atarihelp/emu10/NativeSegaCoreBridge.java
- app/src/main/assets/emu_sega/index.html

TEST 1 - NOX SONIC STEREO:
1. Nainstaluj APK.
2. Otevri Sega.
3. SBIRKA nech HIGH, AUDIO STEREO.
4. CARTRIDGE -> Sonic The Hedgehog (USA, Europe).gen.
5. Otestuj zvuk, odezvu cinknuti/skoku, levy pruh.
6. Uloz normalni log + C++ log.

TEST 2 - S8 SONIC STEREO:
1. Stejny test na Samsung S8.
2. Sleduj Marble/Spring/bonus, jestli narusta zpomaleni a chrasteni.
3. Uloz oba logy.

TEST 3 - S8 MONO AUDIT:
1. V Sega zmackni SBIRKA.
2. Zvol AUDIO MONO TEST.
3. Vyber ROM znovu pres CARTRIDGE, aby se AudioTrack zalozil jako mono.
4. Otestuj Sonic stejne jako predtim.
5. Uloz oba logy.

DULEZITE MARKERY:
- BUILD2RT_SEGA_NATIVE_CPP_ONLY_REGION_MEMORY_AUDIT_STAGE136
- FM_PSG_REGION_MEMORY_AUDIT_RT
- PURE_CPP_SEGA_PATH=YES
- JAVA_SEGA_WRAPPER_RUNNING=NO
- WEB_AUDIO_RUNNING=NO
- LEGACY_JAVA_AUDIO_THREAD=NO
- NATIVE_WORKERS_ACTIVE=1
- nativeActiveAudioTracks=1
- NATIVE_AUDIO_STREAM_START_RT_REGION_MEMORY_AUDIT_QT
- audioMode=STEREO nebo audioMode=MONO
- PAL_EU_50_OVERSEAS reason=JUE_APP_DEFAULT_EU / ROM_E
- NTSC_US_60_OVERSEAS reason=ROM_U_OR_FALLBACK
- framebufferStride=320
- rawScanlineCopy=YES
- leftBoundaryShift=NO
- left8Checksum=0x...
- audioPullAvgUs / audioWriteAvgUs
- lockWaitMs / mixMs
- audioRingAudit=FIXED_RING_NO_DEQUE
- regionMemoryAudit=YES
- regionClockLocked=YES

CO SE NESMI DIT:
- Nesmí bezet Java/WebView ClownMDEmu wrapper.
- Nesmí bezet vice AudioTracku.
- Nesmí byt hard drop cesta z RP.
- Nesmí byt fake rumble/bass filtr.
- Nesmí se spoustet ROM automaticky po startu bez CARTRIDGE.

CO POSLAT ZPATKY:
- Nox STEREO logy.
- S8 STEREO logy.
- Pokud S8 porad chrci, S8 MONO logy.
- Screenshot leveho pruhu jen pokud se zmenil.
