# Co je v B284

Navazuje na tvůj poslech B283 a přiložený log: "v self testu ti ujíždí
grafika od zvuku... možná furt lagy??" a pak "při delším testu se
opravdu začne emu C++ atari lagovat a zahlcovat se... je tam prostě
něco furt špatně."

## Příčina (nalezená přímo v kódu, ne odhadnutá)

V tvém logu bylo vidět přesně, co se děje: počítadlo výpadků zvuku drží
na 13 celou dobu nečinnosti, ale hned jak zmáčkneš SELECT+START (vstup
do self-testu), vyskočí **na 178 – 165 nových výpadků najednou**.

Důvod: tlačítka SELECT/START/OPTION a HELP běžela 42 snímků (0,84s)
úplně potichu – appka je sice emulovala správně, ale zvuk z nich se
nikam neposlal, prostě zmizel. To je přesně ta stejná nemoc, kterou už
dávno vyřešily opravy psaní textu a bootování (ten zvuk "dohánějící" 5
–15s pozdě) – jen tahle konkrétní tlačítka ten starší fix nikdy
nedostala.

## Oprava

Stejný, už osvědčený postup jako jinde v appce: zvuk se teď zachytává
po celou dobu, co je tlačítko drženo, místo aby se ztratil. Nic jiného
se nezměnilo – stav konzole (SELECT/START/OPTION) se pouští úplně
stejně jako dřív, jen s sebou teď nese i zvuk.

## Co to (zatím jistě) řeší a co ještě neví

Tohle řeší přesně ten jev, co je vidět v tvém logu – ostrý nápor
výpadků přesně v okamžiku stisku tlačítka. Pokud se "zahlcování" dělo
hlavně kvůli opakovanému mačkání tlačítek během procházení self-testu
(a to je dost pravděpodobné), mělo by to zmizet úplně.

Co ověřit: pusť se do self-testu znovu a nech ho běžet **déle** jako
předtím – pokud se appka pořád zpomaluje i bez dalšího mačkání
tlačítek, je v tom ještě něco jiného a budu potřebovat nový log z
takhle delšího běhu, abych to dohledal.
