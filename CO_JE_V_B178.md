# B178 — ROZMĚR UŽ NESKÁČE (versionCode 226)

## To probliknutí byla ta stopa

Napsal jsi: *„při spuštění web vieweru problikává Atari výška šířka.
Jakmile se obraz ustálí, apka spadne."*

Přesně tak to bylo:

```
Atari má nový snímek  ->  384x240
Atari nemá nový       ->  propadne se na snímání okna  ->  720x1336
```

A střídalo se to **šestnáctkrát za vteřinu**. Enkodér, který má z toho
udělat video, dostával pořád jiný rozměr — a spadl.

## Oprava

Když jsi v Atari, **poslední snímek se podrží** místo propadnutí na okno.
Rozměr zůstane stejný.

Ošetřené jsou všechny cesty ven z té funkce, včetně té při chybě —
ověřeno projitím:

```
napTvWebCaptureIntro   drží poslední snímek   ANO
napTvWebCaptureAtari   drží poslední snímek   ANO
```

## K té chybějící ikoně

Manifest je v pořádku — ikona i spouštěč tam jsou:

```
android:icon="@mipmap/ic_launcher"       je
android.intent.category.LAUNCHER         je
```

Nejspíš to bylo tím, že aplikace padala hned po instalaci a systém ji
nestihl zapsat do nabídky. Po tomhle buildu by měla naskočit sama.

## Zůstává

Klávesnice se zapne sama v Atari, cesta pro PS1 a Segu shodná s B156,
intro má vlastní cestu pro obraz jader, kopie snímku opravená.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| žádná cesta nepropadne na okno | ověřeno projitím obou funkcí |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Atari + WEB TV** — probliká to ještě? Spadne to?
**3)** Klávesnice z počítače.
**4)** PS1 a Sega.
