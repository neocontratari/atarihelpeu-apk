AtariHelp.eu EMU-09 FIX232 PURE HW MODE ACTIVE CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani dalsi BASIC testy nejsou zmenene.

FIX231 patch byl omylem mimo live <script>, proto se nechoval viditelne jinak. FIX232 je vlozeny pred </script> a aktivne oddeluje BASIC a OPTION SELF TEST cestu.

Zmeny:
- zadny XEX fallback ve snapshotu
- zadny kresleny READY/SELFTEST
- POWER BASIC: OPTION high, BASIC ROM viditelna, selftest ROM okno blokovane pro basic mode
- POWER OPTION SELF TEST: OPTION low, BASIC ROM skryta, selftest ROM okno zapnute
- snapshot vypisuje FIX232, mode, PIA, windows, CPU, DLIST, POKEY a trace

Test:
1) PURE HW AUDIT + SNAPSHOT
2) POWER XL/XE BASIC + SNAPSHOT
3) POWER OPTION SELF TEST + SNAPSHOT
