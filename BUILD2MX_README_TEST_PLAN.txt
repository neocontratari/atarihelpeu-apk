AtariHelp.eu EMU-10 BUILD2MX_SEGA_ORIGIN_GUARD_SEMICOLON_STAGE11

CIL:
Opravit konkretni chybu z BUILD2MW logu:
CLOSURE_EVAL_COMPILE_FAILED online:lrusso/Genesis.min.js Unexpected token 'var'

TEST:
1. Rozbal ZIP overlay do korene projektu pres app/.
2. Buildni/spust APK.
3. Otevri SEGA modul.
4. Pres HRY / SBIRKA / CARTRIDGE vyber Sonic .gen.
5. Kratce klikni ULOZENE a posli TXT log.

CO HLEDAT V LOGU:
- nesmi se vratit `Unexpected token 'var'`
- hledej `ORIGIN GUARD SEMICOLON PATCH`
- hledej `CLOSURE EVAL OK embedGenesis exported` nebo novou konkretni chybu
- kdyz nabehne core: `embedGenesis CALLBACK STARTED`

POCTIVE:
Pokud BUILD2MX stale neukaze realny obraz, ukoncit online lrusso patchovani a jit na lokalni/offline core soubor nebo native/JNI cestu.
