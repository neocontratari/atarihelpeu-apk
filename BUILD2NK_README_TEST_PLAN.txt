BUILD2NK_SEGA_CLOWNMD_CLEAN_DPAD_ABC_RESET_STAGE24

CIL:
- opravit chybu z BUILD2NJ: skok nesmi byt na krizovem ovladaci / DPAD uz neposila A/S/D, jen sipky.
- A/B/C zkusi znovu: default Z/X/C + 6-button A/S/D jen na akcni tlacitka + virtual gamepad mirror.
- RESET uz nespousti dalsi instanci pres starou; nejdriv ciste zabije wrapper/iframe a az potom znovu bootuje aktualni ROM.
- CARTRIDGE SLOT zustava funkcni pro vyber nove hry.

TEST:
1. Otevri Sega modul.
2. Klikni CARTRIDGE SLOT / HRY / SBIRKA a vyber Sonic.
3. Over, ze DPAD jen chodi/sklani, ale neskace.
4. Over A/B/C: skok.
5. Klikni CARTRIDGE SLOT a vyber jinou ROM.
6. Klikni RESET a over, ze dalsi hra uz neni brutalne zpomalena.
7. Zkus landscape.
8. ULOZENE -> posli log.
