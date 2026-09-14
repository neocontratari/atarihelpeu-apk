# B280 — Oprava ukládání WAV (base64 chyba)

## Měl jsi pravdu

Automatické ukládání WAV při napsání CSAVE (z B273) v kódu **pořád
existuje** — jen jsem si na něj v minulém kole nevzpomněl. Omlouvám
se za to zapomnění vlastní starší práce.

## Skutečná příčina "CSAVE se neukládá"

V logu se opakovaně objevovalo `chyba=base64:bad base-64`. Přímo jsem
to ověřil (ne odhadem): kód dělil nahrávaná data na kusy po 8192
bajtech před zakódováním do base64 — ale **8192 není dělitelné
třemi**. Base64 kóduje přesně po skupinách 3 bajtů → 4 znaky. Když je
velikost kusu jiná, každý kus dostane vlastní vycpávkové znaky `=` na
konci — a když se takové kusy spojí dohromady, vycpávka skončí
UPROSTŘED výsledného řetězce. To je neplatný base64, přesně to
způsobovalo hlášenou chybu.

## Oprava

Změnil jsem velikost kusu na 8190 (= 2730×3, dělitelné třemi).
Přímo jsem to ověřil v Pythonu na testovacích datech — původní kód
dává neplatný base64 s vycpávkou uprostřed, opravený kód dává platný
base64, správně dekódovatelný zpátky na přesně původní data.

## Rozsah změny

Jde o jednořádkovou opravu čistě v JS — C++ jádro vůbec nedotčené.
Všech 9 předchozích testů prochází beze změny.
