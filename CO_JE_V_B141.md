# B141 — SEGA JE VIDĚT (versionCode 189)

Opravuje body 1, 2, 3 a 5. Bod 4 (TV) je samostatný krok — viz konec.

## Proč Sega vůbec nechytla

Z tvého logu to bylo vidět přesně:

```
BUILD2SA30 INTRO_SEGA rom nahrana: REAL_CORE_THREAD_START_OK
SEGA_PLOCHA_VYTVORENA 720x1336
NATIVE_AUDIO_WAIT_FRAME_VIEW hasFrame=true viewReady=false draw=0/0
NATIVE_AUDIO_START_AFTER_FRAME_VIEW      0×
```

**Jádro počítalo, obraz byl hotový** (`hasFrame=true`). Dvě moje chyby:

**Obraz nebyl vidět.** Plocha Segy se přidává na spodek (`addView(sv, 0)`) —
při hře to sedí, protože skin má průhledný střed. V intru jsem přes ni
kreslil černé plátno.

**Zvuk se nikdy nespustil.** Ten řetěz čeká na `viewReady`, což je **starý
pohled přes okno**, který Sega od B117 nepoužívá. V intru se nikdy
nerozkreslí, takže se čekalo donekonečna.

## Co je opravené

**Plátno se během živých jader čistí do průhledna** místo černé, a okno
WebView je průhledné. Obraz jádra pod ním je pak vidět — **včetně původní
grafiky Segy a PS1**, protože si ji ta jádra kreslí sama.

**Zvuk v intru čeká jen na obraz z jádra**, ne na starý pohled. Zkouší to
dvacetkrát po 150 ms, tedy tři vteřiny. **Normální spouštění hry zůstává
nedotčené** — sáhl jsem jen na cestu, kterou používá intro.

**Čekání během náběhu** už není černo:

```
        MEGA DRIVE
   [████████░░░░░░░░]
        NABIHA..
```

Název, plnící se pruh a tečky. Jakmile jádro začne kreslit, samo se to vytratí.

**Stahování má tři pokusy a delší čekání:**

```
připojení   12 s  ->  30 s
čtení       20 s  ->  60 s
pokusy       1    ->   3, s pauzou 1,5 a 3 s
```

Mezi pokusy se v dialogu ukáže `pokus 2 ze 3`, ať víš, co se děje.

## Ověřeno u mě

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS intra i rozcestníku | 0 chyb |
| volání do Javy ve správném pořadí | ověřeno spuštěním |
| jádra Segy i PS1 | **nesaháno** |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci a nech film běžet.
**3)** Ve 31. vteřině — **vidíš skutečnou Segu se Sonicem?** Obraz i zvuk.
**4)** Než naskočí, má být vidět `MEGA DRIVE` a plnící se pruh, ne černo.
**5)** V 38. vteřině **PS1** — logo a zvuk Sony.
**6)** **PŘESKOČIT** během Segy — musí zmlknout.
**7)** Atari, PS1, Sega, WEB TV — jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 3 | Sega i se zvukem | černo nebo ticho |
| 4 | nápis a pruh | černo |
| 5 | PS1 logo a zvuk | nic |
| 6 | zmlkne | hraje dál |
| 7 | jako v B131 | jakákoli změna |

## CO POSLAT ZPĚT

Log. Hledej:

```
BUILD2SA31 INTRO_OKNO pruhledne
BUILD2SA31 INTRO_SEGA_ZVUK pusten po N pokusech
```

Ten druhý řádek je ten hlavní. Když tam bude `obraz nedosel ani po 3 s`,
jádro nedodalo snímek a půjdu hledat tam.

---

## Bod 4 — obraz a zvuk Segy a PS1 na TV

Zatím **nedělám** a řeknu proč: přímá cesta na TV je u PS1 vypnutá už dávno
(`POUZIT_PRIMOU_CESTU_NA_TV = false`) s poznámkou, že se to nedařilo.
TV si bere snímek půjčkou od jádra — a musím zjistit, jestli to během
intra vůbec může fungovat, když je nad tím WebView.

**Nechci ti slíbit TV a pak ji zase nedodat**, jako se stalo s PS1 znělkou.
Až budeš mít tenhle build ověřený, vezmu si TV jako samostatný krok.
