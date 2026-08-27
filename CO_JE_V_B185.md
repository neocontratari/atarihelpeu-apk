# B185 — SKOK A VÝSTŘEL (versionCode 233)

## Proč Ctrl a Alt nefungovaly

Blokoval jsem je **už v prohlížeči**:

```js
var PREHAZOVACE={Shift:1, Control:1, Alt:1, ...};
if(PREHAZOVACE[e.key]) return;
```

Přidal jsem to, aby Shift nepsal písmena — a tím jsem si zabil skok
i výstřel. Do Atari se nikdy nedostaly.

Teď projdou. Atari si s nimi poradí samo: při psaní nedělají nic,
při hraní jsou to tlačítka.

## Zaseknutý směr

Trefil jsi to: *„pamatuje si to poslední klávesu, takže Mario jde
nesmyslně furt doleva."*

Když klávesu pustíš mimo okno nebo přepneš `F9`, **puštění nikam
nedojde** a páka zůstane držená.

Teď se všechno pustí:

```
při přepnutí F9
při ztrátě zaměření okna
když se přepneš na jinou kartu
```

## Ověřeno spuštěním

```
SKOK A VYSTREL
  Ctrl dolů   -> SKOK      páka=1, klávesa=33 (mezerník)
  Ctrl nahoru -> páka=0, klávesa pryč
  Alt dolů    -> VYSTREL   střelba=true
  Alt nahoru  -> střelba=false

ZASEKNUTY SMER
  držím A     -> páka=4 (vlevo)
  zmáčknu F9  -> páka=0, střelba=false, klávesa pryč
```

## K té myši

Klávesy toho Atari na obrazovce **na myš reagují už dneska** — jsou
napsané na `pointerdown`, což je dotyk i myš zároveň.

Co nefunguje je klikání **z prohlížeče na počítači** — ten dostává jen
obraz, ne kliknutí. Jde to dodělat stejnou cestou jako klávesnici:
prohlížeč pošle, kam jsi klikl, a aplikace to přepočítá na místo
v Atari.

Řekni, jestli to chceš — je to podobná práce jako ta klávesnice.

## Ověřeno

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 52 tříd |
| JS Atari i všech etap | 0 chyb |
| skok, výstřel, puštění páky | ověřeno spuštěním |
| cesta PS1 a Segy proti B156 | shodná |
| jádra Segy i PS1 (C++) | nesaháno |

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** `F9`, pak `Ctrl` a `Alt` — skáče a střílí?
**3)** Drž `A`, zmáčkni `F9` — přestane jít doleva?
**4)** Přepni na jinou kartu prohlížeče a zpátky — nezasekne se to?
