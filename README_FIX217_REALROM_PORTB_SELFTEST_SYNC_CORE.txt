AtariHelp.eu EMU-09 FIX217_REALROM_PORTB_SELFTEST_SYNC_CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 a ostatni BASIC testy nejsou zmenene.

Co je cilem FIX217:
- oprava realneho ROM/MMU mapovani $5000-$57FF,
- PURE BASIC boot uz nesmi umrit na BRK v prazdne RAM $5003,
- SELFTEST ROM okno se mapuje podle PORTB bit7 LOW jako real XL/XE MMU,
- RESET v PURE ROM rezimu znovu spousti PURE ROM cestu, nespadne do stareho warm FIX181 resetu,
- zadny fake SELF TEST obraz, zadna staticka zastena.

Dulezite:
FIX216 ukazal, ze OS z ATARIXL.ROM behem bootu skace na $5003. To znamena, ze SELFTEST ROM okno musi byt v tu chvili mapovane. FIX216 ho vypnul prilis tvrde a proto BASIC boot skoncil BRK/pauzou.

Test:
1) PURE ROM BASIC BOOT
2) kdyz po chvili neni READY, snapshot TXT
3) PURE ROM SELF TEST
4) snapshot TXT

V logu hledat:
BUILD TAG FIX217_REALROM_PORTB_SELFTEST_SYNC_CORE
FIX217 SELFTEST GATE LIVE
FIX217 PURE RESET
FIX217 SELFTEST ROM MAP active
REAL OS BOOT ... trap
