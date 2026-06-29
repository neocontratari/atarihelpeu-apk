BUILD2OE_SEGA_REGION_AUTO_PAL_NTSC_STAGE43

CIL:
- zachovat chvalitebny/cisty zvuk z BUILD2OD
- opravit hlasky typu "Developed for use only with PAL and French SECAM Mega Drive systems" u Europe/PAL her

TEST PLAN:
1) Uplne zavri appku.
2) Spust appku.
3) Sega -> vyber problemovou Europe hru:
   Asterix and the Great Rescue (Europe) (En,Fr,De,Es,It).gen
4) Ocekavane:
   - v logu musi byt region AUTO -> EUROPE / PAL / International
   - wrapper musi zapsat configuration.ini s pal=on japanese=off
   - hra by nemela zustat na PAL/SECAM region hlasce
5) Pak vyber USA hru:
   Chase H.Q. II (USA).gen nebo jinou USA ROM
6) Ocekavane:
   - v logu musi byt region AUTO -> USA / NTSC / International
   - USA hra musi jit jako predtim
7) Zkontroluj, ze zvuk zustal cisty jako BUILD2OD.

V LOGU CHCI VIDET:
- BUILD2OE_SEGA_REGION_AUTO_PAL_NTSC_STAGE43
- REGION_CONFIG_WRITTEN path=/libsdl/clownacy/clownmdemu-frontend/configuration.ini
- REGION_MODE AUTO -> EUROPE / PAL / International pal=true japanese=false
- AUDIO_TUNE installed latencyHint=playback
- MODULE_POSTRUN

CO MI POSLAT ZPET:
- jestli Asterix Europe porad ukazuje PAL/SECAM hlasku nebo uz jede dal
- jestli USA hra porad jede
- jestli zvuk zustal chvalitebny/cisty
- log TXT po Europe ROM a idealne screenshot vysledku
