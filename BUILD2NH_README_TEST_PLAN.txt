BUILD2NH_SEGA_CLOWNMD_URL_CARTRIDGE_GAMEPAD_STAGE21

TEST:
1) Rozbal ZIP pres koren projektu.
2) Spust APK/Nox.
3) Otevri Sega modul.
4) Klikni HRY / SBIRKA / CARTRIDGE SLOT.
5) Vyber Sonic .gen.
6) Cekej 10-15 sekund. Cilem je, aby ClownMDEmu nabehl rovnou pres ?cartridge= bez rucniho Software menu.
7) Klikni JOYSTICK a zkus START, D-PAD, A/B/C.
8) Otoč mobil/Nox na landscape a zkus obraz + ovladac.
9) Klikni ULOZENE a posli log.

HLEDEJ V LOGU:
- BUILD2NH ROM SELECTED
- BUILD2NH URL_CARTRIDGE DATA_URL_READY
- BUILD2NH CLOWNMD FRAME OPEN ... cartridge=data
- BUILD2NH CLOWNMD FRAME onload ... CARTRIDGE_URL
- BUILD2NH INPUT A/B/C ... KEY_SENT_URL_PAD

POCTIVE:
Pokud Android WebView/ClownMDEmu neumi natahnout Data URL z query, dalsi krok neni dalsi HTML trick, ale native/local server bridge.
