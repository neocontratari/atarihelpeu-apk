AtariHelp.eu EMU-09 FIX107 MULTIGAME BOOT + PMG/DONKEYJR CORE

Commit summary:
FIX107 multigame boot PMG Donkey Jr core

Changes:
- Separate Donkey Jr profile detection and diagnostics.
- Montezuma automatic gentle boot assist is blocked; let intro enter the maze, then use TAP START/FIRE manually.
- Montezuma PMG stale-row filter trims long PMG clusters to reduce vertical ghosts after respawn/death.
- Cobra title/menu fallback is preserved, but FIX107 does not add new Cobra-specific gameplay hacks.
- Donkey and Arkanoid baseline are preserved.

Test focus:
1) Donkey Kong Junior Title Version: screenshot + snapshot log.
2) Montezuma: intro -> gameplay -> death/respawn; check ghosts.
3) Super Cobra: quick sanity only; menu, collisions, bombs visibility.
4) Donkey and Arkanoid: regression check only.
