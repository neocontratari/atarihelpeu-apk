BUILD2OK_SEGA_EU_COMPAT_PATCH2_STAGE49

TEST PLAN:
1) Nejdriv over USA vetev:
   - Aladdin USA nebo Asterix USA musi porad jet.
2) Prejdi do EU vetve pres 16-BIT / POWERED BY SEGA.
3) Otestuj:
   - Aladdin Europe: musi porad jet.
   - Asterix Europe: musi porad jet.
   - Probotector Europe: mel by jit dal nez Konami logo.
   - Ayrton Senna Europe: zkus, jestli zmizi region hlaska.
4) Uloz log z EU vetve.

V LOGU CHCI VIDET:
- BUILD2OK_SEGA_EU_COMPAT_PATCH2_STAGE49
- EU_COMPAT_APPLIED
- HEADER_REGION_E_TO_U_0x1F0
- u Probotector: PROBOTECTOR_GENERIC_REGION_CMP_C0_TO_80_COUNT_...
- u Ayrton/Senna: AYRTON_GENERIC_REGION_CMP_C0_TO_80_COUNT_... pokud pattern existuje
- CHECKSUM_0x.... pokud byl patchnuty kod
- MODULE_POSTRUN

CO POSLAT ZPET:
- USA vetev jede/ne
- Aladdin EU jede/ne
- Asterix EU jede/ne
- Probotector EU: jen Konami / jde dal / spadne
- Ayrton Senna EU: region hlaska zustala/ne
- zvuk cisty/ne
- log TXT
