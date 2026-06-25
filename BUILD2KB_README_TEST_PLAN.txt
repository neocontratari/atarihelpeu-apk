AtariHelp.eu EMU-10 BUILD2KB_PLAIN_RENDER_LOCK_NO_IO_BREAK_SAFE

ZAKLAD:
- Navazuje na BUILD2JZ/JX uspech: Commando Arnold ANO + hra ANO.
- BUILD2KA byla moc tvrda: u Mission jako weak-IO/plain hry blokovala D640/D740 IO, a po intru uz hra nenabehla.
- BUILD2KB proto vraci IO chovani na JZ-kompatibilni cestu a zamyka jen VBXE XDL/render overlay u nepotvrzenych/weak-IO titulu.

CO JE ZMENENE:
- Potvrzene VBXE tituly (Commando/Night Driver) zustavaji na JX/JZ ceste.
- Weak/plain tituly (Mission/Decathlon) uz nemaji tvrde ignorovane D640/D740 cteni/zapisy.
- U weak/plain titulu se porad zahazuje pending XDL na VSYNC, aby nahodny D640 sum nepustil VBXE overlay pres ANTIC obraz.
- Zadny screen-paint, zadny vlozeny obrazek Arnolda, zadny hack podle nazvu hry.

KODY JSOU STEJNE:
- BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

TEST PLAN:
1) Commando
   - Ocekavani: Arnold ANO + hra ANO.
   - Kdyz Arnold zmizi, STOP a poslat LOG.

2) Mission
   - Ocekavani: po intru hra znovu nabehne aspon jako v JZ, idealne s lepsim dokreslenim grafiky.
   - Napis: Mission nabehne ano/ne, grafika lepsi/stejna/horsi.
   - Poslat LOG.

3) Night Driver
   - Ocekavani: stejny jako JZ/JX; auto porad pravdepodobne NE, uzky sum/duchove u auta sledovat.
   - Napis: auto ano/ne, sum mensi/stejny/vetsi.

4) Decathlon
   - Ocekavani: bez regrese; oprava Decathlonu neni cil tohoto buildu.
   - Napis: zmena ano/ne, regrese ano/ne.

LOG MARKERY:
- AtariHelp.eu EMU-10 BUILD2KB_PLAIN_RENDER_LOCK_NO_IO_BREAK_SAFE pripraven
- VBXE PLAIN RENDER LOCK BUILD2KB
- VBXE PLAIN RENDER LOCK READ BUILD2KB
- VBXE PLAIN RENDER LOCK WRITE BUILD2KB
- VBXE PLAIN RENDER LOCK VSYNC BUILD2KB
- VBXE PROFILE ACTIVE BUILD2KB

POZNAMKA:
BUILD2KA je slepa vetev pro Mission, protoze prerusila IO. BUILD2KB neni dalsi pokus o Decathlon/Night Driver, ale oprava regrese Mission po zachovani Commando uspechu.
