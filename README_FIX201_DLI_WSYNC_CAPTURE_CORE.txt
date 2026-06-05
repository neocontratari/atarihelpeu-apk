AtariHelp.eu EMU-09 FIX201_DLI_WSYNC_CAPTURE_CORE

Commit summary:
FIX201 dli wsync capture core

Co je v overlayi:
- navazuje na potvrzeny FIX199/FIX200 XEX segment loader: STREAMED=OK ACTIVE=YES zustava
- CAS/WAV pilot z FIX200 je vypnuty, protoze test ukazal stejny/horsi obraz; zadny WAV soubor se ted nenahrava
- zadne Donkey/Cobra DLIST hard-locky, zadne profilove sablony
- hlavni obecna oprava: DLI rutina se pri snimani frame zachytava i pri zapisech na WSYNC $D40A
- renderer ted dostava mezilehle GTIA/ANTIC stavy uvnitr jedne DLI rutiny, ne jen finalni stav po navratu
- Cobra v purge rezimu dostala vyssi DLI budget, ale porad bez obrazove berlicky
- log nove hledej: DLI WSYNC CAPTURE FIX201, DLI CAP FIX201, DLIST RESOLVE FIX201

Test plan:
1. TEST SUPER COBRA XEX - menu + hra, porovnat sum proti FIX200 normal XEX.
2. TEST DONKEY KONG XEX - jestli obraz zustava smysluplny a jestli duchove/sum zmenily chovani.
3. G7 TUNNEL 96 TEST jen smoke.
4. ZVUK TEST jen smoke.
5. Snapshot z Cobry a Donkey, hlavne DLI WSYNC CAPTURE FIX201 / DLI CAP FIX201 / DLIST RESOLVE FIX201.

Poznamka:
FIX201 neni WAV loader. Je to navrat z neuspesne CAS/WAV pilotni vetve k obecnemu DLI/WSYNC casovani.
