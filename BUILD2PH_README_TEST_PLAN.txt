AtariHelp.eu EMU-10 BUILD2PH_SEGA_SAFE_ROLLBACK_TO_2PA_LOCKED_STAGE72

PROC EXISTUJE:
BUILD2PE rozbil audio buffer (4096 vs expected 2048).
BUILD2PF mel nestabilni mobilni boot/ERR Aborted.
BUILD2PG vratil EU/PAL bez in-memory compat patche a znovu ukazal region-lock hlasku u Aladdina.
BUILD2PH vraci Sega vetev na BUILD2PA, ktery uzivatel potvrdil jako funkcni pro Nox zvuk + mobilni multitouch joystick.

TEST:
1) Nox portrait:
- otevrit Sega
- Sonic
- zvuk musi byt jako BUILD2PA, ne ticho / ne PE/PF chovani

2) Mobil portrait/landscape:
- Sonic nebo Aladdin
- hra musi nabehnout
- joystick v landscape musi fungovat jako BUILD2PA
- pohyb + skok soucasne musi fungovat

3) EU hry:
- Aladdin Europe, Probotector Europe, Ayrton Senna Europe/Japan
- nesmi se vratit hlaska "developed for PAL..." kvuli BUILD2PG rollbacku
- v logu ma byt EU compat patch jako v BUILD2PA, ne NO_PATCH_PAL_AUTO_ORIGINAL_COPY

4) Zvuk:
- mobilni zvuk je stale otevreny problem
- tento build jej netvrdi jako opraveny

LOG MARKERY:
- BUILD2PH_SEGA_SAFE_ROLLBACK_TO_2PA_LOCKED_STAGE72
- REGION_SAFE_ROLLBACK_2PH_2PA_LOCKED
- inputMode=mobileMultiTouch
- napAudioMode=mobileNativePlayback
- napNoPrime=1
- NO hardGate / NO ScriptProcessor patch
