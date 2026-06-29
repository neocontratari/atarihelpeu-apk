BUILD2OM_SEGA_MAIN_EU_COMPAT_STAGE51

CIL:
- jedna hlavni Sega obrazovka pred novym designem
- bez viditelneho USA/EU prepinani
- zachovat funkcni EU compatibility patche z BUILD2OL
- zachovat lepsi zvuk/obraz subjektivne z EU vetve

TEST PLAN:
1) Spust appku.
2) Otevri normalni Sega sekci.
3) Zkus Aladdin Europe.
4) Zkus Asterix Europe.
5) Zkus Probotector Europe.
6) Zkus Ayrton Senna Japan/Europe.
7) Zkus Aladdin USA nebo Asterix USA.
8) Klikni na panel 16-BIT/POWERED BY SEGA: ma ukazat jen info, nema prepinat vetve.

OCEKAVANI:
- vse jede v jedne hlavni Sega vetvi
- EU hry s patchem funguji
- USA hry jedou bez patchu
- zvuk zustava cisty/super jako v BUILD2OL

V LOGU CHCI VIDET:
- BUILD2OM_SEGA_MAIN_EU_COMPAT_STAGE51
- EU_COMPAT_APPLIED pro EU hry
- NO_PATCH nonEU pro USA hry
- AYRTON_REGION_CMP_80_TO_C0_COUNT_1_FIRST_0x34F5 pro Sennu
- PROBOTECTOR_GENERIC_REGION_CMP_C0_TO_80_COUNT_1_FIRST_0x3BB pro Probotector
- MODULE_POSTRUN
