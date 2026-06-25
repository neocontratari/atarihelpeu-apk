BUILD2KD_RESULTS_PASS_NIGHTDRIVER_DECATHLON_CORE

Partaku, tohle neni dalsi mikrokolecko KC. KC vetvi neverim a neberu ji jako zaklad.
Zaklad je KB/JZ/JX: Commando Arnold ANO + hra ANO, Mission zpet funkcni.

ZMENA:
1) Night Driver: SR low-address origin fix. Platny VBXE SR record ve spodni casti obrazu uz nebere prvni radky z nizke VRAM/XDL oblasti jako grafiku. Cil: odstranit/omezit uzky caj u auta, auto zachovat.
2) Decathlon: PMBASE=$00 DMA zero-page fix. Kdyz P/M DMA bezi s PMBASE=$00, emulator nepouzije zero-page/OS pracovni RAM jako player/missile bitmapu a neprebije software GRAFP/GRAFM multiplex. Cil: min PM/vrstvovy bordel bez hacku podle nazvu hry.
3) Commando/JX a Mission/KB cesty jsou zachovane.

KODY JSOU STEJNE.
UI / kazeta / joystick / loader beze zmen.

MINIMALNI TEST, zadne kolecko kolem dokola:
- Night Driver: auto ano/ne, uzky caj pryc/mensi/stejny.
- Decathlon: viditelna zmena ano/ne, regrese ano/ne.
- Commando jen kdyby se ti chtelo potvrdit checkpoint: Arnold ano/ne.

Kdyz Night Driver a Decathlon nebudou realne lepsi, dalsi krok neni dalsi kosmeticky guard, ale audit ANTIC/GTIA display-list a pravdepodobne predavaci/rollback rozhodnuti.
