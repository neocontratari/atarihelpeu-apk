AtariHelp.eu EMU-10 BUILD2JV_XDL_SR_RESCUE_PM_AUDIT_SAFE

ZAKLAD:
- Navazuje na BUILD2JU / BUILD2JT / BUILD2JS.
- BUILD2JS zustava posledni potvrzeny zaklad pro PM DMA latch.
- BUILD2JU PM audit zustava.
- Tohle neni hack na hru a neni screen-paint: rescue kresli jen realna data z platneho VBXE XDL SR zaznamu a realne VBXE VRAM.

CO JE NOVE:
1) VBXE XDL SR rescue:
   - Pokud XDL parser vidi platny graficky SR record, ale standardni XDL render neda zadne viditelne pixely,
     emulator zkusi obecne najit spravnou 16KB VRAM stranku se stejnym offsetem.
   - Duvod: Commando log ukazuje platny XDL record:
     y=24, lines=160, mode=SR, ov=$16A18, step=512, widthSel=1, pri=$FF,
     a soucasne realne VBXE blitter kopie 862 bloku / 66864 bajtu.
   - Rescue vybira podle obsahu VRAM, ne podle nazvu commando.

2) PM audit:
   - Night Driver / Decathlon dal meri PM DMA, blocked PF, draw, offscreen.
   - GTIA PRIOR se ted nehackuje. Altirra manual rika, ze PRIOR $04 znamena PF0>PF1>PF2>PF3>P0>P1>P2>P3>BAK,
     takze slepe pretazeni P/M dopredu by bylo fake/regrese.

KODY JSOU STEJNE:
- BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.
- UI, kazeta, joystick, loader, ROM baseline nejsou cilene menene.

TEST PLAN:
1) Rozbal ZIP overlay pres GitHub Desktop.
2) Spust appku.
3) V LOGu musi byt:
   AtariHelp.eu EMU-10 BUILD2JV_XDL_SR_RESCUE_PM_AUDIT_SAFE pripraven
4) Test 1 - Commando:
   - Nahraj commando230810.xex.
   - Nech uvodni obraz PRESS ANY KEY TO START.
   - Spravny vysledek: ma se pokusit zobrazit Arnold obraz pod/za textem.
   - V LOGu hledej:
     VBXE XDL SR RESCUE BUILD2JV
   - Pokud Arnold porad neni, posli jeden LOG a screenshot uvodu.
5) Test 2 - Night Driver:
   - Nahraj nightdriver_vbxe.xex.
   - Nech 20-40 s ve hre.
   - Spravny vysledek: hlidej, jestli se objevi auto.
   - Pokud ne, posli LOG. Dulezite jsou radky BUILD2JT PM SUMMARY.
6) Test 3 - Decathlon:
   - Nahraj The Activision Decathlon.xex.
   - Nech 20-40 s.
   - Posli LOG, pokud je porad bez zmeny.
7) Test 4 - Mission:
   - Jen rychla kontrola, ze hra porad jede a barvy nejsou horsi.

KDYZ SELZE:
- Staci zase jeden TXT pres LOG / CHYBA.
- Nemusis v logu nic hledat rucne.
