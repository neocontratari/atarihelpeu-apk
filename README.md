# AtariHelp.eu EMU-09 FIX68 COBRA MANUAL + LONG VBI

FIX68 reaguje na test z 2026-06-01:

- PiTT-KiTT Remaster: oprava z FIX67 zustava, PiTT je referencni titul a neni sahnuto na jeho herni data.
- Super Cobra: odstraneny stary automaticky START+FIRE helper. CPU autostart smi spustit program, ale emulator uz nema sam mackat START/FIRE, preskakovat intro ani vyrabet samovolnou strelbu.
- Obecne XEX jadro: VBI rutina uz neni useknuta na 4500 ops. Donkey Kong potrebuje v intru zhruba 7300 mini-ops; stary limit nechal semafor napul a pozdni START pak nefungoval.
- Obecne XEX jadro: pridany Atari OS frame countdown casovace CDTMV1-5 ($0218-$0221). To pomaha intrum/menu smyckam, ktere cekaji na OS VBI timery misto jedne konkretni hry.
- Super Cobra DLIST oprava z FIX67 zustava: kratke validni DLISTy s vice LMS radky nejsou odmitane jako sum/fallback.
- CORE TEST TXT zustava automaticky ukladany do TXT souboru a po testu vraci puvodni stav emulatoru.

Co testovat jako prvni:

1. RESET + VESTAVENY PiTT-KiTT - pismo musi byt citelne, zadne bile bloky.
2. Super Cobra - nesmi sama strilet; START/FIRE jen po rucnim TAP START/TAP FIRE.
3. Donkey Kong - nech intro chvili bez STARTu, potom zkus START; OS timers by nemely nechat smycku zatuhnout.
4. CORE TEST TXT - musi ulozit TXT report automaticky.
5. Pitstop II / Pac-Man / dalsi vlastni XEX jako dalsi kompatibilitni testy.

GitHub Desktop summary:

FIX68 cobra manual long vbi
