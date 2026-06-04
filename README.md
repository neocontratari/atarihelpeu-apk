AtariHelp.eu APK Builder - EMU-09 FIX115

Aktuální balík: FIX115_MONTEZUMA_MOON_IDLE_CORE.

Směr: obecnější kompatibilita místo ladění jedné hry.
- Montezuma má zvlášť PRELIM a ORIGINAL vestavěný test.
- Moon Patrol handoff už nesmí skočit do segmentu, který vypadá jako data.
- Pac-Man má jasnou BASIC/OS ROM diagnostiku.
- Donkey Kong, Super Cobra a Arkanoid baseline jsou chráněné.

Podrobnosti jsou v README_FIX115_MONTEZUMA_MOON_IDLE_CORE.txt.


## FIX115 MONTEZUMA/MOON IDLE CORE
- Montezuma PRELIM/ORIG/ALT comparison and death/reload render-cache guard.
- Moon Patrol VVBLKD idle fallback instead of unsafe loader handoff into data.
- Pac-Man remains BASIC/OS diagnostic.
