BUILD2NR_SEGA_AUDIO_SYNC_MONITOR_FIX_STAGE31 - TEST PLAN

KODY JSOU ZMENENE.

TEST:
1. Rozbal ZIP pres koren projektu.
2. Otevri SEGA modul.
3. Vyber Sonic pres HRY / SBIRKA / CARTRIDGE SLOT.
4. Otestuj hlavne sbirani prstynku: zvuk ma byt cisty jako BUILD2NN, ale casove bliz k obrazu.
5. Vyber Aladdin. Zkontroluj, jestli je obraz v monitoru vetsi/lepe sedly a jestli neni tak ujety.
6. Dej RESET a potom vyber jinou hru. Sleduj, jestli po resetu nezustane audio ruch.
7. Zkus Golden Axe.
8. ULOZENE -> posli log.

V LOGU HLEDAT:
- BUILD2NR
- AUDIO_TUNE installed latencyHint=playback preferredSampleRate=48000
- VIDEO_SYNC_DELAY installed frames=4 target=audio_clean_2NN
- AUDIO_CONTEXT_CLEANUP reason=...
- image tune: PLAY_CROP_MONITOR_FIX ...

POZNAMKA:
BUILD2NR neni rollback. BUILD2NN byl jen posledni cisty zvukovy referencni bod; v NR je k nemu pridana oprava casovani a velikosti monitoru.
