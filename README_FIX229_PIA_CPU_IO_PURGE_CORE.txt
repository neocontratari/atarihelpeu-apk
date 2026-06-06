AtariHelp.eu EMU-09 FIX229 PIA CPU IO PURGE CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy nejsou zmenene.

Co je opravene:
- PURE HW snapshot/audit zustava izolovany a nevola XEX loader.
- CPU read/write $D301/$D303 v PURE HW jde pres PIA DDR/PBCTL model.
- $D301 uz neni primy PORTB hack: PBCTL bit2=0 zapis DDRB, PBCTL bit2=1 zapis OUT latch.
- Snapshot ukazuje BASIC_WINDOW/SELFTEST_WINDOW podle EFFECTIVE PORTB.
- POKEY status registry jsou idle hardware stav, ne fake zvuk.

Test:
1) PURE HW AUDIT + SNAPSHOT.
2) POWER XL/XE BASIC + SNAPSHOT.
3) POWER OPTION SELF TEST + SNAPSHOT.
