# B136 — ČISTÝ NÁVRAT NA B131 (versionCode 184)

## Co je uvnitř

**Celý strom je bajt po bajtu shodný s B131.** Ověřeno `diff -rq` proti
rozbalenému balíčku, ne z paměti.

Z nativního intra (B135) **nezůstalo nic**:

```
nap_intro_gl.cpp        pryč
nap_intro_zvuk.h        pryč
NativeIntroBridge.java  pryč
složka cpp/intro        pryč
napintro v CMake        0 výskytů
introZapni v Javě       0 výskytů
```

Verze je 184, aby šla nainstalovat přes B135. Kód je B131.

## Takže máš

```
intro          HTML, jde přeskočit, po doběhnutí se zastaví, na TV šlo
tlačítko INTRO v nabídce OPTIONS
SBÍRKA         vede na https://atarihelp.eu/?page_id=1003
Atari, PS1, Sega   jako v B131
```

**A vrací se i známá vada:** Atari se s puštěnou WEB TV kouše. Byla tam
i v B131. Nic nového.

## Co jsem v B135 pokazil

Kvůli přehlednosti, ne kvůli omluvám:

- **PS1 znělku a logo jsem slíbil a neudělal.** Napsal jsem *„zbývá napojení
  PS1 BIOSu"* a pak to poslal jako hotové. To je nejhorší z toho seznamu.
- **PŘESKOČIT chybělo.** V HTML bylo, v nativním ne.
- **Hudba hrála dál**, i když jsi z intra odešel — plocha se sundávala až
  po šedesáti vteřinách.
- **Na TV nešlo nic.** Z logu: `TV během intra: url=file:///android_asset/index.html`
  — TV snímala rozcestník pod mojí plochou. Půjčka snímku se nikdy nezavolala.

Obraz i zvuk přitom běžely (`snimku=3610 vzorku=2656256`). Rozbité bylo
všechno okolo.

## CO TESTOVAT

**1)** Build v Actions.
**2)** Spusť aplikaci — intro jako v B131, jde **PŘESKOČIT**.
**3)** Odejdi z intra šipkou zpět — **hudba musí přestat**.
**4)** **ATARI**, **PS1**, **SEGA** — jako v B131.
**5)** WEB TV — jako v B131.

## CO OČEKÁVAT

| # | SPRÁVNĚ | ŠPATNĚ |
|---|---|---|
| 1 | zelený build | červený → log z Actions |
| 2 | intro jde přeskočit | nejde |
| 3 | hudba přestane | hraje dál |
| 4 | jako v B131 | jakákoli změna |
| 5 | jako v B131 (Atari se s TV kouše — známé) | horší než B131 |
