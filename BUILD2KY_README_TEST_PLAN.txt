AtariHelp.eu EMU-10 BUILD2KY_XEX_RECORD_TURBO_RESTORE_SAFE

Co je opravene:
- Navazuje na BUILD2KX / KV.
- KX vypinal XEX turbo moc brzy: uz po prvnim payloadu, kdyz INITAD/RUN kod na chvili skocil do RAM. To zpomalilo dohravani dalsich XEX zaznamu, hlavne VBXE hry s mnoha zaznamy (Popeye, W3D, dalsi VBXE tituly).
- KY je record-aware: turbo drzi az do posledniho datoveho XEX zaznamu a normalni rychlost pusti az po EOF/stale guardu.
- Donkey Arcade/Junior PMG fyzicka faze z KV zustava.
- GTIA color-clock a HBLANK guard z KX zustavaji.
- Postcard/Decathlon/Night Driver grafiku timhle nelakuju jako hotovou.
- Bez screen-paintu. Bez hacku podle nazvu hry. KODY JSOU STEJNE.

V LOGu musi byt:
AtariHelp.eu EMU-10 BUILD2KY_XEX_RECORD_TURBO_RESTORE_SAFE pripraven
XEX FAST LOAD BUILD2KY: record-aware turbo drzi nahravani az do posledniho XEX datoveho zaznamu
XEX FAST LOAD BUILD2KY: EOF/stale guard ukoncil turbo az po poslednim datovem zaznamu
GTIA PMG VERTICAL PHASE BUILD2KX/KY marker muze zustat z KX vetve
GTIA/ANTIC HBLANK NEXTLINE BUILD2KX marker muze zustat z KX vetve

Test plan:
1) Popeye (VBXE, PAL Version).xex
   - cekam: nahravani vyrazne rychlejsi nez KX, hra nabehne; grafiku zatim jen zapsat OK/spatna.
2) W3D
   - cekam: nahravani kratsi nez 3-4 minuty. Napis realny cas priblizne.
3) Donkey Arcade
   - cekam: clovicek/zebrik OK, zvuk OK, rychlost hry OK, nahravani rychlejsi nez KX.
4) Donkey Junior
   - cekam: opicka/zebrik OK, zvuk OK, rychlost hry OK, nahravani rychlejsi nez KX.
5) Night Driver VBXE
   - cekam: auto ANO, zvuk ANO, pruh/duchove zatim jen pruh mensi/stejny/horsi, nahravani rychlejsi.
6) Decathlon
   - cekam: rychlost OK; telo/nohy a napis jen zaznam lepsi/stejne/horsi.
7) Postcard
   - cekam: okna barvy/pruhy jen zaznam lepsi/stejne/horsi.

Poslat zpet:
BUILD2KY TEST
Popeye VBXE: nahravani rychlejsi/stejne, hra ano/ne, grafika OK/spatna
W3D: cas nahrani cca ..., nabeh ano/ne, grafika OK/spatna
Donkey Arcade: nahravani ..., clovicek/zebrik ..., zvuk ..., rychlost ...
Donkey Junior: nahravani ..., opicka/zebrik ..., zvuk ..., rychlost ...
Night Driver: nahravani ..., auto ..., pruh ..., zvuk ...
Decathlon: telo/nohy ..., napis ..., rychlost ...
Postcard: okna barvy ..., pruhy ...
+ jeden spolecny LOG
