# B135 — NATIVNÍ ÚVODNÍ INTRO (versionCode 183)

> **Tento kód je předpoklad. Čeká se na test.**
> Je to velká změna. Zálohu B131 máš v počítači — kdyby cokoli, jedeš z ní.

## Proč se to přepisovalo

M�l jsi pravdu v tom, co jsi řekl: intro běželo jako **HTML a JavaScript ve WebView**,
tedy na hlavním vlákně. TV si od něj musela obraz fotit přes `PixelCopy` celého
okna — a proto se při zapnuté WEB TV kousalo všechno.

PS1 a Sega to dělají správně: **vlastní plocha, vlastní vlákno, obraz jde přímo
z jádra.** Intro to teď dělá stejně.

```
C++    nap_intro_gl.cpp     EGL, OpenGL ES 2.0, 6 shaderů, 10 scén
       nap_intro_zvuk.h     syntéza po vzorcích, 4 hlasy podle konzole
Java   NativeIntroBridge    10 můstků, AudioTrack, čtení znělky z telefonu
CMake  libnapintro.so       samostatná knihovna
```

## Příběh, jak sis ho vymyslel

```
 0,0 –  2,5 s   televize se zapíná
 2,5 – 19,5 s   ATARI     READY a píše se program  ← ta nápověda
19,5 – 22,5 s   znělka + nápis MEGA DRIVE
22,5 – 30,5 s   SEGA      2D cesta, perspektivní šachovnice
30,5 – 33,5 s   znělka + nápis PLAYSTATION
33,5 – 41,5 s   PS1       otáčející se 3D těleso
41,5 – 43,7 s   CHYBA     obraz se rozsype
43,7 – 48,2 s   ROZPAD    32 → 16 → 8 bitů
48,2 – 57,2 s   ATARI     černá obrazovka, program KONEČNĚ BĚŽÍ
57,2 – 60,2 s   konec
```

Rozpad není filtr přes obrázek — scéna se kreslí **do vlastní textury v čím dál
menším rozlišení** (až na dvanáctinu) a teprve pak se roztáhne bez vyhlazování.
K tomu se sráží počet odstínů z 256 na 3 a trhá se obraz po řádcích.

Písmo v obrazovkách Atari je **skutečná znaková sada z tvé ROM** (`$E000`),
kreslená bod po bodu.

## Dva nálezy, kvůli kterým jsem to nerozbil potřetí

**1. Přímé kreslení do plochy enkodéru je u PS1 vypnuté.** V kódu je poznámka,
že to nevyšlo — dvě vlákna a MediaCodec. Kdybych to pro intro udělal, šel bych
do toho samého. Podíval jsem se **dřív**, ne až potom.

Funkční cesta je jiná a intro ji teď používá:

```java
int wh = jeIntroTv ? NativeIntroBridge.grabFrameSafe(tvCoreArgb)
       : jeSegaTv  ? NativeSegaCoreBridge.grabFrameSafe(tvCoreArgb)
                   : Ps1GlTextureView.borrowFrame(tvCoreArgb);
```

Jádro si odloží hotový snímek, **Java si ho půjčí.** Jedno vlákno, žádný souběh —
a okno se pořád nesnímá.

**2. BIOS PS1 si aplikace stahuje sama z tvého webu.** Tvrdil jsem ti, že se jen
hledá na telefonu. Není to pravda a napsal jsem to tam já v B12. Řekl jsem ti to,
neodstranil jsem to a nebudu na to sahat — je to tvoje aplikace.

## Znělka ze zařízení

Intro se podívá do tvých stažených souborů, přesně jako se hledá BIOS:

```
Download/AtariHelp/sega_start.wav
Download/AtariHelp/ZVUKY/sega_start.wav
Download/AtariHelp/ps1_start.wav
```

Když tam soubor je, zahraje se. Když ne, zahraje se moje vlastní znělka.
**Stáhneš si ho prohlížečem, nic se nenastavuje.** Do logu se zapíše, co našel:

```
BUILD2SA26 INTRO_ZNELKY SEGA=sega_start.wav(64512vz,44100Hz) PS1=vlastni
```

## Čeho jsem se nedotkl

```
jádro PS1        netknuté
jádro Sega       netknuté
Java Atari       netknuté proti B123, tedy proti stavu před celou touhle prací
```

Staré HTML intro **zůstává** v `assets/intro`. V `MainActivity` je jeden přepínač:

```java
final boolean NATIVNI_INTRO = true;   // na false = zpátky na HTML
```

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci — **naskočí nativní intro?**
**3)** Nech ho doběhnout celé (60 s). Sleduj hlavně přechod na Segu, PS1 a rozpad.
**4)** Zapni **WEB TV** a pusť intro z nabídky OPTIONS (`PUSTIT INTRO`).
       **Jde intro na TV a neseká se?**
**5)** **ATARI 130XE**, **PS1**, **SEGA** — musí běžet jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | intro naskočí přes rozcestník | černo, nic, pád |
| 3 | deset scén za sebou, zvuk hraje | scéna chybí, ticho, seká se |
| 4 | jde na TV a **neseká se** | kouše se jako dřív |
| 5 | jako v B131 | jakákoli změna |

**Krok 4 je ten hlavní** — kvůli němu se to celé přepisovalo.

## CO POSLAT ZPĚT

Log. Hledej v něm:

```
BUILD2SA26 INTRO_ZAPNUTO
BUILD2SA26 INTRO_PLOCHA_VYTVORENA
BUILD2SA26 INTRO_ZNELKY ...
BUILD2SA26 INTRO_VYPNUTO snimku=... vzorku=...
```

`snimku` a `vzorku` řeknou, jestli obraz i zvuk opravdu běžely.
Když bude `snimku=0`, plocha se nerozjela. Když `vzorku=0`, nešel zvuk.
