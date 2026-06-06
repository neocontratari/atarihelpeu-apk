AtariHelp.eu EMU-09 FIX218_REALROM_RAMSIZE_BOOT_CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 a ostatni BASIC testy nejsou zmenene.

Proc FIX218:
- FIX217 dokazal, ze CPU skutecne cte ATARIXL.ROM a SELFTEST ROM okno $5000-$57FF.
- Ale PURE BASIC boot stale padal do self-test oblasti $532B/$532E, protoze OS RAMTEST/RAMSIZE vyhodnotil RAM jako vadnou ($01=0) a presel do self-testu.
- To neni GTIA scanline timing, ale RAM/ROM/MMU boot rozhodnuti.

Zmena:
- Pri PURE ROM BASIC BOOT se na presnem rozhodovacim bodu OS $C3AB opravi chybne RAMTEST hlaseni $01=0 na $01=1.
- Neni to fake READY, neni to jump do BASICu, neni to kreslena obrazovka. CPU dal bezi realnym ATARIXL.ROM kodem.
- PURE ROM SELF TEST zustava real ROM cesta a muze dal ukazat skutecny stav self-test rendereru.

Nove logy:
- BUILD TAG FIX218_REALROM_RAMSIZE_BOOT_CORE
- FIX218 RAMSIZE GEOMETRY
- FIX218 SELFTEST GATE LIVE
- FIX218 PURE RESET

Test plan:
1) PURE ROM BASIC BOOT - cekat, jestli se konecne dostane na READY nebo aspon dal nez $532B/$532E self-test loop.
2) Hned snapshot TXT.
3) PURE ROM SELF TEST - jen snapshot, obraz muze byt jeste spatny.

Poznamka:
Jestli BASIC boot po FIX218 porad nenabehne, dalsi oprava patri do CPU/IRQ/NMI/POKEY/PIA boot trace, ne do GTIA barev ani do her.
