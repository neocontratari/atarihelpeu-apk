# B62 — OBRAZ NA OBRAZOVCE (versionCode 110)

## Proč byla v B61 černá i na výšku i na šířku

Moje chyba a našel jsem ji v kódu, ne odhadem. Konec `nap_gles_egl_init()`:

    eglMakeCurrent(display, surface, surface, context);   // kontext JADRA (pbuffer)

Když zobrazovací plocha zavolá napojení, jádro si **na jejím vlákně přepne
kontext na svůj vlastní neviditelný pbuffer**. Plocha od té chvíle kreslila
do něj místo na displej. Proto černo — a proto i na šířku, kde je stránka
průhledná a mělo to jít.

Oprava: plocha si hned po napojení vrátí svůj kontext a svůj povrch.

## Druhá věc: na výšku plocha nemohla být vidět nikdy

Grafika konzole `ps1_final_screen.png` je **úplně neprůhledná** (změřeno:
průměrná průhlednost 255 v celé ploše i v okénku obrazovky). Plocha pod
stránkou tedy na výšku prosvítat nemůže.

Řešení: stránka teď **hlásí, kde přesně má obraz být**, a plocha se tam
postaví:

- **na výšku** → plocha je NAD stránkou, přesně v okénku konzole
- **na šířku** → plocha je POD stránkou (ta je průhledná) přes celou obrazovku,
  takže ovladač zůstane nad obrazem a je vidět

## JPEG ve stránce taky pryč

Ve stránce zůstávalo volání `ps1FramePreviewB64()` každých 80 ms. Smazané —
teď se tam jen jednou za půl vteřiny ohlásí obdélník. Obraz kreslí nativní
plocha přímo ze sdílené textury jádra.

## Co uvidíš v logu

    PS1_OBRAZ_PRIMA_CESTA plocha kresli sdilenou texturu jadra
    PS1_OBRAZ_MISTO 12,340,916,428,false
    PS1_OBRAZ_UMISTEN 12,340 916x428 (na vysku, nad strankou)
    prvni snimek PRIMOU cestou 640x480

## Ověřeno

- `nap_ps1_native.cpp` se u mě přeloží (0 chyb)
- závorky sedí ve všech změněných souborech včetně stránky
- ve stránce nezůstal ani jeden odkaz na JPEG ani base64

Co ověřit nedokážu: NDK překlad a chování Mali.

## Co testovat

1. **BIOS bez disku na výšku** — obraz v okénku konzole.
2. **Hra na šířku** — obraz přes obrazovku, ovladač nad ním.
3. Plynulost obrazu i zvuku.
