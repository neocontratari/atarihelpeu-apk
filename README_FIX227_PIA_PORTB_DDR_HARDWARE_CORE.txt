AtariHelp.eu EMU-09 FIX227_PIA_PORTB_DDR_HARDWARE_CORE

Commit summary:
FIX227 pia portb ddr hardware core

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY a ostatni BASIC testy nebyly menene.

Proc FIX227:
FIX226 audit ukazal zasadni chybu: i POWER XL/XE BASIC skoncil v self-test ROM oblasti $532x.
Log ukazal PORTB=$75 a SELFTEST_WINDOW=ON pri BASIC bootu. To nesedi s realnym BASIC startem.
Koren chyby byl v PIA PORTB emulaci: emulator bral kazdy zapis na $D301 jako PORTB vystup.
Na realnem Atari XL/XE ale $D301 pres PIA zapisuje bud DDRB, nebo PORTB output latch podle PBCTL bit2 ($D303).

Oprava:
- pridan PIA PORTB model:
  - $D303/PBCTL bit2 = 0 -> $D301 zapisuje DDRB
  - $D303/PBCTL bit2 = 1 -> $D301 zapisuje PORTB output latch
  - effective PORTB = (OUT & DDR) | ($FF & ~DDR)
- self-test okno $5000-$57FF se ridi effective PORTB bit7, ne poslednim slepym bajtem zapsanym na $D301.
- BASIC ROM $A000-$BFFF se v PURE HARDWARE ridi effective PORTB bit1 LOW.
- audit a snapshot vypisuji OUT / DDR / PBCTL / effective PORTB.
- aktualizovany PURE status/log, aby v hlavnim PURE toku nebyly zavadejici stare hlasky jako hlavni stav.

Test plan:
1. PURE HW AUDIT
   Posli screenshot/snapshot, hlavne radky PIA PORTB OUT/DDR/PBCTL/EFFECTIVE.

2. POWER XL/XE BASIC
   Cil: uz nesmi skocit do PC $532x a SELFTEST_WINDOW=ON jen kvuli DDR zapisu.
   Pokud READY nenabehne, snapshot hned.

3. POWER OPTION SELF TEST
   Cil: SELFTEST_WINDOW=ON je tady ocekavane, ale pouze pres effective PORTB bit7 LOW.

Dulezite log radky:
BUILD TAG FIX227_PIA_PORTB_DDR_HARDWARE_CORE
FIX227 PIA PORTB
MMU EFFECTIVE PORTB
SELFTEST_WINDOW
PC
DLIST
SAVMSC
