# B140 — ŽIVÁ JÁDRA V INTRU (versionCode 188)

To, kvůli čemu to celé bylo. **Intro na dvou místech spustí skutečné jádro.**

```
 0,0 –  2,5 s   televize se zapíná
 2,5 – 31,5 s   ATARI    READY a píše se program
31,5 – 38,0 s   SEGA     ← SKUTEČNÉ JÁDRO se Sonicem z telefonu
38,0 – 44,5 s   PS1      ← SKUTEČNÝ BIOS bez disku
44,5 – 46,5 s   skok
46,5 – 66,5 s   finále   logo a běžící program
66,5 – 70,5 s   konec
```

**Nic se nepřehrává.** Jádro Segy si obrazovku i znělku spočítá z YM2612
a PSG té konkrétní hry, PS1 si logo a zvuk spočítá z BIOSu. Přesně jak jsi
to sám popsal — a přesně jako když si hru pustíš ručně.

## Nešel jsem zkratkou

Podíval jsem se, jak se hra v Seze spouští doopravdy, a **jdu tím samým
řetězem**:

```java
stopNativeCoreAudioStream()      zastav starý zvuk
NativeSegaCoreBridge.shutdown()  tvrdý reset jádra
realCoreLoadRom(data)            nahraj ROM
segaPlochaZapni()                plocha přes OpenGL
forceNativeViewRedrawBurst()     rozjeď kreslení
scheduleNativeAudioAfterFrameAndViewDraw()
   → čeká na frameReady A viewReady
   → teprve pak spustí zvuk
```

V kódu je u toho i důvod, proč ten pořádek existuje: *„Tvrdý fresh start
před každou ROM brání stavu: Atari 130XE → Sega → nová ROM → černá
obrazovka + zvuk."* Kdybych si to zkrátil po svém, vyrobil bych přesně
ten problém.

Proto ta část trvá 6,5 s a ne 3 — jak jsme se domluvili.

## Když soubory na telefonu nejsou

Nic se nerozbije. Ta část se **nahradí vlastní znělkou a nápisem**
(`MEGA DRIVE`, `PLAYSTATION`) a film jede dál.

## PŘESKOČIT vypne i jádra

Kdyby zůstalo běžet jádro Segy po přeskočení intra, hrálo by dál přes
celou aplikaci. Proto `PŘESKOČIT` volá `zastavSegu()` — ověřeno spuštěním.

## Ověřeno u mě

Intro jsem **pustil** proti napodobenině a sledoval, co volá do Javy:

```
po 40 s:          spustSegu, zastavSegu, spustPs1
po PŘESKOČENÍ:    ... + hotovo, přesměrováno
```

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| volání do Javy ve správném pořadí | ověřeno spuštěním |
| Atari, PS1, Sega — jádra | **nesaháno** |

**A jedna chyba, kterou jsem si u toho našel:** moje první náhrada časové
osy se netrefila do souboru — funkce se přidaly, ale osa zůstala stará
a scény se nikdy nezavolaly. Test to odhalil. Kdybych se spolehl na to,
že „to tam přece je", poslal bych ti build, kde se ta jádra vůbec nepustí.

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Ujisti se, že máš stažené soubory (`Download/AtariHelp/emu`).
**3)** Spusť aplikaci a **nech film běžet celý**.
**4)** V 31. vteřině má naskočit **skutečná Sega se Sonicem** — obrazovka
       i znělka.
**5)** V 38. vteřině **skutečný BIOS PS1** — logo a zvuk Sony.
**6)** Zkus **PŘESKOČIT** během Segy — musí zmlknout a skočit do menu.
**7)** Smaž Sonica a spusť znovu — místo Segy má být můj nápis a znělka.
**8)** Atari, PS1, Sega, WEB TV — jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 4 | Sega naskočí i se zvukem | černo, ticho, seká |
| 5 | PS1 logo a zvuk | nic |
| 6 | zmlkne a skočí do menu | hraje dál |
| 7 | náhradní znělka | černo nebo pád |
| 8 | jako v B131 | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA30 INTRO_SEGA start Sonic The Hedgehog (USA, Europe).gen (524288 B)
BUILD2SA30 INTRO_SEGA rom nahrana: ...
BUILD2SA30 INTRO_SEGA konec
BUILD2SA30 INTRO_PS1 spoustim BIOS
```
