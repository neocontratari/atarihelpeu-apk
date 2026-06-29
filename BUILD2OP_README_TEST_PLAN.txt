BUILD2OP_SEGA_FINAL_UI_REAL_SCREEN_HITBOX_STAGE54

CIL:
- napravit BUILD2OO UI chyby
- realne zakryt vygenerovany obraz monitoru ve skinu
- vlozit skutecny emulator do presneho monitor okna
- vratit LOG pod disketu ULOZENE
- opravit hitboxy ovladace podle finalni grafiky

TEST:
1) Otevri Sega sekci.
2) Na uvodni READY obrazovce over, ze monitor sedi uprostred a nic ze skinu neprosvita.
3) Klikni na ULOZENE / disketu -> musi se ulozit LOG.
4) Klikni na cartridge / HRY / SBIRKA -> musi jit vyber ROM.
5) Otestuj D-pad: nahoru/dolu/vlevo/vpravo musi sedet na stred krize.
6) Otestuj A/B/C: zony musi byt male a sedet na kolecka.
7) Pust Ayrton Senna / Probotector / Aladdin nebo jinou EU hru.
8) Sleduj monitor: realna hra musi zakryt skin, zadne prosvitani podkladoveho Sonica.

V LOGU CHCI VIDET:
- BUILD2OP_SEGA_FINAL_UI_REAL_SCREEN_HITBOX_STAGE54
- BUILD2OP LOG_ON_DISKETA requested
- BUILD2OP LOG SAVE OK
- REAL_SCREEN_MASKED
- EU_COMPAT_APPLIED nebo NO_PATCH nonEU
