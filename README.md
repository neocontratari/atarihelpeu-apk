AtariHelp.eu APK Builder - EMU-09 FIX116

Aktuální balík: FIX116_OS_SHIM_MONTEZUMA_CHBASE_CORE.

Směr: obecnější kompatibilita místo ladění jedné hry.
- Přidán silnější mini OS/CIO shim pro ROM-less XEXy, které padají do $0000/$Dxxx po OS volání.
- Montezuma PRELIM/ORIG/ALT má CHBASE guard pro $3000/$3800 charset/body data a death/reload cache reset bez mazání Atari RAM.
- Moon Patrol zkouší code-window handoff místo čistě segmentového handoffu nebo prázdného idle loopu.
- Pac-Man dál hlásí BASIC/OS závislost, ale OS shim se snaží o bezpečnější návraty.
- Donkey Kong, Super Cobra a Arkanoid baseline jsou chráněné.

Podrobnosti jsou v hlavním EMU-09 HTML a ve snapshot logu FIX116.
