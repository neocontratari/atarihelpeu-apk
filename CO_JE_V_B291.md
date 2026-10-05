# Co je v B291

Tvá slova po B290: „Graficky ti to možná sedí – ale tlačítka se nezamačkávají,
nemáš kazeťák – nemáš nic!!! … to převeď v helpu přesně tak včetně kazeťáku a
obrazovky – podle toho návrhu, na kterém jsme se dohodli.“

## Proč se v B290 tlačítka nezamačkávala (zjištěno v kódu, ne odhadem)

1. Každý dotek posílal do JavaScriptu celý obrázek klávesnice (~3,6 MB textu
   base64) – dvakrát (prst dolů i nahoru), synchronně. Prohlížeč nestihl
   „zmáčknutý“ obrázek mezi tím vůbec vykreslit.
2. C++ funkce pro dotek končila hned na začátku, dokud nebylo stisknuto
   „NABOOTOVAT OS“ – do té doby dotek nedělal vůbec nic.

## Co je teď v HELP

Celý přístroj ze schváleného návrhu (Main.dc.html), kreslený **čistě v C++
přímo na displej** – žádný JavaScript, žádný PNG obrázek, žádná HTML tlačítka:

- N&P logo (skutečné logo z Java/JS emu), rámeček obrazovky a v něm **živý
  obraz Atari** (s CRT linkami a stínem skla), POWER vypínač, 3D nápis
  ATARI 130XE, barevné pruhy, konzolová lišta HELP/START/SELECT/OPTION/RESET
  (překrytí tlačítek ponecháno, jak sis přál), deska klávesnice a 57 kláves.
- Kazeťák: tělo, okénko s nápisem AUTOMATIC STOP, cívky (točí se při PLAY i
  když běží motor při CSAVE), páska, kladka, dvířka (EJECT je otevře, ťuknutí
  na okénko je zavře), počítadlo 000.0 + nulovací spínač, mřížky reproduktoru,
  tlačítka REC / REW / PLAY / FWD / STOP / EJECT.
- Servisní panel s 8 tlačítky a LED, duhový pruh, stavový řádek pod přístrojem.

Každá klávesa a každé tlačítko se viditelně zamáčkne (i krátké ťuknutí je
vidět aspoň 110 ms). Víc prstů najednou funguje.

## Co dělají ovládací prvky

- **POWER**: vypnout = přístroj ztmavne, obrazovka zhasne, nic jiného
  nereaguje. Zapnout = studený start Atari (paměť i hardware od nuly).
- **RESET**: jako skutečné Atari (program v paměti zůstane).
- **HELP/START/SELECT/OPTION**: drží se po dobu stisku; i krátké ťuknutí
  drží stroj aspoň 5 snímků, aby ho program nepropásl.
- **SHIFT, CONTROL**: ťuknutí = zamčeno (zlatá), další ťuknutí = odemčeno.
- **Kazeťák**: REC a PLAY drží, STOP pustí, EJECT otevře dvířka, REW/FWD
  přetáčí počítadlo. **CSAVE**: napíšeš CSAVE, RETURN, po pípnutí RETURN –
  nahrává se zvuk z jádra, jen když běží motor kazety, a po skončení se sám
  uloží do `Download/AtariHelp/Atari_emu/csave_vystup.wav`. Pod přístrojem
  se ukáže „CSAVE ULOZENO“.
- **XEX/MOBIL**: výběr XEX nebo ZIP z telefonu a spuštění v C++.
- **TURBO/BASIC**: spustí Turbo-BASIC XL 1.5 (stejný soubor jako v Java emu).
- **NET/HRY**: seznam her z atarihelp.eu – vybraná hra se spustí v C++ HELP
  (starý ATARI 130XE EMULATOR se tím nijak nemění).
- **BASIC/TBXL TXT**: okno pro vložení výpisu programu; píše se klávesnicí
  Atari, klávesa po klávese (žádný zápis do paměti „bokem“).
- **LOG/CHYBA**: ukáže log a testy (zpět tlačítkem ZPĚT NA ATARI 130XE nebo
  šipkou zpět). **HELP**: nápověda. **MENU**: zpět do nabídky.

## Co jsem ověřil (na počítači, ne odhadem)

- Kreslení přístroje jsem porovnal s obrázkem, který z PŘESNĚ stejného CSS
  schváleného návrhu vykreslil prohlížeč Chromium (velikost jako na tvém
  telefonu). Texty sedí na 0–1 px, tvary, barvy a stíny odpovídají.
- Přeložil a spustil jsem **skutečný** `nap_atari_native.cpp` (stejný soubor
  jako v appce) se skutečnými vlákny emulace a kreslení; Android byl jen
  nahrazený (okno displeje v paměti). Test „sahal prstem“ na přístroj jako
  Java: napsal PRINT 2+2 a Atari odpovědělo 4; SHIFT zůstal zamčený a SHIFT+1
  napsal !; krátké ťuknutí na START program zachytil; PLAY/STOP/EJECT/dvířka/
  nulování; POWER vypnul a zapnul; Turbo-BASIC XL běží; CSAVE dotyky na
  klávesnici vytvořil 24 s WAV (264 bajtů dat). **24 + 3 kontrol, 0 chyb.**
- Psaní textu: BASIC program včetně číslic, uvozovek a zdvojených písmen
  (LL, 000) se zapsal přesně (ověřeno výpisem LIST z paměti Atari).
- XEX zavaděč: Turbo-BASIC XL, Activision Decathlon a Mission se spustily.
- Jádro (procesor, ANTIC, GTIA, POKEY) se pro normální běh **nezměnilo**:
  obraz po 600 i 1500 snímcích je bit po bitu stejný jako v B290.
- Javu jsem přeložil proti skutečnému Android API 34 a názvy/typy všech
  nativních funkcí sedí 1:1 (vygenerovaná JNI hlavička).

## Co v B291 není (poctivě)

- **ATR / disketová mechanika**: v C++ jádře zatím není emulace mechaniky
  (SIO D1:). Tlačítko ATR/DISK to na přístroji napíše, nic se nepředstírá.
- **CLOAD z pásky**: čtení z kazety zpět do jádra ještě není (jako v návrhu).
- **Joystick**: ve schváleném návrhu není, proto ani tady.
- Start Atari bez mechaniky trvá jako dosud ~10 s (jádro čeká na disk).
  Při spouštění XEX se to čekání zkracuje na ~1,5 s.

## Co jde ověřit jen na telefonu

Skutečné vykreslení na displej (SurfaceView), dotyky a zvuk (OpenSL). V
appce je pod LOG/CHYBA nový seznam testů (9 kroků) – odklepni ho a pošli log.
