BUILD2SA5P / STAGE164 - PS1 audio rollback + BIOS audit

Co je zmeneno:
- Jen PS1 vrstva. Sega emu ani 130XE assets se nemenily.
- PS1 audio vraceno z posledniho 735-frame tahu na jemnejsi 384-frame profil.
- Nativni PS1 audio FIFO minPull vracen z 256 na 128, aby core nedelal tvrde davky.
- CD access pro PCSX ReARMed nastaven na async, coz muze omezit skubnuti u her streamujicich data/zvuk z disku.
- L1/L2/R1/R2 zustavaji dole u palcu jako ve Stage163.
- Pred kazdym PS1 bootem se do logu pise BUILD2SA5P PS1_BIOS_AUDIT.
- Ukladani BIOSu uz lepe zachova regionove jmeno: SCPH-5502.BIN / scph_5502.bin dostane alias scph5502.bin, ne omylem scph5501.bin.

Co sledovat pri testu:
- V PS1 logu musi byt marker BUILD2SA5P_PS1_AUDIO_ROLLBACK_BIOS_AUDIT_STAGE164.
- Pri startu hry najdi radek BUILD2SA5P PS1_BIOS_AUDIT.
- Pokud audit neukaze scph5501.bin/scph5502.bin/scph5500.bin nebo podobny 512KB BIOS, Sony intro se nejspis neukaze.
- BIOS musi byt rozbaleny .bin, ne ZIP archiv.

Regiony pro test:
- USA hra: scph5501.bin nebo scph1001.bin.
- EU/PAL hra: scph5502.bin nebo scph7502.bin.
- Japan hra: scph5500.bin.

Poznamka:
- V kodu je stale pcsx_rearmed_show_bios_bootlogo=enabled. Pokud Sony intro stale nenajede a BIOS audit BIOS vidi, posli log s radkem PS1_BIOS_AUDIT a PS1_BOOT_OK.
