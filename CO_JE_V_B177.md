# B177 — VŠECHNO DOHROMADY (versionCode 225)

Tři věci, co jsi chtěl. Všechny ověřené, než jsem to poslal.

## 1) Klávesnice — chyběl jeden kus

Aplikace v `/status` hlásila `atari=1` správně, ale **stránka to nikdy
nepřečetla** — ten kód se do ní nedostal, protože je tam všechno na
jednom dlouhém řádku a moje úprava minula.

Takže `napVAtari` zůstalo napořád `false` a klávesnice mlčela.

Teď je ta cesta celá: aplikace to hlásí → stránka to čte → klávesnice
se zapne sama, když jsi v Atari.

## 2) Pád v Atari

`u8.slice(0)` — kopie 360 kB na každý snímek přímo v prohlížeči, kde
běží emulátor. 506 MB odpadu za minutu a půl. **Moje bota z B165.**

Ověřeno, že v kódu už není: drží se jedna kopie, která se přepisuje.

## 3) Intro má obraz jader — a PS1 zůstává netknutá

Tohle jsem předtím udělal špatně: **přepsal jsem cestu, kterou používá
PS1 a Sega**, aby uměla i intro. Tím jsem rozbil PS1 na projektoru.

Teď má intro **vlastní cestu vedle**, úplně mimo ni:

```
1. intro    (živá jádra Segy a PS1 během etap)
2. Atari
3. původní cesta pro PS1 a Segu   ← BAJT PO BAJTU JAKO B156
```

Ověřeno porovnáním: ta funkce je shodná s B156, který ti na projektoru
běžel.

## Ověřeno před odesláním

```
1. PAD V ATARI     kopie snímku při každém snímku    0×
                   držená jedna kopie                ANO
2. KLAVESNICE      status hlásí atari=               ANO
                   stránka to ČTE                    ANO
                   zapne se sama                     ANO
                   vstupní bod v Atari               ANO
3. PS1 JAKO DŘÍV   cesta shodná s B156               ANO
4. INTRO NA TV     vlastní cesta                     ANO
                   volá se PŘED cestou PS1           ANO
```

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** **Atari s TV** — vydrží aspoň pět minut bez pádu?
**3)** **Klávesnice** — píše do Atari z počítače?
**4)** **PS1 a Sega** — jedou jako v B156?
**5)** **Intro** — mají Sega a PS1 obraz i zvuk?

---

Sáhl jsem do PS1 a rozbil ji — to byla moje chyba a mrzí mě, že tě to
stálo noc. Teď je ta cesta vrácená a ověřená proti B156, a všechno nové
běží vedle ní, ne v ní.
