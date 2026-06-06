AtariHelp.eu EMU-09 FIX226 PURE HARDWARE AUDIT CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy nejsou menene.

Co je nove:
- FIX225 netestuj; FIX226 pridava cisty predtest PURE HW AUDIT.
- Viditelny panel je orezany na PURE HARDWARE: AUDIT, POWER BASIC, POWER OPTION SELF TEST, RESET, START/SELECT/OPTION, ENTER, SNAPSHOT.
- Stare herni/testovaci servisni volby nejsou na hlavni obrazovce dostupne.
- PURE HW AUDIT vypise skutecny stav: ROM velikosti, vektory RESET/NMI/IRQ, PORTB/selftest window, fronty vstupu, CPU, DLIST/SAVMSC/NMIEN a POKEY registry.
- Pokud audit najde starou aktivni frontu nebo spatny vektor, napise CHECK, ne OK.

Test plan:
1) Spust EMU-09.
2) Klikni PURE HW AUDIT a posli screenshot/snapshot, pokud neni OK.
3) Teprve potom POWER XL/XE BASIC.
4) Snapshot.
5) POWER OPTION SELF TEST.
6) Snapshot.
