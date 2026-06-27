AtariHelp.eu EMU-10 BUILD2LH_PMBASE_ZERO_GUARD_ROLLBACK_REAL_PRIORITY_CORE

Proc vznikl:
BUILD2LG byl spatne. Test Decathlonu ukazal: po PMBASE zero guardu zmizeli atleti a zustaly jen casti/nohy.
Log potvrzuje, ze BUILD2LG vypnul realne PMG DMA z base=$0000 a pNonzeroLines/pPixels spadly na nulu.

Co je zmenene:
- LG PMBASE zero DMA guard je pryc.
- PMBASE=$00 se znovu povoluje, protoze u single-line PMG lezi player data na $0400-$0700, ne primo v zeropage.
- PRIOR tabulka se vraci na KT/LF vyklad: PRIOR=$04 znamena PF0/PF1 nad PMG, ale PMG nad PF2/PF3.
- LE brutalni DMA/HALT zustava vypnute pres LF rollback.
- Donkey Arcade / Donkey Junior PMG faze zustava chranena.
- XEX/OBX/ATR fake-fast jen pro nahravani.
- CLOAD/CSAVE bez fake.
- Bez screen-paintu, bez hacku podle nazvu hry.
- KODY JSOU STEJNE.

V LOGu musi byt:
AtariHelp.eu EMU-10 BUILD2LH_PMBASE_ZERO_GUARD_ROLLBACK_REAL_PRIORITY_CORE pripraven
GTIA PMBASE ZERO DMA ALLOWED BUILD2LH
GTIA PRIOR TABLE BUILD2LH

Test jen prvni krok:
Decathlon: atleti zpet ano/ne, telo/nohy lepsi/stejne/horsi, napis lepsi/stejne/horsi, rychlost OK/pomala/rychla

Kdyz atleti nejsou zpet, stop a poslat LOG.
