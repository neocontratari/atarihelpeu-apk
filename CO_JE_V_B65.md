# B65 — LOG NAPSANÝ PRO MĚ (versionCode 113)

Ty vidíš a slyšíš. Já ne. Takže log musí být napsaný tak, abych z něj
viděl já — a ne abys ty něco poznával.

## Jedna řádka za vteřinu, která řekne všechno

    PS1_STAV | jadro: fps=50.00 res=640x512 | zvuk: fronta=1840 zahozeno=0 doskoku=0
      | obraz: cesta=PRIMA snimku/s=50 kresleni=4.1ms swap=1.2ms zdroj=640x512 jas=37 GLchyb=0
      | plocha: 12,340 916x428 poradi=2z3 videt=ANO pruhlednost=1.00
      | nad plochou: WebView(1080x2220)

Co mi která část řekne:

- **jadro** — běží emulace a jak rychle
- **zvuk** — kolik je ve frontě a jestli se zahazuje. Když se ti trhá zvuk,
  uvidím to jako číslo, ne z popisu.
- **cesta=PRIMA** — obraz jde ze sdílené textury GPU. Kdyby tam bylo
  `ZALOZNI`, sdílení neprošlo.
- **kresleni / swap** — kde se ztrácí čas. Odtud poznám trhání.
- **jas** — přečtu 16×16 bodů uprostřed obrazu. **Tohle je klíč**: rozliší
  „obraz se kreslí, ale je schovaný" od „obraz se kreslí černý". To jsem
  posledně nedokázal rozlišit a stálo tě to dva testy.
- **plocha + nad plochou** — kde leží a co ji překrývá. Kdybych tohle měl
  dřív, hned bych viděl, že přes ni leží neprůhledná stránka.

## Utlumený šum

Řádek `TV_WEB_DARK_FRAME_SPIKE` byl v logu **595×** a přebil všechno
podstatné. Teď je jednou za pět vteřin.

## Zůstává z B64

Odchytávač pádů (zapíše výpis chyby přímo do souboru) a hlášení tichých
chyb OpenGL.

## Přeloženo

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang 18`, i s `--target=aarch64` | **0** |
| jádro PS1 pro ARM64 | `aarch64-linux-gnu` vč. assembleru | **0** |

## Co testovat

To co vždycky: BIOS bez disku a hru. A pošli log — tentokrát mi bude stačit
i těch pár řádků `PS1_STAV`.
