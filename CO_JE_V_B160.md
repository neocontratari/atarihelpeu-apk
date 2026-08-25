# B160 — PRAVÁ ZÁVORA (versionCode 208)

## Diagnostika mlčela, a to byla ta odpověď

V tvém logu je `BUILD2SA45 TV_INTRO` **nulakrát**, přestože TV
prokazatelně běžela současně s jádry:

```
TV záznamy:   82461,8 – 82572,9
SEGA start:   82521,7   TV v té době běžela: ANO
PS1 start:    82528,7   TV v té době běžela: ANO
```

Takže jsi to udělal správně a moje diagnostika se stejně nespustila.
To znamenalo jediné: **do té části kódu se vůbec nechodí.**

## Co jsem našel

V té funkci byly **dvě závory na adresu, ne jedna**:

```java
řádek 1254   if (!naPs1) return false;                    ← TAHLE
řádek 1286   if (!naSege && !ps1SessionActive ...) return  ← tuhle jsem opravil v B158
```

Během intra je adresa pořád `etapa2.html`, takže se vyskočilo hned
na té první — o dvacet řádků dřív, než kam sahala moje oprava.

**Opravil jsem tu druhou v pořadí.** Proto nezabrala ani ona, ani
diagnostika za ní.

Teď procházejí obě a **prošel jsem si všechny odchody z té funkce**,
aby tam netrčela třetí:

```
řádek 1228  if (tvPrimoBezi)          pouští intro dál
řádek 1229  if (bw <= 0 || bh <= 0)   pouští intro dál
řádek 1265  if (!naPs1 && !introZivaCast)   pouští intro dál
řádek 1289  if (!introZivaCast && ...)      pouští intro dál
```

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech etap | 0 chyb |
| všechny odchody z funkce prověřené | 4/4 |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Zapni WEB TV, pusť intro, nech doběhnout.
**3)** **Mají Sega a PS1 obraz na TV?**

## CO POSLAT ZPĚT

Log. Teď tam **musí být** řádky:

```
BUILD2SA45 TV_INTRO zdroj=SEGA segaPlocha=true snimek=320x224
BUILD2SA45 TV_INTRO zdroj=PS1  ps1Plocha=true  snimek=...
```

Když tam budou a obraz přesto nebude, uvidím z nich přesně kde —
jestli jádro nic nedává (`snimek=NIC`), nebo se bere ze špatného.

Když tam **zase nebudou**, je v cestě čtvrtá závora a půjdu ji hledat.
