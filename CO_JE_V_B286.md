# Co je v B286

Navazuje na B285 a tvůj nový log. Tvá slova: "Sakra lepší, synchron je
lepší, ale furt to není 1ku1... myslím, že jsme hodně blízko, cca 95
procent, ale furt to není to pravé. Vím, že jsem hnidopich, ale v C++
chci atari stoprocent."

Nejdřív dobrá zpráva potvrzená přímo tvým logem: **zahlcování je
pryč.** Nový bezpečnostní mechanismus z B285 (`ZVUK_PREDSTIH_OREZAN`)
se v logu spustil přesně 3×, přesně při vstupu do self-testu – přesně
jak měl, a žádný růst do nekonečna.

## Co jsem našel dál

V logu dál rostlo jiné počítadlo – `ZVUK_PODTEKANI` (výpadek/díra ve
zvuku), nezávisle na tom novém. Rostlo hlavně těsně po vstupu do
self-testu a v menší míře kolem každého uložení CSAVE do .wav souboru.
Dva konkrétní, změřené nálezy:

**1) Ukládání CSAVE do .wav blokovalo přehrávání.** Přesně v okamžiku,
kdy motor kazety "dohraje" (CSAVE vizuálně skončí), appka vzala celý
nahraný zvuk (~2,5 MB – přesně tvoje velikost z logu) a zakódovala ho
do base64 jedním souvislým během, bez přestávky. Změřil jsem to (kopie
přesně stejného algoritmu): ~20–25 ms na rychlém stolním počítači – na
reálném telefonu to může být klidně víc. Po tu dobu "vynechávalo"
generování zvuku, přesně v okamžiku, kdy bys normálně pokračoval dál
(třeba do self-testu).

Oprava: stejný výsledný soubor (ověřeno bajt po bajtu, žádná změna
výstupu), ale kódování teď běží po malých dávkách rozložených přes
více "tiků" prohlížeče místo jednoho velkého bloku.

**2) Samotné ořezání fronty (z B285) nemělo žádnou rezervu.** Ořezání
nastavilo frontu přesně na "teď" – ale než se naplánuje další kousek
zvuku, uplyne malá chvilka na vlastní práci ořezání, a appka to pak
mylně započítala jako výpadek. Přidal jsem 50ms rezervu – hluboko pod
hranicí slyšitelnosti, ale dost na to, aby to pohltila.

## Co jsem ověřil o C++ jádru (podruhé, jinak)

Chápu, že chceš jistotu o C++, ne jen moje ujištění. Takže tentokrát
jiné měření, ne recyklace toho předchozího: změřil jsem čas **každého
jednotlivého snímku zvlášť** – v klidu, v CSAVE LEADER fázi a v CSAVE
DATA fázi (reálný dvouton, všechny kanály aktivní) – místo jen
"předtím vs. potom" jako v B285.

Výsledek: prakticky stejné (rozdíl pod 5 %), nejhorší jednotlivý
snímek 11,35 ms z 20 ms rozpočtu na snímek (PAL, 50 snímků/s) – tedy i
nejhorší případ má přes polovinu času navíc v rezervě. Jádro tedy
**není pomalejší ani po CSAVE (B285), ani přímo během přenosu
(B286)** – dvě nezávislá měření, stejný závěr. Do jádra jsem se v
tomhle buildu vůbec nedotkl.

## Co ještě neví jistě

Oprava (1) je změřená stejně tvrdě jako dřívější nálezy. Oprava (2) je
zdůvodněná logicky (přetečení/podtečení uvnitř běžícího prohlížeče),
ale ne změřená se stejnou jistotou – CLI testy na to nedosáhnou,
potřebuju tvůj telefon. Pokud "furt to není 1ku1" zůstane i po
tomhle, bude to dobré znamení, že to NENÍ C++ (dvakrát změřeno a
čisté), a budu dál hledat v JS/Java vrstvě – řekni mi prosím, jestli
se gap zmenšil, zůstal stejný, nebo se změnil charakter problému
(zahlcování vs. drobné zaškobrtnutí).
