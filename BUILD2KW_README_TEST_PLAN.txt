BUILD2KW_XEX_SPEED_GUARD_AND_GTIA9_DOC_COLOR_CORE

Navazuje na BUILD2KV.

Proc:
- KV vratil Donkey Arcade/Junior postavy na spravne misto.
- Junior a Donkey Title ale porad zustavaly rychle.
- Log ukazuje, ze nektere XEX po poslednim payloadu uz bezi hru, ale neprectou EOF/header ze SIO fronty, takze stary turbo guard neukoncil rychly rezim.
- Postcard/G2F porad ukazuje spatne pruhy/barvy; GTIA9 jas v KV bezel syrove pres bit0, BUILD2KW ho vraci podle GTIA dokumentace.

Zmeny:
1) XEX turbo stale-guard:
   - konci turbo podle neaktivity SIO od posledniho header/payloadu,
   - uz neceka na uplne prazdnou rxq/frontu,
   - zbytky loader handshake se po stopu zahodi,
   - cil: Donkey Junior a Donkey Title prestanou byt RYCHLE.

2) PMG Y faze:
   - zustava jako v KV / KT,
   - Donkey Arcade/Junior postavy by mely zustat u zebriků spravne.

3) GTIA9:
   - luma bit0 ignorovan podle dokumentace,
   - cíl: Postcard/G2F sediny/pruhy a obecne barvy, bez screen-paintu.

KODY JSOU STEJNE.

Test:
Donkey Arcade: clovicek/zebrik OK, zvuk, rychlost
Donkey Junior: opicka/zebrik OK, zvuk, rychlost OK/RYCHLA
Donkey Title: obrazek, zvuk, rychlost OK/RYCHLA
Decathlon: telo/nohy, napis nahore, rychlost
Postcard: okna barvy, pruhy
Night Driver: auto, pruh, zvuk

Log markery:
BUILD2KW_XEX_SPEED_GUARD_AND_GTIA9_DOC_COLOR_CORE
XEX FAST LOAD BUILD2KW: stale-guard ukoncil turbo loader
GTIA PMG VERTICAL PHASE BUILD2KW
GTIA COLOR CLOCK BUILD2KW
GTIA9 DOC LUMA BUILD2KW
