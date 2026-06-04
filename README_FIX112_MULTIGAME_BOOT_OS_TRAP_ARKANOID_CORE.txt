AtariHelp.eu EMU-09 FIX112 MULTIGAME BOOT/OS TRAP + ARKANOID CORE

Zamer:
- vratit Montezume stabilni reload/prechod bez stareho frameholdu po game over,
- zkusit Moon Patrol / Pac-Man obecne pres boot-trap guard a delsi INITAD pre-run,
- nehonit uz jen Cobru, ponechat Donkey/Cobra baseline,
- Arkanoid ponechat na DLIST $3C00 a sbirat lepsi diagnostiku char/DLI cesty.

Test:
1) Montezuma: game over, znovu nacteni, prechod do hry, snapshot log.
2) Moon Patrol: nacteni, 5-10 s nic, snapshot; pak Universal Assist, snapshot.
3) Pac-Man: nacteni, snapshot pri zaseku.
4) Arkanoid III: menu + hra snapshot.
5) Donkey/Cobra jen rychla regresni kontrola.
