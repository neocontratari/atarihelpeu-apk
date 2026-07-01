BUILD2QY TEST PLAN

Aplikuj overlay pres BUILD2QX/QV aktualni projekt.

1) Nox sanity test
- Otevri SEGA MEGA DRIVE.
- C++ CORE funkcni tlacitko ani cesta nesmi byt potreba.
- SBIRKA/CARTRIDGE -> Sonic.
- Hraj 30 s.
- Atari 130XE -> zpet Sega -> SBIRKA/CARTRIDGE -> Sonic/Aladdin.
Ocekavani:
- Zadna cerna obrazovka se zvukem po navratu.
- Zvuk zustava QT/QP profil, nemel by byt horsi nez QV.

2) Samurai Shodown Europe region test
- SBIRKA/CARTRIDGE -> Samurai Shodown (Europe).gen.
Ocekavani:
- Log musi ukazat NO_PATCH_RESPECT_ROM_HEADER region=E.
- C++ status musi jit nativeRegionAuto=EU_PAL_OVERSEAS header=E.
- Hra nesmi koncit jen hlaskou PAL/French region lock kvuli tomu, ze jsme ji prepsali na U/NTSC.
Poznamka:
- Pokud stale zustane region lock, neposilat dalsi per-game patch naslepo; poslat log + screenshot, protoze pak problem neni jen header patch, ale konkretni IO/region bity core.

3) S8/A12 mobile landscape
- Spust Sonic na vysku.
- Otoc mobil do landscape.
Ocekavani:
- Zadny horni text C++ CORE/SBIRKA jako viditelna navigace pres hru.
- Joystick/A/B/C jsou pruhledne pres obraz.
- Obraz nesmi zustat cerna plocha.
- Po navratu na vysku nesmi zustat cerna obrazovka.

Log markery:
- BUILD2QY_SEGA_NATIVE_CPP_ONLY_AUDIT_REGION_MOBILE_STAGE115
- NATIVE_CPP_BUTTON_REMOVED
- REGION_AUDIT_NO_PATCH
- NO_PATCH_RESPECT_ROM_HEADER
- SET_RECT_SKIP_SMALL_QY
- NATIVE_RECT_DPR ... landscapeFullVideo=YES
- NATIVE_AUDIO_STREAM_START_QY_QT_AUDIO_KEEP

Co poslat zpet:
- Nox: jeden log po Sonic + navrat z 130XE.
- S8/A12: screenshot landscape + log.
- U Samurai: screenshot a log s region markerem.
