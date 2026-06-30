BUILD2PM TEST PLAN - SEGA MOBILE AUDIO PLAYBACK 48000 NO INPUT CHANGE

CIL:
- Overit, jestli explicitni 48000 Hz pri playback latency odstrani chrceni bez toho, aby se sahalo na joystick.
- 2PL balanced bereme jako spatny smer pro cistotu, protoze chraptel i Nox.

TEST:
1) Nasad ZIP overlay pres GitHub Desktop.
2) Build APK.
3) NOX: Sonic/Aladdin, 20 s intro + 30 s hrani.
4) MOBIL: Sonic/Aladdin, nejdriv 20 s intro bez mackani, potom 30 s hrani.
5) Uloz LOG z Noxu i mobilu.

OCEKAVANE LOG MARKERY:
- BUILD2PM_SEGA_MOBILE_AUDIO_PLAYBACK_48000_NO_INPUT_CHANGE_STAGE77
- napAudioMode=mobilePlayback48000
- latencyHint=playback
- preferredSampleRate=48000
- mobilePlayback48000=YES
- noInputChange=YES
- noScriptProcessorPatch=YES

NESMI TAM BYT:
- BUILD2PL mobileBalancedNative=YES
- latencyHint=balanced
- 4096 samples vs expected 2048

NAPIS ZPET:
- Nox: cisty / chrci / horsi
- Mobil: cistejsi / stejne chrci / horsi / ozvena
- Joystick: stejny / zmeneny
- Sync skok/prstynek: lepsi / stejny / horsi
