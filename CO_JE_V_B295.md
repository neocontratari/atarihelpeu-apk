# Co je v B295

Tvá slova po B293: „všechno šlape, ale CLOAD nechápu a nebo má problémy –
VBXE zatím nejede, ale to chápu … ale jsi dál než Java, Decathlon ti jede :-)“

## CLOAD – co se stalo (z tvého logu)

Kazetu jsi vložil správně (`csave_2026-10-06_01-51-24.wav`, 265 bajtů ve
2 záznamech, 0 chyb). Pak už ale v logu není ani stisk PLAY, ani zapnutí
motoru – Atari zůstalo stát na pípnutí. Na skutečném Atari je postup:
**CLOAD, RETURN → pípne → zmáčkni PLAY na magnetofonu → zmáčkni klávesu**.
Ten druhý RETURN po pípnutí je ten nepochopitelný krok – Atari tím čeká,
až připravíš kazetu.

Teď je to jednodušší:

1. **EJECT** → vyber kazetu. Dvířka se zavřou a **PLAY se zmáčkne samo**
   (jako bys vložil kazetu a zmáčkl PLAY).
2. Napiš **CLOAD** a **RETURN** – Atari pípne.
3. Zmáčkni **ještě jednou RETURN** – rozběhne se motor, cívky se točí,
   za ~22 s je READY a LIST ukáže program.

Pod přístrojem je po vložení kazety nápověda „NAPIS CLOAD + RETURN, PO PIPNUTI
RETURN“, a když se rozběhne motor, „MOTOR BEZI – CTU PASKU“ (nebo „ZMACKNI
PLAY!“, kdybys PLAY mezitím pustil). Do logu se píše každé zapnutí/vypnutí
motoru – kolik bajtů z pásky přišlo a kde páska stojí (`B295 KAZETA motor …`).

## Disketa ATR (mechanika D1:)

Ve tvém logu byl pokus o `wolf3d.atr` – v C++ zatím mechanika nebyla. **Teď je:**
ATR/DISK (nebo ATR přes XEX/MOBIL, NET/HRY, i v ZIP) vloží disketu do D1:
a Atari z ní nabootuje (OPTION drženo = BASIC vypnutý, jako u her). Disketa
v mechanice zůstává i přes POWER vyp/zap. Mechanika je v C++ na úrovni příkazů
SIO (čtení/zápis/stav/formát sektoru).

`wolf3d.atr` teď nabootuje a napíše **WOLF3D NEEDS VBXE** – to je správně:
tahle verze Wolfensteinu potřebuje VBXE kartu, kterou běžné 130XE nemá
(stejně by to napsalo i skutečné 130XE bez VBXE).

## VBXE

VBXE je přídavná grafická karta (není v běžném 130XE). V C++ zatím není –
přijde jako samostatný krok.

## Ověřeno na počítači

Skutečný `nap_atari_native.cpp` se skutečnými vlákny (`test_b292_jni_host`):
15 kontrol, 0 chyb – mimo jiné: po vložení kazety je PLAY zmáčknuté samo,
CLOAD jen s RETURN po pípnutí nahraje program, ATR (Acid800) nabootuje z D1:
a po POWER vyp/zap znovu. `wolf3d.atr`: 274 sektorů přečteno, „WOLF3D NEEDS
VBXE“.
