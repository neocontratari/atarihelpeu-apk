BUILD2SA5P / STAGE164 CLEAN REDO - PS1 audio rollback

Co je zmeneno:
- Jen PS1 vrstva. Sega emu ani 130XE assets se nemenily.
- PS1 audio je ve stabilnejsim 384-frame profilu.
- Nativni PS1 audio FIFO minPull je 128.
- CD access pro PCSX ReARMed je nastaven na async kvuli mensimu skubani pri cteni disku.
- L1/L2/R1/R2 zustavaji dole u palcu.
- Pred PS1 bootem se do logu pasivne pise BUILD2SA5P PS1_BIOS_AUDIT.

Co je zamerne pryc:
- Zadny importer BIOS archivu.
- Picker po uzivateli nechce vybirat BIOS.
- Sony intro neni podminka testu; hlavni je zvuk, obraz a ovladani hry.

Test:
- V PS1 logu musi byt marker BUILD2SA5P_PS1_AUDIO_ROLLBACK_BIOS_AUDIT_STAGE164.
- Pri startu hry musi byt PS1_AUDIO_START s chunk=384.
- V nativnim core je pcsx_rearmed_async_cd=async.
