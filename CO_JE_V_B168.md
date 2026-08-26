# B168 — JEDNA CESTA, NE DVĚ (versionCode 216)

## 1) Proč to bylo pomalejší, ne rychlejší

Z tvého logu:

```
ATARI_SNIMEK   18 hlášení, 27-50 snímků za 5 s
PIXELCOPY     238×
```

**Běželo obojí naráz.** Atari posílalo snímky **a zároveň** se dál
snímalo okno — takže dvojí práce. Proto to bylo pomalejší než předtím.

Pojistku jsem měl **o 200 řádků výš, v úplně jiné větvi**, než odkud
se `PixelCopy` opravdu volá. Teď je přímo u něj.

## 2) Klávesnice se nikdy nespustila

V logu **ani jedna klávesa**. Vložil jsem ji dovnitř toho velkého
skriptu na stránce — a když cokoli nad ní selže, už se nespustí.

Teď je ve **vlastním skriptu**, který nemůže nic shodit, a používá
`XMLHttpRequest` místo `fetch`, aby fungovala i ve starším prohlížeči.

Ověřeno rozborem vygenerované stránky:

```
skriptů ve stránce:           2 (klávesnice má vlastní)
addEventListener keydown:     ANO
posílá na /klavesa:           ANO
F2 = BREAK:                   ANO
neblokuje F5/F12:             ANO
```

## 3) Aby se to příště poznalo

Když klávesa dorazí, zapíše se to:

```
BUILD2SA54 KLAVESA code=KeyA znak=a dolu=true celkem=17
```

Když tam ten řádek **nebude**, klávesa nedošla do aplikace — chyba je
v prohlížeči. Když **bude** a v Atari se nic neděje, chyba je za tím
a půjdu tam.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| rozbor stránky pro TV | 6/6 v pořádku |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV, otevři adresu v prohlížeči na počítači.
**3)** Vpravo dole má být `KLAVESNICE ZAPNUTA - F2 = BREAK`.
**4)** Jdi na telefonu do **ATARI 130XE** a **piš na klávesnici počítače**.
**5)** Kouše se ještě Atari?

## CO POSLAT ZPĚT

Log. Hledej dvě věci:

```
BUILD2SA54 KLAVESA code=... celkem=..
PIXELCOPY                         ← těch má být MÁLO nebo ŽÁDNÉ
```

Když bude `PIXELCOPY` pořád stovky, běží to dál obojí a jdu tam znovu.
