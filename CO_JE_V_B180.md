# B180 — VLÁKNA SE PŮJČUJÍ (versionCode 228)

Psal jsi mi to pořád dokola a měl jsi pravdu: **Atari + TV, samo o sobě,
po minutě.** Konečně jsem šel jen po té jedné cestě.

## Co jsem našel

```java
Socket s = ss.accept();
new Thread(() -> napTvWebHandleClient(s)).start();   // NOVE VLAKNO
```

Server dělá **nové vlákno na každé spojení**. Dokud chodilo pár požadavků
za vteřinu, nevadilo to.

Jenže snímky z Atari, které jsem tam přidal, chodí **16× za vteřinu** —
to je **přes 900 nových vláken za minutu**. To systém neustojí.

Proto to padalo:

| co jsi popisoval | proč |
|---|---|
| jen když je zapnutá TV | bez TV se snímky neposílají |
| jen v Atari | PS1 a Sega snímky neposílají, berou se z jádra |
| po zhruba minutě | tolik času trvá, než těch vláken bude moc |
| jedno, co bylo předtím | nezáleželo na tom, jen na počtu vláken |

**Tuhle zátěž jsem tam přidal já**, tak ji tam i řeším.

## Oprava

Vlákna se **půjčují ze zásoby**, nevyrábějí se. Dlouhé proudy (H264,
MJPEG, zvuk) si vlákno drží celou dobu, krátké požadavky si ho půjčí
a vrátí.

Ověřeno spuštěním:

```
dlouhé proudy:        3, drží vlákno
snímků odesláno:      24 z 24, nic odmítnuto
celkem VYROBENO:      4 vlákna
```

```
vláken za minutu v Atari s TV
   B178:  960
   B180:  nejvýš 24, a půjčují se
```

A snímky se posílají **8× za vteřinu místo 16×** — na TV to stačí a je
to o polovinu míň spojení.

## Zůstává

Tiché držení snímku z B179, cesta pro PS1 a Segu shodná s B156,
klávesnice, intro.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| zásoba vláken proti dlouhým proudům | 24/24 prošlo, 4 vlákna |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Atari + WEB TV.** Nech běžet pět minut. Piš si v BASICu.
**3)** Spadne to?
