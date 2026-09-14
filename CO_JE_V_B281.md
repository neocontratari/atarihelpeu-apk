# B281 — Zvuk po CSAVE se už nezpožďuje

## Co jsem našel v logu

Přesný vzor: podtékání zvukového bufferu zůstávalo stabilní kolem 9
**během** CSAVE, ale **po** jeho dokončení explodovalo — 10 → 123 →
323 → 364, postupně přes několik minut.

## Skutečná příčina

Přímo jsem ověřil: registr SKCTL (bit pro dvoutónové kódování)
zůstává zapnutý **navždy** po CSAVE — i 30 vteřin po dokončení, bez
jakékoli další aktivity, je pořád zapnutý. To je nejspíš autentické
chování originálního OS (neuklízí po sobě), ne chyba mé emulace.

Problém byl v tom, že appka kontrolovala JEN "je tenhle bit
zapnutý?" a pokud ano, vždy použila dražší, dvoukanálový výpočet
zvuku — i když už žádná data dávno neprobíhala. To appku trvale
zatěžovalo navíc po každém CSAVE, což vedlo k narůstajícímu
zpožďování zvuku přesně, jak jsi pozoroval.

## Oprava

Dražší výpočet se teď použije jen když je SOUČASNĚ zapnutý ten bit
A zároveň právě probíhá opravdu nedávný přenos (poslední bajt byl
odeslán před méně než vteřinou). Jinak — i když bit zůstává zapnutý
navždy — appka použije levný, běžný výpočet.

## Ověření

Přímo změřeno: 280 "drahých" volání během 24 vteřin CSAVE (správně),
ale přesně 0 nových drahých volání v následujících 30 vteřinách po
CSAVE (správně). Všech 9 předchozích testů prochází, dvoutónové
frekvence i CLOAD zůstávají nedotčené, obrazovka správně končí
READY.
