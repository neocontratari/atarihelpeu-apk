AtariHelp.eu EMU-09 FIX245_VERIFY_BUILD_STAMP_CORE

Duvod:
- Rene hlasi, ze po aplikaci FIX244 snapshot stale ukazuje BUILD FIX240.
- FIX244 ZIP obsahoval nove jadro, ale handoff nebyl dostatecne verifikovatelny: index mel stale stare texty a build tag byl prepisovan az pozde v JS.

FIX245:
- tvrdy build stamp hned v emu09_pmg_gtia_overlay.html: EMU_BUILD_TAG = FIX245_VERIFY_BUILD_STAMP_CORE
- index.html ma cache-bust link na emu asset
- snapshot musi obsahovat BUILD FIX245_VERIFY_BUILD_STAMP_CORE a FIX245 VERIFY BUILD STAMP CHECK
- pokud se stale ukaze BUILD FIX240, nebezi tento overlay/APK
- core logika FIX244 pro FAST BASIC screen-editor helper a SELF TEST DLIST DMA renderer zustava zachovana
- bez fake READY, bez fake SELF TEST

KODY JSOU STEJNE.
