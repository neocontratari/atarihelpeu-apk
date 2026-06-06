AtariHelp.eu EMU-09 FIX230 PURE HW BOOT BUS CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy nejsou zmenene.

Cil FIX230:
- PURE HW bez XEX fallbacku.
- Normalni POWER BASIC ma cold BASIC ROM window, dokud OS sam nezapise PORTB DATA.
- OPTION POWER ma cold SELFTEST window, dokud OS sam nezapise PORTB DATA.
- SIO v PURE HW nema primy PC navrat.
- NMI v PURE HW se vektori pres CPU pred dalsi instrukci, ne pres async dobeh do RTI.
- ANTIC/DLIST diagnostika cte pres stejnou ROM/RAM bus cestu jako CPU.
- Snapshot zustava kratky PURE HW vypis.

Test:
1) PURE HW AUDIT + SNAPSHOT.
2) POWER XL/XE BASIC + SNAPSHOT.
3) POWER OPTION SELF TEST + SNAPSHOT.
