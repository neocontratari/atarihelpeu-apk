# B82 — TV NEKRADE PROCESOR + VRSTVA OD ZAČÁTKU (versionCode 130)

> **Tento kód je předpoklad. Čeká se na test na telefonu a na log.**
> Ověřeno: překlad Javy proti `android.jar`, překlad C++ přes clang
> pro aarch64.

## 1. Zvuk se kouše jen se zapnutou TV — tvůj nález

Příčina je na jednom řádku, konec smyčky snímání pro TV:

```java
long effectiveDelay = Math.max(10, napTvWebFrameDelayMs);
if (!napTvWebH264ClientQueues.isEmpty()) {
    effectiveDelay = napTvWebH264FastTickMs;   // = 5 ms
}
```

Po připojení TV se smyčka přepne z 10 ms na **5 ms** — dvě stě průchodů za
vteřinu, přestože jádro vyrobí nejvýš šedesát snímků. Každý průchod snímá,
ořezává černé okraje a krmí enkodér. Devět z deseti je zbytečných a bere to
procesor emulaci → zvuk podtéká. Bez TV se smyčka nepřepne a zvuk je čistý.

V logu to bylo celou dobu: `TV_WEB_TICK_AVG avgTickGapMs=35 targetDelayMs=5`.

**Teď je tam 16 ms** (~60 za vteřinu). Nesnižovat.

## 2. Obraz na výšku — měl jsi pravdu i tady

Prošel jsem si to řádek po řádku, jak se to opravdu vykoná:

1. start BIOSu → plocha vznikne s vrstvou **dole** → na výšku ji zakryje
   neprůhledná grafika konzole
2. do 400 ms stránka nahlásí obdélník → plocha se postaví znovu **nahoru**
   → obraz naskočí

Takže by naskočil, ale až po necelé vteřině černa. To je pryč:

- výchozí vrstva je **nahoře** (PS1 se spouští na výšku), takže se
  nepřestavuje vůbec
- plocha se vytvoří **neviditelná** a ukáže se, až se ví, kam patří —
  jinak by přes celou obrazovku překryla skříň konzole

---

## CO TESTOVAT

**1)** PS1 bez disku na **výšku** — hned od začátku
**2)** **LOAD GAME bez TV** — poslechni zvuk
**3)** **Zapni TV** a nech hru běžet

## CO OČEKÁVAT

| krok | správně | špatně |
|---|---|---|
| 1 | obraz v okénku konzole, **bez černého záblesku** a bez překrytí skříně | okénko černé / plocha přes celou obrazovku |
| 2 | zvuk čistý | |
| 3 | **zvuk se po zapnutí TV nezmění** | začne se kousat |

## CO POSLAT ZPĚT

Jednou větou u každého kroku. Z logu dva řádky:

    PLOCHA_UMISTENA        <- ma tam byt 39,182 642x533
    TV_WEB_TICK_AVG        <- avgTickGapMs kolem 16-20, ne 35
