# Co je v B308

Tvoje zprávy: „V PS1 emu … překrývá se nám tam kanfas na nahrávání her – nemáme ovládání
přes klávesnici na TV viewer – doom hra má špatně grafiku – je potřeba nachystat log do
emu ps1, abys nebyl slepej“ a pak: „Problém je čistě v DOOMovi, kde se vůbec neukazuje
strop a celkově zlobí grafika … prostuduj kód z GitHubu a oprav naše jádro emulátoru tak,
aby ty stropy fungovaly.“

Měnilo se jen **PS1** (jádro, stránka PS1, klávesnice z TV / PC pro PS1) a body testu
v HELP. Atari 130XE (hlavní nabídka i C++ v HELP), Sega a úvodní demo se neměnily.

## 1. DOOM – strop (chyba byla v našem jádře, opravená)

**Kde:** grafický čip našeho PS1 jádra (gpu_neon z PCSX-ReARMed) ve **2x rozlišení**
(Enhanced Resolution). To je u nás zapnuté pořád – kvůli němu je 3D ostré.

**Co dělá Doom** (vyčteno z kódu PSX Doomu na GitHubu): podlahu a strop kreslí **po
řádcích** – každý řádek je trojúhelník vysoký 1 bod: (L,y) (R,y) (R,y+1). Zdi kreslí **po
sloupcích** širokých 1 bod. Původní PS1 z takového „proužku“ nakreslí celý řádek.

**Proč to bylo rozbité:** ve 2x je z jednoho řádku řádek dvojitý a najednou se ukáže
skutečný tvar toho proužku – trojúhelník: horní řádek celý, spodní jen pravá půlka.
Ve stropu a podlaze tak chyběla **čtvrtina bodů** (černé zubaté pruhy, přes ně prosvítalo,
co tam bylo dřív) a zdi měly svislé skvírky.

**Oprava** (`psx_gpu_parse.c`, `do_thin_triangle_enhanced`): takový tenký trojúhelník
nakreslím ve 2x **po obou řádcích (sloupcích) celý** – jako 2× větší bod původního PS1 –
a textura se dál počítá v plném 2x rozlišení. Jen pro trojúhelníky s texturou, která se
napříč tenkým směrem nemění (přesně podpis Doomu). Tenké kousky 3D sítí jiných her jdou
postaru, aby se jejich sítě ve 2x dál spojovaly bez mezer.

Změřeno na skutečném kódu grafického čipu z appky (scéna jako z PSX Doomu, 2x):

| | bez opravy (B307) | s opravou (B308) |
|---|---|---|
| strop v místnosti | **25,6 % díry**, 30,7 % bodů jinak než PS1 | **0 % díry**, 6,4 % jinak¹ |
| podlaha | **26,0 % díry**, 29,2 % jinak | **0 % díry**, 4,6 % jinak¹ |
| zdi | **7,8 % díry**, 33,3 % jinak | **0 % díry**, 8,2 % jinak¹ |
| postavy, stavový řádek, nebe | – | beze změny |

¹ zbytek je jemnější textura ve 2x (víc detailů než původní PS1), ne chyba.

- **Ostatní hry:** scéna s běžnými trojúhelníky a 1 685 tenkými kousky 3D sítí jiných her
  vychází s opravou i bez ní **bit po bitu stejně**. V původním rozlišení (bez 2x) se
  nemění nic.
- **Telefon:** přeloženo i pro ARM64 s NEON (jako v telefonu, přes qemu) – obraz bit po
  bitu stejný jako na PC.
- **Rychlost:** kreslení Doomu stojí o ~1/3 víc (na PC 1,5 → 2,0 ms na snímek včetně
  původního 1x obrazu) – pořád hluboko pod 1/60 s. Jak to vypadá v telefonu, ukáže deník
  (řádek ZDRAVI, viz níže).

## 2. Deník PS1 (log) – v C++

Jádro samo zapisuje, co se děje, s časem na milisekundy.

**Kde ho najdeš:**
- **PS1 → kolečko → ULOZIT LOG** (na šířku: kolečko → DALŠÍ (BIOS, CD, LOG...) → ULOZIT
  LOG) nebo **KOPIROVAT LOG**. Za řádky stránky je oddíl `DENIK PS1 (C++, jadro)`.
- **HELP → ULOŽIT LOG A ODESLAT** – celý log appky + deník PS1.
- **TV / PC: /log** a soubor relace (i „soubor z minula“ po pádu) – řádky PS1 jsou tam taky.

**Co v něm je** (ukázka z testu na PC – v telefonu budou čísla tvoje):

