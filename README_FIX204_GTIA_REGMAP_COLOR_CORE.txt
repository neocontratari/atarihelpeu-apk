AtariHelp.eu EMU-09 FIX204_GTIA_REGMAP_COLOR_CORE

Navazuje na FIX203. Rene porovnal GTIA 9/10/11 VERIFY proti Altirre a realnemu Atari 130XE a potvrdil, ze emulator je mimo hlavne v zakladnim GTIA mapovani barev.

Zmeny:
- oprava GTIA GRAPHICS 10 mapovani: pixel 0-3 = PCOLR0-3, pixel 4-7 = COLPF0-3, pixel 8 = COLBK. Predchozi build mel poradi obracene.
- oprava GTIA GRAPHICS 11: pixel 0 je background z COLBK s luminanci vynucenou na 0; uz se nepouziva COLBK luminance jako seda plocha pro nulu.
- oprava GTIA GRAPHICS 9: nulovy/clear pixel pouziva viditelny COLBK/background, ne forced black.
- doplnen snapshot radek GTIA REGMAP FIX204 s realne pouzitou tabulkou 16 nibbles.
- zachovany FIX203 BASIC/OS verifier kod, ale opravene popisy mapovani.
- zadne herni hacky, jde o obecny GTIA core zaklad pro hry i BASIC/OS testy.

Test plan:
1) GTIA 9 VERIFY: porovnat s Altirrou/realnym Atari. Pozor hlavne na pozadi a nulove pixely.
2) GTIA 10 VERIFY: porovnat mapovani barev. Pixel 0 ma byt PCOLR0, ne COLBK.
3) GTIA 11 VERIFY: pozadi pod nedokreslenou casti nesmi byt seda plocha; pixel 0 ma byt background s lum 0.
4) Poslat screenshoty + snapshot txt. Hledat radky GTIA REGMAP FIX204, GTIA VERIFY FIX204, GTIA MODE, GTIA RENDER, COLPF/BK HW, PRIOR.
5) Potom smoke test Donkey Kong + Super Cobra, jestli se nezmenila stabilita.

Commit summary:
FIX204 gtia regmap color core
