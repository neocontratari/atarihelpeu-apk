BUILD2PD TEST PLAN

1) Nox portrait:
- zkontrolovat, ze zvuk zustal dobry jako 2PA/2PC.
- ovladani se nesmi zhorsit.

2) Mobil landscape Sonic/Aladdin:
- zkontrolovat, ze jsou videt zivoty/HUD nahore.
- zkontrolovat, ze neprosvitaji horni napisy SBIRKA/ULOZENE/HRY/NAVODY/WEB/MENU/CART.
- zkontrolovat, ze joystick + skok funguje stale jako BUILD2PA.

3) Zvuk:
- ocekavany stav: stale spatny/ozvena muze zustat.
- pokud stale spatny, je potvrzeno: problem neni UI ani multitouch, ale remote ClownMDEmu audio pipeline v Android WebView.

LOG MARKERY:
- BUILD2PD_SEGA_MOBILE_SCREEN_GHOST_AUDIO_REPORT_STAGE68
- LANDSCAPE_MULTITOUCH_ENGINE_READY
- image tune: REAL_SCREEN_MASKED_USER_PD portraitFrom2OT + LANDSCAPE_HUD_SAFE_NO_GHOSTS + multitouch2PA
- audio tune: MOBILE_AUDIO_UNRESOLVED_REMOTE_CLOWNMDEMU_WEBVIEW baseLatencyKnown; UI_NOT_CAUSE
