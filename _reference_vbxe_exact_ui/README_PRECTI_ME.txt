VBXE 130XE EXACT JAVA OVERLAY - pro Reneho / AtariHelp.eu EMU-10
================================================================

CIL:
Udelat v Android Java aplikaci presne to, co je na PNG navrhu:
- obrazovka KEYBOARD modu 1:1
- obrazovka JOYSTICK modu po prepnuti 1:1
- zadny joystick pod klavesnici
- klavesnice a kazetak zustavaji original ve skinu
- tlacitka NET HRY, XEX MOBIL, ATR DISK, TURBO BASIC, BASIC/TBXL TXT, LOG/CHYBA, HELP, MENU zustavaji pod kazetakem
- monitor ma byt zivy emulatorovy slot, ne fake obrazek

PROC JE TO TAKTO:
Fotorealisticky vzhled nelze udelat 1:1 cistym kreslenim v Jave bez PNG skinu.
Spravne reseni je:
1) PNG skin = presna grafika.
2) Java = zobrazi PNG presne na celou obrazovku.
3) Java = presne hitboxy nad tlacitky.
4) Monitor = samostatny slot pro existujici emulatorovy WebView/Canvas/Surface.

SOUBORY:
app/src/main/res/drawable-nodpi/vbxe_130xe_keyboard_exact.png
app/src/main/res/drawable-nodpi/vbxe_130xe_joystick_exact.png
app/src/main/java/com/atarihelp/emu10/ui/Vbxe130xeExactSkinScreen.java
app/src/main/java/com/atarihelp/emu10/ui/MainActivity_integration_snippet.java

DULEZITE:
- Package v Java souboru mozna bude nutne zmenit podle realne MainActivity.
- Pokud emulator pouziva WebView, nesmi zustat pod celou grafikou. Musi se vlozit do monitorSlotu.
- Stary UI skin nenavrstit pod to. Nahradit obrazovku timto exact screenem.

KODY JSOU ZMENENE:
Meni se UI/skin/navigace/hitboxy.

BASIC / Altirra testovaci kody: KODY JSOU STEJNE.
Emulator core timto nesahat.
