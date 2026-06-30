BUILD2PN_SEGA_MOBILE_AUDIO_PRIME_DIAG_NATIVE_NO_INPUT_CHANGE_STAGE78

CIL TESTU:
Zjistit, jestli realny audio prime pred core zlepsi mobilni chraplani/ozvenu a zaroven v logu zjistit realny ScriptProcessor buffer bez jeho zmeny.

TEST:
1) Nox: Sonic nebo Aladdin, nechat intro 20 s, potom 30 s hrani.
2) Mobil: nejdriv intro bez mackani 20 s, potom hrani 30-60 s.
3) Ulozit LOG z Noxu i mobilu.

OCEKAVANE LOG MARKERY:
- BUILD2PN_SEGA_MOBILE_AUDIO_PRIME_DIAG_NATIVE_NO_INPUT_CHANGE_STAGE78
- mobilePrimeDiagNative=YES
- noPrime=false
- AUDIO_PRIME_START
- AUDIO_PRIME_DONE ok
- AUDIO_SP_DIAG installed noPatch=YES
- AUDIO_SP_DIAG requested=... actual=... noPatch=YES
- noInputChange=YES

NESMI TAM BYT:
- mobileBalancedNative=YES
- mobilePlayback48000=YES
- targets=3 shield=YES
- 4096 samples vs expected 2048

NAPIS ZPET:
- Nox cisty / chrci / horsi
- Mobil cistejsi / stejne chrci / horsi / ozvena
- Joystick stejny / zmeneny
- Sync skok/prstynek lepsi / stejny / horsi
