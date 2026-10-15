# Co je v B309

Tvoje zpráva: „funguje to, ale ten log udělej jako v atari emu help – prostě s možností
okamžitého odeslání … další chybička – pokaždé při přehození hry či emulátoru musím
refreshovat stránku F5, aby se chytil nový obraz. Ale doom šlape na jedničku s hvězdičkou …
na intro na začátku ti přidám 14 s – intro může mít 59 vteřin – ty ukázky her jsou hektické
a rychlé … ty dvě hry, co jsi tam ukázal, budeme muset časem udělat – jak do Segy, tak do PS1.“

## 1. TV / PC: nový obraz bez F5 (chyba nalezená v tvém logu)

V tvém logu z TV (`Tv_prenos_-_a_log.txt`) je po každém přepnutí stejný vzor:
`B306 PS1 PROUD start` → hned `stream ended, resetting` → za ~3 s `PAGE LOADED` (tvoje F5).

**Příčina:** když TV ukazovala obrazovku telefonu přes H.264 (menu appky, nebo chvilka, kdy se
při přepnutí hry restartuje jádro) a pak naskočil proud z C++ (PS1, Sega, demo), stránka H.264
zastavila – jenže zrušené čtení H.264 doběhlo až potom a „vrátilo“ starý obrázek `#v`
(černý, přes celou obrazovku, ve stránce nad canvasem). Nový obraz se kreslil pod ním. F5
pomohlo, protože čerstvá stránka H.264 vůbec nezapne.

**Proč to testy nechytily:** prohlížeč v testech neměl internet, takže se přehrávač H.264
(JMuxer z cdn.jsdelivr.net) nenačetl a cesta H.264 nikdy neběžela. Nový test ho nahrazuje
falešným se stejným rozhraním – H.264 běží jako u tebe.

**Oprava** (stránka prohlížeče v `MainActivity.java`):
- každé spojení H.264 má své číslo – zrušené už na obraz nesáhne,
- obrázek `#v` se ukáže jen tehdy, když neběží proud z C++; canvas je ve stránce nad ním,
- hlídač při každé kontrole stavu (1× za s): dokud běží proud z C++, nic přes něj nesmí ležet,
- při přepnutí hry (jádro chvíli stojí) zůstane 2,5 s poslední obraz – žádné probliknutí
  na telefon a telefon zbytečně nerozjíždí kodér H.264.

**Ověřeno:** scénář podle tvého logu (menu → PS1 → přepnutí hry → menu → Sega → PS1 → F5 →
menu) se stránkou B309: **16 kontrol, 0 chyb**, nový obraz do 0,2–1,3 s; tentýž scénář se
stránkou B308: **6 chyb** – přesně tvoje chyba (`#v` navrchu, canvas s novým obrazem pod ním).

## 2. PS1: ULOŽIT LOG A ODESLAT

- Jako v HELP: jedno klepnutí → log (stránka + deník PS1 + log appky) se uloží do
  `Downloads/AtariHelp` a **hned se otevře nabídka Poslat log** (Messenger, mail …); log je
  i ve schránce.
- Na výšku: kolečko → **ULOŽIT LOG A ODESLAT**. Na šířku: kolečko → tlačítko je rovnou
  v panelu D-PAD (ne až přes DALŠÍ).
- Ten druhý soubor, co jsi poslal (`orca_share_media…`), je opravdu můj log PS1 – je v něm
  oddíl DENIK PS1.

## 3. Úvodní demo 59 s

| čas | co je na obrazovce |
|---|---|
| 0–32,8 s | beze změny (garáž, mraky, blesk, Atari, pruty, trojice N&P, průlet ke CRT) |
| 32,8–38,4 s | **Pitt-Kitt Platinum**: titulek N&P, kapitola Las Vegas 1982, **jízda 3,4 s se zvukem hry** |
| 38,4–44,4 s | **16 BIT**: loď, drony v 5 vlnách, moře se sluncem – celá hra v 72 % tempa |
| 44,4–50,65 s | **32 BIT**: noční závod – předjetí modrého (≈2 s) a žlutého auta (≈4,4 s), 3. → 2. → 1. místo, 166–190 km/h (dřív 212+) |
| 50,65–59 s | záblesk, zlaté logo a titulky (stejné jako dřív, jen o 14 s později) |

- Každá ukázka teď trvá ~6 s (dřív 1,2–1,45 s) a má **vlastní hudbu** v tempu 120 BPM,
  střihy na začátku taktu: Atari – tichý „POKEY“ doprovod pod titulkem a kapitolou, pak hudba
  tvé hry (teď stejně hlasitá jako zbytek – byla o 10 dB tišší); 16 bit – FM skladba (basa,
  bicí, plechy, hrdinská melodie); 32 bit – noční jízda (breakbeat, basa, plochy Dm9 – Bbmaj7 –
  C – Dm – A, zvonky, svist při předjetí).
