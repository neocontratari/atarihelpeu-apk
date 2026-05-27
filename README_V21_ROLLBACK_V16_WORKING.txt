# AtariHelp.eu v21 - ROLLBACK NA FUNKCNI v16

Tahle verze je zamerne nudna a opatrna.

## Co je v21

- vychazi primo z v16, kde uz uzivateli fungovala tlacitka
- zachovana puvodni funkcni logika menu z v16
- zachovany nazev AtariHelp.eu
- zachovane N&P logo v aplikaci
- zachovana app_icon.png
- zachovany cleanup duplicitni ic_launcher.png ve workflow
- jen zvednuta verze na 0.21-rollback-v16-working

## Co jsem zamerne NEUDELAL

- neprepisoval jsem menu
- nepridaval jsem novou JS navigaci
- neprepisoval jsem to na multi-page
- neprepisoval jsem to na nativni Android
- necpal jsem do toho novou hru najednou

## Proc

v16 fungovala. Dalsi verze jsem rozbil tim, ze jsem menil moc veci najednou.
Od ted pouze male kroky z funkcni v16.

## Test

1. Nainstaluj app-debug.apk z tohoto buildu.
2. Otevri AtariHelp.eu.
3. Klikni Domu / Hry / Navody / N&P / Runner.
4. Pokud funguje stejne jako v16, pokracujeme dalsim malym krokem.

## Aktualizace

1. Rozbal ZIP.
2. GitHub -> atarihelp-apk -> Code.
3. Add file -> Upload files.
4. Pretahni obsah rozbalene slozky.
5. Commit changes.
6. Actions.
7. Stahni HOTOVE_APK_ATARIHELP_STAHNI_ME.
8. Rozbal a nainstaluj app-debug.apk.
