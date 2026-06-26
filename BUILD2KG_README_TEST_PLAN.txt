AtariHelp.eu EMU-10 BUILD2KG_GTIA_MODE_PRIORITY_PMG_COLOR_MASK_PASS

KODY JSOU STEJNE.
BASIC / Turbo BASIC / Altirra porovnavaci kody nejsou menene.

ZAKLAD:
- Navazuje na KF/KE/KB/JZ/JX.
- Nevraci KC/KD mikrovetve.
- KD PMBASE=$00 zasah je pryc.
- Commando Arnold ANO + hra ANO musi zustat chranene.
- Mission KB navrat musi zustat chraneny.

CO JE OPRAVENO / PROC:
- Postcard_Atari_Rocky ukazal, ze problem neni jen Night Driver a neni to ciste VBXE.
- Log rika, ze Postcard nema silnou VBXE signaturu a bezi jako bezny Atari/ANTIC titul.
- Soucasne log rika, ze hra kresli pres P/M DMA a software GTIA/PMG multiplex.
- Renderer mel u GTIA 9/10/11 pixelu PF priority/collision tridu = 0.
- To znamena, ze P/M barevne vrstvy mohly kreslit pres GTIA obraz i tam, kde je ma PRIOR schovat.
- BUILD2KG dava GTIA 9/10/11 pixelum realnou PF tridu pro P/M PRIOR/collision masku.
- Cilem je omezit fialove/zelene pruhy a spatne PMG prekryvy v G2F/Postcard obrazech bez hacku podle nazvu hry.

CO NENI LAKOVANO:
- Night Driver caj u auta neni prohlasen za opraveny.
- Decathlon neni prohlasen za opraveny.
- Zvuk Night Driveru neni prohlasen za opraveny.

MINIMALNI TEST, ABY NEBYLO KOLECKO:
1) Spust Postcard_Atari_Rocky.xex z test_assets nebo svuj stejny soubor.
   Ocekavani: mene fialovych/zelenych svislych pruhu, PMG barvy vice drzi s obrazem.
   Kdyz neni viditelny posun, stop a posli jen screenshot + LOG.

2) Kdyz Postcard posun viditelny je, teprve potom kratce zkontroluj:
   - Night Driver: caj u auta stejny/mensi/vetsi, zvuk ano/ne.
   - Commando: Arnold ANO + hra ANO.
   - Mission: nabehne ano/ne, grafika stejna/lepsi/horsi.

LOG MARKER:
- AtariHelp.eu EMU-10 BUILD2KG_GTIA_MODE_PRIORITY_PMG_COLOR_MASK_PASS pripraven
- GTIA MODE PFCLASS BUILD2KG
