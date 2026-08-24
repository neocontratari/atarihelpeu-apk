# B153 — STARÝ LISTING PRYČ (versionCode 201)

## Měl jsi pravdu a byla to ošklivá chyba

V každé etapě byl **`var LISTING` dvakrát**:

```
etapa2.html:72   nový listing - poloviční strojak
etapa2.html:448  STARÝ listing ze společné části
```

Ten druhý ten první přepsal, takže se v aplikaci psal **původní kód**,
i když v souboru byl ten nový. Když jsem to kontroloval, četl jsem první
výskyt a hlásil, že je to v pořádku.

Vzniklo to tím, jak jsem etapy stavěl — vzal jsem společnou část z původního
filmu a ta v sobě ten listing měla taky.

## A nebyla sama

Prohledal jsem všechny etapy na dvojité definice a bylo jich **sedm**:

```
etapa1.html   BOOT_TEXT, bootNapsano
etapa2.html   LST_PAUZA, listing, lstZnaku (3×)
etapa5.html   VYSTUP, finVys
```

Všechny odstraněné. Kontrola po opravě: **čisto ve všech třech**.

Tohle mohlo dělat další divné věci, které bys objevoval jednu po druhé.

## Ověřeno

```
etapa2 řádek 20:  FOR I=0 TO 73          ← poloviční strojak
etapa5 řádek 20:  FOR I=0 TO 73
VYSTUP:           19 řádků, 377 znaků
řetěz:            etapa1 → etapa2 → jádra → etapa5 → konec
```

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 45 tříd |
| JS všech tří etap | 0 chyb |
| dvojité definice | 0 |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Etapa 2 — **musí tam být `20 FOR I=0 TO 73`** a devět řádků `DATA`.
       Když uvidíš `20 SETCOLOR 2,12,2`, je to pořád starý listing.
**3)** Po PS1 etapa 5 — duha jako na Atari a krteček kolem.
**4)** Atari, PS1, Sega — jako dosud.
