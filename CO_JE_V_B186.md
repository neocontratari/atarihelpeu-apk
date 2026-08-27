# B186 — POŘADÍ KLÁVES A ZVUK (versionCode 234)

## 1) Zaseknutá klávesa — trefil jsi to

Napsal jsi: *„po přepnutí na psaní jelo neustále písmeno A."*

Každá klávesa se posílala **jako samostatný požadavek** a server je
zpracoval **na různých vláknech** ze zásoby. Puknutí tedy mohlo přijít
**dřív než stisk** — a klávesa zůstala držená.

Proto to fungovalo jen jednou: první stisk prošel, a od druhého se to
prohodilo.

Teď je jedna fronta a posílá se **jedna po druhé**. Ověřeno spuštěním
i s náhodným zpožděním serveru:

```
KeyA-dolu KeyA-nahoru KeyA-dolu KeyA-nahoru KeyA-dolu KeyA-nahoru

střídá se správně:                        ANO
poslední je puknutí (klávesa nezůstane):  ANO
```

## 2) Pravý Alt

Na české klávesnici hlásí pravý Alt `AltGraph` — a ten jsem měl
v seznamu blokovaných. Proto šel výstřel jen z levého.

Odblokován, a Atari ho teď taky pozná.

## 3) Ořezávání zvuku jako u Segy

Vzpomněl sis správně. Jádro Segy má v C++ tohle:

```cpp
if (q.size() > max) { drop = q.size() - max;
                      while (drop--) q.pop_front(); }
```

**Průběžně a po malém.** Atari to dělalo jinak — čekalo na 48 000 položek
a pak naráz vyhodilo 24 000 přes `splice()`. To je skoro půl vteřiny
zvuku ve frontě a ten `splice` je navíc drahý: dělá se na hlavním vlákně,
kde běží emulátor.

Teď je strop 12 000 a ořezává se průběžně. Ověřeno:

```
zápisů celkem:            30 000
fronta nikdy nepřešla:    12 000 položek
zahozeno starých:         26 000
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| pořadí kláves i při náhodném zpoždění | ověřeno spuštěním |
| ořezávání zvuku | ověřeno spuštěním |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** `F9`, drž `A` — jde doleva a **po puštění přestane**?
**3)** Zkus to desetkrát za sebou — funguje pořád, ne jen poprvé?
**4)** `Ctrl` a `Alt` z **obou stran** klávesnice.
**5)** Chrastí ještě zvuk Atari se zapnutou TV?

Bod 5 je ten, kvůli kterému sis vzpomněl na Segu — a měl jsi pravdu,
že tam ta cesta vede.
