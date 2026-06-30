BUILD2QT TEST PLAN

Aplikuj pres BUILD2QS.

1) Spust aplikaci.
2) Otevri SEGA MEGA DRIVE.
3) Bez vyberu ROM zmackni SBIRKA nebo CARTRIDGE.
   Ocekavani: musi se otevrit vyber ROM. Nesmí to jen bliknout.
4) Vyber Sonic The Hedgehog (USA, Europe).gen.
   Ocekavani: ROM jde rovnou do C++ native core, zadna Java/WebView Sega cesta.
5) Hraj jen kratce, zkontroluj zda reaguje START/B/DPAD.
6) Zmackni ULOZENE.
   Ocekavani: log se ulozi.
7) Zmackni MENU.
   Ocekavani: Sega C++ se zastavi a appka odejde ze Sega bez zustavajiciho zvuku/obrazu.

Hledej v logu:
- BUILD2QT_SEGA_NATIVE_CPP_ONLY_BUTTON_FIX_QP_AUDIO_STAGE110
- BUILD2QT BUTTON_BIND_OK id=btnCollection key=romPicker
- BUILD2QT BUTTON_BIND_OK id=cartSlot key=romPicker
- BUILD2QT ROM_INPUT_CLICK direct OK
- BUILD2QT ROM SELECTED name=Sonic...
- BUILD2QT NATIVE_ROM_TO_CPP

Dulezite:
Pokud picker ROM neotevre, poslat log i screenshot. Pokud picker otevre, pak teprve pokracovat Sonic audio/sync testem.
