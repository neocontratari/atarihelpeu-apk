# KROK F — obraz na TV přímo z jádra + automatický poměr stran

## 1) Zrušil jsem to překážející tlačítko

Omlouvám se — dal jsem ho přesně na tvoje vyskočení z PS1. Je pryč.

Poměr stran se teď přepíná **sám podle otočení telefonu**, přesně jak
jsi navrhl: **na výšku 4:3**, **na šířku 16:9**. Nic neklikáš.

## 2) Cast bere obraz PŘÍMO Z JÁDRA (tvůj nápad)

Dřív to šlo takhle: hra 640×480 → roztažení na displej telefonu
(s černými pruhy a ovládacími prvky) → **vyfotit celou obrazovku** →
přepočítat → H264 → TV. Dvojí zvětšení = měkký obraz.

Teď: framebuffer **rovnou z jádra** → **jedno** čisté roztažení na
plných 16:9 → H264 → TV.

Co to znamená:
- ostřejší obraz (jen jedno zvětšení místo dvou)
- **bez černých pruhů** — hra vyplní celou plochu 16:9
- **bez ovládacích prvků** na TV (joystick a tlačítka zůstanou jen na
  mobilu, kam patří)
- rychlejší — odpadá fotografování obrazovky, takže by měly stoupnout
  i FPS a zmenšit se zpoždění zvuku

## Verze: EMU10-F-TV-PRIMO-Z-JADRA

## Co mi pošli

1. **Jde teď vyskočit z PS1** zpátky do aplikace? (tlačítko je volné)
2. Je obraz na TV **ostřejší** než minule?
3. Zmizely černé pruhy a ovládací prvky z TV?
4. Kolik FPS ukazuje cast dole v liště?
5. Je zvuk blíž obrazu, nebo pořád stejně mimo?
6. Otoč mobil na výšku a na šířku — mění se poměr sám?
