# Co je v B282

Navazuje na tvoje hlášení po B281: rostoucí zpoždění zvuku, mizející
píšťavý podkres a tišší datový tok při CSAVE. Řeší **dvě samostatné věci**:

## 1. Paměť – proč lag s každým CSAVE rostl

Skutečná příčina nebyla v jádru emulace, ale v tom, jak appka ukládala
WAV nahrávku: **při každém CSAVE nahrávala vždy přesně 50 vteřin zvuku**,
bez ohledu na to, že samotný přenos trvá jen kolem 24 vteřin. Ten
přebytek (desítky vteřin zvuku navíc) zůstával v paměti při každém
spuštění – a to se postupně hromadilo, přesně jak jsi tušil.

Oprava: appka teď pozná **skutečný konec CSAVE** (motor kazetového
mechanismu se vypne) a nahrávání ukončí okamžitě. Pevných 50s zůstává
jen jako záložní pojistka, kdyby se něco pokazilo.

## 2. Zvuk – píšťavý podkres a hlasitost

Máš pravdu, že na originále běží pískavý tón dál i pod datovým tokem –
to je čtvrtý zvukový kanál POKEY, který SIO rutina v ROM při přenosu dat
nevypíná. V appce se ale během přenosu úplně umlčel, což zároveň
způsobovalo i to, že se zvuk zdál tišší (hrál jen jeden kanál místo
dvou najednou).

Oprava: podkres teď hraje nepřetržitě, přesně jako na originále.

## Jak jsem to ověřoval

Otestováno nanovo, end-to-end, přes skutečné BASIC a ROM (napsání
programu, CSAVE, potvrzení druhým RETURN – přesně jak to děláš ty) –
motor doběhl za 23,9s (odpovídá tomu, co jsme dřív naměřili), a ve
zvuku zachyceném přímo během přenosu je vidět FFT analýzou současně
podkres i datový tón. Regrese (oprava lagu z B281, obrazovka po CSAVE,
CLOAD bez kazety) beze změny.

Delší dobu bootování jsem zatím neřešil – zůstává na příště.
