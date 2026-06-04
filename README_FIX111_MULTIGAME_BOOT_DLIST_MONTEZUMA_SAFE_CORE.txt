AtariHelp.eu EMU-09 FIX111 MULTIGAME BOOT/DLIST + MONTEZUMA SAFE CORE
====================================================================

Commit Summary
--------------
FIX111 multigame boot DLIST Montezuma safe core

Commit Description
------------------
- Move back to broad emulator-core work instead of chasing only Super Cobra.
- Add safer boot path for unknown XEXs: no hidden automatic START/FIRE for generic, Moon Patrol or Pac-Man-style profiles.
- Add filename/profile detection for Moon Patrol and Pac-Man so future logs are separated from generic unknowns.
- Add Montezuma START/FIRE gate: early input is queued until the intro/maze state is safe, reducing the chance of corrupt graphics when the timing is missed.
- Scope ANTIC mode 4/5 64-glyph color-attribute rendering to Arkanoid/Donkey Jr only; Montezuma returns to the safer legacy 128-glyph path for score/body text.
- Reduce Donkey Jr transition flicker by allowing live title/menu DLIST switching instead of holding/restoring $5919/$8052 frames.
- Preserve Donkey Kong and Super Cobra baseline; Cobra remains a reference test, not the main tuning target.
- Update visible UI/log/build labels to FIX111.

Test Plan
---------
1. Arkanoid III: menu, then START/FIRE into game. Capture screenshot + snapshot log. Report whether gameplay tea/noise is same, smaller or worse.
2. Montezuma: press START/FIRE even if timing is not perfect; FIX111 should queue it until safer. Check score/body text and death/respawn.
3. Donkey Kong Junior: first title/intro, then menu. Check blinking and text alignment.
4. Donkey Kong + Super Cobra: quick regression only.
5. New broad tests: Moon Patrol and Pac-Man. Load XEX, wait 5-10 seconds without Universal Assist, then screenshot + snapshot log. If stuck, use Universal Assist and save a second log.

Notes
-----
FIX111 is intentionally not a magic one-game patch. It adds safer loader/start behavior and better diagnostics for the next wave of broken XEXs.
