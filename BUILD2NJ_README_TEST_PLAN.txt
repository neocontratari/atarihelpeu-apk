BUILD2NJ_SEGA_SLOT_RESET_MINHOLD_GAMEPAD_STAGE23

CIL:
- opravena reakce naseho CARTRIDGE SLOT panelu i vlozene cartridge
- pridany RESET hitbox vpravo u cartridge/power panelu
- A/B/C maji delsi minimalni stisk + vice klavesovych variant + lepsi virtual gamepad
- nova ROM pres HRY/SBIRKA/CARTRIDGE/CARTRIDGE image restartuje wrapper a nacte novou hru

TEST:
1. Rozbal overlay pres koren projektu.
2. Otevri Sega modul.
3. Klikni HRY nebo SBIRKA nebo pravou plochu CARTRIDGE SLOT nebo vlozenou cartridge uprostred.
4. Vyber Sonic .gen.
5. Over, ze Sonic nabehne.
6. Zkus START, smer RIGHT, A/B/C skok.
7. Klikni pravou plochu CARTRIDGE SLOT nebo vlozenou cartridge a vyber jinou ROM.
8. Zkus RESET vpravo u cartridge/power panelu - ma restartovat aktualni ROM.
9. Otoč mobil/Nox na landscape a over obraz + ovladani.
10. ULOZENE posli LOG.
