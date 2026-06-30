BUILD2PK_SEGA_MOBILE_AUDIO_LOG_SHIELD_NO_INPUT_CHANGE_STAGE75

NEPOUZIVAT BUILD2PJ jako hlavni test, pokud nechceme riskovat input targety. BUILD2PK je navazujici bezpecna vetev: input/joystick zustava z 2PI.

TEST PLAN:
1) Nasad ZIP overlay pres GitHub Desktop.
2) Build APK.
3) NOX test:
   - Sonic nebo Aladdin
   - 20 s intro/hudba bez mackani
   - 30 s hrani: smer + skok/prstynek/mec
   - ulozit LOG pres disketu ULOZENE
4) MOBIL test:
   - stejne jako NOX
   - dulezite: vsimnout si, jestli joystick a skoky zustaly presne stejne jako v 2PI/2PH

OCEKAVANE MARKERY V LOGU:
- BUILD2PK_SEGA_MOBILE_AUDIO_LOG_SHIELD_NO_INPUT_CHANGE_STAGE75
- inputLogShield=YES
- noInputChange=YES
- mobileCleanConfig=YES
- noScriptProcessorPatch=YES
- AUDIO_WATCHDOG_START ... inputLogShield=YES noInputChange=YES

NESMI BYT:
- 4096 samples vs expected 2048
- BUILD2PJ targets=3 shield=YES
- zmena rychlosti joysticku nebo zhorseni hratelnosti

CO POSLAT ZPET:
- LOG z NOXU
- LOG z MOBILU
- kratce: Nox ciste/horsi, Mobil cistejsi/stejne chrci/horsi, joystick stejny/zmeneny
