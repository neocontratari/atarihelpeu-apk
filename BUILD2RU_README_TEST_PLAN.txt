BUILD2RU_SEGA_NATIVE_CPP_ONLY_REGION_FIX_VISIBLE_FRAME_STAGE137

STATUS:
- KODY JSOU ZMENENE.
- Hotfix po BUILD2RT, kde JUE ROM byla natvrdo vybrana jako PAL a vysledek byl modry hlidaci monitor / zadny realny frame.
- BUILD2RU vraci JUE multi-region ROM na bezpecnou NTSC overseas cestu, ktera predtim kreslila realny obraz.
- E-only ROM zustava PAL, J-only ROM zustava JP NTSC, U/fallback zustava US NTSC.

OPRAVY:
1) JUE region uz NENI automaticky PAL.
   - Log: NTSC_US_60_OVERSEAS reason=JUE_AUTO_SAFE_NTSC_VISIBLE_FRAME_FIX ... palJueRtNoFrameFix=YES
   - Duvod: BUILD2RT log ukazal frameReady=NO, videoCaptured=0, audio nestartovalo kvuli NO_VISIBLE_FRAME.
2) PAL zustava jen pro skutecne E-only ROM.
   - Log: PAL_EU_50_OVERSEAS reason=ROM_E_ONLY
3) FIXED_RING_NO_DEQUE audit zustava zachovan.
4) PURE_CPP audit zustava zachovan.

TEST PLAN:
A) Nox / Sonic USA, Europe / STEREO
- Vybrat ROM pres CARTRIDGE.
- Ocekavani: musi se zobrazit realna hra, zadny modry auditni monitor.
- V logu hledej: BUILD2RU_SEGA_NATIVE_CPP_ONLY_REGION_FIX_VISIBLE_FRAME_STAGE137, JUE_AUTO_SAFE_NTSC_VISIBLE_FRAME_FIX, frameReady=YES, videoCaptured>0.

B) S8 / USA ROM, ktera hrala ciste
- Ocekavani: obraz nabehne, zvuk porovnat s predeslym spravnym USA chovanim.

C) S8 / EU-only ROM
- Ocekavani: pokud header je E-only, log musi ukazat PAL_EU_50_OVERSEAS reason=ROM_E_ONLY.
- Pokud EU-only stale chrci, poslat log. Pak uz budeme mit oddelene: JUE viditelny frame fix vs skutecny PAL audio problem.
