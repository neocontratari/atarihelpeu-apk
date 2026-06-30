AtariHelp.eu EMU-10 BUILD2QI_SEGA_NATIVE_CPP_CORE_THREAD_RENDER_STAGE99

KODY JSOU ZMENENE: ANO

CIL:
- Opravit pad po vyberu ROM v predchozich native ClownMDEmu-core buildech.
- Normalni Sega obrazovka zustava.
- Zadna dalsi C++ aktivita/okno.
- C++ CORE bezi primo v monitoru Sega UI.
- Core uz neni volany z WebView JavaBridge threadu ani z View.onDraw.
- Real ClownMDEmu-core init/reset/iterate bezi v jednom dedikovanem native worker threadu.
- Monitor pouze kopiruje posledni cached framebuffer.

DULEZITE:
- 2QI neni fake pattern build.
- Kostky/ctverecky/bile bloky zustavaji vypnute.
- Audio hry jeste neni napojene; C++ AUDIO je stale pouze tone test.
- WebView Sega zustava zaloha.

TEST:
1. Nasad overlay pres GitHub Desktop.
2. Build APK.
3. Otevri SEGA.
4. Dej C++ CORE.
5. Vyber Aladdin/Sonic pres normalni CARTRIDGE/SBIRKA/HRY.
6. Sleduj, jestli appka nespadne.
7. Pokud nespadne, cekej 5-10 sekund na prvni cached frame.
8. Zkus DPAD/A/B/C/START.
9. Dej ULOZENE a posli log + screenshot.

CO POSLAT:
- jestli appka spadne: po C++ CORE / po vyberu ROM / az po chvili
- screenshot monitoru
- AtariHelp SEGA log

HLAVNI LOG MARKERY:
BUILD2QI_SEGA_NATIVE_CPP_CORE_THREAD_RENDER_STAGE99
REAL_CORE_THREAD_START_OK
threading=DEDICATED_NATIVE_WORKER_THREAD
render=CACHED_FRAME_COPY_ONLY_FROM_NATIVE_MONITOR_DRAW
REAL_CORE_WORKER_ALIVE=YES
REAL_CORE_RENDER_OK_WORKER_THREAD
