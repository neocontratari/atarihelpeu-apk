BUILD2QE_SEGA_NATIVE_CPP_STAGED_ROM_NO_CRASH_STAGE95

KODY JSOU ZMENENE: ANO

TEST:
1. Nasad overlay.
2. Build APK.
3. Otevri SEGA.
4. Dej C++ CORE.
5. Monitor ma byt tmavy s modrym okrajem, bez kostek/ctverecku/bilych bloku.
6. Vyber Aladdin/Sonic pres HRY/SBIRKA/CARTRIDGE.
7. Aplikace NESMI spadnout.
8. Dej ULOZENE a posli log.

OCEKAVANY LOG MARKERY:
- BUILD2QE_SEGA_NATIVE_CPP_STAGED_ROM_NO_CRASH_STAGE95
- ROM_STAGED_CPP_SAFE_NO_CRASH
- REAL_CORE_ROM_STAGED_ONLY_NO_NATIVE_EXEC
- CORE_NATIVE_AUTO_LOAD_DISABLED=YES
- pattern=OFF java-safe dark monitor only

POZNAMKA:
Tohle nespousti gameplay. Je to crash-stop build po tom, co 2QB/2QC/2QD padaly jeste pred logem.
