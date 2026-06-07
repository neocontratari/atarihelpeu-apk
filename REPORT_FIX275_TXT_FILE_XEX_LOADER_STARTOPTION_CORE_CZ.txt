AtariHelp.eu EMU-09 FIX275 TXT FILE XEX LOADER STARTOPTION CORE

Commit summary:
FIX275 TXT file XEX loader STARTOPTION core

Zmeny:
- dopredu z funkcniho FIX274/FIX270/FIX267/FIX261, zadny rollback.
- FIX274 modalni textarea workflow nahrazen mobilnim souborovym workflow.
- Novy jasny panel: vybrat TXT/BAS/LST soubor z Androidu -> poslat do real BASICu -> volitelne RUN.
- Vlastni TXT nejde pres screen RAM ani program RAM inject; pouziva existujici radkovou ATASCII/KGETCH frontu po real BASIC READY.
- Pridan obecny uzivatelsky XEX picker: vybrat XEX z Androidu -> START+OPTION + loadXex(false).
- Zachovan built-in turbo-basic-xl-1.5-copy.xex a tlacitko TURBO BASIC XL XEX START+OPTION.
- PiTT/KiTT profil se pro Turbo/custom XEX nenastavuje; loader startuje jako generic XEX.
- Staré duplicitni FAST/modal panely se schovavaji.
- Header/snapshot sjednocen na FIX275.

Co to NENI:
- zadny fake READY
- zadny fake LOAD
- zadny RAM/program inject
- zadny screen RAM write
- zadny herni hack
- zadny fake kazetovy zvuk

CLOAD/WAV pravidlo:
Pozdeji pouze real Atari kazetova cesta pres POKEY/SIO se skutecnym zvukem. Zadny beep jako dukaz a zadne bokem nacitani do RAM.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody nejsou menene.
