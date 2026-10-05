# Co je v B292

Tvá slova po B291: „mám pocit, že sis tam plno věcí přetáhl z Javy … při
spuštění nějaké hry to skočilo do Java emu Atari … některé hry mají chybu
v grafice … další krok navrhuji CLOAD a CSAVE … EJECT bude sloužit jako cesta
na výběr uložených WAV souborů v mobilu … pro jistotu udělej důkladnou
kontrolu jádra.“

## 1. Skok do Java emu – příčina a oprava

Z tvého logu: výběr hry z NET/HRY přišel **dvakrát** (38 ms po sobě). B291 po
první hře hned zrušila „cíl = C++ HELP“, takže druhé stažení šlo starou cestou
do starého JS emulátoru (ATARI 130XE EMULATOR z hlavního menu).

Teď: cíl platí po celou dobu procházení her (zruší se až odchodem jinam),
stejný požadavek do 5 s se zahodí (v logu `B292 NET_HRY duplicitni pozadavek
zahozen`), a když přístroj mezitím vypneš, odložené spuštění se zruší.

**Co je v HELP Java a co C++:** Java jen vytvoří plochu displeje, předá doteky
prstů a obslouží to, co umí jen Android (výběr souboru, dialog, uložení WAV,
prohlížeč her NET/HRY). Celá emulace – procesor, ANTIC, GTIA, POKEY, PIA,
paměť 130XE, zavaděč XEX, psaní textu, kazeta (demodulace WAV i nahrávání),
kreslení přístroje – je v C++. Staré C++ jádro (`nap_atari_cpu.*`,
`nap_atari_mem.h`, `nap_atari_video.h`, `nap_atari_pokey.h`, `nap_atari_xex.h`)
je z appky **odstraněné**.

## 2. Důkladná kontrola jádra → nové jádro přesné po cyklech

Kontrola starého jádra našla vážné chyby (proto „některé hry mají chybu
v grafice“):

- ANTIC nebral procesoru **žádné** cykly (skutečné Atari si pro obraz bere až
  ~70 % cyklů na řádku) – hry běžely s jiným časováním než na Atari,
- přerušení DLI chodilo o řádek pozdě,
- bity svislého a vodorovného rolování v display listu byly **prohozené**,
- kolize hráčů a střel vracely vždy „srážka se vším“ ($0F),
- chyběly priority PRIOR, pátý hráč, vícebarevní hráči, režimy GTIA 9/10/11,
  CHACTL (inverze/blikání znaků),
- zápis do registrů se počítal od začátku instrukce, ne od cyklu zápisu.

Nové jádro (čisté C++, `nap_atari_6502.h` + `nap_atari_machine.h`) je postavené
podle chování skutečného hardwaru (Altirra Hardware Reference Manual a zdroj
emulátoru Altirra jako zdroj **faktů** o časování):

- **6502**: každý přístup na sběrnici = 1 cyklus (i „zbytečná“ čtení a dvojitý
  zápis u RMW instrukcí), přerušení přesně podle 6502 (i zvláštnosti BRK/NMI,
  skoků, CLI/SEI, ztracené NMI).
- **ANTIC**: DMA po jednotlivých cyklech (střely, display list, hráči, LMS,
  obnova paměti, hrací pole podle režimu/šířky/HSCROL), DLI/VBI na cyklu 8,
  WSYNC, VCOUNT, NMIST/NMIRES.
- **GTIA**: kreslí po barevných taktech, změna registru se projeví přesně na
  taktu zápisu, hráči/střely jako posuvné registry, priority rovnicemi GTIA,
  kolize, režimy 9/10/11 i „pseudo režim E“ (přepnutí GTIA režimu uprostřed
  řádku – používá ho např. Postcard).
- **POKEY**: čítače po cyklech, polynomy přesně jako hardware (RANDOM),
  časovače, přerušení, sériový port (kazeta), klávesnice.

### Testy (na počítači, ne odhadem)

| test | staré jádro | atari800 | **nové jádro** |
|---|---|---|---|
| 6502 SingleStepTests (2 560 000 instrukcí po cyklech) | – | – | **0 chyb** |
| Acid800 (58 testů hardwaru od autora Altirry) | 7 (pak zamrzlo) | ~25 | **42** |

