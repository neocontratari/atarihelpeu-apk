# Co je v B307

Tvoje zpráva: „Z klávesnice mého reálného Atari 130XE vystřelí v C++ shaderu ty svítící
neonové pruty a linie … se protnou a zformují celou naši trojici … na monitoru bleskově
prostříháš ta herní defilé v 50 FPS … Přidej tam prosím i tlačítko SKIP“ a pak:
„Jen to atari 130xe prosím aspoň to zachovej … ty 16bit grafiky a 32 bit ukázky musí být
lepší a to opravdu.“

Změnila se jen **scéna 4 úvodního dema (27–37 s)**, tlačítko SKIP a to, co z intra ještě
dělala Java. Scény 1–3 (garáž, mraky a blesk, Atari 130XE) a scéna 5 (logo, titulky)
jsou stejné jako v B304. Hlavní nabídka a emulátory se neměnily.

## 1. Scéna 4 – z kláves tvého Atari 130XE

| čas | co se děje |
|---|---|
| 27,00 s | klávesy tvého Atari 130XE se rozzáří (vlna od kontrolky) |
| 27,55 s | z kláves vystřelí neonové pruty – vlevo azurové, vpravo růžové – a letí po obloucích |
| 28,75 s | pruty se protnou u konzolí, záblesk, kreslí se drátěný obrys |
| 29,45 s | konzole N&P 16 a N&P 32 se zhmotní zdola nahoru (zářící pás) |
| 30,60 s | celá trojice vedle sebe: **Atari 130XE** (tvůj model, beze změny), **N&P 16**, **N&P 32** |
| 31,95 s | kamera proletí kolem konzolí k CRT monitoru, 32,35 s se CRT zapne |
| 32,80 s | defilé 50 snímků/s: **Pitt-Kitt Platinum** → **16 BIT** (34,25 s) → **32 BIT** (35,45 s) |
| 36,65 s | záblesk do loga (scéna 5 od 37 s jako dřív) |

Obraz i zvuk se řídí stejnými čísly (jeden soubor `nap_demo_cas.h`), takže rána, střih
a zvuk přijdou spolu.

## 2. Konzole N&P 16 a N&P 32

Podle tvého „nechám na tobě“ jsem obě konzole navrhl vlastní – žádný cizí tvar ani logo:

- **N&P 16** (16 bit, cartridge) – kartáčovaný kov a tmavě modrá, ve slotu cartridge
  se štítkem, kontrolka, **zlaté logo N&P**.
- **N&P 32** (32 bit, CD) – černá s azurovými pruhy, víko s duhově lesklým diskem,
  tlačítko, **zlaté logo N&P**.
- **Atari 130XE** je přesně tvůj model ze scény 3 – nesahal jsem na něj, jen mu
  svítí klávesy, když z nich vyletí pruty.

## 3. Defilé na CRT – 50 snímků za sekundu

- **Pitt-Kitt Platinum** – opravdové záběry z tvé hry. Nahrálo je naše C++ jádro Atari
  130XE z appky přímo z XEX, který jsi poslal (žádné video ani screenshoty z jiného
  emulátoru): titul N&P → KAPITOLA 1 (Las Vegas 1982) → jízda po Vegas, a k jízdě
  **zvuk hry z POKEY**. Klip má 117 kB (`assets/intro/pittkitt.klip`).
- **16 BIT** – nová ukázka, kreslí ji procesor jako grafický čip 16bitové konzole:
  320×224, paleta 9 bit, nebe s přechodem po řádcích, slunce s pruhy, paralaxa (hory,
  město), moře vlnící se po pásech, odlesk slunce, loď s „options“ střílí dávky na
  formace dronů (výbuchy, střely nepřátel), skály v popředí, HUD. Hra se opravdu
  hraje snímek po snímku (60/s), nic není předkreslené.
- **32 BIT** – nová ukázka na GPU: noční závod městem v low-poly 3D s typickým 32bitovým
  vzhledem – „chvění“ vrcholů, afinní textury, Gouraudovo stínování, mlha, dithering
  a 15bitové barvy; lampy svítí na silnici, neonové nápisy, předjíždění, HUD.
- Přepnutí kanálu (šum, zvlnění obrazu), CRT má zakřivení, řádky, masku RGB a odlesk.

## 4. Zvuk scény 4

Nová hudba: náběh při rozzáření kláves, groove 120 BPM (Dm – Bb – F – C), zapnutí
každého prutu, rána při dopadu, akord a zvonky při zhmotnění, vír při průletu, cvaknutí
a šum zapnutí CRT, ke každému střihu vlastní znělka (Atari ve stylu POKEY, 16 bit FM
riff, 32 bit pad) – a při jízdě Pitt-Kitt **zvuk tvé hry**. Celá stopa (45,6 s) se spočítá
v C++ za ~0,75 s, nic se neořezává.

