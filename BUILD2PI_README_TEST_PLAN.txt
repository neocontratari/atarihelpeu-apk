AtariHelp.eu EMU-10 BUILD2PI_SEGA_MOBILE_AUDIO_CLEAN_CONFIG_STAGE73

CIL TESTU:
Otestovat jen mobilni zvuk Sega. Grafika, UI, joystick a region kompatibilita musi zustat jako BUILD2PH.

JAK NASADIT:
1) Rozbal ZIP overlay do korene projektu pres GitHub Desktop workflow jako obvykle.
2) Build APK.
3) Nainstaluj na Nox i real Android mobil.

TEST NOX:
1) Otevri SEGA.
2) Vyber Sonic / Aladdin.
3) Over, ze hra nabehne a zvuk neni horsi nez BUILD2PH.
4) Uloz LOG pres disketu ULOZENE.

TEST MOBIL:
1) Otevri SEGA v portrait.
2) Vyber Sonic The Hedgehog nebo Aladdin.
3) Po nabehu nech intro/uvodni hudbu cca 20 s.
4) Zkus pohyb + skok, prstynek / sek mecem.
5) Otoc landscape a over, ze multitouch pohyb + skok porad funguje.
6) Uloz LOG pres disketu ULOZENE.

CO POSLAT ZPET:
- LOG z Noxu.
- LOG z real mobilu.
- Slovne jen: zvuk je cisty / porad chrci / je opozdeny / je ozvena / zhorsilo se.

LOG MARKERY, KTERE HLEDAT:
- BUILD2PI_SEGA_MOBILE_AUDIO_CLEAN_CONFIG_STAGE73
- napAudioMode=mobileCleanConfig
- AUDIO_TUNE installed ... mobileCleanConfig=YES ... noScriptProcessorPatch=YES
- REGION_CONFIG_WRITTEN ... audioClean=YES lowPass=off lowVolumeDistortion=off rewinding=off
- AUDIO_WATCHDOG_START
- AUDIO_WATCHDOG tick=...
- AUDIO_WATCHDOG_DONE

NESMI TAM BYT:
- Web Audio playback buffer length mismatch
- Destination size: 4096 samples vs expected 2048 samples
- BUILD2PE hardGate
- NO_PATCH_PAL_AUTO_ORIGINAL_COPY

POCTIVY STAV:
Tohle neni tvrzeni, ze je mobilni zvuk definitivne opraveny. Je to prvni bezpecna audio-only vetev po BUILD2PH, bez zasahu do UI/region/joysticku a bez rozbiteho buffer patche.
