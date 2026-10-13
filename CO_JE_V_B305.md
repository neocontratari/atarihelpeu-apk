# Co je v B305

Tvoje zpráva: „Udělej ten TV přenos u Segy a PS1 na TV a PC jako u Atari emu C++,
aby to bylo taky v C++ a tak super jako v emu Atari C++.“

## Sega a PS1 na TV / PC – celé v C++ (stejně jako Atari 130XE v B303)

**Dosud:** Java si vzala snímek z jádra, zvětšila ho na 1280×720 a zkomprimovala
(H.264 nebo JPEG) na hlavním vlákně appky, prohlížeč si H.264 ještě bufferoval
a zvuk chodil zvlášť (`/audio.raw` každých 20 ms). Odtud zpoždění a měkký,
„kostičkovaný“ obraz.

**Teď (nová část `app/src/main/cpp/tvproud`):**

- C++ si snímek vezme **hned, jak ho jádro dokreslí** – Sega po posledním řádku
  VDP, PS1 hned po `retro_run` (přesně ten snímek, který jde na displej telefonu).
  Zvuk bere z toho, co opravdu hraje reproduktor (Sega z AudioTracku, PS1 z OpenSL).
- Pro každý prohlížeč C++ zabalí obraz **i zvuk do jednoho paketu**. Java
  (`/sega.stream`, `/ps1.stream`) jen pošle bajty – žádné zvětšování, žádná
  komprese v Javě, hlavní vlákno appky zůstane volné.
- Prohlížeč dostane **vždy poslední snímek** oproti tomu, co už má. Když Wi-Fi
  nestíhá, snímky se přeskočí – zpoždění neroste. Zvuk nejvýš 0,25 s napřed,
  polštář v prohlížeči 50–300 ms podle sítě (jako u Atari).

### Sega – bez ztráty kvality

- Obraz v prohlížeči je **bod po bodu přesně** obraz jádra (71 680 bodů z 71 680),
  žádné kostičky ani rozmazání z komprese.
- Barvy přes paletu (Sega má naráz nejvýš 192 barev), posílají se jen změny –
  **i když se obraz hýbe** (Sonic běží, pozadí jinou rychlostí než popředí):
  každý kus řádku se popíše jako „tenhle kus minulého snímku, posunutý o tolik“
  (LZ s odkazem do minulého snímku). Scrollující hra ≈ 3–5 Mbit/s při 60 snímcích/s,
  stojící obraz pár bajtů.
- Stmívání obrazovky (všechny barvy nové každý snímek) projde bez celého snímku
  znovu – nové barvy dostanou místa barev, které už na obrazovce nejsou.

### PS1 – JPEG jen tam, kde se něco změnilo

3D grafika má tisíce barev – bez ztráty by to bylo 10× víc dat, než Wi-Fi do TV
unese. Proto JPEG, ale:

- **v původním rozlišení hry** (320×240, 512×480, 640×480 …) – zvětšuje až
  prohlížeč (dřív se zvětšovalo na 1280×720 a teprve pak komprimovalo),
- **jen obdélník, který se změnil** (blikající kurzor v menu = pár set bajtů),
- hra na 30 snímků/s: druhý (stejný) snímek = jen zvuk,
- když obraz stojí (menu, text), přijde po chvilce **ještě jednou ve vysoké
  kvalitě** – text je ostrý,
- **kvalitu řídí síť**: když Java čeká na zápis do Wi-Fi, kvalita klesne
  (menší snímky), když je síť volná, zase stoupne.
- Vlastní rychlý JPEG kodér v C++: výsledek stejný jako stb_image_write (stejný
  obraz, stejná velikost), ale 1,5–1,7× rychlejší; velký obraz (512×480, 640×480)
  se kóduje **na 2 jádrech naráz** a spojí se do jednoho JPEG. Tvůj Galaxy S8
  (Android 9 – z logu) nemá systémový JPEG z C++ (ten je až od Androidu 11).

### V prohlížeči na TV / PC

- Stránka se **sama přepne** podle toho, co je na telefonu: Atari / Sega / PS1
  (i bez obnovení stránky). Když hra neběží, jede to postaru (snímání okna appky).
- Panel **OBRAZ** (pohni myší): „vyplnit obrazovku“ (jako dosud – obsah bez
  černých okrajů přes celou TV 16:9) nebo „celý, bez ořezu“ (celý snímek 4:3,
  po stranách černo). Jas / kontrast / sytost a VYLADĚNÍ OBRAZU fungují i tady.
- Klávesnice z PC u Segy funguje jako dosud.

## Ověřeno na PC (stejný C++ kód jako v appce)