Hry proti emulátoru atari800 (stejné snímky): Donkey Kong Jr. 4 body z 92 160,
Mission a River Rumble shodné, **Postcard** shodné (dřív v rámečcích chyběly
obrázky), Commando u nás běží (atari800 ho ani nespustí – skončí v self-testu),
Decathlon se liší jen v okraji obrazu, který atari800 nekreslí, a fází
animace.

Zbývajících 15 testů Acid800 jsou okrajové věci (změna DMACTL uprostřed řádku,
„fantomová“ DMA hráčů, chyby ANTIC při HSCROL, přesné časování časovačů POKEY
na 1–2 cykly, přímý sériový vstup SIO, PIA CA1/CB1). Hry je skoro nepoužívají –
dodělám je v dalších buildech.

## 3. Kazeta – CSAVE i CLOAD

- **CSAVE**: napíšeš `CSAVE`, RETURN, po pípnutí RETURN. Nahrává se **linka
  SIO DATA OUT** – přesně to, co Atari posílá do magnetofonu (FSK 5327/3995 Hz,
  600 Bd), ne zvuk z reproduktoru. Každé CSAVE se uloží do vlastního souboru
  `Download/AtariHelp/Atari_emu/csave_<datum>_<čas>.wav`.
- **EJECT** otevře dvířka a nabídne kazety: WAV uložené CSAVE (nejnovější
  nahoře), „JINÝ WAV Z TELEFONU…“ (systémový výběr souboru) a „VYJMOUT
  KAZETU“. Po výběru se kazeta vloží a dvířka zavřou.
- **CLOAD**: EJECT a vyber kazetu, napiš `CLOAD`, RETURN, po pípnutí stiskni
  **PLAY** a RETURN. Atari si samo změří rychlost pásky z úvodních bajtů
  `$55 $55` (stejně jako skutečné Atari) a nahraje program.
- REW = páska na začátek, FWD = páska o 10 s dál. Kazeta zůstává
  v magnetofonu i přes vypnutí/zapnutí Atari.

Jak to funguje: WAV se v C++ demoduluje (energie obou tónů v okně 1/1332 s),
u sterea se vezme kanál, kde jsou data. Bajty skládá až POKEY v emulaci
rychlostí, kterou si nastavil OS – jako na skutečném stroji.

Ověřeno na počítači: CSAVE → WAV → CLOAD → LIST ukáže přesně uložený program.
Totéž i s „poškozenými“ WAV: 48 kHz stereo (data v pravém kanále, v levém
hudba), páska o 3 % rychlejší, šum, zúžené pásmo, obrácená polarita – i 22 kHz
8 bit, páska o 4 % pomalejší, silný šum. Vše se nahrálo správně.

## 4. Klávesnice

Klávesa drží, dokud na ní držíš prst – OS ji pak sám opakuje jako skutečné
Atari (po ~1 s). SHIFT zámek nastavuje i SKSTAT (programy, které testují
držený SHIFT). Při psaní textu (TXT) se už neztrácejí zdvojená písmena
(„HELLO 1000“).

## 5. Turbo-BASIC 34009

Je to správně. Čerstvý Turbo-BASIC XL ukáže `?FRE(0)` = **34021**. Když předtím
napíšeš třeba `?FREE`, vznikne proměnná FREE (12 bajtů) → **34009**. Stejná
čísla dává emulátor atari800 i staré jádro.

## 6. Samokontrola na telefonu

Samokontrola jádra (stránka LOG/CHYBA) má nová očekávaná čísla pro nové jádro:
procesor `29C55806`, paměť `8CD99DC5`. Rychlost se teď měří na celém stroji
(150 snímků startu OS) – v logu „…x realny cas Atari“.

## Testy na PC

- `test_overeni/b292/test_core.cpp` – boot, Acid800, XEX hry, kazeta
  (`./test_core kazeta`).
- `test_overeni/b292/test_6502_sst.cpp` – procesor proti SingleStepTests.
- `test_overeni/b292/test_b292_jni_host.cpp` – **skutečný**
  `nap_atari_native.cpp` se skutečnými vlákny: samokontrola, držení klávesy,
  CSAVE přes vlákna, EJECT, vložení kazety, CLOAD přes tlačítko PLAY → LIST
  (11 kontrol, 0 chyb).
- `test_overeni/b291/*` přeložené na nové jádro: 24 + 10 kontrol, 0 chyb.
