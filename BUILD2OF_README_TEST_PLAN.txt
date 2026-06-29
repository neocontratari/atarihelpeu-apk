BUILD2OF_SEGA_REGION_PRERUN_PAL_NTSC_STAGE44

CIL:
- opravit PAL/SECAM region lock u evropskych ROMek
- zachovat chvalitebny/cisty zvuk z BUILD2OD/2OE
- nezasahovat do grafiky, skinu, ovladace a ROM pickeru

PROC VZNIKL BUILD2OF:
- V BUILD2OE log ukazal: REGION_CONFIG_SKIP no FS object
- To znamena, ze jsme region konfiguraci zkouseli zapsat moc brzo, jeste pred existenci Emscripten FS.
- BUILD2OF ji pise v Module.preRun, tedy tesne pred startem C++ ClownMDEmu frontendu.

TEST PLAN:
1) Uplne zavri appku.
2) Spust appku.
3) Sega -> vyber Asterix and the Great Rescue (Europe).
4) Ocekavani: PAL/SECAM hlaska zmizi a hra se spusti dal.
5) Uloz LOG.
6) Pak zkus Aladdin (Europe).
7) Uloz LOG.
8) Pak zkus Chase H.Q. II (USA), musi porad fungovat.
9) Uloz LOG.

V LOGU CHCI VIDET U EVROPSKE HRY:
- BUILD2OF_SEGA_REGION_PRERUN_PAL_NTSC_STAGE44
- ROM_POST_TO_LOCAL_WRAPPER ... region=AUTO -> EUROPE / PAL / International mode=PAL_EU
- REGION_PRERUN_START ... writtenBefore=NO
- REGION_CONFIG_WRITTEN path=/libsdl/clownacy/clownmdemu-frontend/configuration.ini ... pal=true japanese=false
- REGION_PRERUN_DONE written=YES pal=true japanese=false
- MODULE_POSTRUN regionConfigWritten=YES

KDYZ TO PORAD NEPUJDE:
- Posli screenshot + LOG.
- Bude to znamenat, ze oficialni web build ClownMDEmu region z configu nebere dost brzy/nebo ho prepisuje.
- Pak dalsi krok neni dalsi config pokus, ale manualni PAL boot pres jiny core build / vlastni lokalni build frontendu.