```
PS1 ==================== PS1 START: HRA ====================
PS1 HRA: Doom.cue | typ .cue | 243 B | cesta …
PS1 CUE: 3 stop (data 1, hudba 2), souboru 3, na disku 2 (4.0 MB): … Doom (Track 3).bin CHYBI
PS1 CHYBA CUE: 1 soubor(u) chybi -> hra nepujde nebo bez hudby: Doom (Track 3).bin
PS1 BIOS ve slozce appky: scph5501.bin 512 kB
PS1 JADRO: BIOS scph5501.bin (soubor) | region USA, NTSC 60 Hz | procesor dynarec (rychly) | disk ID "SLUS00077" nazev "DOOM" | …
PS1 NASTAVENI GRAFIKY: 2x (Enhanced Resolution) ZAPNUTO, …
PS1 PAMETOVA KARTA nactena: 131072/131072 B z …
PS1 start trval 53 ms
PS1 OBRAZ: obraz 320x240, 15 bit, NTSC | 2x ANO
PS1 TV KLAVESNICE: KeyK dolu -> KRIZEK
PS1 TLACITKA: -NAHORU +NAHORU -> drzeno: NAHORU
PS1 ZDRAVI 15 s: emulace 60.0 kroku/s (cil 59.94, rezerva 99 % casu) | retro_run prumer 0.1 ms, nejdele 1.1 ms | snimky 60.0/s | zvuk fronta 86 ms, vypadky 0 | obraz 320x240, 15 bit, NTSC | 2x ANO | oprava Doomu: 25234 tenkych trojuhelniku ve 2x | tlacitka nic …
PS1 ==================== PS1 KONEC po 31 s, snimku 1850 ====================
```

- **START / HRA / CUE** – jaký soubor, jestli jsou všechny stopy (.bin) na disku (chybějící
  stopa = hra nejde nebo hraje bez hudby).
- **BIOS / JADRO** – který BIOS, oblast, PAL / NTSC, ID a název disku, hlášky jádra.
- **OBRAZ** – každá změna rozlišení (menu, video 24 bit, hra), jestli běží 2x a proč ne.
- **ZDRAVI** každých 5 s (po první minutě 10 s) – rychlost emulace a rezerva výkonu,
  snímky, zvuk (fronta, výpadky), stav obrazu, **oprava Doomu: N** – kolik tenkých
  trojúhelníků se kreslilo opraveně. Když u Doomu bude N = 0, kreslí hra jinak, než
  předpokládám – a uvidím to.
- **TLACITKA / TV KLAVESNICE** – co se zmáčklo a pustilo, co je drženo.
- Pozice (SAVE / LOAD), paměťová karta, chyby startu, konec.
- Aby log nerostl donekonečna: v paměti posledních 800 řádků, hlášky jádra jen prvních
  150 (varování a chyby až 600), tlačítka a obraz omezené za každých 5 s.

## 3. Nahrávání hry – „překrývá se kanfas“

V kódu jsem našel tři příčiny:

1. **Na výšku** byl banner stahování (Google Drive) nahoře – zakrýval nadpis a lezl do
   monitoru, kde je obraz z jádra NAD stránkou: půl banneru bylo vidět, půl pod obrazem.
2. **Nahrávací okno** mělo pevný odpočet 10 s nezávislý na tom, co appka dělala. Na výšku
   leželo POD obrazem z jádra (v monitoru visel starý obraz BIOSu), na šířku 10 s
   zakrývalo už běžící hru, nebo zmizelo dřív, než hra naběhla.
3. Appka hru před startem **kopírovala na hlavním vlákně** (stránka zamrzla, hra zabírala
   místo dvakrát) a hlásila „hotovo“ dřív, než jádro hru opravdu spustilo.

**Teď:**
- Nahrávací okno je v monitoru a obraz z jádra se na tu dobu schová (na výšku i na
  šířku). Vteřiny běží **nahoru** (kolik už start trvá; od 15 s poznámka „velká hra nebo
  pomalé úložiště“). Okno zmizí, **až jádro hru opravdu spustí**, a hned je vidět hra.
  Když se hra nespustí, okno zmizí a toast napíše proč. Platí pro knihovnu (CD / ISO),
  výběr souboru, Google Drive i RESET (BIOS).
- Hra se už **nekopíruje** – jádro čte přímo z knihovny, start běží na pozadí. Staré
  kopie her z dřívějších verzí se samy smažou (uvolní místo).
- Banner stahování je na výšku **pod monitorem** (přes obrázek ovladače – na výšku se
  nehraje), na šířku zůstal nahoře.

