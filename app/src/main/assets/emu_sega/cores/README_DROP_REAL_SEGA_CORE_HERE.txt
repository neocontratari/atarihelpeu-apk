BUILD2MS_SEGA_LRUSSO_GLOBAL_LEXICAL_BOOT_STAGE6

Sem patri realny lokalni Mega Drive / Genesis core engine pro offline finalni build.
BUILD2MS umi automaticky zkusit zejmena:
- cores/Genesis.min.js
- cores/Genesis.js
- cores/lrusso_Genesis.min.js
- cores/lrusso_Genesis.js

BUILD2MS obsahuje adapter nap_sega_lr_genesis_adapter.js pro lrusso Genesis/PicoDrive browser API embedGenesis().

ROM SOUBORY SEM NEPATRI.
Sonic/Aladdin/komercni ROM se do APK/ZIP nepridavaji. ROM vybira tester lokalne pres HRY/SBIRKA/CARTRIDGE.

LICENCE:
Pokud se sem prida cizi open-source core, musi se spolu s nim pridat odpovidajici licence a idealne i odkaz/source notice. Finalni app nema tise schovavat cizi emulator bez licence.


BUILD2MS NOTE:
Nejlepsi lokalni test: vlozit legalne ziskany lrusso Genesis.min.js do teto slozky jako cores/Genesis.min.js.
Pak se nacita jako same-origin asset a ochranny guard v engine by nemel padat.
Online probe zustava jen diagnosticky fallback.
