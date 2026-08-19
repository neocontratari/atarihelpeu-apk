# B131 — ÚVODNÍ FILM PRO SRAZ KLANU (versionCode 179)

> **Tento kód je předpoklad. Čeká se na test.**
> Zatím jsi viděl jen moje obrázky — tohle je první build, kde si to pustíš.

## Není to obrazovka, je to film

Šest scén, celkem **58 vteřin**, pak samo skočí do rozcestníku.
Tlačítko **PŘESKOČIT** je vpravo dole a jde kdykoli.

```
0,0 –  2,5 s   televize se zapíná — čára se rozšíří a rozevře do plochy
2,5 – 12,0 s   bootovací výpis, píše se po písmenkách, zelený fosfor
               ATARI 130XE ... OK / SEGA MEGA DRIVE ... OK / PLAYSTATION ... OK
               GOLD BETA — KKT ONLY / READY.  a kurzor spadne pod něj
12,0 – 17,0 s  průlet hvězdami, zrychluje, na konci zášleh
17,0 – 22,0 s  logo přilétá zdálky, dosedá, naskočí GOLD BETA
22,0 – 54,0 s  logo plave, dole běží celý tvůj text jako vlnící se scroller
54,0 – 58,0 s  zatmívačka, KAMARADI KRTECKA TONDY
```

## Grafika — druhé kolo

Přidal jsem, cos chtěl:

- **plazma** na pozadí — počítá se na malém plátně 96×168 a roztahuje se
  přes obrazovku, přesně jak to dělala osmibitová dema
- **perspektivní mřížka** — podlaha, která se blíží, s ubíhajícími čarami
- **slunce nad horizontem** s vodorovnými prořezanými pruhy
- **bobíky** — barevné koule po Lissajousových křivkách, sčítají se
- **odraz loga** pod ním, zrcadlený a vytrácející se
- **hvězdy s ohonem** ve warpu — čím rychleji, tím delší pruh

### Výkon jsem měřil, ne odhadoval

Profiloval jsem každý efekt zvlášť a našel dvě zbytečnosti:

```
                předtím    potom
logo             23,0 ms    0,7 ms     předkresleno jednou
odraz            18,1 ms    6,3 ms     totéž
slunce            8,1 ms    6,6 ms     zář zapečená do plátna
celá scéna         98 ms     60 ms
```

Logo se každý snímek zmenšovalo z obrázku 1200 bodů širokého a k tomu
se vyráběl gradient. Teď se to nakreslí **jednou** do malého plátna a
pak už se jen kopíruje — třicetkrát rychleji.

Těch 60 ms je **softwarové** vykreslování v mém testu. Telefon má GPU,
kde je roztažení plazmy prakticky zadarmo. Ale kdyby se to přesto seklo,
je v `intro/index.html` přepínač:

```js
var PLAZMA_PLYNULE = true;   // na false = kostičkovaná plazma, 11x rychlejší
```

Řekni a přehodím ho.

## Co jsem opravil podle tvých připomínek

**Ťukání teď sedí na písmena.** Dřív běželo na časovač a s textem to
neladilo. Teď se ozve **právě tehdy, když přibude znak** — a konec řádku
má svůj vlastní zvuk. Ověřeno: 127 znaků, 7 konců řádku, **135 tónů**.

**Psaní je pomalejší** — 48 ms na znak, celý výpis se píše 6,1 s a pak
3,4 s bliká kurzor.

**Kurzor spadne pod READY.** Dokud se píše, stojí za posledním znakem;
jakmile je hotovo, přeskočí na další řádek — jak to dělá skutečné Atari.

**Logo víc plave.** Zvětšuje a zmenšuje se třikrát víc než dřív a k tomu
se jemně posouvá nahoru a dolů.

Přes celou obrazovku je jemný rastr a zaoblení jako na CRT, aby to
nevypadalo jako webová stránka.

## Zvuk

Všechno se vyrábí za běhu, žádný soubor navíc:

- **zapnutí** — dunivý náběh a pisknutí obrazovky
- **boot** — nahrávací pípání, jaké dělá POKEY při přenosu ze SIO
- **warp** — stoupající sweep
- **logo** — akord a zášleh
- **hlavní scéna** — vlastní chiptune: basa, arpeggio, melodie

