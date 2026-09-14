# B279 — Mark/space podle oficiální De Re Atari specifikace

## Díky za popostrčení

Měl jsi pravdu — prohledal jsem internet místo dalšího odhadování a
našel jsem **oficiální, definitivní specifikaci** přímo v De Re Atari
(dodatek C, oficiální technická dokumentace Atari):

> POKEY zpracovává každý bajt: 1 start bit (space), 8 datových bitů
> (0=space, 1=mark), 1 stop bit (mark). **Mark = 5327 Hz, Space =
> 3995 Hz.**

## Co to znamenalo pro appku

Přepočítal jsem to přesně: `audf=5` dává přesně 5327 Hz (mark),
`audf=7` dává přesně 3995 Hz (space). A appka měla **`AUDF=[5,7,...]`
zalogované celou dobu** — jen jsem si předtím nevšiml, že jde o
**dva samostatné kanály (1 a 2)**, ne kanál 1 vs. spojený kanál 3+4,
jak jsem usoudil z jedné konkrétní nahrávky minule. To byl omyl.

## Další chyba nalezená při opravování

Když appka jen přepínala hlasitost mezi kanály na jednom sdíleném
stavu, narazila na skrytou past — když se hlasitost vypne, appka
přestane počítat i fázi kanálu, takže po zapnutí pokračuje odjinud,
než má. Opravil jsem to tak, že oba kanály běží NEZÁVISLE a POŘÁD,
appka jen vybírá, který výstup použít.

## Ověření na tvém návrhu — skutečný program

Napsal jsem si (jak jsi radil) krátký program a podíval se, co appka
skutečně posílá. Výsledek:

```
55 55 FA 00 ... 48 45 4C 4C ...
```

- `55 55` = přesně ty dvě značkovací značky, co popisuje specifikace
- `FA` = řídicí bajt, přesně podle specifikace
- `48 45 4C 4C` = ASCII "HELL" — kus textu "HELLO" z mého programu!

Appka teď opravdu kóduje **skutečná data programu**, ne jen nuly —
49 různých hodnot bajtů v celém přenosu.

## Ověření

Frekvence obou kanálů teď sedí s oficiální specifikací. Všech 9
předchozích testů prochází, CLOAD nedotčený, obrazovka správně končí
READY.

## Zůstává otevřené — poctivě

Zmínil jsi, že "CSAVE se neukládá" — appka teď zvuk **správně kóduje**,
ale nemá žádnou virtuální kazetu, kam by se to uložilo pro pozdější
CLOAD zpátky v appce. To je samostatná, větší funkce, ne oprava zvuku
— zůstává pro příště. Delší bootování taky zůstává neprozkoumané, jak
jsi řekl, že to teď není priorita.
