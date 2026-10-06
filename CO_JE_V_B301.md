# Co je v B301

Tvoje zpráva: „U emu C++ Atari web viewer – je tam špatně, je vidět
akorát to testování, a ne Atari emu na obrazovce TV nebo PC.“

## Atari 130XE (HELP) na TV / PC

- **Proč to tak bylo:** Atari 130XE pod tlačítkem HELP kreslí C++ přímo na
  displej, do vlastní plochy (SurfaceView). Přenos na TV/PC fotí okno appky
  (PixelCopy) a tu plochu nevidí. Na TV proto šla stránka s testy, která je
  v appce pod přístrojem.
- **Teď:** obraz jde na TV/PC **přímo z jádra**, stejně jako u PS1 a Segy.
  - Výřez je stejný jako na displeji telefonu (336 × 240 bodů Atari).
  - Na TV je obraz roztažený na výšku 720 se stejným poměrem stran jako
    na telefonu, po stranách černo: 887 × 720 uprostřed obrazu 1280 × 720.
  - Mírné vyhlazení při zvětšení: na TV je obraz čistší.
- **Zvuk Atari jde na TV/PC taky**, přesně to, co hraje telefon (44 100 Hz
  stereo). Když hraje Atari, PS1 do TV nemluví.
- LOG/CHYBA: na TV je stránka s logem. Po návratu je na TV zase Atari.
- POWER vypnuto: na TV je černá obrazovka.

## Ověřeno na počítači

- Skutečný `nap_atari_native.cpp` se skutečnými vlákny: 62 kontrol,
  0 chyb, nová kontrola B301:
  - obraz 672 × 240 (336 bodů Atari) ve formátu pro Android Bitmap,
  - modrá obrazovka BASICu má modrou barvu (pořadí R a B sedí),
  - malé pole pozná,
  - zvuk stereo jde.
- Simulace obrazu pro TV (titulka Ghostbusters): poměr stran a umístění
  odpovídají telefonu.
