BUILD2PW vendor slot

Sem patri skutecny nativni Sega Mega Drive / Genesis C/C++ core source.
Dokud zde neni realny core a CMake neni zapnuty pres NAP_SEGA_VENDOR_CORE_PRESENT=1,
appka nesmi fakeovat Sonic/Aladdin gameplay.

Soucasny stav BUILD2PW:
- normalni Sega UI -> Java -> JNI -> C++ funguje
- ROM picker -> bytes -> C++ parser funguje
- input -> C++ funguje
- audio test -> Android AudioTrack funguje
- render in-place v monitoru funguje
- real core vendor missing -> hlaseno CORE_VENDOR_MISSING

Dalsi realny krok:
- dodat source ZIP realneho native core
- pridat ho sem do vendor slozky
- upravit CMakeLists.txt
- napojit loadRom / stepFrame / renderFrame / audioPull misto patternu
