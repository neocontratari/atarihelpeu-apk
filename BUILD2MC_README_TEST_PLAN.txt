AtariHelp.eu EMU-10 BUILD2MC_DOC_XDL_TRANSPARENCY_PRIORITY_MAP_SAFE

ZAKLAD
- BUILD2LR jako stabilni app/loader zaklad.
- Zahrnuje dokumentacni fixy z MA/MB: FX1.26 regmap + BCB21/blitter constant/collision.
- NEzaklada se na LT/LU/LV/LW/LY/LZ filtrech.
- UI / loader / klavesnice / joystick / kazeta / CLOAD / CSAVE beze zmen.
- KODY JSOU STEJNE.

PROC BUILD2MC EXISTUJE
Po znovu-projiti docs.zip / examples.zip / cores126.zip / release-install a webovych zdroju Lotharek/Pigwa jsem nasel dalsi tvrde chyby v EMU-10 VBXE render jadre:
1) Transparentnost SR/LR/HR byla v kodu prebijena priority=$FF. FX1.26 manual rika, ze index 0 / nibble 0 je transparentni, pokud neni nastaven VC_NO_TRANS. Priorita to nema menit.
2) Attribute Map souradnice jsou v ANTIC GR.8 pixelech 256/320/336, ne v 640px HR canvas souradnicich.
3) MAP OV palette ma pri aktivni Attribute Map prednost pred XDL OV palette. Paleta 0 je platna, neni to "nenastaveno".
4) Priority register: bit6 znamena PF2 i PF3, bit7 znamena COLBAK. Stary kod mapoval PF3 na bit7 a COLBAK poustel automaticky.
5) SR stripe/low-origin filtry nejsou ve fx1.26 dokumentaci a rezaly Night Driver auto. V MC jsou vypnute.

CO JE MENENO
- vbxeGraphTransparent8LC/4LC: vraceno na ciste fx1.26 transparent rules.
- vbxeMapAttrAtHi/Pixel: Attribute Map prepocet pres GR.8 souradnice.
- vbxeGraphPalAtHiLC: MAP OV palette vzdy pred XDL OV palette, kdyz je mapa aktivni.
- vbxePriorityAllowsHiPixel: PF2/PF3 = bit6, COLBAK = bit7.
- vbxeSrLowOriginSkipRowsKE a SR stripe filtry: vypnute, zadne rezani radku.

LOG MARKERY
Po startu APK hledej:
- AtariHelp.eu EMU-10 BUILD2MC_DOC_XDL_TRANSPARENCY_PRIORITY_MAP_SAFE pripraven
- VBXE XDL TRANSPARENCY DOC BUILD2MC
- VBXE ATTRIBUTE MAP GR8 DOC BUILD2MC
- VBXE MAP PALETTE DOC BUILD2MC
- VBXE PRIORITY DOC BUILD2MC
- VBXE SR FILTERS OFF BUILD2MC
- VBXE FX126 REGMAP BUILD2MC
- VBXE BLITTER DOC BUILD2MC

TEST PLAN
1) Popeye VBXE
   - Spustit test asset Popeye (VBXE, PAL Version)(2).xex.
   - Ocekavani: proti LZ by mel byt mensi chaos, protoze nula uz nema prekryvat pozadi jako plny overlay a AM paleta/priority by mely byt podle fx1.26.
   - Nahlasit: horni napis je/neni, schody-plosiny lepsi/stejne/horsi, chaos mensi/stejny/horsi.

2) Night Driver VBXE
   - Spustit nightdriver_vbxe.xex.
   - Ocekavani: auto uz nesmi byt rezane SR filtrem. Pruh muze zustat, pokud neni jen chyba transparentnosti/priority; MC ho zamerne neschovava filtrem.
   - Nahlasit: pruh pryc/stejny/horsi, auto cele/urizle.

3) W3D ATR
   - Spustit wolf3d(1).atr.
   - Ocekavani: zadna regrese loaderu/rychlosti/klavesnice.
   - Nahlasit: stejny/lepsi/horsi.

4) River Raid jen regrese, pokud uzivatel bude chtit
   - Neni hlavni test. LR River Raid TOP se ma zachovat, protoze MC nemeni loader ani non-VBXE zaklad.

KDYZ TEST SELZE
- Poslat log z APK a screenshot Popeye + Night Driver.
- Neopravovat dalsim filtrem. Dalsi krok smi byt jen porovnani XDL/VRAM dumpu proti Altirre nebo realnemu VBXE.
