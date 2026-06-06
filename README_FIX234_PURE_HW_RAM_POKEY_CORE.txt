AtariHelp.eu EMU-09 FIX234 PURE HW RAM POKEY CORE

KODY JSOU STEJNE. SUBMARINE BASIC, GTIA 9/10/11 VERIFY ani ostatni BASIC testy beze zmen.

Proc FIX234:
FIX233 log ukazal, ze POWER BASIC i POWER OPTION SELF TEST porad konci ve self-test smycce $532A-$533D. To neni spatna ROM; ROM jde do self-testu protoze OS RAM test spadne. Koren byl v emulatoru: v PURE HW se porad interceptovaly RAM shadow adresy $0278/$0284/$02FC jako joystick/klavesnice. XL OS RAM test zapisuje a cte oblast $0008-$05FF, takze tyhle helpery mu kazily RAM.

Oprava:
- v PURE HW jsou vsechny adresy pod $D000 plain RAM, pokud nejsou zrovna ROM okno
- OS shadow helpery STICK/STRIG/CH jsou v PURE HW vypnute pro CPU read
- POKEY $D20F write = SKCTL latch, read = SKSTAT status; uz se nemirruje $03
- POKEY $D20E write = IRQEN latch, read = IRQST status
- KBCODE $D209 je read-only idle $FF bez klavesy

Test:
1. PURE HW AUDIT + SNAPSHOT
2. POWER XL/XE BASIC + SNAPSHOT
3. POWER OPTION SELF TEST + SNAPSHOT

Hledej:
PURE HARDWARE SNAPSHOT FIX234
FIX234 ROOT CHECK
RAM TEST BUS
POKEY SPLIT
