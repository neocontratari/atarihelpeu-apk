# Co je v B304

Tvoje zpráva: „Udělej úvodní demo nově podle tohoto návrhu a nechat to na
tobě – grafika může být HD a nemusí splňovat požadavky Atari, Sega a PS1.“

## Nové úvodní demo podle tvého scénáře – celé v C++

Žádné video, žádné WebView, žádné obrázky kromě oficiálního loga N&P.
Všechno se počítá v C++ a kreslí přes OpenGL ES na vlastním vlákně přímo
do displeje (nová knihovna `napintro`). Telefon kreslí demo na výšku i na
šířku (otočíš-li telefon, demo se přeskládá), titulky česky i anglicky
přesně podle scénáře.

| čas | scéna | co je vidět | co je slyšet |
|---|---|---|---|
| 0–8,7 s | 1 | úplná tma, ve tmě bliká stará televize na ponku, rozsvítí se žárovka, šedá garáž (ponk s nářadím, regál, sud), vrata se zvednou jako opona, kamera vyjede ven | bzučení staré obrazovky z dálky (50 Hz síť, sníh, pískot řádků), cvaknutí žárovky, motor a řinčení vrat, vítr, svist |
| 8,7–17,3 s | 2 | valící se černá mračna nad kopci a lesy, záblesky v mracích, hejno ptáků, vzdálené blesky, déšť, atmosféra houstne | vítr, zpěv ptáků, hromy (vzdálené dunění i blízké prásknutí), déšť, temný tón |
| 17,3–27 s | 3 | obrovský oslepující blesk, kamera s úctou najede na Atari 130XE na stole (3D model, 62 kláves s popisky, ATARI 130XE, HELP/START/SELECT/OPTION/RESET), lampa, prach ve světle, okno s deštěm, rozsvítí se kontrolka | klidné plochy a zvonky (Dm – B – F – C), déšť za oknem, hromy, tlukot srdce, když se rozsvítí kontrolka |
| 27–37 s | 4 | z Atari vystřelí energie a rozsvítí displeje pěti mobilů, rychlé střihy: ATARI (BASIC, program, duha), SEGA, PLAYSTATION, 1 KLIK · LAN · TV / PC, pak všech pět displejů vedle sebe „BEZ REKLAM · BEZ BALASTU“, energie zesílí do bílé | 120 BPM (kopák, basa, arpeggio jako POKEY), výstřely a zásahy energie, ke každému střihu vlastní znělka (klávesy Atari, FM plechy Sega, třpyt PS1, pípnutí sítě) |
| 37–45 s | 5 | tma, rozsvítí se zlaté logo N&P (C) 2026 VCS se zlatým prachem a odleskem, bílé filmové titulky (vydání 20. listopadu 2026, atarihelp.eu, Neil, Melbourne House & my AI Wingman, (C) 2026 N&P Studio) | gong a hluboké dunění, D dur, zlaté zvonky, tiché tóny klavíru k titulkům |

- **Přeskočit:** dotek kdekoli (vpravo nahoře svítí PŘESKOČIT ›) nebo
  tlačítko ZPĚT. Menu se mezitím načetlo pod demem, takže je hned vidět.
- **Pustit znovu:** OPTIONS → PUSTIT INTRO.
- **TV / PC:** když běží přenos, C++ kreslí pro TV zvlášť obraz na šířku
  16:9 (854×480) i se zvukem – i když držíš telefon na výšku.
- **Obraz a zvuk:** celý zvuk se spočítá dopředu v C++ (asi za vteřinu).
  Java ho jen přehrává a C++ se po každé dávce dozví, kolik už reproduktor
  opravdu přehrál – podle toho běží obraz. Hrom tedy udeří s bleskem i na
  pomalejším telefonu.
- **Rychlost:** mraky a garáž se kreslí v menším rozlišení a zvětší se
  (jsou měkké, rozdíl není vidět); když telefon nestíhá, rozlišení těchto
  dvou vrstev se samo sníží. Nakonec jde celý obraz přes „film“ – záře kolem
  světel, vinětace a zrno.
- Starý ATARI 130XE EMULATOR, HELP (Atari C++), SEGA a PS1 jsou beze změny.
  Původní intro (etapy ve WebView) zůstává v appce jen jako záloha, kdyby
  se knihovna `napintro` nenačetla.

## Ověřeno na PC (stejný C++ kód jako v appce)

- `test_overeni/b304/demo_snimky` – demo přes Mesa (OpenGL ES bez okna) do
  PNG v libovolném čase a rozlišení: všechny shadery se přeloží, celé demo
  jsem prošel po půl vteřině na výšku (telefon) i na šířku (TV) a podle
  snímků ladil scény (garáž, mraky, blesk, kamera u Atari, střihy, logo).
- `test_overeni/b304/demo_zvuk` – celý zvuk do WAV: 45,6 s spočítáno za
  1,0 s (46× rychleji než přehrávání), první vteřina za 15 ms; hlasitost po
  vteřinách (scéna 1 tiše kolem −24 dBFS, hudba ve scéně 4 kolem −12 dBFS),
  0 přebuzených vzorků mimo limiter; spektrogram odpovídá scénáři (bzučení,
  ptáci 8–13 s, hromy v časech blesků, bicí 28–37 s, gong 37,85 s).
- `test_overeni/b304/test_b304_jni_host` – **skutečný most do Androidu**
  (`nap_intro_gl.cpp`) na PC: místo okna telefonu pbuffer, Java a
  AudioTrack nahrazené testem (zápis po 1024 rámcích, buffer 100 ms, hlášení
  přehraných rámců). Výsledek 8 / 8:
  - demo se připraví (0,2–0,7 s), Java do té doby dostává ticho,
  - čas obrazu jde za tím, co „reproduktor“ přehrál: průměr 33 ms, nejvíc
    83 ms (i při pomalých snímcích pod ThreadSanitizerem nejvíc 207 ms),
  - TV dostává snímky 854×480,
  - zvuk, který Java dostala, je **přesně** stopa dema (2 010 959 rámců,
    0 rozdílů),
  - po celé stopě konec (demo hlásí hotovo, ne dřív), přeskočení zastaví
    kreslení i zvuk, okno se uvolní,
  - ThreadSanitizer: 0 hlášení v našem kódu (hlášení uvnitř Mesy, PC
    ovladače grafiky, jsem nepočítal).
- Skutečná Java třída `NativeIntroBridge` z appky v JVM na PC načetla
  knihovnu a zavolala všech 12 svých native metod – názvy i typy sedí
  (jinak by na telefonu spadlo `UnsatisfiedLinkError`).
- Všech 28 shaderů prošlo referenčním překladačem Khronos v přísném režimu
  OpenGL ES 2.0 (GLSL ES 1.00) – Mesa na PC je shovívavější než překladače
  v telefonech, tohle by odhalilo, co by na Mali / Adreno neprošlo.
- Most přeložený na PC přes gcc i clang bez varování.

**Co jsem ověřit nemohl:** skutečný telefon a TV (rychlost na tvém GPU,
přesné sesazení zvuku s obrazem na reproduktoru) a zvuk jsem neslyšel –
kontroloval jsem ho jen měřením a spektrogramem.

## Log

Řádky `B304 DEMO_START`, `B304 DEMO_BEZI` (po 3 s: GPU, příprava, rychlost
kreslení, jestli čas řídí zvuk, odchylka obrazu od zvuku), `B304 DEMO_KONEC`
(proč skončilo, kolik snímků za vteřinu) a `B304 DEMO_INFO`. Když něco
nesedí, pošli log a napiš, v kolikáté vteřině.
