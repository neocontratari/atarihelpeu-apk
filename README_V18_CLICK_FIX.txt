# AtariHelp.eu APK Builder v18 - Click fix

## Co opravuje

Ve v17 se úvodní obrazovka načetla, ale nešlo na nic kliknout.
To znamená, že JavaScript spadl nebo inline onclick nebyl v Android WebView spolehlivě dostupný.

V18:
- odstraněné inline onclick ovládání
- tlačítka mají data-view / data-action
- klikání se připojí až po DOMContentLoaded
- hra zůstává v mobilním portu
- manuál, ASM i PDF zůstávají přibalené
- název appky zůstává AtariHelp.eu

## Test po instalaci

1. Otevři appku.
2. Klikni Hry.
3. Klikni Hrát mobilní PiTT-KiTT.
4. Zkus LEFT / RIGHT / FIRE / JUMP.
5. Tlačítko Zpět musí zavřít hru.

## Aktualizace na GitHubu

1. Rozbal ZIP.
2. GitHub → atarihelp-apk → Code.
3. Add file → Upload files.
4. Přetáhni obsah rozbalené složky.
5. Commit changes.
6. Actions.
7. Stáhni HOTOVE_APK_ATARIHELP_STAHNI_ME.
8. Rozbal a nainstaluj app-debug.apk.

Před instalací nové verze odinstaluj starou AtariHelp.eu.
