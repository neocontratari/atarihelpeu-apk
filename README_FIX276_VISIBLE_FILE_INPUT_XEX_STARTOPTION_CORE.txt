FIX276_VISIBLE_FILE_INPUT_XEX_STARTOPTION_CORE

Dopredny overlay z funkcniho BASIC jadra FIX270/FIX267/FIX261.

Co je nove:
- viditelny TXT/BAS file input pro Android/WebView, zadne skryte programove click();
- TXT/BAS -> FileReader -> existujici real BASIC ATASCII/KGETCH line queue;
- viditelny XEX file input;
- XEX -> ArrayBuffer -> customXexBytes -> normalni loadXex(false) -> START+OPTION drzeni;
- Turbo Basic XL XEX asset turbo-basic-xl-1.5-copy.xex zachovan a spousten stejnou XEX cestou;
- opraven FIX270 finish(a) bug po poslednim EOL;
- opraven warm reset bug, kde undefined bindUi shazoval teply reset do cold fallbacku;
- schovane matoucí modal/old FAST panely, ale zakladni POWER/SNAPSHOT/RESET tlacitka zustavaji puvodni.

Zakaz/fakta:
- zadny fake READY;
- zadny fake LOAD;
- zadny RAM/program inject;
- zadny screen RAM write;
- zadny herni hack;
- CLOAD/WAV zatim nerozsirovano; pozdeji jen real Atari kazeta pres POKEY/SIO se zvukem.

KODY JSOU STEJNE.