Kdyby se ještě něco překrývalo, napiš mi **přesně co přes co** a jestli na výšku nebo na
šířku – opravil jsem, co jsem v kódu našel, ale tvou obrazovku nevidím.

## 4. TV / PC – klávesnice počítače i pro PS1

| klávesa na PC | PS1 |
|---|---|
| šipky, WASD, numerická 8 / 4 / 6 / 2 | směr |
| K, mezerník | KŘÍŽEK |
| L | KOLEČKO |
| J | ČTVEREC |
| I | TROJÚHELNÍK |
| Q / E | L1 / R1 |
| Z / C | L2 / R2 |
| F / G | L3 / R3 |
| Enter, P | START |
| Backspace, O | SELECT |

- Klávesa jde z prohlížeče přes Javu (jen přepošle) **rovnou do C++**, které ji drží jako
  vlastní tlačítka vedle dotyku – dotyk v telefonu a klávesnice fungují najednou.
- Když okno prohlížeče ztratí zaměření (přepneš jinam), všechno se pustí. Když se
  prohlížeč odmlčí (Wi-Fi), C++ samo pustí držené klávesy po 2,5 s – nic nezůstane
  „zaseknuté“.
- Nápověda je v prohlížeči vpravo dole, za ní poslední tlačítko, např. `[KRIZEK]`.

## 5. Další opravy v PS1 (našel jsem při procházení kódu)

- **SAVE GAME** (uložení pozice) se mohlo trefit doprostřed kroku emulace z jiného
  vlákna → rozbitá pozice nebo pád. Teď zámek (v testu 25 souběhů → 0).
- **Paměťová karta** se uloží i při přepnutí rovnou na jinou hru (nebo z BIOSu do hry) –
  dřív jen při zavření PS1. Karta je společná pro BIOS i všechny hry.
- **Zvuk PS1:** při otevření zvukové karty se nejdřív spustilo přehrávání a teprve pak
  naplnila fronta – zvuková karta si mohla sáhnout na nepřipravená data. Teď naopak.
- Start a stop PS1 už nemůžou běžet přes sebe (zámek).
- Stav PS1 v logu ukazoval `running=NO`, i když hra běžela – teď skutečný stav.

## Ověřeno na PC (skutečný kód z appky)

- **Doom** – skutečný grafický čip appky (gpulib + gpu_neon), s opravou i bez ní, x86
  i ARM64 NEON: **91 kontrol, 0 chyb** (čísla v tabulce nahoře).
- **Deník + klávesnice + zámek** – skutečný `nap_ps1_native.cpp` (místo PS1 hry náhrada,
  která mění obraz, píše hlášky jádra a má paměťovou kartu): **63 kontrol, 0 chyb**.
- **ThreadSanitizer** – vlákno emulace, zvuk, 2 vlákna prohlížeče s klávesami, deník,
  SAVE / LOAD, dotyk – 7 s souběžně: **0 hlášení**.
- **JVM** – skutečná třída `NativePs1CoreBridge` se skutečným C++ (`-Xcheck:jni`): nové
  metody i obaly sedí; všech **23 native metod** má v knihovně svou funkci.
- **Prohlížeč (Chromium)** – stránka TV / PC přesně z appky, klávesnice PC → PS1:
  **17 kontrol, 0 chyb**.
- **Stránka PS1 (Chromium)** – nahrávací okno podle skutečného stavu, banner, LOG
  s deníkem: **23 kontrol, 0 chyb**.
- **Regrese Atari 130XE** v prohlížeči se stránkou B308: **23 kontrol, 0 chyb**.

Postup: `test_overeni/b308/JAK_SPUSTIT.txt`, výstupy `test_overeni/b308/VYSTUP_*`.

**Co jsem ověřit nemohl:** skutečný telefon (rychlost s opravou, zvuk) a skutečný disk
Doomu – scény jsou postavené podle kódu PSX Doomu, ne nahrané z tvé hry. Proto deník:
řádky ZDRAVI a OBRAZ z tvého telefonu mi řeknou, jestli oprava u tebe opravdu pracuje.

## Log – na co se podívám

- `PS1 START`, `HRA`, `CUE`, `JADRO`, `OBRAZ`, `ZDRAVI` (… `oprava Doomu: N` …),
  `TLACITKA`, `TV KLAVESNICE`, `PS1 KONEC`, `CHYBA …`
- ze stránky PS1: `NAHRAVANI_START`, `NAHRAVANI_KONEC`

Když strop v Doomu pořád chybí, napiš ve kterém levelu a pošli LOG z PS1 (kolečko →
ULOZIT LOG) – nejlíp hned po tom, co se na strop podíváš.
