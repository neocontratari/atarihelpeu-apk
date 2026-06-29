BUILD2PB_SEGA_MOBILE_AUDIO_GAIN_TEST_STAGE66

TEST:
1) Nox portrait: overit, ze zvuk zustal cisty a ovladani se nerozbilo.
2) Mobil landscape Sonic/Senna/Probotector:
   - joystick musi fungovat jako v BUILD2PA
   - pohyb + skok soucasne musi fungovat
   - zvuk: porovnat chraplak/ozvenu proti BUILD2PA
3) V logu hledat:
   - BUILD2PB_SEGA_MOBILE_AUDIO_GAIN_TEST_STAGE66
   - AUDIO_TUNE installed latencyHint=playback preferredSampleRate=native ... masterGain=0.62
   - AUDIO_MASTER_GAIN installed value=0.62
   - LANDSCAPE_MULTITOUCH_ENGINE_READY

POCTIVE INFO:
Pokud se zvuk nezlepsi ani pri snizenem WebAudio gainu, problem je pravdepodobne v Android WebView/remote ClownMDEmu audio pipeline a dalsi krok musi byt lokalni core nebo jina audio-core vetev.
