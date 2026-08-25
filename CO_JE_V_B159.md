# B159 — PŘESTÁVÁM HÁDAT (versionCode 207)

Čtyřikrát jsem tvrdil „opraveno" a čtyřikrát to bylo jinde. Ten kód
vypadá správně, takže dál hádat nemá smysl.

## Co jsem přidal

**Kód teď sám řekne, co dělá.** Jednou za vteřinu během živých jader:

```
BUILD2SA45 TV_INTRO zdroj=SEGA segaPlocha=true ps1Plocha=false
                    biosBezi=false snimek=320x224
```

nebo

```
BUILD2SA45 TV_INTRO zdroj=PS1 segaPlocha=false ps1Plocha=true
                    biosBezi=true snimek=NIC(0)
```

Z toho poznám všechno, co mi doteď chybělo: **jestli se ta větev pro
intro vůbec spustila**, ze kterého jádra se bralo a jestli něco přišlo.

Řádek `TV_WEB_PERIODIC` hlásí adresu vždycky, takže se z něj nedalo
poznat, kterou cestou se šlo — proto jsem se motal.

## Co jsem u toho opravil

Když jádro zrovna nemá snímek, kód se propadl na **snímání okna**. Jenže
pod oknem leží plocha jádra a okno je průhledné — vyfotilo by se prázdno.
A protože se to střídalo tam a zpět, **nejspíš právě tohle způsobovalo,
že se přenos sem tam zasekl**.

Během intra se teď místo toho podrží poslední dobrý snímek.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS všech etap | 0 chyb |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO POTŘEBUJU

**Zapni WEB TV, nech intro doběhnout a pošli log.** Nic víc.

V něm bude pár řádků `BUILD2SA45 TV_INTRO` a ty mi řeknou přesně,
kde to vázne:

```
snimek=320x224   jádro dává obraz, chyba je až za tím
snimek=NIC(0)    jádro nic nedává, jdu do něj
zdroj=PS1 kdyz mela hrat Sega   spatne se pozna, ktere jadro bezi
```

Je možné, že se zvedne i ta obrazová část sama — ta pojistka proti
propadnutí na okno mohla být ta příčina zasekávání. Ale **netvrdím to**,
dokud to neuvidím v logu.

---

Vím, že tohle není oprava, ale krok k ní. Po čtyřech pokusech naslepo
je to poctivější než páté hádání.
