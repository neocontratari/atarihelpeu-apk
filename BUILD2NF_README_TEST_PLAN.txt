BUILD2NF_SEGA_CLOWNMD_FIT_KEYS_STAGE19

TEST:
1. Rozbal ZIP pres koren projektu.
2. Spust APK/Nox.
3. Otevri Sega modul.
4. Klikni HRY nebo CARTRIDGE SLOT.
5. V ClownMDEmu menu Software vyber Sonic ROM.
6. Az Sonic bezi, klikni JOYSTICK - prepnuti do herniho vyrezu schova horni menu a vycentruje obraz.
7. Testuj D-PAD / A / B / C / START.
8. Klikni ULOZENE a posli log.

OCEKAVANI:
- obraz zustane v nasi Sega obrazovce
- A/B/C hitboxy sedi na skutecnych tlacitkach ve skinu
- D-PAD je posunuty na skutecny ovladac
- START zustava Enter

POCTIVE OMEZENI:
- automaticke predani lokalni ROM do cross-origin oficialniho ClownMDEmu iframe nejde ciste udelat pres HTML overlay kvuli bezpecnosti prohlizece.
- Pokud tlacitka porad neovladaji hru, dalsi krok neni dalsi JS trik, ale native WebView KeyEvent bridge nebo lokalni/native ClownMDEmu integrace.
