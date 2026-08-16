# B122 — ZVUK JEN OD TOHO, KDO HRAJE (versionCode 170)

> **Tento kód je předpoklad. Čeká se na test.**

## Co jsem našel v tvém logu

Řádek `TV_WEB_AUDIO_SOURCE` se zapisuje **jen když se zdroj změní**.
Projel jsem celý log a v celých šesti minutách **není jediné místo, kde
by šly dva stejné zdroje po sobě**:

```
17:34:49.085  SEGA  hz=48000
17:34:49.099  PS1   hz=44100
17:34:49.102  SEGA  hz=48000
17:34:49.190  PS1   hz=44100
```

**PS1 posílalo zvuk na televizi i když hrál Sonic.** Potom i když hrálo
Atari. Naposledy v 17:40:30 — čtyři a půl minuty po tom, co se v 17:36:04
zapsalo `PS1_SESSION_STOP core=PS1_STOPPED`.

## Proč z toho byla němá televize

V `napTvWebAudioPush` je tohle:

```java
if (sampleRate > 8000 && sampleRate != napTvWebAudioRate) {
    napTvWebAudioRate = sampleRate;
    napTvWebAudioSeq = 0;        // vynuluje počítadlo
}
```

Sega má 48000, PS1 má 44100. Střídaly se dávku po dávce, takže se
počítadlo nulovalo několikrát za vteřinu.

Prohlížeč si pamatuje, kde skončil, a ptá se `after=<velké číslo>`.
Server má `end=0`, podmínka `after <= end` neprojde — a pošle
**prázdnou odpověď**.

Proto v telefonu zvuk hrál a na webu bylo ticho.
A proto to u samotné PS1 fungovalo: tam žádný druhý zdroj nepřekáží.

## Ta samá past už je v kódu popsaná — u obrazu

V `napTvWebCaptureFromCore` je tvoje poznámka:

> *Jenže jádro běží dál i potom, co uživatel z PS1 odejde do Atari nebo
> Segy — takže si TV pořád brala jeho snímek a zůstala viset na posledním
> obrázku z PlayStation.*

U obrazu se to vyřešilo otázkou „je PS1 opravdu na obrazovce?".
**U zvuku se ta otázka nikdy nepoložila.**

## Co jsem změnil

Jedno místo, řádek 793. Odbočka se teď ptá stejně jako obraz:

```java
boolean ps1JeNaObrazovce = (uZvuk != null)
        && uZvuk.contains("emu_ps1")
        && (ps1SessionActive || ps1BiosRunning || ps1GameWindowOwnsCore);
...
if (got > 0 && ps1JeNaObrazovce) napTvWebAudioPush(...);
```

Vyzvedávání z jádra běží **pořád** — jinak by odbočka přetékala a po
návratu do PS1 by na TV šlo pár vteřin starého zvuku. Mění se jen to,
kdy se dávka **pošle**.

Do Sega vlákna, do C++, do assets ani do HTML/CSS jsem nesáhl.
Proti B121 se liší **jen dva soubory**: `MainActivity.java` a `build.gradle`.

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Java | `javac` proti android.jar API 34 | 0 chyb, 37 tříd |
| PS1 C++ | `clang` aarch64 + pravé NDK hlavičky | 0 chyb |
| Sega C++ | `clang` aarch64 + clownmdemu | 0 chyb |
| linkování | 37 native metod vs. symboly v .o | 37/37 sedí |
| JS stránky | `node --check` | 0 chyb |
| zvuková cesta | skutečný JS stránky v node proti serveru | viz níže |

Přepsal jsem server (`napTvWebAudioPush` + `napTvWebWriteAudioRaw`) do
node a pustil proti němu **skutečný JavaScript vytažený ze stránky**.
20 vteřin běhu:

| | prázdných odpovědí | resetů počítadla | dorazilo zvuku |
|---|---|---|---|
| před opravou | 870 | 2000 | **0,1 %** |
| po opravě | 0 | 1 | **99,9 %** |

## Dvě hypotézy z minulého balíčku můžeš škrtnout

1. **Zásobník přetéká** — ne. 1 MB je při 48 kHz přes 2,7 vteřiny,
   prohlížeč se ptá po 20 ms.
2. **Klient čeká 44100** — ne. Čte si frekvenci z hlavičky a strká ji
   rovnou do `ac.createBuffer(2, frames, rate)`. 48000 mu nevadí.
   Ověřeno spuštěním.

---

## CO TESTOVAT

**1)** Zapni TV cast. Spusť **Segu** (Sonic). Poslouchej **televizi**.
**2)** U té samé Segy poslouchej **telefon**.
**3)** Vrať se do **PS1**, spusť hru. Poslouchej **televizi i telefon**.
**4)** Spusť **Atari**. Poslouchej **televizi**.

## CO OČEKÁVAT

| krok | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | z televize jde zvuk Segy | ticho |
| 2 | jako dosud, čistý, nepraská | praská / kouše se |
| 3 | obojí hraje jako v B121 | ticho nebo trhání |
| 4 | z televize jde zvuk Atari | ticho |

Krok 3 je ten důležitý — tam se pozná, jestli jsem ti nerozbil PS1.
Krok 4 by měl fungovat jako vedlejší efekt, i když jsem se Atari nedotkl.

## CO POSLAT ZPĚT

Jen tyhle dvě věci:

**a)** Čtyři odpovědi podle tabulky (stačí „1 dobrý, 2 dobrý, 3 dobrý, 4 ticho").

**b)** Z logu (`http://IP:8765/log`) tenhle jeden příkaz a jeho výsledek —
nebo mi rovnou pošli celý log:

```
grep 'TV_WEB_AUDIO_SOURCE' log | awk '{print $NF}' | uniq -c
```

Chci vidět, jestli u Segy zmizelo to střídání. **Správně** vypadá takhle
(dlouhé série jednoho zdroje):

```
   1 PS1
 412 SEGA
   1 PS1
 380 ATARI
```

**Špatně** vypadá takhle (u každého řádku jednička):

```
   1 SEGA
   1 PS1
   1 SEGA
   1 PS1
```

---

## Co jsem NEDODĚLAL — ať to víš

Opravil jsem, že se zvuk z PS1 **neposílá**, když PS1 nehraje.
Nevyřešil jsem otázku, **proč jádro PS1 vůbec ještě 4,5 minuty po
`PS1_SESSION_STOP` nějaký zvuk vydává.**

Můžou to být tři věci a nerozliším je bez dalšího logu:
- jádro opravdu běží dál (a pak bere procesor Atari — což by sedělo
  s tím, že ti Atari hry na mobilu zpomalily)
- neuklízí se ukazatele `g_tvring_r` / `g_tvring_w` v `nap_ps1_native.cpp`
- souběh v `ps1PullTvAudio`: čtenář načte `r`, pisař ho mezitím posune,
  čtenář pak uloží svoje starší `r` a vrátí ho zpátky

Na chování téhle opravy to nemá vliv — ale je to otevřené a zapsal jsem
to do předávacího balíčku jako první věc, co vzít po B122.
