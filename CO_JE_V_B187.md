# B187 — ZVUK ZPĚT (versionCode 235)

## Rozbil jsem ti zvuk a vracím to

Tvoje stopa se Segou byla dobrá, ale **provedl jsem to špatně ve dvou
věcech naráz**:

**Snížil jsem strop na čtvrtinu.** Z 48 000 na 12 000 — a tím se
ustřihával zvuk, který ještě nezazněl. Odtud ta morzeovka.

**A to „průběžné ořezávání" bylo dražší, ne levnější.** Změřil jsem to
až teď:

```
              fronta max    zásahů do ní
původní         48 000            3
moje            48 000       24 000
```

Sega si to v C++ dovolit může — má frontu, kde je **odebrání zepředu
zadarmo**. Javascriptové pole ne: každý `splice(0,n)` posune celé pole.
Takže 24 000 malých zásahů je horší než tři velké.

**Původní kód je zpátky přesně tak, jak byl.** Ověřeno porovnáním
s B185, kde ti zvuk hrál — jediná změna v Atari je ten pravý Alt.

## Co zůstává z B186

**Pořadí kláves.** Každá se posílala zvlášť a server je zpracoval na
různých vláknech, takže puknutí mohlo přijít dřív než stisk. Teď je
jedna fronta a posílá se jedna po druhé.

**Pravý Alt.** Na české klávesnici hlásí `AltGraph` a ten jsem blokoval.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| zvuk Atari proti B185 | shodný |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Atari — **hraje zvuk normálně?** Nesmí být morzeovka.
**3)** `F9` a WASD — funguje pořád, ne jen poprvé?
**4)** `Ctrl` a `Alt` z obou stran.

---

Ta myšlenka se Segou nebyla špatná. Jen se to nedá přenést jedna ku
jedné — v C++ je ta operace zadarmo, v JavaScriptu ne. Měl jsem to
změřit dřív, než jsem to poslal.