## 5. SKIP a Java z intra pryč

- **Tlačítko SKIP** vpravo nahoře (jen v telefonu – na TV / PC není co mačkat). Kreslí
  ho C++ a **o dotyku rozhoduje C++** – Java jen pošle souřadnice prstu. Klepnutí kamkoli
  jinam demo přeskočí taky (jako dřív); prvních 0,7 s se dotyk nepočítá (prst, kterým
  jsi appku spustil).
- **Zvuk startuje a končí C++ samo** (OpenSL ES). Záložní Java AudioTrack je pryč –
  kdyby OpenSL na nějakém telefonu nešel, demo poběží podle hodin bez zvuku.
- Než demo začne, C++ si **nanečisto** (bez zobrazení, obrazovka je ještě černá)
  nakreslí 5 snímků scény 4. Ovladač GPU tak připraví všechny shadery a textury
  předem a obraz se nezadrhne až při prvním použití (střih na 32 bit, konzole, CRT).
- V Javě z intra zůstalo jen to, co bez ní na Androidu nejde: plocha (SurfaceView),
  předání dotyku, start / stop a přeposílání hotových bajtů proudu na TV. Pro staré
  prohlížeče bez ReadableStream (ty `/intro.stream` neumí) zůstala stará cesta – i tam
  už snímek přichází hotový z C++.

## 6. Co jsem rozhodl sám (jak jsi napsal „nechám na tobě“)

- Konzole jsou vlastní N&P (ne podoba Segy ani PlayStation), na CRT je tvoje hra a
  dvě naše vlastní ukázky – žádné cizí hry, postavičky ani BIOS.
- Záběry Pitt-Kitt: titul, kapitola, jízda – to nejvíc „prodá“ hru za 1,5 s.
- Ze scény 4 zmizely mobily a karty „1 KLIK · LAN · TV/PC“ a „BEZ REKLAM“ – místo
  nich je trojice a CRT. CRT monitor stojí za Atari už ve scéně 3 (vypnutý), aby se
  neobjevil z ničeho.

## Ověřeno na PC (stejný C++ jako v telefonu)

- **Celé demo po 0,25 s** na výšku i na šířku (TV): 2 × 181 snímků, **0 chyb OpenGL**.
- **Shadery** (všech 31, i nové) přísně podle OpenGL ES 2.0 přes referenční překladač
  Khronos: 0 chyb (Mesa na PC je shovívavější než Mali / Adreno – tohle by odhalilo,
  co by na telefonu neprošlo).
- **SKIP**: nakreslené tlačítko leží přesně tam, kde ho hledá dotyk – na výšku, na šířku
  i na tabletu (20/20); před 1,2 s a po 44,6 s není, na TV nikdy.
- **Skutečný `nap_intro_gl.cpp`** (11/11): zvuk pustí C++ samo, záběry i zvuk Pitt-Kitt
  načtené, dotyk SKIP / jinam / hned po startu nic, čas obrazu jde za tím, co
  reproduktor přehrál (průměr 5–8 ms, nejvíc 19 ms), **celá stopa bit po bitu** včetně zvuku hry,
  OpenSL nejde → demo běží dál bez zvuku, chybí klip → demo jede dál. ThreadSanitizer:
  v našem kódu 0 hlášení (jen uvnitř Mesy).
- **JVM**: skutečné třídy `NativeIntroBridge` a `NapDemoView` se skutečným C++ – všech
  16 native metod sedí, klepnutí na SKIP → `demoKonec("tlacitko SKIP")`, jinam →
  `demoKonec("dotyk mimo SKIP")`.
- **Prohlížeč (Chromium)**: demo na TV / PC proudem z C++ až za defilé – obraz 854×480,
  zvuk 44 100/s, **zvuk na TV bit po bitu = stopa dema včetně zvuku Pitt-Kitt**, snímek
  z prohlížeče během jízdy ukazuje tvou hru na CRT.
- **Klip** jsem nahrál znovu z tvého XEX naším jádrem – bajt po bajtu stejný jako ten v appce.

**Co jsem ověřit nemohl:** skutečný telefon a jeho GPU (rychlost, barvy) – PC kreslí
procesorem (Mesa). Když telefon nestíhá, demo samo zmenší rozlišení těžkých vrstev
(jako od B304).

## Log

- `B307 DEMO_ZVUK prehrava C++ (OpenSL ES), zvuk Pitt-Kitt ano, …`
- `B304 DEMO_INFO … priprava … ms (nanecisto 5 snimku … ms) … zabery Pitt-Kitt ano, zvuk hry ano …`
- `B304 DEMO_KONEC (tlacitko SKIP)` / `(dotyk mimo SKIP)` / `(dobehlo)`

Když něco nesedí, napiš v kolikáté vteřině a co (pruty, konzole, CRT, zvuk, SKIP) a pošli log.
