# AtariHelp.eu EMU-09 FIX76 FIX69 INSTALL + N&P LOGO + COBRA

FIX76 reaguje na testy z 2026-06-02:

- GitHub Actions jsou vratene na overeny FIX69 debug APK styl: `assembleDebug` a artifact `HOTOVE_APK_ATARIHELP_STAHNI_ME`.
- N&P pecet z `pecet.png` je v launcher ikone i v `assets/np_logo.png`; EMU-09 ji zobrazuje i primo v appce.
- Manifest je vraceny smerem k FIX69 kvuli instalaci na telefonu: `INTERNET`, `allowBackup=true`, bez `roundIcon`.
- Mobilni ovladani je prestavene na levy palcovy kriz, pravy velky FIRE a Atari klavesnice je hned pod krizem.
- PiTT-KiTT Remaster: textove obrazovky maji cisty background a citelny fallback foreground, aby rekord/intro/konec nemizely.
- Super Cobra: FIRE je kratky puls a missile kolize je uzsi, hlavne podle spicky rakety.
- Galactic Chase a Pacman nejsou timto balikem prohlasene za opravene; zustavaji jako dalsi OS/loader prace.
- Hlavni vestavene testy jsou PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chraneny referencni titul.
- KiTT Garage zustava druha chranena reference.
- CORE TEST TXT se uklada do mobilu automaticky a po testu vraci puvodni hru.

Build:

FIX76_FIX69_INSTALL_LOGO_COBRA


