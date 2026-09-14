# B278 — Dvoutónové kódování opravené podle skutečné nahrávky

## Díky za nahrávku a za to důrazné "přestaň odhadovat"

Měl jsi pravdu. Analyzoval jsem syrová data z tvého WAV souboru přímo
(autokorelace pro přesné frekvence) a zjistil jsem, že můj předchozí
přístup (B276, "půlení periody") byl **principiálně špatně**.

## Co ukázala analýza

Během pískání appka (na skutečné nahrávce) střídá mezi **594,0 Hz** a
**5278,0 Hz** — obrovský skok, ne malá odchylka. A tahle dvě čísla
přesně (na desetiny Hz) odpovídají:
- 594,0 Hz = spojený kanál 3+4 (přesně to, co appka už používala)
- 5278,0 Hz = **samostatný kanál 1** (úplně jiný, nezávislý oscilátor)

Během datového toku totéž, jen rychleji: 565/4009 Hz, bit po bitu.

## Skutečný mechanismus

Skutečný POKEY dvoutónový obvod **nemění frekvenci** jednoho kanálu —
**přepíná, který ze dvou nezávisle běžících oscilátorů** (kanál 1
samostatně vs. spojený kanál 3+4) je právě "připojený na výstup".
Oba oscilátory běží pořád, na svých vlastních frekvencích — mění se
jen hlasitost/výběr.

## Oprava

Appka teď pro každý bit vytvoří dočasnou kopii hlasitosti: bit=1
necháFrom znít jen kanál 1, bit=0 necháFrom znít jen spojený kanál
3+4. Frekvence (AUDF) zůstávají po celou dobu nezměněné.

## Ověření

Nový test přímo porovnává naměřené frekvence s tím, co ukázala tvá
nahrávka — obě sedí. Všech 9 předchozích testů prochází beze změny,
CLOAD nedotčený, obrazovka správně končí na READY.

## Zůstává otevřené

Zmínil jsi, že počáteční bootování teď trvá o něco déle — menší
problém, řekls, ale zapsal jsem si to. V tomhle kole jsem se tomu
nevěnoval, zůstává pro příště.
