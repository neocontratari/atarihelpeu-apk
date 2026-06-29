BUILD2OU_SEGA_MOBILE_LANDSCAPE_PERF_STAGE59

CIL:
- nechat portrait/Nox final jako baseline
- udelat prvni real mobile landscape rezim
- overit rychlost, zvuk a pruhledny joystick na realnem Android 12

TEST NA NOX/PORTRAIT:
1) Otevri Sega v portraitu.
2) Otestuj, ze portrait vypada jako BUILD2OT a tlacitka sedi.

TEST NA REAL ANDROID 12:
1) Spust appku.
2) Otevri Sega.
3) Vyber Sonic nebo Aladdin.
4) Otoc mobil na sirku.
5) Ocekavani: hra pres celou obrazovku, Sega skin skryty kvuli vykonu.
6) Otestuj pruhledny joystick vlevo dole.
7) Otestuj A/B/C vpravo dole a START dole uprostred.
8) Sleduj zvuk: cisty / kouse se / chrci.
9) Sleduj rychlost: normalni / pomala / trhana.
10) Disketa ULOZENE ma ulozit LOG.

V LOGU CHCI VIDET:
- BUILD2OU_SEGA_MOBILE_LANDSCAPE_PERF_STAGE59
- ORIENTATION CHANGE
- mobilePerf=YES ve wrapper logu, pokud projde wrapper
- LOG_ON_DISKETA requested
