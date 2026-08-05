# B69 — NÁVRAT NA B56 (versionCode 117)

## Co to je

**Přesný obsah commitu `b893018 emu10_B56_ZPET_NA_B53_A_UKLID`** z tvého
repozitáře — tedy poslední build, u kterého jsi napsal „funguje to jak
před tím". Jediné, co jsem změnil, je číslo verze, aby se dal nainstalovat
přes ten rozbitý.

Ověřeno: rozdíl proti tvému B56 je **jediný soubor — `app/build.gradle`**,
a v něm jen řádek s verzí.

## Co je tím pádem pryč

Všechno, co jsem od B57 dál dělal se zobrazováním: přepojení hry, přímá
cesta přes sdílenou texturu, umisťování plochy podle stránky, měření.
Nic z toho na tvém telefonu nefungovalo.

Zůstává, co fungovalo: renderer ve stavu B53, odstraněný vadný rámec,
evropský region, zavírání mechaniky a smazaná mrtvá zobrazovací cesta.

## Co v tom logu bylo

    CHYBA_OPENGL 0x506

To je `GL_INVALID_FRAMEBUFFER_OPERATION` — neplatný cíl kreslení. Plocha
kreslila do povrchu, který na tvém telefonu neplatí. Proto byla černá na
obou cestách a ani přepnutí na záložní nepomohlo.

## Ověřeno

| část | čím | chyb |
|---|---|---|
| Java | `javac` proti `android.jar` API 34 | **0** |
| C++ | `clang --target=aarch64` | **0** |
