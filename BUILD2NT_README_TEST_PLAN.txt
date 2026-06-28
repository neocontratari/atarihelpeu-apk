BUILD2NT_SEGA_AUDIO_SYNC_80MS_FULL_MONITOR_STAGE33

CIL:
- Zachovat cisty zvukovy zaklad z BUILD2NS/2NN.
- Zmensit zpozdeni zvuku bez navratu k chraplavemu interactive/nativnimu experimentu.
- Zrusit agresivni orez ClownMDEmu frame: frame=0/0/100/100, aby byl videt cely obraz Aladdina/Sonica.
- Nesahat na funkcni ovladac a reset/power-cycle.

TEST:
1. Otevri SEGA modul.
2. Vyber Sonic.
3. Zkus prstynky: zvuk ma byt cisty a mene zpozdeny nez BUILD2NS.
4. Vyber Aladdin: obraz ma byt cely v monitoru, bez useknutych krajů.
5. Zkontroluj, ze ovladac porad funguje.
6. Reset a vymena hry musi zustat bez zpomalovani.
7. ULOZENE -> poslat log.

HLEDAT V LOGU:
- BUILD2NT
- AUDIO_TUNE installed latencyHint=0.080 preferredSampleRate=48000
- baseLatency/outputLatency pokud je WebView vrati
- image tune: FULL_MONITOR_NO_CROP
