AtariHelp.eu EMU-09 FIX265 BUTTONS SAFE FORWARD FROM FIX261 CASSETTE PREFLIGHT CORE

Commit summary:
FIX265 buttons safe forward from 261 cassette preflight

Zaklad:
- Vychazi z funkcniho FIX261.
- Neni to rollback commit, je to forward oprava nad posledni funkcni vetvi.
- FIX262 a FIX264 CPU/audio/POKEY hook vrstva neni soucasti.

Zmeny:
- Zamceny build/header na FIX265.
- Index otevírá emu09_pmg_gtia_overlay.html?fix=265&from=261&safe=1&t=Date.now().
- Zachovana jedna klavesnice a jeden kazetakovy panel.
- POWER XL/XE BASIC, SELF TEST, RESET a SNAPSHOT deleguji na puvodni funkcni FIX261 core.
- Pridan pouze ochranny wrapper s chybovym vypisem; zadne CPU prototype hooky, zadny POKEY/SERIN inject, zadny audio context.
- Built-in TurboBasicXXL.wav asset cesta opravena z TurboBasicXXL.wav?fix=251 na TurboBasicXXL.wav.
- Snapshot pridava audit, ze FIX262/FIX264 hooky nejsou pritomne.

Zakaz / pravda:
- Zadny fake READY.
- Zadny fake SELF TEST.
- Zadny fake LOAD.
- Zadny RAM inject.
- Zadny herni hack.

Poznamka ke kazete:
FIX265 zatim nema novou POKEY serial fyziku. Opravuje bezpecnou cestu: appka musi reagovat, built-in WAV musi jit nacist a CLOAD je realny BASIC prikaz pres klavesnici. Dalsi krok muze pridat kazetovou emulaci az izolovane bez sahani na POWER/SNAPSHOT tlacitka.

KODY JSOU STEJNE
BASIC/Altirra porovnavaci kody se nemenily.