**Co tam není a proč:** to zvolání „SEEEGAA" je nahrávka hlasu, kterou
vlastní Sega, a startovní zvuk PS1 patří Sony. Nebudu je napodobovat.
Atari pípání je jiná věc — to není nahrávka, ale funkční tón, který
vyrábí čip, takže ten jsem udělal.

Prohlížeč někdy nepustí zvuk bez dotyku. Zkouší se to hned po startu
i po prvním doteku obrazovky.

## Ověřeno u mě

Film jsem **spustil proti skutečnému plátnu** a vyrenderoval snímky ze
všech scén. Doběhne za 58 s a přesměruje.
Ťukání ověřeno na počet: 135 tónů na 127 znaků a 7 konců řádku.

| část | výsledek |
|---|---|
| Java, `javac` proti android.jar 34 | 0 chyb, 42 tříd |
| C++ Atari, PS1, Sega (aarch64) | 4/4 |
| linkování PS1+Sega | 31/31 |
| JS rozcestníku, intra i HELP | 0 chyb |
| film spuštěný v node | doběhne, přesměruje, zvuk běží |
| původní Atari | bajt po bajtu netknuté |

### Tři chyby, které jsem si při tom sám našel

Bez skutečného spuštění bych ti poslal rozbité intro:

1. **Zář se hromadila** — mazal jsem pozadí poloprůhledně kvůli stopám
   za hvězdami, takže bílá koule kolem loga se přes snímky nabalovala,
   až z ní byla šedivá placka přes celou obrazovku.
2. **Časová osa byla špatně** — psal jsem čísla jako konce scén, ale kód
   je čte jako délky. Intro by trvalo **dvě a půl minuty**.
3. **Polykal jsem výjimky** — `catch(e){}` bez ničeho. Když se scéna
   rozbila, byla jen černá obrazovka a žádná stopa.

---

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci a **nech film doběhnout celý**.
**3)** Spusť znovu (ukončit a otevřít) — film má naskočit zas.
**4)** Během filmu zmáčkni **PŘESKOČIT**.
**5)** Vrať se z hry do nabídky — film **nemá** naskočit znovu.
**6)** **ATARI**, **PS1** a **SEGA** — musí běžet jako dosud.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | šest scén za sebou, logo čitelné, text projede celý, hraje zvuk | scéna chybí, seká se, ticho, černo |
| 3 | film naskočí znovu | nenaskočí |
| 4 | okamžitě do nabídky | nic |
| 5 | film nenaskočí | naskočí |
| 6 | jako dosud | jakákoli změna |

## CO MI ŘEKNI

Na tohle se nedokážu podívat sám:

1. **Neruší tě něco v tom ťukání?** Teď je vázané na písmena.
2. **Neseká se to na S8?** Hvězdy a pruhy stojí výkon.
3. **Je text ve scrolleru čitelný?**
4. **Hraje zvuk?** A není moc hlasitý?
5. **Sedí to pípání?** Dělal jsem ho po paměti podle SIO.

## CO POSLAT ZPĚT

Log přes **ULOZIT LOG A ODESLAT** na obrazovce HELP.
Hledej v něm `BUILD2SA21 START` a `BUILD2SA21 INTRO_HOTOVO`.

---

## Atari v C++ — kde se skončilo

Práce **pozastavená** na tvůj pokyn. Celý stav je v
`PREDAVACI_BALICEK_PS1.txt`, aby se k tomu dal vrátit kdokoli.

**Běží:** procesor (1 024 000 instrukcí, 0 rozdílů), paměť (16 252 928
čtení, 0 rozdílů), WSYNC, zavaděč XEX, display list, DLI, OS nabootuje,
BASIC naskočí, **self-test z ROM se vykreslí celý**, Decathlon se nahraje.

**Neběží:** vykreslení Decathlonu, kolize, zvuk, VBXE.

**Poučení zapsané v protokolu:** vrstvy 1 a 2 jsem překládal z JavaScriptu
a vyšly napoprvé. Vrstvu 3 jsem začal psát znovu a za den z toho bylo
**osm vlastních chyb**, které v původním JS nejsou. Další krok je vrstvu 3
**přeložit**, ne psát.
