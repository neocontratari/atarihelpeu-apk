BUILD2MZ_SEGA_CONSOLE_TOGGLE_ROMFLOW_STAGE13

TEST PLAN:
1) Rozbal ZIP pres koren projektu.
2) Spust APK/Nox.
3) Otevri Sega modul.
4) Kratce klikni KONZOLE: ma se viditelne zmenit spodni cast na konzolovou obrazovku.
5) Kratce klikni JOYSTICK: ma se vratit joystick obrazovka.
6) Klikni HRY / SBIRKA / CARTRIDGE a vyber Sonic .gen.
7) Klikni ULOZENE a posli TXT log.

CEKANY LOG:
- pokud jen ulozis log bez ROM, uvidis TEST STAV: ROM jeste nebyla vybrana.
- po vyberu ROM musi log obsahovat SEGA ROM HEADER BUILD2MZ a pak vysledek core startu.

KODY JSOU ZMENENE.
