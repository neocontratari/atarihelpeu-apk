# B150 — PĚT SAMOSTATNÝCH ETAP (versionCode 198)

M�l jsi pravdu. Přehrával jsem **jeden film pod cizími plochami** a to byla
špatná stavba — proto mi to nevycházelo, ať jsem časování opravoval jakkoli.

## Jak to je teď

```
etapa1.html    ATARI ... OK, SEGA ... OK, PLAYSTATION ... OK
etapa2.html    Atari, READY, píše se program
   (jádro Segy)
   (jádro PS1)
etapa5.html    Atari, program běží, krteček, poděkování
```

**Každá etapa je vlastní stránka a začíná od nuly.** Po jádrech se načte
nová stránka — takže ji nemá co uspat.

Původní problém: když nad WebView ležela plocha jádra, prohlížeč kreslení
uspal a po probuzení už stará stránka nekreslila. Ať jsem opravoval cokoli,
seděl jsem na špatné stavbě.

## Ověřeno projetím celého řetězu

Pustil jsem všechny etapy za sebou tak, jak to udělá aplikace:

```
etapa1.html   doběhla za 12,0 s  ->  etapa 2
etapa2.html   doběhla za 33,5 s  ->  etapa 5
etapa5.html   doběhla za 24,0 s  ->  konec

řetěz: etapa(2), spustSegu, zastavSegu, spustPs1, zastavPs1,
       etapa(5), hotovo, etapa(0)
```

**Etapa 5 se skutečně načte a doběhne celých 24 vteřin** — to je ten
krteček, který se doteď neukázal.

Ten test našel při stavbě tři chyby, které bych ti jinak poslal:
chybějící `BOOT`, `zvukPohlidej` z verze, kterou jsem vrátil, a chybějící
`LISTING` v etapě 5. Všechny opravené před odesláním.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS všech tří etap | 0 chyb |
| JS rozcestníku | 0 chyb |
| celý řetěz etap projetý | 1 → 2 → jádra → 5 → konec |
| jádra Segy i PS1 (C++) | nesaháno |

Starý `intro/index.html` je smazaný, aby se nedal omylem spustit.

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci — etapa 1, pak Atari s kódem.
**3)** Sega, pak PS1.
**4)** **Po PS1 se musí načíst etapa 5 s krtečkem** — to je ta věc.
**5)** Zapni WEB TV a projdi to znovu.
**6)** Atari, PS1, Sega — jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 4 | krteček a poděkování | prázdno |
| 5 | obraz i zvuk | něco chybí |
| 6 | jako dosud | jakákoli změna |

## CO POSLAT ZPĚT

Log. Musí v něm být tahle posloupnost:

```
BUILD2SA38 ETAPA 2 -> file:///android_asset/intro/etapa2.html
BUILD2SA38 ETAPA 5 -> file:///android_asset/intro/etapa5.html
BUILD2SA38 ETAPA konec -> rozcestnik
```

Když `ETAPA 5` chybí, po jádrech se nenačetla — a půjdu tam.
