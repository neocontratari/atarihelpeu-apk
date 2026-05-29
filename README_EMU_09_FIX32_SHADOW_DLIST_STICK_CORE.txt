EMU-09 FIX32 SHADOW DLIST + STICK CORE

- Rebase from FIX31.
- PiTT-KiTT remains the protected reference XEX.
- Generic XEX core fixes:
  - Prefer valid Atari OS shadow display list SDLSTL/SDLSTH ($0230/$0231) when games update it.
  - This is critical for Pitstop II: it leaves hardware DLIST at old $B38F while shadow SDLST points to $A0D7.
  - OS STICK0 shadow ($0278) now returns Atari nibble values 0F/0E/0D/0B/07, not full PORTA bytes like FF/FE.
  - Input shadows STRIG0 and CH are kept in RAM for programs reading shadow memory directly.
  - Generic DLI capture uses DLI scanlines from display-list $80 bits.
  - Generic frame DLI can run scheduled DLI routines; not a per-game River patch.

Test targets:
1. PiTT-KiTT must remain OK.
2. Pitstop II snapshot should show DLIST=$A0D7, not $B38F.
3. Arkanoid III menu should have better joystick/keyboard shadow handling; FIRE/START should still enter game.
