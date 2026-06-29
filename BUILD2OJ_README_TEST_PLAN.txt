BUILD2OJ_SEGA_EU_COMPAT_PATCH_STAGE48

CIL:
- zkusit rozjet EU hry v oddelene EU vetvi bez rozbiti USA stable vetve
- nepouzivat --user/--cartridge cestu z 2OG, ktera rozbila nacitani her
- zachovat zvuk playback 48 kHz + hard reset mezi hrami

TEST PLAN:
1) Uplne zavri appku.
2) Spust normalni Sega sekci.
3) V USA vetvi zkus Aladdin USA Final Cut nebo Asterix USA.
   Ocekavani: hra jede jako driv.
4) Klikni vlevo dole na panel 16-BIT / POWERED BY SEGA.
5) Otevre se EU vetev.
6) V EU vetvi vyber Aladdin Europe.
   Ocekavani: v logu bude EU_COMPAT_APPLIED a HEADER_REGION_E_TO_U_0x1F0.
7) V EU vetvi vyber Asterix Europe.
   Ocekavani: v logu bude EU_COMPAT_APPLIED, HEADER_REGION_E_TO_U_0x1F0 a ASTERIX_REGION_CMP_C0_TO_80.
8) Pokud hra porad ukaze PAL/SECAM hlasku, poslat screenshot + log.
9) Z EU vetve se vratis zpet do USA vetve stejnym 16-BIT / POWERED BY SEGA panelem.

V LOGU CHCI VIDET:
- BUILD2OJ_SEGA_EU_COMPAT_PATCH_STAGE48
- BUILD2OJ EU_COMPAT_APPLIED
- EU_COMPAT_PATCH v wrapperu
- ASTERIX_REGION_CMP_C0_TO_80 u Asterixe
- HEADER_REGION_E_TO_U_0x1F0 u obou EU her
- MODULE_POSTRUN

CO POSLAT ZPET:
- USA vetev: Aladdin/Asterix USA jede/ne
- EU vetev: Aladdin Europe jede/ne
- EU vetev: Asterix Europe jede/ne
- PAL/SECAM hlaska zustala/ne
- zvuk cisty/ne
- TXT log z EU vetve
