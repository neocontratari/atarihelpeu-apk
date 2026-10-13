# Co je v B306

Tvoje zpráva: „Zvuk naběhl v Sega až po určité době a po refresh stránky zase se zvuk
odmlčel … u přenosu Segy a PS1 mám pocit, že by to mohlo být ještě lepší … a pokud
možno vše předělat postupně do čistého C++.“

## 1. Zvuk v prohlížeči – proč naskočil až po čase a po F5 zmizel

**Z tvého logu:** telefon posílal zvuk celou dobu správně (Sega 47 970–48 125 vzorků/s,
PS1 44 070–44 127/s, 0 přeskoků). Problém byl v prohlížeči: Chrome (i Edge, Firefox)
nepustí zvuk stránky, dokud do ní neklikneš nebo nestiskneš klávesu – a po F5 znovu.
Prohlížeč to sám hlásil do logu: Sega `zvuk NEHRAJE` 1 minutu, PS1 celé 3 minuty.
Moje stránka se o zvuk pokusila jen jednou hned po načtení (to prohlížeč odmítne)
a pak už ne. Zvuk pak naskočil jen náhodou – když Wi-Fi na chvilku zadrhla, stránka
zvuk znovu zařadila a Chrome ho (protože jsi předtím kliknul) pustil. Proto „až po
určité době“. V nápovědě jsem ti navíc radil F5 – tím se to celé vrátilo na začátek.

**Teď:**

- dokud zvuk nehraje, je nahoře **žlutá výzva „ZVUK: KLIKNI KAMKOLI DO OBRAZU“**,
- **jedno kliknutí nebo klávesa kdekoli** (i hra z klávesnice PC) zvuk hned pustí –
  stránka to zkouší při každém kliknutí, ne jen jednou,
- po F5 se výzva vrátí a zase stačí jedno kliknutí,
- do logu appky: `B306 ZVUK: prohlížeč zvuk nepustil …` a `B306 ZVUK: hraje – prohlížeč
  ho pustil po X s`.

Ověřeno v Chromiu **bez** přepínače, který autoplay vypíná (chová se jako tvůj Chrome):
bez kliknutí ticho + výzva, kliknutí → hraje, F5 → zase výzva, klávesa → hraje. Stejný
test se stránkou z B305 ukázal přesně tvoje chování: po kliknutí ticho, zvuk naskočil
až po zadrhnutí sítě, po F5 a kliknutí zase ticho.

## 2. Zvuk blíž obrazu (u Segy hlavně na S8)

Zvuk šel na TV v B305 až **za frontou reproduktoru telefonu** – na S8 drží appka
~6 000 vzorků (≈ 130 ms, z tvého logu `audioBacklog=6553`), aby reproduktor nikdy
nehladověl. A prohlížeč k tomu držel další polštář 120–210 ms a občas kus zvuku zahodil.

- **C++:** zvuk pro TV se teď bere **hned, jak ho jádro vyrobí** – Sega: FM + PSG se
  smíchají přímo v jádře (stejné zesílení a omezovač jako pro reproduktor), PS1: přímo
  z `retro_run`. Snímek jde do přenosu až po celém kroku jádra, takže **paket nese obraz
  i zvuk téhož okamžiku hry** (B305: zvuk o 4–12 paketů později).
- **Prohlížeč:** zvuk se převzorkuje přímo na vzorkování zvukové karty a kusy navazují
  vzorek na vzorek. Rezerva se drží jen tak velká, jak síť opravdu potřebuje (podle
  nejhoršího zakolísání posledních 10 s); přebytek se ubírá zrychlením o nejvýš 0,8 %
  (neslyšitelné). Jen když je přebytek přes 60 ms (rozjezd hry, po výpadku Wi-Fi),
  přeskočí se najednou – jeden krátký skok místo dlouhého zpoždění. Když Wi-Fi
  vypadne, zvuk se rozjede hned znovu a rezerva se přizpůsobí.

## 3. Sega na TV 60 snímků/s i na S8

Na S8 kreslí appka displej jen každý druhý snímek (tvůj log: 30 paketů/s). Pro TV se
teď snímá **každý snímek (60/s)** – jen pro přenos, displej telefonu dál 30/s (telefon
nemá práci navíc; jádro na S8 ~8 ms z 16,7 ms). Když zvuk telefonu hladoví nebo zvolíš
LOW, platí původní dělení i pro TV.

## 4. Další vylepšení přenosu

- Když se obraz na chvíli zadrhne a zvuk běží dál, chodí **pakety jen se zvukem** –
  zvuk na TV nevypadne.
