# AtariHelp APK Builder v15 - Clean build + Mobile logo

Tohle je oprava po chybě:

```text
Error: Duplicate resources
drawable/ic_launcher.png
drawable/ic_launcher.xml
```

## Co bylo špatně

GitHub upload umí přepsat a přidat soubory, ale neumí sám smazat staré soubory.
V repozitáři zůstala stará ikona:

```text
app/src/main/res/drawable/ic_launcher.png
```

a vedle ní byla nová:

```text
app/src/main/res/drawable/ic_launcher.xml
```

Android build pak spadne, protože obě mají stejný resource název `ic_launcher`.

## Co opravuje v15

Workflow teď před každým buildem automaticky smaže staré duplicitní ikony:

```text
ic_launcher.png
ic_launcher.webp
ic_launcher.jpeg
ic_launcher.jpg
```

Takže i když v GitHubu starý PNG omylem zůstane, build ho v pracovním prostředí odstraní a pokračuje.

## Co je v appce

- N&P logo uvnitř appky
- bezpečná XML ikona aplikace
- mobilní AtariHelp centrum, ne kopie webu
- sekce Domů / Hry / Návody / N&P / Runner
- PiTT-KiTT v sekci Hry, ne na úvodní stránce
- XEX runner základ

## Poctivě ke hře

PiTT-KiTT XEX je uvnitř APK a runner ho načte, ale plně hratelné Atari ještě není.
Další krok bude náš N&P mini-emulátor pro hry, které budeme dělat podle vlastního jednoduchého standardu.

## Aktualizace na GitHubu

1. Rozbal tento ZIP.
2. Otevři svůj GitHub projekt `atarihelp-apk`.
3. Klikni `Code`.
4. Klikni `Add file` → `Upload files`.
5. Přetáhni obsah rozbalené složky.
6. Klikni `Commit changes`.
7. Klikni `Actions`.
8. Počkej na zelenou fajfku.
9. Stáhni artifact `HOTOVE_APK_ATARIHELP_STAHNI_ME`.
10. Rozbal a nainstaluj nové `app-debug.apk`.

Starou AtariHelp appku v mobilu před instalací radši odinstaluj.
