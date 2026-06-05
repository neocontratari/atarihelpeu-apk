AtariHelp.eu EMU-09 FIX199 HW DLIST TRUST SEGMENT CORE

Commit summary:
FIX199 hw dlist trust segment core

Co je ve FIX199:
- FIX198 RAM scan umel vybrat nahodna obrazova/kodova data jako DLIST ($0925/$4E78).
- FIX199 meni poradi obecneho XEX renderu: skutecny ANTIC HW DLIST/SDLST prvni, XEX segment scan druhy, RAM scan az posledni diagnosticky fallback.
- HW/SDLST pointer se uz neodmita jen proto, ze bajty vypadaji jako 6502 kod; ANTIC DLIST instrukce se s CPU opcode prirozene prekryvaji. Kandidat musi mit rozumny ANTIC tvar a platne LMS.
- Zadny Donkey/Cobra hard-lock a zadne profilove DLIST sablony.
- Loader porad hlasi STREAMED=OK ACTIVE=YES.
- Log: XEX STREAM STATUS FIX199, NATIVE XEX LOADER FIX199, HARDWARE PURGE FIX199, HW DLIST TRUST FIX199, DLIST RESOLVE FIX199.

Test:
1. TEST SUPER COBRA XEX - menu i hra.
2. TEST DONKEY KONG XEX - jestli se vrati z uplne rozsypaneho FIX198 aspon na predchozi obecny stav.
3. Snapshot/log z obou.

Poznamka k WAV/CAS:
WAV/CAS loader muze byt dalsi paralelni experiment, ale sam o sobe neopravuje ANTIC/DLIST vykresleni po nahrani hry. FIX199 proto nejdriv stabilizuje obecny DLIST resolver.
