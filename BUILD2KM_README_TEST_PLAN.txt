AtariHelp.eu EMU-10 BUILD2KM_GTIA9_11_PIXEL_GENERATOR_FULL_CORE

Ucel:
- Po KL uz nejde o PMG masku ani WSYNC naslepo.
- KK dokazal, ze PMG hard-mask funguje technicky, ale obraz Postcardu se nehnul.
- KM proto jde primo do GTIA 9/11 pixel generatoru a vraci G2F PMG vrstvu pres normalni PRIOR.

Zmeny:
1) GTIA9: hue z COLBK, plny 4bit jas z pixelu; pixelovy bit0 se uz neztraci pred slozenim barvy.
2) GTIA11: standardni hue = pixel nibble, luminance = COLBK; odstranena KL OR-hue cesta.
3) KK hard-mask PMG pres GTIA obraz je zrusena. PMG jde pres normalni PRIOR tabulku, aby G2F barevne vrstvy mohly znovu kreslit.
4) KD PMBASE hack zustava pryc.
5) Commando Arnold / Mission / Tetris chraneno.

KODY JSOU STEJNE.

Test:
- Postcard_Atari_Rocky.xex
- Napsat: posun ano/ne.
- Kdyz ne: screenshot + LOG.

Log markery:
- BUILD2KM_GTIA9_11_PIXEL_GENERATOR_FULL_CORE
- GTIA9 COLOR BUILD2KM
- GTIA11 COLOR BUILD2KM
- GTIA PMG PRIOR BUILD2KM
