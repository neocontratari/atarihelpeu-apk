AtariHelp.eu EMU-10
BUILD2QG_SEGA_NATIVE_CPP_REAL_CORE_RENDER_FIRST_STAGE97

KODY JSOU ZMENENE: ANO

CIL:
- zastavit laboratorni ctverecky/kostky
- nepouzivat dalsi C++ okno
- spustit prvni REAL ClownMDEmu-core render primo v normalnim Sega monitoru
- odstranit predchozi soubezny pristup k core: ROM picker thread vs UI render thread

CO JE ZMENENO:
- C++ CORE zustava v normalni Sega obrazovce
- po vyberu ROM se realny ClownMDEmu-core inicializuje pod jednim mutexem
- NativeInPlaceView render vola ClownMDEmu_Iterate() pod stejnym mutexem
- zadne fake moving cubes, zadny center square, zadne bile hash bloky
- audio hry jeste neni zapojene; C++ AUDIO zustava jen tone test
- WebView Sega zustava zaloha

TEST:
1. Nasad overlay pres GitHub Desktop.
2. Nech vyrobit APK.
3. Otevri SEGA.
4. Dej C++ CORE.
5. Vyber Aladdin/Sonic pres normalni cartridge.
6. Ocekavani: v monitoru se ma objevit prvni realny native core obraz/render, ne kostky.
7. Zkus DPAD/A/B/C/START.
8. Dej ULOZENE a posli log + screenshot.

KDYZ SPADNE:
- napis presne jestli po C++ CORE, po vyberu ROM, nebo az po chvili renderu
- kdyz jde, posli GitHub/logcat/log

LOG MARKERY:
BUILD2QG_SEGA_NATIVE_CPP_REAL_CORE_RENDER_FIRST_STAGE97
REAL_CORE_LOAD_OK
threading=single_mutex_java_ui_render_no_concurrent_core_access
render=ON first real ClownMDEmu_Iterate from native monitor draw
REAL_CORE_RENDER_OK
