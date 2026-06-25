AtariHelp.eu EMU-10 BUILD2KA_PLAIN_PROGRAM_VBXE_LOCK_MISSION_SAFE

NAVAZUJE NA:
- BUILD2JZ_ROLLBACK_TO_JX_SUCCESS_SAFE
- JZ vratil JX uspech: Commando Arnold ANO + hra ANO.

PROC TENTO BUILD:
- Log z Mission ukazal slaby IO-only nalez, tedy obycejny/nepotvrzeny VBXE titul.
- Po startu se presto nahodnymi D640/D740 zapisy znovu rozjel VBXE/XDL/MAP stav s base kolem $000C5 a mode OFF/MAP.
- To je pravdepodobna pricina nedokreslene Mission grafiky po oprave Commando.

ZMENA:
- Pridan trvaly plain-program VBXE lock pro hry bez potvrzene silne VBXE signatury nebo MEMAC payloadu.
- U takove hry cteni VBXE vraci $FF a zapisy D640/D740 se ignoruji.
- Commando a Night Driver maji silnou VBXE signaturu, proto zustavaji na JX/JZ render ceste.
- Nejde o hack podle nazvu hry.
- Zadny screen-paint.
- UI, joystick, kazeta, loader beze zmen.

KODY JSOU STEJNE.
BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou zmenene.

TEST PLAN:
1) Spust Commando.
   Ocekavani: Arnold ANO + hra ANO.
   Pokud ne: poslat LOG / CHYBA a dal netestovat.

2) Spust Mission.
   Ocekavani: grafika se proti JZ zlepsi / dokresli se vic.
   Poslat: Mission lepsi / stejna / horsi + LOG.

3) Spust Night Driver.
   Ocekavani: auto stav stejny jako JZ, uzky sum u auta zatim nemusi byt opraven.
   Poslat: auto ano/ne, sum stejny/mensi/vetsi + LOG.

4) Decathlon jen kontrola regrese.
   Ocekavani: minimalne ne horsi.

LOG MARKERY:
- AtariHelp.eu EMU-10 BUILD2KA_PLAIN_PROGRAM_VBXE_LOCK_MISSION_SAFE pripraven
- VBXE PROFILE ACTIVE BUILD2KA ... u potvrzene VBXE hry
- VBXE PLAIN LOCK BUILD2KA ... u Mission/Decathlon pokud nejsou potvrzene VBXE
- VBXE PLAIN LOCK WRITE BUILD2KA ... pokud obycejna hra zkusi sahnout do D640/D740