- Kamera se během defilé pomalu přenese zleva doprava a přiblíží – záběr není mrtvý.
- Nový klip z tvé hry nahrálo naše jádro Atari z tvého XEX: 316 snímků (titulek 80, kapitola 58,
  jízda 178), 253 kB. Jízda končí na snímku 615 – hned potom hra přepne pohled a vynuluje body.
- SKIP funguje celou dobu (do 58,6 s).

K „bičíku“: obě ukázky (16 bit střílečka, 32 bit závod) jsou naše vlastní – žádná cizí
grafika ani hra. Jestli z nich jednou budou hry pro Segu a PS1, základ (grafika, pravidla,
hudba) už je v C++.

## 4. Doom – co ukázal tvůj log

- Oprava u tebe opravdu pracuje: ~33 000 opravených tenkých trojúhelníků za sekundu.
- V těžkých scénách je S8 na hraně: krok emulace 14–20 ms z 20 ms (PAL 50 Hz), rezerva
  5–30 %; několikrát emulace spadla na 46–48 z 50 kroků/s a zvuk vypadával (až 95 výpadků
  za 10 s – krátké lupnutí). Bylo to při zapnutém přenosu na TV.
- Návrh na další krok (v B309 jsem to nedělal – nežádal jsi o to a je potřeba to vyzkoušet
  na telefonu): kreslení grafiky PS1 do vlastního vlákna – PCSX-ReARMed to umí
  („Threaded Rendering“, v RetroArch je výchozí zapnuté), u nás je zatím vypnuté. Pomohlo by
  všem 3D hrám. Napiš, jestli to chceš.

## 5. Drobnosti

- Deník PS1: start BIOSu bez disku už se nepíše jako „JADRO CHYBA“.
- Pojistka dema v Javě 80 s (demo 59 s + až 7 s příprava).

## Ověřeno na PC (stejný kód jako v telefonu)

- **TV / PC bez F5** (Chromium, cesta H.264 jako u tebe, skutečný proud PS1 a Segy z C++):
  **16 kontrol, 0 chyb**; tentýž scénář se stránkou B308: 6 chyb (tvoje chyba).
- **PS1 ULOŽIT LOG A ODESLAT** (Chromium, na výšku i na šířku): **16 kontrol, 0 chyb**.
- **Intro celé po 0,25 s** na výšku i na šířku: 2 × 237 snímků, **0 chyb OpenGL**.
- **Zvuk dema 59,6 s**: spočítaný za 0,8 s, nic neořezává; jízda Pitt-Kitt teď −18 až −21 dBFS
  (dřív −26 až −29), zbytek defilé −14 až −17.
- **Tlačítko SKIP**: 20/20 (i to, že po 58,6 s už není).
- **Skutečný `nap_intro_gl.cpp`**: 11/11 – celá stopa 59,6 s do reproduktoru bit po bitu, zvuk hry
  35,0–38,4 s, demo samo doběhne, bez klipu i bez OpenSL jede dál.
- **Demo na TV / PC** v Chromiu: 12/12 – Pitt-Kitt, 16 bit i 32 bit jsou na TV vidět, zvuk
  v paketech bit po bitu = stopa dema.
- **16bit ukázka**: 1,1 ms na snímek (PC), drony a výbuchy až do konce 6 s.
- **Klip** jsem nahrál znovu z tvého XEX – bajt po bajtu stejný jako v appce.
- **Regrese**: PS1 (Doom 91/0, deník 63/0, ThreadSanitizer 0, JVM 23/23, klávesnice z TV 17/0,
  stránka PS1 23/0), Atari v prohlížeči 23/0, Sega / PS1 / demo proudem se zvukem 37/0.

Postup: `test_overeni/b309/JAK_SPUSTIT.txt`, výstupy `test_overeni/b309/VYSTUP_*`.

**Co jsem ověřit nemohl:** skutečný Chrome na TV / PC, který opravdu dekóduje H.264
(test pouští stejnou logiku stránky s falešným JMuxerem), nabídku „Poslat log“ v Androidu
(stejný kód jako HELP, který ti funguje) a jak nová hudba zní – je spočítaná a změřená
(hlasitost, žádné ořezání, na TV bit po bitu stejná), poslech je na tobě.

## Log

- TV / PC: `B306 … PROUD start / konec`, `B309 HLIDAC: proud … byl schovaný – opraveno`
  (kdyby se to ještě někde stalo, hlídač to opraví sám a zapíše).
- PS1: `B309 PS1_LOG_ODESLAT …` (kam se log uložil).
- Demo: `B304 DEMO_KONEC … cas dema=59000 ms`.