- Vlákno přenosu a pomocná vlákna JPEG mají vyšší prioritu (jako vykreslování) – na S8
  se kódování neztratí mezi ostatními vlákny; jádro a zvuk mají pořád přednost.

## 5. Úvodní demo – další kus do C++

- **Zvuk dema přehrává C++ (OpenSL ES)**, ne Java AudioTrack (ten zůstal jen jako
  záloha). Obraz dema se řídí tím, co reproduktor opravdu přehrál (OpenSL GetPosition).
- **Na TV / PC jde demo proudem z C++** (`/intro.stream`, obraz 16:9 JPEG + zvuk v jednom
  paketu, jako Sega a PS1) – Java už nic nepřevádí ani nekomprimuje.

## Ověřeno na PC (stejný C++ a stejná stránka jako v appce)

Testovací stroj má jen 2 jádra a běží na něm Chromium, emulátory i server – čísla
zpoždění proto kolísají víc, než budou u tebe.

- `test_b306_prohlizec.py` (Chromium bez autoplay přepínače): zvuk – výzva / kliknutí /
  F5 / klávesa; Sega bod po bodu přesně, 60 snímků/s, 48 000 vzorků/s, **zvuk téhož
  kroku jádra ve stejném paketu jako snímek**, kusy zvuku bez mezer a bez lupanců;
  zadrhávající síť (kolísání 0–25 ms + výpadek 150 ms každé 3 s): bez lupanců, zvuk se
  srovná sám; PS1 totéž; demo: obraz 854×480 z C++, zvuk 44 100/s, **zvuk v paketech
  bit po bitu = stopa dema** a končí v čase obrazu dema.
- `--srovnani` (stránka i C++ z B305 proti B306, stejné měření, fronta reproduktoru
  jako na S8) a hlavní test: **zvuk za obrazem Sega B305 ≈ 250–350 ms → B306
  ≈ 60–125 ms, PS1 ≈ 240–260 ms → ≈ 60–140 ms** (včetně ~35 ms výstupu zvukové karty
  prohlížeče; poslední běh: Sega 252 → 67 ms, PS1 251 → 60 ms). Zadrhávající síť:
  B305 367 ms a 4× zahozený kus zvuku → B306 184 ms, nic zahozeno, bez lupanců.
- `test_sega_60` – **skutečné vlákno emulace a skutečné jádro clownmdemu** (malý program
  pro 68000 místo hry): S8 bez prohlížeče displej 30/s; s prohlížečem **TV 60/s, displej
  dál 30/s**; LOW → TV 30/s; jiný telefon 60/60; zvuk 48 000/s z jádra; ThreadSanitizer 0.
- `test_intro_host` – skutečný `nap_intro_gl.cpp`: do OpenSL jde přesně stopa dema,
  čas obrazu jde za GetPosition (průměr 7 ms, nejvíc 17 ms), zastavení / nový start,
  záloha AudioTrack; ThreadSanitizer: v našem kódu 0 (hlášení jen uvnitř Mesy).
- `test_hub6` – pakety jen se zvukem při zastaveném obrazu, zvuk vzorek po vzorku; TSan 0.
- JVM: skutečné třídy `NativeIntroBridge`, `NativeSegaCoreBridge`, `NativePs1CoreBridge`
  zavolaly všech 7 nových native metod dema + 10 metod přenosu – názvy i typy sedí.
- Regrese: B305 kodeky 35/35, JPEG na víc jádrech (TSan 0), rozbočovač 16/16 (TSan 0),
  Atari B303 se stránkou B306 23/23.

**Co jsem ověřit nemohl:** tvůj telefon, TV a Wi-Fi. Zpoždění zvuku za obrazem u tebe
bude záviset hlavně na Wi-Fi (rezerva se drží nad jejím nejhorším zakolísáním za
posledních 10 s) a na zvukové kartě PC / TV.

## Log

- prohlížeč: `B306 ZVUK: …`, `B306 SEGA PROUD start`, každých 30 s `B306 PROHLIZEC
  SEGA:` / `PS1:` / `INTRO:` – snímky/s, zvuk (napřed ms, **zvuk za obrazem ~ms**,
  rychlost dorovnávání, vzorkování), podtečení, zahozeno,
- appka: `B306 TV PROUD SEGA / PS1 / INTRO` (pakety/s, Mbit/s, doba balení, zvuk,
  pakety jen se zvukem), `B306 DEMO_ZVUK přehrává C++ OpenSL ES`.

Když něco nesedí, pošli log a napiš co (zvuk nehraje ani po kliknutí, zaostává, lupe …).
