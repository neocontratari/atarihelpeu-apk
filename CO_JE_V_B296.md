# Co je v B296

Tvá slova po B295: „super – ale chci, Partaku, zaraz otestovat i VBXE – prosím
udělej to – těším se na W3D :-) a Popeye :-)“

## VBXE v C++ jádře

VBXE (VideoBoard XE, jádro FX 1.26) je teď součástí C++ jádra 130XE – karta je
„zasunutá“ pořád, hry si ji najdou samy (stejně jako na skutečném 130XE s VBXE):

- registry na **$D640** (verze FX 1.26), **512 kB vlastní paměti** (VRAM),
- okna **MEMAC A / MEMAC B** – procesor a ANTIC vidí kus VRAM v paměti Atari
  (ROM má přednost, pak VBXE, pak rozšířená paměť 130XE),
- **XDL** (rozšířený display list) – po řádcích zapíná **overlay**:
  LR 160 bodů, SR 320 bodů, HR 640 bodů / 16 barev, text 80 sloupců,
- **atributová mapa** (barvy, paleta a priorita po buňkách),
- **4 palety po 256 barvách** (paleta 0 = původní barvy Atari),
- priority overlay vs. hrací pole a hráči, kolize,
- **blitter** (kopírování / vyplňování / sčítání … bloků ve VRAM, zoom, vzor,
  kolize, rychlost podle skutečného počtu paměťových cyklů) + IRQ po dokončení.

Obraz z VBXE má **4 body na barevný takt** (640 bodů na šířku) – celý obraz
Atari je proto teď 768 bodů široký (hry bez VBXE mají každý bod dvakrát,
vypadají stejně jako dřív).

## Joystick – bez něj se W3D ani Popeye hrát nedají

V HELP (C++) dosud **nebyl joystick vůbec**. Wolfenstein 3D i Popeye se ovládají
joystickem, takže jsem ho přidal:

- **obrazovka Atari = joystick 1**: prst na **LEVOU půlku** obrazovky a posouvat
  (nahoru = dopředu, do stran, i šikmo), **PRAVÁ půlka = FIRE**. Jde to oběma
  palci najednou. Vzhled přístroje se nemění, pod přístrojem se jednou ukáže
  „JOYSTICK: LEVO = SMER, VPRAVO = FIRE“.
- **herní ovladač** (bluetooth / USB): kříž nebo páčka = směr, A/B = FIRE,
  **X = mezera** (Wolfenstein: otevřít dveře), Y = RETURN, START/SELECT jako
  konzole, L2/R2 = OPTION.

## Ověřeno na počítači (obrázky jsem kontroloval já)

- **Popeye (VBXE, PAL)**: „VBXE found at $D640, FX core version 1.26“ → klávesa →
  Loading… (~20 s, rozbalení grafiky do VRAM) → titulka POPEYE → START → menu →
  FIRE → GET READY, ROUND 1 → hra (Popeye, Olive, Brutus, schody).
- **Wolfenstein 3D (wolf3d.atr)**: titulka → menu NEW GAME → epizoda → obtížnost →
  hra (FLOOR 1, LIVES 3, HEALTH 100 %, AMMO 8); joystickem chodím a otáčím se,
  FIRE střílí (AMMO 8 → 7), **mezera otevře dveře**.
- **Night Driver VBXE**: dvě titulky (klávesa) → START → jízda (VBXE kreslí auto).
- **Robots Rumble**: titulka „USE KEYBOARD OR JOY 1“.
- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: **29 kontrol, 0 chyb**
  (mimo jiné joystick dotykem, ovladač, Popeye a W3D až do hry).

## Co jsem cestou našel a opravil

1. **Priority GTIA – vícebarevní hráči**: jádro bralo jako „vícebarevné hráče“
   bit 4 registru PRIOR (to je pátý hráč), správně je to bit 5. Donkey Kong Jr.
   je teď **bod po bodu shodný** s emulátorem atari800 (dřív 4 body rozdíl).
2. **Zavaděč XEX**: když INIT čekal na klávesu déle než 10 s (titulka „stiskni
   klávesu“ – Night Driver), zavaděč přestal nahrávat a hra zůstala stát. Teď
   po návratu z INIT nahraje zbytek.

## Na co dát pozor

- Rychlost: hry s VBXE běží na počítači ~310 snímků/s (bez VBXE ~420). Telefon
  je pomalejší – kdyby W3D nebo Popeye zadrhávaly, pošli log.
- Netestováno (žádná hra z test_assets to nepoužívá): text 80 sloupců, IRQ
  blitteru, změny palety VBXE uprostřed řádku (kreslí se po řádcích).
- Testy pro tebe jsou na stránce **LOG/CHYBA** (11 kroků).
