FIX194_GENERIC_RESET_OS_LOADER_CORE

Hlavni smer:
- Zadna dalsi jednorazova oprava jedne hry.
- FIX193 Donkey-only PMG experiment je vypnuty a neni v UI.
- XEX hry zustavaji pres cisty loader: segmenty + INITAD/RUNAD, BASIC ROM direct pro hry vypnuty.
- RESET z jakekoliv XEX hry ted skoci do REAL OS BASIC AUTO BOOT, tedy modra READY obrazovka.
- RESET v REALOS/BASIC zustava teply reset a ma zachovat BASIC program v RAM; program maze NEW.
- Virtualni klavesa RESET pouziva stejne pravidlo.
- Snapshot pise RESET READY FIX194 a GENERIC PMG POLICY FIX194.

Test:
1. CISTY XEX DONKEY a CISTY XEX COBRA musi zustat jako FIX191/192.
2. Behem hry zmacknout RESET -> musi nabehnout modra READY BASIC obrazovka.
3. V REAL OS BASIC napsat maly program, RESET, LIST -> program ma zustat.
4. G7 TUNNEL a ZVUK TEST jen smoke.
