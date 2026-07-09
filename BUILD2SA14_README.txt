BUILD2SA14_STAGE202_YT_ORIGINAL_FIRST (zaklad = RC10; 1 zmeneny soubor)

YOUTUBE ZVUK - OPRAVA "CHCI ORIGINAL":
Nalez pitvy RC10 (fakta): EQ NENI atrapa (skutecne filtry), ALE:
a) audio graf se stavel HNED pri nacteni videa - YouTube uz nikdy nehral
   "cisty" original, i kdyz jsi EQ vubec nepouzil
b) deprecated ScriptProcessor byl primo ve SLYSITELNE ceste = zadrhavani
c) PCM pro TV se zeslabovalo na 0.86 + limiter = zabarveny signal
OPRAVA:
1) ORIGINAL FIRST: dokud NEOTEVRES EQ panel, YouTube hraje 100% nedotceny
   original (zadny graf, zadne filtry, nic). Graf se postavi az prvnim
   otevrenim EQ. Marker: result=controls_ready_ORIGINAL_SOUND_untouched
2) ScriptProcessor presunut MIMO slysitelnou cestu (paralelni ztlumeny
   odboc jen pro TV vzorky) = zadne zadrhavani ani se zapnutym EQ
3) PCM pro TV bez zeslabeni a bez limiteru = poctivy signal
RC10 obsah (player, ps1, vbxe stranky, manifest) zustava beze zmeny.

ATARI AUDIT - PRVNI NALEZ (poctivy stav):
V poslednim snapshotu co mam (30.6.) je assets/emu/index.html jen 607B
STUB a vedle nej lezi stare 290KB verze puvodniho JS emulatoru jako
zalohy. Co je v repu TED (po Atari vetvich z jinych chatu) odsud nevidim.
=> POSLI MI prosim aktualni app/src/main/assets/emu/ slozku (staci ZIP)
   + posledni Atari handoff. Pak udelam stejny tvrdy audit jako u Segy:
   pasivni log, ktery ukaze, CO doopravdy bezi (JS interpret? nativni
   core? nebo fake?). Cisla, ne dojmy - a kdyz tam fake je, vyriznem ho.
