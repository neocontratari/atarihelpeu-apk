AtariHelp.eu EMU-09 FIX221 REALROM STACK IRQ CORE

KODY JSOU STEJNE.
SUBMARINE BASIC, GTIA 9/10/11 ani ostatni BASIC testy nejsou zmenene.

Cil:
- PURE ROM BASIC BOOT nesmi po VBI/SIO poskodit stack a po prikazu spadnout do $0000/$0100.
- PURE ROM SELF TEST uz nema nekonecne vektorovat BRK z $0000.

Zmeny:
- runAsyncAtariRoutine v PURE ROM rezimu uklada a obnovuje celou stack page $0100-$01FF.
- BRK v PURE ROM z low RAM $0000-$01FF se uz nebere jako platny OS BRK; zaloguje se FIX221 low-RAM BRK guard.
- Realny BRK v PURE ROM pushuje spravny navrat PC+2 z adresy opcodu.
- Snapshot obsahuje FIX221 PURE STACK.

Test:
1. PURE ROM BASIC BOOT: musi nabehnout READY bez nutnosti RESET, nebo posli snapshot.
2. Zadej kratky prikaz PRINT FRE(0) nebo LIST. Pokud se sekne, posli snapshot.
3. PURE ROM SELF TEST: screenshot + snapshot.
