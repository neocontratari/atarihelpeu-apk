BUILD2MY_SEGA_DOC_CURRENTSCRIPT_BIND_STAGE12

TEST PLAN:
1. Rozbal ZIP overlay pres koren projektu.
2. Spust APK / Nox.
3. Otevri SEGA modul.
4. Pres HRY / SBIRKA / CARTRIDGE vyber Sonic .gen.
5. Sleduj monitor: nema zustat jen Illegal invocation.
6. Zkus kratce KONZOLE a JOYSTICK: nema se zobrazit velka textova servisni obrazovka pres monitor.
7. Klikni kratce ULOZENE a posli TXT log.

V LOGU HLEDAT:
- CURRENT_SCRIPT TOKEN PATCH ... realDocument=YES
- CLOSURE EVAL OK embedGenesis exported
- embedGenesis CALL RETURNED ...
- idealne embedGenesis CALLBACK STARTED

KDYZ SELZE:
Posli cely AtariHelp_SEGA_LOG_*.txt a screenshot monitoru.
