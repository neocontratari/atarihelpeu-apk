AtariHelp.eu EMU-09 FIX231 PURE HW BOOT IRQ CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy nejsou zmenene.

Cil:
- audit/snapshot zustava bez XEX loaderu,
- POWER BASIC nesmi pri OPTION high spadnout do SELFTEST okna,
- POWER OPTION SELF TEST drzi OPTION low a selftest okno oddelene,
- low-RAM BRK/IRQ smycka se zastavi s pravym PC misto nekonecneho blikani,
- zadna kreslena READY/SELFTEST obrazovka.

Test:
1. PURE HW AUDIT + SNAPSHOT
2. POWER XL/XE BASIC + SNAPSHOT
3. POWER OPTION SELF TEST + SNAPSHOT

