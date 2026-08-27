# B193 — KONTROLA, JAK TO VIDÍ RENE (versionCode 241)

## Přečetl jsem si protokol a měl jsi pravdu

Je tam napsané, bod 3d:

```
JavaScript ve strance: vytahnout retezce z Javy, SLOZIT A SPUSTIT
v node s napodobeninou prohlizece

POZOR: pri vytahovani retezcu se MUSI NEJDRIV ODSTRANIT JAVA KOMENTARE.
V komentarich jsou taky uvozovky a bez toho vznikne nesmysl
a falesna chyba.
```

**Já dělal jen `node --check`.** To najde chybějící závorku, ale ne to,
že se stránka rozbije až za běhu. A komentáře jsem neodstraňoval.

## Co je teď

Stránka se **sestaví a SPUSTÍ** proti napodobenině prohlížeče — s
`document`, `fetch`, `XMLHttpRequest`, `AudioContext`, časovači. Pak se
nechá chvíli běžet a **čte se, co by napsala do konzole**.

Tedy vidím to, co bys viděl ty.

**Ověřil jsem, že by tu chybu z B190 chytila:**

```
kontrola na ROZBITEM B190:
  skript 1: SYNTAX CHYBA
  návratový kód: 1
```

Kdybych ji měl včera, ta černá obrazovka by k tobě nikdy nedošla.

## Co je v tomhle buildu

Všechno z B192, plus tahle kontrola. Konkrétně:

```
fronta pro diváka       300 -> 24, zahazuje stare misto cekani
                        (zpozdeni projektoru 12,5 s -> 1,5 s)
start aplikace          uz neleze na internet
Atari                   v POMALE vetvi, jak rika protokol
cesta PS1 a Segy        shodna s B156
klavesnice              Atari i Sega
TV web                  overeno SPUSTENIM
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| **stránka pro TV SPUŠTĚNA v node** | **0 chyb, oba skripty** |
| totéž přímo ze zabaleného balíčku | 0 chyb |
| kontrola chytí chybu z B190 | ověřeno |
| JS všech stránek v assets | 0 chyb |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **TV web — naskočí obraz?**
**3)** **Plátno — trhá se? Musíš dávat refresh?**
**4)** Atari `F9`+WASD+K+L, Sega WASD+K+L+O+P.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 2 | obraz naskočí | černá stránka |
| 3 | jede plynule, refresh netřeba | trhá se, roste zpoždění |
| 4 | klávesy fungují | nereaguje |
