BUILD2OG_SEGA_REGION_USERDIR_ARGS_STAGE45

CIL:
- zkusit opravit PAL/SECAM region lock pres oficialni --user konfiguracni adresar a --cartridge start ROM
- ponechat stabilni cisty zvuk z BUILD2OD/2OF

TEST PLAN:
1) Uplne zavri appku.
2) Spust appku.
3) Sega -> vyber Asterix Europe.
4) Pokud porad ukaze PAL/SECAM hlasku, uloz log.
5) Sega -> vyber Aladdin Europe.
6) Pokud porad ukaze PAL/SECAM hlasku, uloz log.
7) Sega -> vyber Chase H.Q. II USA nebo Chess/USA.
8) USA hra musi porad fungovat.

V LOGU CHCI VIDET:
- BUILD2OG_SEGA_REGION_USERDIR_ARGS_STAGE45
- REGION_ARGUMENTS --user /nap_sega_region_config --cartridge blob:...
- REGION_CONFIG_MULTI_DONE written=YES
- REGION_PRERUN_DONE written=YES pal=true japanese=false userDir=/nap_sega_region_config
- MODULE_POSTRUN regionConfigWritten=YES

POZOR:
Jestli v BUILD2OG USA hra nenajede vubec, znamena to, ze remote ClownMDEmu build nepodporuje --cartridge s blob URL tak, jak potrebujeme. Pak se vratime na BUILD2OF/2OD pro stabilitu.

CO POSLAT ZPET:
- Asterix Europe: jede / porad PAL hlaska / nenajede vubec
- Aladdin Europe: jede / porad PAL hlaska / nenajede vubec
- Chase/Chess USA: jede / nejede
- zvuk: zustal cisty / zhorsil se
- log TXT
