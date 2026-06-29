BUILD2OL_SEGA_EU_COMPAT_PATCH3_SENNA_STAGE50

CIL:
- Udrzet funkcni EU hry z BUILD2OK: Aladdin EU, Asterix EU, Probotector EU.
- Opravit Ayrton Senna's Super Monaco GP II (Japan, Europe), kde BUILD2OK hlasil AYRTON_GENERIC_PATTERN_NOT_FOUND.
- Nerozbit USA stabilni vetev.

TEST PLAN:
1) USA vetev: otestuj Aladdin USA nebo Asterix USA. Musi jet.
2) EU vetev: otestuj Aladdin Europe. Musi jet.
3) EU vetev: otestuj Asterix Europe. Musi jet.
4) EU vetev: otestuj Probotector Europe. Musi jit dal nez Konami a hrat.
5) EU vetev: otestuj Ayrton Senna's Super Monaco GP II (Japan, Europe). Region hlaska by mela zmizet.

V LOGU U SENNY CHCI VIDET:
- BUILD2OL_SEGA_EU_COMPAT_PATCH3_SENNA_STAGE50
- EU_COMPAT_APPLIED
- HEADER_REGION_E_TO_U_0x1F0
- AYRTON_REGION_CMP_80_TO_C0_COUNT_1_FIRST_0x34F5
- CHECKSUM_0x0142
- MODULE_POSTRUN

CO POSLAT ZPET:
- Probotector porad jede ano/ne
- Ayrton Senna region hlaska zmizela ano/ne
- Aladdin/Asterix EU porad OK ano/ne
- zvuk super / horsi
- log TXT
