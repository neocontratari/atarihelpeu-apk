# B253 — Atari C++: boot klik doplněn, "turuturuturu" opraveno (versionCode 299)

Rene: "Základ je v pořádku - teď to běží rychle jako Atari 130XE.
Nicméně zvuk: na začátku při bootování přece Atari má svůj specifický
zvuk. Bootovací zvuk. V self-testu to máš lepší než v Java Atari i
zvuk test, ale do toho čistého zvuku - co tam správně je - máš nějaký
podzvuk turuturuturu. Jinak melodie je správně."

Dvě věci k opravě - chybějící zvuk a rušivý artefakt v jinak správném
zvuku.

===============================================================================
 1) CHYBĚJÍCÍ BOOT KLIK
===============================================================================

Kód už měl komentář PŘÍMO u zápisu do GTIA registru CONSOL ($D01F,
bit 3), který doslova říkal: "OS sem píše při KAŽDÉM SNÍMKU (pro
klapnutí reproduktoru)" - appka o tomhle mechanismu VĚDĚLA, ale zápis
se prostě zahazoval, nic se s ním nedělalo.

Doplněn skutečný mechanismus - přesný překlad z tvé JS reference
(`M.onSpeaker`/`spkLevel`/`spkDecay`): když se bit reproduktoru
změní, spustí se krátký (~4ms) napěťový skok. Přesně tohle je ten
charakteristický "klik", který Atari dělá při bootu (a self-testu, a
kdykoli jinde, kdy OS na tenhle registr sáhne).

===============================================================================
 2) "TURUTURUTURU"
===============================================================================

Příčina: appka plánovala KAŽDÝ jednotlivý obrazový snímek (50× za
vteřinu, 882 vzorků) jako SAMOSTATNÝ zvukový buffer ve Web Audio. 50
hranic mezi buffery za vteřinu spadá přímo do slyšitelného pásma -
zní to jako rychlé chvění/tremolo, i když SIGNÁL uvnitř byl celou
dobu správný (proto "melodie je správně" - to bylo přesné pozorování).

Tenhle problém dokonce narazil i na tvou vlastní JS referenci - má v
sobě komentář "WebView nestíhá 2048 -> chřestí" a používá bloky o
4096 vzorcích. Malé kousky (882 vzorků, správná velikost pro
synchronizaci s obrazem) se teď HROMADÍ, a teprve když jich je
nahromážděno dost (4096, stejná velikost jako tvá ověřená reference),
naplánují se jako JEDEN větší buffer. Hranic bufferu tak kleslo z
50 za vteřinu na přibližně 10.

===============================================================================
 OVĚŘENÍ
===============================================================================

Nový C++ test (2 kontroly): klik reproduktoru má správnou amplitudu
(0.25) hned na začátku, a dozní správně během ~4ms.

Nový JS test hromadění bufferu (4 kontroly): malé kousky samy o sobě
nenaplánují nic, až při dosažení 4096 vzorků se naplánuje přesně
JEDEN buffer správné velikosti, zbytek (přebytek nad 4096) se
neztratí, a při simulaci 1 vteřiny zvuku (50 snímků) vznikne
přibližně 10 bufferů místo 50.

Všech 8 izolovaných POKEY testů (6 původních + 2 nové) prošlo. Celý
`nap_atari_native.cpp` + `machine.h` + `pokey.h` znovu zkompilovány
čistě s JNI stub hlavičkou.

CO NEJDE OVĚŘIT ODSUD: jak to skutečně zní na tvém telefonu - to je
zase na tvém testu, přesně jak sis přál. Poslouchej znovu boot i
self-test.
