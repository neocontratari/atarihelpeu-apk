BUILD2PL_SEGA_MOBILE_AUDIO_BALANCED_NATIVE_NO_INPUT_CHANGE_STAGE76

TEST PLAN:
1) Nasad ZIP overlay pres GitHub Desktop.
2) Build APK.
3) NOX: Sonic/Aladdin, 20 s intro bez mackani + 30 s hrani. Uloz LOG.
4) REAL MOBIL: Sonic/Aladdin, 20 s intro bez mackani + 30 s hrani. Uloz LOG.

HLAVNI KONTROLA:
- joystick musi byt stejny jako 2PI/2PK
- pohyb, skoky a rychlost mackani musi byt stejne
- Nox musi zustat cisty
- mobil: sleduj jestli chraplavy/kousavy zvuk zmizi nebo se zmeni

OCEKAVANE LOG MARKERY:
- BUILD2PL_SEGA_MOBILE_AUDIO_BALANCED_NATIVE_NO_INPUT_CHANGE_STAGE76
- mobileBalancedNative=YES
- latencyHint=balanced
- preferredSampleRate=native
- noInputChange=YES
- inputLogShield=YES
- noScriptProcessorPatch=YES

NESMI TAM BYT:
- BUILD2PJ targets=3 shield=YES
- 4096 samples vs expected 2048

POSLI ZPET:
- LOG Nox
- LOG mobil
- kratce: joystick stejny/zmeneny, mobil cistejsi/stejne chrci/horsi, chrci v intru nebo az pri hrani.
