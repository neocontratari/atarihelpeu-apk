AtariHelp.eu EMU-09 FIX271_TURBOBASIC_XEX_STARTOPTION_WARMRESET_CORE

Forward base: FIX270 working FAST BASIC + FIX267/FIX261 stable UI.
No rollback.

Changes:
- Adds built-in asset app/src/main/assets/turbo-basic-xl-1.5-copy.xex.
- Adds Turbo BASIC XL XEX START+OPTION button.
- Uses normal EMU-09 XEX segment loader, not WAV, not RAM program inject.
- Holds START+OPTION only as real input state after loader start.
- Replaces keyboard RESET with WARM RESET BASIC path using OS WARMSV $E474 without cold fallback.
- Adds COLD RESET BASIC as separate button.
- Snapshot now reports FIX271 turbo XEX and warm reset audit.

NO FAKE READY / NO FAKE LOAD / NO PROGRAM RAM INJECT / NO SCREEN RAM WRITE / NO GAME HACK.

KODY JSOU STEJNE.
