BUILD2NG_SEGA_CLOWNMD_CARTRIDGE_DROP_GAMEPAD_LANDSCAPE_STAGE20

TEST:
1. Rozbal ZIP pres koren projektu.
2. Spust APK/Nox.
3. Otevri SEGA modul.
4. Klikni HRY / SBIRKA / CARTRIDGE SLOT.
5. Vyber Sonic .gen jako NAS cartridge.
6. Pockej 5-10 sekund, jestli ClownMDEmu prijme auto drag/drop.
7. Klikni JOYSTICK pro herni crop/focus.
8. Otestuj START, DPAD, A/B/C.
9. Otoč mobil/Nox do landscape a zkus obraz + ovladac.
10. Klikni ULOZENE a posli LOG + screenshot.

CEKANE:
- ClownMDEmu se otevre v nasi monitorove obrazovce.
- BUILD2NG zkusi vybrany ROM soubor poslat jako DROP do ClownMDEmu bez rucniho menu Software.
- A/B/C neposila jen keyboard Z/X/C, ale i virtual Gamepad API.

KDYZ AUTO CARTRIDGE NEPROJDE:
- LOG bude obsahovat DROP_FAILED nebo DataTransfer/Drop chybu.
- V tom pripade pujde porad rucne KONZOLE > Software, ale dalsi krok musi byt native/lokalni ClownMDEmu build, ne dalsi grafika.
