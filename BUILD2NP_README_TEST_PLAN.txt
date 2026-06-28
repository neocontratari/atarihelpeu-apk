BUILD2NP_SEGA_AUDIO_CLEAN_POWER_CROP_STAGE29 - TEST PLAN

1) Rozbal overlay pres koren projektu.
2) Spust APK/Nox.
3) Otevri SEGA modul.
4) Vyber Sonic, over ovladac a zvuk.
5) Dej RESET 3x. Zvuk nesmi po resetu zanechat ruch/chraplani.
6) Vyber Aladdin. Porovnej: intro a gameplay, jestli po resetu zustava ruch.
7) Vyber Golden Axe a zkus reset + vymenu hry.
8) Zkontroluj obraz v monitoru: crop je vracen na stabilni NI hodnoty.
9) ULOZENE -> posli log.

LOG MARKERY:
- BUILD2NP POWER_OFF_WRAPPER_BEGIN strongerAudioClean=YES
- WRAPPER AUDIO_TRACK installed native options only
- WRAPPER AUDIO_CONTEXT_CLEANUP reason=...
- image tune: PLAY_CROP stableNI left=-8% top=-9% width=116% height=121% landscapeTop=-8vh height=116vh

POZNAMKA:
PAL/FRENCH SECAM hlaska je region-lock konkretni ROM. Tenhle build zatim neresi prepinani regionu, jen loguje region hint.
