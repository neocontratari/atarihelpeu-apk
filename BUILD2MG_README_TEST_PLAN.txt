BUILD2MG_RETRO_MAIN_ROUTER_EMU2_SEGA_SAFE

Ucel:
- Oprava nedorozumeni z BUILD2MF.
- Zakladni obrazovka N&P VISION se NEMA prepisovat.
- Velke tlacitko EMULATOR ma vest na soucasny EMU2/VBXE Atari modul.
- Spodni leve tlacitko ATARI / puvodni EMU2 slot ma vest na SEGA slot.
- Ostatni tlacitka/funkce zustavaji zachovane.

Zaklad:
- Atari/VBXE index je z BUILD2ME, bez MF zmen NET HRY -> SEGA uvnitr Atari.
- Atari/VBXE jadro beze zmen.
- CLOAD/CSAVE beze zmen.
- Player / AtariHelp / Help / Menu na zakladni obrazovce se v tomto overlayi nemenily.
- KODY JSOU ZMENENE jen v router/slot vrstve.

Co overlay meni/pridava:
1) app/src/main/assets/emu_vbxe/index.html
   - vraceny cisty Atari/VBXE modul z BUILD2ME.
   - vnitrni Atari tlacitka NET HRY/XEX/ATR/TXT/LOG/HELP zustavaji jako Atari servis.

2) app/src/main/assets/emu/index.html
   - stary EMU1 slot presmeruje na ../emu_vbxe/index.html.
   - cil: velke tlacitko EMULATOR na hlavni obrazovce otevre nase EMU2/VBXE.

3) app/src/main/assets/emu1/index.html + emu_1/index.html
   - bezpecne aliasy pro EMU1 -> EMU2/VBXE.

4) app/src/main/assets/emu2/index.html + emu_2/index.html + atari/index.html + atari_8bit/index.html + legacy_atari/index.html
   - bezpecne aliasy pro spodni ATARI/EMU2 legacy slot -> SEGA.

5) app/src/main/assets/emu_sega/index.html
   - samostatny SEGA placeholder slot.
   - zadny Mega Drive core, zadne ROM, zadny fake Sonic.
   - ZPET NA MENU vraci na ../index.html.

Test plan:
1. Rozbal overlay do repozitare pres GitHub Desktop.
2. Commit: BUILD2MG main router EMU2 Sega slot.
3. Nech GitHub Actions postavit APK.
4. Spust appku.
5. Hlavni obrazovka N&P VISION musi zustat stejna graficky.
6. Stiskni velke tlacitko EMULATOR:
   Ocekavani: otevre se Atari/VBXE 130XE emulator, ne stary EMU1.
7. Vrat se MENU.
8. Stiskni spodni leve tlacitko ATARI / ATARI 8-BIT LEGACY:
   Ocekavani: otevre se SEGA MEGA DRIVE placeholder.
9. V SEGA placeholderu stiskni ZPET NA MENU:
   Ocekavani: vrati se hlavni N&P VISION menu.
10. Otestuj, ze PREHRAVAC, ATARIHELP.EU, HELP zustaly jako predtim.

Pokud spodni leve ATARI tlacitko porad otevira Atari/VBXE:
- znamena to, ze hlavni menu miri primo na emu_vbxe/index.html, ne na emu2/atari alias.
- v tom pripade je potreba poslat konkretni soubor hlavni obrazovky index.html z repozitare, aby se prehodil presny onclick toho jedineho tlacitka.
