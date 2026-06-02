# AtariHelp.eu EMU-09 FIX77 RELEASE + 240 VIEWPORT + COBRA UI

FIX77 reaguje na testy z 2026-06-02:

- GitHub Actions vyrabi release APK: `assembleRelease` a artifact `HOTOVE_RELEASE_APK_ATARIHELP_STAHNI_ME`.
- N&P pecet z `pecet.png` je v launcher ikone i v `assets/np_logo.png`; EMU-09 ji zobrazuje i primo v appce.
- Manifest nema INTERNET opravneni, ma N&P icon i roundIcon a appka cisti WebView cache, aby se nevracel stary obraz/logotyp.
- Obecny ANTIC renderer kresli 320x240, aby Cobra a dalsi hry neztracely spodni HUD/FUEL cast.
- Mobilni ovladani je prestavene na levy palcovy kriz bez START uprostred, pravy velky FIRE a Atari klavesnice je hned pod krizem.
- TAP START je nad FIRE; smerova tlacitka pouzivaji pointer capture pro plynulejsi drzeni palcem.
- PiTT-KiTT Remaster: textove obrazovky maji cisty background a citelny fallback foreground, aby rekord/intro/konec nemizely.
- Super Cobra: FIRE je kratky puls, HITCLR maze kolize hned a missile kolize se cte cerstveji pri GTIA read.
- Galactic Chase a Pacman nejsou timto balikem prohlasene za opravene; zustavaji jako dalsi OS/loader prace.
- Hlavni vestavene testy jsou PiTT-KiTT Remaster, Donkey Kong, Pitstop II, KiTT Garage a Super Cobra.
- PiTT-KiTT Remaster zustava chraneny referencni titul.
- KiTT Garage zustava druha chranena reference.
- CORE TEST TXT se uklada do mobilu automaticky a po testu vraci puvodni hru.

Build:

FIX77_RELEASE_VIEWPORT_COBRA_UI


