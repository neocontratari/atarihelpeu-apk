EMU-09 FIX116 OS SHIM MONTEZUMA CHBASE CORE

Co je nové:
- Mini OS/CIO shim: zachytává CIOV/SIOV/SETVBV a bezpečněji vrací ROM-less XEXy z OS/BASIC/I/O volání.
- Montezuma CHBASE guard: když ORIG/ALT drží CHBASE na $E0, renderer i DLI capture preferuje skutečný charset/body blok $3000/$3800 jako $0C.
- Montezuma death/reload: čistí pouze emulační render/DLI/PMG cache, ne Atari RAM.
- Moon Patrol: místo prázdného idle loopu zkouší code-window handoff a diagnostikuje vybraný kandidát v logu moonWin/osShim.
- Pac-Man: dál ukazuje BASIC/OS závislost, ale s bezpečnějším OS návratem pro diagnostiku.
- Donkey, Cobra a Arkanoid jsou chráněné proti regresi.

Test:
1) Montezuma PRELIM: třetí level, lebka/nástrahy, smrt/reload.
2) Montezuma ORIG a ALT: jestli CHBASE guard zlepší rozhozenou grafiku.
3) Moon Patrol: 5-10 sekund, screenshot + snapshot log.
4) Pac-Man: nechat doběhnout do seknutí, snapshot log.
5) Donkey/Cobra/Arkanoid: rychlá kontrola baseline.

Důležité: pokud v aplikaci nevidíš FIX116_OS_SHIM_MONTEZUMA_CHBASE_CORE, běží starý APK/ZIP nebo starý label.
