# B249 — Atari C++ Fáze 1: POKEY zvuk (versionCode 295)

Rene: "budeme postupovat podle tvého plánu, necham to zatím vše na
tobě - já budu testovat, vždy si nachystej co a jak jako doposud."

Tohle je Fáze 1 z navrženého postupu (viz analýza po dokončení Fáze 0):
POKEY zvuk, jen tolik, aby self-test v ROM produkoval zvuk.

===============================================================================
 CO PŘIBYLO
===============================================================================

**Nový soubor `nap_atari_pokey.h`** — POKEY zvukový generátor. Je to
**přesný překlad** z Reneho fungující JS reference
(`emu_vbxe/index.html`, `startAudio()`/`onaudioprocess`) - stejné
polynomiální generátory šumu (poly4/5/9/17 - lineární zpětnovazební
posuvné registry), stejné bity AUDCTL (spojení kanálů 1+2 a 3+4,
rychlý hodinový signál 1,79 MHz, 15kHz/64kHz základní takt), stejný
vzorec periody kanálu, stejný DC-blocker na konci. Žádná vlastní
teorie - jen překlad ověřeného kódu do C++.

Jeden rozdíl od reference: JS generuje vzorky ASYNCHRONNĚ (samostatný
zvukový callback), a proto potřebuje frontu událostí pro vzorkově
přesné časování změn registrů uprostřed bufferu. Tenhle C++ kód
generuje zvuk SYNCHRONNĚ (stejný vzor, jaký už má obraz přes
`runFrame()`) - fronta zatím není potřeba, protože pro Fázi 1
(self-test, kde se registry během jednoho čtení typicky nemění)
stačí číst aktuální stav. Až přijde na řadu hudba/efekty ve hrách,
možná bude potřeba frontu doplnit - poznamenáno přímo v komentáři
kódu, ať to nástupce nepřekvapí.

**`nap_atari_machine.h`** — přidány registry `audf[4]`, `audc[4]`,
`audctl`, zápis do `$D200-$D208` (přesně stejné rozložení jako
reference), STIMER (`$D209`) vynuluje čítadla kanálů (na skutečném
POKEY dělá totéž).

**`nap_atari_native.cpp`** — nová funkce `audioNative()`: vygeneruje
zadaný počet snímků zvuku (44100 Hz, mono, 16bit) z aktuálního stavu
registrů, vrátí jako base64 PCM16 + statistiku (špička, RMS,
aktuální AUDF/AUDC/AUDCTL) v JSON.

**Java strana** — `audioSafe()` bezpečný wrapper (stejný vzor jako
`screenSafe()`), `atariAudio()` bridge metoda s logováním (jen
statistika, ne velký base64 náklad, ať log zůstane čitelný).

**JS testovací stránka** — nové tlačítko "🔊 PŘEHRÁT ZVUK" - přečte
skutečně vygenerovaný zvuk z jádra a přehraje ho přímo v prohlížeči
přes Web Audio API. Žádné předstírání - je to přesně to, co by hrálo
reproduktorem.

===============================================================================
 OVĚŘENÍ - SKUTEČNÁ KOMPILACE A BĚH, NE JEN "MĚLO BY FUNGOVAT"
===============================================================================

Izolovaný test (`test_atari_pokey.cpp`) zkompilován a spuštěn proti
SKUTEČNÉMU `Machine` (ne mock), pět kontrol:

  1. ticho při hlasitosti 0 (max vzorek = 0.000000)
  2. čistý tón na daném AUDF vyšel na 0,4 % přesně podle vzorce
     skutečného POKEY (775,2 Hz naměřeno vs. 772,4 Hz očekáváno)
  3. žádné NaN/Inf v bufferu
  4. STIMER správně vynuluje čítadla kanálů
  5. volume-only (digi) režim generuje nenulový výstup, který DC-blocker
     postupně utlumí k nule (přesně jak filtr tohoto typu má)

Celý `nap_atari_native.cpp` (včetně nové `audioNative()`) zkompilován
čistě s minimální JNI stub hlavičkou (žádné chyby, žádná varování) -
odhalilo by to jakoukoli syntaktickou/typovou chybu v JNI obalu, i
když se JNI hlavičky liší od skutečného Androidu.

node --check (Atari C++ testovací stránka), žádná duplicitní id (36).
`stranka_kontrola.py` (TV cesta) - 0 chyb. Rovnováha `{ }` v
MainActivity.java ověřena chytrým kontrolorem (ignoruje řetězce a
komentáře, kde běžně vznikají falešné poplachy) - 0 na konci, správně.

Existující Sega i PS1 jsdom simulace spuštěny znovu po zásahu do
sdíleného `MainActivity.java` - beze změny prošly.

===============================================================================
 CO NEJDE OVĚŘIT ODSUD
===============================================================================

Skutečné přehrání na telefonu (zvuk z self-testu skrz repro) - to je
na tvém testu. Log teď ukazuje `spicka`/`rms` i aktuální
`audf`/`audc`/`audctl` při každém stisku tlačítka ZVUK, takže i kdyby
zvuk neseděl, bude z logu vidět přesně, jaké hodnoty registrů self-test
v tu chvíli nastavil - konkrétní data k diagnostice, ne dohady.
