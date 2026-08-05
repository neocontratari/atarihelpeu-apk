# B78 — PLOCHA V KLIDU (versionCode 126)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> Ověřeno: překlad Javy proti `android.jar`, překlad C++ přes clang
> pro aarch64. Chování na Mali ověřené není.

## Test 3 — po otočení zmizel obraz

V logu je to vidět jako mela vláken:

    PLOCHA_ZRUSENA -> PRIPRAVENA -> UKONCENA -> PRIPRAVENA -> VYTVORENA

a po posledním vytvoření už žádné `PRIPRAVENA` **není** — takže po otočení
nekreslilo nic. Příčina: při otočení přijdou `surfaceCreated`
i `surfaceChanged` těsně po sobě a **obojí** plochu znovu připojovalo.
Každé připojení zastavuje staré kreslicí vlákno a spouští nové.

`surfaceChanged` už plochu znovu nepřipojuje. A kreslicí vlákno se teď při
odpojení ukončí dřív, než se pokusí překlopit obraz — aby odpojení nikde
nezůstalo viset.

## Boot Sony lezl do celé aplikace

Plocha leží přes celou obrazovku, takže když jsi odešel z obrazovky PS1,
obraz prosvítal i pod ostatními stránkami.

Teď se plocha **schová**, jakmile adresa stránky neobsahuje `emu_ps1`,
a zase objeví, když se vrátíš. V logu `PLOCHA_SCHOVANA` / `PLOCHA_ZOBRAZENA`.

## Umisťování plochy zrušeno

Stránka měla hlásit, kde má obraz být — v logu to nebylo ani jednou, takže
to nefungovalo. Ale testy 1 a 2 vyšly i tak, protože plocha přes celou
obrazovku pod stránkou stačí v obou otočeních. Nechávám to takhle a to
umisťování jsem vypnul — méně věcí, které se můžou rozbít.

## Co ještě není

**Zvuk se kouše na začátku intra hry.** Nesahal jsem na to, aby se to
nemíchalo s opravou zobrazení. Intro je film (24bitový režim) a to je
nejdražší část — podívám se na to dalším krokem, až bude obraz v klidu.

**ISO CD** — pořád předává hru jako `/proc/self/fd/N` místo skutečného
souboru. Zapsáno jako další krok.

---

## CO TESTOVAT

**1)** PS1 bez disku na **výšku** — menu BIOSu
**2)** Otoč **na šířku**, pořád v BIOSu
**3)** **LOAD GAME z netu**, hru na výšku, pak otoč **na šířku**
**4)** Z hry se vrať šipkou zpět do **hlavní nabídky aplikace**
**5)** Koukni na **TV prohlížeč** u kroků 1 až 3

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz v okénku konzole, správně otočený | černo / vzhůru nohama |
| 2 | obraz přes obrazovku, ovladač nad ním | černo / jen ovladač |
| 3 | obraz na výšku **i po otočení na šířku** | po otočení jen ovladač |
| 4 | **žádný obraz PS1** nikde v aplikaci | prosvítá bootovací Sony |
| 5 | obraz roztažený, bez ovladače | ovladač na TV |

Zvuk zatím neřeš — vím, že se u intra kouše, je to na řadě.

## CO POSLAT ZPĚT

Očima jen správně / špatně podle tabulky. Z logu:

    PLOCHA_VYTVORENA
    PLOCHA PRIPRAVENA
    PLOCHA_ZMENENA
    PLOCHA: snimku=...        <- musi rust i PO otoceni
    PLOCHA_SCHOVANA / PLOCHA_ZOBRAZENA
