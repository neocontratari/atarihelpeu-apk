EMU-09 FIX105_SHARED_INPUT_PMG_COBRA_CORE

Commit summary:
FIX105 shared input PMG Cobra core

Commit description:
- Refine shared TRIG0 repeat cadence: touch FIRE now releases for only one frame per cycle.
- Align Cobra secondary DOWN/bomb window with every second trigger edge instead of long or permanent joystick injection.
- Preserve předchozí GTIA collision latch path and Cobra title/menu fallback.
- Add Montezuma PMG stale-row window filter to reduce vertical ghosts from old player rows.
- Preserve Donkey safe baseline and Arkanoid clean menu.
