# B121 — ZVUK SEGY NA TV (versionCode 169)

> **Tento kód je předpoklad. Čeká se na test.**

## Proč zvuk Segy nešel na web

Našel jsem to v pořadí operací:

```java
track.write(pcm, ..., AudioTrack.WRITE_BLOCKING);   // CEKA, az se prehraje
...
napTvWebAudioPush(pcm, ..., "SEGA");                 // az POTOM na TV
```

`WRITE_BLOCKING` **čeká, dokud se zvuk v telefonu nepřehraje** — a to trvá
přesně tak dlouho, jak je dávka dlouhá. Na televizi se posílalo až potom,
takže tam zvuk přicházel pozdě a zahazoval se.

**V telefonu přitom hrál normálně** — proto to vypadalo, že zvuk funguje,
jen na webu ne. Přesně jak jsi to popsal.

Teď se dávka posílá na TV **hned, ještě před přehráním**.

## Kdyby to nestačilo

Zbývají dvě věci, které by to mohly být — obě jsou zapsané v předávacím
balíčku pro dalšího:

1. `napTvWebAudioPush` přepisuje kruhový zásobník rychleji, než si ho
   prohlížeč stíhá brát
2. Sega posílá 48000 Hz, ale klient v prohlížeči může čekat 44100 (PS1)

## Ověřeno

| část | čím | výsledek |
|---|---|---|
| Sega C++ | `clang` pro aarch64 | 0 chyb |
| PS1 C++ | `clang` pro aarch64 | 0 chyb |
| Java | `javac` proti `android.jar` API 34 | 0 chyb |

---

## CO TESTOVAT

**1)** **Sega — zvuk na TV.** Hraje?
**2)** **Sega — zvuk na mobilu.** Nezhoršil se? Nepraská?
**3)** **PS1** — kontrola, že jsem nic nerozbil.

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | zvuk na TV hraje | ticho |
| 2 | jako dosud, čistý | praská / kouše se |
| 3 | jako v B118 | zhoršilo se |

## CO POSLAT ZPĚT

Hraje zvuk Segy na TV? A nepraská v telefonu?
