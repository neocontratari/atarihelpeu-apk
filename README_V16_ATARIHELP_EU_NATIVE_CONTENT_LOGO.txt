# AtariHelp.eu APK Builder v16 - Native content + real logo

## Co opravuje v16

- Název aplikace je `AtariHelp.eu`
- Appka už slepě neotevírá PC web jako hlavní obsah
- Obsah je postavený jako mobilní sekce:
  - Domů
  - Hry
  - Návody
  - N&P
  - Runner
- PiTT-KiTT není úvodní stránka
- N&P logo je uvnitř aplikace
- Ikona APK používá skutečný obrázek loga jako `app_icon.png`
- Workflow pořád čistí staré duplicitní `ic_launcher.png`

## Hra

Poctivě:
PiTT-KiTT XEX je uvnitř APK a runner ho načte.
Plně hratelné to ještě není, protože chybí video/joystick emulace nebo mobilní port hry.

Nejrychlejší cesta ke hratelné hře:
1. Pošli zdroják PiTT-KiTT nebo pravidla + obrázky.
2. Udělám mobilní port do našeho N&P engine.
3. Paralelně budeme stavět mini-emulátor pro nové komunitní hry.

## Co od tebe budu potřebovat pro mobilní obsah

Pro každou sekci stačí dodat:
- název
- krátký popis
- obrázek/screenshot
- odkaz na PDF/WAV/XEX/TXT
- prioritu: co má být v appce první

## Aktualizace na GitHubu

1. Rozbal ZIP.
2. GitHub → `atarihelp-apk` → Code.
3. Add file → Upload files.
4. Přetáhni obsah rozbalené složky.
5. Commit changes.
6. Actions.
7. Stáhni artifact `HOTOVE_APK_ATARIHELP_STAHNI_ME`.
8. Rozbal a nainstaluj `app-debug.apk`.
