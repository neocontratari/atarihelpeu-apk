AtariHelp.eu EMU-09 FIX113 MULTIGAME RESET/DLI/BOOT CORE

- Montezuma: frame-hold/stable-frame restore disabled for live game-over/reload transitions.
- Moon Patrol: loader handoff after INITAD for small RUNAD loader segments to larger loaded code segment.
- Pac-Man: BASIC/OS ROM dependency diagnostic, avoids pretending it is a normal machine-code XEX.
- Generic DLI cache: no stale DLI state reused across DLIST/frame changes.
- Arkanoid: keep gameplay DLIST diagnostics.
- Donkey Kong and Super Cobra: baseline preserved; no new Cobra-specific gameplay hacks.
