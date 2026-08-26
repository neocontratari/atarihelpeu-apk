# B163 — POKUS, KTERÝ TO ROZHODNE (versionCode 211)

## Co víme z měření

```
most drží JS vlákno    0,5 ms na dávku, fronta prázdná, nic se nezahazuje
snímání okna           asi 19 % hlavního vlákna
```

**Ani jedno z toho by kousat nemělo.** A přesto se to kouše, jakmile
zapneš TV.

Zkusil jsem tři opravy — tempo snímání, rozlišení, most — a všechny
minuly. Přestávám hádat.

## Co jsem udělal

V nabídce OPTIONS je nové tlačítko, které přepíná dokola:

```
TV POKUS: OBOJI       normální provoz
TV POKUS: JEN ZVUK    obraz se nesnímá
TV POKUS: JEN OBRAZ   zvuk se neposílá
```

Tím se ta příčina dá **chytit za jeden test**.

## CO POTŘEBUJU, ABYS UDĚLAL

Zapni WEB TV, jdi do **ATARI 130XE** a zahraj si. Pak přepínej to
tlačítko a sleduj, kdy se kousání zastaví:

| nastavení | kouše se? | co to znamená |
|---|---|---|
| OBOJI | asi ano | výchozí stav |
| **JEN ZVUK** | ? | když **přestane**, dělá to snímání obrazu |
| **JEN OBRAZ** | ? | když **přestane**, dělá to zvuková cesta |

**Když se kouše i při obou** — pak to nedělá ani jedno a příčina je
v tom, že TV vůbec běží (třeba enkodér nebo síť). To by byla úplně jiná
stopa a věděl bych, kam jít.

Pošli mi jen tu jednu větu: která volba to zastavila.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS rozcestníku i etap | 0 chyb |
| Java Atari (`emu_vbxe`) | netknuté proti B123 |
| jádra Segy i PS1 (C++) | nesaháno |

---

Vím, že tohle není oprava, ale krok k ní. Po třech pokusech naslepo je
to poctivější než čtvrtý — a jeden tvůj test to rozhodne líp než další
den mého hádání.
