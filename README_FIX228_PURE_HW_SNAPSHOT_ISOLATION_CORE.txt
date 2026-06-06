AtariHelp.eu EMU-09 FIX228_PURE_HW_SNAPSHOT_ISOLATION_CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy nejsou zmenene.

Proc tento fix:
FIX227 audit byl OK, ale SNAPSHOT bez nastartovane CPU spadl do stare pojistky a zavolal XEX loader. Tim se vratil vestaveny PiTT-KiTT a log uz nebyl relevantni.

Co FIX228 meni:
- PURE HW AUDIT prepnul UI do audit-only PURE rezimu bez XEX.
- PURE HW SNAPSHOT nikdy nevola loadXex().
- PURE HW SNAPSHOT nevypisuje historicky diagnosticky spam ze starych FIXu.
- PURE HW status nevypisuje XEX profil ani stare mobile guard texty.
- Snapshot pise jen: build, ROM md5, vectors, PIA OUT/DDRB/PBCTL/effective, input queues, CPU, DLIST/SAVMSC/NMIEN, POKEY regs, ANTIC, trace.

Test:
1. PURE HW AUDIT.
2. SNAPSHOT. Nesmí nahrat PiTT-KiTT ani jinou hru.
3. POWER XL/XE BASIC. Snapshot.
4. POWER OPTION SELF TEST. Snapshot.

Commit summary:
FIX228 pure hw snapshot isolation core
