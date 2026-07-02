BUILD2RY_SEGA_NATIVE_CPP_ONLY_CORE_O3_VDP_LEFT_BLANK_STAGE141

=========================================================
DVE NALEZENE PRICINY (obe cerne na bilem):
=========================================================

1) S8 ZVUK - JADRO EMULATORU SE STAVELO BEZ OPTIMALIZACE (-O0)
   - GitHub Actions vyrabi DEBUG APK.
   - build.gradle mel jen cppFlags "-std=c++17 -O2" - to plati POUZE pro C++
     frontend soubor. Cele jadro emulatoru (68000, Z80, VDP, FM) jsou .c soubory
     a zadne cFlags neexistovaly => debug build je prekladal s -O0.
   - Tvuj RX audit to potvrdil cisly:
     * S8 Sonic NTSC: coreAvgMs=17.0-18.1 uz ZA STUDENA (34.6 C) pri max taktu
       2314 MHz. Budget NTSC je 16.67 ms => hudba chrci OD ZACATKU. Sedi s testem.
     * S8 Senna PAL: budget 20 ms => za studena to tesne stihalo => cista hudba.
       Pak CPU podtaktoval: cpu4KHz 2314 -> 2002 -> 1807 -> 1703 MHz behem ~100 s
       a coreAvgMs vyrostl 18.2 -> 24.6 => kolaps "ani ne po minute". Sedi s testem.
     * Telefon "na omak teply neni" - baterie jen 34.6 -> 35.3 C. CPU se throttluje
       podle teploty CIPU uvnitr, ne krytu. Proto to necitis, ale cip to dela.
     * Nox: silne PC jadro utahne i -O0 (coreAvgMs=8.4) => proto tam hudba drzi.
   - FIX: CMakeLists vynucuje -O3 pro vsech 5 core targetu + frontend,
     NEZAVISLE na debug/release. build.gradle navic dostal cFlags "-O3" jako pojistku.
   - Ocekavani: interpreterovy kod byva s -O3 nekolikanasobne rychlejsi nez s -O0.
     Realny odhad: coreAvgMs na S8 z ~17-18 nekam k 4-8 ms. To da rezervu i pro
     throttlovany takt. TOHLE JE HLAVNI OPRAVA S8 ZVUKU.

2) LEVY ODSKOK/PRUH V NOXU - CHYBEJICI VDP FUNKCE V JADRE
   - Screenshot dokazal, ze pruh je UVNITR herniho obrazu (jsou v nem utrzky levelu).
   - Ve zdrojaku jadra vdp.c radek 1065 stoji doslova:
     "'Blank 8 leftmost pixel columns' flag set but is currently unemulated."
   - Sonic 1 tenhle VDP flag (Mode Register 1, bit 5) pri hrani ZAPINA, aby schoval
     wrap horizontalniho scrollu v levem sloupci. Jadro ho ignorovalo => odpad videt.
   - Stage-percent fix z RX byl spravny (rect uz sedi presne na artwork: log
     NATIVE_RECT_STAGE_PERCENT, posun z 35 na 29 px), ale pruh mel jinou pricinu.
   - FIX: VENDOR PATCH vdp.c + vdp.h - flag je ted implementovany:
     kdyz ho hra zapne, levych 8 pixelu ukazuje barvu pozadi, presne jako real HW.
     ZADNY per-game hack - je to dokumentovana funkce Mega Drive VDP, kterou
     upstream jadro samo oznacuje jako TODO. Patch je oznacen NAP BUILD2RY VENDOR PATCH.

=========================================================
ZMENENE SOUBORY:
=========================================================
- app/build.gradle                                  (cFlags -O3, cppFlags -O3, versionName RY)
- app/src/main/cpp/CMakeLists.txt                   (NAP_CORE_OPT_FLAGS -O3 pro vsechny targety)
- app/src/main/cpp/vendor/clownmdemu-core/source/vdp.c  (VENDOR PATCH: left column blank)
- app/src/main/cpp/vendor/clownmdemu-core/source/vdp.h  (VENDOR PATCH: novy stavovy flag)
- app/src/main/cpp/nap_sega_native_proof.cpp        (jen RY labely; audit z RX zustava)
- app/src/main/java/eu/atarihelp/emu10/MainActivity.java (jen RY labely)
- app/src/main/assets/emu_sega/index.html           (jen RY label)

POZOR pro GitHub Desktop:
- build.gradle v tomto ZIPu vychazi z posledni zname verze (QB vetev). Pokud sis
  build.gradle od te doby menil rucne, prenes si jen blok externalNativeBuild
  s cFlags/cppFlags a novy versionName.

OVERENO U ME (bez lakovani, co jsem realne zkontroloval):
- Patchnuty vdp.c + zavisly clownmdemu.c zkompilovany gcc proti realnemu vendoru: OK.
- Cely CMake build core knihoven v Debug modu: OK, C_FLAGS obsahuji "-O3
  -fomit-frame-pointer" (overeno ve vygenerovanych flags.make).
- Frontend .cpp syntax check: OK. Plny Android NDK build probehne az u tebe.

=========================================================
TEST PLAN (po lopate):
=========================================================
1) NOX - LEVY PRUH:
   - Sonic, rozjet level. Pruh s odpadem vlevo MUSI zmizet (misto nej barva pozadi,
     v Green Hill modra obloha). Screenshot + SAVE LOG.
2) S8 - SONIC (hlavni test zvuku):
   - Hudba by NEMELA chrcet od zacatku. Nech hrat 5 minut.
   - V logu sleduj coreAvgMs v PASSIVE_AUDIT_RX radcich - cekam vyrazny pokles
     proti 17-18 ms z minula. SAVE LOG a posli i kdyz to bude hrat dobre -
     potrebuju to cislo videt.
3) S8 - SENNA: 5 minut, jestli vydrzi cistou hudbu i po zahrati. SAVE LOG.
4) NOX - prodleva cinknuti/vyskoku ~1 s: zmer po tomhle buildu znovu (jen dojem,
   nic merit nemusis). Rychlejsi jadro zmeni chovani FIFO; ladit latenci budeme
   az v RZ na zaklade novych cisel, ne naslepo.

LOG MARKERY:
- BUILD2RY_SEGA_NATIVE_CPP_ONLY_CORE_O3_VDP_LEFT_BLANK_STAGE141
- audio_mode=...CORE_O3_RY ... coreOptO3=YES vdpLeftBlank=IMPLEMENTED
- PASSIVE_AUDIT_RX tick=N ... coreAvgMs=...  (cekam znatelne mensi nez 17)
- NATIVE_RECT_STAGE_PERCENT ... leftStripFix=YES
- V logcatu uz se NESMI objevovat "Blank 8 leftmost pixel columns ... unemulated"

CO SE NESMI STAT:
- Modry monitor po ROM loadu, RT/RU markery.
- Kdyby -O3 build v Actions spadl (nemel by, ale priznavam, ze NDK build jsem
  nemohl spustit): posli mi vypis chyby z Actions, opravim obratem.

DALSI KROK PO TESTU (RZ):
- Kdyz S8 zvuk drzi: doladit latenci (mensi AudioTrack buffer / target FIFO),
  protoze rychlejsi jadro da rezervu - to vyresi i tu ~1 s prodlevu efektu.
- Kdyz S8 stale chrci: mame audit, uvidime nova cisla a pujdeme po dalsi pricine.
