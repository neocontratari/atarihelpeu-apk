BUILD2NM_SEGA_CLOWNMD_TWO_PHASE_POWER_STAGE26

TEST PLAN:
1. Rozbal overlay pres koren projektu.
2. Otevri Sega modul.
3. Vyber Sonic pres HRY / SBIRKA / CARTRIDGE SLOT.
4. Otestuj ovladac - nema se zmenit proti BUILD2NK.
5. Dej RESET 3x po sobe.
6. Vyber jinou hru pres CARTRIDGE SLOT / HRY / SBIRKA.
7. Dulezite: pri vymene hry se ma kratce objevit SEGA POWER OFF mezikrok a potom nova hra.
8. Kontrola: druha hra se nesmi brutalne zpomalit.
9. Zkus jeste RESET po druhe hre.
10. ULOZENE -> posli log.

LOG MARKERY:
- BUILD2NM HARD_POWER_BOOT
- BUILD2NM POWER_OFF_WRAPPER_BEGIN
- BUILD2NM TWO_PHASE_POWER_NAV
- WRAPPER POWER_OFF_REQUEST
- BUILD2NM HARDBOOT AUTOBOOT
- WRAPPER MODULE_POSTRUN

KDYZ SE TO ZPOMALI:
Posli log a napis, jestli se mezi hrami ukazala cerna mezistranka SEGA POWER OFF.
