AtariHelp.eu EMU-09 FIX219_REALROM_VBI_SIO_BOOT_CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 a dalsi BASIC testy nejsou menene.

Cil:
- PURE ROM BASIC BOOT ve FIX218 uz postavil DLIST $9C20 a SAVMSC $9C40, ale zustal jen kurzor bez READY.
- Log ukazuje PC kolem $EA9E, tedy real OS ceka v SIO/POKEY boot smycce.
- FIX219 proto nepouziva fake READY, ale zapina real ROM VBI volani a minimalni SIO no-device timeout, aby ROM pokracovala k BASIC cartridge vektoru.

Zmeny:
- goodEntry povoli REALOS/PURE ROM VBI rutiny v OS ROM ($C000-$CFFF / $D800-$FFFF), hlavne $C0E2.
- PURE ROM BASIC ma emulaci no-device navratu ze SIO boot smycky ($EA9E/$EB16/$EB24), pokud uz real OS postavil editor DLIST/SAVMSC.
- RESET/ROM/GTIA/BASIC test kody beze zmeny.

Test:
1. PURE ROM BASIC BOOT - cil je READY nebo aspon posun z PC $EA9E.
2. Snapshot TXT.
3. PURE ROM SELF TEST - jen snapshot, obraz muze byt stale spatny.