- `test_overeni/b305/test_kodeky` – **35 / 35**:
  - Sega: syntetická hra jako Sonic (2 vrstvy dlaždic s paralaxou, vlnící se voda,
    postavičky, skóre, koloběh barev), 10 s: každý snímek v „prohlížeči“ přesně,
    2,8 Mbit/s při 60 snímcích/s, balení průměr 0,6 ms (PC); stmívání, paleta
    přes 256 barev (30× přestavba, žádný celý snímek), stojící obraz (6 B),
    přepnutí 320 → 256 → 320 bodů, náhodný šum (nejhorší případ 8 ms).
  - PS1: snímky 3D scény (úvodní demo B304) 320×240: 16,6 kB na snímek, PSNR 37 dB,
    kódování 1,0–1,5 ms; 640×480 na 2 jádrech 2,2 ms (na 1 jádře 3,9 ms, stb 5,5 ms); zjemnění stojícího obrazu
    +5 až +7 dB; malá změna = obdélník 16×16 (≈ 750 B); řízení kvality sítí;
    rozměry 368×240, 512×480, 640×512, 257×193, 256×224, 33×17 a obdélník
    na kraji obrazu – JPEG stejný jako ze stb.
- `test_pasy` – kodér JPEG na víc jádrech: počet pásů se mění každý snímek
  (1 / 2 / 3), 300 snímků bajt po bajtu stejných; ThreadSanitizer 0 hlášení.
- `test_hub` – **16 / 16** (vlákna jako v appce: jádro 60 snímků/s, zvuk po 10 ms,
  rychlý a pomalý prohlížeč): Sega vždy přesně, rychlý dostává každý snímek,
  pomalý vždy ten poslední (žádná fronta), zvuk navazuje vzorek po vzorku, paket,
  který se nevešel do pole Javy, přijde znovu (žádná ztráta). ThreadSanitizer:
  **0 hlášení**.
- `test_b305_prohlizec.py` – **24 / 24**: **skutečný** `nap_sega_native_proof.cpp`
  (snímky přes skutečné zpětné volání VDP, zvuk přes JNI pullAudioStereo)
  a **skutečný** `nap_ps1_native.cpp` (start přes JNI ps1BootDoMonitoru, skutečné
  vlákno emulace a OpenSL callback), skutečná stránka z appky v Chromiu:
  - Sega: canvas 320×224 **přesně** jako jádro, 60 snímků/s, 48 000 vzorků/s,
    **zpoždění změna v jádru → obraz v prohlížeči medián 16 ms** (1 snímek),
  - PS1: 30 obrázků/s (hra na 30), PSNR 36–37 dB, 44 100 vzorků/s,
    **zpoždění medián 30–36 ms**, stojící obraz zaostří (PSNR 39 → 43 dB),
  - přepnutí Sega → PS1 bez obnovení stránky, spadlé spojení se obnoví samo,
    hra pryč → zpět postaru, žádné dotazy na MJPEG / H.264 / starý zvuk,
    „vyplnit obrazovku“ i „celý, bez ořezu“ přesně, žádná chyba JavaScriptu.
- ThreadSanitizer nad skutečnými soubory Segy a PS1 (jádro + zvuk + 2 prohlížeče):
  **0 hlášení v kódu B305**. (Jedno staré hlášení mimo B305: PS1 při otevření
  zvuku nastaví `s_sl_next` až po zařazení bloků – na telefonu přijde první
  zpětné volání až po 23 ms, takže se to nepotká; nesahal jsem na to.)
- Skutečné Java třídy `NativeSegaCoreBridge` a `NativePs1CoreBridge` v JVM na PC
  zavolaly všech 10 nových native metod – názvy i typy sedí.
- Atari (B303) test znovu se stránkou z B305: **23 / 23** (stránka je společná).
- Překlad pro ARM64 (clang): DCT jde přes NEON (vektory 4×float).

**Co jsem ověřit nemohl:** skutečný telefon, TV a Wi-Fi; skutečnou hru (ROM ani
CD na PC nemám – Segu jsem krmil syntetickou hrou přes skutečné zpětné volání
VDP, PS1 snímky 3D scény přes skutečné vlákno emulace). Rychlost na S8 odhaduji:
Sega ~1,5–2 ms na snímek, PS1 320×240 ~3–4 ms, 640×480 na 2 jádrech ~6–8 ms.
Rozbalení JPEG na slabší TV – když nestíhá, starší obrázky, které nový celý
překryje, se zahodí (zpoždění neroste).

## Log

- appka: `B305 TV PROUD SEGA` / `B305 TV PROUD PS1` – prohlížeč připojen, každých
  30 s (pakety/s, snímky se změnou/s, Mbit/s, kB na snímek, rozměr, doba balení,
  zvuk, u PS1 kvalita JPEG a doba kódování), odpojen.
- prohlížeč: `B305 SEGA PROUD start`, `B305 PS1 PROUD start` a každých 30 s
  `B305 PROHLIZEC SEGA:` / `B305 PROHLIZEC PS1:` (snímky/s, JPEG/s, zvuk, polštář,
  chyby, velikost okna, režim obrazu).

Když něco nesedí, pošli log a napiš, která hra a co přesně (zpoždění zhruba
kolik, trhání, barvy, zvuk).
